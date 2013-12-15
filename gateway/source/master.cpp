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

      MasterState::MasterState(State &s): m_global (s)
      {

      }

      /**********************************************************************\
       *  MasterThread
      \**********************************************************************/

      /**********************************************************************\
       *  MasterHandler
      \**********************************************************************/

      MasterHandler::MasterHandler (MasterState &ms) : m_state (ms)
      {
      }

      MasterHandler::~MasterHandler()
      {
      }

      /*
       * Types this handler handles
       */
      int MasterHandler::events() const
      {
         return POLLIN;
      }

      int MasterHandler::dataCanBeRead (int events, common::ipc::Socket &socket)
      {
         std::unique_ptr<common::ipc::Socket> pS;

         common::logger::information << "MasterHandler::dataCanBeRead : Incoming connection, event = " << events;

         /* Accept the connection */
         pS = socket.accept();
         if (pS==0L) {
            common::logger::warning << "MasterHandler::dataCanBeRead : Incoming connection not accepted";
         } else {
            common::logger::information << "MasterHandler::dataCanBeRead : Incoming connection accepted";
         }

         /* Add an eventhandler to the socket */
         std::unique_ptr<common::ipc::SocketEventHandler> pRL;
         pRL.reset (new ServerHandler());
         pS->setEventHandler (pRL);

         /* Add the socket to the group and continue with the business */
         std::shared_ptr<common::ipc::Socket> pP = std::shared_ptr<common::ipc::Socket>(pS.release());
         m_state.listOfAcceptedConnections.push_back (pP);

         /* Add the socket to the group that we are polling */
         m_state.socketGroupServer.addSocket(pP);
      }

      /**********************************************************************\
       *  MasterThread
      \**********************************************************************/

      MasterThread::MasterThread (MasterState &ms) : m_state (ms)
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
             * Create the socket
             */
            m_state.endpoint = resolver.get().front();
            m_state.socket = std::make_shared<common::ipc::Socket>(m_state.endpoint);

            /*
             * Add the socket handler and add it to the group we want to poll
             */
            m_state.socketGroupServer.addSocket(m_state.socket);
            std::unique_ptr<common::ipc::SocketEventHandler> eH = std::make_unique<MasterHandler>(m_state);
            m_state.socket->setEventHandler(eH);

            /* Initialize the socket to accept incoming calls */
            int n = -1, m = -1;
            n = m_state.socket->bind();
            if (n>=0)
               m = m_state.socket->listen();

            /* All is well */
            m_state.bInitialized = (m>=0) && (n>=0);
            if (!m_state.bInitialized)
               common::logger::error << "MasterThread::MasterThread : Unable to initialized socket on " << m_state.m_global.configuration.endpoint;
         }
      }

      /*
       * Destructor
       */
      MasterThread::~MasterThread ()
      {
         if (m_state.bInitialized) {
            stop();
         }
      }

      /*
       * Starts the masterthread. Returns true if the thread has been started.
       */
      bool MasterThread::start()
      {
         bool bStarted = false;
         common::logger::information << "Masterthread::start : Start";

         /* Can we start and are we not already running ? */
         if (m_state.bInitialized && thread == nullptr) {

            /* Change state to running */
            m_state.bRunning = true;

            /* Start the thread */
            common::logger::information << "Masterthread::start : Thread started";
            thread = std::make_unique<std::thread>(&MasterThread::loop, this);
            bStarted = true;
         }

         common::logger::information << "Masterthread::start : Stop";

         return bStarted;
      }

      /*
       * Stops the masterthread. Returns true if the master thread has been stopped.
       */
      bool MasterThread::stop()
      {
         bool bStopped = false;
         common::logger::information << "Masterthread::stop : Start";

         /* Are we running ? */
         if (thread != nullptr && m_state.bRunning) {

            /* Stop the thread */
            m_state.bRunning = false;

            /* Wait for the thread to finish */
            common::logger::information << "Masterthread::stop : Waiting for thread to finish";
            thread->join();
            thread = nullptr;
            bStopped = true;
         }

         common::logger::information << "Masterthread::stop : Stop";

         return bStopped;
      }

      /*
       * The master threads thread loop
       */
      void MasterThread::loop()
      {
         int status;

         common::logger::information << "Masterthread::loop : Start";

         /* Never ending loop */
         do {

            status = m_state.socketGroupServer.poll(1000);
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

         common::logger::information << "Masterthread::loop : Stop";
      }
   }
}




