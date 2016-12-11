#include "app_log.h"

namespace casual
{
   namespace app
   {
      namespace log
      {
         casual::common::log::Stream debug { "app.debug"};
         casual::common::log::Stream verbose { "app.verbose"};
         casual::common::log::Stream error { "app.error"};
         casual::common::log::Stream warning { "app.warning"};
         casual::common::log::Stream information { "app.information"};
      }
   }

}