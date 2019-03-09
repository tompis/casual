//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>
#include "common/unittest.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/managervo.h"

#include "common/message/domain.h"
#include "common/message/event.h"
#include "common/message/service.h"

#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"
#include "common/mockup/process.h"
#include "common/service/lookup.h"
#include "common/exception/xatmi.h"
#include "common/communication/instance.h"
#include "common/event/listen.h"

#include "serviceframework/service/protocol/call.h"

namespace casual
{
   namespace service
   {
      namespace local
      {
         namespace
         {
            struct Manager
            {
               Manager() : process{ "./bin/casual-service-manager", { "--forward", "./bin/casual-service-forward"}}
               {
                  // Wait for manager to get online
                  EXPECT_TRUE( process.handle() != common::process::handle());
               }
               ~Manager() = default;

               common::mockup::Process process;
            };

            struct Domain : common::mockup::domain::Manager
            {
               using common::mockup::domain::Manager::Manager;

               local::Manager service;
               common::mockup::domain::transaction::Manager tm;
            };

            namespace call
            {
               manager::admin::StateVO state()
               {
                  serviceframework::service::protocol::binary::Call call;

                  auto reply = call( manager::admin::service::name::state());

                  manager::admin::StateVO serviceReply;

                  reply >> CASUAL_MAKE_NVP( serviceReply);

                  return serviceReply;
               }

            } // call

            namespace service
            {
               const manager::admin::ServiceVO* find( const manager::admin::StateVO& state, const std::string& name)
               {
                  auto found = common::algorithm::find_if( state.services, [&name]( auto& s){
                     return s.name == name;
                  });

                  if( found) { return &( *found);}

                  return nullptr;
               }
            } // service

            namespace instance
            {
               const manager::admin::instance::SequentialVO* find( const manager::admin::StateVO& state, common::strong::process::id pid)
               {
                  auto found = common::algorithm::find_if( state.instances.sequential, [pid]( auto& i){
                     return i.process.pid == pid;
                  });

                  if( found) { return &( *found);}

                  return nullptr;
               }
            } // instance

         } // <unnamed>
      } // local


