//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


//#include "common/ipc.h"
#include "common/communication/message.h"
#include "common/communication/ipc.h"
#include "common/pimpl.h"
#include "common/platform.h"
#include "common/message/type.h"
#include "common/message/dispatch.h"

#include "common/serialize/native/binary.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace mockup
         {
            using Disconnect =  common::message::basic_message< common::message::Type::mockup_disconnect>;
            using Clear =  common::message::basic_message< common::message::Type::mockup_clear>;

            namespace thread
            {
               using Process = common::message::basic_request< common::message::Type::mockup_need_worker_process>;
            } // thread

         }
      }

      namespace mockup
      {
         namespace pid
         {
            strong::process::id next();

         } // pid

         namespace ipc
         {

            using id_type = strong::ipc::id;

            using transform_type = std::function< std::vector< communication::message::Complete>( communication::message::Complete&)>;

            namespace eventually
            {

               Uuid send( id_type destination, communication::message::Complete&& complete);

               template< typename M, typename C = serialize::native::binary::create::Output>
               Uuid send( id_type destination, M&& message, C creator = serialize::native::binary::create::Output{})
               {
                  return send( destination, serialize::native::complete( std::forward< M>( message), creator));
               }


            } // eventually



            //!
            //! Collects messages from input and put them in output
            //! caches messages if the output is full
            //!
            //!
            struct Collector
            {
               Collector();
               ~Collector();


               //!
               //! input-queue is owned by the Collector
               //!
               id_type input() const;
               inline id_type id() const { return input();}

               //!
               //! output-queue is owned by the Collector
               //!
               communication::ipc::inbound::Device& output() const;

               process::Handle process() const;

               void clear();

               friend std::ostream& operator << ( std::ostream& out, const Collector& value);

            private:
               struct Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };



            //!
            //! Replies to a request
            //!
            //!
            struct Replier
            {
               //!
               //! @param replier invoked on receive, and could send a reply
               //!
               Replier( communication::ipc::dispatch::Handler&& replier);

               ~Replier();


               Replier( Replier&&) noexcept;
               Replier& operator = ( Replier&&) noexcept;

               void add( communication::ipc::dispatch::Handler&& handler);

               //!
               //! input-queue is owned by the Replier
               //!
               id_type input() const;
               process::Handle process() const;


               friend std::ostream& operator << ( std::ostream& out, const Replier& value);

            private:
               struct Implementation;
               move::basic_pimpl< Implementation> m_implementation;
            };


         } // ipc

      } // mockup
   } // common


} // casual


