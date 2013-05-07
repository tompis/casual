//!
//! trace.h
//!
//! Created on: Nov 25, 2012
//!     Author: Lazan
//!

#ifndef TRACE_H_
#define TRACE_H_

#include "common/logger.h"

#include <string>

namespace casual
{
   namespace common
   {
      class Trace
      {
      public:
         template< typename T>
         Trace( T&& information) : m_information( std::forward< T>( information))
         {
            logger::trace << m_information << " - in";
         }

         ~Trace()
         {
            logger::trace << m_information << " - out";
         }

         Trace( const Trace&) = delete;
         Trace& operator = ( const Trace&) = delete;

      private:
         const std::string m_information;
      };
   }


}



#endif /* TRACE_H_ */