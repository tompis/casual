//## includes protected section begin [.10]


#include "monitor/statisticimplementation.h"

//## includes protected section end   [.10]

//
// xatmi
//
#include <xatmi.h>

//
// sf
//
#include "sf/server.h"
#include "sf/service.h"


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


extern "C"
{
   int tpsvrinit(int argc, char **argv);
   void tpsvrdone();
   
   
   void getMonitorStatistics( TPSVCINFO *transb);
   
   
}

namespace local
{
   namespace
   {
      typedef casual::statistics::monitor::StatisticImplementation implementation_type;

      casual::sf::server::type server;
      casual::sf::server::implementation::type< implementation_type> implementation;
   }
}



int tpsvrinit(int argc, char **argv)
{
   try
   {
      local::server = casual::sf::server::create( argc, argv);

      local::implementation = casual::sf::server::implementation::make< local::implementation_type>( argc, argv);
   }
   catch( ...)
   {
      // TODO
   }

   return 0;
}

void tpsvrdone()
{
   //
   // delete the implementation an server implementation
   //
   casual::sf::server::sink( local::implementation);
   casual::sf::server::sink( local::server);
}

//
// Services provided
//



void getMonitorStatistics( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
   
     
      auto service_io = local::server->createService( serviceInfo);

      //
      // Instantiate and serialize input parameters
      //
            
      

      //## input protected section begin [2000.110]
      using namespace casual::statistics::monitor;
      //## input protected section end   [2000.110]


      //
      // Instantiate the output parameters
      //
            
      std::vector< vo::MonitorVO> outputValues;

      //## output protected section begin [2000.120]
      //## output protected section end   [2000.120]


      //
      // Call the implementation
      //
      
      bool serviceReturn = service_io.call( 
         *local::implementation, 
         &local::implementation_type::getMonitorStatistics, 
         outputValues);
      
      
      //
      // Serialize output
      //
            
      service_io << CASUAL_MAKE_NVP( serviceReturn);
      service_io << CASUAL_MAKE_NVP( outputValues);

      //## output protected section begin [2000.200]
      //## output protected section end   [2000.200]

      reply = service_io.finalize();
   }
   catch( ...)
   {
      local::server->handleException( serviceInfo, reply);
   }

   tpreturn(
      reply.value,
      reply.code,
      reply.data,
      reply.size,
      reply.flags);
}
	
	