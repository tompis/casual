//!
//! resource.cpp
//!
//! Created on: Oct 20, 2014
//!     Author: Lazan
//!

#include "common/transaction/resource.h"
#include "common/transaction/context.h"

#include "common/flag.h"

namespace casual
{

   namespace common
   {
      namespace transaction
      {

         namespace local
         {
            namespace
            {
               XID* non_const_xid( const Transaction& transaction)
               {
                  return const_cast< XID*>( &transaction.trid.xid);
               }

            } // <unnamed>
         } // local


         Resource::Resource( std::string key, xa_switch_t* xa, int id, std::string openinfo, std::string closeinfo)
            : key( std::move( key)), xa_switch( xa), id( id), openinfo( std::move( openinfo)), closeinfo( std::move( closeinfo))
         {
            log::internal::transaction << "associated resource: " << *this << " name: '" <<  xa_switch->name << "' version: " << xa_switch->version << '\n';
         }

         Resource::Resource( std::string key, xa_switch_t* xa) : Resource( std::move( key), xa, 0, {}, {}) {}


         int Resource::start( const Transaction& transaction, long flags)
         {
            log::internal::transaction << "start resource: " << *this << " transaction: " << transaction << " flags: " << std::hex << flags << std::dec << '\n';

            auto result = xa_switch->xa_start_entry( local::non_const_xid( transaction), id, flags);


            if( result == XAER_DUPID)
            {
               //
               // Transaction is already associated with this thread of control, we try to join instead
               //
               log::internal::transaction << "XAER_DUPID - action: try to join instead\n";

               flags |= TMJOIN;

               result = xa_switch->xa_start_entry( local::non_const_xid( transaction), id, flags);
            }

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to start resource - " << *this << " - trid: " << transaction << '\n';
            }
            return result;
         }

         int Resource::end( const Transaction& transaction, long flags)
         {
            log::internal::transaction << "end resource: " << *this << " transaction: " << transaction << " flags: " << std::hex << flags << std::dec << '\n';

            auto result = xa_switch->xa_end_entry( local::non_const_xid( transaction), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to end resource - " << *this << " - trid: " << transaction << '\n';
            }
            return result;

         }

         int Resource::open( long flags)
         {
            log::internal::transaction << "open resource: " << *this <<  " flags: " << std::hex << flags << std::dec << '\n';

            auto result = xa_switch->xa_open_entry( openinfo.c_str(), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to open resource - " << *this << '\n';
            }

            return result;
         }

         int Resource::close( long flags)
         {
            log::internal::transaction << "close resource: " << *this <<  " flags: " << std::hex << flags << std::dec <<'\n';

            auto result = xa_switch->xa_close_entry( closeinfo.c_str(), id, flags);

            if( result != XA_OK)
            {
               log::error << error::xa::error( result) << " failed to close resource - " << *this << '\n';
            }

            return result;
         }

         int Resource::commit( const Transaction& transaction, long flags)
         {
            log::internal::transaction << "commit resource: " << *this <<  " flags: " << std::hex << flags << std::dec <<'\n';

            auto result = xa_switch->xa_commit_entry( local::non_const_xid( transaction), id, flags);

            return result;
         }

         int Resource::rollback( const Transaction& transaction, long flags)
         {
            log::internal::transaction << "rollback resource: " << *this <<  " flags: " << std::hex << flags << std::dec <<'\n';

            auto result = xa_switch->xa_rollback_entry( local::non_const_xid( transaction), id, flags);

            return result;
         }

         bool Resource::dynamic() const
         {
            return common::flag< TMREGISTER>( xa_switch->flags);
         }


         std::ostream& operator << ( std::ostream& out, const Resource& resource)
         {
            return out << "{key: " << resource.key << ", id: " << resource.id << ", openinfo: " << resource.openinfo << ", closeinfo: " << resource.closeinfo << "}";
         }


      } //transaction
   } // common

} // casual
