//!
//! casual 
//!

#include "common/message/domain.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace domain
         {
            namespace scale
            {
               /*

               std::ostream& operator << ( std::ostream& out, const Executable::Scale& value)
               {
                  return out << "{ id: " << value.id
                        << ", instances: " << value.instances
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Executable& value)
               {
                  return out << "{ servers: " << range::make( value.servers)
                        << ", executables: " << range::make( value.executables)
                        << '}';
               }
               */

            } // scale
            namespace process
            {
               namespace connect
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ identification: " << value.identification
                           << ", process: " << value.process
                           << '}';
                  }
               } // connect

               namespace lookup
               {
                  std::ostream& operator << ( std::ostream& out, Request::Directive value)
                  {
                     if( value == Request::Directive::wait) { return out << "wait";}
                     return out << "direct";
                  }

                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ identification: " << value.identification
                           << ", pid: " << value.pid
                           << ", directive: " << value.directive
                           << ", process: " << value.process
                           << '}';
                  }

               } // lookup

               namespace prepare
               {
                  namespace shutdown
                  {
                     std::ostream& operator << ( std::ostream& out, const Request& value)
                     {
                        return out << "{ process: " << value.process
                              << ", processes: " << range::make( value.processes)
                              << '}';
                     }

                     std::ostream& operator << ( std::ostream& out, const Reply& value)
                     {
                        return out << "{ process: " << value.process
                              << ", processes: " << range::make( value.processes)
                              << '}';
                     }
                  } // shutdown
               } // prepare

            } // process

         } // domain

      } // message
   } // common

} // casual
