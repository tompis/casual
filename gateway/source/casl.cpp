/*
 * casl.cpp
 *
 *  Created on: 5 jan 2014
 *      Author: tomas
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
#include <thread>

/*
 * Casual common
 */
#include "common/logger.h"
#include "common/types.h"
#include "gateway/std14.h"
#include "gateway/wire.h"
#include "gateway/ipc.h"
#include "gateway/casl.h"

/*
 * Casual namespace
 */
namespace casual
{
   namespace gateway
   {
      /**********************************************************************\
       *  CASLSocket
      \**********************************************************************/

      /*
       * Creates a casl socket based on an endpoint that it connects to
       */
      CASLSocket::CASLSocket (common::ipc::Endpoint &p) : common::ipc::Socket (p)
      {
         writeBuffer = std::make_unique<char[]>(CASLSocket::maxWriteBufferSize);
         buffer = std::make_unique<char[]>(CASLSocket::maxBufferSize);
      }

      /*
       * Shuts down a connect handler
       */
      CASLSocket::~CASLSocket()
      {
      }

      /*
       * Called when the listening state has an incoming connection that we need to accept.
       * This is not a scenario in the client socket.
       */
      common::ipc::Socket::State CASLSocket::handleIncomingConnection (int events)
      {
         common::logger::warning << "CASLSocket::handleIncomingConnection : Should never get an event here";
         return common::ipc::Socket::State::error;
      }

      /*
       * Handle events when we are in connected state, usually reading and writing data to and from the socket.
       */
      common::ipc::Socket::State CASLSocket::handleConnected (int events)
      {
         common::ipc::Socket::State ret = state;

         common::logger::information << "CASLSocket::handleConnected : Entered";

         /* POLLERR */
         if ((events & POLLERR) != 0) {

            int err = getLastError();
            common::logger::information << "CASLSocket::handleConnected : Error " << strerror (err) << "(" << err << ")";

            /* Socket is in error and so is the thread */
            ret = common::ipc::Socket::State::error;
         }

         /* HANGUP */
         if ((events & POLLHUP) != 0) {

            common::logger::information << "CASLSocket::handleConnected : Hangup";

            /* The other side has hung up */
            ret = common::ipc::Socket::State::hung_up;
         }

         /* POLLWRNORM */
         if ((events & POLLWRNORM) != 0) {
            ret = dataCanBeWritten ();
         }

         /* POLLRDNORM */
         if ((events & POLLRDNORM) != 0) {
            ret = dataCanBeRead ();
         }

         common::logger::information << "CASLSocket::handleConnected : Exited";
         return ret;
      }

      /*
       * Called when the connecting state has an outgoing connect and we have an incoming accept.
       */
      common::ipc::Socket::State CASLSocket::handleOutgoingConnection (int events)
      {
         common::ipc::Socket::State  ret = state;
         common::logger::information << "CASLSocket::handleOutgoingConnection : Entered";

         /* POLLERR */
         if ((events & POLLERR) != 0) {
            int err = getLastError();
            common::logger::information << "CASLSocket::handleOutgoingConnection : Error " << strerror (err) << "(" << err << ")";
            ret = common::ipc::Socket::State::error;
         }

         /* POLLHUP */
         if ((events & POLLHUP) != 0) {

            /* The other side has hung up, retry connection */
            common::logger::information << "CASLSocket::handleOutgoingConnection : Hangup on far end, try again";
            ret = common::ipc::Socket::State::hung_up;
         }

         /* POLLWRNORM */
         if ((events & POLLWRNORM) != 0) {

            common::logger::information << "CASLSocket::handleOutgoingConnection : Connected to far end";

            /* It is now possible to write data */
            possibleToWrite = true;

            /* We only need to poll for read for now, we got the ok for writing */
            setEventMask (POLLRDNORM);

            /*
            message::Registration registrationMessage;
            registrationMessage.type = message::type_registration;
            registrationMessage.from = m_state.m_global.server_uuid.string();
            registrationMessage.id = m_state.messageCounter++;
            registrationMessage.name = m_state.m_global.configuration.name;
            common::marshal::output::NWBOBinary nwbo;
            nwbo << registrationMessage;
            common::binary_type bt = nwbo.get();
            this->writeMessage(bt);
            */

            /* Change state */
            ret = common::ipc::Socket::State::connected;

         }

         common::logger::information << "CASLSocket::handleOutgoingConnection : Exited";
         return ret;

      }

