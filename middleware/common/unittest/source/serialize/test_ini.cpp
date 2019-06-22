//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>


#include "common/serialize/ini.h"

#include <sstream>
#include <locale>

#include "../../include/test_vo.h"



namespace casual
{
   namespace common
   {
      namespace
      {
         namespace local
         {
            template<typename T>
            void value_to_string( T&& value, std::string& string)
            {
               auto writer = serialize::ini::writer( string);
               writer << CASUAL_MAKE_NVP( value);
            }

            template<typename T>
            void string_to_value( const std::string& string, T& value)
            {
               auto reader = serialize::ini::strict::reader( string);
               reader >> CASUAL_MAKE_NVP( value);
            }

         } // local
      } //



      TEST( serviceframework_ini_archive, write_read_string_with_new_line)
      {
         std::string ini;
         std::string source = "foo\nbar";
         local::value_to_string( source, ini);
         std::string target;
         local::string_to_value( ini, target);

         EXPECT_TRUE( source == target);
      }

      TEST( serviceframework_ini_archive, write_read_boolean)
      {
         std::string ini;
         local::value_to_string( true, ini);
         bool target = false;
         local::string_to_value( ini, target);
         EXPECT_TRUE( target == true);
      }

      TEST( serviceframework_ini_archive, write_read_decimal)
      {
         std::string ini;
         float source = 3.14;
         local::value_to_string( source, ini);
         float target = 0.0;
         local::string_to_value( ini, target);
         EXPECT_TRUE( source == target);
      }

      TEST( serviceframework_ini_archive, write_read_container)
      {
         std::string ini;
         std::vector<long> source{ 1, 3, 5, 7 };
         local::value_to_string( source, ini);
         std::vector<long> target;
         local::string_to_value( ini, target);
         EXPECT_TRUE( source == target);
      }

      namespace
      {
         struct SomeVO
         {
            struct FirstVO
            {
               long integer;
               std::string string;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( integer);
                  CASUAL_SERIALIZE( string);
               )
            };


            struct OtherVO
            {
               struct InnerVO
               {
                  long long huge;

                  CASUAL_CONST_CORRECT_SERIALIZE
                  (
                     CASUAL_SERIALIZE( huge);
                  )

               };

               InnerVO first_inner;
               InnerVO other_inner;


               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( first_inner);
                  CASUAL_SERIALIZE( other_inner);
               )
            };

            bool boolean;
            FirstVO first;
            short tiny;
            std::vector<OtherVO> others;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( boolean);
               CASUAL_SERIALIZE( first);
               CASUAL_SERIALIZE( tiny);
               CASUAL_SERIALIZE( others);
            )

         };

         TEST( serviceframework_ini_archive, write_read_serializable)
         {
            std::string ini;
            SomeVO source;
            source.boolean = true;
            source.first.integer = 654321;
            source.first.string = "foo";
            source.tiny = 123;
            {
               SomeVO::OtherVO other;
               other.first_inner.huge = 123456789;
               other.other_inner.huge = 987654321;
               source.others.push_back( other);
            }
            {
               SomeVO::OtherVO other;
               other.first_inner.huge = 13579;
               other.other_inner.huge = 97531;
               source.others.push_back( other);
            }


            local::value_to_string( source, ini);
            SomeVO target;
            local::string_to_value( ini, target);

            EXPECT_TRUE( source.boolean == target.boolean);
            EXPECT_TRUE( source.first.integer == target.first.integer);
            EXPECT_TRUE( source.first.string == target.first.string);
            EXPECT_TRUE( source.tiny == target.tiny);
            ASSERT_TRUE( target.others.size() == 2);
            EXPECT_TRUE( source.others.at( 0).first_inner.huge == target.others.at( 0).first_inner.huge);
            EXPECT_TRUE( source.others.at( 0).other_inner.huge == target.others.at( 0).other_inner.huge);
            EXPECT_TRUE( source.others.at( 1).first_inner.huge == target.others.at( 1).first_inner.huge);
            EXPECT_TRUE( source.others.at( 1).other_inner.huge == target.others.at( 1).other_inner.huge);
         }


         TEST( serviceframework_ini_archive, test_control_characters)
         {
            std::vector< char> result;
            for( short idx = 0; idx < 255; ++idx)
            {
               if( std::iscntrl( static_cast<char>( idx), std::locale{}))
               {
                  result.push_back( idx);
               }
            }

            auto expected = std::vector< char>{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,127};

            EXPECT_TRUE( result == expected);
         }
      }

   /*
      struct TinyVO
      {
         bool boolean = true;
         long integer = 123;

         template<typename A>
         void serialize( A& archive)
         {
            CASUAL_SERIALIZE( boolean);
            CASUAL_SERIALIZE( integer);
         }
      };

      TEST( serviceframework_ini_archive, nested_stuff)
      {
         std::string ini;
         //std::vector<std::vector<std::vector<long>>> source{ {{1, 3, 5}, {2, 4, 6, 8}}, {{4, 3, 2}} };
         //std::map<long,TinyVO> source{ { 1, TinyVO()}, { 2, TinyVO()} };
         std::map<long,TinyVO> source{ { 1, TinyVO()}, { 2, TinyVO()}, { 3, TinyVO()}};
         //std::map<long,std::string> source{ { 1, "foo"}};
         local::value_to_string( source, ini);
         std::cout << ini << '\n';
      }
   */
   } // common
} // casual