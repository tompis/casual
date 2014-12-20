//!
//! main.cpp
//!
//! Created on: Dec 19, 2014
//!     Author: Lazan
//!


#include "common/process.h"
#include "common/arguments.h"
#include "common/environment.h"



#include <string>
#include <vector>

#include <iostream>


namespace casual
{
   namespace admin
   {

      namespace dispatch
      {
         void execute( const std::string& command, const std::vector< std::string>& arguments)
         {
            static const auto path = common::environment::variable::get( "CASUAL_HOME") + "/internal/bin/";

            if( common::process::execute( path + command, arguments) != 0)
            {
               // TODO: throw?
            }
         }


         void domain( const std::vector< std::string>& arguments)
         {
            execute( "casual-broker-admin", arguments);
         }

         void queue( const std::vector< std::string>& arguments)
         {
            execute( "casual-queue-admin", arguments);
         }

      } // dispatch


      int main( int argc, char **argv)
      {
         common::Arguments arguments;

         arguments.add(
            common::argument::directive( { "domain" }, "domain related administration", &dispatch::domain),
            common::argument::directive( { "queue" }, "casual-queue related administration", &dispatch::queue));

         arguments.parse( argc, argv);

         return 0;
      }

   } // admin

} // casual


int main( int argc, char **argv)
{
   return casual::admin::main( argc, argv);
}


