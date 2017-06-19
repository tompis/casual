//!
//! casual 
//!

#include "gateway/outbound/routing.h"

#include "sf/platform.h"

#include "common/chronology.h"


#include <chrono>

namespace casual
{
   namespace gateway
   {
      namespace outbound
      {
         namespace routing
         {
            Point::Point( const common::Uuid& correlation, common::process::Handle destination, common::message::Type type)
            : correlation{ correlation}, destination{ destination}, type{ type} {}



            std::ostream& operator << ( std::ostream& out, const Point& value)
            {
               return out << "{ correlation: " << value.correlation
                     << ", destination: " << value.destination
                     << ", type: "<< value.type
                     << '}';
            }

         } // routing

         namespace service
         {
            namespace routing
            {
               Point::Point( const common::Uuid& correlation,
                  common::process::Handle destination,
                  std::string service,
                  common::platform::time::point::type start)
                  : correlation( correlation), destination( std::move( destination)),
                    service( std::move( service)), start( std::move( start))
               {

               }

               std::ostream& operator << ( std::ostream& out, const Point& value)
               {
                  return out << "{ correlation: " << value.correlation
                        << ", destination: " << value.destination
                        << ", service: "<< value.service
                        << ", start: "<< common::chronology::local( value.start) << "us"
                        << '}';
               }

            } // routing

         } // call
      } // outbound
   } // gateway
} // casual
