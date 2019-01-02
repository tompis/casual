//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "transaction/manager/state.h"


#include <string>

namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace environment
         {
            namespace log
            {
               std::string file();
            } // log
         } // environment

         struct Settings
         {
            Settings();

            std::string log;
         };

      } // manager

      class Manager
      {
      public:

         Manager( manager::Settings settings);
         ~Manager();

         void start();

         const manager::State& state() const;

      private:

         void handle_pending();

         manager::State m_state;
      };
   } // transaction
} // casual




