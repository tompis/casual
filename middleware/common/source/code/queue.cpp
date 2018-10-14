//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/code/queue.h"
#include "common/log/category.h"
#include "common/log.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace local
         {
            namespace
            {
               const char* message( queue code)
               {
                  switch( code)
                  {
                     case queue::ok: return "";
                     case queue::no_message: return "TPEBLOCK: no message ready to consume";
                     case queue::argument: return "TPEINVAL: invalid arguments was given";
                     case queue::limit: return "TPELIMIT: system limit was reached";
                     case queue::no_entry: return "TPENOENT: failed to lookup service";
                     case queue::os: return "TPEOS: operating system level error detected";
                     case queue::protocol: return "TPEPROTO: routine was called in an improper context";
                     case queue::service_error: return "TPESVCERR: system level service error";
                     case queue::service_fail: return "TPESVCFAIL: application level service error";
                     case queue::system: return "TPESYSTEM: system level error detected";
                     case queue::timeout: return "TPETIME: timeout reach during execution";
                     case queue::transaction: return "TPETRAN: transaction error detected";
                     case queue::signal: return "TPGOTSIG: signal was caught during blocking execution";
                     default: return "unknown";
                  }
               }

               struct Category : std::error_category
               {
                  const char* name() const noexcept override
                  {
                     return "queue";
                  }

                  std::string message( int code) const override
                  {
                     return local::message( static_cast< code::queue>( code));
                  }
               };

               const Category category{};

            } // <unnamed>
         } // local

         std::error_code make_error_code( queue code)
         {
            return std::error_code( static_cast< int>( code), local::category);
         }

         const char* message( queue code) noexcept
         {
            return local::message( code);
         }

      } // code
   } // common
} // casual