//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/named/value.h"
#include "common/serialize/traits.h"
#include "common/serialize/customize.h"

#include "common/exception/casual.h"
#include "common/string.h"
#include "common/platform.h"
#include "common/pimpl.h"


#include <utility>


namespace casual
{
   namespace common
   {
      namespace serialize
      {
         //! Reader interface
         class Reader
         {
         public:

            using need_named = void;

            ~Reader();

            Reader( Reader&&) noexcept;
            Reader& operator = ( Reader&&) noexcept;

            template< typename Protocol, typename... Ts>
            static Reader emplace( Ts&&... ts) { return { std::make_unique< model< Protocol>>( std::forward< Ts>( ts)...)};}

            inline std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name) { return m_protocol->container_start( size, name);}
            inline void container_end( const char* name) { m_protocol->container_end( name);}

            inline bool serialtype_start( const char* name) { return m_protocol->serialtype_start( name);}
            inline void serialtype_end( const char* name) { m_protocol->serialtype_end( name);}

            inline bool read( bool& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( char& value, const char* name){ return m_protocol->read( value, name);}
            inline bool read( short& value, const char* name) { return m_protocol->read( value, name);}
            bool read( int& value, const char* name);
            inline bool read( long& value, const char* name) { return m_protocol->read( value, name);}
            bool read( unsigned long& value, const char* name);
            inline bool read( long long& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( float& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( double& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( std::string& value, const char* name) { return m_protocol->read( value, name);}
            inline bool read( platform::binary::type& value, const char* name) { return m_protocol->read( value, name);}

            //! Validates the 'consumed' archive, if the implementation has a validate member function.
            //!
            //! It throws if there are information in the source that is not consumed by the object-model
            inline void validate() { m_protocol->validate();}

         private:

            struct concept
            {
               virtual ~concept() = default;

               virtual std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name) = 0;
               virtual void container_end( const char* name) = 0;

               virtual bool serialtype_start( const char* name) = 0;
               virtual void serialtype_end( const char* name) = 0;

               virtual bool read( bool& value, const char* name) = 0;
               virtual bool read( char& value, const char* name) = 0;
               virtual bool read( short& value, const char* name) = 0;
               virtual bool read( long& value, const char* name) = 0;
               virtual bool read( long long& value, const char* name) = 0;
               virtual bool read( float& value, const char* name) = 0;
               virtual bool read( double& value, const char* name) = 0;
               virtual bool read( std::string& value, const char* name) = 0;
               virtual bool read( platform::binary::type& value, const char* name) = 0;

               virtual void validate() = 0;
            };

            template< typename P>
            struct model : concept
            {
               using protocol_type = P;

               template< typename... Ts>
               model( Ts&&... ts) : m_protocol( std::forward< Ts>( ts)...) {}

               std::tuple< platform::size::type, bool> container_start( platform::size::type size, const char* name) override { return m_protocol.container_start( size, name);}
               void container_end( const char* name) override { m_protocol.container_end( name);}

               bool serialtype_start( const char* name) override { return m_protocol.serialtype_start( name);}
               void serialtype_end( const char* name) override { m_protocol.serialtype_end( name);}

               bool read( bool& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( char& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( short& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( long& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( long long& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( float& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( double& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( std::string& value, const char* name) override { return m_protocol.read( value, name);}
               bool read( platform::binary::type& value, const char* name) override { return m_protocol.read( value, name);}


               void validate() override { selective_validate( m_protocol);}

            private:
               template< typename T>
               using has_validate = decltype( std::declval< T&>().validate());

               template< typename T>
               static auto selective_validate( T& protocol) -> 
                  std::enable_if_t< common::traits::detect::is_detected< has_validate, T>::value>
               {
                  protocol.validate();
               }
               template< typename T>
               static auto selective_validate( T& protocol) -> 
                  std::enable_if_t< ! common::traits::detect::is_detected< has_validate, T>::value>
               {
               }

               protocol_type m_protocol;
            };

            Reader( std::unique_ptr< concept>&& base) : m_protocol( std::move( base)) {}

            std::unique_ptr< concept> m_protocol;

         };



         template< typename NVP>
         Reader& operator >> ( Reader& archive, NVP&& nvp);


         template< typename T>
         std::enable_if_t< traits::is_pod< T>::value, bool>
         serialize( Reader& archive, T& value, const char* const name)
         {
            return archive.read( value, name);
         }


         template< typename T>
         std::enable_if_t< traits::has::serialize< T, Reader&>::value, bool>
         serialize( Reader& archive, T& value, const char* const name)
         {
            if( archive.serialtype_start( name))
            {
               value.serialize( archive);
               archive.serialtype_end( name);
               return true;
            }
            return false;
         }

         template< typename T>
         std::enable_if_t< std::is_enum< T >::value, bool>
         serialize( Reader& archive, T& value, const char* const name)
         {
            typename std::underlying_type< T>::type contained;

            if( serialize( archive, contained, name))
            {
               value = static_cast< T>( contained);
               return true;
            }
            return false;
         }

         namespace detail
         {
            template< platform::size::type index>
            struct tuple_read
            {
               template< typename T>
               static void serialize( Reader& archive, T& value)
               {
                  archive >> named::value::make( std::get< std::tuple_size< T>::value - index>( value), nullptr);
                  tuple_read< index - 1>::serialize( archive, value);
               }
            };

            template<>
            struct tuple_read< 0>
            {
               template< typename T>
               static void serialize( Reader&, T&) {}
            };

            template< typename T>
            void serialize_tuple( Reader& archive, T& value, const char* const name)
            {
               const auto expected_size = std::tuple_size< T>::value;
               const auto context = archive.container_start( expected_size, name);

               if( std::get< 1>( context))
               {
                  auto size = std::get< 0>( context);

                  if( expected_size != size)
                  {
                     throw exception::casual::invalid::Node{ string::compose( "got unexpected size: ", size, " - expected: ", expected_size)};
                  }
                  tuple_read< std::tuple_size< T>::value>::serialize( archive, value);

                  archive.container_end( name);
               }
            }
         } // detail

         template< typename... T>
         void serialize( Reader& archive, std::tuple< T...>& value, const char* const name)
         {
            detail::serialize_tuple( archive, value, name);
         }

         template< typename K, typename V>
         void serialize( Reader& archive, std::pair< K, V>& value, const char* const name)
         {
            detail::serialize_tuple( archive, value, name);
         }


         template< typename T>
         std::enable_if_t< 
            traits::container::is_sequence< T>::value 
            && ! traits::container::is_string< T>::value
            && ! std::is_same< platform::binary::type, T>::value, bool>
         serialize( Reader& archive, T& container, const char* const name)
         {
            auto properties = archive.container_start( 0, name);

            if( std::get< 1>( properties))
            {
               container.resize( std::get< 0>( properties));

               for( auto& element : container)
               {
                  archive >> named::value::make( nullptr, element);
               }
               archive.container_end( name);

               return true;
            }
            return false;
         }

         namespace detail
         {
            template< typename T>
            struct value { using type = T;};

            template< typename K, typename V>
            struct value< std::pair< K, V>> { using type = std::pair< typename std::remove_cv< K>::type, V>;};

         } // detail

         template< typename T>
         std::enable_if_t< traits::container::is_associative< T >::value, bool>
         serialize( Reader& archive, T& container, const char* const name)
         {
            auto properties = archive.container_start( 0, name);

            if( std::get< 1>( properties))
            {
               auto count = std::get< 0>( properties);

               while( count-- > 0)
               {
                  typename detail::value< typename T::value_type>::type element;
                  archive >> named::value::make( element, nullptr);

                  container.insert( std::move( element));
               }

               archive.container_end( name);
               return true;
            }
            return false;
         }


         template< typename NVP>
         Reader& operator & ( Reader& archive, NVP&& nvp)
         {
            return archive >> std::forward< NVP>( nvp);
         }

         template< typename NVP>
         Reader& operator >> ( Reader& archive, NVP&& nvp)
         {
            serialize( archive, nvp.value(), nvp.name());
            return archive;
         }


         class Writer
         {
         public:

            using need_named = void;

            ~Writer();

            Writer( Writer&&) noexcept;
            Writer& operator = ( Writer&&) noexcept;

            template< typename Protocol, typename... Ts>
            static Writer emplace( Ts&&... ts) { return { std::make_unique< model< Protocol>>( std::forward< Ts>( ts)...)};}

            inline void container_start( platform::size::type size, const char* name) { m_protocol->container_start( size, name);}
            inline void container_end( const char* name) { m_protocol->container_end( name);}

            inline void serialtype_start( const char* name) { m_protocol->serialtype_start( name);}
            inline void serialtype_end( const char* name) { m_protocol->serialtype_end( name);}

            inline void write( bool value, const char* name) { m_protocol->write( value, name);}
            inline void write( char value, const char* name) { m_protocol->write( value, name);}
            inline void write( short value, const char* name) { m_protocol->write( value, name);}
            void write( int value, const char* name);
            inline void write( long value, const char* name) { m_protocol->write( value, name);}
            void write( unsigned long value, const char* name);
            inline void write( long long value, const char* name) { m_protocol->write( value, name);}
            inline void write( float value, const char* name) { m_protocol->write( value, name);}
            inline void write( double value, const char* name) { m_protocol->write( value, name);}
            inline void write( const std::string& value, const char* name) { m_protocol->write( value, name);}
            inline void write( const platform::binary::type& value, const char* name) { m_protocol->write( value, name);}

            //! Flushes the archive, if the implementation has a flush member function.
            inline void flush() { m_protocol->flush();}

         private:

            struct concept
            {
               virtual ~concept() = default;

               virtual void container_start( platform::size::type size, const char* name) = 0;
               virtual void container_end( const char* name) = 0;

               virtual void serialtype_start( const char* name) = 0;
               virtual void serialtype_end( const char* name) = 0;

               virtual void write( bool value, const char* name) = 0;
               virtual void write( char value, const char* name) = 0;
               virtual void write( short value, const char* name) = 0;
               virtual void write( long value, const char* name) = 0;
               virtual void write( long long value, const char* name) = 0;
               virtual void write( float value, const char* name) = 0;
               virtual void write( double value, const char* name) = 0;
               virtual void write( const std::string& value, const char* name) = 0;
               virtual void write( const platform::binary::type& value, const char* name) = 0;

               virtual void flush() = 0;
            };

            template< typename P>
            struct model : concept
            {
               using protocol_type = P;

               template< typename... Ts>
               model( Ts&&... ts) : m_protocol( std::forward< Ts>( ts)...) {}

               void container_start( platform::size::type size, const char* name) override { m_protocol.container_start( size, name);}
               void container_end( const char* name) override { m_protocol.container_end( name);}

               void serialtype_start( const char* name) override { m_protocol.serialtype_start( name);}
               void serialtype_end( const char* name) override { m_protocol.serialtype_end( name);}

               void write( bool value, const char* name) override { m_protocol.write( value, name);}
               void write( char value, const char* name) override { m_protocol.write( value, name);}
               void write( short value, const char* name) override { m_protocol.write( value, name);}
               void write( long value, const char* name) override { m_protocol.write( value, name);}
               void write( long long value, const char* name) override { m_protocol.write( value, name);}
               void write( float value, const char* name) override { m_protocol.write( value, name);}
               void write( double value, const char* name) override { m_protocol.write( value, name);}
               void write( const std::string& value, const char* name) override { m_protocol.write( value, name);}
               void write( const platform::binary::type& value, const char* name) override { m_protocol.write( value, name);}

               void flush() override { selective_flush( m_protocol);}

            private:
               template< typename T>
               using has_flush = decltype( std::declval< T&>().flush());

               template< typename T>
               static auto selective_flush( T& protocol) -> 
                  std::enable_if_t< common::traits::detect::is_detected< has_flush, T>::value>
               {
                  protocol.flush();
               }
               template< typename T>
               static auto selective_flush( T& protocol) -> 
                  std::enable_if_t< ! common::traits::detect::is_detected< has_flush, T>::value>
               {
               }

               protocol_type m_protocol;
            };

            Writer( std::unique_ptr< concept>&& base) : m_protocol( std::move( base)) {}

            std::unique_ptr< concept> m_protocol;

         };

         template< typename NV>
         Writer& operator << ( Writer& archive, NV&& nvp);


         template< typename T>
         std::enable_if_t< traits::is_pod< T>::value>
         serialize( Writer& archive, const T& value, const char* const name)
         {
            customize::value::write( archive, value, name);
         }


         template< typename T>
         std::enable_if_t< traits::has::serialize< traits::remove_cvref_t< T>, Writer&>::value>
         serialize( Writer& archive, const T& value, const char* const name)
         {
            archive.serialtype_start( name);

            customize::composite::write( archive, value);

            archive.serialtype_end( name);
         }


         namespace detail
         {
            template< platform::size::type index>
            struct tuple_write
            {
               template< typename T>
               static void serialize( Writer& archive, const T& value)
               {
                  archive << named::value::make( std::get< std::tuple_size< T>::value - index>( value), nullptr);
                  tuple_write< index - 1>::serialize( archive, value);
               }
            };

            template<>
            struct tuple_write< 0>
            {
               template< typename T>
               static void serialize( Writer&, const T&) {}
            };

            template< typename T>
            void serialize_tuple( Writer& archive, const T& value, const char* const name)
            {
               archive.container_start( std::tuple_size< T>::value, name);
               tuple_write< std::tuple_size< T>::value>::serialize( archive, value);
               archive.container_end( name);
            }
         } // detail

         template< typename... T>
         void serialize( Writer& archive, const std::tuple< T...>& value, const char* const name)
         {
            detail::serialize_tuple( archive, value, name);
         }

         template< typename K, typename V>
         void serialize( Writer& archive, const std::pair< K, V>& value, const char* const name)
         {
            detail::serialize_tuple( archive, value, name);
         }


         template< typename T>
         std::enable_if_t< std::is_enum< T >::value>
         serialize( Writer& archive, const T& value, const char* const name)
         {
            serialize( archive, static_cast< std::underlying_type_t< T>>( value), name);
         }


         template< typename T>
         std::enable_if_t< 
            traits::container::is_container< T>::value 
            && ! traits::container::is_string< T>::value 
            && ! std::is_same< platform::binary::type, T>::value>
         serialize( Writer& archive, const T& container, const char* const name)
         {
            archive.container_start( container.size(), name);

            for( auto& element : container)
            {
               archive << named::value::make( element, nullptr);
            }

            archive.container_end( name);
         }


         template< typename T>
         void serialize( Writer& archive, const optional< T>& value, const char* const name)
         {
            if( value)
            {
               archive << named::value::make( value.value(), name);
            }
         }

         template< typename NVP>
         Writer& operator & ( Writer& archive, NVP&& nvp)
         {
            return operator << ( archive, std::forward< NVP>( nvp));
         }


         template< typename NVP>
         Writer& operator << ( Writer& archive, NVP&& nvp)
         {
            serialize( archive, nvp.value(), nvp.name());
            return archive;
         }                  
      } // serialize
   } // common
} // casual


