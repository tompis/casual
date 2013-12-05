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

         /* Test handler */
         class IPSERVERHandler1 : public SocketEventHandler {

            /* A socket */
            std::shared_ptr<Socket> pS;

         public:

            /* Only POLLIN events */
            int events () const { return POLLIN; };

            /* Handle incoming connections */
            int dataCanBeRead (int events, Socket &socket)
            {
               common::logger::information << "SERVER: Incoming connection";
               pS = socket.accept();
               if (pS==0L) {
                  common::logger::information << "SERVER: Incoming connection not accepted";
               } else {
                  common::logger::information << "SERVER: Incoming connection accepted";
               }
            }

            /* Cleanup */
            ~IPSERVERHandler1()
            {
               EXPECT_TRUE(pS != 0L);
               pS->close();
            }
         };

         class IPCLIENTHandler1 : public SocketEventHandler {

         private:

            int client_connected = 0;

         public:

            /* Only POLLIN events */
            int events () const { return POLLOUT; };

            /* Handle write is possible */
            int dataCanBeWritten (int events, Socket &socket)
            {
               common::logger::information << "CLIENT: Connected";
               client_connected = 1;
            }

            /* Destructor */
            ~IPCLIENTHandler1()
            {
               EXPECT_TRUE (client_connected == 1);
            }
         };

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

         TEST( casual_gateway_ipc_socket, ipc_bind_ipv4)
         {
            common::logger::information << "TEST : casual_gateway_ipc_socket.ipc_bind_ipv4";

            Resolver resolver("127.0.0.1:11234");

            /* Loopback interface */
            int n = resolver.get().size();
            if (n>0) {
               Endpoint p = *resolver.begin();
               Socket s(p);
               n = s.bind();
               EXPECT_TRUE(n==0);
            } else
               EXPECT_TRUE(false);
         }

         TEST( casual_gateway_ipc_socket, ipc_bind_ipv6)
         {
            common::logger::information << "TEST : casual_gateway_ipc_socket.ipc_bind_ipv6";

            Resolver resolver("::1:11234");

            /* IPv6 loopback */
            int n = resolver.get().size();
            if (n>0) {
               Endpoint p = *resolver.begin();
               Socket s(p);
               n = s.bind();
               EXPECT_TRUE(n==0);
            } else {
               EXPECT_TRUE(false);
            }
         }

         TEST( casual_gateway_ipc_socket, ipc_connect_ipv4thread)
         {
            common::logger::information << "TEST : casual_gateway_ipc_socket.ipc_connect_ipv4thread";

            std::unique_ptr<SocketEventHandler> pServerHandler = std::unique_ptr<SocketEventHandler>(new IPSERVERHandler1 ());
            std::unique_ptr<SocketEventHandler> pClientHandler = std::unique_ptr<SocketEventHandler>(new IPCLIENTHandler1 ());

            Resolver resolver ("127.0.0.1:11234");
            int resolved = resolver.get().size();
            if (resolved>0) {

               /* Initialize the testbed */
               Endpoint p = *resolver.begin();
               Socket source(p);
               source.setEventHandler (pServerHandler);
               common::logger::information << "SERVER: Server socket bound";
               int bound = source.bind();
               EXPECT_TRUE(bound==0);
               if (bound==0) {
                  common::logger::information << "SERVER: Server socket listening";
                  int listen = source.listen();
                  EXPECT_TRUE(listen==0);

                  if (listen==0) {

                     /* Start the client */
                     std::thread t([&](){
                        Socket sink(p);
                        sink.setEventHandler (pClientHandler);
                        common::logger::information << "CLIENT: Waiting to connect client";
                        std::chrono::milliseconds dura( 100 );
                        std::this_thread::sleep_for( dura );
                        int connected = sink.connect();
                        EXPECT_TRUE(errno==EINPROGRESS);
                        int x = sink.poll (1000);
                        EXPECT_TRUE(x==1);
                        common::logger::information << "CLIENT: Waiting to close client";
                        std::this_thread::sleep_for( dura );
                        sink.close();
                        common::logger::information << "CLIENT: Client closed";
                     });

                     /* Accept incoming call */
                     int x = source.poll (1000);

                     /* Close the testbed */
                     t.join();
                  }
               }

               /* Close the testbed */
               source.close();
            } else
               EXPECT_TRUE(false);
         }

         TEST( casual_gateway_ipc_socket, ipc_connect_ipv6thread)
         {
            common::logger::information << "TEST : casual_gateway_ipc_socket.ipc_connect_ipv6thread";

            std::unique_ptr<SocketEventHandler> pServerHandler = std::unique_ptr<SocketEventHandler>(new IPSERVERHandler1 ());
            std::unique_ptr<SocketEventHandler> pClientHandler = std::unique_ptr<SocketEventHandler>(new IPCLIENTHandler1 ());

            Resolver resolver ("::1:11234");
            int resolved = resolver.get().size();
            if (resolved>0) {

               /* Initialize the testbed */
               Endpoint p = *resolver.begin();
               Socket source(p);
               source.setEventHandler (pServerHandler);
               common::logger::information << "SERVER: Server socket bound";
               int bound = source.bind();
               EXPECT_TRUE(bound==0);
               if (bound==0) {
                  common::logger::information << "SERVER: Server socket listening";
                  int listen = source.listen();
                  EXPECT_TRUE(listen==0);

                  if (listen==0) {

                     /* Start the client */
                     std::thread t([&](){
                        Socket sink(p);
                        sink.setEventHandler (pClientHandler);
                        common::logger::information << "CLIENT: Waiting to connect client";
                        std::chrono::milliseconds dura( 100 );
                        std::this_thread::sleep_for( dura );
                        int connected = sink.connect();
                        EXPECT_TRUE(errno==EINPROGRESS);
                        int x = sink.poll (1000);
                        EXPECT_TRUE(x==1);
                        common::logger::information << "CLIENT: Waiting to close client";
                        std::this_thread::sleep_for( dura );
                        sink.close();
                        common::logger::information << "CLIENT: Client closed";
                     });

                     /* Accept incoming call */
                     int x = source.poll (1000);

                     /* Close the testbed */
                     t.join();
                  }
               }

               /* Close the testbed */
               source.close();
            } else
               EXPECT_TRUE(false);
         }

      }
   }
}
