//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/environment.h"

#include "common/serialize/macro.h"
#include "common/optional.h"

namespace casual
{
   namespace configuration
   {
      namespace executable
      {
         struct Default
         {
            common::platform::size::type instances = 1;
            std::vector< std::string> memberships;
            Environment environment;
            bool restart = false;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( CASUAL_MAKE_NVP( instances));
               CASUAL_SERIALIZE( CASUAL_MAKE_NVP( restart));
               CASUAL_SERIALIZE( CASUAL_MAKE_NVP( memberships));
               CASUAL_SERIALIZE( CASUAL_MAKE_NVP( environment));
            )
         };

      } // executable

      struct Executable
      {
         std::string path;
         common::optional< std::string> alias;
         common::optional< std::string> note;

         common::optional< std::vector< std::string>> arguments;

         common::optional< common::platform::size::type> instances;
         common::optional< std::vector< std::string>> memberships;
         common::optional< Environment> environment;
         common::optional< bool> restart;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            
            CASUAL_SERIALIZE( CASUAL_MAKE_NVP( path));
            CASUAL_SERIALIZE( CASUAL_MAKE_NVP( alias));
            CASUAL_SERIALIZE( CASUAL_MAKE_NVP( note));
            CASUAL_SERIALIZE( CASUAL_MAKE_NVP( arguments));

            CASUAL_SERIALIZE( CASUAL_MAKE_NVP( instances));
            CASUAL_SERIALIZE( CASUAL_MAKE_NVP( memberships));
            CASUAL_SERIALIZE( CASUAL_MAKE_NVP( environment));
            CASUAL_SERIALIZE( CASUAL_MAKE_NVP( restart));
            
         )

         Executable& operator += ( const executable::Default& value);

      };
   } // configuration
} // casual


