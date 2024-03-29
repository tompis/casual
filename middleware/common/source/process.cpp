//!
//! process.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#include "common/process.h"
#include "common/exception.h"
#include "common/error.h"
#include "common/file.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/signal.h"
#include "common/string.h"
#include "common/environment.h"
#include "common/uuid.h"

#include "common/message/server.h"
#include "common/communication/ipc.h"
#include "common/flag.h"

//
// std
//
#include <algorithm>
#include <functional>
#include <fstream>

// TODO: temp
#include <iostream>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
//#include <fcntl.h>

#include <spawn.h>


#ifdef __APPLE__
   #include <crt_externs.h>
#else
   #include <unistd.h>
#endif



namespace casual
{
   namespace common
   {
      namespace process
      {
         namespace local
         {
            namespace
            {
               std::string getProcessPath()
               {
                  if( environment::variable::exists( "_"))
                  {
                     return environment::variable::get( "_");
                  }

                  return std::string{};
               }

               std::string& path()
               {
                  static std::string path = getProcessPath();
                  return path;
               }

               char* const * environment()
               {
                  #ifdef __APPLE__
                      return *::_NSGetEnviron();
                  #else
                     return ::environ;
                  #endif
               }


            } // <unnamed>
         } // local

         const std::string& path()
         {
            return local::path();
         }

         void path( const std::string& path)
         {
            local::path() = path;
         }


         platform::pid_type id()
         {
            static const platform::pid_type pid = getpid();
            return pid;
         }

         Handle handle()
         {
            Handle result{ id(), communication::ipc::inbound::id()};

            return result;
         }

         namespace instance
         {
            namespace identity
            {
               const Uuid& broker()
               {
                  const static Uuid singleton{ "f58e0b181b1b48eb8bba01b3136ed82a"};
                  return singleton;
               }

               namespace traffic
               {
                  const Uuid& manager()
                  {
                     const static Uuid singleton{ "1aa1ce0e3e254a91b32e9d2ab22a8d31"};
                     return singleton;
                  }
               } // traffic
            } // identity


            namespace transaction
            {
               namespace manager
               {
                  const Uuid& identity()
                  {
                     const static Uuid singleton{ "5ec18cd92b2e4c60a927e9b1b68537e7"};
                     return singleton;
                  }

                  namespace local
                  {
                     namespace
                     {
                        Handle& handle()
                        {
                           static Handle singleton = fetch::handle( manager::identity(), fetch::Directive::wait);
                           return singleton;
                        }
                     } // <unnamed>
                  } // local

                  const Handle& handle()
                  {
                     return local::handle();
                  }

                  const Handle& refetch()
                  {
                     local::handle() = fetch::handle( manager::identity(), fetch::Directive::wait);
                     return handle();
                  }



               } // manager

            } // transaction

            namespace fetch
            {
               Handle handle( const Uuid& identity, Directive directive)
               {
                  trace::Scope trace{ "instance::handle::fetch", log::internal::trace};

                  common::message::lookup::process::Request request;
                  request.directive = static_cast< message::lookup::process::Request::Directive>( directive);
                  request.identification = identity;
                  request.process = common::process::handle();

                  auto reply = communication::ipc::call( communication::ipc::broker::id(), request);

                  if( ! reply.domain.empty())
                  {
                     environment::domain::name( reply.domain);
                  }

                  return reply.process;
               }

            } // fetch
         } // instance


         bool operator == ( const Handle& lhs, const Handle& rhs)
         {
            return lhs.pid == rhs.pid && lhs.queue == rhs.queue;
         }

         std::ostream& operator << ( std::ostream& out, const Handle& value)
         {
            return out << "{ pid: " << value.pid << ", queue: " << value.queue << '}';
         }



         const Uuid& uuid()
         {
            static const Uuid singleton = uuid::make();
            return singleton;
         }

         void sleep( std::chrono::microseconds time)
         {
            std::this_thread::sleep_for( time);
         }

