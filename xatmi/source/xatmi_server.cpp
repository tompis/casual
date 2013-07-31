//!
//! xatmi_server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#include "xatmi_server.h"


#include <vector>
#include <algorithm>


#include "common/service_context.h"
#include "common/server_context.h"
#include "common/message_dispatch.h"


#include "common/error.h"
#include "common/logger.h"
#include "common/environment.h"


using namespace casual;


namespace local
{
   namespace
   {
	   namespace transform
	   {

         struct ServerArguments
         {
            common::server::Arguments operator ()( struct casual_server_argument& value) const
            {
               common::server::Arguments result;

               auto service = value.services;

               while( service->functionPointer != nullptr)
               {
                  result.m_services.emplace_back( service->name, service->functionPointer);
                  ++service;
               }

               auto xaSwitch = value.xaSwitches;

               while( xaSwitch->xaSwitch != nullptr)
               {
                  result.resources.emplace_back( xaSwitch->key, xaSwitch->xaSwitch);
                  ++xaSwitch;
               }

               result.m_argc = value.argc;
               result.m_argv = value.argv;

               result.m_server_init = value.serviceInit;
               result.m_server_done = value.serviceDone;

               return result;

            }
         };
	   } // transform
	} // <unnamed>
} // local



int casual_start_server( casual_server_argument* serverArgument)
{
   try
   {


      auto arguments = local::transform::ServerArguments()( *serverArgument);

      common::environment::setExecutablePath( serverArgument->argv[ 0]);

      //
      // Start the message-pump
      //
      common::message::dispatch::Handler handler;

      handler.add< common::callee::handle::Call>( arguments);

      common::message::dispatch::pump( handler);
	}
	catch( ...)
	{
	   return casual::common::error::handler();
	}
	return 0;
}








