//!
//! broker_admin.cpp
//!
//! Created on: Jul 9, 2013
//!     Author: Lazan
//!


#include "sf/xatmi_call.h"
#include "sf/namevaluepair.h"
#include "sf/archive/log.h"

#include "broker/admin/brokervo.h"

#include "common/file.h"
#include "common/arguments.h"
#include "common/chronology.h"
#include "common/terminal.h"
#include "common/server/service.h"


#include "broker/broker.h"

//
// std
//
#include <iostream>
#include <iomanip>
#include <limits>




namespace casual
{

   using namespace common;

   namespace broker
   {

      namespace global
      {
         bool porcelain = false;

         bool no_colors = false;
         bool no_header = false;

      } // global



      namespace call
      {

         admin::StateVO state()
         {
            sf::xatmi::service::binary::Sync service( ".casual.broker.state");
            auto reply = service();

            admin::StateVO serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }


         admin::ShutdownVO shutdown()
         {
            sf::xatmi::service::binary::Sync service( ".casual.broker.shutdown");

            bool broker = true;
            service << CASUAL_MAKE_NVP( broker);

            auto reply = service();

            admin::ShutdownVO serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }


         admin::StateVO boot()
         {
            common::process::spawn( common::environment::variable::get( "CASUAL_HOME") + "/bin/casual-broker", {});

            common::process::sleep( std::chrono::milliseconds{ 20});

            return call::state();
         }


         namespace instance
         {
            void update( std::string alias, std::size_t count)
            {
               admin::update::InstancesVO instance;

               instance.alias = std::move( alias);
               instance.instances = count;

               sf::xatmi::service::binary::Sync service( ".casual.broker.update.instances");

               service << CASUAL_MAKE_NVP( std::vector< admin::update::InstancesVO>{ instance});

               service();
            }
         } // instance

      } // call



      namespace format
      {


         struct base_instances
         {
            base_instances( const std::vector< admin::InstanceVO>& instances) : m_instances( instances) {}

         protected:

            std::vector< admin::InstanceVO> instances( const std::vector< platform::pid_type>& pids) const
            {
               std::vector< admin::InstanceVO> result;

               for( auto& pid : pids)
               {
                  auto found = range::find_if( m_instances, [=]( const admin::InstanceVO& v){
                     return v.process.pid == pid;
                  });
                  if( found)
                  {
                     result.push_back( *found);
                  }
               }

               return result;
            }

            const std::vector< admin::InstanceVO>& m_instances;
         };

         struct format_state :  base_instances
         {
            using base_instances::base_instances;

            template< typename T>
            std::size_t width( const T& value) const
            {
               return value.instances.size();
            }



            template< typename T>
            void print( std::ostream& out, const T& value, std::size_t width, bool color) const
            {
               if( color)
               {
                  for( auto& instance : instances( value.instances))
                  {
                     switch( instance.state)
                     {
                        case admin::InstanceVO::State::booted: out << terminal::color::magenta.start() << '^'; break;
                        case admin::InstanceVO::State::idle: out << terminal::color::green.start() << '+'; break;
                        case admin::InstanceVO::State::busy: out << terminal::color::yellow.start() << '*'; break;
                        case admin::InstanceVO::State::shutdown: out << terminal::color::red.start() << 'x'; break;
                        default: out << terminal::color::red.start() <<  '-'; break;
                     }
                  }
                  out << terminal::color::green.end();
               }
               else
               {
                  for( auto& instance : instances( value.instances))
                  {
                     switch( instance.state)
                     {
                        case admin::InstanceVO::State::booted: out << '^'; break;
                        case admin::InstanceVO::State::idle: out << '+'; break;
                        case admin::InstanceVO::State::busy: out << '*'; break;
                        case admin::InstanceVO::State::shutdown: out << 'x'; break;
                        default: out <<  '-'; break;
                     }
                  }
               }

               //
               // Pad manually
               //

               if( width != 0 )
               {
                  out << std::string( width - value.instances.size(), ' ');
               }
            }
         };


