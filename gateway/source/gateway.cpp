//!
//! gateway.cpp
//!
//! Created on: Okt 8, 2013
//!     Author: dnulnets
//!

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
 ** Standard casual header files
 */
#include "config/domain.h"
#include "common/environment.h"
#include "common/log.h"
#include "common/queue.h"
#include "common/message/type.h"
#include "common/message/dispatch.h"
#include "common/signal.h"

#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/gateway.h"
#include "gateway/master.h"
#include "gateway/client.h"

#include "sf/archive/maker.h"

/*
 ** Standard libraires
 */
#include <fstream>
#include <algorithm>

/*
 ** namespace casual::gateway
 */
namespace casual
{
	namespace gateway
	{
		/*
		 * The policy when signals occurs
		 */
		namespace policy
		{

			/*
			 * The Gateway policy class
			 *
			 */

			/*
			 * Policy constructor
			 */
			Gateway::Gateway(casual::gateway::State& state) :
					m_state(state)
			{
			}

			/*
			 * Policy apply
			 */
			void Gateway::apply()
			{
				try {
					throw;
				} catch (const common::exception::signal::Timeout& timeout) {
					gateway::Gateway::houskeeping(m_state);

					/*
					 * Reset the alarm
					 */
					casual::common::signal::alarm::set(
							m_state.configuration.queuetimeout);
				}
			}
		}

		/*
		 ** Gateway class
		 */

		/*
		 ** Gateway constructor
		 */
		Gateway::Gateway() :
				m_gatewayQueueFile("/tmp/casual/gateway")
		{
		}

		/*
		 ** Gateway singleton
		 */
		Gateway& Gateway::instance()
		{
			static Gateway singleton;
			return singleton;
		}

		/*
		 ** Destructor for the gateway
		 */
		Gateway::~Gateway()
		{
		}

		/*
		 * Boots the gateway
		 */
		void Gateway::boot()
		{

			/*
			 * Sets the housekeeping alarm, i.e when to do housekeeping.
			 */
			common::signal::alarm::set(m_state.configuration.queuetimeout);

			/*
			 * Start the master TCP server. This server listens to this gateways socket and spawn of
			 * a new thread for each connecting remote gateway.
			 */
			m_state.masterThread = std::make_unique<MasterThread>(m_state);
			m_state.masterThread->start();

			/*
			 * Start up all the TCP clients, one for each remote gateway
			 */
			{
				std::lock_guard < std::mutex > lock(m_state.listOfClientsMutex);
				for (auto &gw : m_state.configuration.remotegateways) {

					/* Create the thread for the client so it can connect to the remote gateway */
					std::unique_ptr<ClientThread> ct = std::make_unique<
							ClientThread>(m_state, gw);
					ct->start();

					/* Add the thread to the list of remote gateway threads */
					m_state.listOfClients.push_back(std::move(ct));

				}
			}

		}

		/*
		 * Shuts down the gateway nicely
		 */
		void Gateway::shutdown()
		{
			/*
			 * Cancel all alarms
			 */
			common::signal::alarm::set(0);

			/*
			 * Stop all clients
			 */
			{
				std::lock_guard < std::mutex > lock(m_state.listOfClientsMutex);

				for (auto &ct : m_state.listOfClients) {
					ct->stop();
				}
			}

			/*
			 * Stop the master thread
			 */
			m_state.masterThread->stop();

		}

