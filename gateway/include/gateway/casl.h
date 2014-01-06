/*
 * casl.h
 *
 *  Created on: 5 jan 2014
 *      Author: tomas
 */

#ifndef CASL_H_
#define CASL_H_

#include <thread>

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
      *  The CASL Default message handling layer on top of a socket
     \**********************************************************************/

     class CASLSocket : public common::ipc::Socket {

     public:

        /*
         * Constructors destructors
         */
        CASLSocket ();
        CASLSocket (common::ipc::Endpoint &endpoint);
        ~CASLSocket();

        /*
         * Writes messages to the other end
         */
        bool writeMessage (common::binary_type &message);

     protected:

        /*
         * Called when the listening state has an incoming connection that we need to accept.
         * This is not a scenario in the client socket.
         */
        virtual State handleIncomingConnection (int events);

        /*
         * Called when the connecting state has an outgoing connect and we have an incoming accept.
         */
        virtual State handleOutgoingConnection (int events);

        /*
         * Handle events when we are in connected state, usually reading and writing data to and from the socket.
         */
        virtual State handleConnected (int events);

        /*
         * Function that gets called whenever a message has been completed, have to be overridden by the
         * superclass of this socket implementation
         */
        virtual bool handleMessage() = 0;

    private:

        /*
         * Called when data can be read during connected state (from handleConnected)
         */
        State dataCanBeRead ();

        /*
         * Called when data can be written during connected state (from handleConnected)
         */
        State dataCanBeWritten ();

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
         * Fill the write buffer from the list of messages
         */
        bool fillWriteBuffer();

        /*
         * Fills the buffer and then writes it to the socket until there is no more data in the buffer or the
         * message list or if the socket is full
         */
        common::ipc::Socket::State writeAll();

       /**********************************************************************
        * Message reader area
        *********************************************************************/

       /*
        * Message state machine for the reader.
        *
        * wait_for_header - Socket is synchronizing the stream with the CASL<size> header to identify the start of a message
        * read_message - Socket has found the start header and is reading the message.
        * message_complete - Socket has reached the end of the message and calls the message handling function.
        *
        * Transition is : wait_for_header -> read_message -> message_complete -> wait_for_header
        *
        */
       enum { wait_for_header, read_message, message_complete } state = wait_for_header;

       /* Maximum size of the read buffer */
       static const int maxBufferSize = 1024; /* Maximum size of the buffer */

       /* The actual read buffer socket data ends up here */
       std::unique_ptr<char[]> buffer;

       /* The current position in the read buffer where the read cursor is located */
       int position = 0;

       /* The current size of the data in the buffer */
       int buffer_size = 0; /* Current buffer size */

       /* The expected message size of a message. This is determined when the header is located */
       int message_size = 0;

       /* The currently number of read bytes of the message, when it reaches message_size the message is complete */
       int message_read = 0;

       /* The actual message data without the transport header CASL<size> */
       common::binary_type message; /* The actual mesage */

       /**********************************************************************
        * Message writer area
        *********************************************************************/

       /*
        * Holds the message in the local message buffer before it is actually written to
        * the socket.
        */
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

       /* Maximum buffer size when writing */
       static const int maxWriteBufferSize = 1024;

       /* Maximum number of messages in the write queue until we start dropping them */
       static const int maxMessagesInWriteQueue = 100;

       /* List of messages to be sent, this is a waiting queue until they end up in the socket */
       std::list<std::unique_ptr<Buffer>> listOfMessages;

       /* True if it is possible to write to the socket */
       bool possibleToWrite = false;

       /* The actual write buffer */
       std::unique_ptr<char[]> writeBuffer;

       /* The current write position in the buffer */
       int writePosition = 0;

       /* The current size of the data in the buffer */
       int writeBuffer_size = 0;

     };
  }
}

#endif /* CASL_H_ */
