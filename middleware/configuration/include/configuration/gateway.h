//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/serialize/macro.h"
#include "common/platform.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace gateway
      {
         namespace listener
         {
            struct Limit
            {
               common::optional< common::platform::size::type> size;
               common::optional< common::platform::size::type> messages;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( messages);
               )
            };

            struct Default
            {
               common::optional< Limit> limit;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( limit);
               )
            };

         } // listener

         struct Listener : listener::Default
         {
            Listener() = default;
            Listener( std::function<void( Listener&)> foreign) { foreign( *this);}

            std::string address;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               listener::Default::serialize( archive);
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( note);
            )

            friend bool operator == ( const Listener& lhs, const Listener& rhs);
            friend Listener& operator += ( Listener& lhs, const listener::Default& rhs);
         };

         namespace connection
         {
            struct Default
            {
               common::optional< bool> restart;
               common::optional< std::string> address;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( restart);
                  CASUAL_SERIALIZE( address);
               )

            };

         } // connection

         struct Connection : connection::Default
         {
            Connection() = default;
            Connection( std::function<void( Connection&)> foreign) { foreign( *this);}

            std::vector< std::string> services;
            std::vector< std::string> queues;
            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               connection::Default::serialize( archive);
               CASUAL_SERIALIZE( note);
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )

            friend bool operator == ( const Connection& lhs, const Connection& rhs);
            friend Connection& operator += ( Connection& lhs, const connection::Default& rhs);
         };

         namespace manager
         {
            struct Default
            {
               Default();

               listener::Default listener;
               connection::Default connection;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  CASUAL_SERIALIZE( listener);
                  CASUAL_SERIALIZE( connection);
               )
            };
         } // manager


         struct Manager
         {
            manager::Default manager_default;
            std::vector< gateway::Listener> listeners;
            std::vector< gateway::Connection> connections;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE_NAME( manager_default, "default");
               CASUAL_SERIALIZE( listeners);
               CASUAL_SERIALIZE( connections);
            )

            //!
            //! Complement with defaults and validates
            //!
            void finalize();

            Manager& operator += ( const Manager& rhs);
            Manager& operator += ( Manager&& rhs);
            friend Manager operator + ( const Manager& lhs, const Manager& rhs);

         };


      } // gateway
   } // configuration
} // casual


