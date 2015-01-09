//!
//! casual_calling_context.cpp
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#include "common/call/context.h"
#include "common/call/timeout.h"

#include "common/queue.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"

#include "common/buffer/pool.h"
#include "common/buffer/transport.h"

#include "common/environment.h"
#include "common/flag.h"
#include "common/error.h"
#include "common/exception.h"
#include "common/signal.h"

#include "common/transaction/context.h"

#include "xatmi.h"

//
// std
//
#include <algorithm>
#include <cassert>

namespace casual
{
   namespace common
   {
      namespace call
      {
         namespace local
         {
            namespace
            {

               namespace queue
               {
                  struct Policy
                  {
                     void apply()
                     {
                        try
                        {
                           throw;
                        }
                        catch( const exception::signal::Timeout&)
                        {
                           throw exception::xatmi::Timeout{};
                        }
                     }
                  };

                  namespace blocking
                  {
                     using Send = common::queue::blocking::basic_send< Policy>;
                     using Receive = common::queue::blocking::basic_reader< Policy>;

                  } // blocking

                  namespace non_blocking
                  {
                     using Receive = common::queue::non_blocking::basic_reader< Policy>;

                  } // non_blocking

               } // queue


               namespace service
               {
                  struct Lookup
                  {
                     Lookup( const std::string& service)
                     {
                        message::service::name::lookup::Request serviceLookup;
                        serviceLookup.requested = service;
                        serviceLookup.process = process::handle();

                        queue::blocking::Send send;
                        send( ipc::broker::id(), serviceLookup);
                     }

                     message::service::name::lookup::Reply operator () () const
                     {
                        message::service::name::lookup::Reply result;
                        queue::blocking::Receive receive( ipc::receive::queue());
                        receive( result);

                        return result;
                     }
                  };

               } // service


               namespace validate
               {

                  inline void input( const char* buffer, long size, long flags)
                  {
                     if( buffer == nullptr)
                     {
                        throw exception::xatmi::InvalidArguments{ "buffer is null"};
                     }
                     if( flag< TPNOREPLY>( flags) && ! flag< TPNOTRAN>( flags))
                     {
                        throw exception::xatmi::InvalidArguments{ "TPNOREPLY can only be used with TPNOTRAN"};
                     }
                  }

               } // validate


            } // <unnamed>

         } // local

         State::Pending::Pending()
          : m_descriptors{
            { 1, false },
            { 2, false },
            { 3, false },
            { 4, false },
            { 5, false },
            { 6, false },
            { 7, false },
            { 8, false }}
         {

         }


         descriptor_type State::Pending::reserve( const Uuid& correlation, const transaction::ID& transaction)
         {
            auto descriptor = reserve( correlation);

            auto found = range::find( m_transactions, transaction);

            if( found)
            {
               found->add( descriptor);
            }
            else
            {
               m_transactions.emplace_back( transaction, descriptor);
            }

            return descriptor;
         }

         descriptor_type State::Pending::reserve( const Uuid& correlation)
         {
            auto descriptor = reserve();

            m_correlations.emplace_back( descriptor, correlation);

            return descriptor;
         }

         descriptor_type State::Pending::reserve()
         {
            auto found = range::find_if( m_descriptors, negate( std::mem_fn( &Descriptor::active)));

            if( found)
            {
               found->active = true;
               return found->descriptor;
            }
            else
            {
               m_descriptors.emplace_back( m_descriptors.back().descriptor + 1, true);
               return m_descriptors.back().descriptor;
            }
         }

         void State::Pending::unreserve( descriptor_type descriptor)
         {

            {
               auto found = range::find( m_descriptors, descriptor);

               if( found)
               {
                  found->active = false;
               }
               else
               {
                  throw exception::invalid::Argument{ "invalid call descriptor: " + std::to_string( descriptor)};
               }
            }

            //
            // Remove message correlation association
            //
            {
               auto found = range::find( m_correlations, descriptor);
               if( found)
               {
                  m_correlations.erase( found.first);
               }
            }

            //
            // Remove transaction association
            //
            {
               auto found = range::find( m_transactions, descriptor);
               if( found && found->remove( descriptor))
               {
                  m_transactions.erase( found.first);
               }
            }
         }

         bool State::Pending::active( descriptor_type descriptor) const
         {
            auto found = range::find( m_descriptors, descriptor);

            if( found)
            {
               return found->active;
            }
            return false;
         }

