//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/manager/admin/cli.h"
#include "gateway/manager/admin/vo.h"
#include "gateway/manager/admin/server.h"

#include "common/argument.h"
#include "common/message/queue.h"
#include "common/terminal.h"
#include "common/chronology.h"
#include "common/transcode.h"
#include "common/exception/handle.h"

#include "sf/service/protocol/call.h"
#include "sf/archive/maker.h"
#include "sf/log.h"

#include "xatmi.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace normalize
      {

         std::string timestamp( const platform::time::point::type& time)
         {
            if( time != platform::time::point::type::min())
            {
               return chronology::local( time);
            }
            return {};
         }




      } // normalize

      namespace call
      {

         manager::admin::vo::State state()
         {
            sf::service::protocol::binary::Call call;
            auto reply = call( manager::admin::service::name::state());

            manager::admin::vo::State result;
            reply >> CASUAL_MAKE_NVP( result);

            return result;
         }

      } // call


      namespace format
      {
         namespace connection
         {


         } // connection

         auto connections()
         {
            using vo = manager::admin::vo::Connection;

            auto format_domain_name = []( const vo& c) { return c.remote.name; };
            auto format_domain_id = []( const vo& c) { return transcode::hex::encode( c.remote.id.get());};

            auto format_pid = []( const vo& c){ return c.process.pid;};
            auto format_queue = []( const vo& c){ return c.process.queue;};

            auto format_bound = []( const vo& c)
            {
               switch( c.bound)
               {
                  case vo::Bound::in: return "in";
                  case vo::Bound::out: return "out";
                  default: return "unknown";
               }
            };

            auto format_type = []( const vo& c)
            {
               switch( c.type)
               {
                  case vo::Type::tcp: return "tcp";
                  case vo::Type::ipc: return "ipc";
                  default: return "unknown";
               }
            };

            auto format_runlevel = []( const vo& c)
            {
               switch( c.runlevel)
               {
                  case vo::Runlevel::connecting: return "connecting";
                  case vo::Runlevel::online: return "online";
                  case vo::Runlevel::shutdown: return "shutdown";
                  case vo::Runlevel::error: return "error";
                  default: return "absent";
               }
            };

            auto format_address = []( const vo& c)
            {
               return string::join( c.address, " ");
            };

            return terminal::format::formatter<  manager::admin::vo::Connection>::construct( 
               terminal::format::column( "name", format_domain_name, terminal::color::yellow),
               terminal::format::column( "id", format_domain_id, terminal::color::blue),
               terminal::format::column( "bound", format_bound, terminal::color::magenta),
               terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "queue", format_queue, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "type", format_type, terminal::color::cyan),
               terminal::format::column( "runlevel", format_runlevel, terminal::color::no_color),
               terminal::format::column( "address", format_address, terminal::color::blue)
            );

         }

      } // format

      namespace action
      {
         
         void list_connections()
         {
            auto state = call::state();

            auto formatter = format::connections();

            formatter.print( std::cout, state.connections);
         }

         void state( const common::optional< std::string>& format)
         {
            auto state = call::state();

            auto archive = sf::archive::writer::from::name( std::cout, format.value_or( ""));
            archive << CASUAL_MAKE_NVP( state);
         }

      } // action

      namespace manager
      {
         namespace admin 
         {
            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  auto complete_state = []( auto values, bool){
                     return std::vector< std::string>{ "json", "yaml", "xml", "ini"};
                  };

                  return common::argument::Group{ [](){}, { "gateway"}, "gateway related administration",
                     common::argument::Option( &gateway::action::list_connections, { "-c", "--list-connections"}, "list all connections"),
                     common::argument::Option( &gateway::action::state, complete_state, {"--state"}, "gateway state"),
                  };
               }
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Group cli::options() &
            {
               return m_implementation->options();
            }
            
         } // admin
      } // manager
   } // gateway
} // casual



