//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_TOOLS_INCLUDE_COMMON_H_
#define CASUAL_MIDDLEWARE_TOOLS_INCLUDE_COMMON_H_

#include "common/log.h"
#include "common/trace.h"

namespace casual
{
   namespace tools
   {
      extern common::log::Stream log;


      struct Trace : common::Trace
      {
         template< typename T>
         Trace( T&& value) : common::Trace( std::forward< T>( value), log) {}
      };


   } // tools


} // casual

#endif // CASUAL_MIDDLEWARE_TOOLS_INCLUDE_COMMON_H_
