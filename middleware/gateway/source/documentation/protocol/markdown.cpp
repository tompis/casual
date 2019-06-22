//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/documentation/protocol/example.h"
#include "gateway/message.h"

#include "common/serialize/native/binary.h"
#include "common/serialize/native/network.h"
#include "common/network/byteorder.h"
#include "common/communication/tcp.h"

#include "common/message/transaction.h"
#include "common/message/service.h"

#include "common/view/binary.h"
#include "common/terminal.h"
#include "common/execute.h"

#include <iostream>
#include <typeindex>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace customize
         {

            namespace composit
            {
               //! just a local marshaler to help format Header...
               template< typename A>
               struct Value< communication::message::complete::network::Header, A>
               {
                  template< typename V>  
                  static void serialize( A& archive, V&& value)
                  {
                     CASUAL_SERIALIZE_NAME( static_cast< communication::message::complete::network::Header::host_type_type>( value.type), "type");
                     CASUAL_SERIALIZE_NAME( common::view::binary::make( value.correlation), "correlation");
                     CASUAL_SERIALIZE_NAME( static_cast< communication::message::complete::network::Header::host_size_type>( value.size), "size");
                  }
               };
            } // composit
            
         } // customize
         
      } // serialize
   } // common

   namespace gateway
   {
      namespace documentation
      {
         namespace protocol
         {
            namespace local
            {
               namespace
               {
                  namespace type
                  {
                     struct Name
                     {
                        std::string role;
                        std::string description;
                     };

                     struct Type
                     {
                        std::string type;
                        common::platform::size::type size;
                     };

                     struct Info
                     {
                        Name name;
                        Type network;
                     };

                     template< typename T>
                     const char* name( T&& value)
                     {
                        static std::unordered_map< std::type_index, const char*> names
                        {
                           { typeid( std::uint8_t), "uint8"},
                           { typeid( std::uint16_t), "uint16"},
                           { typeid( std::uint32_t), "uint32"},
                           { typeid( std::uint64_t), "uint64"},
                        };

                        const auto found = common::algorithm::find( names, typeid( common::serialize::native::binary::network::detail::cast( value)));

                        if( found)
                           return found->second;

                        return typeid( T).name();
                     }

                     template< typename T>
                     auto network( T&& value) -> std::enable_if_t< ! common::serialize::native::binary::network::detail::is_network_array< T>::value, Type>
                     {
                        auto network = common::network::byteorder::encode( common::serialize::native::binary::network::detail::cast( value));
                        return Type{ name( network), common::memory::size( network)};
                     }

                     template< typename T>
                     auto network( T&& value) -> std::enable_if_t< common::serialize::native::binary::network::detail::is_network_array< T>::value, Type>
                     {
                        return Type{ "fixed array", static_cast< common::platform::size::type>( common::memory::size( value))};
                     }


                     template< typename T>
                     Info info( T&& value, type::Name name)
                     {
                        return Info{ std::move( name), network( std::forward< T>( value))};
                     }

                     template< typename T>
                     Info info( T&& value, std::string role, std::string description)
                     {
                        return info( std::forward< T>( value), { std::move( role), std::move( description)});
                     }


                     template< typename T>
                     void print( std::ostream& out, T&& value, std::string role, std::string description)
                     {
                        print( out, info( std::forward< T>( value), std::move( role), std::move( description)));
                     }

                     template< typename T>
                     void print( std::ostream& out, std::string role, std::string description)
                     {
                        print( out, T{}, std::move( role), std::move( description));
                     }

                  } // type



                  struct Printer
                  {
                     using need_named = void;
                     using is_network_normalizing = void;

                     Printer() = default;

                     Printer( std::initializer_list< type::Name> roles)
                     {
                        common::algorithm::for_each( roles, [&]( auto& name)
                        {
                           m_descriptions.emplace( std::move( name.role), std::move( name.description));
                        });
                     }


                     template< typename T>
                     Printer& operator & ( T&& value)
                     {
                        return *this << std::forward< T>( value);
                     }

                     template< typename T>
                     Printer& operator << ( T&& value)
                     {
                        common::serialize::value::write( *this, std::forward< T>( value), nullptr);
                        return *this;
                     }

                     inline void container_start( common::platform::size::type size, const char* name) 
                     { 
                        canonical.push( name);
                        write_size( size);
                        canonical.push( "element");
                     }
                     inline void container_end( const char*) 
                     { 
                        canonical.pop(); // element
                        canonical.pop(); // name
                     }

                     inline void composite_start( const char* name) { canonical.push( name);}
                     inline void composite_end(  const char*) { canonical.pop();}


                     template< typename T> 
                     void write( T&& value, const char* name) 
                     { 
                        canonical.push( name);
                        write( std::forward< T>( value));
                        canonical.pop();
                     }

                     std::vector< type::Info> release() 
                     {
                        return std::exchange( m_types, {});
                     }

                  private:


                     template< typename T>
                     void type( T&& value)
                     {
                        m_types.push_back( type::info( std::forward< T>( value), get_name( canonical.name())));
                     }

                     void write_size( common::platform::size::type value)
                     {
                        canonical.push( "size");
                        type( value);
                        canonical.pop();
                     }

                     void dynamic( common::platform::size::type size, const char* type)
                     {
                        type::Info info;
                        info.name = get_name( canonical.name());
                        info.network.size = size;
                        info.network.type = type;
                        m_types.push_back( std::move( info));               
                     }
                     template< typename T>
                     auto write( T&& value) -> std::enable_if_t< std::is_arithmetic< common::traits::remove_cvref_t< T>>::value>
                     {
                        type( value);
                     }

                     void write( const std::string& value)
                     {
                        write_size( value.size());
                        canonical.push( "data");
                        dynamic( value.size(), "dynamic string");
                        canonical.pop();
                     }

                     void write( const common::platform::binary::type& value)
                     {
                        write_size( value.size());
                        canonical.push( "data");
                        dynamic( value.size(), "dynamic binary");
                        canonical.pop();
                     }

                     void write( common::view::immutable::Binary value)
                     {
                        //write_size( value.size());
                        dynamic( value.size(), "(fixed) binary");
                     }

                     struct 
                     {
                        inline void push( const char* name)
                        {
                           m_parts.push_back( name);
                        }

                        inline void pop()
                        {
                           m_parts.pop_back();
                        }

                        std::string name() const  
                        {
                           auto parts = common::algorithm::find_if( m_parts, []( auto part) { return part != nullptr;});

                           if( ! parts)
                              return {};
                           
                           std::ostringstream out;
                           out << *parts;
                           ++parts;
                           
                           common::algorithm::for_each( parts, [&out]( auto part)
                           {
                              if( part != nullptr)
                                 out << '.' << part;
                           });

                           return std::move( out).str();
                        } 

                     private:
                        std::deque< const char*> m_parts;

                     } canonical;



                     type::Name get_name( std::string role)
                     {
                        type::Name name{ std::move( role), {}};

                        auto found = common::algorithm::find( m_descriptions, name.role);

                        if( found)
                           name.description = found->second;

                        return name;
                     } 

                     
                     std::map< std::string, std::string> m_descriptions;
                     std::vector< type::Info> m_types;

                  };
               }
            }
         }
      } // documentation
   } // gateway

   namespace gateway
   {
      namespace documentation
      {

         namespace protocol
         {
            namespace local
            {
               namespace
               {

                  namespace extract
                  {
                     template< typename T>
                     std::vector< type::Info> types( T&& type, std::initializer_list< type::Name> roles)
                     {
                        local::Printer printer{ std::move( roles)};
                        printer << type;

                        return printer.release();
                     }


                  } // extract

                  namespace format
                  {

                     auto type_info()
                     {
                        return common::terminal::format::formatter< type::Info>::construct( 
                           std::string{ " | "},
                           common::terminal::format::column( "role name", []( const type::Info& i) { return i.name.role;}, common::terminal::color::no_color),
                           common::terminal::format::column( "network type", []( const type::Info& i) { return i.network.type;}, common::terminal::color::no_color),
                           common::terminal::format::column( "network size", []( const type::Info& i) { return i.network.size;}, common::terminal::color::no_color, common::terminal::format::Align::right),
                           common::terminal::format::column( "description", []( const type::Info& i) { return i.name.description;}, common::terminal::color::no_color)
                        );
                     }

                     template< typename T>
                     void type( std::ostream& out, T&& value, std::initializer_list< type::Name> roles)
                     {
                        auto formatter = type_info();

                        formatter.print( out, extract::types( std::forward< T>( value), std::move( roles)));
                     }


                  } // format

                  template< typename M>
                  std::ostream& message_type( std::ostream& out, M message)
                  {
                     return out << "message type: **" << common::message::type( message) << "**";
                  }

                  namespace binary
                  {
                     auto value( common::platform::size::type size)
                     {
                        constexpr auto min = std::numeric_limits< common::platform::binary::value::type>::min();
                        constexpr auto max = std::numeric_limits< common::platform::binary::value::type>::max();

                        common::platform::binary::type result;
                        result.resize( size);

                        auto current = min;

                        common::algorithm::for_each( result, [&current]( auto& c)
                        {
                           if( current > max)
                              current = min;

                           c = current++;
                        });

                        return result;
                     }
                  } // binary

                  namespace string
                  {
                     auto value( common::platform::size::type size)
                     {
                        constexpr common::platform::binary::value::type letters[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'a', 'b', 'c', 'd', 'e', 'f'};

                        std::string result;
                        result.resize( size);

                        auto current = std::begin( letters);

                        common::algorithm::for_each( result, [&]( auto& c)
                        {
                           if( current == std::end( letters))
                              current = std::begin( letters);

                           c = *(current++);
                        });

                        return result;
                     }
                  } // binary

               } // <unnamed>
            } // local

            namespace print
            {
               void header( std::ostream& out)
               {
                  out << R"(
   | role name     | network type | network size  | comments
   |---------------|--------------|---------------|---------
   )";
               }

               void message_header( std::ostream& out)
               {
                  common::communication::message::complete::network::Header header;

                  out << R"(
   # casual domain protocol _version 1000_

   Attention, this documentation refers to **version 1000** (aka, version 1)




   Defines what messages is sent between domains and exactly what they contain. 

   Some definitions:

   * `fixed array`: an array of fixed size where every element is an 8 bit byte.
   * `dynamic array`: an array with dynamic size, where every element is an 8 bit byte.

   If an attribute name has `element` in it, for example: `services.element.timeout`, the
   data is part of an element in a container. You should read it as `container.element.attribute`


   ## common::communication::message::complete::network::Header 

   This header will be the first part of every message below, hence it's name, _Header_

   message.type is used to dispatch to handler for that particular message, and knows how to (un)marshal and act on the message.

   It's probably a good idea (probably the only way) to read the header only, to see how much more one has to read to get
   the rest of the message.


   )";

                  local::format::type( out, CASUAL_MAKE_NVP( header), {
                     { "header.type", "type of the message that the payload contains"},
                     { "header.correlation", "correlation id of the message"},
                     { "header.size", "the size of the payload that follows"},
                  });
               }


               template< typename M>
               void transaction_request( std::ostream& out, M&& message)
               {
                  message.trid = common::transaction::id::create();

                  local::message_type( out, message) << "\n\n";

                  local::format::type( out, message, {
                           { "execution", "uuid of the current execution context (breadcrumb)"},
                           { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                           { "xid.gtrid_length", "length of the transaction gtrid part"},
                           { "xid.bqual_length", "length of the transaction branch part"},
                           { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                           { "resource", "RM id of the resource - has to correlate with the reply"},
                           { "flags", "XA flags to be forward to the resource"},
                        });
               }

               template< typename M>
               void transaction_reply( std::ostream& out, M&& message)
               {
                  message.trid = common::transaction::id::create();

                  local::message_type( out, message) << "\n\n";

                  local::format::type( out, message, {
                           { "execution", "uuid of the current execution context (breadcrumb)"},
                           { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                           { "xid.gtrid_length", "length of the transaction gtrid part"},
                           { "xid.bqual_length", "length of the transaction branch part"},
                           { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                           { "resource", "RM id of the resource - has to correlate with the request"},
                           { "state", "The state of the operation - If successful XA_OK ( 0)"},
                        });
               }


               void transaction( std::ostream& out)
               {
                  out << R"(
## Transaction messages

### Resource prepare

   )";
                  {

                     using message_type = common::message::transaction::resource::prepare::Request;

                     out << R"(
#### message::transaction::resource::prepare::Request

Sent to and received from other domains when one domain wants to prepare a transaction. 

   )";
                     transaction_request( out, message_type{});
                  }

                  {
                     using message_type = common::message::transaction::resource::prepare::Reply;

                     out << R"(
#### message::transaction::resource::prepare::Reply

Sent to and received from other domains when one domain wants to prepare a transaction. 

   )";
                     transaction_reply( out, message_type{});
                  }

                  out << R"(
### Resource commit

   )";

                  {
                     using message_type = common::message::transaction::resource::commit::Request;

                     out << R"(
#### message::transaction::resource::commit::Request

Sent to and received from other domains when one domain wants to commit an already prepared transaction.

   )";
                     transaction_request( out, message_type{});
                  }

                  {
                     using message_type = common::message::transaction::resource::commit::Reply;

                     out << R"(
#### message::transaction::resource::commit::Reply

Reply to a commit request. 

)";
                     transaction_reply( out, message_type{});
                  }



                  out << R"(
### Resource rollback

)";


                  {
                     using message_type = common::message::transaction::resource::rollback::Request;

                     out << R"(
#### message::transaction::resource::rollback::Request

Sent to and received from other domains when one domain wants to rollback an already prepared transaction.
That is, when one or more resources has failed to prepare.

)";
                     transaction_request( out, message_type{});
                  }

                  {
                     using message_type =  common::message::transaction::resource::rollback::Reply;

                     out << R"(
#### message::transaction::resource::rollback::Reply

Reply to a rollback request. 

)";
                     transaction_reply( out, message_type{});
                  }

               }



               void service_call( std::ostream& out)
               {
                  out << R"(
## Service messages

### Service call 

)";
                  {
                     using message_type = common::message::service::call::callee::Request;

                     out << R"(
#### message::service::call::Request

Sent to and received from other domains when one domain wants call a service in the other domain

)";
                     local::message_type( out, message_type{}) << "\n\n";

                     message_type request;
                     request.trid = common::transaction::id::create();
                     request.service.name = local::string::value( 128);
                     request.parent = local::string::value( 128);
                     request.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
                     request.buffer.memory = local::binary::value( 1024);

                     local::format::type( out, request, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "service.name.size", "service name size"},
                              { "service.name.data", "byte array with service name"},
                              { "service.timeout", "timeout of the service in use (in microseconds)"},
                              { "parent.size", "parent service name size"},
                              { "parent.data", "byte array with parent service name"},

                              { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                              { "xid.gtrid_length", "length of the transaction gtrid part"},
                              { "xid.bqual_length", "length of the transaction branch part"},
                              { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},

                              { "flags", "XATMI flags sent to the service"},

                              { "buffer.type.size", "buffer type name size"},
                              { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                              { "buffer.memory.size", "buffer payload size (could be very big)"},
                              { "buffer.memory.data", "buffer payload data (with the size of buffer.payload.size)"},
                           });
                  }


                  {
                     using message_type = common::message::service::call::Reply;

                     out << R"(
#### message::service::call::Reply

Reply to call request

)";
                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     message.transaction.trid = common::transaction::id::create();
                     message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
                     message.buffer.memory = local::binary::value( 1024);

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},

                              { "code.result", "XATMI result/error code, 0 represent OK"},
                              { "code.user", "XATMI user supplied code"},

                              { "transaction.xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                              { "transaction.xid.gtrid_length", "length of the transaction gtrid part"},
                              { "transaction.xid.bqual_length", "length of the transaction branch part"},
                              { "transaction.xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                              { "transaction.state", "state of the transaction TX_ACTIVE, TX_TIMEOUT_ROLLBACK_ONLY, TX_ROLLBACK_ONLY"},

                              { "buffer.type.size", "buffer type name size"},
                              { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                              { "buffer.memory.size", "buffer payload size (could be very big)"},
                              { "buffer.memory.data", "buffer payload data (with the size of buffer.payload.size)"},


                           });
                  }

               }


               void domain_connect( std::ostream& out)
               {
                  out << R"(
## domain connect messages

messages that is used to set up a connection

)";     

                  {
                     using message_type = common::message::gateway::domain::connect::Request;
                     
                     out << R"(
### common::message::gateway::domain::connect::Request
      
Connection requests from another domain that wants to connect
      
)";
      
                        local::message_type( out, message_type{}) << "\n\n";
      
                        message_type message;
                        message.versions = { common::message::gateway::domain::protocol::Version::version_1};
                        message.domain.name = "domain-A";

                        local::format::type( out, message, {
                           { "execution", "uuid of the current execution context (breadcrumb)"},
                           { "domain.id", "uuid of the outbound domain"},
                           { "domain.name.size", "size of the outbound domain name"},
                           { "domain.name.data", "dynamic byte array with the outbound domain name"},
                           { "protocol.versions.size", "number of protocol versions outbound domain can 'speak'"},
                           { "protocol.versions.element", "a protocol version "},
                        });

                  }

                  {
                     using message_type = common::message::gateway::domain::connect::Reply;
                     
                     out << R"(
### common::message::gateway::domain::connect::Reply
      
Connection reply
      
)";
      
                        local::message_type( out, message_type{}) << "\n\n";
      
                        message_type message;
                        message.version = common::message::gateway::domain::protocol::Version::version_1;
                        message.domain.name = "domain-A";

                        local::format::type( out, message, {
                           { "execution", "uuid of the current execution context (breadcrumb)"},
                           { "domain.id", "uuid of the inbound domain"},
                           { "domain.name.size", "size of the inbound domain name"},
                           { "domain.name.data", "dynamic byte array with the inbound domain name"},
                           { "protocol.version", "the chosen protocol version to use, or invalid (0) if incompatible"},
                        });

                  }               
               }


               void domain_discovery( std::ostream& out)
               {
                  out << R"(
## Discovery messages

### domain discovery 

)";

                  {
                     using message_type = common::message::gateway::domain::discover::Request;

                     out << R"(
#### message::gateway::domain::discover::Request

Sent to and received from other domains when one domain wants discover information abut the other.

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;
                     message.domain.name = "domain-A";

                     message.services.push_back( local::string::value( 128));
                     message.queues.push_back( local::string::value( 128));

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "domain.id", "uuid of the caller domain"},
                              { "domain.name.size", "size of the caller domain name"},
                              { "domain.name.data", "dynamic byte array with the caller domain name"},
                              { "services.size", "number of requested services to follow (an array of services)"},
                              { "services.element.size", "size of the current service name"},
                              { "services.element.data", "dynamic byte array of the current service name"},
                              { "queues.size", "number of requested queues to follow (an array of queues)"},
                              { "queues.element.size", "size of the current queue name"},
                              { "queues.element.data", "dynamic byte array of the current queue name"},
                           });


                  }


                  {
                     using message_type = common::message::gateway::domain::discover::Reply;

                     out << R"(
#### message::gateway::domain::discover::Reply

Sent to and received from other domains when one domain wants discover information abut the other.

)";

                     local::message_type( out, message_type{}) << "\n\n";
                     
                     auto message = protocol::example::message< message_type>();

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              // { "version", "the chosen version - 0 if no compatible version was possible"},
                              { "domain.id", "uuid of the caller domain"},
                              { "domain.name.size", "size of the caller domain name"},
                              { "domain.name.data", "dynamic byte array with the caller domain name"},
                              { "services.size", "number of services to follow (an array of services)"},
                              { "services.element.name.size", "size of the current service name"},
                              { "services.element.name.data", "dynamic byte array of the current service name"},
                              { "services.element.category.size", "size of the current service category"},
                              { "services.element.category.data", "dynamic byte array of the current service category"},
                              { "services.element.transaction", "service transaction mode (auto, atomic, join, none)"},
                              { "services.element.timeout", "service timeout"},
                              { "services.element.hops", "number of domain hops to the service (local services has 0 hops)"},
                              { "queues.size", "number of requested queues to follow (an array of queues)"},
                              { "queues.element.name.size", "size of the current queue name"},
                              { "queues.element.name.data", "dynamic byte array of the current queue name"},
                              { "queues.element.retries", "how many 'retries' the queue has"},
                           });
                  }
               }

               void queue( std::ostream& out)
               {
                  out << R"(
## queue messages

### enqueue 

)";

                  {
                     using message_type = common::message::queue::enqueue::Request;

                     out << R"(
#### message::queue::enqueue::Request

Represent enqueue request.

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     message.trid = common::transaction::id::create();

                     message.name = local::string::value( 128);
                     message.message.payload = local::binary::value( 1024);


                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "name.size", "size of queue name"},
                              { "name.data", "data of queue name"},
                              { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                              { "xid.gtrid_length", "length of the transaction gtrid part"},
                              { "xid.bqual_length", "length of the transaction branch part"},
                              { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                              { "message.id", "id of the message"},
                              { "message.properties.size", "length of message properties"},
                              { "message.properties.data", "data of message properties"},
                              { "message.reply.size", "length of the reply queue"},
                              { "message.reply.data", "data of reply queue"},
                              { "message.available", "when the message is available for dequeue (us since epoc)"},
                              { "message.type.size", "length of the type string"},
                              { "message.type.data", "data of the type string"},
                              { "message.payload.size", "size of the payload"},
                              { "message.payload.data", "data of the payload"},
                           });
                  }

                  {
                     using message_type = common::message::queue::enqueue::Reply;

                     out << R"(
#### message::queue::enqueue::Reply

Represent enqueue reply.

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "id", "id of the enqueued message"},
                           });
                  }


                  {
                     using message_type = common::message::queue::dequeue::Request;

                     out << R"(
### dequeue 

#### message::queue::dequeue::Request

Represent dequeue request.

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     message.trid = common::transaction::id::create();
                     message.name = local::string::value( 128);


                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "name.size", "size of the queue name"},
                              { "name.data", "data of the queue name"},
                              { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                              { "xid.gtrid_length", "length of the transaction gtrid part"},
                              { "xid.bqual_length", "length of the transaction branch part"},
                              { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                              { "selector.properties.size", "size of the selector properties (ignored if empty)"},
                              { "selector.properties.data", "data of the selector properties (ignored if empty)"},
                              { "selector.id", "selector uuid (ignored if 'empty'"},
                              { "block", "dictates if this is a blocking call or not"},
                           });
                  }

                  {
                     using message_type = common::message::queue::dequeue::Reply;

                     out << R"(
#### message::queue::dequeue::Reply

Represent dequeue reply.

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     message.message.resize( 1);
                     message.message.at( 0).properties = local::string::value( 128);
                     message.message.at( 0).reply = local::string::value( 128);
                     message.message.at( 0).type = local::string::value( 128);
                     message.message.at( 0).payload = local::binary::value( 1024); 

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "message.size", "number of messages dequeued"},
                              { "message.element.id", "id of the message"},
                              { "message.element.properties.size", "length of message properties"},
                              { "message.element.properties.data", "data of message properties"},
                              { "message.element.reply.size", "length of the reply queue"},
                              { "message.element.reply.data", "data of reply queue"},
                              { "message.element.available", "when the message was available for dequeue (us since epoc)"},
                              { "message.element.type.size", "length of the type string"},
                              { "message.element.type.data", "data of the type string"},
                              { "message.element.payload.size", "size of the payload"},
                              { "message.element.payload.data", "data of the payload"},
                              { "message.element.redelivered", "how many times the message has been redelivered"},
                              { "message.element.timestamp", "when the message was enqueued (us since epoc)"},
                           });
                  }
               }

               void conversation( std::ostream& out)
               {
                  out << R"(
## conversation messages

### connect 

)";

                  {
                     using message_type = common::message::conversation::connect::callee::Request;

                     out << R"(
#### message::conversation::connect::Request

Sent to establish a conversation

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     message.service.name = local::string::value( 128);
                     message.parent = local::string::value( 128);
                     message.trid = common::transaction::id::create();
                     message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
                     message.buffer.memory = local::binary::value( 1024);
                     message.recording.nodes.resize( 1);

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "service.name.size", "size of the service name"},
                              { "service.name.data", "data of the service name"},
                              { "service.timeout", "timeout (in ns"},
                              { "parent.size", "size of the parent service name (the caller)"},
                              { "parent.data", "data of the parent service name (the caller)"},
                              { "xid.formatID", "xid format type. if 0 no more information of the xid is transported"},
                              { "xid.gtrid_length", "length of the transaction gtrid part"},
                              { "xid.bqual_length", "length of the transaction branch part"},
                              { "xid.data", "byte array with the size of gtrid_length + bqual_length (max 128)"},
                              { "flags", "xatmi flag"},
                              { "recording.nodes.size", "size of the recording of 'passed nodes'"},
                              { "recording.nodes.element.address", "'address' of a node'"},
                              { "buffer.type.size", "buffer type name size"},
                              { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                              { "buffer.memory.size", "buffer payload size (could be very big)"},
                              { "buffer.memory.data", "buffer payload data (with the size of buffer.payload.size)"},
                           });
                  }

                  {
                     using message_type = common::message::conversation::connect::Reply;

                     out << R"(
#### message::conversation::connect::Reply

Reply for a conversation

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     message.route.nodes.resize( 1);
                     message.recording.nodes.resize( 1);
   

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "route.nodes.size", "size of the established route"},
                              { "route.nodes.element.address", "'address' of a 'node' in the route"},
                              { "recording.nodes.size", "size of the recording of 'passed nodes'"},
                              { "recording.nodes.element.address", "'address' of a node'"},
                              { "code.result", "result code of the connection attempt"},
                           });
                  }

                  {
                     using message_type = common::message::conversation::callee::Send;

                     out << R"(
### send

#### message::conversation::Send

Represent a message sent 'over' an established connection

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     message.route.nodes.resize( 1);
                     message.buffer.type = local::string::value( 8) + '/' + local::string::value( 16);
                     message.buffer.memory = local::binary::value( 1024); 

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "route.nodes.size", "size of the established route"},
                              { "route.nodes.element.address", "'address' of a 'node' in the route"},
                              { "events", "events"},
                              { "code.result", "status of the connection"},
                              { "buffer.type.size", "buffer type name size"},
                              { "buffer.type.data", "byte array with buffer type in the form 'type/subtype'"},
                              { "buffer.memory.size", "buffer payload size (could be very big)"},
                              { "buffer.memory.data", "buffer payload data (with the size of buffer.payload.size)"},
                           });
                  }

                  {
                     using message_type = common::message::conversation::Disconnect;

                     out << R"(
### disconnect

#### message::conversation::Disconnect

Sent to abruptly disconnect the conversation

)";

                     local::message_type( out, message_type{}) << "\n\n";

                     message_type message;

                     message.route.nodes.resize( 1);

                     local::format::type( out, message, {
                              { "execution", "uuid of the current execution context (breadcrumb)"},
                              { "route.nodes.size", "size of the established route"},
                              { "route.nodes.element.address", "'address' of a 'node' in the route"},
                              { "events", "events"},
                           });
                  }
                  
               }



               

               void protocol()
               {
                  //static_assert( common::marshal::is_network_normalizing< local::Printer>::value, "not network...");

                  common::terminal::output::directive().color = false;
                  
                  message_header( std::cout);
                  domain_connect( std::cout);
                  domain_discovery( std::cout);
                  service_call( std::cout);
                  transaction( std::cout);
                  queue( std::cout);
                  conversation( std::cout);
               }
            } // print
         } // protocol
      } // documentation
   } // gateway
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::guard( []()
   {
      casual::gateway::documentation::protocol::print::protocol();
   });
}




