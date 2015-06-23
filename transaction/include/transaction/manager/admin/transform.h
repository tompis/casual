//!
//! transform.h
//!
//! Created on: Jun 14, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_TRANSACTION_MANAGER_ADMIN_TRANSFORM_H_
#define CASUAL_TRANSACTION_MANAGER_ADMIN_TRANSFORM_H_

#include "transaction/manager/admin/transactionvo.h"
#include "transaction/manager/state.h"

namespace casual
{
   namespace transaction
   {
      namespace transform
      {


         vo::State state( const State& state);


      } // transform

   } // transaction



} // casual

#endif // TRANSFORM_H_
