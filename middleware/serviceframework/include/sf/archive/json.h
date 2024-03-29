//!
//! archive_jsaon.h
//!
//! Created on: Jul 10, 2013
//!     Author: Lazan
//!

#ifndef ARCHIVE_JSON_H_
#define ARCHIVE_JSON_H_


#include "sf/reader_policy.h"
#include "sf/archive/basic.h"
#include "sf/platform.h"

// TODO: Move this to makefile
#define RAPIDJSON_HAS_STDSTRING 1

#include <rapidjson/document.h>


#include <iosfwd>
#include <string>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace json
         {

            class Load
            {

            public:

               typedef rapidjson::Document source_type;

               Load();
               ~Load();

               const rapidjson::Document& serialize( std::istream& stream);
               const rapidjson::Document& serialize( const std::string& json);
               // TODO: make this a binary::Stream instead
               const rapidjson::Document& serialize( const char* json);

               const rapidjson::Document& source() const;


               template<typename T>
               const rapidjson::Document& operator() ( T&& json)
               {
                  return serialize( std::forward<T>( json));
               }


            private:

               rapidjson::Document m_document;

            };


            namespace reader
            {

               class Implementation
               {
               public:

                  explicit Implementation( const rapidjson::Value& object);
                  ~Implementation();

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char* name);
                  void container_end( const char* name);

                  bool serialtype_start( const char* name);
                  void serialtype_end( const char* name);


                  template< typename T>
                  bool read( T& value, const char* name)
                  {
                     const bool result = start( name);

                     if( result)
                     {
                        if( m_stack.back()->IsNull())
                        {
                           //
                           // Act (somehow) relaxed
                           //

                           value = T();
                        }
                        else
                        {
                           //
                           // TODO: Perhaps validate type (to avoid possible asserts/exceptions) ?
                           //

                           read( value);
                        }
                     }

                     end( name);

                     return result;
                  }

               private:

                  bool start( const char* name);
                  void end( const char* name);

                  void read( bool& value);
                  void read( short& value);
                  void read( long& value);
                  void read( long long& value);
                  void read( float& value);
                  void read( double& value);
                  void read( std::string& value);
                  void read( char& value);
                  void read( platform::binary_type& value);

               private:

                  std::vector<const rapidjson::Value*> m_stack;
               };
            } // reader

            class Save
            {

            public:

               typedef rapidjson::Document target_type;

               Save();
               ~Save();

               void serialize( std::ostream& stream) const;
               void serialize( std::string& json) const;
               // TODO: make a binary::Stream overload

               rapidjson::Document& target();

               rapidjson::Document& operator() ()
               {
                  return target();
               }


            private:

               rapidjson::Document m_document;

            };


            namespace writer
            {
               class Implementation
               {
               public:

                  explicit Implementation( rapidjson::Document& document);
                  Implementation( rapidjson::Value& object, rapidjson::Document::AllocatorType& allocator);
                  ~Implementation();

                  std::size_t container_start( std::size_t size, const char* name);
                  void container_end( const char* name);

                  void serialtype_start( const char* name);
                  void serialtype_end( const char* name);

                  template< typename T>
                  void write( const T& value, const char* name)
                  {
                     start( name);
                     write( value);
                     end( name);
                  }


               private:

                  void start( const char* name);
                  void end( const char* name);

                  void write( const bool value);
                  void write( const char value);
                  void write( const short value);
                  void write( const long value);
                  void write( const long long value);
                  void write( const float value);
                  void write( const double value);
                  void write( const std::string& value);
                  void write( const platform::binary_type& value);


               private:

                  rapidjson::Document::AllocatorType& m_allocator;
                  std::vector< rapidjson::Value*> m_stack;
               };

            } // writer

            template< typename P>
            using basic_reader = archive::basic_reader< reader::Implementation, P>;


            using Reader = basic_reader< policy::Strict>;

            namespace relaxed
            {
               using Reader = basic_reader< policy::Relaxed>;
            }


            typedef basic_writer< writer::Implementation> Writer;

         } // json
      } // archive
   } // sf
} // casual




#endif /* ARCHIVE_JSAON_H_ */
