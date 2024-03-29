//!
//! ipc.cpp
//!
//! Created on: Jan 5, 2016
//!     Author: Lazan
//!

#include "common/communication/ipc.h"
#include "common/environment.h"
#include "common/error.h"


#include <fstream>

#include <sys/msg.h>

namespace casual
{
   namespace common
   {

      namespace communication
      {
         namespace ipc
         {

            namespace native
            {
               bool send( handle_type id, const message::Transport& transport, long flags)
               {

                  auto size = message::Transport::header_size + transport.size();

                  auto result = msgsnd( id, &const_cast< message::Transport&>( transport).message, size, flags);

                  if( result == -1)
                  {
                     auto code = errno;

                     switch( code)
                     {
                        case EAGAIN:
                        {
                           return false;
                        }
                        case EINTR:
                        {
                           log::internal::ipc << "ipc::native::send - signal received\n";
                           common::signal::handle();

                           //
                           // we got a signal we don't have a handle for
                           // We continue
                           //
                           return send( id, transport, flags);
                        }
                        case EIDRM:
                        {
                           throw exception::queue::Unavailable{ "queue unavailable - id: " + std::to_string( id) + " - " + common::error::string()};
                        }
                        case ENOMEM:
                        {
                           throw exception::limit::Memory{ "id: " + std::to_string( id) + " - " + common::error::string()};
                        }
                        case EINVAL:
                        {
                           if( /* message.size() < MSGMAX  && */ transport.message.type > 0)
                           {
                              //
                              // The problem is with queue-id. We guess that it has been removed.
                              //
                              throw exception::queue::Unavailable{ "queue unavailable - id: " + std::to_string( id) + " - " + common::error::string()};
                           }
                           // we let it fall through to default
                        }
                        // no break
                        case EFAULT:
                        default:
                        {
                           throw common::exception::invalid::Argument( "invalid queue arguments - id: " + std::to_string( id) + " - " + common::error::string());
                        }
                     }
                  }

                  log::internal::ipc << "---> [" << id << "] send transport: " << transport << " - flags: " << flags << '\n';

                  return true;
               }
               bool receive( handle_type id, message::Transport& transport, long flags)
               {
                  auto result = msgrcv( id, &transport.message, message::Transport::message_max_size, 0, flags);

                  if( result == -1)
                  {
                     auto code = errno;

                     switch( code)
                     {
                        case EINTR:
                        {
                           log::internal::ipc << "ipc::native::receive - signal received\n";

                           common::signal::handle();

                           //
                           // we got a signal we don't have a handle for
                           // We continue
                           //
                           return receive( id, transport, flags);
                        }
                        case ENOMSG:
                        case EAGAIN:
                        {
                           return false;
                        }
                        case EIDRM:
                        {
                           throw exception::queue::Unavailable{ "queue removed - id: " + std::to_string( id) + " - " + common::error::string()};
                        }
                        default:
                        {
                           std::ostringstream msg;
                           msg << "ipc < [" << id << "] receive failed - transport: " << transport << " - flags: " << flags << " - " << common::error::string();
                           log::internal::ipc << msg.str() << std::endl;
                           throw exception::invalid::Argument( msg.str(), __FILE__, __LINE__);
                        }
                     }
                  }

                  log::internal::ipc << "<--- [" << id << "] receive transport: " << transport << " - flags: " << flags << '\n';

                  return true;

               }

            } // native

            namespace inbound
            {

               Connector::Connector()
                : m_id( msgget( IPC_PRIVATE, IPC_CREAT | 0660))
               {
                  if( m_id  == cInvalid)
                  {
                     throw exception::invalid::Argument( "ipc queue create failed - " + common::error::string(), __FILE__, __LINE__);
                  }
                  log::internal::ipc << "queue id: " << m_id << " created\n";
               }

               Connector::~Connector()
               {
                  remove( m_id);
               }

               Connector::Connector( Connector&& rhs) noexcept
               {
                  swap( *this, rhs);
               }
               Connector& Connector::operator = ( Connector&& rhs) noexcept
               {
                  Connector temp{ std::move( rhs)};
                  swap( *this, temp);
                  return *this;
               }


               void swap( Connector& lhs, Connector& rhs)
               {
                  using std::swap;
                  swap( lhs.m_id, rhs.m_id);
               }


               Device& device()
               {
                  static Device singlton;
                  return singlton;
               }

               handle_type id()
               {
                  return device().connector().id();
               }

            } // inbound



            namespace policy
            {

               namespace prefix
               {

                  Signal::Signal()
                  {
                     common::signal::handle();
                  }


               } // prefix



               bool basic_blocking::operator() ( inbound::Connector& ipc, message::Transport& transport)
               {
                  return native::receive( ipc.id(), transport, 0);
               }

               bool basic_blocking::operator() ( const outbound::Connector& ipc, const message::Transport& transport)
               {
                  return native::send( ipc, transport, 0);
               }


               namespace non
               {
                  bool basic_blocking::operator() ( inbound::Connector& ipc, message::Transport& transport)
                  {
                     return native::receive( ipc.id(), transport, platform::cIPC_NO_WAIT);
                  }

                  bool basic_blocking::operator() ( const outbound::Connector& ipc, const message::Transport& transport)
                  {
                     return native::send( ipc, transport, platform::cIPC_NO_WAIT);
                  }

               } // non
            } // policy

            namespace local
            {
               namespace
               {
                  outbound::Device initialize_broker_device()
                  {
                     static const std::string brokerFile = common::environment::file::broker::device();

                     std::ifstream file( brokerFile.c_str());

                     if( ! file)
                     {
                        log::internal::ipc << "Failed to open broker queue configuration file" << std::endl;
                        throw common::exception::xatmi::System( "Failed to open broker queue configuration file: " + brokerFile);
                     }

                     handle_type id{ 0};
                     file >> id;

                     return { id};

                  }
               } // <unnamed>
            } // local

            namespace broker
            {
               outbound::Device& device()
               {
                  static outbound::Device singelton = local::initialize_broker_device();
                  return singelton;
               }

               handle_type id()
               {
                  return device().connector().id();
               }

            } // broker

            bool exists( handle_type id)
            {
               struct msqid_ds info;

               return msgctl( id, IPC_STAT, &info) == 0;
            }

            bool remove( handle_type id)
            {
               if( id != -1)
               {
                  if( msgctl( id, IPC_RMID, nullptr) == 0)
                  {
                     log::internal::ipc << "queue id: " << id << " removed\n";
                     return true;
                  }
                  else
                  {
                     log::error << "failed to remove ipc-queue with id: " << id << " - " << common::error::string() << "\n";
                  }
               }
               return false;
            }

            bool remove( const process::Handle& owner)
            {
               struct msqid_ds info;

               if( msgctl( owner.queue, IPC_STAT, &info) != 0)
               {
                  return false;
               }
               if( info.msg_lrpid == owner.pid)
               {
                  return remove( owner.queue);
               }
               return false;
            }

         } // ipc

      } // communication
   } // common
} // casual
