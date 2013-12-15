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
          std::string name="unknown";
          std::string endpoint="*:55555";
          int housekeeping = 10;
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
        * Master socket
        */
       common::ipc::Endpoint endpointMaster; /* The endpoint to our tcp main server */
       std::shared_ptr<common::ipc::Socket> socketMaster = nullptr; /* The socket to our tcp main server */

    };

  } // broker
} // casual

#endif /* STATE_H_ */
