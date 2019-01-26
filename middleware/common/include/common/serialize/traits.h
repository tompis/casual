//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#pragma once


#include "common/traits.h"
#include "common/platform.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace traits
         {
            using namespace common::traits;

            template< typename T>
            struct is_pod : public traits::bool_constant< std::is_pod< T>::value && ! std::is_class< T>::value && ! std::is_enum< T>::value> {};

            template<>
            struct is_pod< std::string> : public traits::bool_constant< true> {};

            template<>
            struct is_pod< std::wstring> : public traits::bool_constant< true> {};

            template<>
            struct is_pod< std::vector< char> > : public traits::bool_constant< true> {};


            namespace need
            {
               namespace detail
               {
                  template< typename A>
                  using named = typename A::need_named;
               } // detail
               
               template< typename A>
               using named = detect::is_detected< detail::named, A>;

            } // need


            namespace has
            {
               namespace detail
               {
                  template< typename T, typename A>
                  using serialize = decltype( std::declval< T&>().serialize( std::declval< A&>()));
               } // detail

               template< typename T, typename A>
               using serialize = detect::is_detected< detail::serialize, T, A>;
            } // has

            namespace is
            {
               namespace value
               {                                 
                  template< typename T>
                  using serializable = traits::bool_constant<
                     std::is_arithmetic<T>::value ||
                     ( std::is_array<T>::value && sizeof( typename std::remove_all_extents<T>::type) == 1 ) ||
                     traits::container::is_array< T>::value ||
                     std::is_enum< T>::value>;
               } // value

               namespace network
               {
                  template< typename A>
                  struct normalizing : std::false_type {};
               } // network
            } // is
         } // traits
      } // serialize
   } // common
} // casual

