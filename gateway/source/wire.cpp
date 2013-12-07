/*
 * wire.cpp
 *
 *  Created on: 12 okt 2013
 *      Author: dnulnets
 */

/*
 * Casual
 */
#include "common/ipc.h"
#include "common/types.h"
#include "common/uuid.h"

#include "gateway/wire.h"

/*
 * STL
 */
#include <vector>
#include <cassert>

/*
 * Network type and function definitions
 */
#include <inttypes.h>
#include <netinet/in.h>

/*
 * Not all platforms have a swap for 64 bit words. This is internal!!!!
 */
#define SWAP64(x) ((uint64_t)(\
   (((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) | \
   (((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) | \
   (((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) | \
   (((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) | \
   (((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) | \
   (((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) | \
   (((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) | \
   (((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56)))

/*
 * Casual namespace
 */
namespace casual
{
   namespace common
   {
      namespace marshal
      {

         /*
          * Host to network for 64 bit integers (version 1)
          */
         uint64_t __htonll(uint64_t value)
         {
             static const int num = 69; // My birthyear

             // Check the endianness
             if (*reinterpret_cast<const char*>(&num) == num)
             {
                 return (SWAP64(value));
             } else
             {
                 return value;
             }
         }

         /*
          * Host to network for 64 bit integers (version 2)
          */
         uint64_t __htonll2(uint64_t value)
         {
             static const int num = 69; // My birthyear

             // Check the endianness
             if (*reinterpret_cast<const char*>(&num) == num)
             {
                /* Little endian, need to make it big endian */
                 const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
                 const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

                 return (static_cast<uint64_t>(low_part) << 32) | high_part;
             } else
             {
                 return value;
             }
         }

         /*
          * The output namespace
          */
         namespace output {

            /*
             * Dumps the binary buffer to standard out as hexadecimal a C++ array.
             */
            void NWBOBinary::dump()
            {
               int i = 0;
               std::cout << std::endl << "buffer={";
               for(char& current : m_buffer)
               {
                  i++;
                  std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(*(reinterpret_cast<unsigned char *>(&current)));
                  if (i!=m_buffer.size())
                     std::cout << ",";
               }
               std::cout << "}" << std::endl;
            }

            /*
             * Internally used network type emitter for U8
             */
            void NWBOBinary::writeIntegralU8(uint8_t value)
            {
               const auto size = m_buffer.size();
               m_buffer.resize( size + 1);
               memcpy(&m_buffer[size], &value, 1);
            }

            /*
             * Internally used network type emitter for I8
             */
            void NWBOBinary::writeIntegral8(int8_t value)
            {
               const auto size = m_buffer.size();
               m_buffer.resize( size + 1);
               memcpy(&m_buffer[size], &value, 1);
            }

            /*
             * Internally used network type emitter for U16
             */
            void NWBOBinary::writeIntegralU16(uint16_t value)
            {
               const auto size = m_buffer.size();
               m_buffer.resize( size + 2);
               uint16_t u = htons (value);
               memcpy(&m_buffer[size], &u, 2);
            }

            /*
             * Internally used network type emitter for I16
             */
            void NWBOBinary::writeIntegral16(int16_t value)
            {
               const auto size = m_buffer.size();
               m_buffer.resize( size + 2);
               uint16_t u = htons (*(reinterpret_cast<uint16_t *>(&value)));
               memcpy(&m_buffer[size], &u, 2);
            }

            /*
             * Internally used network type emitter for U32
             */
            void NWBOBinary::writeIntegralU32(uint32_t value)
            {
               const auto size = m_buffer.size();
               m_buffer.resize( size + 4);
               uint32_t u = htonl (value);
               memcpy(&m_buffer[size], &u, 4);
            }

            /*
             * Internally used network type emitter for I32
             */
            void NWBOBinary::writeIntegral32(int32_t value)
            {
               const auto size = m_buffer.size();
               m_buffer.resize( size + 4);
               uint32_t u = htonl (*(reinterpret_cast<uint32_t *>(&value)));
               memcpy(&m_buffer[size], &u, 4);
            }

            /*
             * Internally used network type emitter for U64
             */
            void NWBOBinary::writeIntegral64(uint64_t value)
            {
               const auto size = m_buffer.size();
               m_buffer.resize( size + 8);
               uint64_t u = _htonll (value);
               memcpy(&m_buffer[size], &u, 8);
            }

            /*
             * Internally used network type emitter for I64
             */
            void NWBOBinary::writeIntegralU64(int64_t value)
            {
               const auto size = m_buffer.size();
               m_buffer.resize( size + 8);
               uint64_t u = _htonll (*(reinterpret_cast<uint64_t *>(&value)));
               memcpy(&m_buffer[size], &u, 8);
            }

            /*
             * Write string as the number of characters followed by all the characters.
             *
             * We assume that the length of the string will always fit in 16 bits.
             *
             */
            void NWBOBinary::write( std::string& value)
            {
               writeIntegralU16( static_cast<unsigned short int>(value.size()));
               const auto size = m_buffer.size();

               m_buffer.resize( size + value.size());

               memcpy( &m_buffer[ size], value.c_str(), value.size());
            }

            /*
             * Write a binary buffer as the number of bytes followed by all the bytes.
             *
             * We assume that the length of the binary buffer will always fit in 64 bits.
             *
             */
            void NWBOBinary::write( common::binary_type& value)
            {
               writeIntegralU64( static_cast<unsigned long int>(value.size()));

               m_buffer.insert( std::end( m_buffer), std::begin( value), std::end( value));
            }

         }

         /*
          * The input namespace
          */
         namespace input {

            /*
             * Unmarshaling for a string. The first two bytes (16bit) are the length of the string followed by the characters.
             */
            void NWBOBinary::read( std::string& value)
            {
               unsigned short int size;
               *this >> size;

               value.resize( size); /* I know, not good should be size_type */

               std::copy(
                  std::begin( m_buffer) + m_offset,
                  std::begin( m_buffer) + m_offset + size,
                  std::begin( value));

               m_offset += size;
            }

            /*
             * Unmarshaling of a binary array. The first eight bytes (64 bits) are the length of the binary blob.
             */
            void NWBOBinary::read( common::binary_type& value)
            {
               uint64_t size;
               assert( m_buffer.size() >= ( m_offset+8));
               *this >> size;
               assert( m_buffer.size() >= ( m_offset+size));
               value.assign(
                  std::begin( m_buffer) + m_offset,
                  std::begin( m_buffer) + m_offset + size);

               m_offset += size;
            }

            /*
             * Read an unsigned 8 bit integer.
             */
            void NWBOBinary::readIntegralU8 (uint8_t& value)
            {
               assert( m_buffer.size() >= ( m_offset+1));
               memcpy( &value, &m_buffer[ m_offset], 1);
               m_offset += 1;
            }

            /*
             * Read a signed 8 bit integer.
             */
            void NWBOBinary::readIntegral8 (int8_t& value)
            {
               assert( m_buffer.size() >= ( m_offset+1));
               memcpy( &value, &m_buffer[ m_offset], 1);
               m_offset += 1;
            }

            /*
             * Read a unsigned 16 bit integer.
             */
            void NWBOBinary::readIntegralU16 (uint16_t& value)
            {
               assert( m_buffer.size() >= ( m_offset+2));
               value = ntohs (*(reinterpret_cast<uint16_t *>(&m_buffer[ m_offset])));
               m_offset += 2;
            }

            /*
             * Read a signed 16 bit integer.
             */
            void NWBOBinary::readIntegral16 (int16_t& value)
            {
               assert( m_buffer.size() >= ( m_offset+2));
               *(reinterpret_cast<int16_t *>(&value))
                     = ntohs (*(reinterpret_cast<uint16_t *>(&m_buffer[ m_offset])));
               m_offset += 2;
            }

            /*
             * Read a unsigned 32 bit integer.
             */
            void NWBOBinary::readIntegralU32 (uint32_t& value)
            {
               assert( m_buffer.size() >= ( m_offset+4));
               value = ntohl (*(reinterpret_cast<uint32_t *>(&m_buffer[ m_offset])));
               m_offset += 4;
            }

            /*
             * Read a unsigned 32 bit integer.
             */
            void NWBOBinary::readIntegral32 (int32_t& value)
            {
               assert( m_buffer.size() >= ( m_offset+4));
               *(reinterpret_cast<int32_t *>(&value))
                     = ntohl (*(reinterpret_cast<uint32_t *>(&m_buffer[ m_offset])));
               m_offset += 4;
            }

            /*
             * Read a unsigned 64 bit integer.
             */
            void NWBOBinary::readIntegralU64 (uint64_t& value)
            {
               assert( m_buffer.size() >= ( m_offset+8));
               value = _ntohll (*(reinterpret_cast<uint64_t *>(&m_buffer[ m_offset])));
               m_offset += 8;
            }

            /*
             * Read a unsigned 64 bit integer.
             */
            void NWBOBinary::readIntegral64 (int64_t& value)
            {
               assert( m_buffer.size() >= ( m_offset+8));
               *(reinterpret_cast<int64_t *>(&value))
                     = _ntohll (*(reinterpret_cast<uint64_t *>(&m_buffer[ m_offset])));
               m_offset += 8;
            }
         }
      }
   }
}


