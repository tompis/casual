/*
 * client.cpp
 *
 *  Created on: 16 dec 2013
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
#include <string>
#include <utility>
#include <thread>
#include <condition_variable>
#include <memory>
#include <algorithm>
#include <thread>

/*
 * Casual common
 */
#include "common/log.h"
#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/state.h"
#include "gateway/master.h"
#include "gateway/client.h"
#include "gateway/messages.h"

/*
 * Casual namespace
 */
namespace casual
{
	namespace gateway
	{

		/**********************************************************************\
		 *  ClientState
		 \**********************************************************************/

		ClientState::ClientState(State &s) :
			m_global(s)
		{

		}

		/*
		 * Returns with the state of the client as a string
		 */
		std::string ClientState::getState()
		{
			std::string s = "ClientState::unknown";

			switch (state) {
			case initialized:
				s = "ClientState::";
				break;
			case connecting:
				s = "ClientState::connecting";
				break;
			case retrying:
				s = "ClientState::retry";
				break;
			case active:
				s = "ClientState::active";
				break;
			case failed:
				s = "ClientState::failed";
				break;
			}
			return s;
		}

		/**********************************************************************\
		 *  ClientSocket
		 \**********************************************************************/

		/*
		 * Creates a client socket based on an endpoint that it connects to
		 */
		ClientSocket::ClientSocket(ClientState &s, common::ipc::Endpoint &p) :
			CASLSocket(p), m_state(s)
		{
		}

		/*
		 * Creates a client socket based on no endpoint.
		 */
		ClientSocket::ClientSocket(ClientState &s) :
			CASLSocket(), m_state(s)
		{
		}

		/*
		 * Shuts down a connect handler
		 */
		ClientSocket::~ClientSocket()
		{
		}

		/*
		 * Handle the message
		 */
		bool ClientSocket::handleMessage()
		{
			common::log::information << "ClientSocket::handleMessage : Incoming message arrived" << std::endl;
			return true;
		}

		/**********************************************************************\
		 *  ClientThread
		 \**********************************************************************/

		/*
		 * Creates a client to a remote gateway based on an incoming connection from the remote gateway. The other
		 * parameter, i,e. the socket is the listening socket that has the incoming socket. We need to accept the
		 * connection. This client is not restartable, because the connection originates on the far end.
		 */
		ClientThread::ClientThread(State &s, common::ipc::Socket *pSocket) :
			m_state(s)
		{
			/* we are initialized and can be run */
			m_state.state = ClientState::MachineState::fatal;
			m_state.localRemoteName = "";
			m_state.localRemoteURL = "";
			bRestartable = false;

			/* Create an empty socket and accept the connection */
			m_state.socket = std::make_unique<ClientSocket>(m_state);
			int n = pSocket->accept(m_state.socket.get());
			if (n < 0) {
				common::log::warning << "ClientThread::ClientThread : Incoming connection not accepted" << std::endl;
			} else {
				common::log::information << "ClientThread::ClientThread : Incoming connection accepted" << std::endl;
				m_state.state = ClientState::MachineState::active;
			}

		}

		/*
		 * Creates a client to a remote gateway based on configuration, this client reconnects in case of
		 * a failure and is also restartable because it originates from this gateway not the remote one (far end).
		 */
		ClientThread::ClientThread(State &s, configuration::RemoteGateway &remote) :
			m_state(s)
		{
			/* We are in a starting state */
			m_state.state = ClientState::MachineState::fatal;
			m_state.localRemoteName = remote.name;
			m_state.localRemoteURL = remote.endpoint;
			bRestartable = true;

			/* Resolve the bind address for the gateway */
			common::ipc::Resolver resolver;
			if (resolver.resolve(m_state.localRemoteURL) < 0) {

				/* Unable to resolve address, there is no idea to start */
				common::log::error << "ClientThread::ClientThread : Unable to resolve address " << remote.endpoint
					<< " for " << m_state.localRemoteName << std::endl;

			} else {

				/* Save the endpoint */
				m_state.endpoint = resolver.get().front();
				m_state.state = ClientState::MachineState::initialized;
			}
		}

		/*
		 * Destructor
		 */
		ClientThread::~ClientThread()
		{
			/* Stop the thread */
			if (thread != nullptr) {
				stop();
			}
		}

		/*
		 * Starts the ClientThread. Returns true if the thread has been started.
		 */
		bool ClientThread::start()
		{
			bool bStarted = false;
			common::log::information << "ClientThread::start : Entered" << std::endl;

			/* Can we start and are we not already running ? */
			if (m_state.state != ClientState::MachineState::fatal && m_state.state != ClientState::MachineState::failed
				&& thread == nullptr) {

				/* Allow it to run */
				bRun = true;
				bStarted = true;

				/* Start the thread */
				common::log::information << "ClientThread::start : Thread started" << std::endl;
				thread = std::make_unique<std::thread>(&ClientThread::loop, this);

			}

			common::log::information << "ClientThread::start : Exited" << std::endl;

			return bStarted;
		}

