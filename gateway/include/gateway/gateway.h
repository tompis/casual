//!
//! gateway.h
//!
//! Created on: Oct 9, 2013
//!     Author: dnulnets
//!

#ifndef CASUAL_GATEWAY_H_
#define CASUAL_GATEWAY_H_

/*
** Casual header files
*/
#include "common/file.h"
#include "common/platform.h"

#include "common/ipc.h"
#include "common/message.h"
#include "common/environment.h"

#include "sf/namevaluepair.h"

#include "gateway/state.h"

/*
** Standard libraries
*/
#include <string>
#include <map>

/*
** Namespace casual::gateway
*/
namespace casual
{
  namespace gateway
  {

    /*
     * The Gateway policy
     */
    namespace policy {

       /*
        * The gateway policy
        *
        * TODO: What state do we need to send in here ???? We set it to int until we know what to send in.
        */
       struct Gateway
       {
          /*
           * Constructor
           */
          Gateway( casual::gateway::State& state);

          /*
           * The gateway policy apply
           */
          void apply();

       private:

          /*
           * The state of the gateway
           */
          casual::gateway::State& m_state;
       };

    }

    /*
     * The blocking reader for the input queue
     */
    typedef common::queue::blocking::basic_reader< policy::Gateway> GatewayQueueBlockingReader;

    /*
    ** The gateway class
    */
    class Gateway
    {
    public:
      
       /* Singleton */
      static Gateway& instance();
      ~Gateway();

      /*
       * Starts the gateway
       */
      void start( const Settings& arguments);
      
    private:
      Gateway();

      /*
       * Boot and shutdown internal functions
       */
      void boot ();
      void shutdown();

      /*
       * State of the gateway
       */
      State m_state;

      /*
       * Other stuff
       */
      common::file::ScopedPath m_gatewayQueueFile;
      common::ipc::receive::Queue& m_receiveQueue = common::ipc::getReceiveQueue();

    };
  } // broker
} // casual

#endif /* CASUAL_GATEWAY_H_ */
