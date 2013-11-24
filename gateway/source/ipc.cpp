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
#include <netinet/in.h>
#include <arpa/inet.h>

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
         Endpoint::Endpoint (int f, int t, int p, const void *sa, size_t len)
         {
            family = f;
            protocol = p;
            type = t;
            copyData (sa, len);
         }

         /*
          * Copy constructor
          */
         Endpoint::Endpoint ( const Endpoint &other)
         {
            copy (other);
         }

         Endpoint& Endpoint::operator=( const Endpoint&other)
         {
            copy (other);
            return *this;
         }

         void Endpoint::copy (const Endpoint &other)
         {
            family = other.family;
            protocol = other.protocol;
            type = other.type;
            copyData (static_cast<void*>(other.m_data.get()), other.m_size);
         }

         /*
          * Destroys an endpoint
          */
         Endpoint::~Endpoint()
         {
         }

         /*
          * Information string builder
          */
         std::string Endpoint::info ()
         {
            switch (family) {
               case AF_INET:
                  return infoTCPv4();
                  break;
               case AF_INET6:
                  return infoTCPv6();
                  break;
               default:
                  return "Unknown";
                  break;
            }
         }

         std::string Endpoint::infoTCPv4 ()
         {
            struct sockaddr_in *pS = reinterpret_cast<struct sockaddr_in *>(m_data.get());
            char str[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &(pS->sin_addr), str, INET_ADDRSTRLEN);
            std::stringstream sb;
            sb << "TCPv4 " << str << ":" << ntohs (pS->sin_port);
            return sb.str();
         }

         std::string Endpoint::infoTCPv6()
         {
            struct sockaddr_in6 *pS = reinterpret_cast<struct sockaddr_in6 *>(m_data.get());
            char str[INET6_ADDRSTRLEN];

            inet_ntop(AF_INET6, &(pS->sin6_addr), str, INET6_ADDRSTRLEN);
            std::stringstream sb;
            sb << "TCPv6 " << str << ":" << ntohs(pS->sin6_port);
            return sb.str();
         }

         /*
          * Used by the resolver to set the data
          */
         void Endpoint::copyData(const void *data, std::size_t size)
         {
            m_data = std::unique_ptr<char[]>(new char[size]);
            m_size = size;
            memcpy (m_data.get(), data, size);
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
            struct addrinfo *result = nullptr;
            struct addrinfo *rp;
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
            hints.ai_family = AF_UNSPEC;     /* Allow only IPv4 or IPv6 */
            hints.ai_socktype = SOCK_STREAM; /* Stream socket */
            hints.ai_flags = AI_CANONNAME;     /* For wildcard IP address */
            hints.ai_protocol = IPPROTO_TCP; /* TCP protocol */
            hints.ai_canonname = nullptr;
            hints.ai_addr = nullptr;
            hints.ai_next = nullptr;

            /*
             * Get the address info
             */
            common::logger::information << "Resolving " << host << ":" << service;
            s = getaddrinfo(host.c_str(), service.c_str(), &hints, &result);
            if (s != 0) {
               common::logger::error << gai_strerror(s);
               return -1;
            }


            /* Loop through all returned adresses */
            for (rp = result; rp != nullptr; rp = rp->ai_next) {

              /* We only support TCP over INET or INET6 */
              if (rp->ai_socktype == SOCK_STREAM) {

                 if (rp->ai_protocol == IPPROTO_TCP) {

                    if (rp->ai_family == AF_INET || rp->ai_family == AF_INET6) {

                       Endpoint p(rp->ai_family, SOCK_STREAM, IPPROTO_TCP, rp->ai_addr, rp->ai_addrlen);
                       common::logger::information << p.info();
                       listOfEndpoints.push_back(p);

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
         Socket::Socket(Endpoint &p) : endpoint (p)
         {
            fd = socket (endpoint.family, endpoint.type, endpoint.protocol);
         }

         /*
          * Constructor of the socket based on a fikle descriptor
          */
         Socket::Socket (int socket, Endpoint &p) : endpoint (p), fd (socket)
         {
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
            return ::connect (fd, reinterpret_cast<struct sockaddr *>(endpoint.m_data.get()), endpoint.m_size);
         }

         /*
          * Binds to an endpoint
          */
         int Socket::bind(){
            return ::bind (fd, reinterpret_cast<struct sockaddr *>(endpoint.m_data.get()), endpoint.m_size);
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
            PSocket pS = nullptr;
            int new_fd = -1;
            struct sockaddr *pSockaddr = nullptr;
            socklen_t len;

            /* Determine the size of the socket address */
            switch (endpoint.family) {
               case AF_INET:
                  len = sizeof (struct sockaddr_in);
                  break;
               case AF_INET6:
                  len = sizeof (struct sockaddr_in6);
                  break;
               default:
                  common::logger::error << "Unsupported protocol family to accept connection on " << endpoint.info();
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
                  Endpoint p(AF_INET, SOCK_STREAM, IPPROTO_TCP, pSockaddr, len);
                  pS = PSocket(new Socket (n, p));
               } else {
                  common::logger::error << "Unable to accept connection on " << endpoint.info() << " => " << strerror (errno);
               }

               /* Free the container */
               free (pSockaddr);

            } else {
               common::logger::error << "Unable to allocate memory for accepting connection on " << endpoint.info();
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