         namespace local
         {
            namespace
            {
               namespace current
               {

                  namespace internal
                  {
                     std::vector< const char*> environment()
                     {
                        std::vector< const char*> result;

                        auto current = local::environment();

                        while( (*current) != nullptr)
                        {
                           result.push_back( *current);
                           ++current;
                        }

                        return result;
                     }
                  } // internal

                  // we need to force lvalue parameter
                  std::vector< const char*> environment( const std::vector< std::string>& environment)
                  {
                     auto result = internal::environment();

                     std::transform(
                        std::begin( environment),
                        std::end( environment),
                        std::back_inserter( result),
                        std::mem_fn( &std::string::data));

                     result.push_back( nullptr);
                     return result;
                  }

               } // current

            } // <unnamed>
         } // local

         platform::pid_type spawn(
            const std::string& path,
            std::vector< std::string> arguments,
            std::vector< std::string> environment)
         {
            trace::Scope trace{ "process::spawn", log::internal::trace};

            //
            // check if path exist and process has permission to execute it.
            // could still go wrong, since we don't know if the path will actually execute,
            // but we'll probably get rid of most of the errors (due to bad configuration and such)
            //
            if( ! file::permission::execution( path))
            {
               throw exception::invalid::Argument( "spawn failed", CASUAL_NIP( path),
                  exception::make_nip( "arguments", range::make( arguments)),
                  exception::make_nip( "environment", range::make( environment)));
            }

            //
            // prepare arguments
            //
            std::vector< const char*> c_arguments;

            //
            // think we must add application-name as first argument...
            //
            {
               c_arguments.push_back( path.data());

               //
               // We need to expand environment
               //
               for( auto& argument : arguments)
               {
                  argument = environment::string( argument);
               }


               std::transform(
                     std::begin( arguments),
                     std::end( arguments),
                     std::back_inserter( c_arguments),
                     std::mem_fn( &std::string::data));

               c_arguments.push_back( nullptr);
            }

            //
            // We need to expand environment
            //
            for( auto& variable : environment)
            {
               variable = environment::string( variable);
            }



            auto c_environment = local::current::environment( environment);

            posix_spawnattr_t attributes;

            posix_spawnattr_init( &attributes);

            platform::pid_type pid;

            log::internal::debug << "process::spawn " << path << " " << range::make( arguments) << " - environment: " << range::make( environment) << std::endl;

            auto status =  posix_spawnp(
                  &pid,
                  path.c_str(),
                  nullptr,
                  &attributes,
                  const_cast< char* const*>( c_arguments.data()),
                  const_cast< char* const*>( c_environment.data())
                  );
            switch( status)
            {
               case 0:
                  break;
               default:
                  throw exception::invalid::Argument( "spawn failed", CASUAL_NIP( path),
                        exception::make_nip( "arguments", range::make( arguments)),
                        exception::make_nip( "environment", range::make( environment)),
                        CASUAL_NIP( error::string( status)));
            }

            //
            // Try to figure out if the process started correctly..
            //
            /* We can't really do this, since we mess up the semantics for the caller

            {
               auto deaths = lifetime::wait( { pid}, std::chrono::microseconds{ 1});

               if( ! deaths.empty() && deaths.front().reason != lifetime::Exit::Reason::exited)
               {
                  auto& reason = deaths.front();

                  throw exception::invalid::Argument( "spawn failed", CASUAL_NIP( path),
                        exception::make_nip( "arguments", range::make( arguments)),
                        exception::make_nip( "environment", range::make( environment)),
                        CASUAL_NIP( reason));
               }
            }
            */
            // TODO: try something else to detect if the process started correct or not.


            return pid;
         }


         platform::pid_type spawn( const std::string& path, std::vector< std::string> arguments)
         {
            return spawn( path, std::move( arguments), {});
         }



         int execute( const std::string& path, std::vector< std::string> arguments)
         {
            return wait( spawn( path, std::move( arguments)));
         }


