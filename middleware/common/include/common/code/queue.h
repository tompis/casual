//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/code/xatmi.h"
#include "common/cast.h"

namespace casual
{
   namespace common
   {
      namespace code
      {
         enum class queue : int
         {
            ok = cast::underlying( xatmi::ok),
            no_message = cast::underlying( xatmi::no_message),
            argument = cast::underlying( xatmi::argument),
            limit = cast::underlying( xatmi::limit),
            no_entry = cast::underlying( xatmi::no_entry),
            os = cast::underlying( xatmi::os),
            protocol = cast::underlying( xatmi::protocol),
            service_error = cast::underlying( xatmi::service_error),
            service_fail = cast::underlying( xatmi::service_fail),
            system = cast::underlying( xatmi::system),
            timeout = cast::underlying( xatmi::timeout),
            transaction = cast::underlying( xatmi::transaction),
            signal = cast::underlying( xatmi::signal),
         };

         std::error_code make_error_code( queue code);

         common::log::Stream& stream( code::queue code);

         const char* message( queue code) noexcept;

      } // code
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::queue> : true_type {};
}