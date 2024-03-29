//!
//! main.cpp
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
//!

#include "gateway/manager/manager.h"


#include "common/error.h"
#include "common/arguments.h"


namespace casual
{
   namespace gateway
   {
      namespace manager
      {

         int main( int argc, char **argv)
         {

            try
            {
               Settings settings;
               {
                  casual::common::Arguments parser{{

                  }};
                  parser.parse( argc, argv);
               }

               Manager manager{ std::move( settings)};
               manager.start();

            }
            catch( ...)
            {
               return casual::common::error::handler();
            }
            return 0;
         }

      } // manager
   } // gateway

} // casual


int main( int argc, char **argv)
{
   return casual::gateway::manager::main( argc, argv);
}

