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
         pollEvents = POLLRDNORM | POLLWRNORM;
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
         return pollEvents;
      }

      /*
       * Reads the message. Returns true if the message is completely read, false otherwise.
       */
      bool BaseHandler::readMessage ()
      {
         bool complete = false;
         int byte_to_read;

         common::logger::information << "BaseHandler::readMessage : Entered";

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

         common::logger::information << "BaseHandler::readMessage : Exited";

         return complete;
      }

      /*
       * Header is 'C', 'A', 'S', 'L', 4 bytes length, i.e. total 8 bytes. Returns true if the header was located
       * and false otherwise.
       */
      bool BaseHandler::locateHeader ()
      {
         bool found = false;

         common::logger::information << "BaseHandler::locateHeader : Entered";

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

         common::logger::information << "BaseHandler::locateHeader : Exited";

         /* Return with status */
         return found;
      }

      /*
       * Gets called whenever there is data to be read from the socket.
       */
      int BaseHandler::dataCanBeRead (int events, common::ipc::Socket &socket)
      {
         common::logger::information << "BaseHandler::dataCanBeRead : Entered";
         common::logger::information << "BaseHandler::dataCanBeRead : Can be read " << maxBufferSize - buffer_size;

         /* Read as much data as possible, i.e. try always to fill up the buffer */
         if (maxBufferSize - buffer_size > 0) {
            int numberOfReadBytes = socket.read(&buffer[buffer_size], maxBufferSize-buffer_size);
            if (numberOfReadBytes<0) {
               common::logger::information << "BaseHandler::dataCanBeRead : " << strerror (errno) << "(" << errno << ") - " << numberOfReadBytes;
               numberOfReadBytes = 0;
            }
            buffer_size += numberOfReadBytes;
            common::logger::information << "BaseHandler::dataCanBeRead : Read " << numberOfReadBytes;

            /* Runt the statemachine if we did not get any errors */
            if (numberOfReadBytes > 0 ) {

               /* Are we in header synch state */
               if (state == wait_for_header) {

                  /* Locate the header in the data stream */
                  common::logger::information << "BaseHandler::dataCanBeRead : Trying to locate header 'CASL' - Buffer size = " << buffer_size << " position = " << position;
                  if (locateHeader ())
                  {
                     common::logger::information << "BaseHandler::dataCanBeRead : Header located, message size = " << message_size;

                     /* Change state */
                     state = read_message;
                  }
               }

               /* Are we in message read state */
               if (state == read_message) {

                  common::logger::information << "BaseHandler::dataCanBeRead : Reading message buffer size = " << buffer_size << " position " << position;
                  common::logger::information << "BaseHandler::dataCanBeRead : Reading message current message size " << message_read << " need " << message_size;
                  if (readMessage ()) {
                     common::logger::information << "BaseHandler::dataCanBeRead : Message is completed, message size = " << message_size;

                     /* Change state */
                     state = message_complete;
                  } else {
                     common::logger::information << "BaseHandler::dataCanBeRead : Message is not complete, message_size = " << message_size << " message_read = " << message_read;
                  }

               }

               /* Are we in completed message state */
               if (state == message_complete) {

                  common::logger::information << "BaseHandler::dataCanBeRead : Handle message, message size = " << message_size;
                  handleMessage ();

                  /* Change state */
                  state = wait_for_header;
               }

            }

            /* If we have reached the end of the buffer, clear the position and buffer_size */
            if (position == buffer_size)
               position = buffer_size = 0;
         }

         common::logger::information << "BaseHandler::dataCanBeRead : Exited";

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

         common::logger::information << "BaseHandler::fillWriteBuffer : Entered";

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

         common::logger::information << "BaseHandler::fillWriteBuffer : Exited";

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

         common::logger::information << "BaseHandler::writeAll : Entered";

         /* Fill up the buffer */
         if (possibleToWrite)
            fillWriteBuffer();

         /* Only if we can write to the socket */
         while (possibleToWrite && writeBuffer_size-writePosition>0) {

            /* Write to the socket */
            common::logger::information << "BaseHandler::writeAll : Write buffer size=" << writeBuffer_size << " position=" << writePosition << " towrite=" << writeBuffer_size - writePosition;
            int bytesWritten = socket.write(&writeBuffer[writePosition], writeBuffer_size-writePosition);
            if (bytesWritten<0) {
               common::logger::warning << "BaseHandler::writeAll : Error during socket write " << errno << "=>" << strerror (errno);
               possibleToWrite = false;
            }

            common::logger::information << "BaseHandler::writeAll : Wrote size=" << bytesWritten;
            if (bytesWritten>0) {

               /* Data was written */
               bWrite = true;

               /* Did we write all? If not the socket is probably full */
               if (writeBuffer_size-writePosition != bytesWritten) {
                  possibleToWrite = false;
               }

               /* Advance the position */
               writePosition += bytesWritten;

               /* Are we at the end ? If so we need to fille it up with more data if we got any */
               if (writePosition == writeBuffer_size) {
                  writePosition = writeBuffer_size = 0;
               }

            }

            /* Try to fill the buffer */
            if (possibleToWrite)
               fillWriteBuffer();
         }

         /* If we cannot write, poll for write as we ll */
         if (!possibleToWrite)
            pollEvents = POLLRDNORM | POLLWRNORM;

         common::logger::information << "BaseHandler::writeAll : Exited";

         /* Return true if we wrote anything out through the socket */
         return bWrite;
      }

      /*
       * Gets called whenever data can be written to the buffer
       */
      int BaseHandler::dataCanBeWritten (int events, common::ipc::Socket &socket)
      {
         common::logger::information << "BaseHandler::dataCanBeWritten : Entered";
         common::ipc::dumpEvents(events);

         /* It is now possible to write data */
         possibleToWrite = true;

         /* We only need to poll for read */
         pollEvents = POLLRDNORM;

         /*
          * Write data if we got any
          */
         writeAll(socket);

         common::logger::information << "BaseHandler::dataCanBeWritten : Exited";

      }

      /*
       * Write message
       */
      bool BaseHandler::writeMessage (common::binary_type &message)
      {
         common::logger::information << "BaseHandler::writeMessage : Entered";

         /*
          * Add the message to the list
          */
         std::unique_ptr<Buffer> local = std::make_unique<Buffer>(message);
         listOfMessages.push_back(std::move (local));

         /*
          * Write everything, problem, need socket
          */
         /* writeAll(); */

         common::logger::information << "BaseHandler::writeMessage : Exited";

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



