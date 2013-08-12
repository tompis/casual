


#ifndef CASUAL_MESSAGES_H_
#define CASUAL_MESSAGES_H_




#include "common/ipc.h"
#include "common/buffer_context.h"
#include "common/types.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/uuid.h"

#include <vector>
#include <chrono>

namespace casual
{
   namespace common
   {

      namespace message
      {
         enum Type
         {

            cServerConnect  = 10, // message type can't be 0!
            cServerConfiguration,
            cServerDisconnect,
            cServiceAdvertise = 20,
            cServiceUnadvertise,
            cServiceNameLookupRequest,
            cServiceNameLookupReply,
            cServiceCall,
            cServiceReply,
            cServiceAcknowledge,
            cMonitorConnect = 30,
            cMonitorDisconnect,
            cMonitorNotify,
            cTransactionManagerConnect = 40,
            cTransactionBegin,
            cTransactionPrepare,
            cTransactionCommit,
            cTransactionRollback,
            cTransactionGenericReply,
            cTransactionPreparedReply,
            cTransactionResurceConnectReply,
            cTransactionResurceGenericReply

            //cTransactionMonitorUnadvertise,

         };


         struct Transaction
         {
            typedef common::platform::pid_type pid_type;

            Transaction() : creator( 0)
            {
               xid.formatID = common::cNull_XID;
            }

            XID xid;
            pid_type creator;

            template< typename A>
            void marshal( A& archive)
            {
               archive & xid;
               archive & creator;
            }
         };


         struct Service
         {
            typedef int Seconds;

            Service() = default;
            Service& operator = (const Service& rhs) = default;

            explicit Service( const std::string& name_) : name( name_)
            {}

            std::string name;
            Seconds timeout = 0;
            common::platform::queue_id_type monitor_queue = 0;

            template< typename A>
            void marshal( A& archive)
            {
               archive & name;
               archive & timeout;
               archive & monitor_queue;
            }
         };



         namespace server
         {

            //!
            //! Represents id for a server.
            //!
            struct Id
            {

               typedef platform::queue_id_type queue_id_type;
               typedef common::platform::pid_type pid_type;


               Id() = default;
               Id( queue_id_type id, pid_type pid) : queue_id( id), pid( pid) {}


               queue_id_type queue_id = 0;
               pid_type pid = common::process::id();

               template< typename A>
               void marshal( A& archive)
               {
                  archive & queue_id;
                  archive & pid;
               }
            };

            template< message::Type type>
            struct basic_connect
            {
               enum
               {
                  message_type = type
               };

               server::Id server;
               std::string path;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
                  archive & path;
               }
            };

            template< message::Type type>
            struct basic_disconnect
            {
               enum
               {
                  message_type = type
               };

               server::Id server;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
               }
            };

            struct Connect : public basic_connect< cServerConnect>
            {
               typedef basic_connect< cServerConnect> base_type;

               std::vector< Service> services;

               template< typename A>
               void marshal( A& archive)
               {
                  base_type::marshal( archive);
                  archive & services;
               }

            };

            namespace resource
            {
               struct Manager
               {
                  /*
                  Manager() = default;
                  Manager( Manager&&) = default;
                  Manager& operator = ( Manager&&) = default;
                  */

                  std::string key;
                  std::string openinfo;
                  std::string closeinfo;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & key;
                     archive & openinfo;
                     archive & closeinfo;
                  }
               };

            }

            //!
            //! Sent from the broker with "start-up-information" for a server
            //!
            struct Configuration
            {
               enum
               {
                  message_type = cServerConfiguration
               };

               Configuration() = default;
               Configuration( Configuration&&) = default;
               Configuration& operator = ( Configuration&&) = default;

               typedef platform::queue_id_type queue_id_type;

