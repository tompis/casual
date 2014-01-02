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
        enum {initialized=0, connecting=1, retry=2, active=10, failed=100 } state = failed;

        /*
         * Master endpoint
         */
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

     class ClientSocket : public common::ipc::Socket {

     public:

        /*
         * Constructors destructors
         */
        ClientSocket () = delete;
        ClientSocket (ClientState &s);
        ClientSocket (ClientState &s, common::ipc::Endpoint &p);
        ~ClientSocket();

        /*
         * Writes messages to the other end
         */
        bool writeMessage (common::binary_type &message);

     protected:

        /*
         * Called when the listening state has an incoming connection that we need to accept.
         * This is not a scenario in the client socket.
         */
        int handleIncomingConnection (int events);

        /*
         * Called when the connecting state has an outgoing connect and we have an incoming accept.
         */
        int handleOutgoingConnection (int events);

        /*
         * Handle events when we are in connected state, usually reading and writing data to and from the socket.
         */
        int handleConnected (int events);

        /*
         * Functions that gets called whenever an event occurs for the socket
         */
        int dataCanBeRead ();
        int dataCanBeWritten ();

        /*
         * Internal message handling functions. Returns true if header is found. Changes state to
         * read_message if the header arrives.
         */
        bool locateHeader ();

        /*
         * Reads a message from the datastream. Returns true if message is completed. Changes state to
         * message_complete if the complete message has been read.
         */
        bool readMessage ();

        /*
         * Function that gets called whenever a message has been completed.
         */
        bool handleMessage();

        /*
         * Fill the write buffer from the list of messages
         */
        bool fillWriteBuffer();

        /*
         * Fills the buffer and then writes it to the socket until there is no more data in the buffer or the
         * message list or if the socket is full
         */
        int writeAll();

    private:

       /*
        * Reader area
        */

       /* Message state machine */
       enum { wait_for_header, read_message, message_complete } state = wait_for_header;

       /* Data buffer */
       static const int maxBufferSize = 1024; /* Maximum size of the buffer */
       std::unique_ptr<char[]> buffer;
       int position = 0;  /* Position in the buffer, where our cursor is located */
       int buffer_size = 0; /* Current buffer size */

       /* Message buffer */
       int message_size = 0; /* Expected message size */
       int message_read = 0; /* Currently read bytes of the message */
       common::binary_type message; /* The actual mesage */

       /*
        * Writer area
        */

       /* Holds the message and the current write position */
       class Buffer {

       public:

          /*
           * Constructor and destructor
           */
          Buffer (common::binary_type &message);
          ~Buffer () = default;

          /*
           * Returns with the current buffer
           */
          const void *getBuffer ();

          /*
           * Returns with the number of bytes left in the buffer
           */
          int getSize();

          /*
           * Move cursor
           */
          void moveCursor(int move);

       private:

          /*
           * Internal data
           */
          casual::common::marshal::output::NWBOBinary buffer;
          int position;
       };

       /* List of messages to be sent */
       std::list<std::unique_ptr<Buffer>> listOfMessages;

       /* The write buffer */
       bool possibleToWrite = false;
       static const int maxWriteBufferSize = 1024; /* Maximum size of the buffer */
       std::unique_ptr<char[]> writeBuffer; /* The actual buffer */
       int writePosition = 0;  /* Position in the buffer, where our cursor is located */
       int writeBuffer_size = 0; /* Current buffer size */

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
        ClientThread (State &s, common::ipc::Socket *pSocket);

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
