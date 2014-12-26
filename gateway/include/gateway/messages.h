/*
 * messages.h
 *
 *  Created on: 17 dec 2013
 *      Author: tomas
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <sys/types.h>
#include <string.h>

/*
 * STL
 */
#include <string>


/*
 * Casual common
 */
#include "common/log.h"
#include "common/marshal/marshal.h"
#include "gateway/wire.h"
#include "gateway/std14.h"

/*
 * Casual namespace
 */
namespace casual
{
   namespace gateway
   {
      namespace message {

         /* Message types */
         static const int type_registration = 1;

         /*
          * The header for all messages
          */
         struct Header {

            unsigned int id;  /* Id of the message */
            unsigned int type; /* Type of message */
            std::string from; /* UUID of the sender */

            /*
             * Marshaller
             */
            template<typename A>
            void marshal( A& archive)
            {
               archive & id;
               archive & type;
               archive & from;
            }

         };

         /*
          * Registration message
          */
         struct Registration {

            unsigned int id;  /* Id of the message */
            unsigned int type; /* Type of message */
            std::string from; /* UUID of the sender */

            /* The body */
            std::string name; /* Name of the client that wants to register */

            /*
            * Marshaller
            */
            template<typename A>
            void marshal( A& archive)
            {
               archive & id;
               archive & type;
               archive & from;
               archive & name;
            }
         };
      }
   }
}

#endif /* MESSAGES_H_ */
