//!
//! service_implementation.h
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#ifndef SERVICE_IMPLEMENTATION_H_
#define SERVICE_IMPLEMENTATION_H_

#include "sf/service/interface.h"
#include "sf/service/model.h"

#include "sf/archive/yaml.h"
#include "sf/archive/binary.h"
#include "sf/archive/json.h"
#include "sf/archive/xml.h"
#include "sf/archive/log.h"
#include "sf/archive/service.h"
#include "sf/log.h"

namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace protocol
         {
            class Base : public Interface
            {

            public:
               Base( TPSVCINFO* serviceInfo);

               Base( Base&&);

            private:

               bool doCall() override;

               reply::State doFinalize() override;

               void doHandleException() override;

               Interface::Input& doInput() override;

               Interface::Output& doOutput() override;


            protected:

               TPSVCINFO* m_info;
               reply::State m_state;

               Interface::Input m_input;
               Interface::Output m_output;
            };


            class Binary : public Base
            {
            public:
               Binary( TPSVCINFO* serviceInfo);

               static std::vector< buffer::Type> types();

               reply::State doFinalize() override;

            private:

               buffer::binary::Stream m_readerBuffer;
               archive::binary::Reader m_reader;

               buffer::binary::Stream m_writerBuffer;
               archive::binary::Writer m_writer;

            };

            class Yaml : public Base
            {
            public:

               Yaml( TPSVCINFO* serviceInfo);

               reply::State doFinalize() override;

               static std::vector< buffer::Type> types();

            private:

               archive::yaml::Load m_load;
               archive::yaml::Reader m_reader;
               archive::yaml::Save m_save;
               archive::yaml::Writer m_writer;
            };

            class Json : public Base
            {
            public:

               Json( TPSVCINFO* serviceInfo);

               reply::State doFinalize() override;

               static std::vector< buffer::Type> types();


            private:

               archive::json::Load m_load;
               archive::json::Reader m_reader;
               archive::json::Save m_save;
               archive::json::Writer m_writer;
            };

            class Xml : public Base
            {
            public:

               Xml( TPSVCINFO* serviceInfo);

               reply::State doFinalize() override;

               static std::vector< buffer::Type> types();

            private:

               archive::xml::Load m_load;
               archive::xml::Reader m_reader;
               archive::xml::Save m_save;
               archive::xml::Writer m_writer;
            };



            namespace parameter
            {
               template< typename B>
               class Log : public B
               {
               public:
                  using base_type = B;

                  Log( TPSVCINFO* serviceInfo) : base_type( serviceInfo), m_writer( log::parameter)
                  {
                     this->m_input.writers.push_back( &m_writer);
                     this->m_output.writers.push_back( &m_writer);
                  }

               private:

                  archive::log::Writer m_writer;

               };
            } // parameter

            class Describe : public Base
            {
            public:

               Describe( TPSVCINFO* information);

               static std::vector< buffer::Type> types();

            private:

               static std::unique_ptr< Interface> protocoll( TPSVCINFO* information);

               bool doCall() override;
               reply::State doFinalize() override;

               Model m_model;

               archive::service::Prepare m_prepare;

               struct writer_t
               {
                  writer_t( Model& model) : input( model.arguments.input), output( model.arguments.output) {}

                  archive::service::Writer input;
                  archive::service::Writer output;
               } m_writer;

               std::unique_ptr< Interface> m_protocoll;

            };

         } // protocol
      } // service
   } // sf
} // casual


#endif /* SERVICE_IMPLEMENTATION_H_ */
