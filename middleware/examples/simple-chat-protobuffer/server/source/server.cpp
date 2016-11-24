//!
//! casual 
//!

#include "xatmi.h"
#include "chat.pb.h"
#include <cstdio> 
#include <sstream>

namespace casual
{

   namespace simple_chat_protobuffer
   {
      namespace server
      {

         extern "C"
         {
            static int _ch_id = 1;
             
            void simple_chat_protobuffer_echo( TPSVCINFO *info)
            {
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }
            
            void create_chat_room(TPSVCINFO *info)
            {
               // Receive message
               std::stringstream input_stream((std::string(info->data)));
               chat::CreateChatRoom create_chat_room_req;
               create_chat_room_req.ParseFromIstream(&input_stream);
               tpfree(info->data);
               // Create the channel
               chat::ChatRoom chat_room_resp;
               chat_room_resp.set_room_name(create_chat_room_req.room_name());
               chat_room_resp.set_creator_nick(create_chat_room_req.creator_nick());
               chat_room_resp.set_room_id(_ch_id++);
               // Send response message
               char* send_buf = tpalloc("CARRAY", NULL, chat_room_resp.ByteSize());
               std::stringstream output_stream((std::string(send_buf)));
               chat_room_resp.SerializeToOstream(&output_stream);
               tpreturn( TPSUCCESS, 0, send_buf, chat_room_resp.ByteSize(), 0);
            } // create_chat_room
         } // extern "C"
      } // server
   } // simple_chat_protobuffer
} // casual