               queue_id_type transactionManagerQueue = 0;
               std::vector< resource::Manager> resourceManagers;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & transactionManagerQueue;
                  archive & resourceManagers;
               }
            };

            typedef basic_disconnect< cServerDisconnect> Disconnect;

         } // server



         namespace service
         {

            struct Advertise
            {
               enum
               {
                  message_type = cServiceAdvertise
               };

               std::string serverPath;
               server::Id server;
               std::vector< Service> services;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & serverPath;
                  archive & server;
                  archive & services;
               }
            };

            struct Unadvertise
            {
               enum
               {
                  message_type = cServiceUnadvertise
               };

               server::Id server;
               std::vector< Service> services;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
                  archive & services;
               }
            };

            namespace name
            {
               namespace lookup
               {
                  //!
                  //! Represent "service-name-lookup" request.
                  //!
                  struct Request
                  {
                     enum
                     {
                        message_type = cServiceNameLookupRequest
                     };

                     std::string requested;
                     server::Id server;

                     template< typename A>
                     void marshal( A& archive)
                     {
                        archive & requested;
                        archive & server;
                     }
                  };

                  //!
                  //! Represent "service-name-lookup" response.
                  //!
                  struct Reply
                  {

                     enum
                     {
                        message_type = cServiceNameLookupReply
                     };

                     Service service;

                     std::vector< server::Id> server;

                     template< typename A>
                     void marshal( A& archive)
                     {
                        archive & service;
                        archive & server;
                     }
                  };
               } // lookup
            } // name

            struct base_call
            {
               enum
               {
                  message_type = cServiceCall
               };

               base_call() = default;

               base_call( base_call&&) = default;
               base_call& operator = ( base_call&&) = default;

               base_call( const base_call&) = delete;
               base_call& operator = ( const base_call&) = delete;

               int callDescriptor = 0;
               Service service;
               server::Id reply;
               common::Uuid callId;
               std::string callee;
               Transaction transaction;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & callDescriptor;
                  archive & service;
                  archive & reply;
                  archive & callId;
                  archive & callee;
                  archive & transaction;
               }
            };

            namespace callee
            {

               //!
               //! Represents a service call. via tp(a)call, from the callee's perspective
               //!
               struct Call: public base_call
               {

                  Call() = default;

                  Call( Call&&) = default;
                  Call& operator = ( Call&&) = default;

                  Call( const Call&) = delete;
                  Call& operator = ( const Call&) = delete;

                  buffer::Buffer buffer;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     base_call::marshal( archive);
                     archive & buffer;
                  }
               };}

            namespace caller
            {
               //!
               //! Represents a service call. via tp(a)call, from the callers perspective
               //!
               struct Call: public base_call
               {

                  Call( buffer::Buffer& buffer_)
                        : buffer( buffer_)
                  {
                  }

                  Call( Call&&) = default;
                  Call& operator = ( Call&&) = default;

                  Call( const Call&) = delete;
                  Call& operator = ( const Call&) = delete;

                  buffer::Buffer& buffer;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     base_call::marshal( archive);
                     archive & buffer;
                  }
               };

            }

            //!
            //! Represent service reply.
            //!
            struct Reply
            {
               enum
               {
                  message_type = cServiceReply
               };

               Reply() = default;
               Reply( Reply&&) = default;


               Reply( const Reply&) = delete;
               Reply& operator = ( const Reply&) = delete;

               int callDescriptor = 0;
               int returnValue = 0;
               long userReturnCode = 0;
               buffer::Buffer buffer;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & callDescriptor;
                  archive & returnValue;
                  archive & userReturnCode;
                  archive & buffer;
               }

            };

            //!
            //! Represent the reply to the broker when a server is done handling
            //! a service-call and is ready for new calls
            //!
            struct ACK
            {
               enum
               {
                  message_type = cServiceAcknowledge
               };

               std::string service;
               server::Id server;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & service;
                  archive & server;
               }
            };
         } // service


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
            struct Notify
            {
               enum
               {
                  message_type = cMonitorNotify
               };

               std::string parentService;
               std::string service;

               common::Uuid callId;

               std::string transactionId;

               common::time_type start;
               common::time_type end;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & parentService;
                  archive & service;
                  archive & callId;
                  archive & transactionId;
                  archive & start;
                  archive & end;
               }
            };
         } // monitor

         namespace transaction
         {
            //!
            //! Used to connect the transaction monitor to broker
            //!
            typedef server::basic_connect< cTransactionManagerConnect> Connect;

            //!
            //! Used to unadvertise the transaction monitor
            //!
            //typedef basic_disconnect< cTransactionMonitorUnadvertise> Unadvertise;


            namespace reply
            {
               template< message::Type type>
               struct basic_reply
               {
                  typedef common::platform::pid_type pid_type;
                  enum
                  {
                     message_type = type
                  };

                  server::Id id;
                  int state = 0;

                  template< typename A>
                  void marshal( A& archive)
                  {
                     archive & id;
                     archive & state;

                  }
               };

               //!
               //! Reply from the transaction monitor
               //!
               typedef basic_reply< cTransactionGenericReply> Generic;

               //!
               //! Reply from the transaction monitor
               //!
               typedef basic_reply< cTransactionPreparedReply> Prepared;


               namespace resource
               {
                  //!
                  //! Used to notify the TM that a resource proxy is up and running, or not...
                  //!
                  typedef basic_reply< cTransactionResurceConnectReply> Connect;

                  typedef basic_reply< cTransactionResurceGenericReply> Generic;

               } // resource

            } // reply



            template< message::Type type>
            struct basic_transaction
            {
               typedef basic_transaction< type> base_type;

               enum
               {
                  message_type = type
               };

               server::Id server;
               XID xid;

               template< typename A>
               void marshal( A& archive)
               {
                  archive & server;
                  archive & xid;
               }
            };

            struct Begin : public basic_transaction< cTransactionBegin>
            {
               template< typename A>
               void marshal( A& archive)
               {
                  base_type::marshal( archive);
                  archive & start;
               }

               common::time_type start;
            };

            typedef basic_transaction< cTransactionPrepare> Prepare;
            typedef basic_transaction< cTransactionCommit> Commit;
            typedef basic_transaction< cTransactionRollback> Rollback;


         } // transaction

         //!
         //! Deduce witch type of message it is.
         //!
         template< typename M>
         ipc::message::Transport::message_type_type type( const M&)
         {
            return M::message_type;
         }
      } // message
   } //common
} // casual

#endif /* CASUAL_IPC_MESSAGES_H_ */
