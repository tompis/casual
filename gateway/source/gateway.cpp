//!
//! gateway.cpp
//!
//! Created on: Okt 8, 2013
//!     Author: dnulnets
//!

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

#include "gateway/gateway.h"
#include "gateway/ipc.h"

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
              casual::common::signal::alarm::set (m_state.configuration.housekeeping);
           }
        }
     }

    /*
    ** Gateway class
    */

    /*
    ** Gateway constructor
    */ 
    Gateway::Gateway()
      : m_gatewayQueueFile("/tmp/casual/gateway")
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
       common::signal::alarm::set (m_state.configuration.housekeeping);

    }

    /*
     * SHuts down the gateway nicely
     */
    void Gateway::shutdown ()
    {
       /*
        * Cancel all alarms
        */
       common::signal::alarm::set (0);

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

       common::ipc::Resolver resolver;
       if (resolver.resolve (m_state.configuration.connectioninformation)<0) {
          common::logger::error << "Unable to resolve address";
       }
       if (resolver.resolve ("localhost:12345")<0) {
          common::logger::error << "Unable to resolve address";
       }
    }
        
  } // gateway
  
} // casual
