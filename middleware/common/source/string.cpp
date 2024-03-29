//!
//! string.cpp
//!
//! Created on: Apr 6, 2014
//!     Author: Lazan
//!

#include "common/string.h"
#include "common/algorithm.h"

#include <memory>
#include <numeric>

#include <cstdlib>
#include <cctype>

#include <cxxabi.h>


namespace casual
{
   namespace common
   {
      namespace type
      {
         namespace internal
         {
            std::string name( const std::type_info& type)
            {
               const auto result = abi::__cxa_demangle( type.name(), nullptr, nullptr, nullptr);
               return std::unique_ptr<char, decltype(std::free)*> { result, &std::free }.get();
            }
         } // internal
      } // type


      namespace string
      {

         std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter)
         {
            std::vector< std::string> result;

            auto current = std::begin( line);

            while( current != std::end( line))
            {
               auto found = std::find( current, std::end( line), delimiter);

               result.emplace_back( current, found);

               current = found;

               if( current != std::end( line))
               {
                  ++current;
               }
            }
            return result;
         }

         namespace adjacent
         {

            std::vector< std::string> split( const std::string& line, typename std::string::value_type delimiter)
            {
               typedef typename std::string::value_type value_type;

               std::vector< std::string> result;

               auto current = std::begin( line);

               while( current != std::end( line))
               {
                  auto found = std::find( current, std::end( line), delimiter);

                  if( current != found)
                  {
                     result.emplace_back( current, found);
                  }

                  current = std::find_if( found, std::end( line), [=]( value_type value) { return value != delimiter;});
               }
               return result;
            }

         } // adjacent

         std::string join( const std::vector< std::string>& strings)
         {
            return std::accumulate( strings.begin(), strings.end(), std::string());
         }


         std::string join( const std::vector< std::string>& strings, const std::string& delimiter)
         {
            if( strings.empty())
            {
               return std::string();
            }

            //
            // This will give a delimiter between empty strings (as well)
            //

            return std::accumulate( strings.begin() + 1, strings.end(), strings.front(),
               [&]( const std::string& f, const std::string& s){ return f + delimiter + s;});
         }


         std::string trim( const std::string& value)
         {
            const auto ws = [] ( const std::string::value_type character)
            { return std::isspace( character, std::locale::classic()); };

            const auto first = std::find_if_not( std::begin( value), std::end( value), ws);
            const auto last = std::find_if_not( value.rbegin(), value.rend(), ws);

            return first < last.base() ? std::string( first, last.base()) : std::string();
         }


         std::string lower( std::string value)
         {
            const auto lower = [] ( const std::string::value_type character)
            { return std::tolower( character, std::locale::classic());};

            std::transform( value.begin(), value.end(), value.begin(), lower);

            return value;
         }

         std::string upper( std::string value)
         {
            const auto upper = [] ( const std::string::value_type character)
            { return std::toupper( character, std::locale::classic());};

            std::transform( value.begin(), value.end(), value.begin(), upper);

            return value;
         }


         bool integer( const std::string& value)
         {
            if( value.empty())
            {
               return false;
            }

            return range::includes( "0123456789", value);
         }

      } // string
   } // common
} // casual
