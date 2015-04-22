//!
//! monitor.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#ifndef COMMMONMESSAGEMONITOR_H_
#define COMMMONMESSAGEMONITOR_H_

#include "common/message/type.h"


namespace casual
{
   namespace common
   {

      namespace message
      {
         namespace monitor
         {
            //!
            //! Used to advertise the monitorserver
            //!
            typedef server::basic_connect< cMonitorConnect> Connect;

            //!
            //! Used to unadvertise the monitorserver
            //!
            typedef server::basic_disconnect< cMonitorDisconnect> Disconnect;

            //!
            //! Notify monitorserver with statistics
            //!
            struct Notify : basic_message< cMonitorNotify>
            {

               std::string parentService;
               std::string service;

               common::Uuid callId;

               std::string transactionId;

               common::platform::time_point start;
               common::platform::time_point end;

               CASUAL_CONST_CORRECT_MARSHAL
               (
                  archive & parentService;
                  archive & service;
                  archive & callId;
                  archive & transactionId;
                  archive & start;
                  archive & end;
               )

               friend std::ostream& operator << ( std::ostream& out, const Notify& value);
            };
         } // monitor

      } // message
   } // common
} // casual

#endif // MONITOR_H_
