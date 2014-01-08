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
#include "gateway/casl.h"
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

     /*
      * Predefines
      */
     class ClientSocket;

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
        enum MachineState {initialized=0, connecting, retry, active=10, failed=100, fatal } state = failed;

        /*
         * Remote gateway data
         */
        std::string localRemoteName;
        std::string localRemoteURL;
        std::string remoteName;
        casual::common::Uuid remoteUUID;
        common::ipc::Endpoint endpoint;

        /*
         * client socket
         */
        std::unique_ptr<ClientSocket> socket;

        /*
         * Message header information
         */
        int messageCounter = 0;

        /*
         * State as  text
         */
        std::string getState ();

     };

     /**********************************************************************\
      *  The client socket
     \**********************************************************************/

     class ClientSocket : public CASLSocket {

     public:

        /*
         * Constructors destructors
         */
        ClientSocket () = delete;
        ClientSocket (ClientState &s);
        ClientSocket (ClientState &s, Endpoint &p);
        ~ClientSocket();

     protected:

        /*
         * The message handler, i.e. every incoming message from the other end, ends up here
         */
        bool handleMessage();

        /*
         * Fill the write buffer from the list of messages
         */
        bool fillWriteBuffer();


     private:

        /* The client state for the handler */
        ClientState &m_state;
     };

     /**********************************************************************\
      *  The client thread
     \**********************************************************************/

     class ClientThread
     {

     public:

        /*
         * Not allowed, means nothing ...
         */
        ClientThread () = delete;

        /*
         * Creates a client to a remote gateway based on an incoming connection from the remote gateway. The socket is
         * in listening mode and the incoming connection has to be handled with an accept.
         */
        ClientThread (State &s, Socket *pSocket);

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
        ClientState::MachineState getMachineState();

        /*
         * The name of the thread
         */
        std::string getName ();

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
        bool bRun = false;
        bool bRestartable = false;
        bool bExited = false;

        /*
         * The Client state
         */
        ClientState m_state;
     };

  }
}

#endif /* CLIENT_H_ */
