//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "common/platform.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace transaction
      {
         namespace resource
         {
            struct Default
            {
               common::optional< std::string> key;
               common::optional< common::platform::size::type> instances;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( key);
                  CASUAL_SERIALIZE( instances);
               )
            };
         } // resource

         struct Resource : resource::Default
         {
            std::string name;
            std::string note;

            common::optional< std::string> openinfo;
            common::optional< std::string> closeinfo;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               resource::Default::serialize( archive);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( openinfo);
               CASUAL_SERIALIZE( closeinfo);
            )

            friend bool operator == ( const Resource& lhs, const Resource& rhs);
            friend Resource& operator += ( Resource& lhs, const resource::Default& rhs);
         };

         namespace manager
         {
            struct Default
            {
               resource::Default resource;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( resource);
               )
            };

         } // manager

         struct Manager
         {
            Manager();

            manager::Default manager_default;

            std::string log;
            std::vector< Resource> resources;

            //! Complement with defaults and validates
            void finalize();

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE_NAME( manager_default, "default");
               CASUAL_SERIALIZE( log);
               CASUAL_SERIALIZE( resources);
            )
         };


      } // transaction
   } // configuration
} // casual


