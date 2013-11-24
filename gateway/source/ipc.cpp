/*
 * ipc.cpp
 *
 *  Created on: 9 nov 2013
 *      Author: tomas
 */

#include <sys/types.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

/*
 * STL
 */
#include <cstring>
#include <string>
#include <utility>
#include <thread>
#include <condition_variable>
#include <memory>

/*
 * Casual common
 */
#include "common/logger.h"
#include "gateway/ipc.h"

/*
 * Casual namespace
 */
namespace casual
{
   namespace common
   {
      namespace ipc
      {

         /*---------------------------------------------------------------------------------------------
          * The signal class
          *---------------------------------------------------------------------------------------------*/

         /*
          * Wait for the signal
          */
         void Signal::wait()
         {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_conditionVariable.wait(lk);
         }

         /*
          * Notify everyone waiting for the signal
          */
         void Signal::notifyAll()
         {
            m_conditionVariable.notify_all();
         }

         /*---------------------------------------------------------------------------------------------
          * The basic_endpoint class
          *---------------------------------------------------------------------------------------------*/

         /*
          * Constructs the endpoint
          */
         Endpoint::Endpoint (int f, int t, int p, struct sockaddr *sa, socklen_t len)
         {
            family = f;
            protocol = p;
            type = t;
            copy_data (sa, len);
         }

         /*
          * Destroys an endpoint
          */
         Endpoint::~Endpoint()
         {
         }

         /*
          * Used by the resolver to set the data
          */
         void Endpoint::copy_data(const void *data, std::size_t size)
         {
            m_data = std::unique_ptr<void>(new char[size]);
            memcpy (m_data.get(), data, size);
         }
         /*---------------------------------------------------------------------------------------------
          * Endpoints
          *---------------------------------------------------------------------------------------------*/

         /*
          * Constructor POSIX, only for the Resolver
          */
         EndpointTCPv4::EndpointTCPv4 (sockaddr *sa, socklen_t len) : Endpoint (AF_INET, SOCK_STREAM, IPPROTO_TCP, sa, len)
         {
         }

         /*
          * Destructor
          */
         EndpointTCPv4::~EndpointTCPv4()
         {

         }

         /*
          * String info
          */
         std::string EndpointTCPv4::info()
         {
            return "";
         }


         /*
          * Constructor POSIX, only for the Resolver
          */
         EndpointTCPv6::EndpointTCPv6 (sockaddr *sa, socklen_t len) : Endpoint (AF_INET6, SOCK_STREAM, IPPROTO_TCP, sa, len)
         {
         }

         /*
          * Destructor
          */
         EndpointTCPv6::~EndpointTCPv6()
         {

         }

         /*
          * String info
          */
         std::string EndpointTCPv6::info()
         {
            return "";
         }

         /*---------------------------------------------------------------------------------------------
          * The Resolver class
          *---------------------------------------------------------------------------------------------*/

         /*
          * The iterator functions
          */
         Resolver::iterator Resolver::begin()
         {
            return listOfEndpoints.begin();
         }

         Resolver::iterator Resolver::end()
         {
            return listOfEndpoints.end();
         }

         /*
          * Creates a list of endpoints that the url points to
          */
         int Resolver::resolve(std::string connectionInfo)
         {
            struct addrinfo hints;
            struct addrinfo *result, *rp;
            int s;

            /* First get rid of the old list */
            listOfEndpoints.clear();

            /*
             * Split up the connectioninfo into host and service.
             * It is of the format <host>:<service>
             */
            std::string host, service;
            size_t pos = connectionInfo.find_last_of(':');
            if (pos != std::string::npos) {
               host = connectionInfo.substr(0,pos);
               service = connectionInfo.substr(pos+1);
            } else {
               common::logger::error << "Endpoint '" << connectionInfo << "' not of correct format <host>:<service>";
               return -1;
            }

            /*
             * Set up the hinting
             */
            memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_family = AF_INET | AF_INET6;     /* Allow only IPv4 or IPv6 */
            hints.ai_socktype = SOCK_STREAM; /* Stream socket */
            hints.ai_flags = AI_CANONNAME;     /* For wildcard IP address */
            hints.ai_protocol = IPPROTO_TCP; /* TCP protocol */
            hints.ai_canonname = NULL;
            hints.ai_addr = NULL;
            hints.ai_next = NULL;

            /*
             * Get the address info
             */
            common::logger::information << "Resolving " << host << ":" << service;
            s = getaddrinfo(host.c_str(), service.c_str(), &hints, &result);
            if (s != 0) {
               common::logger::error << gai_strerror(s);
               return -1;
            }


            /* Loop through all returned addreses */
            for (rp = result; rp != nullptr; rp = rp->ai_next) {

              /* We only support TCP over INET or INET6 */
              if (rp->ai_socktype == SOCK_STREAM) {

                 if (rp->ai_protocol == IPPROTO_TCP) {

                    if (rp->ai_family == AF_INET) {

                       if (rp->ai_canonname!=nullptr)
                          common::logger::information << rp->ai_canonname;
                       listOfEndpoints.push_back(PEndpoint(new EndpointTCPv4(rp->ai_addr, rp->ai_addrlen)));

                    }

                    if (rp->ai_family == AF_INET6) {

                       if (rp->ai_canonname!=nullptr)
                          common::logger::information << rp->ai_canonname;
                       listOfEndpoints.push_back(PEndpoint(new EndpointTCPv6(rp->ai_addr, rp->ai_addrlen)));

                    }

                 }
              }
            }

           /* Free up the allocated data */
           freeaddrinfo(result);

           /* Return with the number of endpoints found */
           return listOfEndpoints.size();
         }

