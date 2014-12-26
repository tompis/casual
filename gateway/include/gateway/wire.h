//!
//! casual_presentation_layer.h
//!
//! Created on: Okt 11, 2013
//!     Author: dnulnets
//!

#ifndef CASUAL_PRESENTATION_LAYER_H_
#define CASUAL_PRESENTATION_LAYER_H_

/*
 * Casual
 */
#include "common/ipc.h"
#include "common/platform.h"
#include "common/uuid.h"
#include "common/marshal/marshal.h"
/*
 * STL
 */
#include <iostream>
#include <iomanip>
#include <vector>
#include <cassert>

/*
 * Network type and function definitions
 */
#include <inttypes.h>
#include <netinet/in.h>

/* 64 bit hton functions not available on all platforms. */
#define _ntohll casual::common::marshal::__htonll
#define _htonll casual::common::marshal::__htonll

/*
 * The network binary buffer
 */
namespace casual
{
   namespace common
   {
      namespace marshal
      {
         /*
          * The 64 bit hton, ntoh experimental. Not all platforms have these
          * functions. So we better use our own.
          *
          * TODO: How to check if they have it or not? We should use platform
          * specific version instead of our own.
          */
         uint64_t __htonll(uint64_t value);
         uint64_t __htonll2(uint64_t value);

         /*
          * Output namespace
          */
         namespace output
         {
            /*
             * A buffer that handles network byte order. TCP/IP  always goes in big endian so we stick to that regardless of
             * local endian.
             */
            struct NWBOBinary
            {
               /*
                * Buffer type
                */
               typedef common::platform::binary_type buffer_type;

               /*
                * Standard operators
                */
               NWBOBinary() = default;
               NWBOBinary( NWBOBinary&&) = default;
               NWBOBinary( const NWBOBinary&) = delete;
               NWBOBinary& operator = ( const NWBOBinary&) = delete;

               /*
                * Releases the buffer
                */
               buffer_type release()
               {
                  return std::move( m_buffer);
               }


               /*
                * Getter for the buffer
                */
               const buffer_type& get() const
               {
                  return m_buffer;
               }

               /*
                * Add operator, same as shift
                */
               template< typename T>
               NWBOBinary& operator & ( T& value)
               {
                  return *this << value;
               }

               /*
                * Shift operator
                */
               template< typename T>
               NWBOBinary& operator << ( T& value)
               {
                  write( value);
                  return *this;
               }

               /*
                * Dumps the binary buffer as a C++ hexadecimal array to standard out.
                */
               void dump();

            private:

               //
               // Be friend with free marshal function so we can use more
               // bare-bone stuff when we do non-intrusive marshal for third-party types
               //
               template< typename T, typename M>
               friend void casual_marshal_value( T& value, M& marshler);

               /*
                * Marshal complex types, use their own marshaling function.
                *
                * This will get called for structs and floats as well and they probably do not have any marshaling
                * function so they will probably give a compiler error. But that is ok, becasue I do not know how to convert
                * them between CPU-architectures, only themselves know that.
                *
                */
               template< typename T>
               typename std::enable_if< ! std::is_arithmetic< T>::value>::type
               write( T& value)
               {
                  casual::casual_marshal_value( value, *this);
               }

               /*
                * Marshal simple types, do it yourself
                */
               template< typename T>
               typename std::enable_if< std::is_arithmetic< T>::value>::type
               write( T& value)
               {
                  writeIntegral( value);
               }

               /*
                * Internally used network type emitter for U8
                */
               void writeIntegralU8(uint8_t value);

               /*
                * Internally used network type emitter for I8
                */
               void writeIntegral8(int8_t value);

               /*
                * Internally used network type emitter for U16
                */
               void writeIntegralU16(uint16_t value);

               /*
                * Internally used network type emitter for I16
                */
               void writeIntegral16(int16_t value);

               /*
                * Internally used network type emitter for U32
                */
               void writeIntegralU32(uint32_t value);

               /*
                * Internally used network type emitter for I32
                */
               void writeIntegral32(int32_t value);

               /*
                * Internally used network type emitter for U64
                */
               void writeIntegral64(uint64_t value);

               /*
                * Internally used network type emitter for I64
                */
               void writeIntegralU64(int64_t value);

               /*
                * Theese writeIntegrals are to decide exactly how to handle each C++ type, i.e. to what network type
                * the C++ type is converted to.
                */

