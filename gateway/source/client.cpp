/*
 * client.cpp
 *
 *  Created on: 16 dec 2013
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
#include <thread>

/*
 * Casual common
 */
#include "common/logger.h"
#include "common/marshal.h"
#include "gateway/wire.h"
#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/state.h"
#include "gateway/listener.h"
#include "gateway/master.h"
#include "gateway/client.h"
#include "gateway/messages.h"

/*
 * Casual namespace
 */
namespace casual
{
   namespace gateway
   {

      /**********************************************************************\
       *  ClientState
      \**********************************************************************/

      ClientState::ClientState(State &s): m_global (s)
      {

      }
      /**********************************************************************\
       *  ClientConnectHandler
      \**********************************************************************/

      /*
       * Creates a connecthandler
       */
      ClientConnectHandler::ClientConnectHandler (ClientState &s) : m_state (s)
      {
      }

      /*
       * Shuts down a connect handler
       */
      ClientConnectHandler::~ClientConnectHandler()
      {
      }

      /*
       * Types this handler handles, we are only interested in POLLOUT because it
       * signals that we are connected.
       */
      int ClientConnectHandler::events() const
      {
         return POLLOUT;
      }

      /*
       * Hangup handling, enter retry state.
       */
      int ClientConnectHandler::hangup (int events, common::ipc::Socket &socket)
      {
         common::logger::information << "ClientConnectHandler::hangup: Hung up";
         m_state.state = ClientState::retry;
         return POLLHUP;
      }

      /*
       * Error handling, enter retry state
       */
      int ClientConnectHandler::error (int events, common::ipc::Socket &socket)
      {
         common::logger::warning << "ClientConnectHandler::error: Error " << strerror (socket.getError()) << "(" << errno << ")";
         m_state.state = ClientState::retry;
         return POLLERR;
      }

      /*
       * Functions that gets called whenever an event occurs for the socket. In this case
       * I am only after that data can be written, because for a socket when connecting
       * this signals that a connection has been completed
       */
      int ClientConnectHandler::dataCanBeWritten (int events, common::ipc::Socket &socket)
      {
         common::logger::information << "ClientConnectHandler::dataCanBeWritten : Entered";
         common::ipc::dumpEvents(events);

         /* If we got connected, change the handler to a read/write handler */
         if (!m_state.state == ClientState::connecting) {

            common::logger::information << "ClientConnectHandler::dataCanBeWritten : Connected";

            /* Change handler to the real client handler */
            std::unique_ptr<common::ipc::SocketEventHandler> pRL;
            pRL.reset (new ClientHandler(m_state));
            m_state.socket->setEventHandler (pRL);

            /* Change to active state */
            m_state.state = ClientState::active;

         }
         common::logger::information << "ClientConnectHandler::dataCanBeWritten : Exited";
         return POLLOUT;
      }

      /**********************************************************************\
       *  ClientHandler
      \**********************************************************************/

      ClientHandler::ClientHandler (ClientState &s) : m_state (s)
      {
      }

      ClientHandler::~ClientHandler()
      {
      }

      /*
       * Handles all incoming messages
       */
      bool ClientHandler::handleMessage ()
      {
         common::logger::information << "ClientHandler::handleMessage : Message has arrived";
         return true;
      }

      /**********************************************************************\
       *  ClientThread
      \**********************************************************************/

      /*
       * Creates a client to a remote gateway based on an incoming connection from the remote gateway
       */
      ClientThread::ClientThread (State &s, std::unique_ptr<common::ipc::Socket> socket) : m_state (s)
      {
         /* we are initialized and can be run */
         m_state.state = ClientState::failed;

         /* Get the ownership of the socket */
         m_state.socket = std::move(socket);

         /* Set the handler directly to the client server handler */
         std::unique_ptr<common::ipc::SocketEventHandler> pRL;
         pRL.reset (new ClientHandler(m_state));
         m_state.socket->setEventHandler (pRL);

         /* We are ok */
         m_state.state = ClientState::active;

      }

      /*
       * Creates a client to a remote gateway based on configuration
       */
      ClientThread::ClientThread (State &s, configuration::RemoteGateway &remote) : m_state (s)
      {
         /* We are in a starting state */
         m_state.state = ClientState::failed;

         /* Resolve the bind address for the gateway */
         common::ipc::Resolver resolver;
         if (resolver.resolve (remote.endpoint)<0) {

            /* Unable to resolve address, there is no idea to start */
            common::logger::error << "ClientThread::ClientThread : Unable to resolve address " << remote.endpoint << " for " << remote.name;

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
            run = true;

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
         run = false;

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
       * Connects the client to the remote gateway
       */
      bool ClientThread::connect()
      {
         bool bOK = true;

         /* Create the socket */
         common::logger::information << "ClientThread::connect : Create the socket";
         m_state.socket = std::make_unique<common::ipc::Socket>(m_state.endpoint);

         /* Add the client connect handler to the socket */
         std::unique_ptr<common::ipc::SocketEventHandler> eH = std::make_unique<ClientConnectHandler>(m_state);
         m_state.socket->setEventHandler(eH);

         /* Connect to the remote endpoint */
         common::logger::information << "ClientThread::connect : Trying to connect the socket to the remote gateway";
         if (m_state.socket->connect()<0)
         {
            /* If we are in progres, then that is ok, otherwise we failed */
            if (errno != EINPROGRESS) {
               bOK = false;
               common::logger::error << "ClientThread::connect : Unable to connect the socket " << strerror (errno) << "(" << errno << ")";
            } else {
               common::logger::error << "ClientThread::connect : Connection in progress";
            }
         } else {
            common::logger::error << "ClientThread::connect : We got connected at once - Strange";
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

         /* Loop until we are shut down */
         while (run && m_state.state != ClientState::failed) {

            /* If we are not connected, try to connect */
            if (m_state.state == ClientState::initialized || m_state.state == ClientState::retry) {

               /* Wait if we are retrying */
               if (m_state.state == ClientState::retry) {
                  std::chrono::milliseconds dura( m_state.m_global.configuration.clientreconnecttime );
                  std::this_thread::sleep_for( dura );
               }

               /* Try to connect */
               if (connect())
                  m_state.state = ClientState::connecting;
               else
                  m_state.state = ClientState::failed;
            }

            /* Poll the group of sockets */
            status = m_state.socket->poll(m_state.m_global.configuration.clienttimeout);
            if (status < 0) {

               /* Some serious polling error, stop this */
               common::logger::information << "ClientThread::loop : Error on socket, " << strerror (errno) << "(" << errno << ")";
               m_state.state = ClientState::failed;
            } else {
               common::logger::information << "ClientThread::loop : Timeout";
            }

         }

         common::logger::information << "ClientThread::loop : Exited";
      }
   }
}


/*
else {

      common::logger::information << "ClientThread::ClientThread : Connection initiated " << remote.endpoint << " for " << remote.name;

      message::Registration registrationMessage;
      registrationMessage.header.from = m_state.m_global.server_uuid.string();
      registrationMessage.header.id = m_state.messageCounter++;
      registrationMessage.name = remote.name;
      common::marshal::output::NWBOBinary nwbo;
      nwbo << registrationMessage;
      BaseHandler *pB = static_cast<BaseHandler *>(m_state.socket->getEventHandler());
      common::binary_type b = nwbo.release();
      pB->writeMessage(b);
   }
 */
