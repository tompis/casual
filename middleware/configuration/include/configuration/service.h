//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

namespace casual
{
   namespace configuration
   {
      namespace service
      {

         namespace service
         {
            struct Default
            {
               serviceframework::optional< std::string> timeout;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( timeout);
               )

            };
         } // service


         struct Service : service::Default
         {
            std::string name;
            serviceframework::optional< std::vector< std::string>> routes;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               service::Default::serialize( archive);
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( routes);
            )

            friend bool operator == ( const Service& lhs, const Service& rhs);

            //! Will assign any unassigned values in lhs
            friend Service& operator += ( Service& lhs, const service::Default& rhs);
         };

      } // service
   } // configuration
} // casual


