//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/traits.h"
#include "common/algorithm.h"
#include "common/functional.h"

#include "serialize/line.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace stream
      {
         template< typename T, typename Enable = void>
         struct has_formatter : std::false_type{};

         //! Specialization for iterables, to log ranges
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            traits::is::iterable< C>::value 
            && ! traits::is::string::like< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               template< typename R>
               void operator () ( std::ostream& out, R&& range) const
               {
                  out << '[';

                  algorithm::for_each_interleave( 
                     range,
                     [&out]( auto& v){ out << v;},
                     [&out](){ out << ", ";}
                  );

                  out << ']';
               }
            };
         };

         //! Specialization for enum
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            std::is_enum< C>::value && ! std::is_error_code_enum< C>::value && ! std::is_error_condition_enum< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               void operator () ( std::ostream& out, C value) const
               {
                  out << cast::underlying( value);
               }
            };
         };

         //! Specialization for error code
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            std::is_error_code_enum< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               void operator () ( std::ostream& out, C value) const
               {
                  auto code = std::error_code( value);
                  out << '[' << code << ' ' << code.message() << ']';
               }
            };
         };

         //! Specialization for error condition
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            std::is_error_condition_enum< C>::value>>
            : std::true_type
         {
            struct formatter
            {
               void operator () ( std::ostream& out, C value) const
               {
                  auto condition = std::error_condition( value);
                  out << condition.category().name() << ':' << condition.value() << " - " << condition.message();
               }
            };
         };

         //! Specialization for stuff that serialize::line::Writer can take care of
         template< typename C> 
         struct has_formatter< C, std::enable_if_t< 
            traits::is::binary::like< C>::value 
            || traits::is::tuple< C>::value 
            || serialize::traits::has::serialize< C, serialize::line::Writer>::value
            || serialize::traits::has::forward::serialize< C, serialize::line::Writer>::value
            || serialize::traits::is::named::value< C>::value
            >> : std::true_type
         {
            struct formatter
            {
               template< typename V>
               void operator () ( std::ostream& out, V&& value) const
               {
                  serialize::line::Writer{ out} << std::forward< V>( value);
               }
            };
         };

         namespace detail
         {
            template< typename S>
            void part( S& stream) {}

            template< typename S, typename T>
            void part( S& stream, T&& value)
            {
               stream << std::forward< T>( value);
            }

            template< typename S, typename T, typename... Ts>
            void part( S& stream, T&& value, Ts&&... ts) 
            {
               detail::part( stream, std::forward< T>( value));
               detail::part( stream, std::forward< Ts>( ts)...);
            }
         } // detail

         template< typename S, typename... Ts>
         S& write( S& stream, Ts&&... ts)
         {
            detail::part( stream, std::forward< Ts>( ts)...);
            return stream;
         }
/*!

         namespace detail
         {
            namespace ostream
            {
               template< typename T> 
               auto indirection( std::ostream& out, T&& value, traits::priority::tag< 0>) 
                  -> decltype( std::declval< serialize::line::Writer&>() << std::declval< T&>(), void())
               {
                  casual::common::serialize::line::Writer archive{ out};
                  archive << std::forward< T>( value);
               }

               template< typename T> 
               auto indirection( std::ostream& out, T&& value, traits::priority::tag< 1>) 
                  -> decltype( typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, std::forward< T>( value)))
               {
                  typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, std::forward< T>( value));
               } 

            } // ostream
         } // detail
       */   
      } // stream
   } // common
} // casual

namespace std
{
   // extended stream operator for std... as I understand it, but I find it 
   // hard to see what damage it could do, since it is restricted to the 
   // customization point 'casual::common::log::has_formatter', so we roll with it...
   
   template< typename T> 
   enable_if_t< 
      casual::common::stream::has_formatter< casual::common::traits::remove_cvref_t< T>>::value
      && ! casual::common::traits::has::ostream_stream_operator< casual::common::traits::remove_cvref_t< T>>::value
      , 
      ostream&>
   operator << ( ostream& out, T&& value)
   {
      using namespace casual::common;
      typename stream::has_formatter< traits::remove_cvref_t< T>>::formatter{}( out, std::forward< T>( value));
      return out;
   }
} // std