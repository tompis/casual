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
				Signal(const Signal &other) = delete;
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
				friend class SocketPair;
				friend class Resolver;

			public:

				/*
				 * Normal constructor, empty endpoint
				 */
				Endpoint() = default;

				/*
				 * Destructor
				 */
				virtual ~Endpoint();

				/*
				 * Copy constructor
				 */
				Endpoint(const Endpoint &other);
				Endpoint& operator=(const Endpoint&other);

				/*
				 * Information of the endpoint in string format
				 */
				std::string info();

			private:

				/*
				 * Constructor, only for friends.
				 *
				 * family and socket address
				 */
				Endpoint(int f, const void *sa, size_t len);

				/*
				 * The POSIX data
				 */
				std::unique_ptr<char[]> m_data = nullptr;
				std::size_t m_size = 0;
				int family = AF_UNIX;
				int protocol = IPPROTO_TCP;
				int type = SOCK_STREAM;

				/*
				 * Information string builders for various families. UGLY!
				 */
				std::string infoTCPv4();
				std::string infoTCPv6();
				std::string infoUNIX();

				/*
				 * POSIX helper function that copies address info
				 */
				void copyData(const void *data, std::size_t size);

				/*
				 * Copy and assignment operator helper
				 */
				void copy(const Endpoint &other);

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
				Resolver(std::string connectionString);
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
				Resolver(const Resolver &other) = delete;
				Resolver &operator=(const Resolver &other) = delete;

				/*
				 * Resolves a connection string to one or several endpoints. Use the iterator to traverse the list
				 *
				 * Returns number of endpoints this connection resolved to or less than zero on failure.
				 */
				int resolve(std::string connectionString);

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

				/*
				 * Socket states
				 */
				enum State {
					unknown,
					initialized = 1,
					bound,
					listening,
					connecting,
					connected,
					closed,
					error = 100
				};

				/* Friends, so we can encapsulate posix much more */
				friend SocketGroup;
				friend SocketPair;

				/*
				 * Constructors and destructors
				 */
				Socket();
				Socket(Endpoint &e);
				virtual ~Socket();

				/*
				 * No copy and assignment possible with sockets, there can only be one!
				 */
				Socket(const Socket &other) = delete;
				Socket &operator=(const Socket &other) = delete;

				/*
				 * Close the socket, returns -1 on error, zero otherwise. Leaves socket in closed state.
				 */
				int close();

				/*
				 * Connect to the current endpoint, returns -1 on error, zero otherwise. Leaves socket in connecting state and
				 * once the connection is established in connected.
				 */
				int connect();

				/*
				 * Bind the socket to an endpoint, returns -1 on error, zero otherwise. Leaves the socket in bound state.
				 */
				int bind();

				/*
				 * Listen to an bound socket, returns -1 on error, zero otherwise. Leaves the socket in listening state.
				 */
				int listen(int backlog = SOMAXCONN);

				/*
				 * Accept incoming connection and move it to the socket given as a paramter and set it to connected state.
				 */
				int accept(Socket *pS);

				/*
				 * Executes the socket state machine.
				 *
				 * Returns 0 on timeout, 1 on event occured and -1 on error.
				 */
				int execute(int timeout);

				/*
				 * write data
				 */
				int write(void *pData, int size);

				/*
				 * read data
				 */
				int read(void *pData, int size);

				/*
				 * Returns the string of the sockets current state
				 */
				std::string getStateAsString() const;

				/*
				 * Returns with the sockets current state
				 */
				State getState() const;

				/*
				 * Returns with the error associated with that caused the error state
				 */
				int getStateError() const;

				/*
				 * Returns with a string of events
				 */
				std::string dumpEvents(int events) const;

				/*
				 * Returns true if sockes is in initialized state
				 */
				bool isInitialized() const;

				/*
				 * Returns true if socket is in in error state
				 */
				bool hasError() const;

			protected:

				/*
				 * Called when the listening state has an incoming connection that we need to accept.
				 */
				virtual State handleIncomingConnection(int events) = 0;

				/*
				 * Called when the connecting state has an outgoing connect and we have an incoming accept.
				 */
				virtual State handleOutgoingConnection(int events) = 0;

				/*
				 * Handle events when we are in connected state, usually reading and writing data to and from the socket.
				 */
				virtual State handleConnected(int events) = 0;

				/*
				 * Sets or gets the events mask that the socket waits for during polling
				 */
				int getEventMask() const;

				/*
				 * Sets the eventmask
				 */
				void setEventMask(int mask);

				/*
				 * Sets the state
				 */
				void setState(State newState);

				/*
				 * Sets the state error
				 */
				void setStateError(int err);

				/*
				 * Returns with the error associated with the socket not the state
				 */
				int getLastError() const;

			private:

				/*
				 * Socket created from a file descriptor. We have no Endpoint. We assume that
				 * the socket is in connected state.
				 */
				Socket(int fd, Endpoint &e);

				/*
				 * Socket file descriptor
				 */
				int fd;

				/*
				 * Eventmask containing which events to wait for
				 */
				int events = POLLRDNORM | POLLWRNORM;

				/*
				 * Socket state
				 */
				State state;
				int lastError = 0;

				/*
				 * Socket endpoint
				 */
				Endpoint endpoint;

			};

			/**********************************************************************\
			 *  SocketPair
			 \**********************************************************************/

			class SocketPair {

			public:

				/*
				 * Constructor and destructor
				 */
				SocketPair();
				~SocketPair();

				/*
				 * No copy and assignment possible with sockets, there can only be one!
				 */
				SocketPair(const SocketPair &other) = delete;
				SocketPair &operator=(const SocketPair &other) = delete;

				/*
				 * Getters for the socketpair A and B side, moves the ownership!!!!
				 */
				std::unique_ptr<Socket> getSocketA();
				std::unique_ptr<Socket> getSocketB();

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
				SocketGroup(const SocketGroup &other) = delete;
				SocketGroup &operator=(const SocketGroup &other) = delete;

				/*
				 * Polls all sockets
				 *
				 * Returns 0 on timeout, 1 on event occured and -1 on error and calls the eventhandler for all
				 * sockets in the group.
				 */
				int poll(int timeout);

				/*
				 * Add a socket, ownership is not taken
				 */
				void addSocket(std::shared_ptr<Socket> pSocket);
				void removeSocket(std::shared_ptr<Socket> pSocket);

			protected:
			private:

				/*
				 * Creates a execute filter for the POSIX execute
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
				SocketEventHandler(const SocketEventHandler &other) = delete;
				SocketEventHandler &operator=(const SocketEventHandler &other) = delete;

				/*
				 * The handle function
				 */
				int handle(int events, Socket &socket);

				/*
				 * Types this handler handles
				 */
				virtual int events() const = 0;

			protected:

				/*
				 * Functions that gets called whenever an event occurs for the socket
				 */
				virtual int dataCanBeRead(int events, Socket &socket);
				virtual int dataCanBeWritten(int events, Socket &socket);
				virtual int hangup(int events, Socket &socket);
				virtual int error(int events, Socket &socket);

			};

		}
	}
}

#endif /* CASUAL_GATEWAY_IPC_H_ */
