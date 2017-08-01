//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_

#include "common/service/conversation/state.h"
#include "common/flag/service/conversation.h"

#include "common/buffer/type.h"
#include "common/exception/xatmi.h"

#include "xatmi/defines.h"

namespace casual
{

   namespace common
   {
      namespace exception
      {
         namespace conversation
         {
            using base_exception = xatmi::conversational::Event;
            struct Event : base_exception
            {
               Event( flag::service::conversation::Events event) :
                  event( event) {}

               flag::service::conversation::Events event;
            };

         } // conversation

      } // exception

      namespace service
      {
         namespace conversation
         {
            using Event = flag::service::conversation::Event;
            using Events = flag::service::conversation::Events;

            namespace connect
            {
               using Flag = flag::service::conversation::connect::Flag;
               using Flags = flag::service::conversation::connect::Flags;
            } // connect

            namespace send
            {
               using Flag = flag::service::conversation::send::Flag;
               using Flags = flag::service::conversation::send::Flags;
            } // send

            namespace receive
            {
               using Flag = flag::service::conversation::receive::Flag;
               using Flags = flag::service::conversation::receive::Flags;

               struct Result
               {
                  common::Flags< Event> event;
                  common::buffer::Payload buffer;
               };
            } // receive

            class Context
            {
            public:
               static Context& instance();
               ~Context();

               descriptor::type connect( const std::string& service, common::buffer::payload::Send buffer, connect::Flags flags);

               common::Flags< Event> send( descriptor::type descriptor, common::buffer::payload::Send&& buffer, common::Flags< send::Flag> flags);

               receive::Result receive( descriptor::type descriptor, common::Flags< receive::Flag> flags);

               void disconnect( descriptor::type descriptor);

               inline State::holder_type& descriptors() { return m_state.descriptors;}

               bool pending() const;

            private:
               Context();

               State m_state;

            };

            inline Context& context() { return Context::instance();}


         } // conversation

      } // service
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_CONTEXT_H_