		/*
		 * Stops the ClientThread. Returns true if the Client thread has been stopped.
		 */
		bool ClientThread::stop()
		{
			bool bStopped = false;
			common::log::information << "ClientThread::stop : Entered" << std::endl;

			/* Allow it to stop */
			bRun = false;

			/* Are we running ? */
			if (thread != nullptr) {

				/* Wait for the thread to finish */
				common::log::information << "ClientThread::stop : Waiting for thread to finish" << std::endl;
				thread->join();
				thread = nullptr;
				bStopped = true;
			}

			common::log::information << "ClientThread::stop : Exited" << std::endl;

			return bStopped;
		}

		/*
		 * Determine if the thread has exited, either by stopping it or by an error condition.
		 */
		bool ClientThread::hasExited()
		{
			return bExited;
		}

		/*
		 * True if it has been started
		 */
		bool ClientThread::hasStarted()
		{
			return bRun;
		}

		/*
		 * True if it is restartable
		 */
		bool ClientThread::isRestartable()
		{
			return bRestartable;
		}

		/*
		 * The current state
		 */
		ClientState::MachineState ClientThread::getMachineState()
		{
			return m_state.state;
		}

		/*
		 * The name of the thread
		 */
		std::string ClientThread::getName()
		{
			return m_state.localRemoteName;
		}

		/*
		 * Connects the client to the remote gateway
		 */
		bool ClientThread::connect()
		{
			bool bOK = true;

			/* Create the socket */
			common::log::information << "ClientThread::connect : Connecting to endpoint " << m_state.endpoint.info()
				<< std::endl;
			m_state.socket = std::make_unique<ClientSocket>(m_state, m_state.endpoint);

			/* Connect to the remote endpoint */
			if (m_state.socket->connect() < 0) {
				/* If we are in progress, then that is ok, otherwise we failed */
				bOK = false;
				common::log::error << "ClientThread::connect : Unable to connect to " << m_state.endpoint.info()
					<< ", fatal " << strerror(errno) << "(" << errno << ")" << std::endl;
			} else {
				common::log::information << "ClientThread::connect : Connection to " << m_state.endpoint.info()
					<< " initialized" << std::endl;
			}

			/* Return with the status */
			return bOK;
		}

		/*
		 * The Client threads thread loop
		 */
		void ClientThread::loop()
		{
			int status;

			common::log::information << "ClientThread::loop : Entered" << std::endl;

			/* Connection loop, run until we do not want it to run anymore or if it has reached a
			 * fatal state */
			while (bRun && m_state.state != ClientState::MachineState::fatal) {

				/* Execute the master state machine */
				switch (m_state.state) {

				case ClientState::MachineState::failed:

				{
					/* Determine what to do */
					int err = m_state.socket->getStateError();
					if (err == EHOSTUNREACH || err == ECONNREFUSED || err == EHOSTDOWN || err == ETIMEDOUT
						|| err == ECONNRESET)
						m_state.state = ClientState::MachineState::retrying;
					else
						m_state.state = ClientState::MachineState::fatal;
				}
					break;

				case ClientState::MachineState::retrying:

					/* Wait for an amount of time to do a reconnect */
					m_state.socket = nullptr;
					{
						std::chrono::milliseconds duration(m_state.m_global.configuration.clientreconnecttime);
						std::this_thread::sleep_for(duration);
					}

					/* Move the state to initialized */
					m_state.state = ClientState::MachineState::initialized;
					break;

				case ClientState::MachineState::initialized:

					/* Initialize the connection */
					if (connect())
						m_state.state = ClientState::MachineState::connecting;
					else
						m_state.state = ClientState::MachineState::failed;

					break;

				default:

					/* Execute the socket for all other states */
					status = m_state.socket->execute(m_state.m_global.configuration.clienttimeout);
					if (status < 0) {

						/* Some polling error, stop this */
						int err = m_state.socket->getStateError();
						common::log::information << "ClientThread::loop : Error during poll, " << strerror(err) << "("
							<< err << ")" << std::endl;
						m_state.state = ClientState::failed;

					} else {

						if (status == 0) {
							/* Timeout, do some house keeping if we need to. Otherwise just go around again */
						}
					}

					/* If the socket is in error, then the client will enter failed state */
					if (m_state.socket->getState() == Socket::State::error) {
						m_state.state = ClientState::MachineState::failed;
					}

				}
			}

			/* Exit */
			bExited = true;
			common::log::information << "ClientThread::loop : Exited" << std::endl;
		}
	}
}
