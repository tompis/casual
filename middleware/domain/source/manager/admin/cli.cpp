//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!
#include "domain/manager/admin/cli.h"


#include "common/event/listen.h"
#include "domain/manager/admin/vo.h"
#include "domain/manager/admin/server.h"

#include "common/argument.h"
#include "common/terminal.h"
#include "common/environment.h"
#include "common/exception/handle.h"
#include "common/execute.h"

#include "common/communication/ipc.h"

#include "sf/service/protocol/call.h"
#include "sf/archive/maker.h"
#include "sf/log.h"

namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               namespace event
               {
                  struct Done{};

                  struct Handler
                  {
                     using mapping_type = std::map< strong::process::id, std::string>;

                     Handler() = default;
                     Handler( mapping_type mapping) : m_alias_mapping{ std::move( mapping)} {}

                     void operator () ()
                     {
                        //
                        // Make sure we unsubscribe for events
                        //
                        auto unsubscribe = execute::scope( [](){
                           message::event::subscription::End message;
                           message.process = process::handle();

                           communication::ipc::non::blocking::send(
                                 communication::ipc::domain::manager::optional::device(),
                                 message);
                        });

                        common::communication::ipc::inbound::Device::handler_type event_handler{
                           [&]( message::event::process::Spawn& m){
                              group( std::cout) << "spawned: " << terminal::color::yellow << m.alias << " "
                                    << terminal::color::no_color << range::make( m.pids) << '\n';
                              for( auto pid : m.pids)
                              {
                                 m_alias_mapping[ pid] = m.alias;
                              }
                           },
                           [&]( message::event::process::Exit& m){
                              using reason_t = process::lifetime::Exit::Reason;
                              switch( m.state.reason)
                              {
                                 case reason_t::core:
                                    group( std::cout) << terminal::color::red << "core: "
                                          << terminal::color::white << m.state.pid << " " << m_alias_mapping[ m.state.pid] << '\n';
                                    break;
                                 default:
                                    group( std::cout) << terminal::color::green << "exit: "
                                       <<  terminal::color::white << m.state.pid
                                       << " " << terminal::color::yellow << m_alias_mapping[ m.state.pid] << '\n';
                                    break;
                              }

                           },
                           [&]( message::event::domain::server::Connect& m){

                              group( std::cout) << terminal::color::green << "connected: "
                                    <<  terminal::color::white << m.process.pid
                                    << " " << terminal::color::yellow << m_alias_mapping[ m.process.pid] << '\n';


                           },
                           []( message::event::domain::boot::Begin& m){
                                 std::cout << "boot domain: " << terminal::color::cyan << m.domain.name << terminal::color::no_color
                                       << " - id: " << terminal::color::yellow << m.domain.id << '\n';
                           },
                           []( message::event::domain::boot::End& m){
                              throw event::Done{};
                           },
                           []( message::event::domain::shutdown::Begin& m){
                                 std::cout << "shutdown domain: " << terminal::color::cyan << m.domain.name << terminal::color::no_color
                                       << " - id: " << terminal::color::yellow << m.domain.id << '\n';
                           },
                           []( message::event::domain::shutdown::End& m){
                              throw event::Done{};
                           },
                           []( message::event::domain::Error& m)
                           {

                              auto print_error = []( const message::event::domain::Error& m){
                                 std::cerr << terminal::color::yellow << m.executable << " "
                                       << terminal::color::white << m.pid << ": "
                                       << terminal::color::white << m.message
                                       << '\n';

                                 for( auto& detail : m.details)
                                 {
                                    std::cerr << " |- " << detail << '\n';
                                 }
                              };

                              switch( m.severity)
                              {
                                 case message::event::domain::Error::Severity::fatal:
                                 {
                                    std::cerr << terminal::color::red << "fatal: ";
                                    print_error( m);
                                    throw Done{};
                                 }
                                 case message::event::domain::Error::Severity::error:
                                 {
                                    std::cerr << terminal::color::red << "error: ";
                                    print_error( m);
                                    break;
                                 }
                                 default:
                                 {
                                    std::cerr << terminal::color::magenta << "warning ";
                                    print_error( m);
                                 }
                              }


                           },
                           [&]( message::event::domain::Group& m){
                              using context_type = message::event::domain::Group::Context;

                              switch( m.context)
                              {
                                 case context_type::boot_start:
                                 case context_type::shutdown_start:
                                 {
                                    m_group = m.name;
                                    break;
                                 }
                                 default:
                                 {
                                    m_group.clear();
                                    break;
                                 }
                              }
                           }
                        };

                        try
                        {
                           common::message::dispatch::blocking::pump( event_handler, common::communication::ipc::inbound::device());
                        }
                        catch( const event::Done&)
                        {
                           // no-op
                        }
                        catch( const exception::signal::child::Terminate&)
                        {
                           std::cerr << terminal::color::red << "fatal";
                           std::cerr << " failed to boot domain\n";

                           //
                           // Check if we got some error events
                           //
                           common::message::dispatch::pump(
                                 event_handler,
                                 common::communication::ipc::inbound::device(),
                                 common::communication::ipc::inbound::Device::non_blocking_policy{});

                        }
                        catch( ...)
                        {
                           exception::handle();
                        }
                     }

                  private:

                     std::ostream& group( std::ostream& out)
                     {
                        if( ! m_group.empty())
                        {
                           out << terminal::color::blue << m_group << " ";
                        }
                        return out;
                     }

                     std::map< strong::process::id, std::string> m_alias_mapping;
                     std::string m_group;
                  };

               } // event

               namespace call
               {

                  admin::vo::State state()
                  {
                     sf::service::protocol::binary::Call call;
                     auto reply = call( admin::service::name::state());

                     admin::vo::State serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }


                  std::vector< admin::vo::scale::Instances> scale_instances( const std::vector< admin::vo::scale::Instances>& instances)
                  {
                     sf::service::protocol::binary::Call call;
                     call << CASUAL_MAKE_NVP( instances);

                     auto reply = call( admin::service::name::scale::instances());

                     std::vector< admin::vo::scale::Instances> serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }

                  void boot( const std::vector< std::string>& files)
                  {

                     event::Handler events;

                     auto get_arguments = []( auto& value){
                        std::vector< std::string> arguments;

                        auto files = common::range::make( value);

                        if( ! files.empty())
                        {
                           if( ! algorithm::all_of( files, &common::file::exists))
                           {
                              throw exception::system::invalid::File{ string::compose( "at least one file does not exist - files: ", files)};
                           }
                           arguments.emplace_back( "--configuration-files");
                           algorithm::copy( files, std::back_inserter( arguments));
                        }

                        arguments.emplace_back( "--event-queue");
                        arguments.emplace_back( common::string::compose( common::communication::ipc::inbound::id()));

                        return arguments;
                     };

                     common::process::spawn(
                           common::environment::variable::get( common::environment::variable::name::home()) + "/bin/casual-domain-manager",
                           get_arguments( files));

                     events();
                  }

                  auto get_alias_mapping()
                  {
                     auto state = call::state();

                     std::map< strong::process::id, std::string> mapping;

                     for( auto& s : state.servers)
                     {
                        for( auto& i : s.instances)
                        {
                           mapping[ i.handle.pid] = s.alias;
                        }
                     }

                     for( auto& e : state.executables)
                     {
                        for( auto& i : e.instances)
                        {
                           mapping[ i.handle] = e.alias;
                        }
                     }
                     return mapping;
                  }

                  void shutdown()
                  {

                     //
                     // subscribe for events
                     //
                     {
                        message::event::subscription::Begin message;
                        message.process = process::handle();

                        communication::ipc::non::blocking::send(
                              communication::ipc::domain::manager::optional::device(),
                              message);
                     }

                     event::Handler events{ get_alias_mapping()};

                     sf::service::protocol::binary::Call{}( admin::service::name::shutdown());

                     events();
                  }
               } // call

               namespace format
               {

                  template< typename P>
                  auto process()
                  {

                     auto format_configured_instances = []( const P& e){
                        return e.instances.size();
                     };

                     auto format_running_instances = []( const P& e){
                        return common::algorithm::count_if( e.instances, []( auto& i){
                              return i.state == admin::vo::instance::State::running;
                        });
                     };

                     auto format_restart = []( const P& e){
                        if( e.restart) { return "true";}
                        return "false";
                     };

                     auto format_restarts = []( const P& e){
                        return e.restarts;
                     };


                     return terminal::format::formatter< P>::construct(
                        terminal::format::column( "alias", std::mem_fn( &P::alias), terminal::color::yellow, terminal::format::Align::left),
                        terminal::format::column( "CI", format_configured_instances, terminal::color::no_color, terminal::format::Align::right),
                        terminal::format::column( "I", format_running_instances, terminal::color::white, terminal::format::Align::right),
                        terminal::format::column( "restart", format_restart, terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "#r", format_restarts, terminal::color::red, terminal::format::Align::right),
                        terminal::format::column( "path", std::mem_fn( &P::path), terminal::color::blue, terminal::format::Align::left)
                     );
                  }
               } // format

               namespace print
               {

                  template< typename VO>
                  void processes( std::ostream& out, std::vector< VO>& processes)
                  {
                     out << std::boolalpha;

                     auto formatter = format::process< VO>();

                     formatter.print( std::cout, algorithm::sort( processes));
                  }

               } // print

               namespace action
               {
                  auto fetch_aliases()
                  {
                     auto state = call::state();

                     auto aliases = common::algorithm::transform( state.servers, []( auto& s){
                        return std::move( s.alias);
                     });
                     
                     common::algorithm::transform( state.executables, aliases, []( auto& e){
                        return std::move( e.alias);
                     });

                     return aliases;
                  };

                  

                  void list_instances()
                  {
                     auto state = call::state();

                     //print::executables( std::cout, state);
                  }

                  void list_executable()
                  {
                     auto state = call::state();

                     print::processes( std::cout, state.executables);
                  }

                  void list_servers()
                  {
                     auto state = call::state();

                     print::processes( std::cout, state.servers);
                  }

                  /*
                  void list_processes()
                  {
                     auto state = call::state();

                     print::processes( std::cout, state.servers);
                  }
                  */


                  void scale_instances( const std::tuple< std::string, int>& mandatory, const std::vector< std::tuple< std::string, int>>& values)
                  {
                     std::vector< admin::vo::scale::Instances> result;
                      
                     auto transform = []( auto& value){
                        admin::vo::scale::Instances result;
                        result.alias = std::get< 0>( value);
                        result.instances = std::get< 1>( value);
                        return result;
                     };

                     result.push_back( transform( mandatory));
                     common::algorithm::transform( values, result, transform);

                     call::scale_instances( result);
                  }
                

                  auto scale_instances_completion = []( auto values, bool help){
                     if( help)
                     {
                        return std::vector< std::string>{ "<alias> <#>"};
                     }
                     
                     if( values.size() % 2 == 0)
                     {
                        return fetch_aliases();
                     }
                     else
                        return std::vector< std::string>{ "<value>"};
                  };

                  void boot( const std::vector< std::string>& files)
                  {
                     call::boot( files);
                  }

                  void shutdown()
                  {
                     call::shutdown();
                  }

                  namespace persist
                  {
                     void configuration()
                     {
                        sf::service::protocol::binary::Call{}( admin::service::name::configuration::persist());
                     }
                  } // persist


                  void state( const common::optional< std::string>& format)
                  {
                     auto state = call::state();
                     auto archive = sf::archive::writer::from::name( format.value_or( ""));

                     archive << CASUAL_MAKE_NVP( state);
                  }
               } // action

            } // <unnamed>
         } // local

         namespace admin 
         {
            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  auto complete_state = []( auto values, bool){
                     return std::vector< std::string>{ "json", "yaml", "xml", "ini"};
                  };

                  return common::argument::Group{ [](){}, { "domain"}, "local casual domain related administration",
                     common::argument::Option( &local::action::list_servers, { "-ls", "--list-servers"}, "list all servers"),
                     common::argument::Option( &local::action::list_executable, { "-le", "--list-executables"}, "list all executables"),
                     common::argument::Option( &local::action::list_instances, { "-li", "--list-instances"}, "list all instances"),
                     common::argument::Option( &local::action::scale_instances, local::action::scale_instances_completion, { "-si", "--scale-instances"}, "<alias> <#> scale executable instances"),
                     common::argument::Option( &local::action::shutdown, { "-s", "--shutdown"}, "shutdown the domain"),
                     common::argument::Option( &local::action::boot, { "-b", "--boot"}, "boot domain"),
                     common::argument::Option( &local::action::persist::configuration, { "-p", "--persist-state"}, "persist current state"),
                     common::argument::Option( &local::action::state, complete_state, { "--state"}, "domain state")
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
   } // domain
} // casual









