//!
//! client.cpp
//!
//! Created on: Nov 5 2016
//!     Author: more10
//!

#include <vector>
#include <string>
#include <iostream>
#include <ios>
#include <streambuf>
#include <map>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <google/protobuf/message_lite.h>

#include <app_log.h>
#include "xatmi.h"
//#include "command.h"
//#include "buffer/string.h"

#include "client.h"
#include <chat.pb.h>
#include <peek.h>

namespace simple_chat_protobuffer {

   simple_chat_protobuffer::Client::Client(std::vector< std::string> arguments)
   {

      command_map["help"] = Command::Help;
      command_map["h"] = Command::Help;
      command_map["nick"] = Command::Nick;
      command_map["n"] = Command::Nick;
      command_map["create"] = Command::CreateChatRoom;
      command_map["c"] = Command::CreateChatRoom;
      command_map["enter"] = Command::EnterChatRoom;
      command_map["e"] = Command::EnterChatRoom;
      command_map["list"] = Command::ListChatRoom;
      command_map["l"] = Command::ListChatRoom;
      command_map["quit"] = Command::Quit;
      command_map["q"] = Command::Quit;
      command_map["message"] = Command::Message;
      command_map["m"] = Command::Message;
   }

   void simple_chat_protobuffer::Client::command_help(void) {
      //std::cout << "Commands:\n";
      std::cout << " h[elp]\n";
      std::cout << " n[ick] <nickname>\n";
      std::cout << " l[ist]\n";
      std::cout << " c[reate] <chatroom>\n";
      std::cout << " e[nter] [chatroom]\n";
      std::cout << " m[essage] <message>\n";
      std::cout << " q[uit]\n";
   }

   void simple_chat_protobuffer::Client::command_nick(std::vector<std::string> tokens) {
      if ( tokens.size() < 2 ) {
         std::cout << "Nick command needs a name \n";
         return;
      }
      nick = tokens[1];
      std::cout << "Nick is " << nick << "\n";
   }

   void simple_chat_protobuffer::Client::command_create(std::vector<std::string> tokens)
   {
      if ( nick.size() == 0 ) 
      {
         std::cout << "First you need to get a nick \n";
         return;
      }
      if ( tokens.size() < 2 ) 
      {
         std::cout << "Create chat room needs a room name \n";
         return;
      }
      chat_room = tokens[1];
      chat::CreateChatRoom ccr;
      ccr.set_creator_nick(nick);
      ccr.set_room_name(chat_room);
      casual::app::log::debug << "tpalloc ->" << std::endl;
      auto send_buffer = tpalloc("X_OCTET", 0, ccr.ByteSize());
      casual::app::log::debug << "tpalloc <-" << std::endl;

      if ( send_buffer == nullptr)
      {
         std::cout << "send_buffer == nullptr, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }      
      ccr.SerializeWithCachedSizesToArray((google::protobuf::uint8*)send_buffer);

      int cd1 = tpacall("casual.simple-chat-protobuffer.create-chat-room", send_buffer, ccr.ByteSize(), 0);
      tpfree(send_buffer);
      if ( cd1 == -1 ) 
      {
         casual::app::log::error << "tpacall(casual.simple-chat-protobuffer.create-chat-room) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         std::cout << "tpacall(casual.simple-chat-protobuffer.create-chat-room) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }

      long size = 0;
      auto receive_buffer = tpalloc("X_OCTET", 0, 0);
      int result = tpgetrply( &cd1, &receive_buffer, &size, 0);
      if ( result == -1 ) 
      {
         casual::app::log::error << "tpgetrply() failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         std::cout << "tpgetrply() failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);         
      }
      if ( size == 0 )
      {
         std::cout << "room name already exists" << std::endl;
         return;
      }
         
      chat::ChatRoom cr;
      cr.ParseFromArray(receive_buffer, size);
      tpfree(receive_buffer);
      std::cout << "Chat room name=" << cr.room_name() << " with id=" << cr.room_id() << " created by nick=" << cr.creator_nick() << "\n";     
      casual::app::log::debug << "Chat room name=" << cr.room_name() << " with id=" << cr.room_id() << " created by nick=" << cr.creator_nick() << "\n";     
   }
   
