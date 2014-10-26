


#include "transaction/manager/admin/server.h"
#include "transaction/manager/manager.h"

//
// xatmi
//
#include <xatmi.h>

//
// sf
//
#include "sf/server.h"
#include "sf/service.h"



namespace local
{
   namespace
   {
      typedef casual::transaction::admin::Server implementation_type;

      casual::sf::server::type server;
      casual::sf::server::implementation::type< implementation_type> implementation;
   }
}


extern "C"
{

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

   casual::sf::server::sink( local::implementation);
   casual::sf::server::sink( local::server);
}


void listTransactions_( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
      auto service_io = local::server->createService( serviceInfo);

      std::vector< vo::Transaction> serviceReturn = service_io.call(
         *local::implementation,
         &local::implementation_type::listTransactions);

      service_io << CASUAL_MAKE_NVP( serviceReturn);

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


} // extern "C"

namespace casual
{
   namespace transaction
   {
      namespace admin
      {

         common::server::Arguments Server::services( Manager& manager)
         {
            m_manager = &manager;

            common::server::Arguments result{ { common::process::path()}};

            result.services.emplace_back( ".casual.tm.list.transactions", &listTransactions_, common::server::Service::Type::cCasualAdmin, common::server::Service::cNone);

            return result;
         }

         Server::Server( int argc, char **argv)
         {

         }

         Server::~Server()
         {

         }




         std::vector< vo::Transaction> Server::listTransactions()
         {
            std::vector< vo::Transaction> result;


            return result;
         }

         Manager* Server::m_manager = nullptr;


      } // admin
   } // transaction
} // casual
