/*
 * listener.cpp
 *
 *  Created on: 7 dec 2013
 *      Author: Tomas Stenlund
 */

#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

/*
 * STL
 */
#include <cstring>
#include <string>
#include <utility>
#include <thread>
#include <condition_variable>
#include <memory>
#include <algorithm>

/*
 * Casual common
 */
#include "common/logger.h"
#include "common/marshal.h"
#include "gateway/wire.h"
#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/listener.h"

/*
 * Casual namespace
 */
namespace casual
{
   namespace gateway
   {

      /**********************************************************************\
       *  BaseHandler
      \**********************************************************************/

      BaseHandler::BaseHandler ()
      {
         /*
          * Allocate the read and write buffers
          */
         buffer = std::unique_ptr<char[]>(new char[maxBufferSize]);
         writeBuffer = std::unique_ptr<char[]>(new char[maxWriteBufferSize]);
         writeBuffer_size = buffer_size = position = writePosition = message_size = message_read = 0;
         possibleToWrite = false;
         state = wait_for_header;
      }

      BaseHandler::~BaseHandler()
      {
      }

      /*
       * Types this handler handles
       */
      int BaseHandler::events() const
      {
         /* We only handle normal priority data */
         return POLLRDNORM | POLLWRNORM;
      }

      /*
       * Reads the message. Returns true if the message is completely read, false otherwise.
       */
      bool BaseHandler::readMessage ()
      {
         bool complete = false;
         int byte_to_read;

         /* Determine the number of bytes to copy from the read buffer */
         if (buffer_size - position < message_size - message_read)
            byte_to_read = buffer_size - position;
         else
            byte_to_read = message_size - message_read;

         /* Copy the required number of bytes */
         memcpy (&message[message_read], &buffer[position], byte_to_read);

         /* Move forward the buffer pointers */
         position+=byte_to_read;
         message_read+=byte_to_read;

         /* Move the buffer, if needed, i.e. if we got any more data in the buffer for the next sweep */
         /* THIS SHOULD BE OPTIMIZED, RINGBUFFER maybe instead :-) */
         if (position < buffer_size) {
            memcpy (&buffer[0], &buffer[position], buffer_size - position);
            buffer_size = buffer_size - position;
            position = 0;
         }

         /* Are we complete ? */
         if (message_size == message_read)
            complete = true;

         return complete;
      }

      /*
       * Header is 'C', 'A', 'S', 'L', 4 bytes length, i.e. total 8 bytes. Returns true if the header was located
       * and false otherwise.
       */
      bool BaseHandler::locateHeader ()
      {
         bool found = false;

         /* We need at least 12 characters to locate the message header, loop until we find it. It is called message
          * synchronization. Try to locate the magic string "CASL" followed by message size */
         while (buffer_size - position >= 12 && !found) {

            /* Locate the header magic number */
            if(buffer[position] == 'C') {
               if(buffer[position+1] == 'A') {
                  if (buffer[position+2] == 'S') {
                     if (buffer[position+3] == 'L') {

                        /* Header found, get the length of the message */
                        found = true;
                        message_size = casual::common::marshal::__htonll (*(reinterpret_cast<uint64_t *>(&buffer[position+4])));
                        position +=12;
                        if (message.size()>message_size)
                           message.resize (message_size);
                        message_read = 0;

                     } else {
                        position +=3;
                     }
                  } else {
                     position +=2;
                  }
               } else {
                  position+=1;
               }
            } else {
               position+=1;
            }

         }

         /* Fix the databuffer if we did not find it and reached the end of the buffer. */
         if (!found) {
            memcpy (&buffer[0], &buffer[position], buffer_size - position);
            buffer_size = buffer_size - position;
            position = 0;
         }

         /* Return with status */
         return found;
      }

      /*
       * Gets called whenever there is data to be read from the socket.
       */
      int BaseHandler::dataCanBeRead (int events, common::ipc::Socket &socket)
      {
         /* Read as much data as possible, i.e. try always to fill up the buffer */
         int numberOfBytes = 0;
         if (maxBufferSize - buffer_size > 0) {
            int numberOfReadBytes = socket.read(&buffer[buffer_size], maxBufferSize-buffer_size);
            buffer_size += numberOfReadBytes;

         /* Runt the statemachine if we did not get any errors */
         if (numberOfReadBytes >= 0 ) {

            /* Are we in header synch state */
            if (state == wait_for_header) {

               /* Locate the header in the data stream */
               common::logger::information << "BASEHANDLER::read : Trying to locate header 'CASL' - Buffer size = " << buffer_size << " position = " << position;
               if (locateHeader ())
               {
                  common::logger::information << "BASEHANDLER::read : Header located, message size = " << message_size;

                  /* Change state */
                  state = read_message;
               }
            }

            /* Are we in message read state */
            if (state == read_message) {

               common::logger::information << "BASEHANDLER::read : Reading message buffer size = " << buffer_size << " position " << position;
               common::logger::information << "BASEHANDLER::read : Reading message current message size " << message_read << " need " << message_size;
               if (readMessage ()) {
                  common::logger::information << "BASEHANDLER::read : Message is completed, message size = " << message_size;

                  /* Change state */
                  state = message_complete;
               } else {
                  common::logger::information << "BASEHANDLER::read : Message is not complete, message_size = " << message_size << " message_read = " << message_read;
               }

            }

            /* Are we in completed message state */
            if (state == message_complete) {

            }
               common::logger::information << "BASEHANDLER::read : Handle message, message size = " << message_size;
               handleMessage ();

               /* Change state */
               state = wait_for_header;
            }

         } else {

            /*
             * Handle the error cases
             */
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
               if (errno == EINTR) {

                  /* Inerrupted */
                  common::logger::information << "BASEHANDLER::read : Interrupted";

               } else {
                  common::logger::warning << "BASEHANDLER::read : Error during socket read " << errno << "=>" << strerror (errno);
               }
            } else {
               common::logger::information << "BASEHANDLER::read : Would block " << errno << "=>" << strerror (errno);
            }
         }

