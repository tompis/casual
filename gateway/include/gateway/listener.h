/*
 * gwserver.h
 *
 *  Created on: 7 dec 2013
 *      Author: Tomas Stenlund
 */

#ifndef CASUAL_LISTENER_H_
#define CASUAL_LISTENER_H_

/*
 * Casual
 */
#include "gateway/ipc.h"

/*
 * Namespace
 */
namespace casual
{
  namespace gateway
  {
     /**********************************************************************\
      *  GatewayState
     \**********************************************************************/

     /*
      * Holds the state of the listener thread
      */
     struct GatewayState {

        /*
         * List of all sockets that are connected, by we are waiting for a register
         */
        std::list<std::shared_ptr<common::ipc::Socket>> listOfAcceptedConnections;

        /*
         * The socketgroup we are polling for the listener and register service
         */
         common::ipc::SocketGroup socketGroupServer;

     };

     /**********************************************************************\
      *  The listeners eventhandler
     \**********************************************************************/

     class ConnectHandler : public common::ipc::SocketEventHandler {

     public:

        /*
         * Constructors destructors
         */
        ConnectHandler (GatewayState &ls);
        ~ConnectHandler();

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
        GatewayState &state;

     };

     /**********************************************************************\
      *  The register eventhandler
     \**********************************************************************/

     class RegisterHandler : public common::ipc::SocketEventHandler {

     public:

        /*
         * Constructors destructors
         */
        RegisterHandler (GatewayState &ls);
        ~RegisterHandler();

        /*
         * Types this handler handles
         */
        int events() const;

     protected:

        /*
         * Functions that gets called whenever an event occurs for the socket
         */
        int dataCanBeRead (int events, common::ipc::Socket &socket);
        int dataCanBeWritten (int events, common::ipc::Socket &socket);

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
         * Virtual function that gets called when a complete message has arrived. Returns true if the message
         * has been processed.
         */
        bool handleMessage();

     private:

        /* Message information */
        enum { wait_for_header, read_message, message_complete } state = wait_for_header;

        /* Data buffer */
        const int maxBufferSize = 1024; /* Maximum size of the buffer */
        char buffer [maxBufferSize]; /* The actual buffer */
        int position = 0;
        int buffer_size = 0; /* Current buffer size */

        /* Message buffer */
        int message_size = 0; /* Expected message size */
        int message_read = 0; /* Currently read bytes of the message */
        common::binary_type message; /* The actual mesage */

        /* Message */

        /* The state */
        GatewayState &state;

     };

  }
}

#endif /* CASUAL_LISTENER_H_ */