      /*
       * Reads the message. Returns true if the message is completely read, false otherwise.
       */
      bool CASLSocket::readMessage ()
      {
         bool complete = false;
         int byte_to_read;

         common::logger::information << "CASLSocket::readMessage : Entered";

         /* Determine the number of bytes to copy from the read buffer */
         if (buffer_size - position < message_size - message_read)
            byte_to_read = buffer_size - position;
         else
            byte_to_read = message_size - message_read;

         common::logger::information << "ReadMessage : " << message_read << " " << position << " " << byte_to_read;

         /* Copy the required number of bytes */
         memcpy (&message[message_read], &buffer[position], byte_to_read);

         /* Move forward the buffer pointers */
         position+=byte_to_read;
         message_read+=byte_to_read;

         /* Move the buffer, if needed, i.e. if we got any more data in the buffer for the next sweep */
         /* THIS SHOULD BE OPTIMIZED, RINGBUFFER maybe instead :-) */
         if (position < buffer_size) {
            common::logger::information << "ReadMessage : " << position << " " << buffer_size << " " << buffer_size - position;
            memcpy (&buffer[0], &buffer[position], buffer_size - position);
            buffer_size = buffer_size - position;
            position = 0;
         }

         /* Are we complete ? */
         if (message_size == message_read)
            complete = true;

         common::logger::information << "CASLSocket::readMessage : Exited";

         return complete;
      }

      /*
       * Header is 'C', 'A', 'S', 'L', 4 bytes length, i.e. total 8 bytes. Returns true if the header was located
       * and false otherwise.
       */
      bool CASLSocket::locateHeader ()
      {
         bool found = false;

         common::logger::information << "CASLSocket::locateHeader : Entered";

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

         common::logger::information << "CASLSocket::locateHeader : Exited";

         /* Return with status */
         return found;
      }

      /*
       * Gets called whenever there is data to be read from the socket.
       */
      common::ipc::Socket::State CASLSocket::dataCanBeRead ()
      {
         common::ipc::Socket::State ret = state;
         bool stop = false;

         common::logger::information << "CASLSocket::dataCanBeRead : Entered";
         common::logger::information << "CASLSocket::dataCanBeRead : Can read " << maxBufferSize - buffer_size << " bytes";

         /* Read as much data as possible, i.e. try always to fill up the buffer */
         do {

            /* Read data if we have any left in the buffer */
            if (maxBufferSize - buffer_size > 0) {
               int numberOfReadBytes = read(&buffer[buffer_size], maxBufferSize-buffer_size);

               /* Any data or error ? */
               if (numberOfReadBytes<0) {

                  /* Error, so get out of the loop */
                  numberOfReadBytes = 0;
                  stop = true;

                  if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINPROGRESS) {
                     common::logger::information << "CASLSocket::dataCanBeRead : Error : " << strerror (errno) << "(" << errno << ")";
                     ret = common::ipc::Socket::State::error;
                  }

               } else {

                  /* No bytes, it means EOF and/or hangup on the other side */
                  if (numberOfReadBytes == 0) {
                     common::logger::information << "CASLSocket::dataCanBeRead : No data when reading, hangup on far end";
                     common::logger::information << "CASLSocket::dataCanBeRead : " << strerror (errno) << "(" << errno << ")";
                     stop = true;
                     ret = common::ipc::Socket::State::hung_up;
                  }

               }

               /* Add number of bytes read */
               buffer_size += numberOfReadBytes;
               common::logger::information << "CASLSocket::dataCanBeRead : Read " << numberOfReadBytes << " bytes";

               /* Run the read statemachine if we got any data */
               bool bAgain = true;
               while (buffer_size - position > 0 && bAgain) {

                  /* Not again, unless state changes */
                  bAgain = false;

                  /* Are we in header synch state */
                  if (state == wait_for_header) {

                     /* Locate the header in the data stream */
                     common::logger::information << "CASLSocket::dataCanBeRead : Trying to locate header 'CASL' - Buffer size = " << buffer_size << " position = " << position;
                     if (locateHeader ())
                     {
                        common::logger::information << "CASLSocket::dataCanBeRead : Header located, message size = " << message_size;

                        /* Resize the message */
                        if (message.size()<message_size)
                           message.resize(message_size);

                        /* Change state */
                        state = read_message;
                     }
                  }

                  /* Are we in message read state */
                  if (state == read_message) {

                     common::logger::information << "CASLSocket::dataCanBeRead : Reading message buffer size = " << buffer_size << " position " << position;
                     common::logger::information << "CASLSocket::dataCanBeRead : Reading message current message size " << message_read << " need " << message_size;
                     if (readMessage ()) {
                        common::logger::information << "CASLSocket::dataCanBeRead : Message is completed, message size = " << message_size;

                        /* Change state */
                        state = message_complete;

                     } else {
                        common::logger::information << "CASLSocket::dataCanBeRead : Message is not complete, message_size = " << message_size << " message_read = " << message_read;
                     }

                  }

                  /* Are we in completed message state */
                  if (state == message_complete) {

                     common::logger::information << "CASLSocket::dataCanBeRead : Handle message, message size = " << message_size;
                     handleMessage ();

                     /* Change state */
                     state = wait_for_header;
                     bAgain = true;
                  }

               }

               /* If we have reached the end of the buffer, clear the position and buffer_size so we can go around again */
               if (position == buffer_size) {
                  common::logger::information << "CASLSocket::dataCanBeRead : We have reached the end of the read buffer";
                  position = buffer_size = 0;
               }

            }

            /* Do it again unless we have reached the end */
         } while (!stop);

