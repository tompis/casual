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
       *  ClientHandler
      \**********************************************************************/

      ClientHandler::ClientHandler (ClientState &s) : m_state (s)
      {
      }

      ClientHandler::~ClientHandler()
      {
      }

      /*
       * Types this handler handles
       */
      int ClientHandler::events() const
      {
         return POLLRDNORM | POLLWRNORM;
      }

      /*
       * Handles all incoming messages
       */
      bool ClientHandler::handleMessage ()
      {
         return true;
      }

      /**********************************************************************\
       *  ClientThread
      \**********************************************************************/

      ClientThread::ClientThread (State &s) : m_state (s)
      {
         m_state.bInitialized = true;
      }

      /*
       * Destructor
       */
      ClientThread::~ClientThread ()
      {
         if (m_state.bInitialized) {
            stop();
         }
      }

      /*
       * Starts the ClientThread. Returns true if the thread has been started.
       */
      bool ClientThread::start()
      {
         bool bStarted = false;
         common::logger::information << "ClientThread::start : Start";

         /* Can we start and are we not already running ? */
         if (m_state.bInitialized && thread == nullptr) {

            /* Change state to running */
            m_state.bRunning = true;

            /* Start the thread */
            common::logger::information << "ClientThread::start : Thread started";
            thread = std::make_unique<std::thread>(&ClientThread::loop, this);
            bStarted = true;
         }

         common::logger::information << "ClientThread::start : Stop";

         return bStarted;
      }

      /*
       * Stops the ClientThread. Returns true if the Client thread has been stopped.
       */
      bool ClientThread::stop()
      {
         bool bStopped = false;
         common::logger::information << "ClientThread::stop : Start";

         /* Are we running ? */
         if (thread != nullptr && m_state.bRunning) {

            /* Stop the thread */
            m_state.bRunning = false;

            /* Wait for the thread to finish */
            common::logger::information << "ClientThread::stop : Waiting for thread to finish";
            thread->join();
            thread = nullptr;
            bStopped = true;
         }

         common::logger::information << "ClientThread::stop : Stop";

         return bStopped;
      }

      /*
       * The Client threads thread loop
       */
      void ClientThread::loop()
      {
         int status;

         common::logger::information << "ClientThread::loop : Start";

         /* Write the register message to the otherside */

         /* Never ending loop */
         do {

            status = m_state.socketGroupServer.poll(1000);
            if (status>0) {
               common::logger::information << "ClientThread::loop : Event happened";
            } else {

               if (status < 0) {

                  common::logger::information << "ClientThread::loop : Error on socket";
                  m_state.bRunning = false;

               } else {

                  common::logger::information << "ClientThread::loop : Timeout";

               }

            }

         } while (m_state.bRunning);

         common::logger::information << "ClientThread::loop : Stop";
      }
   }
}


