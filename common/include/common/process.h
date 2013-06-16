//!
//! process.h
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#ifndef PROCESS_H_
#define PROCESS_H_

#include "common/platform.h"

//
// std
//
#include <string>
#include <vector>
#include <chrono>

namespace casual
{
   namespace common
   {
      namespace process
      {

         //!
         //! Sleep for a while
         //!
         //! @param time numbers of microseconds to sleep
         //!
         void sleep( std::chrono::microseconds time);

         //!
         //! Sleep for an arbitrary duration
         //!
         //! Example:
         //! ~~~~~~~~~~~~~~~{.cpp}
         //! // sleep for 20 seconds
         //! process::sleep( std::chrono::seconds( 20));
         //!
         //! // sleep for 2 minutes
         //! process::sleep( std::chrono::minutes( 2));
         //! ~~~~~~~~~~~~~~~
         //!
         template< typename R, typename P>
         void sleep( std::chrono::duration< R, P> time)
         {
            sleep( std::chrono::duration_cast< std::chrono::microseconds>( time));
         }


         //!
         //! Spawn a new application that path describes
         //!
         //! @param path path to application to be spawned
         //! @param arguments 0..N arguments that is passed to the application
         //! @return process id of the spawned process
         //!
         platform::pid_type spawn( const std::string& path, const std::vector< std::string>& arguments);

         //!
         //! check if there are sub processes that has been terminated
         //!
         //! @return 0..N terminated process id's
         //!
         std::vector< platform::pid_type> terminated();

         //!
         //! Wait for a specific process to terminate.
         //!
         //! @attention this i mostly for unittest, and it't unlikely we have any use for this
         //!    blocking semantics in real code..
         //!
         platform::pid_type wait( platform::pid_type pid);


      } // process
   } // common
} // casual


#endif /* PROCESS_H_ */