         namespace local
         {
            namespace
            {
               lifetime::Exit wait( platform::pid_type pid, int flags = WNOHANG)
               {
                  log::internal::debug << "wait - pid: " << pid << " flags: " << flags << std::endl;

                  if( ! common::flag< WNOHANG>( flags))
                  {
                     log::internal::debug << "current timeout (us): " << signal::timer::get().count() << std::endl;
                  }

                  signal::handle( signal::Filter::exclude_child_terminate);

                  lifetime::Exit exit;

                  auto loop = [&]()
                  {
                     auto result = waitpid( pid, &exit.status, flags);

                     if( result == -1)
                     {
                        switch( errno)
                        {
                           case ECHILD:
                           {
                              // no child
                              break;
                           }
                           case EINTR:
                           {
                              signal::handle( signal::Filter::exclude_child_terminate);

                              //
                              // We do another turn in the loop
                              //
                              return true;
                           }
                           default:
                           {
                              log::error << "failed to check state of pid: " << exit.pid << " - " << error::string() << std::endl;
                              throw exception::NotReallySureWhatToNameThisException( error::string());
                           }
                        }
                     }
                     else if( result != 0)
                     {
                        exit.pid = result;


                        if( WIFEXITED( exit.status))
                        {
                           exit.reason = lifetime::Exit::Reason::exited;
                           exit.status = WEXITSTATUS( exit.status);
                        }
                        else if( WIFSIGNALED( exit.status))
                        {
                           if( WCOREDUMP( exit.status))
                           {
                              exit.reason = lifetime::Exit::Reason::core;
                              exit.status = 0;
                           }
                           else
                           {
                              exit.reason = lifetime::Exit::Reason::signaled;
                              exit.status = WTERMSIG( exit.status);
                           }
                        }
                        else if( WIFSTOPPED( exit.status))
                        {
                           exit.reason = lifetime::Exit::Reason::stopped;
                           exit.status = WSTOPSIG( exit.status);
                        }
                        else if( WIFCONTINUED( exit.status))
                        {
                           exit.reason = lifetime::Exit::Reason::continued;
                           exit.status = 0;
                        }
                     }
                     return false;
                  };

                  while( loop());

                  return exit;
               }

               void wait( const std::vector< platform::pid_type> pids, std::vector< lifetime::Exit>& result)
               {
                  while( result.size() < pids.size())
                  {
                     auto exit = local::wait( -1, 0);

                     if( range::find( pids, exit.pid))
                     {
                        result.push_back( std::move( exit));
                     }
                     else if( exit.pid == 0)
                     {
                        //
                        // No children exists
                        //
                        return;
                     }
                  }
               }

            } // <unnamed>

         } // local

         int wait( platform::pid_type pid)
         {
            return local::wait( pid, 0).status;
         }




         std::vector< platform::pid_type> terminate( const std::vector< platform::pid_type>& pids)
         {
            std::vector< platform::pid_type> result;
            for( auto pid : pids)
            {
               if( terminate( pid))
               {
                  result.push_back( pid);
               }
            }
            return result;
         }



         bool terminate( platform::pid_type pid)
         {
            return signal::send( pid, signal::Type::terminate);
         }

