//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/macro.h"
#include "common/platform.h"

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
               common::optional< std::string> timeout;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( timeout);
               )

            };
         } // service


         struct Service : service::Default
         {

            Service();
            Service( std::function< void(Service&)> foreign);

            std::string name;
            common::optional< std::vector< std::string>> routes;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               service::Default::serialize( archive);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( routes);
            )

            friend bool operator == ( const Service& lhs, const Service& rhs);

            //! Will assign any unassigned values in lhs
            friend Service& operator += ( Service& lhs, const service::Default& rhs);
         };

      } // service
   } // configuration
} // casual


