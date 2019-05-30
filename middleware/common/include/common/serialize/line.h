//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/archive.h"
#include "common/serialize/traits.h"

#include "common/log/stream.h"

#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace line
         {
            //! serialize serializable objects to one line json-like format
            Writer writer( std::ostream& out);

         } // log
      } // serialize

      namespace stream
      {
         //! Specialization for serial
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            serialize::traits::has::serialize< C, serialize::Writer>::value
            && ! traits::has::ostream_stream_operator< C>::value
            >> : std::true_type
         {
            struct formatter
            {
               template< typename V>
               void operator () ( std::ostream& out, V&& value) const
               {
                  auto archive = serialize::line::writer( out);
                  archive << serialize::named::value::make( std::forward< V>( value), nullptr);
               }
            };
         };
      } // stream
   } // common
} // casual




