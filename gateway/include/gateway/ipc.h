/*
 * ipc.h
 *
 *  Created on: 28 okt 2013
 *      Author: Tomas Stenlund
 */

#ifndef CASUAL_GATEWAY_IPC_H_
#define CASUAL_GATEWAY_IPC_H_

/*
 * Standard headers
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 * STD
 */
#include <thread>
#include <condition_variable>
#include <memory>
#include <list>

/*
 * Casual namespace
 */
namespace casual
{
   namespace common
   {
      namespace ipc
      {

         /**********************************************************************\
          *  Signal
         \**********************************************************************/

         /*
          * Simplified IPC signalling
          */
         class Signal {

         public:

            /*
             * Default constructor and destructor
             */
            Signal() = default;
            ~Signal() = default;

            /*
             * Wait for the signal
             */
            void wait();

            /*
             * Notify everyone waiting for the signal
             */
            void notifyAll();

         private:

            /*
             * The mutex and condition variables needed
             */
            std::condition_variable m_conditionVariable;
            std::mutex m_mutex;

         };

         /**********************************************************************\
          *  Endpoint
         \**********************************************************************/

         /*
          * The basic endpoint
          */
         class Endpoint {

            /*
             * Friends to the endpoints
             */
            friend class Socket;
            friend class Resolver;

         public:

            /*
             * Destructor
             */
            virtual ~Endpoint ();

            /*
             * Copy constructor
             */
            Endpoint ( const Endpoint &other);
            Endpoint& operator=( const Endpoint&other);

            /*
             * Information of the endpoint in string format
             */
            std::string info();

         protected:

            /*
             * Constructor, only for friends.
             *
             * family, type, protocol and socket address
             */
            Endpoint (int f, int t, int p, const void *sa, size_t len);

            /*
             * The POSIX data
             */
            std::unique_ptr<char[]> m_data;
            std::size_t m_size;
            int family;
            int protocol;
            int type;

         private:

            /*
             * Information string builders for various families
             */
            std::string infoTCPv4();
            std::string infoTCPv6();

            /*
             * POSIX helper function that copies address info
             */
            void copyData(const void *data, std::size_t size);

            /*
             * Copy and assignment operator helper
             */
            void copy (const Endpoint &other);

         };

         /**********************************************************************\
          *  Resolver
         \**********************************************************************/

         /*
          * The network service resolver
          */
         class Resolver {

         public:

            /*
             * Iterators
             */
            typedef std::list<Endpoint>::const_iterator const_iterator;

            /*
             * iterators for traversing the list of endpoints valid for the url resolved
             */
            const_iterator begin();
            const_iterator end();

            /*
             * Constructs and destroys the resolver
             */
            Resolver() = default;
            Resolver (std::string connectionString);
            ~Resolver() = default;

            /*
             * Resolves a connection string to one or several endpoints. Use the iterator to traverse the list
             *
             * Returns number of endpoints this connection resolved to or less than zero on failure.
             */
            int resolve (std::string connectionString);

            /*
             * Return with a list
             */
            const std::list<Endpoint> &get() const;

         private:

            /* List of all possible endpoints */
            std::list<Endpoint> listOfEndpoints;

         };

         /**********************************************************************\
          *  Socket
         \**********************************************************************/

         /* Pre declaration */
         class SocketEventHandler;
         class SocketGroup;

         /*
          * Socket
          */
         class Socket {

            /* Friends */
            friend SocketGroup;

         public:

            /*
             * Constructors and destructors
             */
            Socket(Endpoint &p);
            ~Socket();

            /*
             * No copy and assignment possible with sockets
             */
            Socket (const Socket &other) = delete;
            Socket &operator=(const Socket &other) = delete;

            /*
             * Standard socket functions
             */

            /*
             * Close the socket
             */
            int close();

            /*
             * Conect to the current endpoint
             */
            int connect ();

            /*
             * Bind the socket to an endpoint
             */
            int bind();

            /*
             * Listen to an bound socket
             */
            int listen(int backlog = SOMAXCONN);

            /*
             * Accept incoming connection and return a new socket
             */
            std::unique_ptr<Socket> accept();

            /*
             * Other socket functions
             */
            Endpoint getEndpoint ();

            /*
             * Add handler, ownership is taken
             */
            void setEventHandler(std::unique_ptr<SocketEventHandler> &pSEH);

            /*
             * Polls the socket.
             *
             * Returns 0 on timeout, 1 on event occured and -1 on error and calls the eventhandler
             * for the socket.
             */
            int poll(int timeout);

         protected:

            /*
             * Socket created from a file descriptor
             */
            Socket (int fd, Endpoint &p);

            /*
             * Get a hold on the file descriptor
             */
            int getSocket() const;

            /*
             * Return with the eventhandler, ownership is maintained
             */
            const SocketEventHandler *getEventHandler() const;

            /*
             * Executes a handler based on the event mask
             */
             int handle (int events);

         private:

            /*
             * Socket file descriptor
             */
            int fd;

            /*
             * Socket endpoint
             */
            Endpoint endpoint;

            /*
             * Socket event handlers
             */
            std::unique_ptr<SocketEventHandler> pEventHandler = nullptr;

         };

         /**********************************************************************\
          *  SocketEventHandler
         \**********************************************************************/

         /* Base class for the event handler for a socket */
         class SocketEventHandler {

         public:

            /*
             * Create and destroys a socket handler
             */
            SocketEventHandler() = default;
            virtual ~SocketEventHandler() = default;

            /*
             * The handle function
             */
            int handle (int events, Socket &socket);

            /*
             * Types this handler handles
             */
            virtual int events() const = 0;

         protected:

            /*
             * Handlers
             */
            virtual int dataCanBeRead (int events, Socket &socket);
            virtual int dataCanBeWritten (int events, Socket &socket);
            virtual int dataHangup (int events, Socket &socket);
            virtual int dataError (int events, Socket &socket);

         };

         /**********************************************************************\
          *  SocketGroup
         \**********************************************************************/

         class SocketGroup {

         public:

            SocketGroup() = default;
            ~SocketGroup() = default;

            /*
             * Polls all sockets
             *
             * Returns 0 on timeout, 1 on event occured and -1 on errorued and calls the eventhandler for all
             * sockets in the group.
             */
            int poll(int timeout);

            /*
             * Add a socket, ownership is not taken
             */
            void addSocket(Socket *pSocket);
            void removeSocket (Socket *pSocket);

         protected:
         private:

            /*
             * Creates a poll filter for the POSIX poll
             */
            void generatePollFilter();

            /* List of all sockets in the group */
            std::list<Socket *> listOfSockets;

            /* Poll filter */
            std::unique_ptr<struct pollfd[]> clients;
         };

      }
   }
}

#endif /* CASUAL_GATEWAY_IPC_H_ */
