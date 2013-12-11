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
             * No copy and assignment possible with sockets, there can only be one!
             */
            Signal (const Signal &other) = delete;
            Signal &operator=(const Signal &other) = delete;

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

            /* Friends, so we can encapsulate posix much more */
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

         private:

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
             * Constructs and destroys the resolver
             */
            Resolver() = default;
            Resolver (std::string connectionString);
            ~Resolver() = default;

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
             * No copy and assignment possible with sockets, there can only be one!
             */
            Resolver (const Resolver &other) = delete;
            Resolver &operator=(const Resolver &other) = delete;

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
         class SocketPair;

         /*
          * Socket
          */
         class Socket {

         public:

            /* Friends, so we can encapsulate posix much more */
            friend SocketGroup;
            friend SocketPair;

            /*
             * Constructors and destructors
             */
            Socket(Endpoint &p);
            ~Socket();

            /*
             * No copy and assignment possible with sockets, there can only be one!
             */
            Socket (const Socket &other) = delete;
            Socket &operator=(const Socket &other) = delete;

            /*
             * Standard socket functions
             */

            /*
             * Close the socket, returns -1 on error, zero otherwise
             */
            int close();

            /*
             * Conect to the current endpoint, returns -1 on error, zero otherwise
             */
            int connect ();

            /*
             * Bind the socket to an endpoint, returns -1 on error, zero otherwise
             */
            int bind();

            /*
             * Listen to an bound socket, returns -1 on error, zero otherwise
             */
            int listen(int backlog = SOMAXCONN);

            /*
             * Accept incoming connection and return a new socket
             */
            std::unique_ptr<Socket> accept();

            /*
             * Other socket functions
             */
            Endpoint *getEndpoint ();

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

            /*
             * write data
             */
            int write (void *pData, int size);

            /*
             * read data
             */
            int read (void *pData, int size);

         private:

            /*
             * Socket created from a file descriptor, we maybe dont have an endpoint if so set it to null.
             */
            Socket (int fd, Endpoint *p = nullptr);

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

            /*
             * Socket file descriptor
             */
            int fd;

            /*
             * Socket endpoint
             */
            std::unique_ptr<Endpoint> pEndpoint;

            /*
             * Socket event handlers
             */
            std::unique_ptr<SocketEventHandler> pEventHandler = nullptr;

         };

         /**********************************************************************\
          *  SocketPair
         \**********************************************************************/

         class SocketPair {

         public:

            /*
             * Constructor and destructor
             */
            SocketPair ();
            ~SocketPair();

            /*
             * No copy and assignment possible with sockets, there can only be one!
             */
            SocketPair (const SocketPair &other) = delete;
            SocketPair &operator=(const SocketPair &other) = delete;

            /*
             * Getters for the socketpair A and B side, moves the ownership!!!!
             */
            std::unique_ptr<Socket> getSocketA ();
            std::unique_ptr<Socket> getSocketB ();

         protected:

         private:

            /*
             * Sockets
             */
            std::unique_ptr<Socket> pA = nullptr;
            std::unique_ptr<Socket> pB = nullptr;

         };

         /**********************************************************************\
          *  SocketGroup
         \**********************************************************************/

         class SocketGroup {

         public:

            SocketGroup() = default;
            ~SocketGroup() = default;

            /*
             * No copy and assignment possible with sockets, there can only be one!
             */
            SocketGroup (const SocketGroup &other) = delete;
            SocketGroup &operator=(const SocketGroup &other) = delete;

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
            void addSocket(std::shared_ptr<Socket> pSocket);
            void removeSocket (std::shared_ptr<Socket> pSocket);

         protected:
         private:

            /*
             * Creates a poll filter for the POSIX poll
             */
            void generatePollFilter();

            /*
             * List of all sockets in the group
             */
            std::list<std::shared_ptr<Socket>> listOfSockets;

            /*
             * Poll structure
             */
            bool dirty = false;
            std::unique_ptr<struct pollfd[]> clients;
         };

         /**********************************************************************\
          *  SocketEventHandler
         \**********************************************************************/

         class SocketEventHandler {

         public:

            /*
             * Create and destroys a socket handler
             */
            SocketEventHandler() = default;
            virtual ~SocketEventHandler() = default;

            /*
             * No copy and assignment possible with sockets, there can only be one!
             */
            SocketEventHandler (const SocketEventHandler &other) = delete;
            SocketEventHandler &operator=(const SocketEventHandler &other) = delete;

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
             * Functions that gets called whenever an event occurs for the socket
             */
            virtual int dataCanBeRead (int events, Socket &socket);
            virtual int dataCanBeWritten (int events, Socket &socket);
            virtual int hangup (int events, Socket &socket);
            virtual int error (int events, Socket &socket);

         };

      }
   }
}

#endif /* CASUAL_GATEWAY_IPC_H_ */
