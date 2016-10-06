//!
//! casual 
//!

#include "xatmi.h"

namespace casual
{

   namespace simple_chat_protobuffer
   {
      namespace server
      {

         extern "C"
         {
            void simple_chat_protobuffer_echo( TPSVCINFO *info)
            {
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }

         }
      } // server
   } // example
} // casual
