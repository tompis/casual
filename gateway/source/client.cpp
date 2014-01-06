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
#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/state.h"
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

      /*
       * Returns with the state of the client as a string
       */
      std::string ClientState::getState ()
      {
         std::string s = "ClientState::unknown";

         switch (state) {
            case initialized:
               s = "ClientState::";
               break;
            case connecting:
               s = "ClientState::connecting";
               break;
            case retry:
               s = "ClientState::retry";
               break;
            case active:
               s = "ClientState::active";
               break;
            case failed:
               s = "ClientState::failed";
               break;
         }
         return s;
      }

      /**********************************************************************\
       *  ClientSocket
      \**********************************************************************/

      /*
       * Creates a client socket based on an endpoint that it connects to
       */
      ClientSocket::ClientSocket (ClientState &s, common::ipc::Endpoint &p) : CASLSocket (p), m_state (s)
      {
      }

      /*
       * Creates a client socket based on no endpoint.
       */
      ClientSocket::ClientSocket (ClientState &s) : CASLSocket (), m_state (s)
      {
      }

      /*
       * Shuts down a connect handler
       */
      ClientSocket::~ClientSocket()
      {
      }

      /*
       * Handle the message
       */
      bool ClientSocket::handleMessage()
      {
         common::logger::information << "ClientSocket::handleMessage : Incoming message arrived";
         return true;
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
         m_state.socket = std::make_unique<ClientSocket>(m_state);
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
         m_state.socket = std::make_unique<ClientSocket>(m_state, m_state.endpoint);

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
