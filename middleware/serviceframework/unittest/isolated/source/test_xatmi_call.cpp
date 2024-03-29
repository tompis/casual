//!
//! test_xatmi_call.cpp
//!
//! Created on: Jun 2, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "sf/xatmi_call.h"

#include "common/internal/log.h"
#include "common/internal/trace.h"

#include "../include/test_vo.h"

namespace casual
{
   namespace sf
   {
      namespace xatmi
      {
         namespace mockup
         {
            //namespace
            //{
               struct xatmi_call_mirror
               {
                  void operator() ( const std::string& service, buffer::Buffer& input, buffer::Buffer& output, long flags) const
                  {
                     common::trace::internal::Scope trace{ "service::call"};

                     common::log::internal::debug << "input: " << input << std::endl;
                     output.reset( input.release());
                     common::log::internal::debug << "output: " << output << std::endl;
                  }
               };

               struct xatmi_send_receive_mirror
               {
                  service::call_descriptor_type operator() ( const std::string& service, buffer::Buffer& input, long flags)
                  {
                     common::trace::internal::Scope trace{ "service::send"};

                     static int cd = 10;

                     common::log::internal::debug << "input: " << input << std::endl;

                     m_holder.emplace( ++cd, buffer::copy( input));

                     return cd;
                  }

                  bool operator() ( service::call_descriptor_type& cd, buffer::Buffer& output, long flags)
                  {
                     common::trace::internal::Scope trace{ "service::receive"};

                     auto found = m_holder.find( cd);

                     if( found != std::end( m_holder))
                     {
                        output.swap( found->second);
                        m_holder.erase( found);

                        common::log::internal::debug << "output: " << output << std::endl;

                        return true;
                     }

                     return false;
                  }

                  void cancel( service::call_descriptor_type cd)
                  {

                  }

               private:
                  typedef std::map< int, buffer::Buffer> buffer_holder;
                  static buffer_holder m_holder;
               };

               xatmi_send_receive_mirror::buffer_holder xatmi_send_receive_mirror::m_holder;
               //buffer::Base* xatmi_send_receive_mirror::m_inputBuffer = nullptr;

               typedef service::async::basic_call< service::policy::Binary, xatmi_send_receive_mirror> Async;

               typedef service::sync::basic_call< service::policy::Binary, mockup::xatmi_call_mirror> Sync;

            //} // <unnamed>
         } // mockup


         TEST( casual_sf_xatmi_call, sync_binary_mockup_mirror)
         {

            mockup::Sync service( "someService");


            test::SimpleVO input;
            service << CASUAL_MAKE_NVP( input);

            auto reply = service();
            test::SimpleVO output;

            reply >> CASUAL_MAKE_NVP( output);


            EXPECT_TRUE( input.m_long == output.m_long);

         }

         TEST( casual_sf_xatmi_call, async_blocking_binary_xatmi_send_receive_mirror)
         {
            mockup::Async service( "someService");


            test::SimpleVO input;
            input.m_long = 42;
            service << CASUAL_MAKE_NVP( input);

            // async send
            auto receiver = service();

            // async recive
            auto reply = receiver();
            test::SimpleVO output;

            reply >> CASUAL_MAKE_NVP( output);

            EXPECT_TRUE( input.m_long == output.m_long);

         }

         TEST( casual_sf_xatmi_call, async_non_blocking_binary_xatmi_send_receive_mirror)
         {
            mockup::Async service( "someService");


            test::SimpleVO input;
            input.m_long = 42;
            service << CASUAL_MAKE_NVP( input);

            // async send
            auto receiver = service();

            // async no-block recive
            auto reply = receiver.receive();

            if( ! reply.empty())
            {
               test::SimpleVO output;

               reply.front() >> CASUAL_MAKE_NVP( output);

               EXPECT_TRUE( input.m_long == output.m_long);
            }

         }


      }
   }
}

