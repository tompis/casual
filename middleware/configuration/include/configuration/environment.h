//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"

#include <string>
#include <vector>


namespace casual
{
   namespace configuration
   {
      namespace environment
      {

         struct Variable
         {
            Variable();
            Variable( std::function< void(Variable&)> foreign);

            std::string key;
            std::string value;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( value);
            )

            friend bool operator == ( const Variable& lhs, const Variable& rhs);

         };

      } // environment

      struct Environment
      {
         Environment();
         Environment( std::function< void(Environment&)> foreign);

         std::vector< std::string> files;
         std::vector< environment::Variable> variables;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            CASUAL_SERIALIZE( files);
            CASUAL_SERIALIZE( variables);
         )

         friend bool operator == ( const Environment& lhs, const Environment& rhs);
      };

      namespace environment
      {

         configuration::Environment get( const std::string& file);

         std::vector< Variable> fetch( configuration::Environment environment);

         std::vector< std::string> transform( const std::vector< Variable>& variables);

      } // environment


   } // config
} // casual




