//!
//! casual 
//!

#include <cstdio> 
#include <sstream>
#include <iostream>
#include <vector>
#include "xatmi.h"
#include "chat.pb.h"
#include <app_log.h>

namespace casual
{

   namespace simple_chat_protobuffer
   {
      namespace server
      {

         extern "C"
         {
            static int _room_id = 1; // 0 is used to represent non existing room
            static std::map <std::string, chat::ChatRoom> _chat_room_map;
            static std::vector<int> _message_ids = {-1};
            static std::vector<std::vector<chat::ChatMessage>> _messages;
            static chat::ChatRoom _null_chat_room;
            
            void create_chat_room(TPSVCINFO *info)
            {
               casual::app::log::debug << "create_chat_room called" << std::endl;
               // Receive message
               chat::CreateChatRoom create_chat_room_req;
               create_chat_room_req.ParseFromArray(info->data, info->len);
               
               auto room_name = create_chat_room_req.room_name();
               tpfree(info->data);
               
               casual::app::log::debug << "create_chat_room_req.room_name()=" << room_name << std::endl;

               // Create the chat room
               char* buffer = nullptr;
               if ( _chat_room_map.count(room_name) == 1 ) 
               {
                  casual::app::log::debug << "room exists" << std::endl;
                  tpreturn(TPFAIL, 0, 0, 0, 0); // room exists
               }
               // new room
               _message_ids.push_back(0); // allocate the first message id for the room
               chat::ChatRoom chat_room_resp;
               chat_room_resp.set_room_name(room_name);
               chat_room_resp.set_creator_nick(create_chat_room_req.creator_nick());
               chat_room_resp.set_room_id(_room_id++);
               _chat_room_map[room_name] = chat_room_resp;
               buffer = tpalloc("X_OCTET", 0, chat_room_resp.ByteSize());
               // Send response message
               if ( buffer == nullptr)
               {
                  casual::app::log::error << "tpalloc failed with error=" << tperrnostring(tperrno) << std::endl;
                  tpreturn(TPSUCCESS, 0, 0, 0, 0); // TPEXIT
               }      
               chat_room_resp.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buffer);
               tpreturn(TPSUCCESS, 0, buffer, chat_room_resp.ByteSize(), 0);
            } // create_chat_room
            
            
            void list_chat_rooms(TPSVCINFO *info)
            {
               // Receive message
               tpfree(info->data);

               // List the chat rooms
               chat::ChatRooms chat_rooms_resp;
               for (auto & it : _chat_room_map) 
                  *chat_rooms_resp.add_chat_room() = it.second;
                 
               // Send response message
               char* buffer = tpalloc("X_OCTET", NULL, chat_rooms_resp.ByteSize());
               if ( buffer == nullptr)
               {
                  casual::app::log::error << "tpalloc failed with error=" << tperrnostring(tperrno) << std::endl;
                  exit(1);
               }      
               chat_rooms_resp.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buffer);
               tpreturn( TPSUCCESS, 0, buffer, chat_rooms_resp.ByteSize(), 0);
            } // list_chat_rooms
            
            
            void enter_chat_room(TPSVCINFO *info)
            {
               // Receive message
               chat::EnterChatRoom enter_room_req;
               enter_room_req.ParseFromArray(info->data, info->len);
               tpfree(info->data);

               // Find the room if it exists and return its id with the current message id
               auto room_name = enter_room_req.room_name();
               auto room_exists = _chat_room_map.count(room_name) == 1;
               const chat::ChatRoom& chat_room = room_exists? _chat_room_map[room_name] : _null_chat_room;
               auto room_id = chat_room.room_id();
               auto message_id = room_exists ? _message_ids[room_id] : -1;
               chat::ChatRoomEntered enter_room_resp;
               enter_room_resp.set_room_id(room_id);
               enter_room_resp.set_message_id(message_id);
                               
               // Send response message
               char* buffer = tpalloc("X_OCTET", NULL, enter_room_resp.ByteSize());
               if ( buffer == nullptr)
               {
                  casual::app::log::error << "tpalloc failed with error=" << tperrnostring(tperrno) << std::endl;
                  tpreturn( TPFAIL, 0, 0, 0, 0); // should retur TPEXIT
                  return;
               }      
               enter_room_resp.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buffer);
               tpreturn( TPSUCCESS, 0, buffer, enter_room_resp.ByteSize(), 0);
            } // list_chat_rooms

            void chat_message(TPSVCINFO *info)
            {
               // Receive message
               chat::ChatMessage chat_message_req;
               chat_message_req.ParseFromArray(info->data, info->len);
               tpfree(info->data);

               // Give message an id and store it
               auto room_id = chat_message_req.room_id();
               auto message_id = _message_ids[room_id];
               auto prev_message_id = chat_message_req.message_id();
               chat_message_req.set_message_id(message_id);
               _messages[room_id][message_id] = chat_message_req;
               _message_ids[room_id] += 1;
               
               // Create response message
               chat::ChatMessages chat_messages_resp;
               for ( int i=prev_message_id; i<=message_id; i++) 
                  *chat_messages_resp.add_chat_message() = _messages[room_id][i];
               
               // Send response message
               char* buffer = tpalloc("X_OCTET", NULL, chat_messages_resp.ByteSize());
               if ( buffer == nullptr)
               {
                  casual::app::log::error << "tpalloc failed with error=" << tperrnostring(tperrno) << std::endl;
                  exit(1);
               }      
               chat_messages_resp.SerializeWithCachedSizesToArray((google::protobuf::uint8*)buffer);
               tpreturn( TPSUCCESS, 0, buffer, chat_messages_resp.ByteSize(), 0);
            } // chat_message
            
         } // extern "C"
      } // server
   } // simple_chat_protobuffer
} // casual