   void simple_chat_protobuffer::Client::command_enter(std::vector<std::string> tokens)
   {
      if ( nick.size() == 0 ) 
      {
         std::cout << "First you need to get a nick \n";
         return;
      }
      if ( chat_room.size() == 0 && tokens.size() == 1 ) 
      {
         std::cout << "You have not created a chat room in this session or given a name to the connect commmand \n";
         return;
      }
      if ( tokens.size() > 1 ) 
         chat_room = tokens[1];
      //std::cout << "Chat room " << chat_room << " entered\n";
      chat::EnterChatRoom enter_room_req;
      enter_room_req.set_nick(nick);
      enter_room_req.set_room_name(chat_room);
      auto send_buffer = tpalloc("X_OCTET", 0, enter_room_req.ByteSize());

      if ( send_buffer == nullptr)
      {
         std::cout << "send_buffer == nullptr, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }      
      enter_room_req.SerializeWithCachedSizesToArray((google::protobuf::uint8*)send_buffer);
      
      int cd1 = tpacall("casual.simple-chat-protobuffer.enter-chat-room", send_buffer, enter_room_req.ByteSize(), 0);
      if ( cd1 == -1 ) 
      {
         casual::app::log::error << "tpacall(casual.simple-chat-protobuffer.enter-chat-room) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         std::cout << "tpacall(casual.simple-chat-protobuffer.enter-chat-room) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }
      tpfree(send_buffer);

      long size = 0;
      auto receive_buffer = tpalloc("X_OCTET", 0, 0);
      tpgetrply( &cd1, &receive_buffer, &size, 0);
      if ( size == 0 )
      {
         std::cout << "No chat room named " << chat_room << std::endl;
         return;
      }
      chat::ChatRoomEntered response;
      response.ParseFromArray(receive_buffer, size);
      tpfree(receive_buffer);
      room_id = response.room_id();
      connected = true;
      message_id = response.message_id();
   }

   void simple_chat_protobuffer::Client::command_list(void)
   {
      int cd1 = tpacall("casual.simple-chat-protobuffer.list-chat-rooms", nullptr, 0, 0);
      if ( cd1 == -1 ) 
      {
         casual::app::log::error << "tpacall(casual.simple-chat-protobuffer.list-chat-rooms) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         std::cout << "tpacall(casual.simple-chat-protobuffer.list-chat-rooms) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }

      long size = 0;
      auto buffer = tpalloc("X_OCTET", 0, 0);
      tpgetrply( &cd1, &buffer, &size, 0);
         
      chat::ChatRooms chat_rooms;
      chat_rooms.ParseFromArray(buffer, size);
      tpfree(buffer);
      for ( int i=0; i<chat_rooms.chat_room_size(); i++) {
         chat::ChatRoom chat_room = chat_rooms.chat_room(i);
         std::cout << "Chat room " << chat_room.room_name() << " with id " << chat_room.room_id() << " created by " << chat_room.creator_nick() << "\n";     
      }
   }
   
   void simple_chat_protobuffer::Client::command_message(void) {
      if ( nick.size() == 0 ) {
         std::cout << "First you need to get a nick \n";
         return;
      }
      if ( !connected ) {
         std::cout << "Not connected to a chat room \n";
         return;                  
      }
      std::istringstream message_stream(command_str);
      std::string dummy;
      std::string message;
      std::getline(message_stream, dummy, ' ');
      std::getline(message_stream, message);
      if ( message.size() == 0 ) {
         std::cout << "No message to send! \n";
         return;                  
      }
      chat::ChatMessage chat_message_req;
      chat_message_req.set_message(message);
      chat_message_req.set_nick(nick);
      chat_message_req.set_chat_room(chat_room);
      chat_message_req.set_message_id(message_id);
   
      auto send_buffer = tpalloc("X_OCTET", 0, chat_message_req.ByteSize());
      casual::app::log::debug << "tpalloc <-" << std::endl;

      if ( send_buffer == nullptr)
      {
         std::cout << "send_buffer == nullptr, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }      
      chat_message_req.SerializeWithCachedSizesToArray((google::protobuf::uint8*)send_buffer);

      int cd1 = tpacall("casual.simple-chat-protobuffer.chat-message", send_buffer, chat_message_req.ByteSize(), 0);
      tpfree(send_buffer);
      if ( cd1 == -1 ) 
      {
         casual::app::log::error << "tpacall(casual.simple-chat-protobuffer.chat-message) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         std::cout << "tpacall(casual.simple-chat-protobuffer.chat-message) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }
      recive_messages_reply(cd1);
   }
   
   void simple_chat_protobuffer::Client::get_messages(void)
   {
      casual::app::log::debug << "get_messages called" << std::endl;
      chat::GetChatMessages chat_message_req;
      chat_message_req.set_chat_room(chat_room);
      chat_message_req.set_message_id(message_id);
   
      auto send_buffer = tpalloc("X_OCTET", 0, chat_message_req.ByteSize());

      if ( send_buffer == nullptr)
      {
         std::cout << "send_buffer == nullptr, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }      
      chat_message_req.SerializeWithCachedSizesToArray((google::protobuf::uint8*)send_buffer);

      int cd1 = tpacall("casual.simple-chat-protobuffer.get-chat-messages", send_buffer, chat_message_req.ByteSize(), 0);
      tpfree(send_buffer);
      if ( cd1 == -1 ) 
      {
         casual::app::log::error << "tpacall(casual.simple-chat-protobuffer.get-chat-messages) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         std::cout << "tpacall(casual.simple-chat-protobuffer.get-chat-messages) failed, tperrno=" << tperrnostring(tperrno) << std::endl;
         exit(1);
      }
      auto number_messages = recive_messages_reply(cd1);
      if ( number_messages )
         write_prompt();
   }
   
   int simple_chat_protobuffer::Client::recive_messages_reply(int& cd)
   {
      long size = 0;
      auto buffer = tpalloc("X_OCTET", 0, 0);
      tpgetrply( &cd, &buffer, &size, 0);
      if ( size == 0 )
         return 0;
      chat::ChatMessages chat_messages;
      chat_messages.ParseFromArray(buffer, size);
      tpfree(buffer);
      auto number_messages = chat_messages.chat_message_size();
      if ( number_messages == 0 )
      {
         casual::app::log::debug << "no messages" << std::endl;
         return 0;         
      }
      std::cout << "\r"; // overwrites prompt
      for ( int i=0; i<number_messages; i++) {
         chat::ChatMessage chat_message = chat_messages.chat_message(i);
         std::cout << chat_message.nick() << "@" << chat_message.chat_room() << ":" << chat_message.message() << std::endl;
         message_id = chat_message.message_id();
      }
      return number_messages;
   }

   void simple_chat_protobuffer::Client::write_prompt(void)
   {
      std::cout << nick << "@" << (connected ? chat_room : "") << "> " << std::flush;      
   }
   
   
   void simple_chat_protobuffer::Client::run(void) {
      write_prompt();
      while ( command != Command::Quit)
      {
         int c = casual::peek(300);
         if ( c == 0 ) {
            // No command being written
            if ( connected ) 
               get_messages(); // Get messages from server
            // Wait some in order to preserve CPU 
            //std::this_thread::sleep_for (std::chrono::seconds(1));
         }
         else 
         {
            // User entered a character get the command
            std::getline (std::cin, command_str);
            std::istringstream iss(command_str);
            std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                            std::istream_iterator<std::string>{}};
            casual::app::log::debug << "tokens.size()=" << tokens.size() << std::endl;
            if ( tokens.size() > 0 )
            {
               if ( command_map.count(tokens[0]) == 1 )
                  command = command_map[tokens[0]];
               else
                  command = Command::None;
               
               switch ( command )
               {
                  case Command::Help:
                     command_help();
                     break;
                  case Command::Nick:
                     command_nick(tokens);
                     break;
                  case Command::CreateChatRoom:
                     command_create(tokens);
                     break;
                  case Command::EnterChatRoom:
                     command_enter(tokens);
                     break;
                  case Command::ListChatRoom:
                     command_list();
                     break;
                  case Command::Message:
                     command_message();
                     break;
                  case Command::Quit:
                     std::cout << "Quitting\n";
                     break;
                  default:
                     std::cout << tokens[0] << " is not a command, try help \n";
                     break;
               }
            }
            write_prompt();
         }
      }      
   }

}

int main( int argc, char** argv)
{
   std::vector< std::string> arguments;
   std::copy(
      argv,
      argv + argc,
      std::back_inserter( arguments));
   if( arguments.size() < 1)
   {
      std::cerr << "need at least 0 argument" << std::endl;
      return 1;
   }
   
   simple_chat_protobuffer::Client client(arguments);
   client.run();
   

}
