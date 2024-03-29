//
// transcoding.h
//
//  Created on: Dec 26, 2013
//      Author: Kristone
//

#ifndef CASUAL_COMMON_TRANSCODE_H_
#define CASUAL_COMMON_TRANSCODE_H_


#include "common/platform.h"

#include <string>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace transcode
      {
         namespace base64
         {
            namespace detail
            {
               std::string encode( const void* data, std::size_t bytes);
            } // detail


            //!
            //! @return Base64-encoded binary data of [first, last)
            //!
            //! @pre @p Iter has to be a random access iterator
            //!
            //! @throw exception::Casual on failure
            //!
            template< typename Iter>
            std::string encode( Iter first, Iter last)
            {
               static_assert( std::is_same<
                     typename std::iterator_traits< Iter>::iterator_category,
                     std::random_access_iterator_tag>::value, "first/last has to be random access iterators");

               return detail::encode( &(*first), std::distance( first , last) * sizeof( decltype( *first)));
            }

            //!
            //! @return Base64-encoded binary data of @p container
            //!
            //! @throw exception::Casual on failure
            //!
            template< typename C>
            std::string encode( C&& container)
            {
               return encode( std::begin( container), std::end( container));
            }

            //!
            //! @return Base64-decoded binary data
            //!
            //! @throw exception::Casual on failure
            //!
            platform::binary_type decode( const std::string& value);

         } // base64

         namespace utf8
         {
            //!
            //! @param value String encoded in local default codeset
            //!
            //! @return UTF-8-encoded string
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::invalid::Argument for bad input
            //! @throw exception::Casual on other failures
            //!
            std::string encode( const std::string& value);

            //!
            //! @param value The UTF-8 encoded string
            //!
            //! @return String encoded in local default codeset
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::invalid::Argument for bad input
            //! @throw exception::Casual on other failures
            //!
            std::string decode( const std::string& value);

            //!
            //! @param value String encoded in provided codeset
            //! @param codeset String-encoding
            //!
            //! @return UTF-8-encoded string
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::invalid::Argument for bad input
            //! @throw exception::Casual on other failures
            //!
            std::string encode( const std::string& value, const std::string& codeset);

            //!
            //! @param value The UTF-8 encoded string
            //! @param codeset Encoding for result
            //!
            //! @return String encoded in provided codeset
            //!
            //! @throw exception::limit::Memory on resource failures
            //! @throw exception::invalid::Argument for bad input
            //! @throw exception::Casual on other failures
            //!
            std::string decode( const std::string& value, const std::string& codeset);

         } // utf8

         namespace hex
         {
            namespace detail
            {
               std::string encode( const void* data, std::size_t bytes);
               void decode( const std::string& value, void* data);
            } // detail


            //!
            //! encode binary sequence [first, last) to hex-representation
            //!
            //! @param first start of binary
            //! @param last end of binary (exclusive)
            //! @return hex-encoded string of [first, last)
            //!
            template< typename Iter>
            std::string encode( Iter first, Iter last)
            {
               return detail::encode( &(*first), last - first);
            }

            //!
            //! encode binary @p container to hex-representation
            //!
            //! @param container binary representation
            //! @return hex-encoded string of @p container
            //!
            template< typename C>
            std::string encode( C&& container)
            {
               return encode( std::begin( container), std::end( container));
            }


            //!
            //! decode hex-string to a binary representation
            //!
            //! @param value hex-string
            //! @return binary representation of @p value
            //!
            platform::binary_type decode( const std::string& value);


            //!
            //! decode hex-string to a binary representation
            //!
            //! @return binary representation of @p value
            //!
            template< typename C>
            void decode( const std::string& value, C&& container)
            {
               return detail::decode( value, &( *std::begin( container)));
            }




         } // hex
      } // transcode
   } // common
} // casual


#endif /* TRANSCODING_H_ */
