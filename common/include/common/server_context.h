//!
//! casual_server_context.h
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_


#include "common/message/server.h"
#include "common/message/monitor.h"

#include "common/ipc.h"
#include "common/queue.h"
#include "common/environment.h"

#include "common/calling_context.h"
#include "common/transaction_context.h"
#include "common/platform.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/exception.h"


#include "common/move.h"


#include <xatmi.h>
#include <xa.h>

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

         struct Service
         {

            enum TransactionType
            {
               cAuto = 0,
               cJoin = 1,
               cAtomic = 2,
               cNone = 3
            };


            Service( const std::string& name, tpservice function, long type, TransactionType transaction)
            : name( name), function( function), type( type), transaction( transaction) {}


            Service( Service&&) = default;


            void call( TPSVCINFO* serviceInformation)
            {
               (*function)( serviceInformation);
            }

            std::string name;
            tpservice function;
            long type = 0;
            TransactionType transaction = TransactionType::cAuto;
            bool active = true;


         };

         inline std::ostream& operator << ( std::ostream& out, const Service& service)
         {
            return out << "{name: " << service.name << " type: " << service.type << " transaction: " << service.transaction
                  << " active: " << service.active << "};";
         }


         struct Arguments
         {
            //Arguments() = default;
            Arguments( Arguments&&) = default;

            Arguments( int argc, char** argv) : argc( argc), argv( argv)
            {

            }

            Arguments( std::vector< std::string> args) : arguments( std::move( args))
            {
               for( auto& argument : arguments)
               {
                  c_arguments.push_back( const_cast< char*>( argument.c_str()));
               }
               argc = c_arguments.size();
               argv = c_arguments.data();
            }

            std::vector< Service> services;
            std::function<int( int, char**)> server_init = &tpsvrinit;
            std::function<void()> server_done = &tpsvrdone;
            int argc;
            char** argv;

            std::vector< std::string> arguments;

            std::vector< transaction::Resource> resources;

         private:
            std::vector< char*> c_arguments;
         };

         struct State
         {
            State() = default;

            State( const State&) = delete;
            State& operator = (const State&) = delete;

            typedef std::unordered_map< std::string, Service> service_mapping_type;

            service_mapping_type services;
            common::platform::long_jump_buffer_type long_jump_buffer;

            message::service::Reply reply;
            message::monitor::Notify monitor;

            std::function<void()> server_done;

         };

         class Context
         {
         public:


            static Context& instance();

            Context( const Context&) = delete;


            //!
            //! Being called from tpreturn
            //!
            void longJumpReturn( int rval, long rcode, char* data, long len, long flags);

            //!
            //! Being called from tpadvertise
            //!
            void advertiseService( const std::string& name, tpservice function);

            //!
            //! Being called from tpunadvertise
            //!
            void unadvertiseService( const std::string& name);

            //!
            //! Share state with callee::handle::basic_call for now...
            //! if this "design" feels good, we should expose needed functionality
            //! to callee::handle::basic_call
            //!
            State& state();

            void finalize();


         private:

            Context();

            State m_state;
         };

      } // server

      namespace callee
      {
         namespace handle
         {

            //!
            //! Handles XATMI-calls
            //!
            //! Sematics:
            //! - construction
            //! -- send connect to broker - connect server - advertise all services
            //! - dispatch
            //! -- set longjump
            //! -- call user XATMI-service
            //! -- when user calls tpreturn we longjump back
            //! -- send reply to caller
            //! -- send ack to broker
            //! -- send time-stuff to monitor (if active)
            //! -- transaction stuff
            //! - destruction
            //! -- send disconnect to broker - disconnect server - unadvertise services
            //!
            //! @note it's a template only to be able to unittest it
            //!
            template< typename P>
            struct basic_call
            {
               typedef P policy_type;

               typedef message::service::callee::Call message_type;

               basic_call( basic_call&&) = default;
               basic_call& operator = ( basic_call&&) = default;

               basic_call() = delete;
               basic_call( const basic_call&) = delete;
               basic_call& operator = ( basic_call&) = delete;




               //!
               //! Connect @p server to the broker, broker will build a dispatch-table for
               //! coming XATMI-calls
               //!
               template< typename... Args>
               basic_call( server::Arguments& arguments, Args&&... arg) : m_policy( std::forward< Args>( arg)...)
               {
                  trace::internal::Scope trace{ "callee::handle::basic_call::basic_call"};

                  auto& state = server::Context::instance().state();

                  state.server_done = arguments.server_done;

                  message::server::connect::Request message;


                  for( auto&& service : arguments.services)
                  {
                     message.services.emplace_back( service.name, service.type, service.transaction);
                     state.services.emplace( service.name, std::move( service));
                  }


                  //
                  // Connect to casual
                  //
                  m_policy.connect( message, arguments.resources);

                  //
                  // Call tpsrvinit
                  //
                  if( arguments.server_init( arguments.argc, arguments.argv) != 0)
                  {
                     throw exception::NotReallySureWhatToNameThisException( "service init failed");
                  }

               }


               //!
               //! Sends a message::server::Disconnect to the broker
               //!
               ~basic_call() noexcept
               {
                  if( ! m_moved)
                  {

                     trace::internal::Scope trace{ "callee::handle::basic_call::~basic_call"};

                     try
                     {
                        auto& state = server::Context::instance().state();

                        //
                        // Call tpsrvdone
                        //
                        state.server_done();

                        //
                        // If there are open resources, close'em
                        //
                        transaction::Context::instance().close();

                        m_policy.disconnect();
                     }
                     catch( ...)
                     {
                        error::handler();
                     }
                  }

               }


               //!
               //! Handles the actual XATM-call from another process. Dispatch
               //! to the registered function, and "waits" for tpreturn (long-jump)
               //! to send the reply.
               //!
               void dispatch( message_type& message)
               {
                  //
                  // Set the call-chain-id for this "chain"
                  //
                  calling::Context::instance().callId( message.callId);


                  trace::internal::Scope trace{ "callee::handle::basic_call::dispatch"};

                  auto& state = server::Context::instance().state();

                  //
                  // Set starttime. TODO: Should we try to get the startime earlier? Hence, at ipc-queue level?
                  //
                  if( message.service.monitor_queue != 0)
                  {
                     state.monitor.start = platform::clock_type::now();
                  }


                  //
                  // Prepare for tpreturn.
                  //
                  // ATTENTION: no types with destructor should be instantiated between
                  // setjmp and the else clause for 'if( jumpState == 0)'
                  //
                  int jumpState = setjmp( state.long_jump_buffer);

                  if( jumpState == 0)
                  {

                     //
                     // No longjmp has been called, this is the first time in this "service call"
                     // Let's call the user service...
                     //

                     //
                     // set the call-correlation
                     //
                     state.reply.callDescriptor = message.callDescriptor;

                     auto findIter = state.services.find( message.service.name);

                     if( findIter == state.services.end())
                     {
                        throw common::exception::xatmi::SystemError( "Service [" + message.service.name + "] not present at server - inconsistency between broker and server");
                     }

                     auto& service = findIter->second;

                     //
                     // Do transaction stuff...
                     // - begin transaction if service has "auto-transaction"
                     // - notify TM about potentially resources involved.
                     //
                     m_policy.transaction( message, service);


                     calling::Context::instance().currentService( message.service.name);

                     TPSVCINFO serviceInformation = transformServiceInformation( message);

                     //
                     // Before we call the user function we have to add the buffer to the "buffer-pool"
                     //
                     buffer::Context::instance().add( std::move( message.buffer));

                     service.call( &serviceInformation);

                     //
                     // User service returned, not by tpreturn. The standard does not mention this situation, what to do?
                     //
                     throw common::exception::xatmi::service::Error( "Service: " + message.service.name + " did not call tpreturn");

                  }
                  else
                  {

                     //
                     // Do transaction stuff...
                     // - commit/rollback transaction if service has "auto-transaction"
                     //
                     m_policy.transaction( state.reply);

                     //
                     // User has called tpreturn.
                     // Send reply to caller. We previously "saved" state when the user called tpreturn.
                     // we now use it
                     //
                     m_policy.reply( message.reply.queue_id, state.reply);

                     //
                     // Send ACK to broker
                     //
                     m_policy.ack( message);

                     //
                     // Do some cleanup...
                     //
                     server::Context::instance().finalize();

                     //
                     // Take end time
                     //
                     if( message.service.monitor_queue != 0)
                     {
                        state.monitor.end = platform::clock_type::now();
                        state.monitor.callId = message.callId;
                        state.monitor.service = message.service.name;
                        state.monitor.parentService = message.callee;

                        m_policy.statistics( message.service.monitor_queue, state.monitor);
                     }
                     calling::Context::instance().currentService( "");
                  }
               }
            private:

               TPSVCINFO transformServiceInformation( message::service::callee::Call& message) const
               {
                  TPSVCINFO result;

                  strncpy( result.name, message.service.name.data(), sizeof( result.name) );
                  result.data = platform::public_buffer( message.buffer.raw());
                  result.len = message.buffer.size();
                  result.cd = message.callDescriptor;
                  result.flags = 0;

                  return result;
               }

               policy_type m_policy;
               move::Moved m_moved;
            };

            namespace policy
            {


               //!
               //! Default policy for basic_call. Only broker and unittest have to define another
               //! policy
               //!
               struct Default
               {
                  template< typename W>
                  struct broker_writer : public W
                  {
                     broker_writer() : W( ipc::broker::id()) {}
                  };


                  void connect( message::server::connect::Request& message, const std::vector< transaction::Resource>& resources);

                  void disconnect();

                  void reply( platform::queue_id_type id, message::service::Reply& message);

                  void ack( const message::service::callee::Call& message);

                  void statistics( platform::queue_id_type id, message::monitor::Notify& message);

                  void transaction( const message::service::callee::Call& message, const server::Service& service);
                  void transaction( message::service::Reply& message);

               private:
                  typedef queue::blocking::Writer reply_writer;
                  typedef queue::non_blocking::Writer monitor_writer;
                  typedef broker_writer< queue::blocking::Writer> blocking_broker_writer;
                  typedef broker_writer< queue::non_blocking::Writer> non_blocking_broker_writer;

               };



            } // policy

            //!
            //! Handle service calls from other proceses and does a dispatch to
            //! the register XATMI functions.
            //!
            typedef basic_call< policy::Default> Call;






         } // handle
      } // callee
	} // common
} // casual



#endif /* CASUAL_SERVER_CONTEXT_H_ */
