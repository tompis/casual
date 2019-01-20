//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"

#include "common/optional.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace customize
         {
            template< typename T> 
            struct Composite
            {
               template< typename A> 
               static void write( A& archive, const T& value)
               {
                  value.serialize( archive);
               }

               template< typename A> 
               static decltype( auto) read( A& archive, T& value)
               {
                  return value.serialize( archive);
               }
            };

            namespace composite
            {
               template< typename A, typename T>
               void write( A& archive, T&& value)
               {
                  customize::Composite< traits::remove_cvref_t< T>>::write( archive, std::forward< T>( value));
               }

               template< typename A, typename T>
               decltype( auto) read( A& archive, T& value)
               {
                  return customize::Composite< traits::remove_cvref_t< T>>::read( archive, value);
               }
            } // composite


            template< typename T> 
            struct Value
            {
               template< typename A, typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  archive.write( std::forward< V>( value), name);
               }

               template< typename A> 
               static decltype( auto) read( A& archive, T& value, const char* name)
               {
                  return archive.read( value, name);
               }
            };

            namespace value
            {

               template< typename A, typename T>
               void write( A& archive, T&& value, const char* name)
               {
                  customize::Value< traits::remove_cvref_t< T>>::write( archive, std::forward< T>( value), name);
               }

               namespace detail
               {
                  template< typename A, typename T>
                  decltype( auto) read( A& archive, T& value, const char* name)
                  {
                     return customize::Value< traits::remove_cvref_t< T>>::read( archive, value, name);
                  }
               } // detail

               template< typename A, typename T>
               auto read( A& archive, T& value, const char* name) -> std::enable_if_t<
                  std::is_same< 
                     decltype( detail::read( archive, value, name)), 
                     bool
                  >::value, bool>
               {
                  return detail::read( archive, value, name);
               }

               template< typename A, typename T>
               auto read( A& archive, T& value, const char* name) -> std::enable_if_t<
                  ! std::is_same< 
                     decltype( detail::read( archive, value, name)), 
                     bool
                  >::value, bool>
               {
                  detail::read( archive, value, name);
                  return true;
               }
            } // value

            //! Specialization for optional
            template< typename T>
            struct Value< common::optional< T>>
            {
               using value_type = common::optional< T>;

               template< typename A, typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  if( value)
                  {
                     value::write( archive, std::forward< V>( value).value(), name);
                  }
               }

               template< typename A> 
               static bool read( A& archive, value_type& value, const char* name)
               {
                  typename value_type::value_type contained;
                  
                  if( value::read( archive, contained, name))
                  {
                     value = std::move( contained);
                     return true;
                  }
                  return false;
               }
            };

            //! Specialization for time_type
            //! @{

            template< typename R, typename P>
            struct Value< std::chrono::duration< R, P>>
            {
               using value_type = std::chrono::duration< R, P>;

               template< typename A, typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  value::write( archive, std::chrono::duration_cast< common::platform::time::serialization::unit>( value).count(), name);
               }

               template< typename A> 
               static bool read( A& archive, value_type& value, const char* name)
               {
                  common::platform::time::serialization::unit::rep representation;

                  if( value::read( archive, representation, name))
                  {
                     value = std::chrono::duration_cast< value_type>( common::platform::time::serialization::unit( representation));
                     return true;
                  }
                  return false;
               }
            };

            template<>
            struct Value< common::platform::time::point::type>
            {
               template< typename A> 
               static void write( A& archive, common::platform::time::point::type value, const char* name)
               {
                  value::write( archive, value.time_since_epoch(), name);
               }

               template< typename A> 
               static bool read( A& archive, common::platform::time::point::type& value, const char* name)
               {
                  common::platform::time::serialization::unit duration;
                  if( value::read( archive, duration, name))
                  {
                     value = common::platform::time::point::type( std::chrono::duration_cast< common::platform::time::unit>( duration));
                     return true;
                  }
                  return false;
               }
            };
            //! @}

            namespace detail
            {
               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name) -> 
                  decltype( customize::composite::write( archive, std::forward< T>( value)))
               {
                  customize::composite::write( archive, std::forward< T>( value));
               }

               template< typename A, typename T>
               auto write( A& archive, T&& value, const char* name) -> 
                  decltype( customize::value::write( archive, std::forward< T>( value)))
               {
                  customize::value::write( archive, std::forward< T>( value), name);
               }

               template< typename A, typename T>
               auto read( A& archive, T& value, const char*) -> 
                  decltype( customize::composite::read( archive, value))
               {
                  return customize::composite::read( archive, value);
               }

               template< typename A, typename T>
               auto read( A& archive, T& value, const char* name) -> 
                  decltype( customize::value::read( archive, value))
               {
                  return customize::value::read( archive, value, name);
               }

            } // detail


            template< typename A, typename T>
            void write( A& archive, T&& value, const char* name)
            {
               detail::write( archive, std::forward< T>( value), name);
            }

            template< typename A, typename T>
            bool read( A& archive, T& value, const char* name)
            {
               return detail::read( archive, value, name);
            }


         } // customize
      } // serialize
   } // common
} // casual