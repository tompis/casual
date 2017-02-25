//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_COMMON_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_COMMON_H_

#include "common/log/stream.h"
#include "common/log/trace.h"

namespace casual
{

   namespace domain
   {
      extern common::log::Stream log;

      struct Trace : common::log::Trace
      {
         template< typename T>
         Trace( T&& value) : common::log::Trace( std::forward< T>( value), log) {}
      };


   } // domain

} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_COMMON_H_
