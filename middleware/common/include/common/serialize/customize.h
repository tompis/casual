//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/traits.h"

#include "common/exception/casual.h"
#include "common/optional.h"
#include "common/string.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace customize
         {
            template< typename T, typename A, typename Enable = void> 
            struct Value
            {
               template< typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  archive.write( std::forward< V>( value), name);
               }

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
                  customize::Value< traits::remove_cvref_t< T>, A>::write( archive, std::forward< T>( value), name);
               }

               namespace detail
               {
                  template< typename A, typename T>
                  decltype( auto) read( A& archive, T& value, const char* name)
                  {
                     return customize::Value< traits::remove_cvref_t< T>, A>::read( archive, value, name);
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


            //! Specialization for serializable
            template< typename T, typename A>
            struct Value< T, A, std::enable_if_t< traits::has::serialize< T, A>::value>>
            {
               template< typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  archive.composite_start( name);
                  value.serialize( archive);
                  archive.composite_end( name);
               }

               static bool read( A& archive, T& value, const char* name)
               {
                  if( archive.composite_start( name))
                  {
                     value.serialize( archive);
                     archive.composite_end( name);
                     return true;
                  }
                  return false;
               }
            };

            //! Specialization for enum
            template< typename T, typename A>
            struct Value< T, A, std::enable_if_t< std::is_enum< T>::value>>
            {
               template< typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  value::write( archive, static_cast< std::underlying_type_t< T>>( value), name);
               }

               static bool read( A& archive, T& value, const char* name)
               {
                  std::underlying_type_t< T> underlying; 
                  if( value::read( archive, underlying, name))
                  {
                     value = static_cast< T>( underlying);
                     return true;
                  }
                  return false;
               }
            };

            namespace detail
            {
               namespace tuple
               {
                  template< platform::size::type index>
                  struct Write
                  {
                     template< typename A, typename T>
                     static void serialize( A& archive, const T& value)
                     {
                        value::write( archive, std::get< std::tuple_size< T>::value - index>( value), nullptr);
                        Write< index - 1>::serialize( archive, value);
                     }
                  };

                  template<>
                  struct Write< 0>
                  {
                     template< typename A, typename T>
                     static void serialize( A&, const T&) {}
                  };

                  template< platform::size::type index>
                  struct Read
                  {
                     template< typename A, typename T>
                     static void serialize( A& archive, T& value)
                     {
                        value::read( archive, std::get< std::tuple_size< T>::value - index>( value), nullptr);
                        Read< index - 1>::serialize( archive, value);
                     }
                  };

                  template<>
                  struct Read< 0>
                  {
                     template< typename A, typename T>
                     static void serialize( A&, T&) {}
                  };
               } // tuple
            } // detail

            //! Specialization for tuple
            template< typename T, typename A>
            struct Value< T, A, std::enable_if_t< common::traits::is::tuple< T>::value>>
            {
               template< typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  
                  archive.container_start( std::tuple_size< T>::value, name);
                  detail::tuple::Write< std::tuple_size< T>::value>::serialize( archive, value);
                  archive.container_end( name);
               }

               static bool read( A& archive, T& value, const char* name)
               {
                  constexpr auto expected_size = std::tuple_size< T>::value;
                  const auto context = archive.container_start( expected_size, name);

                  if( std::get< 1>( context))
                  {
                     auto size = std::get< 0>( context);

                     if( expected_size != size)
                     {
                        throw exception::casual::invalid::Node{ string::compose( "unexpected size: ", size, " - expected: ", expected_size)};
                     }
                     detail::tuple::Read< std::tuple_size< T>::value>::serialize( archive, value);

                     archive.container_end( name);
                     return true;
                  }
                  return false;
               }
            };

            namespace detail
            {
               namespace is
               {
                  template< typename T> 
                  using container = traits::bool_constant< 
                     common::traits::is::container::like< T>::value
                     && ! serialize::traits::is_pod< T>::value
                  >; 
               } // is
     
               namespace container
               {
                  template< typename T>
                  struct value { using type = T;};

                  template< typename K, typename V>
                  struct value< std::pair< K, V>> { using type = std::pair< traits::remove_cvref_t< K>, V>;};

                  template< typename T> 
                  using value_t = typename value< traits::remove_cvref_t< T>>::type;

                  template< typename A, typename C>
                  auto read( A& archive, C& container, const char* name) ->
                     std::enable_if_t< common::traits::is::associative::like< traits::remove_cvref_t< C>>::value, bool>
                  {
                     auto properties = archive.container_start( 0, name);

                     if( std::get< 1>( properties))
                     {
                        auto count = std::get< 0>( properties);

                        while( count-- > 0)
                        {
                           // we need to get rid of const key (if pair), so we can serialize
                           container::value_t< typename traits::remove_cvref_t< C>::value_type> element;
                           customize::value::read( archive, element, nullptr);

                           container.insert( std::move( element));
                        }

                        archive.container_end( name);
                        return true;
                     }
                     return false;
                  }

                  template< typename A, typename C>
                  auto read( A& archive, C& container, const char* name) ->
                     std::enable_if_t< common::traits::is::sequence::like< traits::remove_cvref_t< C>>::value, bool>
                  {
                     auto properties = archive.container_start( 0, name);

                     if( std::get< 1>( properties))
                     {
                        container.resize( std::get< 0>( properties));

                        for( auto& element : container)
                           customize::value::read( archive, element, nullptr);

                        archive.container_end( name);
                        return true;
                     }
                     return false;
                  }

               } // container
               
            } // detail


            //! Specialization for containers
            template< typename T, typename A>
            struct Value< T, A, std::enable_if_t< detail::is::container< T>::value>>
            {
               template< typename C> 
               static void write( A& archive, C&& container, const char* name)
               {
                  archive.container_start( container.size(), name);

                  for( auto& element : container)
                  {
                     value::write( archive, element, nullptr);
                  }
                  archive.container_end( name);
               }

               static bool read( A& archive, T& value, const char* name)
               {
                  return detail::container::read( archive, value, name);
               }
            };

            //! Specialization for optional-like
            template< typename T, typename A>
            struct Value< T, A, std::enable_if_t< common::traits::is::optional_like< T>::value>>
            {
               template< typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  if( value)
                  {
                     archive.container_start( 1, name);
                     value::write( archive, value.value(), nullptr);
                  }
                  else 
                  {
                     archive.container_start( 0, name);
                  }
                  archive.container_end( name);
               }

               template< typename V>
               static bool read( A& archive, V& value, const char* name)
               {
                  auto properties = archive.container_start( 0, name);

                  if( std::get< 1>( properties))
                  {
                     std::decay_t< decltype( value.value())> contained;

                     customize::value::read( archive, contained, nullptr);
                     value = std::move( contained);

                     archive.container_end( name);
                     return true;
                  }
                  return false;
               }
            };

            //! Specialization for time
            //! @{

            template< typename R, typename P, typename A>
            struct Value< std::chrono::duration< R, P>, A>
            {
               using value_type = std::chrono::duration< R, P>;

               template< typename V> 
               static void write( A& archive, V&& value, const char* name)
               {
                  value::write( archive, std::chrono::duration_cast< common::platform::time::serialization::unit>( value).count(), name);
               }

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

            template< typename A>
            struct Value< common::platform::time::point::type, A>
            {
               static void write( A& archive, common::platform::time::point::type value, const char* name)
               {
                  value::write( archive, value.time_since_epoch(), name);
               }

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

         } // customize
      } // serialize
   } // common
} // casual