         const Uuid& State::Pending::correlation( descriptor_type descriptor) const
         {
            auto found = range::find( m_correlations, descriptor);
            if( found)
            {
               return found->correlation;
            }
            throw exception::invalid::Argument{ "invalid call descriptor: " + std::to_string( descriptor)};
         }

         void State::Pending::discard( descriptor_type descriptor)
         {
            //
            // Can't be associated with a transaction
            //
            if( range::find( m_transactions, descriptor))
            {
               throw exception::xatmi::TransactionNotSupported{ "descriptor " + std::to_string( descriptor) + " is associated with a transaction"};
            }

            //
            // Discards the correlation (directly if in cache, or later if not)
            //
            ipc::receive::queue().discard( correlation( descriptor));

            unreserve( descriptor);
         }


         State::Reply::Cache::cache_range State::Reply::Cache::add( message::service::Reply&& value)
         {
            m_cache.push_back( std::move( value));
            return range::back( m_cache);
         }

         State::Reply::Cache::cache_range State::Reply::Cache::search( descriptor_type descriptor)
         {
            return range::find_if( m_cache, [=]( const cache_type::value_type& r){
               return r.descriptor == descriptor;
            });

         }

         void State::Reply::Cache::erase( cache_range range)
         {
            if( range)
            {
               m_cache.erase( range.first);
            }
         }




         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         void Context::execution( const Uuid& uuid)
         {
            if( ! uuid)
            {
               m_state.execution = uuid::make();
            }
            else
            {
               m_state.execution = uuid;
            }
         }

         const common::Uuid& Context::execution() const
         {
            return m_state.execution;
         }



         void Context::service( const std::string& service)
         {
            m_state.service = service;
         }

         const std::string& Context::service() const
         {
            return m_state.service;
         }



         descriptor_type Context::async( const std::string& service, char* idata, long ilen, long flags)
         {
            trace::internal::Scope trace( "calling::Context::asyncCall");

            local::validate::input( idata, ilen, flags);

            local::service::Lookup lookup( service);

            //
            // We do as much as possible while we wait for the broker reply
            //

            auto start = platform::clock_type::now();

            //
            // Invoke pre-transport buffer modifiers
            //
            buffer::transport::Context::instance().dispatch( idata, ilen, service, buffer::transport::Lifecycle::pre_call);

            message::service::caller::Call message( buffer::pool::Holder::instance().get( idata));

            //
            // Prepare message
            //
            {

               message.correlation = uuid::make();

               auto& transaction = transaction::Context::instance().current();

               //
               // Check if we should associate descriptor with message-correlation and transaction
               //
               if( flag< TPNOREPLY>( flags))
               {
                  //
                  // No reply, hence no descriptor and no transaction (we validated this before)
                  //
                  message.descriptor = 0;
               }
               else if( ! flag< TPNOTRAN>( flags) && transaction)
               {
                  message.descriptor = m_state.pending.reserve( message.correlation, transaction.trid);
                  message.trid = transaction.trid;
               }
               else
               {
                  message.descriptor = m_state.pending.reserve( message.correlation);
               }

               message.reply = process::handle();
               message.execution = m_state.execution;
               message.caller = m_state.service;
               message.flags = flags;

               log::internal::debug << "descriptor: " << message.descriptor << " service: " << service << " data: @" << static_cast< void*>( idata) << " len: " << ilen << " flags: " << flags << std::endl;
            }



            //
            // Get a queue corresponding to the service
            //
            auto target = lookup();

            if( ! target.process)
            {
               throw common::exception::xatmi::service::NoEntry( service);
            }

            //
            // Keep track of timeouts
            //
            // TODO: this can cause a timeout directly - need to send ack to broker in that case...
            //
            if( message.descriptor != 0)
            {
               Timeout::instance().add(
                     message.descriptor,
                     flag< TPNOTIME>( flags) ? std::chrono::microseconds{ 0} : target.service.timeout,
                     start);
            }


            //
            // Call the service
            //
            message.service = target.service;

            local::queue::blocking::Send send;
            send( target.process.queue, message);

            return message.descriptor;
         }



