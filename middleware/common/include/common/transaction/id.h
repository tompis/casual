//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include <tx.h>


#include "common/uuid.h"

#include "common/view/binary.h"
#include "common/algorithm.h"
#include "common/process.h"

#include "common/serialize/customize.h"


#include <string>
#include <ostream>


//! Global scope compare operations for XID
//! @{
bool operator == ( const XID& lhs, const XID& rhs);
bool operator < ( const XID& lhs, const XID& rhs);
bool operator != ( const XID& lhs, const XID& rhs);
//! @}


//! Global stream operator for XID
std::ostream& operator << ( std::ostream& out, const XID& xid);

namespace casual
{
   namespace common
   {
      namespace transaction
      {
         using xid_type = XID;

         using xid_range_type = decltype( view::binary::make( std::declval< const xid_type&>().data));

         class ID
         {
         public:

            struct Format
            {
               enum 
               {
                  null = -1,
                  casual = 42
               };
            };

            //! Creates a new unique transaction id, global and branch
            static ID create();
            static ID create( const process::Handle& owner);


            //! Initialize with null-xid
            //! @{
            ID() noexcept;
            ID( const process::Handle& owner);
            //! @}

            explicit ID( const xid_type& xid);


            //! Initialize with uuid, gtrid and bqual.
            //! Sets the format id to "casual"
            //!
            //! @note not likely to be used other than unittesting
            ID( Uuid gtrid, Uuid bqual, const process::Handle& owner);

            ID( ID&&) noexcept;
            ID& operator = ( ID&&) noexcept;

            ID( const ID&) noexcept = default;
            ID& operator = ( const ID&) noexcept = default;

            //! Creates a new Id with same global transaction id but a new branch id.
            ID branch() const;

            //! @return true if XID is null
            bool null() const;

            //! @return true if XID is not null
            explicit operator bool() const;

            //! Return the raw data of the xid in a range
            xid_range_type range() const;

            //! @return owner/creator of the transaction
            const process::Handle& owner() const;
            void owner( const process::Handle& handle);

            friend bool operator < ( const ID& lhs, const ID& rhs);
            friend bool operator == ( const ID& lhs, const ID& rhs);
            friend bool operator == ( const ID& lhs, const xid_type& rhs);
            inline friend bool operator != ( const ID& lhs, const ID& rhs)
            {
               return ! ( lhs == rhs);
            }

            template< typename T>
            friend struct serialize::customize::Value;

            //! The XA-XID object.
            //!
            //! We need to have access to the xid to communicate via xa and such,
            //! no reason to keep it private and have getters..
            xid_type xid{};

            friend std::ostream& operator << ( std::ostream& out, const ID& id);

         private:
            process::Handle m_owner;
         };

         //! @return a (binary) range that represent the data part of the xid, global + branch
         //! @{
         xid_range_type data( const ID& id);
         xid_range_type data( const xid_type& id);
         //! @}

         //! @return a (binary) range that represent the global part of the xid
         //! @{
         xid_range_type global( const ID& id);
         xid_range_type global( const xid_type& id);
         //! @}

         //! @return a (binary) range that represent the branch part of the xid
         //! @{
         xid_range_type branch( const ID& id);
         xid_range_type branch( const xid_type& id);
         //! @}

         //! @return true if trid is null, false otherwise
         //! @{
         bool null( const ID& id);
         bool null( const xid_type& id);
         //! @}
         
      } // transaction

      namespace serialize
      {
         namespace customize
         {
            //! specialization for transaction ID
            template<>
            struct Value< transaction::ID>
            {
               template< typename A> 
               static auto write( A& archive, const transaction::ID& value, const char*) -> 
                  std::enable_if_t< ! traits::need::named< A>::value>
               {
                  archive << value.xid.formatID;

                  if( value)
                  {
                     archive << value.m_owner;
                     archive << value.xid.gtrid_length;
                     archive << value.xid.bqual_length;
                     archive.append( value.range());
                  }
               }

               template< typename A> 
               static auto read( A& archive, transaction::ID& value, const char*) -> 
                  std::enable_if_t< ! traits::need::named< A>::value>
               {
                  archive >> value.xid.formatID;

                  if( value)
                  {
                     archive >> value.m_owner;

                     archive >> value.xid.gtrid_length;
                     archive >> value.xid.bqual_length;

                     archive.consume(
                        std::begin( value.xid.data),
                        value.xid.gtrid_length + value.xid.bqual_length);
                  }
               }
            };

         } // customize
      } // serialize

   } // common
} // casual