               /*
                * usinged char to uint8
                */
               void inline writeIntegral(unsigned char& ui)
               {
                  writeIntegralU8(static_cast<uint8_t>(ui));
               }

               /*
                * char to uint8
                */
               void inline writeIntegral(signed char& ui)
               {
                  writeIntegral8(static_cast<int8_t>(ui));
               }

               /*
                * char to uint8
                */
               void inline writeIntegral(char& ui)
               {
                  writeIntegralU8(static_cast<uint8_t>(ui));
               }

               /*
                * unsigned short int to uint16
                */
               void inline writeIntegral(unsigned short int& ui)
               {
                  writeIntegralU16 (static_cast<uint16_t>(ui));
               }

               /*
                * short int to int16
                */
               void inline writeIntegral(short int& ui)
               {
                  writeIntegral16 (static_cast<int16_t>(ui));
               }

               /*
                * unsigned int to uint32
                */
               void inline writeIntegral(unsigned int& ui)
               {
                  writeIntegralU32 (static_cast<uint32_t>(ui));
               }

               /*
                * int to int32
                */
               void inline writeIntegral(int& ui)
               {
                  writeIntegral32 (static_cast<int32_t>(ui));
               }

               /*
                * unsigned long int to uint64
                */
               void inline writeIntegral(unsigned long int& ui)
               {
                  writeIntegralU64 (static_cast<uint64_t>(ui));
               }

               /*
                * long int to int64
                */
               void inline writeIntegral(long int& ui)
               {
                  writeIntegral64 (static_cast<int64_t>(ui));
               }

               /*
                * float, we assume IEEE 754 and that float is 32 bits
                */
               void inline writeIntegral(float &f)
               {
                  writeIntegralU32(static_cast<uint32_t>(*(reinterpret_cast<unsigned uint32_t *>(&f))));
               }

               /*
                * double, we assume IEEE 754 and that double is 64 bits
                */
               void inline writeIntegral(double &d)
               {
                  writeIntegralU64(static_cast<uint64_t>(*(reinterpret_cast<unsigned uint64_t *>(&d))));
               }

               /*
                * Write a vector of values, start with number of values in the vector followed by all elements.
                *
                * We assume that the number of elements will always fit in 16 bits.
                */
               template< typename T>
               void write(std::vector< T>& value)
               {
                  writeIntegralU16( static_cast<uint16_t>(value.size()));

                  for( auto& current : value)
                  {
                     *this << current;
                  }
               }

               /*
                * Write string as the number of characters followed by all the characters.
                *
                * We assume that the length of the string will always fit in 16 bits.
                *
                */
               void write( std::string& value);

               /*
                * Write a binary buffer as the number of bytes followed by all the bytes.
                *
                * We assume that the length of the binary buffer will always fit in 64 bits.
                *
                */
               void write( common::platform::binary_type& value);

               /*
                * The actual binary buffer
                */
               buffer_type m_buffer;
            };

         } // output

         namespace input
         {

            struct NWBOBinary
            {
               /*
                * Type definitions
                */
               typedef common::platform::binary_type buffer_type;
               typedef buffer_type::size_type offest_type;

               /*
                * Standard functions
                */
               NWBOBinary() = default;
               NWBOBinary( ipc::message::Complete&& message)
                  : m_buffer( std::move( message.payload))
               {
               }
               NWBOBinary( buffer_type payload) : m_buffer(std::move (payload))
               {
               }
               NWBOBinary( NWBOBinary&&) = default;
               NWBOBinary( const NWBOBinary&) = delete;
               NWBOBinary& operator = ( const NWBOBinary&) = delete;

               /*
                * Get next operator
                */
               template< typename T>
               NWBOBinary& operator & ( T& value)
               {
                  return *this >> value;
               }

               /*
                * Shift operator
                */
               template< typename T>
               NWBOBinary& operator >> ( T& value)
               {
                  read( value);
                  return *this;
               }


            private:

               /*
                * Unmarshalling for non pod:s
                */
               template< typename T>
               typename std::enable_if< ! std::is_arithmetic< T>::value>::type
               read( T& value)
               {
                  casual::casual_unmarshal_value( value, *this);
               }

               /*
                * Unmarshalling for integral datattypes
                */
               template< typename T>
               typename std::enable_if< std::is_arithmetic< T>::value>::type
               read( T& value)
               {
                  readIntegral( value);
               }