         /*---------------------------------------------------------------------------------------------
          * The Socket class
          *---------------------------------------------------------------------------------------------*/

         /*
          * Constructor of the socket based on an endpoint
          */
         Socket::Socket(PEndpoint pE)
         {
            pEndpoint = pE;
            fd = socket (pEndpoint->family, pEndpoint->type, pEndpoint->protocol);
         }

         /*
          * Constructor of the socket based on a fikle descriptor
          */
         Socket::Socket (int socket, PEndpoint pE)
         {
            fd = socket;
            pEndpoint = pE;
         }

         /*
          * Destructor of the socket
          */
         Socket::~Socket()
         {
            close();
         }

         /*
          * Connects to an endpoint
          */
         int Socket::connect ()
         {
            return ::connect (fd, static_cast<struct sockaddr *>(pEndpoint->m_data.get()), pEndpoint->m_size);
         }

         /*
          * Binds to an endpoint
          */
         int Socket::bind(){
            return ::bind (fd, static_cast<struct sockaddr *>(pEndpoint->m_data.get()), pEndpoint->m_size);
         }

         /*
          * Listens for incoming connections
          */
         int Socket::listen(int backlog)
         {
            return ::listen (fd, backlog);
         }

         /*
          * Accepts incoming connections
          */
         PSocket Socket::accept()
         {
            PEndpoint pE = nullptr;
            PSocket pS = nullptr;
            int new_fd = -1;
            struct sockaddr *pSockaddr = nullptr;
            socklen_t len;

            /* Determine the size of the socket address */
            switch (pEndpoint->family) {
               case AF_INET:
                  len = sizeof (struct sockaddr_in);
                  break;
               case AF_INET6:
                  len = sizeof (struct sockaddr_in6);
                  break;
               default:
                  common::logger::error << "Unsupported protocol family to accept connection on " << pEndpoint->info();
                  len = 0;
                  break;
            }

            /* Valid address length */
            if (len>0) {

               /* Allocate the address container */
               pSockaddr = static_cast<struct sockaddr *>(malloc (len));

               /* Accept the call */
               int n = ::accept (fd, pSockaddr, &len);

               /* Create the endpoint of the connection */
               if (n>=0) {
                  switch (pEndpoint->family) {
                     case AF_INET:
                        pE = PEndpoint(new EndpointTCPv4 (pSockaddr, len));
                        break;
                     case AF_INET6:
                        pE = PEndpoint(new EndpointTCPv6 (pSockaddr, len));
                        break;
                     default:
                        break;
                  }

                  /* Create the socket */
                  pS = PSocket(new Socket (n, pE));

               } else {

                  common::logger::error << "Unable to accept connection on " << pEndpoint->info() << " => " << strerror (errno);

               }

               /* Free the container */
               free (pSockaddr);

            } else {
               common::logger::error << "Unable to allocate memory for accepting connection on " << pEndpoint->info();
            }

            /* Back with the socket */
            return pS;
         }

         /*
          * Closes a connection
          */
         int Socket::close()
         {
            /*
             * TODO: Should we do a shutdown instead? Determine this later, if needed
             */
            return ::close (fd);
         }
      }
   }
}

