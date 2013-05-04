//!
//! types.h
//!
//! Created on: Nov 24, 2012
//!     Author: Lazan
//!

#ifndef TYPES_H_
#define TYPES_H_

#include <vector>

#include <chrono>

#include <string>


namespace casual
{
   namespace common
   {
      typedef std::vector< char> binary_type;

      typedef const char* raw_buffer_type;

      typedef std::chrono::steady_clock clock_type;

      typedef clock_type::time_point time_type;

      namespace transform
      {
         inline char* public_buffer( raw_buffer_type buffer)
         {
            return const_cast< char*>( buffer);
         }

         std::string local();
         std::string local( const time_type& time);
         std::string universal();
         std::string universal( const time_type& time);

      }

   }

   //!
   //! Overload for time_type
   //!
   //! @{
   template< typename M>
   void marshal_value( M& marshler, common::time_type& value)
   {
      auto time = value.time_since_epoch().count();
      marshler << time;
   }

   template< typename M>
   void unmarshal_value( M& unmarshler, common::time_type& value)
   {
      common::time_type::rep representation;
      unmarshler >> representation;
      value = common::time_type( common::time_type::duration( representation));
   }
   //! @}

}



#endif /* TYPES_H_ */
