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

/*
 * STL
 */
#include <cstring>
#include <string>
#include <utility>
#include <thread>
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
            Resolver resolver;

            /* Loopback interface */
            int n = resolver.resolve ("127.0.0.1:10000");
            EXPECT_TRUE(n==1);
         }

         TEST( casual_gateway_ipc_socket, ipc_resolve_service)
         {
            Resolver resolver;

            /* Loopback interface and tcpmux service */
            int n = resolver.resolve ("127.0.0.1:tcpmux");
            EXPECT_TRUE(n==1);
         }

         TEST( casual_gateway_ipc_socket, ipc_resolve_external)
         {
            Resolver resolver;

            /* external address */
            int n = resolver.resolve ("www.google.com:10000");
            EXPECT_TRUE(n>0);
         }

      }
   }
}
