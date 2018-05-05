//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#ifndef CASUAL_QUEUE_MANAGER_ADMIN_CLI_H_
#define CASUAL_QUEUE_MANAGER_ADMIN_CLI_H_

#include "common/argument.h"
#include "common/pimpl.h"

namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {
            struct cli 
            {
               cli();
               ~cli();

               common::argument::Group options() &;

            private:
               struct Implementation;
               common::move::basic_pimpl< Implementation> m_implementation;
            };
         } // admin
      } // manager  
   } // queue
} // casual

#endif