//!
//! casual_gateway_isolatedunittest_buffer.cpp
//!
//! Created on: 19 Oct 2013
//!     Author: dnulnets
//!


#include <gtest/gtest.h>

#include "gateway/wire.h"
#include "common/marshal.h"

#include <vector>
#include <limits>

namespace casual
{
   namespace common
   {

      namespace marshal
      {

         /*
          * Marshalling test structure
          */
         struct test {

            std::string name;
            unsigned long int value;
            std::vector<int> lista;

            /*
             * Marshaller
             */
            template<typename A>
            void marshal( A& archive)
            {
               archive & name;
               archive & value;
               archive & lista;
            }
         };

         /*
          * Test of a binary pattern
          */
         bool testPattern(const common::platform::binary_type& buffer, const int correct[])
         {
            int i = 0;
            bool b = true;
            for(const char& current : buffer)
            {
               b &= static_cast<const int>(*(reinterpret_cast<const unsigned char *>(&current))) == correct[i];
               i++;
            }
            return b;
         }

         /*
          * Test suite for the marshalling output
          */
         TEST( casual_gateway_marshal_output, nwunsignedlonglong)
         {
            output::NWBOBinary nwbo;
            const common::platform::binary_type &b = nwbo.get();
            const int pattern[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef};
            unsigned long int l = 0x1234567890abcdef;
            nwbo << l;
            if (b.size()==8)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==8);
         }

