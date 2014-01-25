/*
 * ipc.cpp
 *
 *  Created on: 9 nov 2013
 *      Author: Tomas Stenlund
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
#include <sstream>
#include <string>
#include <utility>
#include <thread>
#include <condition_variable>
#include <memory>
#include <algorithm>

/*
 * Casual common
 */
#include "common/log.h"
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
				std::unique_lock < std::mutex > lk(m_mutex);
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
			Endpoint::Endpoint(int f, const void *sa, size_t len)
			{
				family = f;
				protocol = IPPROTO_TCP;
				type = SOCK_STREAM;
				copyData(sa, len);
			}

			/*
			 * Copy constructor
			 */
			Endpoint::Endpoint(const Endpoint &other)
			{
				copy(other);
			}

			Endpoint& Endpoint::operator=(const Endpoint&other)
			{
				copy(other);
				return *this;
			}

			void Endpoint::copy(const Endpoint &other)
			{
				family = other.family;
				protocol = other.protocol;
				type = other.type;
				copyData(static_cast<void*>(other.m_data.get()), other.m_size);
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
			std::string Endpoint::info()
			{
				switch (family) {
				case AF_UNIX:
					return infoUNIX();
					break;
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

			std::string Endpoint::infoUNIX()
			{
				std::stringstream sb;
				sb << "local";
				return sb.str();
			}

			std::string Endpoint::infoTCPv4()
			{
				struct sockaddr_in *pS =
						reinterpret_cast<struct sockaddr_in *>(m_data.get());
				char str[INET_ADDRSTRLEN];

				inet_ntop(AF_INET, &(pS->sin_addr), str, INET_ADDRSTRLEN);
				std::stringstream sb;
				sb << str << ":" << ntohs(pS->sin_port);
				return sb.str();
			}

			std::string Endpoint::infoTCPv6()
			{
				struct sockaddr_in6 *pS =
						reinterpret_cast<struct sockaddr_in6 *>(m_data.get());
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
				if (size != 0 && data != 0) {
					m_data = std::unique_ptr<char[]>(new char[size]);
					m_size = size;
					memcpy(m_data.get(), data, size);
				} else {
					m_data = nullptr;
					m_size = 0;
				}
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

			Resolver::Resolver(std::string connectionString)
			{
				resolve(connectionString);
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
					host = connectionInfo.substr(0, pos);
					service = connectionInfo.substr(pos + 1);
				} else {
					common::log::error << "Endpoint '" << connectionInfo
							<< "' not of correct format <host>:<service>"
							<< std::endl;
					return -1;
				}

				/*
				 * Set up the hinting
				 */
				memset(&hints, 0, sizeof(struct addrinfo));
				hints.ai_family = AF_UNSPEC; /* Allow only IPv4 or IPv6 */
				hints.ai_socktype = SOCK_STREAM; /* Stream socket */
				hints.ai_flags = AI_CANONNAME; /* For wildcard IP address */
				hints.ai_protocol = IPPROTO_TCP; /* TCP protocol */
				hints.ai_canonname = nullptr;
				hints.ai_addr = nullptr;
				hints.ai_next = nullptr;

				/*
				 * Get the address info
				 */
				s = getaddrinfo(host.c_str(), service.c_str(), &hints, &result);
				if (s != 0) {
					common::log::error << gai_strerror(s) << std::endl;
					return -1;
				}

				/* Loop through all returned adresses */
				for (rp = result; rp != nullptr; rp = rp->ai_next) {

					/* We only support TCP over INET or INET6 */
					if (rp->ai_socktype == SOCK_STREAM) {

						if (rp->ai_protocol == IPPROTO_TCP) {

							if (rp->ai_family == AF_INET
									|| rp->ai_family == AF_INET6) {

								Endpoint p(rp->ai_family, rp->ai_addr,
										rp->ai_addrlen);
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
			Socket::Socket(Endpoint &e) :
					endpoint(e)
			{
				/* Default in error state */
				state = State::error;
				events = 0;

				/* Create the socket */
				fd = ::socket(endpoint.family, endpoint.type,
						endpoint.protocol);
				if (fd < 0) {
					common::log::error
							<< "Socket::Socket : Unable to create socket for "
							<< endpoint.info() << " : " << ::strerror(errno)
							<< "(" << errno << ")" << std::endl;
				} else {

					/* Set socket options, reuse address */
					int on = 1;
					int n = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
							reinterpret_cast<char *>(&on), sizeof(int));
					if (n < 0) {
						lastError = errno;
						common::log::error
								<< "Socket::Socket : Unable to force socket to reuse address "
								<< endpoint.info() << " : " << ::strerror(errno)
								<< std::endl;
						::close(fd);
						fd = -1;
					} else {

						/* Set it nonblocking */
						int flags = fcntl(fd, F_GETFL, 0);
						int rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
						if (rc < 0) {
							lastError = errno;
							common::log::error
									<< "Socket::Socket : Unable to set socket as nonblocking "
									<< endpoint.info() << " : "
									<< ::strerror(errno) << std::endl;
							::close(fd);
							fd = -1;
						} else {

							/* All created well */
							state = State::initialized;
						}
					}
				}
			}

			/*
			 * Constructor of the socket based on a file descriptor
			 */
			Socket::Socket(int socket, Endpoint &e)
			{
				/* Default in error */
				state = State::error;
				events = POLLRDNORM | POLLWRNORM;

				/* Copy the endpoint if we got one */
				endpoint = e;

				/* Set up the file descriptor */
				fd = socket;

				/* Connected socket */
				state = State::connected;
			}

			/*
			 * Constructor of the socket based on a file descriptor
			 */
			Socket::Socket()
			{
				/* Default */
				events = 0;

				/* Set up the file descriptor */
				fd = -1;

				/* Unknown state of the socket */
				state = State::unknown;
			}

			/*
			 * Destructor of the socket
			 */
			Socket::~Socket()
			{
				if (fd >= 0)
					close();
				common::log::information << "Socket::~Socket : Closed" << std::endl;
			}

			/*
			 * Connects to an endpoint
			 */
			int Socket::connect()
			{
				int n = -1;
				if (fd >= 0) {
					if (state == initialized) {
						n =
								::connect(fd,
										reinterpret_cast<struct sockaddr *>(endpoint.m_data.get()),
										endpoint.m_size);
						if (n < 0) {
							lastError = errno;
							if (errno != EINPROGRESS) {
								events = 0;
								common::log::error
										<< "Socket::connect : Unable to connect to socket "
										<< endpoint.info() << " "
										<< strerror(errno) << "(" << errno
										<< ")" << std::endl;
								state = State::error;
								lastError = errno;
							} else {
								events = POLLWRNORM;
								state = State::connecting;
								n = 0;
							}
						} else {
							common::log::warning
									<< "Socket::connect : We got connected directly, strange ..."
									<< std::endl;
							events = POLLRDNORM | POLLWRNORM;
							state = State::connected;
						}
					} else {
						events = 0;
						common::log::warning << "Socket::connect : Socket "
								<< endpoint.info()
								<< " is not in initialized state, "
								<< getState() << std::endl;
					}
				} else {
					lastError = EBADFD;
					events = 0;
					common::log::error
							<< "Socket::connect : Unable to connect to "
							<< endpoint.info() << ", socket it is invalid"
							<< std::endl;
					state = State::error;
				}
				return n;
			}

			/*
			 * Binds to an endpoint
			 */
			int Socket::bind()
			{
				int n = -1;

				/* We do not need to wait for any specific events */
				events = 0;

				/* Bind it */
				if (fd >= 0) {
					if (state == initialized) {
						n =
								::bind(fd,
										reinterpret_cast<struct sockaddr *>(endpoint.m_data.get()),
										endpoint.m_size);
						if (n < 0) {
							lastError = errno;
							common::log::error
									<< "Socket::bind : Unable to bind socket "
									<< endpoint.info() << " " << strerror(errno)
									<< "(" << errno << ")" << std::endl;
							state = State::error;
						} else
							state = State::bound;
					} else {
						common::log::warning << "Socket::bind : Socket "
								<< endpoint.info()
								<< " is not in initialized state, "
								<< getState() << std::endl;
					}

				} else {
					lastError = EBADFD;
					common::log::error
							<< "Socket::bind : Unable to bind socket "
							<< endpoint.info() << ", it is invalid"
							<< std::endl;
					state = State::error;
				}
				return n;
			}

			/*
			 * Listens for incoming connections
			 */
			int Socket::listen(int backlog)
			{
				events = 0;
				int n = -1;
				if (state == bound) {
					n = ::listen(fd, backlog);
					if (n < 0) {
						lastError = errno;
						common::log::error
								<< "Socket::listen : Unable to listen to socket "
								<< endpoint.info() << " " << strerror(errno)
								<< "(" << errno << ")" << std::endl;
						state = State::error;
					} else {
						events = POLLRDNORM;
						state = State::listening;
					}
				} else {
					common::log::warning << "Socket::connect : Socket "
							<< endpoint.info() << " is not in bound state, "
							<< getState() << std::endl;
				}
				return n;
			}

			/*
			 * Accepts incoming connections
			 */
			int Socket::accept(Socket *pS)
			{
				struct sockaddr *pSockaddr = nullptr;
				socklen_t len = 0;
				int n = -1;

				/* Are we in correct state ? */
				if (state == listening) {

					/* Determine size of the sockaddr */
					switch (endpoint.family) {
					case AF_INET:
						len = sizeof(struct sockaddr_in);
						break;
					case AF_INET6:
						len = sizeof(struct sockaddr_in6);
						break;
					default:
						lastError = EAFNOSUPPORT;
						common::log::error
								<< "Socket::accept : Unsupported protocol family to accept connection on "
								<< endpoint.info() << std::endl;
						state = State::error;
						break;
					}

					/* Valid address length */
					if (len > 0) {

						/* Allocate the address container */
						pSockaddr = static_cast<struct sockaddr *>(malloc(len));

						/* Accept the call */
						n = ::accept(fd, pSockaddr, &len);

						/* Create the endpoint of the connection */
						if (n >= 0) {

							Endpoint p(endpoint.family, pSockaddr, len);

							pS->fd = n;
							pS->endpoint = p;
							pS->state = State::connected;
							pS->setEventMask(POLLWRNORM | POLLRDNORM);
							n = 0;

						} else {

							lastError = errno;
							common::log::error
									<< "Socket::accept : Accept error on "
									<< endpoint.info() << " " << strerror(errno)
									<< "(" << errno << ")" << std::endl;

							state = State::error;

							pS->fd = -1;
							pS->state = State::error;
							pS->setEventMask(0);

						}

						/* Free the container */
						free(pSockaddr);

					}

				} else {
					common::log::warning << "Socket::connect : Socket "
							<< endpoint.info() << " is not in listening state, "
							<< getState() << std::endl;
				}

				/* Back with the status */
				return n;
			}

			/*
			 * Closes a connection
			 */
			int Socket::close()
			{
				int n = -1;

				/* We do not need to listen to any events */
				events = 0;

				/* Close the socket */
				if (fd >= 0) {
					n = ::close(fd);
					if (n < 0) {
						lastError = errno;
						common::log::error << "Socket::close : Close error on "
								<< endpoint.info() << " " << strerror(errno)
								<< "(" << errno << ")" << std::endl;
						state = State::error;
					} else {
						state = State::closed;
						fd = -1;
					}
				}

				return n;
			}

			/*
			 * Polls the socket and return 0 on timeout, -1 on error and 1 if an event occured
			 */
			int Socket::execute(int timeout)
			{
				int ready = -1; /* Error */
				int n = 0;

				common::log::information << "Socket::execute : State="
						<< getStateAsString() << " Waiting for "
						<< dumpEvents(getEventMask()) << std::endl;

				/* We cannot be in error state or in initialized state */
				if (state != State::error && state != State::initialized) {

					/* Make the poll mask */
					struct pollfd client[1];
					client[0].fd = fd;
					client[0].events = events;
					client[0].revents = 0;

					/* Wait for some data */
					dumpEvents(client[0].events);

					/* Poll */
					ready = ::poll(client, 1, timeout);
					if (ready == 1) {

						/* We got some events */
						common::log::information
								<< "Socket::execute : The events that got fired for "
								<< endpoint.info() << " are "
								<< dumpEvents(client[0].revents) << std::endl;

						/* Decide what to do */
						switch (state) {

						/* States with no action */
						case State::unknown:
						case State::error:
						case State::initialized:
						case State::bound:
						case State::closed:
							common::log::warning
									<< "Socket::execute : No event should come in this state"
									<< std::endl;
							break;

							/* We are waiting for a connect from the other side */
						case State::listening:
							state = handleIncomingConnection(client[0].revents);
							break;

							/* We are waiting for an accept from the other side */
						case State::connecting:
							state = handleOutgoingConnection(client[0].revents);
							break;

							/* We are connected, this is data in or out */
						case State::connected:
							state = handleConnected(client[0].revents);
							break;

						}

					} else {

						/* Poll error ? */
						if (ready < 0) {
							lastError = errno;
							state = State::error;
						}

					}

				} else {

					common::log::warning << "Socket::execute : Socket "
							<< endpoint.info() << " is in wrong state "
							<< getState() << std::endl;

				}

				/* Return with the status */
				return ready;
			}

			/*
			 * write data
			 */
			int Socket::write(void *pData, int size)
			{
				return ::send(fd, pData, size, MSG_DONTWAIT);
			}

			/*
			 * read data
			 */
			int Socket::read(void *pData, int size)
			{
				return ::recv(fd, pData, size, MSG_DONTWAIT);
			}

			/*
			 * Return with the current error
			 */
			int Socket::getLastError() const
			{
				int err = 0;
				socklen_t err_size = sizeof(int);
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_size) < 0)
					common::log::warning
							<< "Socket::getLastError: Getsockopt error "
							<< strerror(errno) << std::endl;
				return err;
			}

			/*
			 * Returns with the error that casued the state change
			 */
			int Socket::getStateError() const
			{
				return lastError;
			}

			/*
			 * Sets the state error
			 */
            void Socket::setStateError (int err)
            {
            	lastError = err;
            }

			/*
			 * Return with the event mask
			 */
			int Socket::getEventMask() const
			{
				return events;
			}

			/*
			 * Sets the event mask
			 */
			void Socket::setEventMask(int mask)
			{
				events = mask;
			}

			/*
			 * Return with the state as a string
			 */
			Socket::State Socket::getState() const
			{
				return state;
			}

			/*
			 * Sets the state
			 */
			void Socket::setState(State newState)
			{
				state = newState;
			}

			/*
			 * Return with the state as a string
			 */
			std::string Socket::getStateAsString() const
			{
				std::string s = "";

				switch (state) {
				case State::unknown:
					s = "SocketState:unknown";
					break;
				case State::error:
					s = "SocketState:error(";
					s.append(strerror (lastError));
					s = s + ")";
					break;
				case State::initialized:
					s = "SocketState:initialized";
					break;
				case State::bound:
					s = "SocketState:bound";
					break;
				case State::closed:
					s = "SocketState:closed";
					break;
				case State::listening:
					s = "SocketState:listening";
					break;
				case State::connecting:
					s = "SocketState:listening";
					break;
				case State::connected:
					s = "SocketState:connected";
					break;
				default:
					s = "SocketState:none";
					break;

				}
				return s;
			}

			/*
			 * Returns true if socket is in initialized state
			 */
			bool Socket::isInitialized() const
			{
				return state == State::initialized;
			}

			/*
			 * Returns true if sockes is in error state
			 */
			bool Socket::hasError() const
			{
				return state == State::error;
			}

			/*
			 * Dump event socket flags
			 */
			std::string Socket::dumpEvents(int events) const
			{
				std::stringstream s;

				s << "Events =";
				if ((events & POLLIN) != 0) {
					s << " POLLIN";
				}
				if ((events & POLLRDNORM) != 0) {
					s << " POLLRDNORM";
				}
				if ((events & POLLRDBAND) != 0) {
					s << " POLLRDBAND";
				}
				if ((events & POLLPRI) != 0) {
					s << " POLLPRI";
				}
				if ((events & POLLOUT) != 0) {
					s << " POLLOUT";
				}
				if ((events & POLLWRNORM) != 0) {
					s << " POLLWRNORM";
				}
				if ((events & POLLWRBAND) != 0) {
					s << " POLLWRBAND";
				}
				if ((events & POLLERR) != 0) {
					s << " POLLERR";
				}
				if ((events & POLLHUP) != 0) {
					s << " POLLHUP";
				}
				if ((events & POLLNVAL) != 0) {
					s << " POLLNVAL";
				}

				return s.str();
			}

		}
	}
}