		/*
		 * Static housekeeping function
		 */
		void Gateway::houskeeping(State &m_state)
		{
			common::log::information
					<< "Gateway::housekeeping : Housekeeping started"
					<< std::endl;
			/*
			 * Master thread houskeeping
			 */
			{
				common::log::information
						<< "Gateway::housekeeping : Checking master"
						<< std::endl;
			}

			/*
			 * Client Thread houskeeping
			 */
			{
				std::lock_guard < std::mutex > m(m_state.listOfClientsMutex);

				common::log::information
						<< "Gateway::housekeeping : Checking all clients, there are "
						<< m_state.listOfClients.size() << " clients started"
						<< std::endl;
				std::list<std::unique_ptr<ClientThread>>::iterator i =
						m_state.listOfClients.begin();
				while (i != m_state.listOfClients.end()) {
					std::unique_ptr<ClientThread> &ct = *i;

					/* Has the thread exited ? */
					if (ct->hasExited()) {

						common::log::information
								<< "Gateway::housekeeping : Client "
								<< ct->getName() << " has exited" << std::endl;
						/*
						 * Is the thread restartable, this is only if we are the initiatior, not incoming gateway
						 * connections. They are restarted by the remote gateway.
						 */
						if (ct->isRestartable()) {

							/* Do the thread still thinks it is running, but some other condition has caused it to stop?
							 * Because if it thinks it is still running then some error condition caused it to stop and
							 * we should try to restart it because we want it to run. */
							if (ct->hasStarted()) {

								/* If it is not fatal, then restart it. Fatal error is errors that makes it impossible to
								 * restart. Such as faulty IP-addresses etc. */
								if (ct->getMachineState()
										!= ClientState::fatal) {

									/* Find the remote gateway configuration */
									configuration::Gateway::RemoteGatewayList::iterator p =
											std::find(
													m_state.configuration.remotegateways.begin(),
													m_state.configuration.remotegateways.end(),
													ct->getName());

									/* Did we get any remote gateway connection information ? */
									if (p
											!= m_state.configuration.remotegateways.end()) {

										/* Create a new thread */
										common::log::information
												<< "Gateway::housekeeping : Restarting client for "
												<< ct->getName() << std::endl;
										*i = std::move(
												std::make_unique<ClientThread>(
														m_state, *p));
										if (i->get()->start()) {

											common::log::information
													<< "Gateway::housekeeping : Client "
													<< i->get()->getName()
													<< " restarted"
													<< std::endl;
											i++;
										} else {

											common::log::warning
													<< "Gateway::housekeeping : Client "
													<< i->get()->getName()
													<< " failed to start, removing it"
													<< std::endl;
											i = m_state.listOfClients.erase(i);
										}

									} else {

										/* Unable to find out how to connect to it, stop it it is fatal */
										common::log::warning
												<< "Gateway::housekeeping : Unable to find connection information for "
												<< ct->getName()
												<< " removing it" << std::endl
												<< std::endl;
										i = m_state.listOfClients.erase(i);
									}

								} else {

									common::log::warning
											<< "Gateway::housekeeping : Unable to restart connection to "
											<< ct->getName()
											<< " it has fatal error"
											<< std::endl;

									/* Next element */
									i = m_state.listOfClients.erase(i);

								}

							} else {

								/* Nope, no one has started it yet, so do not restart it */
								common::log::information
										<< "Gateway::housekeeping : We do not restart "
										<< ct->getName()
										<< " it has not been previously started, removed"
										<< std::endl;

								/* Next element */
								i = m_state.listOfClients.erase(i);

							}

						} else {

							/* No, this is not restartable */
							common::log::information
									<< "Gateway::housekeeping : Removing the client "
									<< ct->getName()
									<< " it is restarted by remote gateway"
									<< std::endl;

							/* Next element */
							i = m_state.listOfClients.erase(i);

						}

					} else {

						/* Next element, no problems with this client */
						i++;
					}

				} /* while */
			}

			common::log::information
					<< "Gateway::housekeeping : Housekeeping ended"
					<< std::endl;

		}

		/*
		 ** Starts the gateway
		 */
		void Gateway::start(const Settings& arguments)
		{
			/*
			 * Read the configuration
			 */
			configuration::Gateway& gateway = m_state.configuration;
			auto reader = sf::archive::reader::makeFromFile(
					arguments.configurationFile);
			reader >> CASUAL_MAKE_NVP(gateway);
			common::log::information << "Gateway " << m_state.configuration.name
					<< " starting up" << std::endl;

			/*
			 * Set up the gateway
			 */
			boot();

			/*
			 * Wait for the queue to come again
			 */
			GatewayQueueBlockingReader blockingReader(m_receiveQueue, m_state);
			casual::common::message::dispatch::Handler handler;

			/* Loop until time ends */
			try {
				while (true) {
					auto marshal = blockingReader.next();

					if (!handler.dispatch(marshal)) {
						common::log::error << "message_type: " << marshal.type()
								<< " not recognized - action: discard"
								<< std::endl;
					}
				}
			} catch (...) {
				common::log::information
						<< "Exception caught, cleaning up and exiting"
						<< std::endl;
				shutdown();
				throw;
			}

		}

	} // gateway

} // casual
