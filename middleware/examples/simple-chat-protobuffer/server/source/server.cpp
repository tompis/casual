//!
//! casual 
//!

#include <cstdio> 
#include <sstream>
#include <iostream>
#include <common/internal/log.h>
#include "xatmi.h"
#include "chat.pb.h"

namespace casual
{

   namespace simple_chat_protobuffer
   {
      namespace server
      {

         extern "C"
         {
            static int _ch_id = 1;
            static std::map <std::string, chat::ChatRoom> chat_room_map;
             
            void simple_chat_protobuffer_echo( TPSVCINFO *info)
            {
               tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
            }
            
            void create_chat_room(TPSVCINFO *info)
            {
               // Receive message
               chat::CreateChatRoom create_chat_room_req;
               create_chat_room_req.ParseFromArray(info->data, info->len);
               tpfree(info->data);

               // Create the chat room
               chat::ChatRoom chat_room_resp;
               chat_room_resp.set_room_name(create_chat_room_req.room_name());
               chat_room_resp.set_creator_nick(create_chat_room_req.creator_nick());
               chat_room_resp.set_room_id(_ch_id++);
               chat_room_map[chat_room_resp.room_name()] = chat_room_resp;
               
               // Send response message
               char* buffer = tpalloc("X_OCTET", NULL, chat_room_resp.ByteSize());
               if ( buffer == nullptr)
               {
                  casual::common::log::internal::debug << "tpalloc failed with error=" << tperrnostring(tperrno) << std::endl;
                  exit(1);
               }      
               chat_room_resp.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buffer);
               tpreturn( TPSUCCESS, 0, buffer, chat_room_resp.ByteSize(), 0);
            } // create_chat_room
            
            void list_chat_rooms(TPSVCINFO *info)
            {
               // Receive message
               tpfree(info->data);

               // List the chat rooms
               chat::ChatRooms chat_rooms_resp;
               for (auto & it : chat_room_map) 
                  *chat_rooms_resp.add_chat_room() = it.second;
                 
               // Send response message
               char* buffer = tpalloc("X_OCTET", NULL, chat_rooms_resp.ByteSize());
               if ( buffer == nullptr)
               {
                  casual::common::log::internal::debug << "tpalloc failed with error=" << tperrnostring(tperrno) << std::endl;
                  exit(1);
               }      
               chat_rooms_resp.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buffer);
               tpreturn( TPSUCCESS, 0, buffer, chat_rooms_resp.ByteSize(), 0);
            } // list_chat_rooms            
         } // extern "C"
      } // server
   } // simple_chat_protobuffer
} // casual
