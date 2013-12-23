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

     /**********************************************************************\
      *  The Masters State
     \**********************************************************************/

     /*
      * The state of the master thread, it also holds a reference to the gateway state which is a global
      * state.
      */
     struct MasterState {

        /*
         * Initializer
         */
        MasterState (State &s);

        /*
         * The global gateway state, note that changing things in this state normally requires
         * mutexlocking. The state has high granularity locks, so you need to check the state
         * definition to know what to lock and not lock.
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
         * The socketgroup we are polling for the listener and register service
         */
         common::ipc::SocketGroup socketGroupServer;
     };

     /**********************************************************************\
      *  The Masters Thread
     \**********************************************************************/

     /*
      * The thread that listens to incoming TCP connections and spawns of clients
      * that handles incoming requests.
      */
     class MasterThread
     {

     public:

        /*
         * Constructor andf destructor
         */
        MasterThread (State &ms);
        ~MasterThread ();

        /*
         * Start the thread. Returns true if the thread has been started.
         */
        bool start ();

        /*
         * Stops the thread. Returns true if the thread has been stopped.
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
        MasterState m_state;
     };

     /**********************************************************************\
      *  The Masters connection eventhandler
     \**********************************************************************/

     /*
      * This class handles the events for a socket in listening state and handles
      * the event when a client is requesting a connection.
      */
     class MasterConnectHandler : public common::ipc::SocketEventHandler {

     public:

        /*
         * Constructors destructors
         */
        MasterConnectHandler (MasterState &s);
        ~MasterConnectHandler();

        /*
         * Types this handler handles
         */
        int events() const;

     protected:

        /*
         * Functions that gets called whenever an event occurs for the socket. In this case
         * I am only after the data can be read, because for a socket in listening state
         * this signals that a connection request has arrived.
         */
        int dataCanBeRead (int events, common::ipc::Socket &socket);

     private:

        /* The state */
        MasterState &m_state;

     };
  }
}


#endif /* MASTER_H_ */
