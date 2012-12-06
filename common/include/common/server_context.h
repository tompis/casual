//!
//! casual_server_context.h
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_

#include "common/service_context.h"
#include "common/message.h"
#include "common/ipc.h"

#include "utility/platform.h"


#include "xatmi.h"

//
// std
//
#include <unordered_map>

namespace casual
{
   namespace common
   {
      namespace server
      {

         class Context
         {
         public:
            typedef std::unordered_map< std::string, service::Context> service_mapping_type;

            static Context& instance();

            Context( const Context&) = delete;

            void add( const service::Context& context);

            int start();

            void longJumpReturn( int rval, long rcode, char* data, long len, long flags);

            void advertiseService( const std::string& name, tpservice function);

            void unadvertiseService( const std::string& name);


         private:

            Context();

            void connect();

            void disconnect();

            void handleServiceCall( message::service::Call& context);

            message::server::Id getId();

            void cleanUp();


            service_mapping_type m_services;

            ipc::send::Queue& m_brokerQueue;
            ipc::receive::Queue& m_queue;


            utility::platform::long_jump_buffer_type m_long_jump_buffer;

            message::service::Reply m_reply;

         };

      } // server
	} // common
} // casual



#endif /* CASUAL_SERVER_CONTEXT_H_ */
