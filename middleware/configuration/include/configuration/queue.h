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
      namespace queue
      {
         namespace queue
         {
            struct Default
            {
               common::optional< common::platform::size::type> retries;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( retries);
               )
            };
         } // queue

         struct Queue : queue::Default
         {
            Queue();
            Queue( std::function<void( Queue&)> foreign);

            std::string name;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               queue::Default::serialize( archive);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
            )

            friend bool operator < ( const Queue& lhs, const Queue& rhs);
            friend bool operator == ( const Queue& lhs, const Queue& rhs);
         };

         struct Group
         {
            Group();
            Group( std::function<void( Group&)> foreign);

            std::string name;
            common::optional< std::string> queuebase;
            std::string note;
            std::vector< Queue> queues;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( queuebase);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( queues);
            )

            friend bool operator < ( const Group& lhs, const Group& rhs);
            friend bool operator == ( const Group& lhs, const Group& rhs);

         };
         namespace manager
         {
            struct Default
            {
               Default();

               queue::Default queue;
               std::string directory;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( queue);
                  CASUAL_SERIALIZE( directory);
               )
            };
         } // manager

         struct Manager
         {
            Manager();

            manager::Default manager_default;
            std::vector< Group> groups;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE_NAME( manager_default, "default");
               CASUAL_SERIALIZE( groups);
            )

            //! Complement with defaults and validates
            void finalize();

            Manager& operator += ( const Manager& rhs);
            Manager& operator += ( Manager&& rhs);
            friend Manager operator + ( const Manager& lhs, const Manager& rhs);
         };


         namespace unittest
         {
            void validate( const Manager& manager);
            void default_values( Manager& manager);
         } // unittest
      } // queue
   } // config
} // casual


