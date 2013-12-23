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
         * The client state
         */
        enum {initialized, connecting, retry, failed, active} state = failed;

        /*
         * Master endpoint
         */
        common::ipc::Endpoint endpoint;

        /*
         * Master socket
         */
        std::unique_ptr<common::ipc::Socket> socket;

        /*
         * Message header information
         */
        int messageCounter = 0;

     };

     /*
      * The thread that listens to incoming TCP connections
      */
     class ClientThread
     {

     public:

        /*
         * Not allowed, means nothing ...
         */
        ClientThread () = delete;

        /*
         * Creates a client to a remote gateway based on an incoming connection from the remote gateway
         */
        ClientThread (State &s, std::unique_ptr<common::ipc::Socket> socket);

        /*
         * Creates a client to a remote gateway based on configuration
         */
        ClientThread (State &s, configuration::RemoteGateway &remote);

        /*
         * Destroys the client
         */
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
         * Connects the socket. Returns true if the connection was initiated.
         */
        bool connect();

        /*
         * The thread loop
         */
        void loop ();

     private:

        /*
         * The currently running thread
         */
        std::unique_ptr<std::thread> thread = nullptr;
        bool run = false;

        /*
         * The Client state
         */
        ClientState m_state;
     };

     /**********************************************************************\
      *  The client connect to eventhandler
     \**********************************************************************/

     class ClientConnectHandler : public common::ipc::SocketEventHandler {

     public:

        /*
         * Constructors destructors
         */
        ClientConnectHandler (ClientState &s);
        ~ClientConnectHandler();

        /*
         * Types of events this handler handles
         */
        int events() const;

     protected:

        /*
         * Functions that gets called whenever an event occurs for the socket. In this case
         * I am only after that data can be written, because for a socket when connecting
         * this signals that a connection has been completed
         */
        int dataCanBeWritten (int events, common::ipc::Socket &socket);

        /*
         * Eror handling
         */
        int hangup (int events, common::ipc::Socket &socket);
        int error (int events, common::ipc::Socket &socket);

     private:

        /* The client state for the handler */
        ClientState &m_state;
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
