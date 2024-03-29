//!
//! file.cpp
//!
//! Created on: Jul 1, 2014
//!     Author: Lazan
//!

#include "config/file.h"

#include "common/file.h"
#include "common/environment.h"

namespace casual
{
   namespace config
   {

      namespace directory
      {

         std::string domain()
         {
            return common::environment::directory::domain() + "/configuration";
         }

      } // directory

      namespace file
      {


         std::string find( const std::string& basename)
         {
            return find( common::directory::name::base( basename), common::file::name::base( basename));
         }

         std::string find( const std::string& path, const std::string& basename)
         {
            return common::file::find( path, std::regex( basename + ".(yaml|yml|json|jsn|xml)" ));
         }

         std::string domain()
         {
            return find( directory::domain(), "domain");
         }

         std::string queue()
         {
            return find( directory::domain(), "queue");
         }


      } // file
   } // config



} // casual
