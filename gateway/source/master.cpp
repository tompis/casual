/*
 * master.cpp
 *
 *  Created on: 15 dec 2013
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
#include <thread>

/*
 * Casual
 */
#include "common/log.h"
#include "common/marshal.h"

/*
 * Gateway
 */
#include "gateway/wire.h"
#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/state.h"
#include "gateway/client.h"
#include "gateway/master.h"

/*
 * Casual namespace
 */
namespace casual
{
	namespace gateway
	{

		/**********************************************************************\
		 *  MasterState
		 \**********************************************************************/

		/*
		 * State initialization
		 */
		MasterState::MasterState(State &s) :
			m_global(s)
		{
		}

		/**********************************************************************\
		 *  MasterSocket
		 \**********************************************************************/

		/*
		 * Master connect handler initialization
		 */
		MasterSocket::MasterSocket(MasterState &s, Endpoint &p) :
			Socket(p), m_state(s)
		{
		}

		/*
		 * Master connect handler destruction
		 */
		MasterSocket::~MasterSocket()
		{
		}

		/*
		 * Handles an incoming request
		 */
		Socket::State MasterSocket::handleIncomingConnection(int events)
		{
			Socket::State ret = getState();
			common::log::information << "MasterSocket::handleIncomingConnection : Entered" << std::endl;

			/* POLLERR */
			if ((events & POLLERR) != 0) {

				/* What error did we get ? */
				int err = getLastError();
				common::log::error << "MasterSocket::handleIncomingConnection : Error " << strerror(err) << "(" << err
					<< ")" << std::endl;

				/* Socket enters error state */
				ret = Socket::State::error;
				setStateError (err);
				return ret;
			}

			/* POLLHUP */
			if ((events & POLLHUP) != 0) {

				common::log::error
					<< "MasterSocket::handleIncomingConnection : Hangup should never happen on this type of socket"
					<< std::endl;

				/* The other side has hung up, retry connection */
				ret = Socket::State::error;
				setStateError (ECONNRESET);
				return ret;
			}

			/* POLLRDNORM - UGLY, do I need the masterThreads state really ???? */
			if ((events & POLLRDNORM) != 0 && m_state.state == MasterState::MachineState::active) {

				/* Accept the connection */
				common::log::information << "MasterSocket::handleIncomingConnection : Accepting incoming connection"
					<< std::endl;

				/* Start a client thread */
				std::unique_ptr<ClientThread> clientThread = std::make_unique<ClientThread>(m_state.m_global, this);
				clientThread->start();

				/* Add the thread to the master list */
				std::lock_guard < std::mutex > lock(m_state.m_global.listOfClientsMutex);
				m_state.m_global.listOfClients.push_back(std::move(clientThread));

			}

			common::log::information << "MasterSocket::handleIncomingConnection : Exited" << std::endl;
			return ret;

		}

		/*
		 * Called when the connecting state has an outgoing connect and we have an incoming accept.
		 * This is not a scenario in the master socket.
		 */
		Socket::State MasterSocket::handleOutgoingConnection(int events)
		{
			common::log::information << "MasterSocket::handleOutgoingConnection : Entered" << std::endl;
			common::log::warning << "MasterSocket::handleOutgoingConnection : Unexpected event" << std::endl;
			common::log::information << "MasterSocket::handleOutgoingConnection : Exited" << std::endl;
			setStateError (EPERM);
			return Socket::State::error;
		}

		/*
		 * Handle events when we are in connected state, usually reading and writing data to and from the socket.
		 * This is not a scenario in the master socket.
		 */
		Socket::State MasterSocket::handleConnected(int events)
		{
			common::log::information << "MasterSocket::handleConnected : Entered" << std::endl;
			common::log::warning << "MasterSocket::handleConnected : Unexpected event" << std::endl;
			common::log::information << "MasterSocket::handleConnected : Exited" << std::endl;
			setStateError (EPERM);
			return Socket::State::error;
		}

		/**********************************************************************\
		 *  MasterThread
		 \**********************************************************************/

		/*
		 * Create and initialize the master thread.
		 */
		MasterThread::MasterThread(State &s) :
			m_state(s)
		{
			m_state.state = MasterState::MachineState::fatal;
			Resolver resolver;

			/* Resolve the bind address for the gateway */
			if (resolver.resolve(m_state.m_global.configuration.endpoint) < 0) {

				/*
				 * Unable to resolve the address, gateway cannot start
				 */
				common::log::error << "MasterThread::MasterThread : Unable to resolve address "
					<< m_state.m_global.configuration.endpoint << std::endl;

			} else {

				/*
				 * Save the endpoint, we use the first one now but we could get
				 * the next in the list, but we do not do that just now. TODO!!!!
				 */
				m_state.endpoint = resolver.get().front();
				m_state.state = MasterState::MachineState::initialized;

			}
		}

