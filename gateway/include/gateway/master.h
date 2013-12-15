/*
 * master.h
 *
 *  Created on: 15 dec 2013
 *      Author: tomas
 */

#ifndef MASTER_H_
#define MASTER_H_

#include <thread>

/*
 * Casual
 */
#include "common/marshal.h"
#include "gateway/wire.h"
#include "gateway/ipc.h"
#include "gateway/state.h"

/*
 * Namespace
 */
namespace casual
{
  namespace gateway
  {

     /*
      * The state of the master thread
      */
     struct MasterState {

        /*
         * Initializer
         */
        MasterState (State &s);

        /*
         * The global gateway state
         *
         */
        State &m_global;

        /*
         * If all configuration and pre initialization has been done okey
         */
        bool bInitialized = false;

        /*
         * If the master thread is running
         */
        bool bRunning = false;

        /*
         * Master socket
         */
        common::ipc::Endpoint endpoint;
        std::shared_ptr<common::ipc::Socket> socket;

        /*
         * List of all sockets that are connected and where we are waiting for a register
         * message from the client to announce its name and services so we know which thread
         * to start.
         */
        std::list<std::shared_ptr<common::ipc::Socket>> listOfAcceptedConnections;

        /*
         * The socketgroup we are polling for the listener and register service
         */
         common::ipc::SocketGroup socketGroupServer;
     };

     /*
      * The thread that listens to incoming TCP connections
      */
     class MasterThread
     {

     public:

        /*
         * Constructor andf destructor
         */
        MasterThread (MasterState &ms);
        ~MasterThread ();

        /*
         * Start the thread. Returns true if the thread has been started.
         */
        bool start ();

        /*
         * Stop the thread. Returns true if the thread has been stopped.
         */
        bool stop ();

     protected:

        /*
         * The thread loop
         */
        void loop ();

     private:

        /*
         * The currently running thread
         */
        std::unique_ptr<std::thread> thread = nullptr;

        /*
         * The master state
         */
        MasterState &m_state;
     };

     /**********************************************************************\
      *  The listeners eventhandler
     \**********************************************************************/

     class MasterHandler : public common::ipc::SocketEventHandler {

     public:

        /*
         * Constructors destructors
         */
        MasterHandler (MasterState &ms);
        ~MasterHandler();

        /*
         * Types this handler handles
         */
        int events() const;

     protected:

        /*
         * Functions that gets called whenever an event occurs for the socket
         */
        int dataCanBeRead (int events, common::ipc::Socket &socket);

     private:

        /* The state */
        MasterState &m_state;

     };
  }
}


#endif /* MASTER_H_ */
