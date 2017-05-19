//!
//! casual 
//!

#include "common/event/listen.h"

#include "common/message/handle.h"

namespace casual
{
   namespace common
   {

      namespace event
      {

         namespace local
         {
            namespace
            {
               using message_type = common::message::Type;


               namespace location
               {
                  template< typename M>
                  void domain( const M& request, const std::vector< message_type>& types)
                  {
                     if( range::find_if( types, []( message_type type){
                        return type >= message_type::EVENT_DOMAIN_BASE && type < message_type::EVENT_DOMAIN_BASE_END;}))
                     {
                        communication::ipc::blocking::send(
                              communication::ipc::domain::manager::optional::device(),
                              request);
                     }
                  }

                  template< typename M>
                  void service( const M& request, const std::vector< message_type>& types)
                  {
                     if( range::find_if( types, []( message_type type){
                        return type >= message_type::EVENT_SERVICE_BASE && type < message_type::EVENT_SERVICE_BASE_END;}))
                     {
                        communication::ipc::blocking::send(
                              communication::ipc::broker::device(),
                              request);
                     }
                  }

               } // location


               void subscribe( const process::Handle& process, std::vector< message_type> types)
               {

                  message::event::subscription::Begin request;

                  request.process = process;
                  request.types = std::move( types);

                  location::domain( request, request.types);
                  location::service( request, request.types);

               }

               void unsubscribe( const process::Handle& process, std::vector< message_type> types)
               {
                  message::event::subscription::End request;
                  request.process = process;

                  location::domain( request, types);
                  location::service( request, types);

               }


               auto subscription( platform::ipc::id::type ipc, std::vector< message_type> types)
               {
                  process::Handle process{ process::id(), ipc};

                  local::subscribe( process, types);

                  return scope::execute( [&](){
                     local::unsubscribe( process, types);
                  });
               }

               namespace standard
               {
                  handler_type handler( handler_type&& handler)
                  {
                     handler.insert(
                           common::message::handle::Shutdown{},
                           common::message::handle::Ping{});

                     return std::move( handler);
                  }
               } // standard


            } // <unnamed>
         } // local


         namespace detail
         {
            handler_type subscribe( handler_type&& handler)
            {
               Trace trace{ "common::event::detail::subscribe"};

               local::subscribe( process::handle(), handler.types());

               return std::move( handler);
            }

            void listen( device_type& device, handler_type&& h)
            {
               Trace trace{ "common::event::detail::listen"};

               auto handler = local::standard::handler( std::move( h));

               auto subscription = local::subscription( device.connector().id(), handler.types());

               while( true)
               {
                  handler( device.next( typename device_type::blocking_policy{}));
               }

            }

            void listen( device_type& device, std::function< void()> empty, handler_type&& h)
            {
               Trace trace{ "common::event::detail::listen"};

               auto handler = local::standard::handler( std::move( h));

               auto subscription = local::subscription( device.connector().id(), handler.types());

               while( true)
               {
                  while( handler( device.next( typename device_type::non_blocking_policy{})))
                  {
                     ;
                  }

                  //
                  // queue is empty, notify user
                  //
                  empty();

                  handler( device.next( typename device_type::blocking_policy{}));
               }

            }
         } // detail

         namespace no
         {
            namespace subscription
            {
               namespace detail
               {
                  void listen( device_type& device, handler_type&& handler)
                  {
                     handler.insert(
                           common::message::handle::Shutdown{},
                           common::message::handle::Ping{});

                     message::dispatch::blocking::pump( handler, device);
                  }
               } // detail
            } // subscription
         } // no

      } // event
   } // common
} // casual