		/*
		 * Destructor
		 */
		MasterThread::~MasterThread()
		{
			if (thread != nullptr) {
				stop();
			}
		}

		/*
		 * Starts the master thread. Returns true if the thread has been started.
		 */
		bool MasterThread::start()
		{
			bool bStarted = false;
			common::log::information << "Masterthread::start : Entered" << std::endl;

			/* Can we start and are we not already running ? */
			if (m_state.state != MasterState::MachineState::failed && m_state.state != MasterState::MachineState::fatal
				&& thread == nullptr) {

				/* Change state to running */
				bRun = true;
				bExited = false;

				/* Start the thread */
				common::log::information << "Masterthread::start : Thread started" << std::endl;
				thread = std::make_unique<std::thread>(&MasterThread::loop, this);
				bStarted = true;
			}

			common::log::information << "Masterthread::start : Exited" << std::endl;

			return bStarted;
		}

		/*
		 * Stops the master thread. Returns true if the master thread has been stopped.
		 */
		bool MasterThread::stop()
		{
			bool bStopped = false;
			common::log::information << "Masterthread::stop : Entered" << std::endl;

			/* Stop the thread */
			bRun = false;

			/* Are we running ? */
			if (thread != nullptr) {

				/* Wait for the thread to finish */
				common::log::information << "Masterthread::stop : Waiting for thread to finish" << std::endl;
				thread->join();
				thread = nullptr;
				bStopped = true;
			}

			common::log::information << "Masterthread::stop : Exited" << std::endl;

			return bStopped;
		}

		/*
		 * Determine if the thread has exited, either by stopping it or by an error condition.
		 */
		bool MasterThread::hasExited()
		{
			return bExited;
		}

		/*
		 * True if it has been started
		 */
		bool MasterThread::hasStarted()
		{
			return bRun;
		}

		/*
		 * True if it is restartable, the master is never restartable. If it fails the gateway fails.
		 */
		bool MasterThread::isRestartable()
		{
			false;
		}

		/*
		 * The current state
		 */
		MasterState::MachineState MasterThread::getMachineState()
		{
			return m_state.state;
		}

		/*
		 * The master threads thread loop
		 */
		void MasterThread::loop()
		{
			int status;

			common::log::information << "Masterthread::loop : Entered" << std::endl;

			/* Never ending loop */
			while (bRun && m_state.state != MasterState::MachineState::fatal) {

				/* Run the state machine */
				switch (m_state.state) {

					case MasterState::MachineState::failed:

						/* For now everything that causes a failure is fatal */
						m_state.state = MasterState::MachineState::fatal;
						break;

					case MasterState::MachineState::initialized:

						/* Create the socket and start the bindin and listening attempt */
						m_state.socket = std::make_unique<MasterSocket>(m_state, m_state.endpoint);
						if (m_state.socket->getState() == Socket::State::initialized) {

							/*
							 * Set up the socket to start accepting incoming calls
							 */;
							int n = m_state.socket->bind();
							if (n >= 0) {
								n = m_state.socket->listen();
							}

							/* Did it go well ? */
							if (n < 0) {

								/* Something went wrong */
								common::log::error << "MasterThread::loop : Unable to bind and listen to socket on "
									<< m_state.endpoint.info() << std::endl;
								int err = m_state.socket->getStateError();
								common::log::error << "MasterThread::loop : " << strerror(err) << "(" << err << ")"
									<< std::endl;
								m_state.state = MasterState::MachineState::fatal;

							} else {

								/* We are active */
								common::log::information << "MasterThread::loop : Bound and listening on "
									<< m_state.endpoint.info() << std::endl;
								m_state.state = MasterState::MachineState::active;

							}

						} else {

							/* We failed */
							common::log::error << "MasterThread::loop : Unable to create socket for "
								<< m_state.endpoint.info() << "," << m_state.socket->getState() << std::endl;
							m_state.state = MasterState::MachineState::fatal;
						}
						break;

					default:

						/* Poll all sockets */
						status = m_state.socket->execute(m_state.m_global.configuration.clienttimeout);
						if (status < 0) {

							/* Something went wrong */
							common::log::error << "Masterthread::loop : Polling error : " << strerror(errno) << "("
								<< errno << ")" << std::endl;
							m_state.state = MasterState::MachineState::failed;

						} else {

							/* Normal timeout or did we actually get some events that got handled */
							if (status == 0) {

								/* Timeout loop, here we can do any house keeping functions if needed */

							}

						}
						break;
				}

				/* If socket is in error, then fail */
				if (m_state.socket->getState() == Socket::State::error) {

					/* Thread cannot function */
					m_state.state = MasterState::MachineState::failed;

				}

			}

			/* Thread has finished */
			bExited = true;
			common::log::information << "Masterthread::loop : Exited" << std::endl;
		}
	}
}
