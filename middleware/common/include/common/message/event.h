//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_MESSAGE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_MESSAGE_H_

#include "common/message/type.h"

#include "common/domain.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace event
         {

            namespace subscription
            {
               using base_begin = common::message::basic_request< common::message::Type::event_subscription_begin>;
               struct Begin : base_begin
               {
                  std::vector< common::message::Type> types;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_begin::marshal( archive);
                     archive & types;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Begin& value);
               };

               using base_end = common::message::basic_request< common::message::Type::event_subscription_end>;
               struct End : base_end
               {

                  friend std::ostream& operator << ( std::ostream& out, const End& value);
               };

            } // subscription

            namespace unsubscribe
            {
               using base_request = common::message::basic_request< common::message::Type::event_unsubscribe_request>;
               struct Request : base_request
               {
                  std::vector< common::message::Type> types;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_request::marshal( archive);
                     archive & types;
                  )
               };
            } // unsubscribe


            namespace domain
            {
               using base_error = common::message::basic_message< common::message::Type::event_domain_error>;
               struct Error : base_error
               {
                  std::string message;

                  enum class Severity : char
                  {
                     fatal, // shutting down
                     error, // keep going
                     warning
                  } severity = Severity::error;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_error::marshal( archive);
                     archive & message;
                     archive & severity;
                  )
               };

               namespace server
               {
                  using base_connect = common::message::basic_message< common::message::Type::event_domain_server_connect>;
                  struct Connect : base_connect
                  {
                     common::process::Handle process;
                     common::Uuid identification;


                     CASUAL_CONST_CORRECT_MARSHAL(
                        base_connect::marshal( archive);
                        archive & process;
                        archive & identification;
                     )

                  };

               } // server

               using base_group = common::message::basic_message< common::message::Type::event_domain_group>;
               struct Group : base_group
               {
                  enum class Context : int
                  {
                     boot_start,
                     boot_end,
                     shutdown_start,
                     shutdown_end,
                  };

                  std::size_t id = 0;
                  std::string name;
                  Context context;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_group::marshal( archive);
                     archive & id;
                     archive & name;
                     archive & context;
                  )
               };

            } // domain

            namespace boot
            {

               using base_start = common::message::basic_message< common::message::Type::event_domain_boot_start>;
               struct Start : base_start
               {
                  common::domain::Identity domain;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_start::marshal( archive);
                     archive & domain;
                  )
               };

               using base_end = common::message::basic_message< common::message::Type::event_domain_boot_end>;
               struct End : base_end
               {
                  common::domain::Identity domain;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_end::marshal( archive);
                     archive & domain;
                  )
               };




            } // boot



            namespace process
            {
               using base_spawn = common::message::basic_message< common::message::Type::event_process_spawn>;
               struct Spawn : base_spawn
               {
                  std::string path;
                  std::vector< platform::pid::type> pids;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_spawn::marshal( archive);
                     archive & path;
                     archive & pids;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Spawn& value);
               };

               using base_exit = common::message::basic_message< common::message::Type::event_process_exit>;
               struct Exit : base_exit
               {
                  Exit() = default;
                  Exit( common::process::lifetime::Exit state) : state( std::move( state)) {}

                  common::process::lifetime::Exit state;

                  CASUAL_CONST_CORRECT_MARSHAL(
                     base_exit::marshal( archive);
                     archive & state;
                  )

                  friend std::ostream& operator << ( std::ostream& out, const Exit& value);
               };
            } // process
         } // event

         namespace reverse
         {
            //template<>
            //struct type_traits< event::message::subscribe::Request> : detail::type< event::message::subscribe::Reply> {};

         } // reverse
      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_MESSAGE_H_