      TEST( service_manager, startup_shutdown__expect_no_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain domain;
         });

      }

      namespace local
      {
         namespace
         {
            bool has_services( const std::vector< manager::admin::ServiceVO>& services, std::initializer_list< const char*> wanted)
            {
               return common::algorithm::all_of( wanted, [&services]( auto& s){
                  return common::algorithm::find_if( services, [&s]( auto& service){
                     return service.name == s;
                  });
               });
            }

         } // <unnamed>
      } // local

      TEST( service_manager, startup___expect_1_service_in_state)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto state = local::call::state();

         EXPECT_TRUE( local::has_services( state.services, { manager::admin::service::name::state()}));
         EXPECT_FALSE( local::has_services( state.services, { "non/existent/service"}));
      }


      TEST( service_manager, advertise_2_services_for_1_server__expect__3_services)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         auto state = local::call::state();

         ASSERT_TRUE( local::has_services( state.services, { manager::admin::service::name::state(), "service1", "service2"}));
         
         {
            auto service = local::service::find( state, "service1");
            ASSERT_TRUE( service);
            ASSERT_TRUE( service->instances.sequential.size() == 1);
            EXPECT_TRUE( service->instances.sequential.at( 0).pid == server.process().pid);
         }
         {
            auto service = local::service::find( state, "service2");
            ASSERT_TRUE( service);
            ASSERT_TRUE( service->instances.sequential.size() == 1);
            EXPECT_TRUE( service->instances.sequential.at( 0).pid == server.process().pid);
         }
      }

      TEST( service_manager, advertise_2_services_for_1_server__expect__2_local_instances)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         auto state = local::call::state();

         ASSERT_TRUE( state.instances.sequential.size() == 2);
         ASSERT_TRUE( state.instances.concurrent.empty());

         {
            auto instance = local::instance::find( state, server.process().pid);
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->state == manager::admin::instance::SequentialVO::State::idle);
         }
      }


      TEST( service_manager, advertise_2_services_for_1_server__prepare_shutdown___expect_instance_exiting)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( server.process());

            auto reply = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);

         }

         auto state = local::call::state();

         {
            auto instance = local::instance::find( state, server.process().pid);
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->state == manager::admin::instance::SequentialVO::State::exiting);
         }
      }

      TEST( service_manager, advertise_2_services_for_1_server__prepare_shutdown__lookup___expect_absent)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( server.process());

            auto reply = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);

         }

         EXPECT_THROW({
            auto service = common::service::Lookup{ "service1"}();
         }, common::exception::xatmi::service::no::Entry);

         EXPECT_THROW({
            auto service = common::service::Lookup{ "service2"}();
         }, common::exception::xatmi::service::no::Entry);
      }

      TEST( service_manager, advertise_new_services_current_server)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }

         {
            auto service = common::service::Lookup{ "service2"}();
            EXPECT_TRUE( service.service.name == "service2");

            // we only have one instance, we expect this to be busy
            EXPECT_TRUE( service.state == decltype( service)::State::busy);
         }
      }


      TEST( service_manager, unadvertise_service)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         server.undadvertise( { { "service2"}});

         //
         // echo server has unadvertise this service. The service is
         // still "present" in service-manager with no instances. Hence it's absent
         //
         EXPECT_THROW({
            auto service = common::service::Lookup{ "service2"}();
         }, common::exception::xatmi::service::no::Entry);
      }



      TEST( service_manager, service_lookup_non_existent__expect_absent_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         EXPECT_THROW({
            auto service = common::service::Lookup{ "non-existent-service"}();
         }, common::exception::xatmi::service::no::Entry);
      }


      TEST( service_manager, service_lookup_service1__expect__busy_reply__send_ack____expect__idle_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }

         common::service::Lookup lookup{ "service2"};
         {
            auto service = lookup();
            EXPECT_TRUE( service.service.name == "service2");

            // we only have one instance, we expect this to be busy
            EXPECT_TRUE( service.state == decltype( service)::State::busy);
         }

         {
            // Send Ack
            {
               common::message::service::call::ACK message;
               message.metric.process = server.process();

               common::communication::ipc::blocking::send( 
                  common::communication::instance::outbound::service::manager::device(),
                  message);

            }

            // get next pending reply
            auto service = lookup();

            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }
      }


      TEST( service_manager, service_lookup_service1__service_lookup_service1_forward_context____expect_forward_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { "service1"}};

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }


         {
            common::service::Lookup lookup{ "service1", common::service::Lookup::Context::forward};
            auto service = lookup();
            EXPECT_TRUE( service.service.name == "service1");

            // service-manager will let us think that the service is idle, and send us the queue to the forward-cache
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
            EXPECT_TRUE( service.process.ipc);
            EXPECT_TRUE( service.process.ipc != server.process().ipc);
         }
      }

      TEST( service_manager, service_lookup_service1__server_terminate__expect__service_error_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         common::Uuid correlation;

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
            
            correlation = service.correlation;
         }

         {
            common::message::event::process::Exit message;
            message.state.pid = server.process().pid;
            message.state.reason = common::process::lifetime::Exit::Reason::core;

            common::communication::ipc::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         }

         {
            // we expect to get a service-call-error-reply
            common::message::service::call::Reply message;
            
            common::communication::ipc::blocking::receive( 
               common::communication::ipc::inbound::device(),
               message);
            
            EXPECT_TRUE( message.code.result == decltype( message.code.result)::service_error);
            EXPECT_TRUE( message.correlation == correlation);
         }
      }

      TEST( service_manager, service_lookup__send_ack__expect_metric)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         auto start = common::platform::time::clock::type::now();
         auto end = start + std::chrono::milliseconds{ 2};

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }

         // subscribe to metric event
         common::message::event::service::Calls event;
         common::event::subscribe( common::process::handle(), { event.type()});

         // Send Ack
         {
            common::message::service::call::ACK message;
            message.metric.process = server.process();
            message.metric.service = "b";
            message.metric.parent = "a";
            message.metric.start = start;
            message.metric.end = end;
            message.metric.code = common::code::xatmi::service_fail;

            common::communication::ipc::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         }

         // wati for event
         {
            common::communication::ipc::blocking::receive( 
               common::communication::ipc::inbound::device(),
               event);
            
            ASSERT_TRUE( event.metrics.size() == 1);
            auto& metric = event.metrics.at( 0);
            EXPECT_TRUE( metric.service == "b");
            EXPECT_TRUE( metric.parent == "a");
            EXPECT_TRUE( metric.process == server.process());
            EXPECT_TRUE( metric.start == start);
            EXPECT_TRUE( metric.end == end);
            EXPECT_TRUE( metric.code == common::code::xatmi::service_fail);
         }
      }

   } // service
} // casual
