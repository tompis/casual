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
//#include "common/marshal.h"
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
      * Make life a bit easier
      */
     using common::ipc::Socket;
     using common::ipc::Endpoint;
     using common::ipc::Resolver;

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
         * The masters state
         */
        enum MachineState {initialized=0, active=10, failed=100, fatal } state = failed;

        /*
         * Master socket
         */
        common::ipc::Endpoint endpoint;
        std::unique_ptr<Socket> socket;

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

        /*
         * Determine if the thread has exited, either by stopping it or by an error condition.
         */
        bool hasExited();

        /*
         * True if it has been started
         */
        bool hasStarted();

        /*
         * True if it is restartable
         */
        bool isRestartable();

        /*
         * The current state
         */
        MasterState::MachineState getMachineState();

     protected:

        /*
         * The thread loop
         */
        void loop ();

        /*
         * The timeout loop
         */
        void housekeeping ();

     private:

        /*
         * The currently running thread
         */
        std::unique_ptr<std::thread> thread = nullptr;
        bool bRun = false;
        bool bExited = false;

        /*
         * The master state
         */
        MasterState m_state;
     };

     /**********************************************************************\
      *  The Master socket
     \**********************************************************************/

     /*
      * This class handles the events for a socket in listening state and handles
      * the event when a client is requesting a connection.
      */
     class MasterSocket : public Socket {

     public:

        /*
         * Constructors destructors
         */
        MasterSocket () = delete;
        MasterSocket (MasterState &s, Endpoint &p);
        ~MasterSocket();

     protected:

        /*
         * Called when the listening state has an incoming connection that we need to accept.
         */
        State handleIncomingConnection (int events);

        /*
         * Called when the connecting state has an outgoing connect and we have an incoming accept.
         * This is not a scenario in the master socket.
         */
        State handleOutgoingConnection (int events);

        /*
         * Handle events when we are in connected state, usually reading and writing data to and from the socket.
         * This is not a scenario in the master socket.
         */
        State handleConnected (int events);

     private:

        /* The state */
        MasterState &m_state;

     };
  }
}


#endif /* MASTER_H_ */
