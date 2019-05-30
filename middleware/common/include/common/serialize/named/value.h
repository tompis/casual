//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"
#include "common/stream.h"

#include <utility>
#include <type_traits>
#include <tuple>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace named
         {
            template< typename T, typename R>
            class Value;


            //! Holds non const lvalue types
            template <typename T>
            class Value< T, std::true_type> : public std::tuple< const char*, T*>
            {
            public:

               explicit Value (const char* name, T& value)
               :  std::tuple< const char*, T*>( name, &value) {}

               const char* name() const
               {
                  return std::get< 0>( *this);
               }

               T& value() const
               {
                  return  *( std::get< 1>( *this));
               }
            };

            //! Holds const lvalue types
            template <typename T>
            class Value< const T, std::true_type> : public std::tuple< const char*, const T*>
            {
            public:

               explicit Value (const char* name, const T& value)
               :  std::tuple< const char*, const T*>( name, &value) {}

               const char* name() const
               {
                  return std::get< 0>( *this);
               }

               const T& value() const
               {
                  return *( std::get< 1>( *this));
               }
            };


            //! Holds rvalue types
            template <typename T>
            class Value< T, std::false_type> : public std::tuple< const char*, T>
            {
            public:

               explicit Value (const char* name, T&& value)
               :  std::tuple< const char*, T>( name, std::move( value)) {}

               const char* name() const
               {
                  return std::get< 0>( *this);
               }

               const T& value() const
               {
                  return std::get< 1>( *this);
               }
            };

            namespace value
            {
               namespace internal
               {
                  template< typename T>
                  using value_traits_t = Value< typename std::remove_reference< T>::type, typename std::is_lvalue_reference<T>::type>;
               }

               template< typename T>
               auto make( T&& value, const char* name)
               {
                  return internal::value_traits_t< T>( name, std::forward< T>( value));
               }
            } // value

         } // named
      } // serialize

      namespace stream
      {
         //! Specialization for serial
         template< typename T, typename R>
         struct has_formatter< serialize::named::Value< T, R>, void>
            : std::true_type
         {
            struct formatter
            {
               template< typename C>
               void operator () ( std::ostream& out, C&& value) const
               {
                  common::stream::write( out, value.name(), ": ", value.value());
               }
            };
         };
      } // stream
   } // common
} // casual
