//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_TRANSACTION_MANAGER_STATE_H_
#define CASUAL_TRANSACTION_MANAGER_STATE_H_

#include "common/platform.h"
#include "common/message/transaction.h"
#include "common/message/domain.h"
#include "common/algorithm.h"
#include "common/marshal/complete.h"

#include "transaction/manager/log.h"

#include "configuration/resource/property.h"



#include <map>
#include <deque>
#include <vector>


namespace casual
{

   namespace transaction
   {
      using size_type = common::platform::size::type;

      namespace handle
      {
         namespace implementation
         {
            struct Interface;
         } // implementation
      } // handle

      class State;

      namespace state
      {
         struct Statistics
         {
            Statistics();

            common::platform::time::unit min;
            common::platform::time::unit max;
            common::platform::time::unit total;
            size_type invoked;

            void start( const common::platform::time::point::type& start);
            void end( const common::platform::time::point::type& end);

            void time( const common::platform::time::point::type& start, const common::platform::time::point::type& end);


            friend Statistics& operator += ( Statistics& lhs, const Statistics& rhs);

         private:
            common::platform::time::point::type m_start;
         };

         struct Stats
         {
            Statistics resource;
            Statistics roundtrip;

            friend Stats& operator += ( Stats& lhs, const Stats& rhs);
         };

         namespace resource
         {
            namespace id
            {
               using type = common::strong::resource::id;

               inline bool remote( type id) { return id < common::strong::resource::id{0};}
               inline bool local( type id) { return id > common::strong::resource::id{0};}

            } // id


            struct Proxy
            {
               struct Instance
               {
                  enum class State
                  {
                     absent,
                     started,
                     idle,
                     busy,
                     error,
                     shutdown
                  };

                  id::type id;
                  common::process::Handle process;

                  Stats statistics;

                  void state( State state);
                  State state() const;

                  inline friend bool operator == ( const Instance& lhs, common::strong::process::id rhs) { return lhs.process.pid == rhs;}
               private:
                  State m_state = State::absent;

               };

               struct generate_id {};

               using id_sequence = common::value::id::sequence< id::type>;

               inline Proxy( generate_id) : id( id_sequence::next()) {}

               id::type id;

               std::string key;
               std::string openinfo;
               std::string closeinfo;
               size_type concurency = 0;

               //!
               //! This 'counter' keep track of statistics for removed
               //! instances, so we can give a better view for the operator.
               //!
               Stats statistics;

               std::vector< Instance> instances;

               std::string name;
               std::string note;

               //!
               //! @return true if all instances 'has connected'
               //!
               bool booted() const;

               bool remove_instance( common::strong::process::id pid);


               friend bool operator < ( const Proxy& lhs, const Proxy& rhs)
               {
                  return lhs.id < rhs.id;
               }

               friend bool operator == ( const Proxy& lhs, id::type rhs) { return lhs.id == rhs; }
               friend bool operator == ( id::type lhs, const Proxy& rhs) { return lhs == rhs.id; }

               friend std::ostream& operator << ( std::ostream& out, const Proxy& value);
               friend std::ostream& operator << ( std::ostream& out, const Proxy::Instance& value);
               friend std::ostream& operator << ( std::ostream& out, const Proxy::Instance::State& value);
            };

            namespace external
            {
               struct Proxy
               {
                  Proxy( const common::process::Handle& process, id::type id);


                  common::process::Handle process;

                  //!
                  //! RM id
                  //!
                  id::type id;

                  friend bool operator == ( const Proxy& lhs, const common::process::Handle& rhs);
               };

               namespace proxy
               {
                  id::type id( State& state, const common::process::Handle& process);
               } // proxy

            } // external
         } // resource

         namespace pending
         {
            struct base_message
            {
               template< typename M>
               base_message( M&& information) : message{ common::marshal::complete( std::forward< M>( information))}
               {
               }

               base_message( base_message&&) noexcept = default;
               base_message& operator = ( base_message&&) noexcept = default;

