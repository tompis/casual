//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/namevaluepair.h"
#include "serviceframework/archive/archive.h"

#include "serviceframework/platform.h"
#include "serviceframework/pimpl.h"


#include <functional>


namespace casual
{
   namespace test
   {

      struct SimpleVO
      {

         SimpleVO() = default;
         SimpleVO( long value) : m_long{ value} {}

         SimpleVO( std::function<void(SimpleVO&)> foreign) { foreign( *this);}


         bool m_bool = false;
         long m_long = 123456;
         std::string m_string = "foo";
         short m_short = 256;
         long long m_longlong = std::numeric_limits< long long>::max();
         serviceframework::platform::time::point::type m_time = serviceframework::platform::time::point::type::max();

         serviceframework::optional< long> m_optional = 42;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            archive & CASUAL_MAKE_NVP( m_bool);
            archive & CASUAL_MAKE_NVP( m_long);
            archive & CASUAL_MAKE_NVP( m_string);
            archive & CASUAL_MAKE_NVP( m_short);
            archive & CASUAL_MAKE_NVP( m_longlong);
            archive & CASUAL_MAKE_NVP( m_time);
            archive & CASUAL_MAKE_NVP( m_optional);
         )

         static std::string yaml()
         {
            return R"(
value:
   m_bool: false
   m_long: 234
   m_string: bla bla bla bla
   m_short: 23
   m_longlong: 1234567890123456789
   m_time: 1234567890
   m_optional: 666
)";
         }

         static std::string json()
         {
            return R"({
"value":
   {
      "m_bool": false,
      "m_long": 234,
      "m_string": "bla bla bla bla",
      "m_short": 23,
      "m_longlong": 1234567890123456789,
      "m_time": 1234567890
   }
}
)";
         }

         static std::string xml()
         {
            return R"(<?xml version="1.0"?>
<value>
   <m_bool>false</m_bool>
   <m_long>234</m_long>
   <m_string>bla bla bla bla</m_string>
   <m_short>23</m_short>
   <m_longlong>1234567890123456789</m_longlong>
   <m_time>1234567890</m_time>
</value>
)";
         }

      };

      struct Composite
      {
         std::string m_string;
         std::vector< SimpleVO> m_values;
         std::tuple< int, std::string, SimpleVO> m_tuple;


         CASUAL_CONST_CORRECT_SERIALIZE
         (
            archive & CASUAL_MAKE_NVP( m_string);
            archive & CASUAL_MAKE_NVP( m_values);
            archive & CASUAL_MAKE_NVP( m_tuple);
         )
      };


      struct Binary : public SimpleVO
      {
         serviceframework::platform::binary::type m_binary;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            SimpleVO::serialize( archive);
            archive & CASUAL_MAKE_NVP( m_binary);
         )
      };


      namespace pimpl
      {
         struct Simple
         {

            // user defined
            Simple( long value);

            Simple();
            ~Simple();
            Simple( const Simple&);
            Simple& operator = ( const Simple&);
            Simple( Simple&&) noexcept;
            Simple& operator = ( Simple&&) noexcept;


            long getLong() const;
            const std::string& getString() const;
            std::string& getString();


            void setLong( long value);
            void setString( const std::string& value);



            void serialize( serviceframework::archive::Reader& reader);
            void serialize( serviceframework::archive::Writer& writer) const;


         private:
            class Implementation;
            serviceframework::Pimpl< Implementation> m_pimpl;
         };

      } // pimpl

   } // test
} // casual