         TEST( casual_gateway_marshal_output, nwlonglong)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0xed, 0xcb, 0xa9, 0x87, 0x6f, 0x54, 0x32, 0x11};
            long int l = -0x1234567890abcdef;
            nwbo << l;
            if (b.size()==8)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==8);
         }

         TEST( casual_gateway_marshal_output, nwunsignedlong)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0x12, 0x34, 0x56, 0x78};

            int l = 0x12345678;
            nwbo << l;
            if (b.size()==4)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==4);
         }

         TEST( casual_gateway_marshal_output, nwlong)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0xed, 0xcb, 0xa9, 0x88};

            unsigned int l = -0x12345678;
            nwbo << l;
            if (b.size()==4)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==4);
         }

         TEST( casual_gateway_marshal_output, nwunsignedshortint)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0x12, 0x34};

            unsigned short int l = 0x1234;
            nwbo << l;
            if (b.size()==2)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==2);
         }

         TEST( casual_gateway_marshal_output, nwshortint)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0xed, 0xcc};

            short int l = -0x1234;
            nwbo << l;
            if (b.size()==2)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==2);
         }

         TEST( casual_gateway_marshal_output, nwunsignedchar)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0xed};

            unsigned char l = 0xed;
            nwbo << l;
            if (b.size()==1)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==1);
         }

         TEST( casual_gateway_marshal_output, nwsignedchar)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0xee};

            signed char l = -0x12;
            nwbo << l;
            if (b.size()==1)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==1);
         }

         TEST( casual_gateway_marshal_output, nwchar)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0xee};

            char l = -0x12;
            nwbo << l;
            if (b.size()==1)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==1);
         }

         TEST( casual_gateway_marshal_output, nwfloat)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0x3e,0xaa,0xaa,0xab}; /* IEEE 754 32 bit 0.33333333333 */

            float  f = 1.0/3.0;
            nwbo << f;
            if (b.size()==4)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==4);
         }

         TEST( casual_gateway_marshal_output, nwdouble)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0x3f,0xd5,0x55,0x55,0x55,0x55,0x55,0x55}; /* IEEE 754 64 bit 0.33333333333 */

            double  d = 1.0/3.0;
            nwbo << d;
            if (b.size()==8)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==8);
         }

         TEST( casual_gateway_marshal_output, nwstring)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0x00,0x05,0x74,0x6f,0x6d,0x61,0x73};
            std::string s = "tomas";
            nwbo << s;
            if (b.size()==7)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==7);
         }

         TEST( casual_gateway_marshal_output, nwvectorint)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0x00,0x02,0x00,0x00,0x00,0x19,0xff,0xff,0x22,0x2b};
            std::vector<int> v;
            v.push_back(25);
            v.push_back(-56789);
            nwbo << v;
            if (b.size()==10)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==10);
         }

         TEST( casual_gateway_marshal_output, nwvectorstring)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &b = nwbo.get();
            const int pattern[] = {0x00,0x03,0x00,0x05,0x74,0x6f,0x6d,0x61,0x73,0x00,0x05,0x70,
                  0x65,0x74,0x65,0x72,0x00,0x04,0x6c,0x61,0x72,0x73};
            std::vector<std::string> v;
            v.push_back("tomas");
            v.push_back("peter");
            v.push_back("lars");
            nwbo << v;
            if (b.size()==22)
               EXPECT_TRUE(testPattern(b, pattern));
            else
               EXPECT_TRUE(b.size()==22);
         }

         TEST( casual_gateway_marshal_output, nwvectorvectorint)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &buffer = nwbo.get();
            const int pattern[] = {0x00,0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x00,
                  0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x01,0x00,0x00,0x00,0x01};
            std::vector<std::vector<int>> v;
            std::vector<int> a, b, c;
            a.push_back(1);
            b.push_back(2);
            b.push_back(3);
            c.push_back(1);
            v.push_back(a);
            v.push_back(b);
            v.push_back(c);
            nwbo << v;
            if (buffer.size()==24)
               EXPECT_TRUE(testPattern(buffer, pattern));
            else
               EXPECT_TRUE(buffer.size()==24);
         }

         TEST( casual_gateway_marshal_output, nwstruct)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &buffer = nwbo.get();
            const int pattern[] = {0x00,0x05,0x74,0x6f,0x6d,0x61,0x73,0x12,0x34,0x56,0x78,0x90,0xab,
                  0xcd,0xef,0x00,0x01,0x00,0x00,0x00,0x08};
            struct test t;
            t.name = "tomas";
            t.value=0x1234567890abcdef;
            t.lista.push_back(8);
            nwbo << t;
            if (buffer.size()==21)
               EXPECT_TRUE(testPattern(buffer, pattern));
            else
               EXPECT_TRUE(buffer.size()==21);
         }

         TEST (casual_gateway_marshal_input, nwunsignedlonglong)
         {
            const common::platform::binary_type b{static_cast<char>(0x12), static_cast<char>(0x34), static_cast<char>(0x56), static_cast<char>(0x78),
                  static_cast<char>(0x90), static_cast<char>(0xab), static_cast<char>(0xcd), static_cast<char>(0xef)};
            input::NWBOBinary nwbo(b);

            unsigned long int l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==0x1234567890abcdef);
         }

         TEST (casual_gateway_marshal_input, nwlonglong)
         {
            const common::platform::binary_type b{static_cast<char>(0xed), static_cast<char>(0xcb), static_cast<char>(0xa9),
                  static_cast<char>(0x87), static_cast<char>(0x6f), static_cast<char>(0x54), static_cast<char>(0x32),
                  static_cast<char>(0x11)};
            input::NWBOBinary nwbo(b);

            long int l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==-0x1234567890abcdef);
         }

         TEST (casual_gateway_marshal_input, nwunsignedlong)
         {
            const common::platform::binary_type b{static_cast<char>(0x12), static_cast<char>(0x34), static_cast<char>(0x56),
               static_cast<char>(0x78)};
            input::NWBOBinary nwbo(b);

            unsigned int l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==0x12345678);
         }

         TEST (casual_gateway_marshal_input, nwlong)
         {
            const common::platform::binary_type b{static_cast<char>(0xed), static_cast<char>(0xcb), static_cast<char>(0xa9), static_cast<char>(0x88)};
            input::NWBOBinary nwbo(b);

            int l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==-0x12345678);
         }

         TEST (casual_gateway_marshal_input, nwunsignedshort)
         {
            const common::platform::binary_type b{static_cast<char>(0x12), static_cast<char>(0x34)};
            input::NWBOBinary nwbo(b);

            unsigned short int l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==0x1234);
         }

         TEST (casual_gateway_marshal_input, nwshort)
         {
            const common::platform::binary_type b{static_cast<char>(0xed), static_cast<char>(0xcc)};
            input::NWBOBinary nwbo(b);

            short int l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==-0x1234);
         }

         TEST (casual_gateway_marshal_input, nwunsignedchar)
         {
            const common::platform::binary_type b{static_cast<char>(0xed)};
            input::NWBOBinary nwbo(b);

            unsigned char l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==0xed);
         }

         TEST (casual_gateway_marshal_input, nwchar)
         {
            const common::platform::binary_type b{static_cast<char>(0xee)};
            input::NWBOBinary nwbo(b);

            char l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==-0x12);
         }

         TEST (casual_gateway_marshal_input, nwsignedchar)
         {
            const common::platform::binary_type b{static_cast<char>(0xee)};
            input::NWBOBinary nwbo(b);

            signed char l = 0;
            nwbo >> l;
            EXPECT_TRUE(l==-0x12);
         }

         TEST( casual_gateway_marshal_input, nwfloat)
         {
            /* IEEE 754 32 bit 0.33333333333 */
            const common::platform::binary_type b{static_cast<char>(0x3e),static_cast<char>(0xaa),static_cast<char>(0xaa),
               static_cast<char>(0xab)};
            input::NWBOBinary nwbo(b);

            float f = 0.0f;
            float fr = 1.0f/3.0f;
            nwbo >> f;
            EXPECT_TRUE(f==fr);
         }

         TEST( casual_gateway_marshal_input, nwdouble)
         {
            /* IEEE 754 32 bit 0.33333333333 */
            const common::platform::binary_type b{static_cast<char>(0x3f),static_cast<char>(0xd5),static_cast<char>(0x55),
               static_cast<char>(0x55),static_cast<char>(0x55),static_cast<char>(0x55),static_cast<char>(0x55),
               static_cast<char>(0x55)};
            input::NWBOBinary nwbo(b);

            double d = 0.0;
            double dr = 1.0/3.0;
            nwbo >> d;
            EXPECT_TRUE(d==dr);
         }

         TEST( casual_gateway_marshal_input, nwstring)
         {
            const common::platform::binary_type b{static_cast<char>(0x00),static_cast<char>(0x05),
               static_cast<char>(0x74),static_cast<char>(0x6f),static_cast<char>(0x6d),
               static_cast<char>(0x61),static_cast<char>(0x73)};
            input::NWBOBinary nwbo(b);
            std::string s;
            nwbo >> s;
            EXPECT_TRUE(s.compare("tomas")==0);
         }

         TEST( casual_gateway_marshal_input, nwvectorint)
         {
            const common::platform::binary_type b{static_cast<char>(0x00),static_cast<char>(0x02),static_cast<char>(0x00),
               static_cast<char>(0x00),static_cast<char>(0x00),static_cast<char>(0x19),static_cast<char>(0xff),
               static_cast<char>(0xff),static_cast<char>(0x22),static_cast<char>(0x2b)};
            input::NWBOBinary nwbo(b);
            std::vector<int> v;
            nwbo >> v;
            EXPECT_TRUE(v[0]==25);
            EXPECT_TRUE(v[1]==-56789);
            EXPECT_TRUE(v.size()==2);
         }

         TEST( casual_gateway_marshal_input, nwvectorstring)
         {
            const common::platform::binary_type b{static_cast<char>(0x00),static_cast<char>(0x03),static_cast<char>(0x00),
               static_cast<char>(0x05),static_cast<char>(0x74),static_cast<char>(0x6f),static_cast<char>(0x6d),
               static_cast<char>(0x61),static_cast<char>(0x73),static_cast<char>(0x00),static_cast<char>(0x05),
               static_cast<char>(0x70),static_cast<char>(0x65),static_cast<char>(0x74),static_cast<char>(0x65),
               static_cast<char>(0x72),static_cast<char>(0x00),static_cast<char>(0x04),static_cast<char>(0x6c),
               static_cast<char>(0x61),static_cast<char>(0x72),static_cast<char>(0x73)};
            input::NWBOBinary nwbo(b);
            std::vector<std::string> v;
            nwbo >> v;
            EXPECT_TRUE(v[0].compare("tomas")==0);
            EXPECT_TRUE(v[1].compare("peter")==0);
            EXPECT_TRUE(v[2].compare("lars")==0);
            EXPECT_TRUE(v.size()==3);
         }

         TEST( casual_gateway_marshal_input, nwvectorvectorint)
         {
            const common::platform::binary_type b{static_cast<char>(0x00),static_cast<char>(0x03),static_cast<char>(0x00),
               static_cast<char>(0x01),static_cast<char>(0x00),static_cast<char>(0x00),static_cast<char>(0x00),
               static_cast<char>(0x01),static_cast<char>(0x00),static_cast<char>(0x02),static_cast<char>(0x00),
               static_cast<char>(0x00),static_cast<char>(0x00),static_cast<char>(0x02),static_cast<char>(0x00),
               static_cast<char>(0x00),static_cast<char>(0x00),static_cast<char>(0x03),static_cast<char>(0x00),
               static_cast<char>(0x01),static_cast<char>(0x00),static_cast<char>(0x00),static_cast<char>(0x00),
               static_cast<char>(0x01)};
            input::NWBOBinary nwbo(b);
            std::vector<std::vector<int>> v;
            nwbo >> v;
            EXPECT_TRUE(v[0][0]==1);
            EXPECT_TRUE(v[1][0]==2);
            EXPECT_TRUE(v[1][1]==3);
            EXPECT_TRUE(v[2][0]==1);
            EXPECT_TRUE(v.size()==3);
            EXPECT_TRUE(v[0].size()==1);
            EXPECT_TRUE(v[1].size()==2);
            EXPECT_TRUE(v[2].size()==1);
         }

         TEST( casual_gateway_marshal_input, nwstruct)
         {
            const common::platform::binary_type b{static_cast<char>(0x00),static_cast<char>(0x05),static_cast<char>(0x74),
               static_cast<char>(0x6f),static_cast<char>(0x6d),static_cast<char>(0x61),static_cast<char>(0x73),
               static_cast<char>(0x12),static_cast<char>(0x34),static_cast<char>(0x56),static_cast<char>(0x78),
               static_cast<char>(0x90),static_cast<char>(0xab),static_cast<char>(0xcd),static_cast<char>(0xef),
               static_cast<char>(0x00),static_cast<char>(0x01),static_cast<char>(0x00),static_cast<char>(0x00),
               static_cast<char>(0x00),static_cast<char>(0x08)};
            input::NWBOBinary nwbo(b);
            struct test t;
            nwbo >> t;
            EXPECT_TRUE(t.name.compare("tomas")==0);
            EXPECT_TRUE(t.value=0x1234567890abcdef);
            EXPECT_TRUE(t.lista.size()==1);
            EXPECT_TRUE(t.lista[0]==8);
         }

         TEST( casual_gateway_marshal_output, nwbinary)
         {
            output::NWBOBinary nwbo;
            const output::NWBOBinary::buffer_type &buffer = nwbo.get();
            common::platform::binary_type bt;
            int pattern[264];
            for (int i = 0; i<256; i++) {
               bt.push_back(i);
               pattern[i+8]=i;
            }
            pattern[0]=pattern[1]=pattern[2]=pattern[3]=0;
            pattern[4]=pattern[5]=pattern[7]=0;
            pattern[6]=1;
            nwbo << bt;
            if (buffer.size()==264)
               EXPECT_TRUE(testPattern(buffer, pattern));
            else
               EXPECT_TRUE(buffer.size()==264);
         }

         TEST( casual_gateway_marshal_input, nwbinary)
         {
            common::platform::binary_type b;
            b.push_back(0);
            b.push_back(0);
            b.push_back(0);
            b.push_back(0);
            b.push_back(0);
            b.push_back(0);
            b.push_back(1);
            b.push_back(0);
            for (int i = 0; i<256; i++) {
               b.push_back(i+8);
            }
            input::NWBOBinary nwbo(b);
            common::platform::binary_type t;
            nwbo >> t;
            EXPECT_TRUE(t.size()==256);
         }

         TEST( casual_gateway_marshal, nwstruct)
         {
            output::NWBOBinary nwbo;
            struct test t, u;
            t.name = "tomas";
            t.value=0x1234567890abcdef;
            t.lista.push_back(-8);
            nwbo << t;
            input::NWBOBinary nwbi(nwbo.get());
            nwbi >> u;
            EXPECT_TRUE(t.name.compare (u.name)==0);
            EXPECT_TRUE(t.value == u.value);
            EXPECT_TRUE(u.lista.size()==1);
            EXPECT_TRUE(t.lista[0] == u.lista[0]);
         }

         TEST( casual_gateway_marshal, floatsize)
         {
            /*
             * We assume that float and double is 4 and 8 bytes
             */
            EXPECT_TRUE(sizeof(float)==4);
            EXPECT_TRUE(sizeof(double)==8);
         }

         TEST( casual_gateway_marshal, charsize)
         {
            /*
             * This is actually enforced by the C++ standard
             */
            EXPECT_TRUE(sizeof(char)==1);
         }

         TEST(casual_gateway_marshal, limits)
         {
            /*
             * Range check for C++ standard so it hasn't changed
             */
            EXPECT_TRUE(std::numeric_limits<int>::min() == -2147483648);
            EXPECT_TRUE(std::numeric_limits<int>::max() == 2147483647);
            EXPECT_TRUE(std::numeric_limits<short int>::min() == -32768);
            EXPECT_TRUE(std::numeric_limits<short int>::max() == 32767);
            EXPECT_TRUE(std::numeric_limits<long int>::min() == -9223372036854775808L);
            EXPECT_TRUE(std::numeric_limits<long int>::max() == 9223372036854775807L);

         }
      }
	}
}



