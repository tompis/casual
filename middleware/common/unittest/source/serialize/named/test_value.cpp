//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/serialize/macro.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         TEST( serialize_named_value, const_lvalue)
         {
            const int value = 42;
            auto named = CASUAL_MAKE_NVP( value);

            EXPECT_TRUE(( std::is_same< decltype( named), named::Value< const int, named::reference::lvalue>>::value ));
         }

         TEST( serialize_named_value, lvalue)
         {
            int value = 42;
            auto named = CASUAL_MAKE_NVP( value);

            EXPECT_TRUE(( std::is_same< decltype( named), named::Value< int, named::reference::lvalue>>::value ));
         }

         TEST( serialize_named_value, rvalue)
         {
            int value = 42;
            auto named = CASUAL_MAKE_NVP( std::move( value));

            EXPECT_TRUE(( std::is_same< decltype( named), named::Value< int, named::reference::rvalue>>::value ));
         }

      } // serialize
   } // common
} // casual