               /*
                * Unmarshaling for a vector. The first two bytes (16 bits) are the length of the vector and all the elements
                * will follow.
                *
                */
               template< typename T>
               void read( std::vector< T>& value)
               {
                  uint16_t size;
                  *this >> size;
                  value.resize(size); /* I know, not good should be size_type */

                  for( auto& current : value)
                  {
                     *this >> current;
                  }
               }

               /*
                * Unmarshaling for a string. The first two bytes (16bit) are the length of the string followed by the characters.
                */
               void read( std::string& value);

               /*
                * Unmarshaling of a binary array. The first eight bytes (64 bits) are the length of the binary blob.
                */
               void read( common::platform::binary_type& value);

               /*
                * Read an unsigned 8 bit integer.
                */
               void readIntegralU8 (uint8_t& value);

               /*
                * Read a signed 8 bit integer.
                */
               void readIntegral8 (int8_t& value);

               /*
                * Read a unsigned 16 bit integer.
                */
               void readIntegralU16 (uint16_t& value);

               /*
                * Read a signed 16 bit integer.
                */
               void readIntegral16 (int16_t& value);

               /*
                * Read a unsigned 32 bit integer.
                */
               void readIntegralU32 (uint32_t& value);

               /*
                * Read a unsigned 32 bit integer.
                */
               void readIntegral32 (int32_t& value);

               /*
                * Read a unsigned 64 bit integer.
                */
               void readIntegralU64 (uint64_t& value);

               /*
                * Read a unsigned 64 bit integer.
                */
               void readIntegral64 (int64_t& value);

               /*
                * Read an unsigned char as uint8
                */
               void inline readIntegral (unsigned char & ch)
               {
                  readIntegralU8(static_cast<uint8_t&>(ch));
               }

               /*
                * Read an char as uint8
                */
               void inline readIntegral (char & ch)
               {
                  readIntegralU8(reinterpret_cast<uint8_t&>(ch));
               }

               /*
                * Read an signed char as int8
                */
               void inline readIntegral (signed char & ch)
               {
                  readIntegral8 (static_cast<int8_t&>(ch));
               }

               /*
                * Read an unsigned short int as uint16
                */
               void inline readIntegral (unsigned short int & s)
               {
                  uint16_t u = static_cast<uint16_t>(s);
                  readIntegralU16(u);
                  s = static_cast<unsigned short int>(u);
               }

               /*
                * Read an signed short int as int16
                */
               void inline readIntegral (short int & s)
               {
                  int16_t i = static_cast<int16_t>(s);
                  readIntegral16 (i);
                  s = static_cast<short int>(i);
               }

               /*
                * Read an unsigned int as uint32
                */
               void inline readIntegral (unsigned int & i)
               {
                  uint32_t u = static_cast<uint32_t>(i);
                  readIntegralU32(u);
                  i = static_cast<unsigned int>(u);
               }

               /*
                * Read an signed int as int32
                */
               void inline readIntegral (int & i)
               {
                  int32_t n = static_cast<int32_t>(i);
                  readIntegral32 (n);
                  i = static_cast<int32_t>(n);
               }

               /*
                * Read an unsigned long int as uint64
                */
               void inline readIntegral (unsigned long int & l)
               {
                  uint64_t t = static_cast<uint64_t>(l);
                  readIntegralU64(t);
                  l = static_cast<unsigned long int>(t);
               }

               /*
                * Read an unsigned long int as uint64
                */
               void inline readIntegral (long int & l)
               {
                  int64_t t = static_cast<int64_t>(l);
                  readIntegral64 (t);
                  l = static_cast<long int>(t);
               }

               /*
                * Read a float as a IEEE754 32 bit
                */
               void inline readIntegral(float & f)
               {
                  readIntegralU32(*(reinterpret_cast<uint32_t*>(&f)));
               }

               /*
                * Read a double as a IEEE754 64 bit
                */
               void inline readIntegral(double & d)
               {
                  readIntegralU64(*(reinterpret_cast<uint64_t*>(&d)));
               }

               /*
                * The binary buffer
                */
               buffer_type m_buffer;

               /*
                * Offset within the buffer we are reading
                */
               offest_type m_offset = 0;

            };

         } // output
      } // marshal
   } // common
} // casual

#endif /* CASUAL_PRESENTATION_LAYER_H_ */
