//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/admin/cli.h"

#include "queue/manager/admin/queuevo.h"
#include "queue/manager/admin/services.h"

#include "queue/api/queue.h"
#include "queue/common/transform.h"

#include "common/argument.h"
#include "common/message/queue.h"
#include "common/terminal.h"
#include "common/chronology.h"
#include "common/transcode.h"
#include "common/execute.h"
#include "common/exception/handle.h"

#include "sf/service/protocol/call.h"
#include "sf/archive/maker.h"
#include "sf/log.h"

#include "xatmi.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace queue
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

         manager::admin::State state()
         {
            sf::service::protocol::binary::Call call;
            auto reply = call( manager::admin::service::name::state());

            manager::admin::State result;
            reply >> CASUAL_MAKE_NVP( result);

            return result;
         }

         std::vector< manager::admin::Message> messages( const std::string& queue)
         {
            sf::service::protocol::binary::Call call;
            call << CASUAL_MAKE_NVP( queue);
            auto reply = call( manager::admin::service::name::list_messages());

            std::vector< manager::admin::Message> result;
            reply >> CASUAL_MAKE_NVP( result);

            return result;
         }

      } // call


      namespace format
      {
         auto messages()
         {
            auto format_state = []( const manager::admin::Message& v)
               {
                  switch( v.state)
                  {
                     case 1: return 'E';
                     case 2: return 'C';
                     case 3: return 'D';
                     default: return '?';
                  }
               };

            auto format_trid = []( const manager::admin::Message& v) { return transcode::hex::encode( v.trid);};
            auto format_type = []( const manager::admin::Message& v) { return v.type;};
            auto format_timestamp = []( const manager::admin::Message& v) { return normalize::timestamp( v.timestamp);};
            auto format_available = []( const manager::admin::Message& v) { return normalize::timestamp( v.available);};

            return terminal::format::formatter< manager::admin::Message>::construct(
               terminal::format::column( "id", std::mem_fn( &manager::admin::Message::id), terminal::color::yellow),
               terminal::format::column( "S", format_state, terminal::color::no_color),
               terminal::format::column( "size", std::mem_fn( &manager::admin::Message::size), terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "trid", format_trid, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "rd", std::mem_fn( &manager::admin::Message::redelivered), terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "type", format_type, terminal::color::no_color),
               terminal::format::column( "reply", std::mem_fn( &manager::admin::Message::reply), terminal::color::no_color),
               terminal::format::column( "timestamp", format_timestamp, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "available", format_available, terminal::color::blue, terminal::format::Align::right)
            );
         }

         auto groups()
         {
            auto format_pid = []( const manager::admin::Group& g) { return g.process.pid;};
            auto format_ipc = []( const manager::admin::Group& g) { return g.process.queue;};

            return terminal::format::formatter< manager::admin::Group>::construct(
               terminal::format::column( "name", std::mem_fn( &manager::admin::Group::name), terminal::color::yellow),
               terminal::format::column( "pid", format_pid, terminal::color::grey, terminal::format::Align::right),
               terminal::format::column( "ipc", format_ipc, terminal::color::grey, terminal::format::Align::right),
               terminal::format::column( "queuebase", std::mem_fn( &manager::admin::Group::queuebase))
            );
         }

         auto restored()
         {
            auto format_name = []( const queue::restore::Affected& a) { return a.queue;};

            return terminal::format::formatter< queue::restore::Affected>::construct(
               terminal::format::column( "name", format_name, terminal::color::yellow, terminal::format::Align::right),
               terminal::format::column( "restored", std::mem_fn( &queue::restore::Affected::restored), terminal::color::green)
            );
         }


         auto queues( const manager::admin::State& state)
         {
            using q_type = manager::admin::Queue;


            /*
            auto format_error = [&]( const q_type& q){
               return algorithm::find_if( state.queues, [&]( const q_type& e){ return e.id == q.error && e.group == q.group;}).at( 0).name;
            };
            */

            auto format_type = [&]( const q_type& q){
               switch( q.type)
               {
                  case q_type::Type::group_error_queue: return 'g';
                  case q_type::Type::error_queue: return 'e';
                  case q_type::Type::queue: return 'q';
               }
               return '-';
            };

            auto format_group = [&]( const q_type& q){
               return algorithm::find_if( state.groups, [&]( const manager::admin::Group& g){ return q.group == g.process.pid;}).at( 0).name;
            };


            return terminal::format::formatter< manager::admin::Queue>::construct(
               terminal::format::column( "name", std::mem_fn( &q_type::name), terminal::color::yellow),
               terminal::format::column( "count", []( const auto& q){ return q.count;}, terminal::color::green, terminal::format::Align::right),
               terminal::format::column( "size", []( const auto& q){ return q.size;}, common::terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "avg", []( const auto& q){ return q.count == 0 ? 0 : q.size / q.count;}, common::terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "uc", []( const auto& q){ return q.uncommitted;}, common::terminal::color::grey, terminal::format::Align::right),
               terminal::format::column( "updated", []( const q_type& q){ return normalize::timestamp( q.timestamp);}),
               terminal::format::column( "r", []( const auto& q){ return q.retries;}, common::terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "t", format_type, common::terminal::color::blue),
               terminal::format::column( "group", format_group)
            );
         }

         namespace remote
         {
            auto queues( const manager::admin::State& state)
            {
               auto format_domain_id = [&]( const auto& q){
                  return uuid::string( algorithm::find_if( state.remote.domains, [&]( const auto& d){ return d.process.pid == q.pid;}).front().id.id);
               };
   
               auto format_domain_name = [&]( const auto& q){
                  return algorithm::find_if( state.remote.domains, [&]( const auto& d){ return d.process.pid == q.pid;}).front().id.name;
               };
   
   
               return terminal::format::formatter< manager::admin::remote::Queue>::construct(
                  terminal::format::column( "name", std::mem_fn( &manager::admin::remote::Queue::name), terminal::color::yellow),
                  terminal::format::column( "domain name", format_domain_name, common::terminal::color::blue),
                  terminal::format::column( "domain id", format_domain_id, common::terminal::color::blue)
               );
            }
            
         } // remote

      } // format


      void list_queues()
      {

         auto state = call::state();

         auto formatter = format::queues( state);

         formatter.print( std::cout, algorithm::sort( state.queues));
      }

      void list_remote_queues()
      {
         auto state = call::state();
         
         auto formatter = format::remote::queues( state);

         formatter.print( std::cout, algorithm::sort( state.remote.queues));

      }

      void list_groups()
      {
         auto state = call::state();

         auto formatter = format::groups();

         formatter.print( std::cout, state.groups);
      }

      void list_messages( const std::string& queue)
      {
         auto messages = call::messages( queue);

         auto formatter = format::messages();

         formatter.print( std::cout, messages);
      }

      namespace local
      {
         namespace
         {
            void enqueue( const std::string& queue)
            {
               tx_begin();

               auto rollback = execute::scope( [](){
                  tx_rollback();
               });

               queue::Message message;

               message.attributes.reply = queue;
               message.payload.type = common::buffer::type::binary();

               while( std::cin)
               {
                  message.payload.data.push_back( std::cin.get());
               }

               auto id = queue::enqueue( queue, message);

               tx_commit();
               rollback.release();

               std::cout << id << '\n';
            }

            struct Empty : public std::runtime_error
            {
               using std::runtime_error::runtime_error;
            };

            void dequeue( const std::string& queue)
            {
               tx_begin();

               auto rollback = execute::scope( [](){
                  tx_rollback();
               });

               const auto message = queue::dequeue( queue);

               tx_commit();
               rollback.release();

               if( message.empty())
               {
                  throw Empty{ "queue is empty"};
               }
               else
               {
                  std::cout.write(
                        message.front().payload.data.data(),
                        message.front().payload.data.size());
               }
               std::cout << '\n';
            }


            void restore( const std::vector< std::string>& queueus)
            {
               auto affected = queue::restore::queue( queueus);

               auto formatter = format::restored();
               formatter.print( std::cout, affected);
            }

            void state( const common::optional< std::string>& format)
            {
               auto state = call::state();

               auto archive = sf::archive::writer::from::name( format.value_or( ""));
               archive << CASUAL_MAKE_NVP( state);
            }
         } // <unnamed>
      } // local

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

                  return common::argument::Group{ [](){}, { "queue"}, "queue related administration",
                     common::argument::Option( &queue::list_queues, { "-q", "--list-queues"}, "list information of all queues in current domain"),
                     common::argument::Option( &queue::list_remote_queues, { "-r", "--list-remote"}, "list all remote discovered queues"),
                     common::argument::Option( &queue::list_groups, { "-g", "--list-groups"}, "list information of all groups in current domain"),
                     common::argument::Option( &queue::list_messages, { "-m", "--list-messages"}, "list information of all messages of a queue"),
                     common::argument::Option( &queue::local::restore, { "--restore"}, 
                        "restores messages to queue\n\nthat has been rolled back to error queue\n  casual queue --restore <queue-name>"),
                     common::argument::Option( &local::enqueue, { "-e", "--enqueue"}, 
                        "enqueue to a queue from stdin\n\n  cat somefile.bin | casual queue --enqueue <queue-name>\n  note: operation is atomic"),
                     common::argument::Option( &local::dequeue, { "-d", "--dequeue"}, 
                        "dequeue from a queue to stdout\n\n  casual queue --dequeue <queue-name> > somefile.bin\n  note: operation is atomic"),
                     common::argument::Option( &queue::local::state, complete_state, {"--state"}, "queue state"),
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
   } // queue
} // casual