         struct format_instances
         {
            template< typename T>
            std::size_t operator () ( const T& value) const { return value.instances.size();}
         };

         terminal::format::formatter< admin::ServiceVO> services( const std::vector< admin::InstanceVO>& instances)
         {

            struct format_timeout
            {
               double operator () ( const admin::ServiceVO& value) const
               {
                  using second_t = std::chrono::duration< double>;
                  return std::chrono::duration_cast< second_t>( value.timeout).count();
               }
            };


            struct format_mode
            {
               const char* operator () ( const admin::ServiceVO& value) const
               {
                  static std::map< server::Service::Transaction, const char*> mapping{
                     {server::Service::Transaction::automatic, "auto"},
                     {server::Service::Transaction::join, "join"},
                     {server::Service::Transaction::atomic, "atomic"},
                     {server::Service::Transaction::none, "none"},
                  };
                  return mapping[ server::Service::Transaction( value.mode)];
               }
            };

            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "name", std::mem_fn( &admin::ServiceVO::name), terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "type", std::mem_fn( &admin::ServiceVO::type), terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "mode", format_mode{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "timeout", format_timeout{}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "requested", std::mem_fn( &admin::ServiceVO::lookedup), terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "#", format_instances{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{ instances})
            };
         }

         terminal::format::formatter< admin::ServerVO> servers( const std::vector< admin::InstanceVO>& instances)
         {

            struct format_last : base_instances
            {
               using base_instances::base_instances;

               std::string operator () ( const admin::ServerVO& value) const
               {
                  auto inst = instances( value.instances);

                  platform::time_point last = platform::time_point::min();

                  for( auto& instance : inst)
                  {
                     last = std::max( last, instance.last);
                  }
                  return chronology::local( last);
               }
            };

            struct format_invoked : base_instances
            {
               using base_instances::base_instances;

               std::size_t operator () ( const admin::ServerVO& value) const
               {
                  auto inst = instances( value.instances);

                  decltype( inst.front().invoked) invoked = 0;

                  for( auto& instance : inst)
                  {
                     invoked += instance.invoked;
                  }
                  return invoked + value.invoked;
               }
            };

            struct format_restart
            {
               char operator () ( const admin::ServerVO& value) const
               {
                  return value.restart ? 'Y' : 'N';
               }
            };

            struct format_deaths
            {
               std::size_t width( const admin::ServerVO& value) const
               {
                  return string::digits( value.deaths);
               }

               void print( std::ostream& out, const admin::ServerVO& value, std::size_t width, bool color) const
               {
                  if( value.deaths > 0 && color)
                  {
                     out << std::right << terminal::color::red.start() << std::setfill( ' ') << std::setw( width) << value.deaths << terminal::color::red.end();
                  }
                  else
                  {
                     out << std::right << std::setfill( ' ') << std::setw( width) << value.deaths;
                  }

               }
            };


            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "alias", std::mem_fn( &admin::ServerVO::alias), terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "invoked", format_invoked{ instances}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "last", format_last{ instances}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "r", format_restart{}, terminal::color::no_color, terminal::format::Align::left),
               terminal::format::custom_column( "d#", format_deaths{}),
               terminal::format::column( "#", format_instances{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{ instances}),
               terminal::format::column( "path", std::mem_fn( &admin::ServerVO::path), terminal::color::no_color, terminal::format::Align::left)
            };
         }


