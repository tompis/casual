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
#include "common/marshal.h"
#include "gateway/wire.h"
#include "gateway/ipc.h"

/*
 * Namespace
 */
namespace casual
{
  namespace gateway
  {
     /**********************************************************************\
      *  The base eventhandler
     \**********************************************************************/

     class BaseHandler : public common::ipc::SocketEventHandler {

     public:

        /*
         * Constructors destructors
         */
        BaseHandler ();
        virtual ~BaseHandler();

        /*
         * Types this handler handles
         */
        int events() const;

        /*
         * Writes messages to the other end
         */
        bool writeMessage (common::binary_type &message);

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
        virtual bool handleMessage() = 0;

        /*
         * Fill the write buffer from the list of messages
         */
        bool fillWriteBuffer();

        /*
         * Fills the buffer and then writes it to the socket until there is no more data in the buffer or the
         * message list or if the socket is full
         */
        bool writeAll(common::ipc::Socket &socket);

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

     };

     /**********************************************************************\
      *  The base eventhandler
     \**********************************************************************/

     class ServerHandler : public BaseHandler {

     public:

        /*
         * Constructors destructors
         */
        ServerHandler ();
        ~ServerHandler();

     protected:

        /*
         * The function that handles registrations of gateways.
         */
        bool handleMessage();

     };

  }
}

#endif /* CASUAL_LISTENER_H_ */
