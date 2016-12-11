#include <common/internal/log.h>

#ifndef CASUAL_APP_LOG_INCLUDED
#define CASUAL_APP_LOG_INCLUDED

namespace casual
{
   namespace app
   {
      namespace log
      {
         extern casual::common::log::Stream debug;
         extern casual::common::log::Stream verbose;
         extern casual::common::log::Stream error;
         extern casual::common::log::Stream warning;
         extern casual::common::log::Stream information;
      }
   }

}

#endif