         terminal::format::formatter< admin::InstanceVO> instances( const std::vector< admin::ServerVO>& servers)
         {
            struct base_server
            {
               base_server( const std::vector< admin::ServerVO>& servers) : m_servers( servers) {}

               const std::string& operator () ( const admin::InstanceVO& value) const
               {
                  return server( value.server).alias;
               }

            protected:

               const admin::ServerVO& server( std::size_t server_id) const
               {
                  auto found = range::find_if( m_servers, [=] (const admin::ServerVO& v){
                     return v.id == server_id;
                  });

                  if( ! found)
                     throw exception::invalid::Configuration{ "Inconsistency"};

                  return *found;
               }

               const std::vector< admin::ServerVO>& m_servers;
            };


            struct format_server_name : base_server
            {
               using base_server::base_server;

               const std::string& operator () ( const admin::InstanceVO& value) const
               {
                  return server( value.server).alias;
               }
            };

            struct format_pid
            {
               platform::pid_type operator () ( const admin::InstanceVO& v) const { return v.process.pid;}
            };

            struct format_queue
            {
               platform::queue_id_type operator () ( const admin::InstanceVO& v) const { return v.process.queue;}
            };


            struct format_state
            {
               std::size_t width( const admin::InstanceVO& value) const
               {
                  switch( value.state)
                  {
                     case admin::InstanceVO::State::booted: return 6;
                     case admin::InstanceVO::State::shutdown: return 8;
                     default: return 4;
                  }
               }

               void print( std::ostream& out, const admin::InstanceVO& value, std::size_t width, bool color) const
               {
                  out << std::setfill( ' ');

                  if( color)
                  {
                     switch( value.state)
                     {
                        case admin::InstanceVO::State::booted: out << std::right << std::setw( width) << terminal::color::red << "booted"; break;
                        case admin::InstanceVO::State::idle: out << std::right << std::setw( width) << terminal::color::green << "idle"; break;
                        case admin::InstanceVO::State::busy: out << std::right << std::setw( width) << terminal::color::yellow << "busy"; break;
                        case admin::InstanceVO::State::shutdown: out << std::right << std::setw( width) << terminal::color::red << "shutdown"; break;
                     }
                  }
                  else
                  {
                     switch( value.state)
                     {
                        case admin::InstanceVO::State::booted: out << std::right << std::setw( width) << "booted"; break;
                        case admin::InstanceVO::State::idle: out << std::right << std::setw( width) << "idle"; break;
                        case admin::InstanceVO::State::busy: out << std::right << std::setw( width) << "busy"; break;
                        case admin::InstanceVO::State::shutdown: out  << std::right << std::setw( width) << "shutdown"; break;
                     }
                  }
               }
            };

            struct format_last
            {
               std::string operator () ( const admin::InstanceVO& v) const { return chronology::local( v.last);}
            };

            struct format_path : base_server
            {
               using base_server::base_server;

               const std::string& operator () ( const admin::InstanceVO& v) const
               {
                  return server( v.server).path;
               }
            };

            return {
               { global::porcelain, ! global::no_colors, ! global::no_header},
               terminal::format::column( "server", format_server_name{ servers}, terminal::color::yellow, terminal::format::Align::left),
               terminal::format::column( "pid", format_pid{}, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "queue", format_queue{}, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::custom_column( "state", format_state{}),
               terminal::format::column( "invoked", std::mem_fn( &admin::InstanceVO::invoked), terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "last", format_last{}, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "path", format_path{ servers}, terminal::color::no_color, terminal::format::Align::left),
            };
         }

      } // format


      namespace print
      {

         void servers( std::ostream& out, admin::StateVO& state)
         {
            out << std::boolalpha;
            range::sort( state.servers, []( const admin::ServerVO& l, const admin::ServerVO& r){ return l.alias < r.alias;});

            auto formatter = format::servers( state.instances);

            formatter.print( std::cout, std::begin( state.servers), std::end( state.servers));
         }

         void services( std::ostream& out, admin::StateVO& state)
         {
            range::sort( state.services, []( const admin::ServiceVO& l, const admin::ServiceVO& r){ return l.name < r.name;});

            auto formatter = format::services( state.instances);

            formatter.print( std::cout, std::begin( state.services), std::end( state.services));
         }

         template< typename IR>
         void instances( std::ostream& out, admin::StateVO& state, IR instances_range)
         {
            range::sort( instances_range, []( const admin::InstanceVO& l, const admin::InstanceVO& r){ return l.server < r.server;});

            auto formatter = format::instances( state.servers);

            formatter.print( std::cout, std::begin( instances_range), std::end( instances_range));
         }

