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
#include "common/logger.h"
#include "common/queue.h"
#include "common/message.h"
#include "common/message_dispatch.h"
#include "common/signal.h"

#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/listener.h"
#include "gateway/gateway.h"
#include "gateway/master.h"
#include "gateway/client.h"

#include "sf/archive_maker.h"

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
     namespace policy {

        /*
         * The Gateway policy class
         *
         */

        /*
         * Policy constructor
         */
        Gateway::Gateway( casual::gateway::State& state) : m_state( state) {}


        /*
         * Policy apply
         */
        void Gateway::apply()
        {
           try
           {
              throw;
           }
           catch(const common::exception::signal::Timeout& timeout)
           {
              common::logger::information << "Timeout - resetting it";

              /*
               * Reset the alarm
               */
              casual::common::signal::alarm::set (m_state.configuration.queuetimeout);
           }
        }
     }

    /*
    ** Gateway class
    */

    /*
    ** Gateway constructor
    */ 
    Gateway::Gateway() : m_gatewayQueueFile("/tmp/casual/gateway")
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
    void Gateway::boot ()
    {

       /*
        * Sets the housekeeping alarm, i.e when to do housekeeping, sending heartbeats to remote gateways
        * and stuff like that.
        */
       common::signal::alarm::set (m_state.configuration.queuetimeout);

       /*
        * Start the master TCP server
        */
       m_state.masterThread = std::make_unique<MasterThread>(m_state);
       m_state.masterThread->start();

       /*
        * Start up all the TCP clients, one for each remote gateway
        */
       {
          std::lock_guard<std::mutex> lock(m_state.listOfClientsMutex);
          for (auto &gw : m_state.configuration.remotegateways) {

             /* Create the thread */
             std::unique_ptr<ClientThread> ct = std::make_unique<ClientThread>(m_state, gw);
             ct->start();

             /* Add the thread to the list of remote gateway threads */
             m_state.listOfClients.push_back(std::move (ct));

          }
       }

    }

    /*
     * Shuts down the gateway nicely
     */
    void Gateway::shutdown ()
    {
       /*
        * Cancel all alarms
        */
       common::signal::alarm::set (0);

       /*
        * Stop all clients
        */
       {
          std::lock_guard<std::mutex> lock(m_state.listOfClientsMutex);

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
    ** Starts the gateway
    */
    void Gateway::start( const Settings& arguments)
    {
       /*
        * Read the configuration
        */
       configuration::Gateway& gateway = m_state.configuration;
       auto reader = sf::archive::reader::makeFromFile(arguments.configurationFile);
       reader >> CASUAL_MAKE_NVP(gateway);
       common::logger::information << "Gateway " << m_state.configuration.name << " starting up";

       /*
        * Set up the gateway
        */
       boot();

       /*
        * Wait for the queue to come again
        */
       GatewayQueueBlockingReader blockingReader( m_receiveQueue, m_state);
       casual::common::message::dispatch::Handler handler;

       /* Loop until time ends */
       try {
          while( true)
          {
             auto marshal = blockingReader.next();

             if( ! handler.dispatch( marshal))
             {
                common::logger::error << "message_type: " << marshal.type() << " not recognized - action: discard";
             }
          }
       }
       catch (...) {
          shutdown();
          throw;
       }

    }
        
  } // gateway
  
} // casual
