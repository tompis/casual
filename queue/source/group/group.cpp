//!
//! queue.cpp
//!
//! Created on: Jun 6, 2014
//!     Author: Lazan
//!

#include "queue/group/group.h"
#include "queue/group/handle.h"

#include "queue/environment.h"

#include "common/message/dispatch.h"


namespace casual
{
   namespace queue
   {

      namespace group
      {

         Server::Server( Settings settings) : m_state( std::move( settings.queuebase))
         {
            //
            // Talk to queue-broker to get configuration
            //

            group::queue::blocking::Writer queueBroker{ environment::broker::queue::id(), m_state};

            {
               common::message::queue::connect::Request request;
               request.server = common::message::server::Id::current();
               queueBroker( request);
            }

            {
               std::vector< std::string> existing;
               for( auto&& queue : m_state.queuebase.queues())
               {
                  existing.push_back( queue.name);
               }



               group::queue::blocking::Reader read( common::ipc::receive::queue(), m_state);
               common::message::queue::connect::Reply reply;
               read( reply);

               std::vector< std::string> added;

               for( auto&& queue : reply.queues)
               {
                  auto exists = common::range::find( existing, queue.name);

                  if( ! exists)
                  {
                     m_state.queuebase.create( Queue{ queue.name, queue.retries});
                     added.push_back( queue.name);
                  }
               }


               //
               // Try to remove queues
               // TODO:
               //
               //auto removed = common::range::difference( existing, added);


               //
               // Send all our queues to queue-broker
               //
               common::message::queue::Information information;
               information.server = common::message::server::Id::current();
               information.queues = m_state.queuebase.queues();

               queueBroker( information);

            }


         }


         void Server::start()
         {
            common::message::dispatch::Handler handler{
               handle::enqueue::Request{ m_state},
               handle::dequeue::Request{ m_state},
               handle::transaction::commit::Request{ m_state},
               handle::transaction::rollback::Request{ m_state},
               handle::information::queues::Request{ m_state},
            };


            group::queue::blocking::Reader blockedRead( common::ipc::receive::queue(), m_state);

            while( true)
            {
               {
                  auto persistent = sql::database::scoped::write( m_state.queuebase);


                  handler.dispatch( blockedRead.next());

                  //
                  // Consume until the queue is empty or we've got pending replies equal to transaction_batch
                  //

                  group::queue::non_blocking::Reader nonBlocking( common::ipc::receive::queue(), m_state);

                  while( handler.dispatch( nonBlocking.next()) &&
                        m_state.persistent.size() < common::platform::transaction_batch)
                  {
                     ;
                  }
               }

               //
               // queuebase is persistent - send pending persistent replies
               //
               group::queue::non_blocking::Send send{ m_state};

               auto remain = common::range::remove_if(
                  m_state.persistent,
                  common::message::pending::sender( send));

               m_state.persistent.erase( remain.last, std::end( m_state.persistent));


            }

         }
      } // server

   } // queue

} // casual
