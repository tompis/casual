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
       *  ListenerState
      \**********************************************************************/

      /**********************************************************************\
       *  ConnectHandler
      \**********************************************************************/

      ConnectHandler::ConnectHandler (GatewayState &ls) : state (ls)
      {
      }

      ConnectHandler::~ConnectHandler()
      {
      }

      /*
       * Types this handler handles
       */
      int ConnectHandler::events() const
      {
         return POLLIN;
      }

      int ConnectHandler::dataCanBeRead (int events, common::ipc::Socket &socket)
      {
         std::unique_ptr<common::ipc::Socket> pS;

         common::logger::information << "SERVER: Incoming connection";

         /* Accept the connection */
         pS = socket.accept();
         if (pS==0L) {
            common::logger::warning << "SERVER: Incoming connection not accepted";
         } else {
            common::logger::information << "SERVER: Incoming connection accepted";
         }

         /* Add an eventhandler to the socket */
         std::unique_ptr<common::ipc::SocketEventHandler> pRL;
         pRL.reset (new RegisterHandler(state));
         pS->setEventHandler (pRL);

         /* Add the socket to the group and continue with the business */
         std::shared_ptr<common::ipc::Socket> pP = std::shared_ptr<common::ipc::Socket>(pS.release());
         state.listOfAcceptedConnections.push_back (pP);

         /* Add the socket to the group that we are polling */
         state.socketGroupServer.addSocket(pP);
      }

      /**********************************************************************\
       *  Register
      \**********************************************************************/

      RegisterHandler::RegisterHandler (GatewayState &ls) : state (ls)
      {
      }

      RegisterHandler::~RegisterHandler()
      {
      }

      /*
       * Types this handler handles
       */
      int RegisterHandler::events() const
      {
         /* We only handle normal priority data */
         return POLLRDNORM | POLLWRNORM;
      }

      /*
       * Reads the message
       */
      bool RegisterHandler::readMessage ()
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
       * Header is 'C', 'A', 'S', 'L', 4 bytes length, i.e. total 8 bytes
       */
      bool RegisterHandler::locateHeader ()
      {
         bool found = false;

         /* We need at least 8 characters to locate the message, loop until we find it. It is called message
          * synchronization. Try to locate the magic string "CASL" */
         while (buffer_size - position >= 8 && !found) {

            /* Locate the header magic number */
            if(buffer[position] == 'C') {
               if(buffer[position+1] == 'A') {
                  if (buffer[position+2] == 'S') {
                     if (buffer[position+3] == 'L') {

                        /* Header found, get the length of the message */
                        found = true;
                        message_size = ntohl (*(reinterpret_cast<uint32_t *>(&buffer[position+4])));
                        position +=8;
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
      int RegisterHandler::dataCanBeRead (int events, common::ipc::Socket &socket)
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
               common::logger::information << "Trying to locate header 'CASL' - Buffer size = " << buffer_size << " position = " << position;
               if (locateHeader ())
               {
                  common::logger::information << "Header located, message size = " << message_size;

                  /* Change state */
                  state = read_message;
               }
            }

            /* Are we in message read state */
            if (state == read_message) {

               common::logger::information << "Reading message buffer size = " << buffer_size << " position " << position;
               common::logger::information << "Reading message current message size " << message_read << " need " << message_size;
               if (readMessage ()) {
                  common::logger::information << "Message is completed, message size = " << message_size;

                  /* Change state */
                  state = message_complete;
               } else {
                  common::logger::information << "Message is not complete, message_size = " << message_size << " message_read = " << message_read;
               }

            }

            /* Are we in completed message state */
            if (state == message_complete) {

            }
               common::logger::information << "Handle message, message size = " << message_size;
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
                  common::logger::information << "RegisterHandler : Interrupted";

               } else {
                  common::logger::warning << "RegisterHandler : Error during socket read " << errno << "=>" << strerror (errno);
               }
            } else {
               common::logger::information << "RegisterHandler : Would block " << errno << "=>" << strerror (errno);
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
       * Gets called whenever data can be written to the buffer
       */
      int RegisterHandler::dataCanBeWritten (int events, common::ipc::Socket &socket)
      {
      }

   }
}