         common::logger::information << "CASLSocket::dataCanBeRead : Exited";

         /* Return with handled event, stay in the state */
         return ret;
      }

      /*
       * Move data from the list of messages to the write buffer. Return true if any data was moved, false
       * otherwise.
       */
      bool CASLSocket::fillWriteBuffer ()
      {
         int numberOfBytes, numberOfBytesToMove;
         const void *pData;
         bool bDataMoved = false;

         common::logger::information << "CASLSocket::fillWriteBuffer : Entered";

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
            if (listOfMessages.front()->getSize()==0) {
               listOfMessages.pop_front();
            }

            /*
             * Do it again if the write buffer is not full and we have more messages
             */
         }

         common::logger::information << "CASLSocket::fillWriteBuffer : Exited";

         /*
          * All is well
          */
         return bDataMoved;
      }

      /*
       * Fills the buffer and then writes it to the socket until there is no more data in the buffer or the
       * message list or if the socket is full. Returns true if any data was written, false otherwise.
       */
      common::ipc::Socket::State CASLSocket::writeAll()
      {
         common::ipc::Socket::State ret = state;
         bool bMovedData = true;
         common::logger::information << "CASLSocket::writeAll : Entered";

         /* Fill up the buffer */
         while (possibleToWrite && bMovedData) {

            /* Fill up the write buffers */
            bMovedData = fillWriteBuffer();

            /* Only if we can write to the socket */
            while (possibleToWrite && writeBuffer_size-writePosition>0) {

               /* Write to the socket */
               common::logger::information << "CASLSocket::writeAll : Write buffer size=" << writeBuffer_size << " position=" << writePosition << " towrite=" << writeBuffer_size - writePosition;
               int bytesWritten = write(&writeBuffer[writePosition], writeBuffer_size-writePosition);
               if (bytesWritten<0) {
                  common::logger::warning << "CASLSocket::writeAll : Error : " << errno << "=>" << strerror (errno);
                  possibleToWrite = false;

                  /* Normal error or not */
                  if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINPROGRESS) {
                     ret = common::ipc::Socket::State::error;
                  }

               }

               common::logger::information << "CASLSocket::writeAll : Wrote size=" << bytesWritten;
               if (bytesWritten>=0) {

                  /* Did we write all? If not the socket is probably full */
                  if (writeBuffer_size-writePosition != bytesWritten) {
                     possibleToWrite = false;
                  }

                  /* Advance the position */
                  writePosition += bytesWritten;

                  /* Are we at the end ? If so we need to fill it up with more data if we got any */
                  if (writePosition == writeBuffer_size) {
                     writePosition = writeBuffer_size = 0;
                  }

               }

            }

         }

         /* If we cannot write then TCP buffer is full so we need to poll for write as well as read
          * so we know when we can start again */
         if (!possibleToWrite) {
            setEventMask (POLLRDNORM | POLLWRNORM);
         }

         common::logger::information << "CASLSocket::writeAll : Exited";

         /* Return true if we wrote anything out through the socket */
         return ret;
      }

      /*
       * Gets called whenever data can be written to the buffer
       */
      common::ipc::Socket::State CASLSocket::dataCanBeWritten ()
      {
         common::logger::information << "CASLSocket::dataCanBeWritten : Entered";

         /* It is now possible to write data */
         possibleToWrite = true;

         /* We only need to poll for read, we got woken for a write */
         setEventMask (POLLRDNORM);

         /*
          * Write data if we got any
          */
         int ret = writeAll();

         common::logger::information << "CASLSocket::dataCanBeWritten : Exited";

         return ret;
      }

      /*
       * Write message
       */
      bool CASLSocket::writeMessage (common::binary_type &message)
      {
         common::logger::information << "CASLSocket::writeMessage : Entered";

         /*
          * If we are full, throw it away
          */
         if (listOfMessages.size()>=maxMessagesInWriteQueue) {
            common::logger::error << "CASLSocket::writeMessage : Too many messages pending, throwing them away";
         } else {

            /*
             * Add the message to the list
             */
            std::unique_ptr<Buffer> local = std::make_unique<Buffer>(message);
            listOfMessages.push_back(std::move (local));

         }

         /*
          * Write everything
          */
         state = writeAll();

         common::logger::information << "CASLSocket::writeMessage : Exited";

      }

      /*
       * Handle the message
       */
      bool CASLSocket::handleMessage()
      {
         common::logger::information << "CASLSocket::handleMessage : Incoming message arrived";
         return true;
      }

      /**********************************************************************\
       *  CASLSocket::Buffer
      \**********************************************************************/
      CASLSocket::Buffer::Buffer (common::binary_type &message)
      {
         int header = 0x4341534c;
         buffer << header;
         buffer << message;
         position = 0;
      }

      /*
       * Return the buffer for its current location
       */
      const void *CASLSocket::Buffer::getBuffer()
      {
         return static_cast<const void *>(&buffer.get()[position]);
      }

      /*
       * Returns with the current size of the buffer
       */
      int CASLSocket::Buffer::getSize()
      {
         return buffer.get().size() - position;
      }

      /*
       * Moves the cursor
       */
      void CASLSocket::Buffer::moveCursor(int move)
      {
         position += move;
      }

      /**********************************************************************\
       *  ClientThread
      \**********************************************************************/

      /*
       * Creates a client to a remote gateway based on an incoming connection from the remote gateway. The other
       * parameter, i,e. the socket is the listening socket that has the incoming socket. We need to accept the
       * connection. This client is not restartable, because the connection originates on the far end.
       */
      ClientThread::ClientThread (State &s, common::ipc::Socket *pSocket) : m_state (s)
      {
         /* we are initialized and can be run */
         m_state.state = ClientState::fatal;
         m_state.localRemoteName = "";
         m_state.localRemoteURL = "";
         bRestartable = false;

         /* Create an empty socket and accept the connection */
         m_state.socket = std::make_unique<CASLSocket>(m_state);
         int n = pSocket->accept(m_state.socket.get());
         if (n<0) {
            common::logger::warning << "ClientThread::ClientThread : Incoming connection not accepted";
         } else {
            common::logger::information << "ClientThread::ClientThread : Incoming connection accepted";
            m_state.state = ClientState::active;
         }

      }

      /*
       * Creates a client to a remote gateway based on configuration, this client reconnects in case of
       * a failure.
       */
      ClientThread::ClientThread (State &s, configuration::RemoteGateway &remote) : m_state (s)
      {
         /* We are in a starting state */
         m_state.state = ClientState::fatal;
         m_state.localRemoteName = remote.name;
         m_state.localRemoteURL = remote.endpoint;
         bRestartable = true;

         /* Resolve the bind address for the gateway */
         common::ipc::Resolver resolver;
         if (resolver.resolve (m_state.localRemoteURL)<0) {

            /* Unable to resolve address, there is no idea to start */
            common::logger::error << "ClientThread::ClientThread : Unable to resolve address " << remote.endpoint << " for " << m_state.localRemoteName;

         } else {

            /* Save the endpoint */
            m_state.endpoint = resolver.get().front();
            m_state.state = ClientState::initialized;
         }
      }

      /*
       * Destructor
       */
      ClientThread::~ClientThread ()
      {
         /* Stop the thread */
         if (thread != nullptr) {
            stop();
         }
      }

      /*
       * Starts the ClientThread. Returns true if the thread has been started.
       */
      bool ClientThread::start()
      {
         bool bStarted = false;
         common::logger::information << "ClientThread::start : Entered";

         /* Can we start and are we not already running ? */
         if (m_state.state!=ClientState::failed && thread == nullptr) {

            /* Allow it to run */
            bRun = true;
            bStarted = true;

            /* Start the thread */
            common::logger::information << "ClientThread::start : Thread started";
            thread = std::make_unique<std::thread>(&ClientThread::loop, this);

         }

         common::logger::information << "ClientThread::start : Exited";

         return bStarted;
      }

      /*
       * Stops the ClientThread. Returns true if the Client thread has been stopped.
       */
      bool ClientThread::stop()
      {
         bool bStopped = false;
         common::logger::information << "ClientThread::stop : Entered";

         /* Allow it to stop */
         bRun = false;

         /* Are we running ? */
         if (thread != nullptr) {

            /* Wait for the thread to finish */
            common::logger::information << "ClientThread::stop : Waiting for thread to finish";
            thread->join();
            thread = nullptr;
            bStopped = true;
         }

         common::logger::information << "ClientThread::stop : Exited";

         return bStopped;
      }

      /*
       * Determine if the thread has exited, either by stopping it or by an error condition.
       */
      bool ClientThread::hasExited()
      {
         return bExited;
      }

      /*
       * True if it has been started
       */
      bool ClientThread::hasStarted()
      {
         return bRun;
      }

      /*
       * True if it is restartable
       */
      bool ClientThread::isRestartable()
      {
         return bRestartable;
      }

      /*
       * The current state
       */
      ClientState::MachineState ClientThread::getMachineState()
      {
         return m_state.state;
      }

      /*
       * The name of the thread
       */
      std::string ClientThread::getName ()
      {
         return m_state.localRemoteName;
      }

      /*
       * Connects the client to the remote gateway
       */
      bool ClientThread::connect()
      {
         bool bOK = true;

         /* Create the socket */
         common::logger::information << "ClientThread::connect : Connecting to endpoint " << m_state.endpoint.info();
         m_state.socket = std::make_unique<CASLSocket>(m_state, m_state.endpoint);

         /* Connect to the remote endpoint */
         if (m_state.socket->connect()<0)
         {
            /* If we are in progres, then that is ok, otherwise we failed */
            bOK = false;
            common::logger::error << "ClientThread::connect : Unable to connect to " << m_state.endpoint.info() << ", fatal " << strerror (errno) << "(" << errno << ")";
         } else {
            common::logger::information << "ClientThread::connect : Connection to " << m_state.endpoint.info() << " established";
         }

         /* Return with the status */
         return bOK;
      }

      /*
       * The Client threads thread loop
       */
      void ClientThread::loop()
      {
         int status;

         common::logger::information << "ClientThread::loop : Entered";

         /* Connection loop */
         while (bRun && m_state.state != ClientState::failed && m_state.state != ClientState::fatal) {

            /* If we are not connected, try to connect */
            if (m_state.state == ClientState::initialized || m_state.state == ClientState::retry) {

               /* Wait if we are retrying */
               if (m_state.state == ClientState::retry) {
                  std::chrono::milliseconds dura( m_state.m_global.configuration.clientreconnecttime );
                  std::this_thread::sleep_for(dura);
               }

               /* Try to connect */
               if (connect())
                  m_state.state = ClientState::connecting;
               else
                  m_state.state = ClientState::fatal;
            }

            /* Execute the socket */
            status = m_state.socket->execute(m_state.m_global.configuration.clienttimeout);
            if (status < 0) {

               /* Some serious polling error, stop this */
               common::logger::information << "ClientThread::loop : Error during poll, " << strerror (errno) << "(" << errno << ")";
               m_state.state = ClientState::failed;

            } else {

               if (status == 0) {
                  /* Timeout */
               }
            }

         }

         /* Exit */
         bExited = true;
         common::logger::information << "ClientThread::loop : Exited";
      }
   }
}