         /* If we have reached the end of the buffer, clear the position and buffer_size */
         if (position == buffer_size) {
            position = buffer_size = 0;
         }

         /* Return with handled event */
         return POLLRDNORM;
      }

      /*
       * Move data from the list of messages to the write buffer. Return true if any data was moved, false
       * otherwise.
       */
      bool BaseHandler::fillWriteBuffer ()
      {
         int numberOfBytes, numberOfBytesToMove;
         const void *pData;
         bool bDataMoved = false;

         /*
          * Fill up the write bufferwith all messages waiting in the queue until all messages are written or
          * the buffer is full.
          */
         while (writeBuffer_size < maxWriteBufferSize && listOfMessages.size()>0) {

            /*
             * First message always
             */
            numberOfBytes = listOfMessages.front()->getSize();
            pData = listOfMessages.front()->getBuffer();

            /*
             * Calculate the number of bytes we can take from the message
             */
            numberOfBytesToMove = maxWriteBufferSize - writeBuffer_size;
            if (numberOfBytesToMove > numberOfBytes)
               numberOfBytesToMove = numberOfBytes;

            /*
             * Move the data, size and cursor of the message
             */
            bDataMoved = true;
            memcpy (&writeBuffer[writeBuffer_size], pData, numberOfBytesToMove);
            writeBuffer_size += numberOfBytesToMove;
            listOfMessages.front()->moveCursor (numberOfBytesToMove);

            /*
             * If no more data in the message. Get rid of it from the list of messages to send.
             */
            listOfMessages.front()->moveCursor (numberOfBytesToMove);
            if (listOfMessages.front()->getSize()==0) {
               listOfMessages.pop_front();
            }

            /*
             * Do it again if the write buffer is not full and we have more messages
             */
         }

         /*
          * All is well
          */
         return bDataMoved;
      }

      /*
       * Fills the buffer and then writes it to the socket until there is no more data in the buffer or the
       * message list or if the socket is full. Returns true if any data was written, false otherwise.
       */
      bool BaseHandler::writeAll(common::ipc::Socket &socket)
      {
         bool bWrite = false;

         /* Fill up the buffer */
         if (possibleToWrite)
            fillWriteBuffer();

         /* Only if we can write to the socket */
         while (possibleToWrite && writeBuffer_size-writePosition>0) {

            /* Write to the socket */
            common::logger::information << "BASEHANDLER::write : Write buffer size=" << writeBuffer_size << " position=" << writePosition << " towrite=" << writeBuffer_size - writePosition;
            int bytesWritten = socket.write(&writeBuffer[writePosition], writeBuffer_size-writePosition);
            common::logger::information << "BASEHANDLER::write : Wrote size=" << bytesWritten;
            if (bytesWritten>=0) {

               /* Data was written */
               bWrite = true;

               /* Did we write all? If not the socket is probably full */
               if (writeBuffer_size-writePosition != bytesWritten)
                  possibleToWrite = false;

               /* Advance the position */
               writePosition += bytesWritten;

               /* Are we at the end ? If so we need to fille it up with more data if we got any */
               if (writePosition == writeBuffer_size) {
                  writePosition = writeBuffer_size = 0;
               }

            } else {

               /*
                * Handle the error cases
                */
               if (errno != EAGAIN && errno != EWOULDBLOCK) {
                  if (errno == EINTR) {

                     /* Interrupted */
                     common::logger::information << "BASEHANDLER::write : Interrupted";

                  } else {
                     common::logger::warning << "BASEHANDLER::write : Error during socket write " << errno << "=>" << strerror (errno);
                  }
               } else {
                  possibleToWrite = false;
                  common::logger::information << "BASEHANDLER::write : Would block " << errno << "=>" << strerror (errno);
               }
            }

            /* Try to fill the buffer */
            if (possibleToWrite)
               fillWriteBuffer();
         }

         /* Return true if we wrote anything out through the socket */
         return bWrite;
      }

      /*
       * Gets called whenever data can be written to the buffer
       */
      int BaseHandler::dataCanBeWritten (int events, common::ipc::Socket &socket)
      {
         common::logger::information << "BASEHANDLER::write : Data can be written";

         possibleToWrite = true;

         /*
          * Write data if we got any
          */
         writeAll(socket);
      }

      /*
       * Write message
       */
      bool BaseHandler::writeMessage (common::binary_type &message)
      {
         common::logger::information << "BASEHANDLER::write : Write a message";

         /*
          * Add the message to the list
          */
         std::unique_ptr<Buffer> local = std::make_unique<Buffer>(message);
         listOfMessages.push_back(std::move (local));

         /*
          * Write everything, hmm, how to handle this?
          */
         /* writeAll(); */

      }

      /**********************************************************************\
       *  BaseHandler::Buffer
      \**********************************************************************/
      BaseHandler::Buffer::Buffer (common::binary_type &message)
      {
         int header = 0x4341534c;
         buffer << header;
         buffer << message;
         position = 0;
      }

      /*
       * Return the buffer for its current location
       */
      const void *BaseHandler::Buffer::getBuffer()
      {
         return static_cast<const void *>(&buffer.get()[position]);
      }

      /*
       * Returns with the current size of the buffer
       */
      int BaseHandler::Buffer::getSize()
      {
         return buffer.get().size() - position;
      }

      /*
       * Moves the cursor
       */
      void BaseHandler::Buffer::moveCursor(int move)
      {
         position += move;
      }
   }
}



