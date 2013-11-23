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

         public:

            /*
             * Destructor
             */
            virtual ~Endpoint ();

         protected:

            /*
             * Constructor, only for friends.
             *
             * family, type, protocol and socket address
             */
            Endpoint (int f, int t, int p, sockaddr *sa, socklen_t len);

            /*
             * The POSIX data
             */
            std::unique_ptr<void> m_data;
            std::size_t m_size;
            int family;
            int protocol;
            int type;

         private:

            /*
             * POSIX helper function that copies address info
             */
            void copy_data(const void *data, std::size_t size);
         };

         typedef std::shared_ptr<Endpoint> PEndpoint;

         /*
          * Endpoint TCPv4
          */
         class EndpointTCPv4 : public Endpoint {

            /*
             * Friends, so they get access to the POSIX stuff, we do not want that to be visible
             */
            friend class Resolver;

         public:

            /*
             * Destructor
             */
            ~EndpointTCPv4();

         private:

            /*
             * Constructor POSIX, only for the Resolver
             */
            EndpointTCPv4 (sockaddr *sa, socklen_t len);

         };

         typedef std::shared_ptr<EndpointTCPv4> PEndpointTCPv4;

         /*
          * Endpoint TCPv6
          */
         class EndpointTCPv6 : public Endpoint {

            /*
             * Friends, so they get access to the POSIX stuff, we do not want that to be visible
             */
            friend class Resolver;

         public:

            /*
             * Destructor
             */
            ~EndpointTCPv6();

         private:

            /*
             * Constructor POSIX, only for the Resolver
             */
            EndpointTCPv6 (sockaddr *sa, socklen_t len);

         };

         typedef std::shared_ptr<EndpointTCPv6> PEndpointTCPv6;

         /*
          * The network service resolver
          */
         class Resolver {

         public:

            /*
             * Iterators
             */
            typedef std::list<PEndpoint>::iterator iterator;
            typedef std::list<PEndpoint>::const_iterator const_iterator;

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
            std::list<PEndpoint> listOfEndpoints;

         };

         /*
          * Socket
          */

         class Socket;
         typedef std::shared_ptr<Socket> PSocket;

         class Socket {

         public:
            Socket(PEndpoint pEndpoint);
            ~Socket();

            /*
             * Standard socket functions
             */
            int close();
            int connect ();
            int bind();
            int listen(int backlog = SOMAXCONN);
            PSocket accept();

         private:

            /*
             * Socket file descriptor
             */
            int fd;

            /*
             * Socket endpoint
             */
            PEndpoint pEndpoint;
         };

      }
   }
}

#endif /* CASUAL_GATEWAY_IPC_H_ */