               common::communication::message::Complete message;
               common::platform::time::point::type created;
            };

            struct Reply : base_message
            {
               using queue_id_type = common::strong::ipc::id;

               template< typename M>
               Reply( queue_id_type target, M&& message) : base_message( std::forward< M>( message)), target( target) {}

               queue_id_type target;
            };

            struct Request : base_message
            {
               template< typename M>
               Request( resource::id::type resource, M&& message) : base_message( std::forward< M>( message)), resource( resource) {}

               resource::id::type resource;

               inline friend bool operator == ( const Request& lhs, resource::id::type rhs) { return lhs.resource == rhs;}
            };

         } // pending
      } // state

      class State;

      struct Transaction
      {
         //!
         //! Holds specific implementations for the current transaction. 
         //! That is, depending on where the task comes from and if there
         //! are some optimizations there are different semantics for 
         //! the transaction.
         //!
         struct Dispatch
         {
            std::function< bool( State&, common::message::transaction::resource::prepare::Reply&, Transaction&)> prepare;
            std::function< bool( State&, common::message::transaction::resource::commit::Reply&, Transaction&)> commit;
            std::function< bool( State&, common::message::transaction::resource::rollback::Reply&, Transaction&)> rollback;

            inline explicit operator bool () { return static_cast< bool>( prepare);}
         };

         struct Resource
         {
            using id_type = state::resource::id::type;

            enum class Stage : short
            {
               involved,
               prepare_requested,
               prepare_replied,
               commit_requested,
               commit_replied,
               rollback_requested,
               rollback_replied,
               done,
               error,
               not_involved,
            };



            //!
            //! Used to rank the return codes from the resources, the lower the enum value (higher up),
            //! the more severe...
            //!
            enum class Result : int
            {
               xa_HEURHAZ,
               xa_HEURMIX,
               xa_HEURCOM,
               xa_HEURRB,
               xaer_RMFAIL,
               xaer_RMERR,
               xa_RBINTEGRITY,
               xa_RBCOMMFAIL,
               xa_RBROLLBACK,
               xa_RBOTHER,
               xa_RBDEADLOCK,
               xaer_PROTO,
               xa_RBPROTO,
               xa_RBTIMEOUT,
               xa_RBTRANSIENT,
               xaer_INVAL,
               xa_NOMIGRATE,
               xaer_OUTSIDE,
               xaer_ASYNC,
               xa_RETRY,
               xaer_DUPID,
               xaer_NOTA,  //! nothing to do?
               xa_OK,      //! Went as expected
               xa_RDONLY,  //! Went "better" than expected
            };

            Resource( id_type id) : id( id) {}
            Resource( Resource&&) noexcept = default;
            Resource& operator = ( Resource&&) noexcept = default;

            id_type id;
            Stage stage = Stage::involved;
            Result result = Result::xa_OK;

            static Result convert( common::code::xa value);
            static common::code::xa convert( Result value);

            void set_result( common::code::xa value);

            //!
            //! @return true if there's nothing more to do, hence this resource can be removed
            //!    from the transaction
            //!
            bool done() const;

            struct update
            {
               static inline auto stage( Resource::Stage stage)
               {
                  return [stage]( Resource& value){ value.stage = stage;};
               }
            };

            struct filter
            {
               static inline auto stage( Resource::Stage stage)
               {
                  return [stage]( const Resource& value){ return value.stage == stage;};
               }

               static inline auto result( Resource::Result result)
               {
                  return [ result]( const Resource& value){ return value.result == result;};
               }
            };

            inline friend bool operator < ( const Resource& lhs, const Resource& rhs) { return lhs.id < rhs.id; }
            inline friend bool operator == ( const Resource& lhs, const Resource& rhs) { return lhs.id == rhs.id; }
            inline friend bool operator == ( const Resource& lhs, id_type id) { return lhs.id == id; }

            inline friend std::ostream& operator << ( std::ostream& out, const Resource& value) { return out << value.id; }

         };


