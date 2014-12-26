/*
 * state.cpp
 *
 *  Created on: 22 okt 2013
 *      Author: tomas
 */

#include <poll.h>

#include <cstring>
#include <string>
#include <utility>
#include <thread>
#include <condition_variable>
#include <memory>
#include <algorithm>

/*
 * Casual
 */
#include "common/ipc.h"
//#include "common/types.h"
#include "common/uuid.h"

#include "common/log.h"
#include "common/marshal/marshal.h"
#include "gateway/wire.h"
#include "gateway/std14.h"
#include "gateway/ipc.h"
#include "gateway/state.h"
#include "gateway/client.h"
#include "gateway/master.h"

/*
 * Network type and function definitions
 */
#include <inttypes.h>
#include <netinet/in.h>

namespace casual {

   namespace gateway {

      namespace configuration {

         /* Remote gateway comparator */
         bool RemoteGateway::operator==(std::string n)
         {
            return name == n;
         }

      }

   }
}



