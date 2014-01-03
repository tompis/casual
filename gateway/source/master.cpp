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
#include "gateway/client.h"
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

      /*
       * State initialization
       */
      MasterState::MasterState(State &s): m_global (s)
      {
      }

      /**********************************************************************\
       *  MasterSocket
      \**********************************************************************/

      /*
       * Master connect handler initialization
       */
      MasterSocket::MasterSocket (MasterState &s, common::ipc::Endpoint &p) : common::ipc::Socket (p), m_state(s)
      {
      }

      /*
       * Master connect handler destruction
       */
      MasterSocket::~MasterSocket()
      {
      }

      /*
       * Handles an incoming request
       */
      int MasterSocket::handleIncomingConnection (int events)
      {
         int ret = 0;
         common::logger::information << "MasterSocket::handleIncomingConnection : Entered";

         /* POLLERR */
         if ((events & POLLERR) != 0) {

            int err = getLastError();
            common::logger::error << "MasterSocket::handleIncomingConnection : Error " << strerror (err) << "(" << err << ")";

            /* Socket is in error, thread is in error */
            ret = -1;
            m_state.state = MasterState::failed;

         }

         /* POLLHUP */
         if ((events & POLLHUP) != 0) {

            common::logger::error << "MasterSocket::handleIncomingConnection : Hangup should never happen";

            /* The other side has hung up, retry connection */
            m_state.state = MasterState::failed;
            ret = -1;
         }

         /* POLLRDNORM */
         if ((events & POLLRDNORM)!=0 && m_state.state == MasterState::active) {

            /* Accept the connection */
            common::logger::information << "MasterSocket::handleIncomingConnection : Accepting incoming connection";

            /* Start a client thread */
            std::unique_ptr<ClientThread> clientThread = std::make_unique<ClientThread>(m_state.m_global, this);
            clientThread->start();

            /* Add the thread to the master list */
            std::lock_guard<std::mutex> lock(m_state.m_global.listOfClientsMutex);
            m_state.m_global.listOfClients.push_back(std::move(clientThread));

         }

         common::logger::information << "MasterSocket::handleIncomingConnection : Exited";
         return ret;

      }

      /*
       * Called when the connecting state has an outgoing connect and we have an incoming accept.
       * This is not a scenario in the master socket.
       */
      int MasterSocket::handleOutgoingConnection (int events)
      {
         common::logger::information << "MasterSocket::handleOutgoingConnection : Entered";
         common::logger::error << "MasterSocket::handleOutgoingConnection : Unexpected event";
         m_state.state = MasterState::failed;
         common::logger::information << "MasterSocket::handleOutgoingConnection : Exited";
         return -1;
      }

      /*
       * Handle events when we are in connected state, usually reading and writing data to and from the socket.
       * This is not a scenario in the master socket.
       */
      int MasterSocket::handleConnected (int events)
      {
         common::logger::information << "MasterSocket::handleConnected : Entered";
         common::logger::error << "MasterSocket::handleConnected : Unexpected event";
         m_state.state = MasterState::failed;
         common::logger::information << "MasterSocket::handleConnected : Exited";
         return -1;
      }

      /**********************************************************************\
       *  MasterThread
      \**********************************************************************/

      /*
       * Create and initialize the master thread.
       */
      MasterThread::MasterThread (State &s) : m_state (s)
      {
         m_state.state = MasterState::fatal;
         common::ipc::Resolver resolver;

         /* Resolve the bind address for the gateway */
         if (resolver.resolve (m_state.m_global.configuration.endpoint)<0) {

            /*
             * Unable to resolve the address, gateway cannot start
             */
            common::logger::error << "MasterThread::MasterThread : Unable to resolve address " << m_state.m_global.configuration.endpoint;

         } else {

            /*
             * Save the endpoint, we use the first one now but we could get
             * the next in the list, but we do not do that just now. TODO!!!!
             */
            m_state.endpoint = resolver.get().front();
            m_state.state = MasterState::initialized;

         }
      }

      /*
       * Destructor
       */
      MasterThread::~MasterThread ()
      {
         if (thread!=nullptr) {
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
         if (m_state.state != MasterState::failed && thread == nullptr) {

            /* Change state to running */
            bRun = true;
            bExited = false;

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

         /* Stop the thread */
         bRun = false;

         /* Are we running ? */
         if (thread != nullptr) {

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
       * Determine if the thread has exited, either by stopping it or by an error condition.
       */
      bool MasterThread::hasExited()
      {
         return bExited;
      }

      /*
       * True if it has been started
       */
      bool MasterThread::hasStarted()
      {
         return bRun;
      }

      /*
       * True if it is restartable, the master is never restartable
       */
      bool MasterThread::isRestartable()
      {
         false;
      }

      /*
       * The current state
       */
      MasterState::MachineState MasterThread::getMachineState()
      {
         return m_state.state;
      }

      /*
       * The master threads thread loop
       */
      void MasterThread::loop()
      {
         int status;

         common::logger::information << "Masterthread::loop : Entered";

         /*
          * Open the socket if we are initialized and not in error
          */
         if (m_state.state == MasterState::initialized) {

            /* Create the socket */
            m_state.socket = std::make_unique<MasterSocket>(m_state, m_state.endpoint);
            if (m_state.socket->isInitialized()) {

               /*
                * Set up the socket to start accepting incoming calls
                */;
               int n = m_state.socket->bind();
               if (n>=0) {

                  n = m_state.socket->listen();

               }

               /* Did it go well ? */
               if (n<0) {

                  /* Something went wrong */
                  common::logger::error << "MasterThread::loop : Unable to bind and listen to socket on " << m_state.endpoint.info();
                  common::logger::error << "MasterThread::loop : " << strerror (errno) << "(" << errno << ")";
                  m_state.state = MasterState::fatal;

               } else {

                  /* We are active */
                  common::logger::information << "MasterThread::loop : Bound and listening on " << m_state.endpoint.info();
                  m_state.state = MasterState::active;

               }

            } else {

               /* We failed */
               common::logger::error << "MasterThread::loop : Unable to create socket for " << m_state.endpoint.info() << "," << m_state.socket->getState();
               m_state.state = MasterState::fatal;
            }
         }

         /* Never ending loop */
         while (bRun && m_state.state != MasterState::failed && m_state.state != MasterState::fatal) {

            /* Poll all sockets */
            status = m_state.socket->execute(m_state.m_global.configuration.clienttimeout);
            if (status < 0) {

               /* Something went wrong */
               common::logger::error << "Masterthread::loop : Error : " << strerror (errno) << "(" << errno << ")";
               m_state.state = MasterState::failed;

            } else {

               /* Normal timeout or we actually got some events to handle */
               if (status == 0) {
                  /* Timeout loop */
               }

            }

         }

         /* Thread has finished */
         bExited = true;
         common::logger::information << "Masterthread::loop : Exited";
      }
   }
}
