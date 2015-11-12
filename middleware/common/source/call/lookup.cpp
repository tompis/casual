//!
//! lookup.cpp
//!
//! Created on: Jun 26, 2015
//!     Author: Lazan
//!

#include "common/call/lookup.h"

#include "common/queue.h"

namespace casual
{
   namespace common
   {
      namespace call
      {
         namespace service
         {
            Lookup::Lookup( std::string service, message::service::lookup::Request::Context context) : m_service( std::move( service))
            {
               message::service::lookup::Request request;
               request.requested = m_service;
               request.process = process::handle();
               request.context = context;

               queue::blocking::Send send;
               m_correlation = send( ipc::broker::id(), request);
            }

            Lookup::Lookup( std::string service) : Lookup( std::move( service), message::service::lookup::Request::Context::regular) {}

            Lookup::~Lookup()
            {
               if( ! m_service.empty())
               {
                  //
                  //
                  //
               }

               if( m_correlation != uuid::empty())
               {
                  ipc::receive::queue().discard( m_correlation);
               }
            }

            message::service::lookup::Reply Lookup::operator () () const
            {
               assert(  m_correlation != uuid::empty());

               message::service::lookup::Reply result;
               queue::blocking::Reader receive( ipc::receive::queue());
               receive( result, m_correlation);

               if( result.state != message::service::lookup::Reply::State::busy)
               {
                  //
                  // We're not expecting another message from broker
                  //
                  m_correlation = Uuid{};
               }
               return result;
            }

         } // service

      } // call
   } // common

} // casual