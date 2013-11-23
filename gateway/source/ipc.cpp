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

/*
 * STL
 */
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
         Endpoint::Endpoint (int f, int t, int p, sockaddr *sa, socklen_t len)
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
          * Constructor of the socket
          */
         Socket::Socket(PEndpoint pE)
         {
            pEndpoint = pE;
            fd = socket (pEndpoint->family, pEndpoint->type, pEndpoint->protocol);
         }

         /*
          * Destructor of the socket
          */
         Socket::~Socket()
         {
            close();
         }

         int Socket::connect ()
         {
            return ::connect (fd, static_cast<struct sockaddr *>(pEndpoint->m_data.get()), pEndpoint->m_size);
         }

         int Socket::bind(){
            return ::bind (fd, static_cast<struct sockaddr *>(pEndpoint->m_data.get()), pEndpoint->m_size);
         }

         int Socket::listen(int backlog)
         {
            return ::listen (fd, backlog);
         }

         PSocket Socket::accept()
         {
            sockaddr *pData = static_cast<sockaddr*>(malloc (pEndpoint->m_size));
            socklen_t len;
            int n = ::accept (fd, pData, &len);
            if (n<0) {
               free (pData);
               return nullptr;
            } else {
               PEndpoint pE = PEndpoint(new Endpoint (pEndpoint->family, pEndpoint->type, pEndpoint->protocol, pData, len));
               PSocket pS = PSocket(new Socket (pE));
               free (pData);
            }
         }

         int Socket::close()
         {
            return ::close (fd);
         }
      }
   }
}

