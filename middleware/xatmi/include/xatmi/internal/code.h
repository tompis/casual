//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/exception/xatmi.h"

namespace casual
{
   namespace xatmi
   {
      namespace internal
      {
         void clear();

         namespace error
         {
            void clear();
            void set( common::code::xatmi value);
            common::code::xatmi get();

            template< typename T>
            int wrap( T&& task)
            {
               try
               {
                  error::clear();
                  task();

                  return 0;
               }
               catch( ...)
               {
                  error::set( casual::common::exception::xatmi::handle());
               }
               return -1;
            }
         } // error

         namespace user
         {
            namespace code
            {
               void set( long value);
               long get();
            } // code
         } // user
      } // internal
   } // xatmi
} // casual