         void instances( std::ostream& out, admin::StateVO& state)
         {
            instances( out, state, state.instances);
         }

      } // print

      namespace action
      {


         void listServers()
         {

            auto state = call::state();

            print::servers( std::cout, state);
         }

         void listServices()
         {
            auto state = call::state();

            print::services( std::cout, state);
         }

         void listInstances()
         {
            auto state = call::state();

            print::instances( std::cout, state);
         }


         void updateInstances( const std::vector< std::string>& values)
         {
            if( values.size() == 2)
            {
               admin::update::InstancesVO instance;

               instance.alias = values[ 0];
               instance.instances = std::stoul( values[ 1]);

               sf::xatmi::service::binary::Sync service( ".casual.broker.update.instances");

               service << CASUAL_MAKE_NVP( std::vector< admin::update::InstancesVO>{ instance});

               service();

            }
         }

         namespace local
         {
            namespace
            {
               struct Batch
               {
                  std::vector< admin::ExecutableVO> executables;
                  std::vector< admin::ServerVO> servers;

                  CASUAL_CONST_CORRECT_SERIALIZE({
                     archive & CASUAL_MAKE_NVP( executables);
                     archive & CASUAL_MAKE_NVP( servers);
                  })
               };

               struct Membership
               {
                  Membership( std::size_t group) : m_group( group) {}

                  bool operator () ( const admin::ExecutableVO& value) const
                  {
                     return static_cast< bool>( range::find( value.memberships, m_group));
                  }
               private:
                  std::size_t m_group;
               };

               std::vector< Batch> shutdown_order( admin::StateVO& state)
               {
                  std::vector< Batch> result;

                  auto group_order = range::reverse( range::stable_sort( state.groups));
                  //auto group_order = range::stable_sort( state.groups);

                  log::internal::debug << CASUAL_MAKE_NVP( group_order);


                  auto executables = range::make( state.executables);
                  auto servers = range::make( state.servers);

                  for( auto& group : group_order)
                  {
                     Batch batch;
                     {
                        auto partition = range::stable_partition( servers, Membership{ group.id});
                        range::copy( std::get< 0>( partition), std::back_inserter( batch.servers));
                        servers = std::get< 1>( partition);
                     }
                     {
                        auto partition = range::stable_partition( executables, Membership{ group.id});
                        range::copy( std::get< 0>( partition), std::back_inserter( batch.executables));
                        executables = std::get< 1>( partition);
                     }
                     result.push_back( std::move( batch));
                  }

                  log::internal::debug << sf::makeNameValuePair( "shutdown-order",  result);

                  return result;
               }

            } // <unnamed>
         } // local

         void shutdown()
         {
            auto set_state = []( admin::InstanceVO& value)
                 {
                    value.state = admin::InstanceVO::State::shutdown;
                 };

            auto origin = call::state();

            auto origin_instances = origin.instances;
            auto origin_active = range::make( origin_instances);

            auto active = range::make( origin.instances);
            range::for_each( active, set_state);

            auto formatter = format::instances( origin.servers);
            formatter.calculate_width( active);

            formatter.print_headers( std::cout);


            auto print_shutdown = [&]( admin::StateVO& state)
                  {
                     {
                        auto split = range::intersection( active, state.instances);
                        active = std::get< 0>( split);
                        formatter.print_rows( std::cout, std::get< 1>( split));
                     }
                     {
                        origin_active = std::get< 0>( range::intersection( origin_active, state.instances));
                     }
                  };

            try
            {
               auto shutdown_set = local::shutdown_order( origin);


               for( auto& batch : shutdown_set)
               {
                  auto do_shutdown = [&]( const admin::ExecutableVO& value)
                        {
                           if( ! value.instances.empty())
                           {
                              call::instance::update( value.alias, 0);

                              auto state = call::state();
                              print_shutdown( state);
                           }
                        };

                  range::for_each( batch.executables, do_shutdown);
                  range::for_each( batch.servers, do_shutdown);
               }

               auto shutdown_result = call::shutdown();
               print_shutdown( shutdown_result.state);
            }
            catch( ...)
            {

            }

            auto count = 10;

            while( count-- > 0 && active.size() > 1)
            {
               process::sleep( std::chrono::milliseconds{ 100});

               auto state = call::state();
               print_shutdown( state);
            }

            if( active)
            {
               formatter.print_rows( std::cerr, origin_active);
            }

         }

