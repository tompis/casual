/*
 * ipc.h
 *
 *  Created on: 28 okt 2013
 *      Author: tomas
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

         /*
          * The network service resolver
          */
         class Resolver {

         public:

            /*
             * Iterators
             */
            typedef std::list<Endpoint>::iterator iterator;
            typedef std::list<Endpoint>::const_iterator const_iterator;

            /*
             * iterators for traversing the list of endpoints valid for the url resolved
             */
            iterator begin();
            iterator end();

            /*
             * Constructs and destroys the resolver
             */
            Resolver() = default;
            ~Resolver() = default;

            /*
             * Resolves a connection string to one or several endpoints. Use the iterator to traverse the list
             *
             * Returns 0 on success.
             */
            int resolve (std::string connectionString);

         private:

            /* List of all possible endpoints */
            std::list<Endpoint> listOfEndpoints;

         };

         /*
          * Socket
          */

         class Socket;
         typedef std::shared_ptr<Socket> PSocket;

         class Socket {

         public:
            Socket(Endpoint &p);
            ~Socket();

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
            PSocket accept();

         protected:

            /*
             * Socket created from a file descriptor
             */
            Socket (int fd, Endpoint &p);

         private:

            /*
             * Socket file descriptor
             */
            int fd;

            /*
             * Socket endpoint
             */
            Endpoint endpoint;
         };

      }
   }
}

#endif /* CASUAL_GATEWAY_IPC_H_ */