         void Context::reply( descriptor_type& descriptor, char** odata, long& olen, long flags)
         {
            trace::internal::Scope trace( "calling::Context::getReply");

            log::internal::debug << "descriptor: " << descriptor << " data: @" << static_cast< void*>( *odata) << " len: " << olen << " flags: " << flags << std::endl;

            //
            // TODO: validate input...


            if( common::flag< TPGETANY>( flags))
            {
               descriptor = 0;
            }
            else if( ! m_state.pending.active( descriptor))
            {
               throw common::exception::xatmi::service::InvalidDescriptor();
            }

            //
            // Keep track of the current timeout for this descriptor.
            // and make sure we unset the timer regardless
            //
            call::Timeout::Unset unset( descriptor);


            //
            // We fetch, and if TPNOBLOCK is not set, we block
            //
            message::service::Reply reply;

            if( ! receive( reply, descriptor, flags))
            {
               throw common::exception::xatmi::NoMessage();
            }

            //
            // This call is consumed, so we remove the timeout.
            //
            Timeout::instance().remove( descriptor);

            //
            // Update transaction state
            //
            transaction::Context::instance().update( reply);

            //
            // Check buffer types
            //
            if( *odata != nullptr && common::flag< TPNOCHANGE>( flags))
            {
               auto& output = buffer::pool::Holder::instance().get( *odata);

               if( output.type != reply.buffer.type)
               {
                  throw exception::xatmi::buffer::TypeNotExpected{};
               }
            }

            //
            // We always deallocate user output buffer
            //
            {
               buffer::pool::Holder::instance().deallocate( *odata);
               *odata = nullptr;
            }

            //
            // We prepare the output buffer
            //
            {
               descriptor = reply.descriptor;
               *odata = reply.buffer.memory.data();
               olen = reply.buffer.memory.size();

               //
               // Apply post transport buffer manipulation
               // TODO: get service-name
               //
               buffer::transport::Context::instance().dispatch(
                     *odata, olen, "", buffer::transport::Lifecycle::post_call, reply.buffer.type);

               //
               // Add the buffer to the pool
               //
               buffer::pool::Holder::instance().insert( std::move( reply.buffer));
            }



            //
            // We remove pending
            //
            m_state.pending.unreserve( descriptor);

         }


         void Context::sync( const std::string& service, char* idata, const long ilen, char*& odata, long& olen, const long flags)
         {
            auto descriptor = async( service, idata, ilen, flags);
            reply( descriptor, &odata, olen, flags);
         }


         int Context::canccel( descriptor_type cd)
         {
            //for( m_state.pending.)

            return 0;
         }

         void Context::clean()
         {

            //
            // TODO: Do some cleaning on buffers, pending replies and such...
            //

         }

         Context::Context()
         {

         }

         namespace local
         {
            namespace
            {
               template< typename... Args>
               bool receive( message::service::Reply& reply, long flags, Args&... args)
               {
                  if( common::flag< TPNOBLOCK>( flags))
                  {
                     local::queue::non_blocking::Receive receive{ ipc::receive::queue()};
                     return receive( reply, args...);
                  }
                  else
                  {
                     local::queue::blocking::Receive receive{ ipc::receive::queue()};
                     receive( reply, args...);
                  }
                  return true;
               }
            } // <unnamed>
         } // local

         bool Context::receive( message::service::Reply& reply, descriptor_type descriptor, long flags)
         {
            if( descriptor == 0)
            {
               //
               // We fetch any
               //
               return local::receive( reply, flags);
            }
            else
            {
               auto& correlation = m_state.pending.correlation( descriptor);

               return local::receive( reply, flags, correlation);
            }
         }


         Context::cache_range Context::fetch( descriptor_type descriptor, long flags)
         {
            //
            // Vi fetch all on the queue.
            //
            consume();

            auto found = m_state.reply.cache.search( descriptor);

            local::queue::blocking::Receive receive{ ipc::receive::queue()};

            while( ! found && ! common::flag< TPNOBLOCK>( flags))
            {
               message::service::Reply reply;
               receive( reply);

               auto cached = m_state.reply.cache.add( std::move( reply));

               if( cached->descriptor == descriptor)
               {
                  return cached;
               }
            }
            return found;
         }




         void Context::consume()
         {
            //
            // pop from queue until it's empty (at least empty for callReplies)
            //

            local::queue::non_blocking::Receive receive{ ipc::receive::queue()};

            while( true)
            {
               message::service::Reply reply;

               if( ! receive( reply))
               {
                  break;
               }

               m_state.reply.cache.add( std::move( reply));
            }

         }
      } // call
   } // common
} // casual