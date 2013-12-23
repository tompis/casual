/*
 * state.h
 *
 *  Created on: 21 okt 2013
 *      Author: dnulnets
 */

#ifndef STATE_H_
#define STATE_H_

/*
** Casual header files
*/
#include "common/environment.h"
#include "sf/namevaluepair.h"

/*
** Standard libraries
*/
#include <string>
#include <map>
#include <mutex>
#include <regex>

#include "gateway/ipc.h"

/*
** Namespace casual::gateway
*/
namespace casual
{
  namespace gateway
  {

    /*
     * The gateway configuration structure
     */
    namespace configuration {

       /*
        * Remote gateway configurations
        */
       struct RemoteGateway {
          std::string name;
          std::string endpoint;

          template< typename A>
          void serialize( A& archive)
          {
             archive & CASUAL_MAKE_NVP(name);
             archive & CASUAL_MAKE_NVP(endpoint);
          }
       };

       /*
        * Local gateway configuration
        */
       struct Gateway {
          std::string name="noname";
          std::string endpoint="0.0.0.0:55555";
          int clientreconnecttime = 5000; /* Default 5 seconds reconnect time for clients */
          int clienttimeout = 1000; /* Default 1 second timeout for sockets */
          int queuetimeout = 1; /* Default 1 second timeout for queue */

          std::vector<RemoteGateway> remotegateways;

          template< typename A>
          void serialize( A& archive)
          {
             archive & CASUAL_MAKE_NVP(name);
             archive & CASUAL_MAKE_NVP(endpoint);
             archive & CASUAL_MAKE_NVP(remotegateways);
          }
       };

    }

    /*
     * Class predeclaration
     */
    class ClientThread;
    class MasterThread;

    /*
    ** The gateway settings structure
    */
    struct Settings
    {
      std::string configurationFile = common::file::find( common::environment::directory::domain() + "/configuration", std::regex( "gateway.(yaml|xml)" ));
    };

    /*
     * The gateway state class
     */
    struct State {

       /*
        * Server UUID
        */
       casual::common::Uuid server_uuid = casual::common::Uuid::make();

       /*
        * The configuration
        */
       configuration::Gateway configuration;

       /*
        * The master threads
        */
       std::unique_ptr<MasterThread> masterThread = nullptr;

       /*
        * The list of client threads
        */
       std::mutex listOfClientsMutex; /* Mutex to manipulate the list */
       std::list<std::unique_ptr<ClientThread>> listOfClients;

    };

  } // broker
} // casual

#endif /* STATE_H_ */
