/*
 * client.h
 *
 *  Created on: 16 dec 2013
 *      Author: tomas
 */

#ifndef CLIENT_H_
#define CLIENT_H_

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
      * The state of the Client thread
      */
     struct ClientState {

        /*
         * Initializer
         */
        ClientState (State &s);

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
         * If the Client thread is running
         */
        bool bRunning = false;

        /*
         * The socketgroup we are polling for the listener and register service
         */
         common::ipc::SocketGroup socketGroupServer;

         /*
          * Our main socket
          */
     };

     /*
      * The thread that listens to incoming TCP connections
      */
     class ClientThread
     {

     public:

        /*
         * Constructor andf destructor, takes the gateway state as a parameter
         */
        ClientThread (State &s);
        ClientThread (State &s, common::ipc::Socket &socket);
        ~ClientThread ();

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
         * The Client state
         */
        ClientState m_state;
     };

     /**********************************************************************\
      *  The listeners eventhandler
     \**********************************************************************/

     class ClientHandler : public BaseHandler {

     public:

        /*
         * Constructors destructors
         */
        ClientHandler (ClientState &s);
        ~ClientHandler();

        /*
         * Types of events this handler handles
         */
        int events() const;

     protected:

        /*
         * The function that handles registrations of gateways.
         */
        bool handleMessage();

     private:

        /* The client state for the handler */
        ClientState &m_state;
     };
  }
}

#endif /* CLIENT_H_ */
