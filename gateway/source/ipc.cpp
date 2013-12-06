/*
 * ipc.cpp
 *
 *  Created on: 9 nov 2013
 *      Author: tomas
 */

#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

/*
 * STL
 */
#include <cstring>
#include <string>
#include <utility>
#include <thread>
#include <condition_variable>
#include <memory>
#include <algorithm>

/*
 * Casual common
 */
#include "common/logger.h"
#include "gateway/std14.h"
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

         /**********************************************************************\
          *  Signal
         \**********************************************************************/

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

         /**********************************************************************\
          *  Endpoint
         \**********************************************************************/

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
            sb << str << ":" << ntohs (pS->sin_port);
            return sb.str();
         }

         std::string Endpoint::infoTCPv6()
         {
            struct sockaddr_in6 *pS = reinterpret_cast<struct sockaddr_in6 *>(m_data.get());
            char str[INET6_ADDRSTRLEN];

            inet_ntop(AF_INET6, &(pS->sin6_addr), str, INET6_ADDRSTRLEN);
            std::stringstream sb;
            sb << str << ":" << ntohs(pS->sin6_port);
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

         /**********************************************************************\
          *  Resolver
         \**********************************************************************/

         /*
          * The iterator functions
          */
         Resolver::const_iterator Resolver::begin()
         {
            return listOfEndpoints.begin();
         }

         Resolver::const_iterator Resolver::end()
         {
            return listOfEndpoints.end();
         }

         Resolver::Resolver (std::string connectionString)
         {
            resolve (connectionString);
         }

         const std::list<Endpoint> &Resolver::get() const
         {
            return listOfEndpoints;
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

         /**********************************************************************\
          *  Socket
         \**********************************************************************/

         /*
          * Constructor of the socket based on an endpoint
          */
         Socket::Socket(Endpoint &p) : pEndpoint (std::make_unique<Endpoint>(p))
         {
            fd = ::socket (pEndpoint->family, pEndpoint->type, pEndpoint->protocol);
            if (fd<0) {
               common::logger::error << "Unable to create socket for " << pEndpoint->info() << " : " << ::strerror(errno);
            } else {

               /* Set socket options */
               int on = 1;
               int n = ::setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&on), sizeof(int));
               if (n<0) {
                  common::logger::error << "Unable to force socket to reuse address " << pEndpoint->info() << " : " << ::strerror(errno);
                  ::close (fd);
                  fd = -1;
               } else {
                  /* Set it nonblocking */
                  int flags  = fcntl(fd,F_GETFL, 0);
                  int rc = fcntl(fd,F_SETFL, flags | O_NONBLOCK);
                  if (rc < 0)
                  {
                     common::logger::error << "Unable to set socket as nonblocking " << pEndpoint->info() << " : " << ::strerror(errno);
                     ::close (fd);
                     fd = -1;
                  }
               }
            }
         }

         /*
          * Constructor of the socket based on a file descriptor
          */
         Socket::Socket (int socket, Endpoint *p)
         {
            /* Copy the endpoint if we got one */
            if (p!=nullptr)
               pEndpoint = std::make_unique<Endpoint>(*p);
            else
               pEndpoint = nullptr;

            /* Set up the file descriptor */
            fd = socket;

            /* Set the reuseaddress option on the socket */
            int on = 1;
            int n = ::setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&on), sizeof(int));
            if (n<0) {
               common::logger::error << "Unable to force socket to reuse address " << pEndpoint->info() << " : " << ::strerror(errno);
               ::close (fd);
               fd = -1;
            } else {

               /* Always make it nonblocking */
               int flags  = fcntl(fd,F_GETFL, 0);
               int rc = fcntl(fd,F_SETFL, flags | O_NONBLOCK);
               if (rc < 0)
               {
                  common::logger::error << "Unable to set socket as nonblocking " << pEndpoint->info() << " : " << ::strerror(errno);
                  fd = -1;
               }
            }
         }

         /*
          * Destructor of the socket
          */
         Socket::~Socket()
         {
            if (fd>=0)
               close();
         }

         /*
          * Connects to an endpoint
          */
         int Socket::connect ()
         {
            int n = -1;
            if (fd>=0) {
               n = ::connect (fd, reinterpret_cast<struct sockaddr *>(pEndpoint->m_data.get()), pEndpoint->m_size);
            } else {
               common::logger::error << "Unable to connect to socket, it is invalid";
            }
            return n;
         }

         /*
          * Binds to an endpoint
          */
         int Socket::bind(){
            int n = -1;
            if (fd>=0) {
               n = ::bind (fd, reinterpret_cast<struct sockaddr *>(pEndpoint->m_data.get()), pEndpoint->m_size);
            } else {
               common::logger::error << "Unable to bind socket, it is invalid";
            }
            return n;
         }

         /*
          * Listens for incoming connections
          */
         int Socket::listen(int backlog)
         {
            int n = ::listen (fd, backlog);
            return n;
         }

         /*
          * Accepts incoming connections
          */
         std::unique_ptr<Socket> Socket::accept()
         {
            std::unique_ptr<Socket> pS = nullptr;
            struct sockaddr *pSockaddr = nullptr;
            socklen_t len = 0;
            int new_fd = -1;

            /* Determine the size of the socket address, we can only do this if we have an enpoint */
            if (pEndpoint != nullptr) {
               switch (pEndpoint->family) {
                  case AF_INET:
                     len = sizeof (struct sockaddr_in);
                     break;
                  case AF_INET6:
                     len = sizeof (struct sockaddr_in6);
                     break;
                  default:
                     common::logger::error << "Unsupported protocol family to accept connection on " << pEndpoint->info();
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
                     Endpoint *p = nullptr;
                     if (pEndpoint != nullptr) {
                        p = new Endpoint(pEndpoint->family, pEndpoint->type, pEndpoint->protocol, pSockaddr, len);
                     }
                     pS = std::unique_ptr<Socket>(new Socket (n, p));
                  }

                  /* Free the container */
                  free (pSockaddr);

               }
            }

            /* Back with the socket */
            return pS;
         }

         /*
          * Return with the endpoint
          */
         Endpoint *Socket::getEndpoint()
         {
            return pEndpoint.get();
         }

         /*
          * Add the event handler
          */
         void Socket::setEventHandler(std::unique_ptr<SocketEventHandler> &pSEH)
         {
            pEventHandler = std::move (pSEH);
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

         /*
          * Polls the socket and return 0 on timeout, -1 on error and 1 if an event occured
          */
         int Socket::poll(int timeout)
         {
            int ready = -1; /* Error */

            /* Do we have a socketeventhandler? */
            if (pEventHandler != nullptr) {

               /* Make the poll mask */
               struct pollfd client[1];
               client[0].fd = fd;
               client[0].events = pEventHandler->events();
               client[0].revents = 0;

               /* Wait for some data */
               ready = ::poll (client, 1, timeout);
               if (ready == 1) {
                  handle (client[0].revents);
               } else {
                  if (ready < 0) {
                     if (pEndpoint != nullptr)
                        common::logger::warning << "Polling " << pEndpoint->info() << " caused error => " << strerror (errno);
                     else
                        common::logger::warning << "Polling caused error => " << strerror (errno);
                  }
               }
            }

            return ready;
         }

         /*
          * Handles an event on the socket
          */
         int Socket::handle (int events)
         {
            int ready = -1; /* Error */

            /* Do we have any socketeventhandler? */
            if (pEventHandler != nullptr) {
               ready = 1;
               pEventHandler->handle (events, *this);
            }

            return ready;
         }

         /*
          * Returns with the socket descriptor
          */
         int Socket::getSocket () const
         {
            return fd;
         }

         /*
          * Returns with the eventhandler, remain ownership of handler
          */
         const SocketEventHandler *Socket::getEventHandler() const
         {
            return pEventHandler.get();
         }

         /*
          * write data
          */
         int Socket::write (void *pData, int size)
         {
            return ::send (fd, pData, size, 0);
         }

         /*
          * read data
          */
         int Socket::read (void *pData, int size)
         {
            return ::recv (fd, pData, size, 0);
         }

         /**********************************************************************\
          *  SocketPair
         \**********************************************************************/

         /*
          * Constructor of a socketpair
          */
         SocketPair::SocketPair ()
         {
            int fd[2]; /* The sockets */

            ::socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
            if (fd<0) {
               common::logger::error << "Unable to create a socketpair for : " << ::strerror(errno);
            } else {

               /* Set it nonblocking */
               int flags  = fcntl(fd[0],F_GETFL, 0);
               int rc = fcntl(fd[0],F_SETFL, flags | O_NONBLOCK);
               if (rc < 0)
               {
                  common::logger::error << "Unable to set the socketpair side A as nonblocking  : " << ::strerror(errno);
                  ::close (fd[0]);
               }

               flags  = fcntl(fd[1],F_GETFL, 0);
               rc = fcntl(fd[1],F_SETFL, flags | O_NONBLOCK);
               if (rc < 0)
               {
                  common::logger::error << "Unable to set the socketpair side B as nonblocking  : " << ::strerror(errno);
                  ::close (fd[1]);
               }

               /* Create the socketwrappers */
               pA = std::unique_ptr<Socket>(new Socket(fd[0]));
               pB = std::unique_ptr<Socket>(new Socket(fd[0]));

               // It does not work with the make_unique :-(, no friend with socket
               //pA = std::make_unique<Socket>(fd[0]);
               //pB = std::make_unique<Socket>(fd[1]);

            }
         }

         /*
          * Destroys socketpair
          */
         SocketPair::~SocketPair()
         {
         }

         /*
          * Getters for the socketpair A and B side
          */
         Socket *SocketPair::getSocketA () const
         {
            /* Create the sockets */
            return pA.get();
         }

         Socket *SocketPair::getSocketB () const
         {
            return pB.get();
         }

         /**********************************************************************\
          *  SocketEventHandler
         \**********************************************************************/

         /*
          * The socket event handler dispatcher
          */
         int SocketEventHandler::handle(int events, Socket &socket)
         {
            int handled = 0;

            /* Data is ready */
            if ((events & POLLIN) != 0) {
               handled |= dataCanBeRead (events, socket);
            }

            /* Data can be written */
            if ((events & POLLOUT) != 0) {
               handled |= dataCanBeWritten (events, socket);
            }

            /* Error */
            if ((events & (POLLERR | POLLNVAL)) != 0) {
               handled |= dataError (events, socket);
            }

            /* Hung up */
            if ((events & POLLHUP) != 0) {
               handled |= dataHangup (events, socket);
            }

            /* Flags to say what we handled */
            return handled;
         }


         /*
          * Base implementation, so we don't require an implementation for all
          */
         int SocketEventHandler::dataCanBeRead(int events, Socket &socket)
         {
            common::logger::warning << "Unimplemented data read event";
            return 0;
         }

         int SocketEventHandler::dataCanBeWritten(int events, Socket &socket)
         {
            common::logger::warning << "Unimplemented data write event";
            return 0;
         }

         int SocketEventHandler::dataError(int events, Socket &socket)
         {
            common::logger::warning << "Unimplemented error event";
            return 0;
         }

         int SocketEventHandler::dataHangup(int events, Socket &socket)
         {
            common::logger::warning << "Unimplemented hangup event";
            return 0;
         }

         /**********************************************************************\
          *  SocketGroup
         \**********************************************************************/

         /*
          * Polls all events from all sockets in the group and call each sockets handle function
          */
         int SocketGroup::poll (int timeout)
         {
            int ready = -1;

            /* Only poll if we got any sockets */
            if (listOfSockets.size()>0) {

               /* Wait for something to happen on the poll filters */
               ready = ::poll (clients.get(), listOfSockets.size(), timeout);
               if (ready > 0) {
                  int n = 0;

                  /* Loop through all clients to see if anything has happened */
                  std::for_each(listOfSockets.begin(), listOfSockets.end(),
                        [&](Socket *pSocket) {
                           if (clients[n].revents!=0)
                              pSocket->handle (clients[n].revents);
                           }
                  );

               } else {
                  if (ready < 0) {
                     common::logger::warning << "Polling socketgroup caused error => " << strerror (errno);
                  }
               }
            }

            return ready;
         }

         /*
          * Generates an array of poll filters
          */
         void SocketGroup::generatePollFilter()
         {
            int n = 0;
            clients.reset (new struct pollfd[listOfSockets.size()]);

            for_each(listOfSockets.begin(),listOfSockets.end(),[&](Socket* socket)
            {
               clients[n].fd = socket->getSocket();
               if (socket->getEventHandler()!=0)
                  clients[n].events = socket->getEventHandler()->events();
               else
                  clients[n].events = 0;
               clients[n].revents = 0;
               n++;
            });

         }

         /*
          * Adds a socket to the group and regenerate the event flag array
          */
         void SocketGroup::addSocket (Socket *pSocket)
         {
            listOfSockets.push_back(pSocket);
            generatePollFilter();
         }

         /*
          * Remove a socket from the group and regenerate the envet flag array
          */
         void SocketGroup::removeSocket (Socket *pSocket)
         {
            listOfSockets.remove(pSocket);
            generatePollFilter();
         }

      }
   }
}

