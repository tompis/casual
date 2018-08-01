//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/inbound/buffer.h"

#include "common/algorithm.h"

namespace casual
{
   namespace gateway
   {
      namespace inbound
      {

         Buffer::complete_type Buffer::get( const common::Uuid& correlation)
         {
            auto found = common::algorithm::find( m_messages, correlation);

            if( ! found)
               throw common::exception::system::invalid::Argument{ common::string::compose( "failed to find correlation: ", correlation)};
            
            auto result = std::move( *found);
            m_messages.erase( std::begin( found));
            m_size -= result.payload.size();

            return result;
         }

         std::ostream& operator << ( std::ostream& out, const Buffer& value)
         {
            return out << "{ size: " << value.m_size
               << ", messages: " << value.m_messages.size()
               << ", limit: { messages: " << value.m_limits.messages << ", size: " << value.m_limits.size << '}'
               << '}';
         }

      } // inbound
   } // gateway
} // casual