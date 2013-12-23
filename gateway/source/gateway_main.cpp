//!
//! gateway_main.cpp
//!
//! Created on: Okt 1, 2013
//!     Author: dnulnets
//!

#include <poll.h>

/*
** Common casual headerfiles
*/
#include "common/error.h"
#include "common/arguments.h"
#include "common/logger.h"
#include "common/file.h"
#include "common/uuid.h"
#include "common/queue.h"

#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/listener.h"
#include "gateway/state.h"
#include "gateway/gateway.h"
#include "gateway/master.h"
#include "gateway/client.h"

/*
** Standard headerfiles
*/
#include <iostream>

/*
** Default namespace
*/
using namespace casual;

/*
** Main
*/
int main( int argc, char** argv)
{
   common::logger::information << "Gateway starting";

   try
   {
      gateway::Settings settings;

      {
         common::Arguments parser;

         parser.add(
               common::argument::directive( {"-c", "--configuration-file"}, "gateway configuration file", settings.configurationFile)
         );

         parser.parse( argc, argv);

         common::environment::file::executable( parser.processName());
      }

      gateway::Gateway::instance().start( settings);

   } catch( ...) {
      return casual::common::error::handler();

   }
   return 0;
}
