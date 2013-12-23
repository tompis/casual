/*
 * master.cpp
 *
 *  Created on: 15 dec 2013
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
#include "common/marshal.h"
#include "gateway/wire.h"
#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/state.h"
#include "gateway/listener.h"
#include "gateway/master.h"
#include "gateway/client.h"

/*
 * Casual namespace
 */
namespace casual
{
   namespace gateway
   {

      /**********************************************************************\
       *  MasterState
      \**********************************************************************/

      /*
       * State initialization
       */
      MasterState::MasterState(State &s): m_global (s)
      {
      }

      /**********************************************************************\
       *  MasterHandler
      \**********************************************************************/

      /*
       * Master connect handler initialization
       */
      MasterConnectHandler::MasterConnectHandler (MasterState &s) : m_state(s)
      {
      }

      /*
       * Master connect handler destruction
       */
      MasterConnectHandler::~MasterConnectHandler()
      {
      }

      /*
       * Types of events this handler handles
       */
      int MasterConnectHandler::events() const
      {
         return POLLIN; /* POLLIN gets signalled whener a socket gets connected */
      }

      /*
       * Data can be read, this is a socket in listening state and that means that there is an incoming
       * socket connection that has to be handled
       */
      int MasterConnectHandler::dataCanBeRead (int events, common::ipc::Socket &socket)
      {
         std::unique_ptr<common::ipc::Socket> pS;

         common::logger::information << "MasterHandler::dataCanBeRead : Entered";
         common::ipc::dumpEvents(events);

         /* Accept the connection */
         common::logger::information << "MasterHandler::dataCanBeRead : Trying to accept incoming connection";
         pS = socket.accept();
         if (pS==0L) {
            common::logger::warning << "MasterHandler::dataCanBeRead : Incoming connection not accepted";
         } else {
            common::logger::information << "MasterHandler::dataCanBeRead : Incoming connection accepted";

            /* Start a client thread */
            std::unique_ptr<ClientThread> clientThread = std::make_unique<ClientThread>(m_state.m_global, std::move(pS));
            clientThread->start();

            /* Add the thread to the master list */
            std::lock_guard<std::mutex> lock(m_state.m_global.listOfClientsMutex);
            m_state.m_global.listOfClients.push_back(std::move(clientThread));
         }

         common::logger::information << "MasterHandler::dataCanBeRead : Exited";

      }

      /**********************************************************************\
       *  MasterThread
      \**********************************************************************/

      /*
       * Create and initialize the master thread.
       */
      MasterThread::MasterThread (State &s) : m_state (s)
      {
         common::ipc::Resolver resolver;

         /* Resolve the bind address for the gateway */
         if (resolver.resolve (m_state.m_global.configuration.endpoint)<0) {

            /*
             * Unable to resolve the address, gateway cannot start
             */
            common::logger::error << "MasterThread::MasterThread : Unable to resolve address " << m_state.m_global.configuration.endpoint;
            m_state.bInitialized = false;

         } else {

            /*
             * Create the socket, we assume the first one is the one to use for now. If this fail we should take
             * the next in the list, but we do not do that just now. TODO!!!!
             */
            m_state.endpoint = resolver.get().front();
            m_state.socket = std::make_shared<common::ipc::Socket>(m_state.endpoint);

            /*
             * Add the socket handler and add it to the group we want to poll
             */
            m_state.socketGroupServer.addSocket(m_state.socket);
            std::unique_ptr<common::ipc::SocketEventHandler> eH = std::make_unique<MasterConnectHandler>(m_state);
            m_state.socket->setEventHandler(eH);

            /*
             * All is well
             */
            m_state.bInitialized = true;

         }
      }

      /*
       * Destructor
       */
      MasterThread::~MasterThread ()
      {
         if (m_state.bInitialized && thread!=nullptr) {
            stop();
         }
      }

      /*
       * Starts the masterthread. Returns true if the thread has been started.
       */
      bool MasterThread::start()
      {
         bool bStarted = false;
         common::logger::information << "Masterthread::start : Entered";

         /* Can we start and are we not already running ? */
         if (m_state.bInitialized && thread == nullptr) {

            /* Change state to running */
            m_state.bRunning = true;

            /* Start the thread */
            common::logger::information << "Masterthread::start : Thread started";
            thread = std::make_unique<std::thread>(&MasterThread::loop, this);
            bStarted = true;
         }

         common::logger::information << "Masterthread::start : Exited";

         return bStarted;
      }

      /*
       * Stops the masterthread. Returns true if the master thread has been stopped.
       */
      bool MasterThread::stop()
      {
         bool bStopped = false;
         common::logger::information << "Masterthread::stop : Entered";

         /* Are we running ? */
         if (thread != nullptr) {

            /* Stop the thread */
            m_state.bRunning = false;

            /* Wait for the thread to finish */
            common::logger::information << "Masterthread::stop : Waiting for thread to finish";
            thread->join();
            thread = nullptr;
            bStopped = true;
         }

         common::logger::information << "Masterthread::stop : Exited";

         return bStopped;
      }

      /*
       * The master threads thread loop
       */
      void MasterThread::loop()
      {
         int status;

         common::logger::information << "Masterthread::loop : Entered";

         /*
          * Set up the socket to start accepting incoming calls
          */
         int n = -1, m = -1;
         n = m_state.socket->bind();
         if (n>=0)
            m = m_state.socket->listen();
         if ((m<0) || (n<0)) {

            common::logger::error << "MasterThread::MasterThread : Unable to initialized socket on " << m_state.m_global.configuration.endpoint;

         } else {

            common::logger::information << "MasterThread::MasterThread : Bound to " << m_state.m_global.configuration.endpoint;

            /* Never ending loop */
            do {

               /* Poll all sockets */
               status = m_state.socketGroupServer.poll(m_state.m_global.configuration.clienttimeout);
               if (status>0) {
                  common::logger::information << "Masterthread::loop : Event happened";
               } else {

                  if (status < 0) {

                     common::logger::information << "Masterthread::loop : Error on socket";
                     m_state.bRunning = false;

                  } else {

                     common::logger::information << "Masterthread::loop : Timeout";

                  }

               }

            } while (m_state.bRunning);

         }

         common::logger::information << "Masterthread::loop : Exited";
      }
   }
}




