//!
//! casual_calling_context.h
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_CALLING_CONTEXT_H_
#define CASUAL_CALLING_CONTEXT_H_


#include "common/queue.h"
#include "common/platform.h"
#include "common/message/server.h"



#include <vector>


namespace casual
{
   namespace common
   {


      namespace server
      {
         class Context;
      }

      namespace call
      {
         using descriptor_type = platform::descriptor_type;

         struct State
         {

            struct Pending
            {
               struct Descriptor
               {
                  Descriptor( descriptor_type descriptor, bool active = true)
                    : descriptor( descriptor), active( active) {}

                  descriptor_type descriptor;
                  bool active;

                  friend bool operator == ( descriptor_type cd, const Descriptor& d) { return cd == d.descriptor;}
                  friend bool operator == ( const Descriptor& d, descriptor_type cd) { return cd == d.descriptor;}
               };

               struct Transaction
               {
                  Transaction( const transaction::ID& trid, descriptor_type descriptor)
                    : m_trid( trid), m_involved{ descriptor} {}



                  bool remove( descriptor_type descriptor)
                  {
                     auto found = range::find( m_involved, descriptor);

                     if( found) { m_involved.erase( found.first);}

                     return m_involved.empty();
                  }

                  void add( descriptor_type descriptor) { m_involved.push_back( descriptor);}

                  friend bool operator == ( const Transaction& lhs, const transaction::ID& rhs) { return lhs.m_trid == rhs;}
                  friend bool operator == ( const Transaction& lhs, descriptor_type rhs) { return range::any_of( lhs.m_involved, std::bind( equal_to{}, std::placeholders::_1, rhs));}
               private:
                  transaction::ID m_trid;
                  std::vector< descriptor_type> m_involved;
               };

               struct Correlation
               {
                  Correlation( descriptor_type descriptor, const Uuid& correlation)
                   : descriptor( descriptor), correlation( correlation) {}

                  descriptor_type descriptor;
                  Uuid correlation;

                  friend bool operator == ( const Correlation& lhs, descriptor_type rhs) { return lhs.descriptor == rhs;}
               };

               Pending();

               //!
               //! Reserves a descriptor and associates it to message-correlation and transaction
               //!
               descriptor_type reserve( const Uuid& correlation, const transaction::ID& transaction);

               //!
               //! Reserves a descriptor and associates it to message-correlation
               //!
               descriptor_type reserve( const Uuid& correlation);

               void unreserve( descriptor_type descriptor);

               bool active( descriptor_type descriptor) const;

               const Uuid& correlation( descriptor_type descriptor) const;

               //!
               //! Tries to discard descriptor, throws if fail.
               //!
               void discard( descriptor_type descriptor);

               //!
               //! @returns true if there are no pending replies or associated transactions.
               //!  Thus, it's ok to do a service-forward
               //!
               bool empty() const;


            private:

               descriptor_type reserve();

               std::vector< Descriptor> m_descriptors;
               std::vector< Correlation> m_correlations;

            } pending;

         };

         class Context
         {
         public:
            static Context& instance();


            descriptor_type async( const std::string& service, char* idata, long ilen, long flags);


            void reply( descriptor_type& descriptor, char** odata, long& olen, long flags);

            void sync( const std::string& service, char* idata, const long ilen, char*& odata, long& olen, const long flags);

            void cancel( descriptor_type cd);

            void clean();

            //!
            //! @returns true if there are pending replies or associated transactions.
            //!  Hence, it's ok to do a service-forward if false is return
            //!
            bool pending() const { return ! m_state.pending.empty();}

         private:


            Context();

            bool receive( message::service::call::Reply& reply, descriptor_type descriptor, long flags);

            State m_state;

         };
      } // call
	} // common
} // casual


#endif /* CASUAL_CALLING_CONTEXT_H_ */
