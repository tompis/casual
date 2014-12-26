/*
 * test_ipc.cpp
 *
 *  Created on: 24 nov 2013
 *      Author: tomas
 */
#include <gtest/gtest.h>

#include <sys/types.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>

/*
 * STL
 */
#include <cstring>
#include <string>
#include <utility>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <memory>

/*
 * Casual common
 */
#include "common/logger.h"
#include "gateway/ipc.h"

/*
 * Casual namespace
 */
namespace casual
{
   namespace common
   {
      namespace ipc
      {

         TEST( casual_gateway_ipc_socket, ipc_resolve)
         {
            common::logger::information << "TEST : casual_gateway_ipc_socket.ipc_resolve";
            Resolver resolver("127.0.0.1:80");

            /* Loopback interface */
            int n = resolver.get().size();
            if (n>0) {
               Endpoint p = *resolver.begin();
               std::string s = p.info();
               EXPECT_TRUE(s == "127.0.0.1:80");
            }
            EXPECT_TRUE(n==1);
         }

         TEST( casual_gateway_ipc_socket, ipc_resolve_ipv6)
         {
            common::logger::information << "TEST : casual_gateway_ipc_socket.ipc_resolve_ipv6";
            Resolver resolver ("::1:80");

            /* IPv6 loopback */
            int n = resolver.get().size();
            if (n>0) {
               Endpoint p = *resolver.begin();
               std::string s = p.info();
               EXPECT_TRUE(s == "::1:80");
            }
            EXPECT_TRUE(n>0);
         }

         TEST( casual_gateway_ipc_socket, ipc_resolve_service)
         {
            common::logger::information << "TEST : casual_gateway_ipc_socket.ipc_resolve_service";
            Resolver resolver("127.0.0.1:tcpmux");

            /* Loopback interface and tcpmux service */
            int n = resolver.get().size();
            if (n>0) {
               Endpoint p = *resolver.begin();
               std::string s = p.info();
               EXPECT_TRUE(s == "127.0.0.1:1");
            }
            EXPECT_TRUE(n==1);
         }

         TEST( casual_gateway_ipc_socket, ipc_resolve_external)
         {
            common::logger::information << "TEST : casual_gateway_ipc_socket.ipc_resolve_external";
            Resolver resolver("www.google.com:80");

            /* external address */
            int n = resolver.get().size();
            EXPECT_TRUE(n>0);
         }

      }
   }
}
