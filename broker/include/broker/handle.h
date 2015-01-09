//!
//! casual_broker_transform.h
//!
//! Created on: Jun 15, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_TRANSFORM_H_
#define CASUAL_BROKER_TRANSFORM_H_


#include "broker/state.h"
#include "broker/broker.h"

#include "common/message/server.h"
#include "common/message/transaction.h"

#include "common/server/handle.h"
#include "common/server/context.h"




namespace casual
{

	namespace broker
	{

      namespace handle
      {
         using state::Base;


         void boot( State& state);


         //!
         //! Shutdown
         //!
         void shutdown( State& state);



         //!
         //! Monitor Connect
         //!
         struct MonitorConnect: public Base
         {
            typedef common::message::monitor::Connect message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };

         //!
         //! Monitor Disconnect
         //!
         struct MonitorDisconnect: public Base
         {
            typedef common::message::monitor::Disconnect message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };

         namespace transaction
         {

            namespace manager
            {

               //!
               //! Transaction Manager Connect
               //!
               struct Connect : public Base
               {
                  using message_type = common::message::transaction::manager::Connect;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

               //!
               //! Transaction Manager Ready
               //!
               struct Ready : public Base
               {
                  using message_type = common::message::transaction::manager::Ready;

                  using Base::Base;

                  void operator () ( message_type& message);
               };

            } // manager


            namespace client
            {
               //!
               //! Transaction Client Connect
               //!
               struct Connect : public Base
               {

                  using message_type = common:: message::transaction::client::connect::Request;

                  using Base::Base;

                  void operator () ( message_type& message);
               };
            } // client
         } // transaction



         //!
         //! Advertise 0..N services for a server.
         //!
         struct Advertise : public Base
         {
            typedef common::message::service::Advertise message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };


         struct Connect : public Base
         {
            typedef common::message::server::connect::Request message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };



		   //!
         //! Unadvertise 0..N services for a server.
         //!
		   struct Unadvertise : public Base
         {
            typedef common::message::service::Unadvertise message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };


		   //!
         //! Looks up a service-name
         //!
         struct ServiceLookup : public Base
         {
            typedef common::message::service::name::lookup::Request message_type;

            using Base::Base;

            void operator () ( message_type& message);

         };

         //!
         //! Handles ACK from services.
         //!
         //! if there are pending request for the "acked-service" we
         //! send response directly
         //!
         struct ACK : public Base
         {

            typedef common::message::service::ACK message_type;

            using Base::Base;

            void operator () ( message_type& message);
         };





         //!
         //! Broker needs to have it's own policy for callee::handle::basic_call, since
         //! we can't communicate with blocking to the same queue (with read, who is
         //! going to write? with write, what if the queue is full?)
         //!
         struct Policy
         {

            Policy( broker::State& state) : m_state( state) {}

            Policy( Policy&&) = default;
            Policy& operator = ( Policy&&) = default;


            void connect( std::vector< common::message::Service> services, const std::vector< common::transaction::Resource>& resources);

            void reply( common::platform::queue_id_type id, common::message::service::Reply& message);

            void ack( const common::message::service::callee::Call& message);

            void transaction( const common::message::service::callee::Call&, const common::server::Service&, const common::platform::time_point&);

            void transaction( const common::message::service::Reply& message);

            void statistics( common::platform::queue_id_type id, common::message::monitor::Notify& message);

         private:

            broker::State& m_state;

         };

         typedef common::server::handle::basic_call< broker::handle::Policy> Call;


		} // handle

	} // broker
} // casual


#endif /* CASUAL_BROKER_TRANSFORM_H_ */