//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/environment.h"

#include "common/serialize/macro.h"
#include "common/platform.h"

namespace casual
{
   namespace configuration
   {
      namespace server
      {
         namespace executable
         {
            struct Default
            {
               common::optional< common::platform::size::type> instances;
               common::optional< bool> restart;
               common::optional< std::vector< std::string>> memberships;

               common::optional< Environment> environment;


               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( instances);
                  CASUAL_SERIALIZE( restart);
                  CASUAL_SERIALIZE( memberships);
                  CASUAL_SERIALIZE( environment);
               )
            };

         } // executable

         struct Executable : executable::Default
         {
            Executable();
            Executable( std::function< void(Executable&)> foreign);

            std::string path;
            common::optional< std::string> alias;
            common::optional< std::string> note;


            common::optional< std::vector< std::string>> arguments;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( alias);
               CASUAL_SERIALIZE( arguments);

               executable::Default::serialize( archive);
               CASUAL_SERIALIZE( note);
            )

            friend bool operator == ( const Executable& lhs, const Executable& rhs);

            //!
            //! Will assign any unassigned values in lhs
            //!
            friend Executable& operator += ( Executable& lhs, const executable::Default& rhs);

            friend bool operator < ( const Executable& lhs, const Executable& rhs);

         };

         struct Server : public Executable
         {

            Server();
            Server( std::function< void(Server&)> foreign);

            common::optional< std::vector< std::string>> restrictions;
            common::optional< std::vector< std::string>> resources;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               Executable::serialize( archive);
               CASUAL_SERIALIZE( restrictions);
               CASUAL_SERIALIZE( resources);
            )

            friend bool operator == ( const Server& lhs, const Server& rhs);
         };


      } // server

   } // configuration


} // casual