         file::scoped::Path singleton( std::string path)
         {
            std::ifstream file( path);

            if( file)
            {
               try
               {

                  common::signal::timer::Scoped alarm{ std::chrono::seconds( 5)};

                  decltype( communication::ipc::inbound::id()) id;
                  std::string uuid;
                  file >> id;
                  file >> uuid;

                  {

                     common::message::server::ping::Request request;
                     request.process = handle();

                     auto reply = communication::ipc::call( id, request, communication::ipc::policy::Blocking{});

                     if( reply.uuid == Uuid( uuid))
                     {

                        //
                        // There are another process for this domain - abort
                        //
                        throw common::exception::invalid::Process( "only a single process of this type is allowed in a domain", __FILE__, __LINE__);
                     }

                     common::log::internal::debug << "process singleton queue file " << path << " is adopted by " << common::process::id() << std::endl;
                  }

               }
               catch( const common::exception::signal::Timeout&)
               {
                  common::log::error << "failed to get ping response from potentially running process - to risky to start - action: terminate" << std::endl;
                  throw common::exception::invalid::Process( "only a single process of this type is allowed in a domain", __FILE__, __LINE__);
               }
               catch( const common::exception::queue::Unavailable&)
               {
                  common::log::internal::debug << "process singleton queue file " << path << " is adopted by " << common::process::id() << std::endl;
               }
            }

            file::scoped::Path result( std::move( path));


            std::ofstream output( result);

            if( output)
            {
               output << communication::ipc::inbound::id() << std::endl;
               output << uuid() << std::endl;
            }
            else
            {
               throw common::exception::invalid::File( "failed to write process singleton queue file: " + path);
            }

            return result;
         }



         Handle singleton( const Uuid& identification, bool wait)
         {
            message::lookup::process::Request request;
            request.directive = wait ? message::lookup::process::Request::Directive::wait : message::lookup::process::Request::Directive::direct;
            request.identification = identification;
            request.process = process::handle();

            return communication::ipc::call( communication::ipc::broker::id(), request).process;
         }

         Handle ping( platform::queue_id_type queue)
         {
            message::server::ping::Request request;
            request.process = process::handle();

            return communication::ipc::call( queue, request).process;
         }

         namespace lifetime
         {
            Exit::operator bool () const { return pid != 0;}

            bool Exit::deceased() const
            {
               return reason == Reason::core || reason == Reason::exited || reason == Reason::signaled;
            }

            bool operator == ( platform::pid_type pid, const Exit& rhs) { return pid == rhs.pid;}
            bool operator == ( const Exit& lhs, platform::pid_type pid) { return pid == lhs.pid;}
            bool operator < ( const Exit& lhs, const Exit& rhs) { return lhs.pid < rhs.pid;}

            std::ostream& operator << ( std::ostream& out, const Exit& terminated)
            {
               out << "{pid: " << terminated.pid << ", reason: ";
               switch( terminated.reason)
               {
                  case Exit::Reason::unknown: out << "unknown"; break;
                  case Exit::Reason::exited: out << "exited"; break;
                  case Exit::Reason::stopped: out << "stopped"; break;
                  case Exit::Reason::continued: out << "continued"; break;
                  case Exit::Reason::signaled: out <<  "signal[ " << signal::type::string( terminated.status) << ']'; break;
                  case Exit::Reason::core: out <<  "core"; break;
               }
               return out << '}';
            }

            std::vector< lifetime::Exit> ended()
            {
               std::vector< lifetime::Exit> result;

               Exit exit;

               while( true)
               {
                  auto exit = local::wait( -1);
                  if( exit)
                  {
                     result.push_back( exit);
                  }
                  else
                  {
                     return result;
                  }
               }
               return result;
            }



            std::vector< Exit> wait( const std::vector< platform::pid_type> pids)
            {
               std::vector< Exit> result;

               local::wait( pids, result);

               return result;
            }

            std::vector< Exit> wait( const std::vector< platform::pid_type> pids, std::chrono::microseconds timeout)
            {
               trace::internal::Scope trace{ "common::process::lifetime::wait"};


               log::internal::debug << "wait for pids: " << range::make( pids) << std::endl;

               if( pids.empty())
                  return {};

               std::vector< Exit> result;
               try
               {
                  signal::timer::Scoped alarm( timeout);

                  local::wait( pids, result);

               }
               catch( const exception::signal::Timeout&)
               {

               }
               return result;
            }


            std::vector< Exit> terminate( std::vector< platform::pid_type> pids)
            {
               return wait( process::terminate( pids));
            }

            std::vector< Exit> terminate( std::vector< platform::pid_type> pids, std::chrono::microseconds timeout)
            {
               return wait( process::terminate( pids), timeout);
            }

         } // lifetime


      } // process
   } // common
} // casual


