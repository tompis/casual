//!
//! casual
//!

#include "configuration/domain.h"
#include "configuration/file.h"

#include "common/exception.h"
#include "common/file.h"
#include "common/environment.h"
#include "common/algorithm.h"

#include "sf/archive/maker.h"

#include <algorithm>


namespace casual
{
   using namespace common;

   namespace configuration
   {
      namespace domain
      {

         namespace local
         {
            namespace
            {

               namespace complement
               {
                  template< typename T>
                  inline void assign_if_empty( sf::optional< T>& value, const sf::optional< T>& def)
                  {
                     if( ! value.has_value())
                        value = def;
                  }

                  inline void assign_if_empty( std::string& value, const std::string& def)
                  {
                     if( value.empty() || value == "~")
                        value = def;
                  }

                  struct Default
                  {
                     Default( const domain::Default& casual_default)
                           : m_casual_default( casual_default)
                     {
                     }

                     void operator ()( domain::Server& server) const
                     {
                        assign_if_empty( server.instances, m_casual_default.server.instances);
                        assign_if_empty( server.restart, m_casual_default.server.restart);
                        assign_if_empty( server.alias, nextAlias( server.path));
                     }

                     void operator ()( domain::Service& service) const
                     {
                        assign_if_empty( service.timeout, m_casual_default.service.timeout);
                        assign_if_empty( service.transaction, m_casual_default.service.transaction);
                     }

                     void operator ()( domain::Domain& configuration) const
                     {
                        std::for_each( std::begin( configuration.servers), std::end( configuration.servers), *this);
                        std::for_each( std::begin( configuration.services), std::end( configuration.services), *this);

                     }

                  private:




                     std::string nextAlias( const std::string& path) const
                     {
                        auto alias = common::file::name::base( path);

                        auto count = m_alias[ alias]++;

                        if( count > 1)
                        {
                           return alias + "_" + std::to_string( count);
                        }

                        return alias;
                     }
                     domain::Default m_casual_default;
                     mutable std::map< std::string, std::size_t> m_alias;
                  };

                  inline void defaultValues( Domain& domain)
                  {
                     Default defaults( domain.casual_default);
                     defaults( domain);
                  }

               } // complement

               void validate( const Domain& settings)
               {

               }

               template< typename LHS, typename RHS>
               void replace_or_add( LHS& lhs, RHS&& rhs)
               {
                  for( auto& value : rhs)
                  {
                     auto found = range::find( lhs, value);

                     if( found)
                     {
                        *found = std::move( value);
                     }
                     else
                     {
                        lhs.push_back( std::move( value));
                     }
                  }
               }

               template< typename D>
               Domain& append( Domain& lhs, D&& rhs)
               {
                  if( lhs.name.empty()) { lhs.name = std::move( rhs.name);}

                  local::replace_or_add( lhs.groups, rhs.groups);
                  local::replace_or_add( lhs.executables, rhs.executables);
                  local::replace_or_add( lhs.servers, rhs.servers);
                  local::replace_or_add( lhs.services, rhs.services);

                  lhs.gateway += std::move( rhs.gateway);
                  lhs.queue += std::move( rhs.queue);

                  return lhs;
               }

               Domain get( Domain domain, const std::string& file)
               {
                  //
                  // Create the reader and deserialize configuration
                  //
                  auto reader = sf::archive::reader::from::file( file);

                  reader >> CASUAL_MAKE_NVP( domain);

                  finalize( domain);

                  return domain;

               }

            } // unnamed
         } // local

         bool operator == ( const Executable& lhs, const Executable& rhs)
         {
            return coalesce( lhs.alias, lhs.path) == coalesce( rhs.alias, rhs.path);
         }

         bool operator == ( const Server& lhs, const Server& rhs)
         {
            return lhs.restriction == rhs.restriction &&
                  static_cast< const Executable&>( lhs) == static_cast< const Executable&>( rhs);
         }

         bool operator == ( const Group& lhs, const Group& rhs)
         {
            return lhs.name == rhs.name;
         }

         bool operator == ( const Service& lhs, const Service& rhs)
         {
            return lhs.name == rhs.name;
         }

         Domain& Domain::operator += ( const Domain& rhs)
         {
            return local::append( *this, rhs);
         }

         Domain& Domain::operator += ( Domain&& rhs)
         {
            return local::append( *this, std::move( rhs));
         }

         Domain operator + ( const Domain& lhs, const Domain& rhs)
         {
            auto result = lhs;
            result += rhs;
            return result;
         }

         void finalize( Domain& configuration)
         {
            //
            // Complement with default values
            //
            local::complement::defaultValues( configuration);

            //
            // Make sure we've got valid configuration
            //
            local::validate( configuration);

            configuration.gateway.finalize();
            configuration.queue.finalize();

         }


         Domain get( const std::vector< std::string>& files)
         {
            if( files.empty())
            {
               return persistent::get();
            }

            return range::accumulate( files, Domain{}, &local::get);
         }



         namespace persistent
         {
            Domain get()
            {
               auto configuration = file::persistent::domain();

               if( common::file::exists( configuration))
               {
                  return domain::get( { configuration});
               }
               else
               {
                  throw common::exception::invalid::File{ "failed to get persistent configuration file", CASUAL_NIP( configuration)};
               }
            }

            void save( const Domain& domain)
            {
               if( ! common::file::exists( directory::persistent()))
               {
                  common::directory::create( directory::persistent());
               }

               auto configuration = file::persistent::domain();

               auto archive = sf::archive::writer::from::file( configuration);
               archive << CASUAL_MAKE_NVP( domain);
            }

         } // persistent

      } // domain

   } // config
} // casual
