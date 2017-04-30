//!
//! casual 
//!

#include "common/service/conversation/context.h"
#include "common/service/lookup.h"

#include "common/log.h"
#include "common/buffer/transport.h"

#include "common/message/conversation.h"

#include "common/communication/ipc.h"


namespace casual
{

   namespace common
   {
      namespace service
      {
         namespace conversation
         {

            namespace local
            {
               namespace
               {
                  namespace prepare
                  {
                     template< typename Message, typename... Args>
                     Message message( const State::descriptor_type& descriptor, Args&&... args)
                     {
                        Message result{ std::forward< Args>( args)...};
                        result.correlation = descriptor.correlation;
                        result.route = descriptor.route;
                        result.process = process::handle();

                        return result;
                     }

                     auto connect(
                           State& state,
                           platform::time::point::type& start,
                           common::buffer::payload::Send buffer,
                           connect::Flags flags,
                           const message::service::call::Service& service)
                     {
                        message::conversation::connect::caller::Request message{ std::move( buffer)};


                        constexpr auto valid_flags = ~message::conversation::connect::Flags{};

                        message.correlation = uuid::make();
                        message.service = service;
                        message.process = process::handle();
                        message.flags = valid_flags.convert( flags);

                        //
                        // we push the ipc-queue-id that this instance has. This will
                        // be the last node (for the route when the other server communicate
                        // with us, in the "reverse" order).
                        //
                        message.recording.nodes.push_back( { communication::ipc::inbound::id()});

                        return message;
                     }

                     message::conversation::caller::Send send(
                           const State::descriptor_type& descriptor,
                           common::buffer::payload::Send&& buffer,
                           send::Flags flags)
                     {
                        return prepare::message< message::conversation::caller::Send>( descriptor, std::move( buffer));
                     }

                     auto& descriptor( State& state, connect::Flags flags)
                     {
                        auto& result = state.descriptors.reserve( uuid::make());
                        result.duplex = flags.exist( connect::Flag::receive_only) ?
                              state::descriptor::Information::Duplex::receive
                            : state::descriptor::Information::Duplex::send;

                        result.initiator = true;

                        return result;
                     }

                  } // prepare

                  namespace validate
                  {
                     void flags( connect::Flags flags)
                     {
                        constexpr connect::Flags duplex{ connect::Flag::send_only, connect::Flag::receive_only};

                        if( ( flags & duplex) == duplex || ! ( flags & duplex))
                        {
                           throw exception::xatmi::invalid::Argument{ "send or receive intention must be provided", CASUAL_NIP( flags)};
                        }
                     }

                     void send( const State::descriptor_type& descriptor)
                     {
                        Trace trace{ "common::service::conversation::local::validate::send"};

                        log::debug << "descriptor: " << descriptor << '\n';

                        if( descriptor.duplex != state::descriptor::Information::Duplex::send)
                        {
                           throw exception::xatmi::Protocoll{ "caller has not the control of the conversation"};
                        }
                     }

                     void receive( const State::descriptor_type& descriptor)
                     {
                        Trace trace{ "common::service::conversation::local::validate::receive"};

                        log::debug << "descriptor: " << descriptor << '\n';

                        if( descriptor.duplex != state::descriptor::Information::Duplex::receive)
                        {
                           throw exception::xatmi::Protocoll{ "caller has not the control of the conversation"};
                        }
                     }

                     void diconnect( const State::descriptor_type& descriptor)
                     {
                        Trace trace{ "common::service::conversation::local::validate::diconnect"};

                        log::debug << "descriptor: " << descriptor << '\n';

                        if( ! descriptor.initiator)
                        {
                           throw exception::xatmi::invalid::Descriptor{ "not the initiator of the conversation"};
                        }
                     }

                  } // validate

                  namespace check
                  {
                     void disconnect( const State::descriptor_type& descriptor)
                     {
                        message::conversation::Disconnect disconnect;

                        if( communication::ipc::non::blocking::receive(
                              communication::ipc::inbound::device(),
                              disconnect,
                              descriptor.correlation))
                        {
                           throw exception::conversation::Event{ disconnect.events | Event::disconnect};
                        }
                     }

                  } // check

                  namespace route
                  {
                     template< typename M>
                     void send( M&& message)
                     {
                        auto node = message.route.next();
                        communication::ipc::blocking::send( node.address, message);
                     }
                  } // route


               } // <unnamed>
            } // local

            Context& Context::instance()
            {
               static Context singleton;
               return singleton;
            }

            Context::Context() = default;

            Context::~Context()
            {
               if( pending())
               {
                  log::category::error << "pending conversations: " << m_state.descriptors << '\n';

               }
            }


