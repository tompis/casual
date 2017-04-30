//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_LOG_CATEGORY_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_LOG_CATEGORY_H_

#include "common/log/stream.h"

namespace casual
{
   namespace common
   {
      namespace log
      {
         namespace category
         {

            //!
            //! Log with category 'parameter'
            //!
            extern Stream parameter;

            //!
            //! Log with category 'information'
            //!
            extern Stream information;

            //!
            //! Log with category 'warning'
            //!
            //! @note should be used very sparsely. Either it is an error or it's not...
            //!
            extern Stream warning;

            //!
            //! Log with category 'error'
            //!
            //! @note always active
            //!
            extern Stream error;


            //!
            //! Log with category 'casual.transaction'
            //!
            extern Stream transaction;

            //!
            //! Log with category 'casual.buffer'
            //!
            extern Stream buffer;

         } // category
      } // log
   } // common



} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_LOG_CATEGORY_H_