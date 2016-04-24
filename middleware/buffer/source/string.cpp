//!
//! string.cpp
//!
//! Created on: Dec 26, 2013
//!     Author: Lazan
//!

#include "buffer/string.h"

#include "common/buffer/pool.h"
#include "common/buffer/type.h"

#include "common/log.h"
#include "common/memory.h"

#include "common/internal/trace.h"


#include <cstring>

namespace casual
{
   namespace buffer
   {
      namespace string
      {

         namespace
         {

            struct trace : common::trace::basic::Scope
            {
               template<decltype(sizeof("")) size>
               explicit trace( const char (&information)[size]) : Scope( information, common::log::internal::buffer) {}
            };


            struct Buffer : common::buffer::Buffer
            {
               using common::buffer::Buffer::Buffer;

               typedef common::platform::binary_type::size_type size_type;
               typedef common::platform::raw_buffer_type data_type;

               size_type size() const noexcept
               {
                  return payload.memory.size();
               }

               size_type used() const noexcept
               {
                  return std::strlen( payload.memory.data()) + 1;
               }

               size_type transport( const size_type user_size) const
               {
                  //
                  // We could ignore user-size all together, but something is
                  // wrong if user supplies a greater size than allocated
                  //
                  if( user_size > size())
                  {
                     throw common::exception::xatmi::invalid::Argument{ "user supplied size is larger than allocated size"};
                  }

                  if( used() > size())
                  {
                     throw common::exception::xatmi::invalid::Argument{ "string is longer than allocated size"};
                  }

                  return used();
               }

               //
               //
               //
               size_type reserved() const
               {
                  return size();
               }



            };



            class Allocator : public common::buffer::pool::basic_pool<Buffer>
            {
            public:

               using types_type = common::buffer::pool::default_pool::types_type;

               static const types_type& types()
               {
                  // The types this pool can manage
                  static const types_type result{{ CASUAL_STRING, "" }};
                  return result;
               }

               common::platform::raw_buffer_type allocate( const common::buffer::Type& type, const common::platform::binary_size_type size)
               {
                  m_pool.emplace_back( type, size > 0 ? size : 1);

                  m_pool.back().payload.memory.front() = '\0';

                  return m_pool.back().payload.memory.data();
               }

               common::platform::raw_buffer_type reallocate( const common::platform::const_raw_buffer_type handle, const common::platform::binary_size_type size)
               {
                  const auto result = find( handle);

                  result->payload.memory.resize( size > 0 ? size : 1);

                  result->payload.memory.back() = '\0';

                  return result->payload.memory.data();
               }
            };

         } //

      } // string

   } // buffer


   //
   // Register and define the type that can be used to get the custom pool
   //
   template class common::buffer::pool::Registration< buffer::string::Allocator>;

   namespace buffer
   {
      namespace string
      {
         using pool_type = common::buffer::pool::Registration< Allocator>;

         namespace
         {

            Buffer* find( const char* const handle)
            {
               //const trace trace( "string::find");

               try
               {
                  auto& buffer = pool_type::pool.get( handle);

                  return &buffer;
               }
               catch( ...)
               {
                  //
                  // TODO: Perhaps have some dedicated string-logging ?
                  //
                  common::error::handler();
               }

               return nullptr;

            }
         }

      } // string

   } // buffer

} // casual

const char* casual_string_description( const int code)
{
   switch( code)
   {
      case CASUAL_STRING_SUCCESS:
         return "Success";
      case CASUAL_STRING_INVALID_HANDLE:
         return "Invalid handle";
      case CASUAL_STRING_INVALID_ARGUMENT:
         return "Invalid argument";
      case CASUAL_STRING_OUT_OF_BOUNDS:
         return "Out of bounds";
      case CASUAL_STRING_OUT_OF_MEMORY:
         return "Out of memory";
      case CASUAL_STRING_INTERNAL_FAILURE:
         return "Internal failure";
      default:
         return "Unknown code";
   }

}

int casual_string_explore_buffer( const char* const handle, long* const size, long* const used)
{
   const auto buffer = casual::buffer::string::find( handle);

   if( buffer)
   {

      if( size) *size = static_cast<long>(buffer->size());
      if( used) *used = static_cast<long>(buffer->used());

   }
   else
   {
      return CASUAL_STRING_INVALID_HANDLE;
   }

   return CASUAL_STRING_SUCCESS;
}

int casual_string_write( char** const handle, const char* const value)
{
   auto buffer = casual::buffer::string::find( *handle);

   if( buffer)
   {
      if( value)
      {
         const auto count = std::strlen( value) + 1;

         if( count > buffer->payload.memory.size())
         {
            try
            {
               buffer->payload.memory.resize( count);
            }
            catch( const std::bad_alloc &)
            {
               return CASUAL_STRING_OUT_OF_MEMORY;
            }
         }

         casual::common::memory::copy(
            casual::common::range::make( value, count),
            casual::common::range::make( buffer->payload.memory));

         *handle = buffer->payload.memory.data();

      }
      else
      {
         return CASUAL_STRING_INVALID_ARGUMENT;
      }

   }
   else
   {
      return CASUAL_STRING_INVALID_HANDLE;
   }

   return CASUAL_STRING_SUCCESS;

}

int casual_string_parse( const char* handle, const char** value)
{
   const auto buffer = casual::buffer::string::find( handle);

   if( buffer)
   {
      if( value)
      {
         *value = buffer->payload.memory.data();

         if( buffer->used() > buffer->size())
         {
            //
            // We need to report this
            //
            return CASUAL_STRING_OUT_OF_BOUNDS;
         }

      }
      else
      {
         return CASUAL_STRING_INVALID_ARGUMENT;
      }

   }
   else
   {
      return CASUAL_STRING_INVALID_HANDLE;
   }

   return CASUAL_STRING_SUCCESS;

}

