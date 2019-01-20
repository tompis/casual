//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/traits.h"
#include "common/serialize/customize.h"

#include "common/communication/message.h"
#include "common/algorithm.h"
#include "common/memory.h"
#include "common/execution.h"

#include <vector>
#include <cassert>


namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace native
         {
            namespace binary
            {
               using size_type = platform::size::type;

               struct Policy
               {

                  template< typename T>
                  static void write( const T& value, platform::binary::type& buffer)
                  {
                     memory::append( value, buffer);
                  }

                  template< typename T>
                  static void write_size( const T& value, platform::binary::type& buffer)
                  {
                     write( value, buffer);
                  }

                  template< typename T>
                  static size_type read( const platform::binary::type& buffer, size_type offset, T& value)
                  {
                     return memory::copy( buffer, offset, value);
                  }

                  template< typename T>
                  static size_type read_size( const platform::binary::type& buffer, size_type offset, T& value)
                  {
                     return read( buffer, offset, value);
                  }

               };

               template< typename P>
               struct basic_output
               {
                  using policy_type = P;

                  basic_output( platform::binary::type& buffer)
                     : m_buffer( buffer)
                  {
                     m_buffer.reserve( 128);
                  }

                  template< typename T>
                  basic_output& operator & ( T&& value)
                  {
                     return *this << std::forward< T>( value);
                  }

                  template< typename T>
                  basic_output& operator << ( T&& value)
                  {
                     customize::value::write( *this, std::forward< T>( value), nullptr);
                     return *this;
                  }

                  template< typename Iter>
                  void append( Iter first, Iter last)
                  {
                     m_buffer.insert(
                           std::end( m_buffer),
                           first,
                           last);
                  }

                  template< typename C>
                  void append( C&& range)
                  {
                     append( std::begin( range), std::end( range));
                  }

                  template< typename T> 
                  void write( const T& value, const char*) 
                  { 
                     write( value);
                  }

               private:

                  template< typename T>
                  std::enable_if_t< ! traits::is::value::serializable< traits::remove_cvref_t< T>>::value>
                  write( const T& value)
                  {
                     customize::composite::write( *this, value);
                  }

                  template< typename T>
                  std::enable_if_t< traits::is::value::serializable< traits::remove_cvref_t< T>>::value>
                  write( const T& value)
                  {
                     write_pod( value);
                  }

                  template< typename T>
                  void write_pod( const T& value)
                  {
                     policy_type::write( value, m_buffer);
                  }

                  void write_size( size_type value)
                  {
                     policy_type::write_size( value, m_buffer);
                  }

                  template< typename T>
                  void write( const std::vector< T>& value)
                  {
                     write_size( value.size());

                     for( auto& current : value)
                     {
                        *this << current;
                     }
                  }

                  void write( const std::string& value)
                  {
                     write_size( value.size());

                     append(
                        std::begin( value),
                        std::end( value));
                  }

                  void write( const platform::binary::type& value)
                  {
                     write_size( value.size());

                     append(
                        std::begin( value),
                        std::end( value));
                  }

                  platform::binary::type& m_buffer;
               };


               using Output = basic_output< Policy>;

               template< typename P>
               struct basic_input
               {
                  using policy_type = P;

                  basic_input( const platform::binary::type& buffer)
                     : m_buffer( buffer)
                  {
                  }

                  template< typename T>
                  basic_input& operator & ( T&& value)
                  {
                     return *this >> value;
                  }

                  template< typename T>
                  basic_input& operator >> ( T&& value)
                  {
                     customize::value::read( *this, value, nullptr);
                     return *this;
                  }


                  template< typename Iter>
                  void consume( Iter out, size_type size)
                  {
                     assert( m_offset + size <= m_buffer.size());

                     const auto first = std::begin( m_buffer) + m_offset;

                     std::copy(
                        first,
                        first + size, out);

                     m_offset += size;
                  }

                  template< typename T> 
                  void read( T& value, const char*) 
                  { 
                     read( value);
                  }

               private:

                  template< typename T>
                  std::enable_if_t< ! traits::is::value::serializable< traits::remove_cvref_t< T>>::value>
                  read( T&& value)
                  {
                     customize::composite::read( *this, std::forward< T>( value));
                  }

                  template< typename T>
                  std::enable_if_t< traits::is::value::serializable< traits::remove_cvref_t< T>>::value>
                  read( T& value)
                  {
                     read_pod( value);
                  }

                  template< typename T>
                  void read( std::vector< T>& value)
                  {
                     decltype( value.size()) size;
                     read_size( size);

                     value.resize( size);

                     for( auto& current : value)
                     {
                        *this >> current;
                     }
                  }

                  void read( std::string& value)
                  {
                     decltype( value.size()) size;
                     read_size( size);

                     value.resize( size);

                     consume( std::begin( value), size);
                  }

                  void read( platform::binary::type& value)
                  {
                     decltype( value.size()) size;
                     read_size( size);

                     value.resize( size);

                     consume( std::begin( value), size);
                  }

                  template< typename T>
                  void read_pod( T& value)
                  {
                     m_offset = policy_type::read( m_buffer, m_offset, value);
                  }

                  template< typename T>
                  void read_size( T& value)
                  {
                     m_offset = policy_type::read_size( m_buffer, m_offset, value);
                  }

               private:

                  const platform::binary::type& m_buffer;
                  platform::binary::type::size_type m_offset = 0;
               };

               using Input = basic_input< Policy>;


               namespace create
               {
                  struct Output
                  {
                     binary::Output operator () ( platform::binary::type& buffer) const
                     {
                        return binary::Output{ buffer};
                     }
                  };

                  struct Input
                  {
                     binary::Input operator () ( platform::binary::type& buffer) const
                     {
                        return binary::Input{ buffer};
                     }
                  };



               } // create

            } // binary

            namespace create
            {
               template< typename T>
               struct reverse;

               template<>
               struct reverse< binary::create::Output> { using type = binary::create::Input;};

               template<>
               struct reverse< binary::create::Input> { using type = binary::create::Output;};



               template< typename T>
               using reverse_t = typename reverse< T>::type;

            } // create


            


         
         } // native
      } // serialize
   } // common
} // casual