         namespace local
         {
            namespace
            {
               template< typename S>
               auto format_instances( S& servers) -> decltype( format::instances( servers))
               {
                  admin::InstanceVO max;
                  max.process.pid = std::numeric_limits< decltype( max.process.pid)>::max();
                  max.process.queue = std::numeric_limits< decltype( max.process.queue)>::max();
                  max.state = admin::InstanceVO::State::shutdown;

                  std::vector< admin::InstanceVO> mockup_instances;

                  for( auto& s : servers)
                  {
                     max.server = s.id;
                     mockup_instances.push_back( max);
                  }

                  auto result = format::instances( servers);
                  result.calculate_width( mockup_instances);

                  return result;
               }
            } // <unnamed>
         } // local

         void boot()
         {

            auto state = call::boot();

            auto calculate_instances = []( const admin::StateVO& state){
               return range::accumulate( state.servers, 0, []( std::size_t s, const admin::ServerVO& server){
                  return server.configured_instances + s;
               });
            };


            auto state_running = []( const admin::InstanceVO& i)
                  {
                     return i.state != admin::InstanceVO::State::booted;
                  };


            auto booted = std::get< 0>( range::stable_partition( state.instances, state_running));

            auto formatter = local::format_instances( state.servers);

            formatter.print_headers( std::cout);
            formatter.print_rows( std::cout, booted);


            auto count = 1000;

            while( booted.size() < calculate_instances( state) && count-- > 0)
            {
               process::sleep( std::chrono::milliseconds{ 10});

               auto check_state = call::state();

               auto total_booted = std::get< 0>( range::stable_partition( check_state.instances, state_running));

               auto booted_since_last = std::get< 1>( range::intersection( total_booted, booted));

               formatter.print_rows( std::cout, booted_since_last);

               state = std::move( check_state);
               booted = total_booted;
            }

            if( booted.size() < calculate_instances( state))
            {
               auto check_state = call::state();
               auto not_booted = std::get< 1>( range::stable_partition( check_state.instances, state_running));

               formatter.print_rows( std::cout, not_booted);
            }
         }

      } // action

   } // broker
} // casual



int main( int argc, char** argv)
{

   casual::common::Arguments parser{ {
         casual::common::argument::directive( {"--porcelain"}, "easy to parse format", casual::broker::global::porcelain),
         casual::common::argument::directive( {"--no-color"}, "no color will be used", casual::broker::global::no_colors),
         casual::common::argument::directive( {"--no-header"}, "no descriptive header for each column will be used", casual::broker::global::no_header),
         casual::common::argument::directive( {"-lsvr", "--list-servers"}, "list all servers", &casual::broker::action::listServers),
         casual::common::argument::directive( {"-lsvc", "--list-services"}, "list all services", &casual::broker::action::listServices),
         casual::common::argument::directive( {"-li", "--list-instances"}, "list all instances", &casual::broker::action::listInstances),
         casual::common::argument::directive( {"-ui", "--update-instances"}, "<alias> <#> update server instances", &casual::broker::action::updateInstances),
         casual::common::argument::directive( {"-s", "--shutdown"}, "shutdown the domain", &casual::broker::action::shutdown),
         casual::common::argument::directive( {"-b", "--boot"}, "boot domain", &casual::broker::action::boot)}
   };




   try
   {
      parser.parse( argc, argv);

   }
   catch( const std::exception& exception)
   {
      std::cerr << "error: " << exception.what() << std::endl;
   }


   return 0;
}


