//!
//! queue.cpp
//!
//! Created on: Jul 22, 2014
//!     Author: Lazan
//!


#include "queue/queue.h"
#include "queue/environment.h"
#include "queue/rm/switch.h"
#include "queue/transform.h"

#include "common/message/queue.h"
#include "common/queue.h"
#include "common/trace.h"


#include "tx.h"



namespace casual
{
   namespace queue
   {

      namespace xatmi
      {
         /*
         struct Message::Implementation
         {

            common::Uuid id;
            std::string correlation;
            std::string reply;
            common::platform::time_type available;

            common::platform::raw_buffer_type payload;

         };

         Message::Message() = default;
         Message::~Message() = default;

         const common::Uuid& Message::id() const
         {
            return m_implementation->id;
         }

         const std::string& Message::correlation() const
         {
            return m_implementation->correlation;
         }

         Message& Message::correlation( std::string value)
         {
            m_implementation->correlation = std::move( value);
            return *this;
         }

         const std::string& Message::reply() const
         {
            return m_implementation->reply;
         }

         Message& Message::reply( std::string value)
         {
            m_implementation->reply = std::move( value);
            return *this;
         }

         common::platform::time_type Message::available() const
         {
            return m_implementation->available;
         }

         Message& Message::available( common::platform::time_type value)
         {
            m_implementation->available = std::move( value);
            return *this;
         }

         common::platform::raw_buffer_type Message::payload() const
         {
            return m_implementation->payload;
         }

         Message& Message::payload( common::platform::raw_buffer_type value)
         {
            m_implementation->payload = value;
            return *this;
         }
         */

      } // xatmi

      namespace local
      {
         namespace
         {
            struct Lookup
            {
               Lookup( const std::string& queue)
               {
                  casual::common::queue::blocking::Writer send( queue::environment::broker::queue::id());

                  common::message::queue::lookup::Request request;
                  request.server = common::message::server::Id::current();
                  request.name = queue;

                  send( request);
               }

               common::message::queue::lookup::Reply operator () () const
               {
                  common::queue::blocking::Reader receive( common::ipc::receive::queue());

                  common::message::queue::lookup::Reply reply;
                  receive( reply);

                  return reply;
               }

            };

            namespace scoped
            {
               struct AX_reg
               {
                  AX_reg( common::transaction::ID& xid) : m_id( xid)
                  {
                     ax_reg( queue::rm::id(), &m_id.xid(), TMNOFLAGS);
                  }

                  ~AX_reg()
                  {
                     if( ! m_id)
                     {
                        ax_unreg( queue::rm::id(), TMNOFLAGS);
                     }

                  }
               private:
                  common::transaction::ID& m_id;
               };
            } // scoped


         } // <unnamed>
      } // local


      common::Uuid enqueue( const std::string& queue, const Message& message)
      {
         common::trace::Scope trace( "queue::enqueue", common::log::internal::queue);

         local::Lookup lookup( queue);

         common::message::queue::enqueue::Request request;
         local::scoped::AX_reg ax_reg( request.xid.xid);

         request.server = common::message::server::Id::current();

         request.message.payload = message.payload.data;
         request.message.type = message.payload.type;
         request.message.correlation = message.attribues.properties;
         request.message.reply = message.attribues.reply;
         request.message.avalible = message.attribues.available;
         request.message.id = common::Uuid::make();

         auto group = lookup();

         if( group.queue == 0)
         {
            throw common::exception::invalid::Argument{ "failed to look up queue: " + queue};
         }

         common::log::internal::queue << "enqueues - queue: " << queue << " group: " << group.queue << " process: " << group.server << std::endl;


         casual::common::queue::blocking::Writer send( group.server.queue_id);
         request.queue = group.queue;

         send( request);

         return request.message.id;
      }


      std::vector< Message> dequeue( const std::string& queue)
      {
         common::trace::Scope trace( "queue::dequeue", common::log::internal::queue);

         std::vector< Message> result;

         local::Lookup lookup( queue);

         common::message::queue::dequeue::Request request;
         local::scoped::AX_reg ax_reg( request.xid.xid);

         {

            request.server = common::message::server::Id::current();

            auto group = lookup();
            casual::common::queue::blocking::Writer send( group.server.queue_id);
            request.queue = group.queue;

            common::log::internal::queue << "dequeues - queue: " << queue << " group: " << group.queue << " process: " << group.server << std::endl;

            send( request);
         }

         {
            casual::common::queue::blocking::Reader receive( common::ipc::receive::queue());
            common::message::queue::dequeue::Reply reply;

            receive( reply);

            common::range::transform( reply.message, result, queue::transform::Message());
         }

         return result;
      }


      namespace peek
      {
         namespace local
         {
            namespace
            {
               namespace transform
               {
                  struct Message
                  {
                     peek::Message operator () ( const common::message::queue::information::Message& message) const
                     {
                        peek::Message result;

                        result.id = message.id;
                        result.type = message.type;
                        result.state = message.state;

                        return result;
                     }
                  };
               } // transform

            } // <unnamed>
         } // local

         std::vector< Message> queue( const std::string& queue)
         {
            std::vector< Message> result;

            {
               casual::common::queue::blocking::Writer send( queue::environment::broker::queue::id());

               common::message::queue::information::queue::Request request;
               request.server = common::message::server::Id::current();
               request.qname = queue;

               send( request);
            }

            {

               common::queue::blocking::Reader receive( common::ipc::receive::queue());

               common::message::queue::information::queue::Reply reply;
               receive( reply);

               common::range::transform( reply.messages, result, local::transform::Message{});

            }
            return result;
         }



      } // peek


   } // queue
} // casual