         Transaction() = default;
         Transaction( Transaction&&) = default;
         Transaction& operator = ( Transaction&&) = default;

         Transaction( common::transaction::ID trid) : trid( std::move( trid)) {}

         //!
         //! Depending on context the transaction will have different
         //! handle-implementations.
         //!
         Dispatch implementation;

         common::transaction::ID trid;
         std::vector< Resource> resources;

         common::platform::time::point::type started;
         common::platform::time::point::type deadline;


         //!
         //! Used to keep track of the origin for commit request.
         //!
         common::Uuid correlation;

         //!
         //! Indicate if the transaction is owned by a remote domain,
         //! and what RM id that domain act as.
         //!
         state::resource::id::type resource;


         Resource::Stage stage() const;

         //!
         //! @return the most severe result from the resources
         //!
         common::code::xa results() const;

         inline friend bool operator == ( const Transaction& lhs, const common::transaction::ID& trid) { return lhs.trid == trid;}
         friend std::ostream& operator << ( std::ostream& out, const Transaction& value);
      };


      class State
      {
      public:
         State( std::string database);

         State( const State&) = delete;
         State& operator = ( const State&) = delete;

         std::vector< Transaction> transactions;

         std::vector< state::resource::Proxy> resources;

         std::vector< state::resource::external::Proxy> externals;


         struct
         {
            //!
            //! Replies that will be sent after an atomic write to the log
            //!
            std::vector< state::pending::Reply> replies;

            //!
            //! Resource request, that will be processed after an atomic
            //! write to the log. If corresponding resources is busy, for some
            //! requests, these will be moved to pending.requests
            //!
            std::vector< state::pending::Request> requests;

         } persistent;



         struct
         {
            //!
            //! Resource request, that will be processed as soon as possible,
            //! that is, as soon as corresponding resources is done/idle
            //!
            std::vector< state::pending::Request> requests;

         } pending;







         //!
         //! the persistent transaction log
         //!
         transaction::Log log;


         std::map< std::string, configuration::resource::Property> resource_properties;


         //!
         //! @return true if there are pending stuff to do. We can't block
         //! if we got stuff to do...
         //!
         bool outstanding() const;



         //!
         //! @return true if all resource proxies is booted
         //!
         bool booted() const;


         //!
         //! @return number of total instances
         //!
         size_type instances() const;

         std::vector< common::strong::process::id> processes() const;

         void operator () ( const common::process::lifetime::Exit& death);

         state::resource::Proxy& get_resource( state::resource::id::type rm);
         state::resource::Proxy::Instance& get_instance( state::resource::id::type rm, common::strong::process::id pid);

         bool remove_instance( common::strong::process::id pid);


         using instance_range = common::range::type_t< std::vector< state::resource::Proxy::Instance>>;
         instance_range idle_instance( state::resource::id::type rm);

         const state::resource::external::Proxy& get_external( state::resource::id::type rm) const;



      private:



      };

      namespace state
      {
         namespace filter
         {

            //!
            //! @return a functor that returns true if instance is idle
            //!
            inline auto idle()
            {
               return []( const resource::Proxy::Instance& i){ return i.state() == resource::Proxy::Instance::State::idle;};
            }


            struct Running
            {

               //!
               //! @return true if instance is running
               //!
               inline bool operator () ( const resource::Proxy::Instance& instance) const
               {
                  return instance.state() == resource::Proxy::Instance::State::idle
                     || instance.state() == resource::Proxy::Instance::State::busy;
               }

               //!
               //! @return true if at least one instance in resource-proxy is running
               //!
               inline bool operator () ( const resource::Proxy& resource) const
               {
                  return common::algorithm::any_of( resource.instances, Running{});
               }
            };



         } // filter



         //!
         //! Base that holds the state
         //!
         struct Base
         {
            Base( State& state) : m_state( state) {}

         protected:
            State& m_state;

         };

         void configure( State& state, const common::message::domain::configuration::Reply& configuration);



      } // state



   } // transaction
} // casual

#endif // MANAGER_STATE_H_