            descriptor::type Context::connect(
                  const std::string& service,
                  common::buffer::payload::Send buffer,
                  connect::Flags flags)
            {
               Trace trace{ "common::service::conversation::connect"};

               local::validate::flags( flags);

               service::Lookup lookup{ service};

               log::debug << "service: " << service << " buffer: " << buffer << " flags: " << flags << '\n';


               auto start = platform::time::clock::type::now();

               auto& descriptor = local::prepare::descriptor( m_state, flags);

               log::debug << "descriptor: " << descriptor << '\n';

               //
               // If some thing goes wrong we unreserve the descriptor
               //
               auto unreserve = common::scope::execute( [&](){ m_state.descriptors.unreserve( descriptor.descriptor);});

               //
               // TODO: Invoke pre-transport buffer modifiers
               //
               //buffer::transport::Context::instance().dispatch( data, size, service, buffer::transport::Lifecycle::pre_call);


               auto target = lookup();

               if( target.state == message::service::lookup::Reply::State::absent)
               {
                  throw common::exception::xatmi::service::no::Entry( service);
               }

               //
               // The service exists. Take care of reserving descriptor and determine timeout
               //
               auto message = local::prepare::connect( m_state, start, std::move( buffer), flags, target.service);
               message.correlation = descriptor.correlation;



               //
               // If something goes wrong (most likely a timeout), we need to send ack to broker in that case, cus the service(instance)
               // will not do it...
               //
               auto send_ack = common::scope::execute( [&]()
                  {
                     message::service::call::ACK ack;
                     ack.process = target.process;
                     ack.service = target.service.name;
                     communication::ipc::blocking::send( communication::ipc::broker::device(), ack);
                  });

               if( target.state == message::service::lookup::Reply::State::busy)
               {
                  //
                  // We wait for an instance to become idle.
                  //
                  target = lookup();
               }

               //
               // connect to the service
               //
               {
                  message.service = target.service;

                  log::debug << "connect - request: " << message << '\n';

                  auto reply = communication::ipc::call( target.process.queue, message);

                  log::debug << "connect - reply: " << reply << '\n';

                  descriptor.route = std::move( reply.recording);
               }

               unreserve.release();
               send_ack.release();


               return descriptor.descriptor;
            }

            common::Flags< Event> Context::send( descriptor::type handle, common::buffer::payload::Send&& buffer, common::Flags< send::Flag> flags)
            {
               Trace trace{ "common::service::conversation::send"};

               auto& descriptor = m_state.descriptors.get( handle);

               local::validate::send( descriptor);

               auto unreserve = common::scope::execute( [&](){ m_state.descriptors.unreserve( descriptor.descriptor);});

               local::check::disconnect( descriptor);


               auto message = local::prepare::send( descriptor, std::move( buffer), flags);

               //
               // Check if the user wants to transfer the control of the conversation.
               //
               if( flags & send::Flag::receive_only)
               {
                  descriptor.duplex = decltype( descriptor.duplex)::receive;
                  message.events = { Event::send_only};
               }

               local::route::send( message);

               unreserve.release();

               return {};

            }

            receive::Result Context::receive( descriptor::type handle, common::Flags< receive::Flag> flags)
            {
               Trace trace{ "common::service::conversation::receive"};

               auto& descriptor = m_state.descriptors.get( handle);

               local::validate::receive( descriptor);

               auto unreserve = common::scope::execute( [&](){ m_state.descriptors.unreserve( descriptor.descriptor);});

               local::check::disconnect( descriptor);

               message::conversation::callee::Send message;

               if( ! communication::ipc::receive::message(
                     communication::ipc::inbound::device(),
                     message,
                     flags & receive::Flag::no_block ?
                           communication::ipc::receive::Flag::non_blocking : communication::ipc::receive::Flag::blocking,
                     descriptor.correlation))
               {
                  throw common::exception::xatmi::no::Message();
               }

               log::debug << "message: " << message << '\n';

               receive::Result result;
               result.buffer = std::move( message.buffer);
               result.event = message.events;

               constexpr Events termination_events{
                  Event::disconnect, Event::service_error, Event::service_fail, Event::service_success};

               if( result.event & termination_events)
               {
                  //
                  // Other side has terminated the conversation
                  //


               }
               else
               {
                  unreserve.release();
               }


               return result;
            }

            void Context::disconnect( descriptor::type handle)
            {
               Trace trace{ "common::service::conversation::disconnect"};

               auto& descriptor = m_state.descriptors.get( handle);

               local::validate::diconnect( descriptor);

               auto unreserve = common::scope::execute( [&](){ m_state.descriptors.unreserve( descriptor.descriptor);});

               {
                  auto message = local::prepare::message< message::conversation::Disconnect>( descriptor);
                  message.events = Event::disconnect;
                  local::route::send( message);
               }


               //
               // If we're not in control we "need" to discard the possible incoming message
               //
               if( descriptor.duplex == state::descriptor::Information::Duplex::receive)
               {
                  communication::ipc::inbound::device().discard( descriptor.correlation);
               }

            }

            bool Context::pending() const
            {
               return m_state.descriptors.active();
            }

         } // conversation

      } // service
   } // common

} // casual