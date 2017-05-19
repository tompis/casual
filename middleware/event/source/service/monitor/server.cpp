//!
//! casual 
//!

#include "event/service/monitor/vo/entry.h"

#include "common/error.h"
#include "common/service/invoke.h"
#include "common/service/type.h"
#include "common/environment.h"
#include "common/arguments.h"
#include "common/server/start.h"

#include "sf/service/protocol.h"

#include "sql/database.h"

namespace casual
{
   namespace event
   {
      namespace service
      {
         namespace monitor
         {
            namespace local
            {
               namespace
               {
                  namespace database
                  {
                     std::string name = "monitor.db";
                  } // database



                  std::vector< vo::Entry> select()
                  {
                     const common::Trace trace( "Database::select");

                     auto connection = sql::database::Connection( common::environment::directory::domain() + "/monitor.db");
                     auto query = connection.query( "SELECT service, parentservice, callid, transactionid, start, end FROM calls;");

                     sql::database::Row row;
                     std::vector< vo::Entry> result;

                     while( query.fetch( row))
                     {
                        vo::Entry vo;
                        vo.setService( row.get< std::string>(0));
                        vo.setParentService( row.get< std::string>( 1));
                        sf::platform::Uuid callId( row.get< std::string>( 2));
                        vo.setCallId( callId);
                        //vo.setTransactionId( local::getValue( *row, "transactionid"));

                        std::chrono::microseconds start{ row.get< long long>( 4)};
                        vo.setStart( common::platform::time::point::type{ start});
                        std::chrono::microseconds end{ row.get< long long>( 5)};
                        vo.setEnd( common::platform::time::point::type{ end});
                        result.push_back( vo);
                     }

                     return result;

                  }

                  namespace service
                  {
                     common::service::invoke::Result metrics( common::service::invoke::Parameter&& parameter)
                     {
                        auto protocol = sf::service::protocol::deduce( std::move( parameter));

                        auto result = sf::service::user( protocol, &local::select);

                        protocol << CASUAL_MAKE_NVP( result);
                        return protocol.finalize();
                     }
                  } // service
               } // <unnamed>
            } // local



            void main( int argc, char **argv)
            {
               common::process::path( argv[ 0]);
               //
               // get database from arguments
               //

               {
                  casual::common::Arguments parser{
                     { casual::common::argument::directive( { "-db", "--database"}, "path to monitor database log", local::database::name)}};

                  parser.parse( argc, argv);
               }

               common::server::start( {
                  {
                     ".casual/event/service/metrics",
                     &local::service::metrics,
                     common::service::transaction::Type::none,
                     common::service::category::admin(),
                  }
               });

            }

         } // monitor
      } // service
   } // event
} // casual


int main( int argc, char **argv)
{
   try
   {
      casual::event::service::monitor::main( argc, argv);
      return 0;
   }
   catch( ...)
   {
      return casual::common::error::handler();
   }
}
