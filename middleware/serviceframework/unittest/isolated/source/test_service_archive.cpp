//!
//! test_service_model.cpp
//!
//! Created on: Mar 15, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "sf/archive/service.h"
#include "sf/service/interface.h"
#include "sf/log.h"

#include <map>
#include <vector>

namespace casual
{
   namespace sf
   {
      TEST( casual_sf_service_model, instancesate)
      {
         service::Model model;

         service::Model::Type type;
         type.attribues.push_back( type);
      }


      TEST( casual_sf_service_archive, basic_serialization)
      {

         service::Model model;

         archive::service::Wrapper writer{ model.arguments.input};

         std::string some_string;
         long some_long = 0;

         writer << CASUAL_MAKE_NVP( some_string);
         writer << CASUAL_MAKE_NVP( some_long);

         ASSERT_TRUE( model.arguments.input.size() == 2);
         EXPECT_TRUE( model.arguments.input.at( 0).role == "some_string");
         EXPECT_TRUE( model.arguments.input.at( 0).type == service::Model::Type::type_string);
         EXPECT_TRUE( model.arguments.input.at( 1).role == "some_long");
         EXPECT_TRUE( model.arguments.input.at( 1).type == service::Model::Type::type_integer);
      }

      TEST( casual_sf_service_archive, container_serialization)
      {

         service::Model model;

         archive::service::Wrapper writer{ model.arguments.input};

         std::vector< std::string> some_strings;
         std::vector< long> some_longs;

         writer & CASUAL_MAKE_NVP( some_strings);
         writer & CASUAL_MAKE_NVP( some_longs);

         ASSERT_TRUE( model.arguments.input.size() == 2);
         EXPECT_TRUE( model.arguments.input.at( 0).role == "some_strings");
         EXPECT_TRUE( model.arguments.input.at( 0).type == service::Model::Type::type_container);
         ASSERT_TRUE( model.arguments.input.at( 0).attribues.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.front().type == service::Model::Type::type_string);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.front().role.empty());
         EXPECT_TRUE( model.arguments.input.at( 1).role == "some_longs");
         EXPECT_TRUE( model.arguments.input.at( 1).type == service::Model::Type::type_container);
         ASSERT_TRUE( model.arguments.input.at( 1).attribues.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 1).attribues.front().type == service::Model::Type::type_integer);
         EXPECT_TRUE( model.arguments.input.at( 1).attribues.front().role.empty());
      }

      namespace local
      {
         namespace
         {
            struct Composite
            {
               std::string some_string;
               platform::binary_type some_binary;
               long some_long = 0;
               bool some_bool = true;
               double some_double = 0.0;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( some_string);
                  archive & CASUAL_MAKE_NVP( some_binary);
                  archive & CASUAL_MAKE_NVP( some_long);
                  archive & CASUAL_MAKE_NVP( some_bool);
                  archive & CASUAL_MAKE_NVP( some_double);
               })



            };

            bool operator < ( const Composite& lhs, const Composite& rhs) { return true;}

         } // <unnamed>
      } // local


      TEST( casual_sf_service_archive, composite_serialization)
      {

         service::Model model;
         archive::service::Wrapper writer{ model.arguments.input};

         local::Composite some_composite;

         writer << CASUAL_MAKE_NVP( some_composite);

         ASSERT_TRUE( model.arguments.input.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 0).role == "some_composite");
         EXPECT_TRUE( model.arguments.input.at( 0).type == service::Model::Type::type_composite);
         ASSERT_TRUE( model.arguments.input.at( 0).attribues.size() == 5);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 0).role == "some_string");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 0).type == service::Model::Type::type_string);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 1).role == "some_binary");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 1).type == service::Model::Type::type_binary);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 2).role == "some_long");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 2).type == service::Model::Type::type_integer);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 3).role == "some_bool");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 3).type == service::Model::Type::type_boolean);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 4).role == "some_double");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 4).type == service::Model::Type::type_float);
      }

      namespace local
      {
         namespace
         {
            struct NestedComposite
            {
               std::vector< long> some_longs;
               std::vector< local::Composite> some_composites;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  archive & CASUAL_MAKE_NVP( some_longs);
                  archive & CASUAL_MAKE_NVP( some_composites);
               })
            };

         } // <unnamed>
      } // local

      TEST( casual_sf_service_archive, complex_serialization)
      {

         service::Model model;
         archive::service::Wrapper writer{ model.arguments.input};

         std::map< local::Composite, local::NestedComposite> complex;
         writer << CASUAL_MAKE_NVP( complex);

         ASSERT_TRUE( model.arguments.input.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 0).role == "complex");
         EXPECT_TRUE( model.arguments.input.at( 0).type == service::Model::Type::type_container);
         ASSERT_TRUE( model.arguments.input.at( 0).attribues.size() == 1);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 0).role.empty());
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 0).type == service::Model::Type::type_container);
         ASSERT_TRUE( model.arguments.input.at( 0).attribues.at( 0).attribues.size() == 2) << CASUAL_MAKE_NVP( model);


         /*
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 0).role == "some_string");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 0).type == service::Model::Type::type_string);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 1).role == "some_binary");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 1).type == service::Model::Type::type_binary);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 2).role == "some_long");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 2).type == service::Model::Type::type_integer);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 3).role == "some_bool");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 3).type == service::Model::Type::type_boolean);
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 4).role == "some_double");
         EXPECT_TRUE( model.arguments.input.at( 0).attribues.at( 4).type == service::Model::Type::type_float);
         */
      }

      /*
      namespace local
      {
         namespace
         {
            TPSVCINFO prepareAPI( buffer::Type type)
            {
               TPSVCINFO info;

               sf::buffer::Buffer buffer( std::move( type), 64);

               auto raw = buffer.release();

               info.data = raw.buffer;
               info.len = raw.size;

               return info;
            }
         } // <unnamed>
      } // local
      */

      TEST( casual_sf_service_archive, service_json_factory)
      {
         //auto information = local::prepareAPI( buffer::type::api::yaml());

         //service::IO service_io{ sf::service::Factory::instance().create( &information)};

      }

   } // sf

} // casual
