//!
//! client.cpp
//!
//! Created on: Nov 5 2016
//!     Author: more10
//!

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "xatmi.h"
//#include "command.h"
//#include "buffer/string.h"

#include "client.h"

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
      std::cout << "Chat room " << chat_room << " created\n";     
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
      std::cout << "Chat room " << chat_room << " entered\n";
      connected = true;      
   }

   void simple_chat_protobuffer::Client::command_list(void)
   {
      std::cout << "List chat rooms\n";      
   }
   
   void simple_chat_protobuffer::Client::run(void) {
      while ( command != Command::Quit)
      {
         std::cout << nick << "@" << (connected ? chat_room : "") << "> ";
         int p = std::cin.peek();
         if ( p == EOF ) {
            // No cammand being written
            if ( connected ) {
               // Get a message from server
            }
            else {
               // Wait some in order to preserve CPU 
               std::this_thread::sleep_for (std::chrono::seconds(1));
            }
         }
         else {
            // User entered a character, lets get the command
            std::getline (std::cin, command_str);
            std::istringstream iss(command_str);
            std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                            std::istream_iterator<std::string>{}};
            
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
                  {
                     if ( nick.size() == 0 ) {
                        std::cout << "First you need to get a nick \n";
                        break;
                     }
                     if ( !connected ) {
                        std::cout << "Not connected to a chat room \n";
                        break;                  
                     }
                     std::istringstream message_stream(command_str);
                     std::string dummy;
                     std::string message;
                     std::getline(message_stream, dummy, ' ');
                     std::getline(message_stream, message);
                     if ( message.size() == 0 ) {
                        std::cout << "No message to send! \n";
                        break;                  
                     }
                     std::cout << "Sending message " << message << "\n";
                     break;
                  }
                  case Command::Quit:
                     std::cout << "Quitting\n";
                     break;
                  default:
                     std::cout << tokens[0] << " is not a command, try help \n";
                     break;
               }
            }
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
   

/*
	auto buffer = tpalloc( CASUAL_STRING, 0, 1024);

	if ( buffer != nullptr)
	{
      const std::string& argument = arguments[ 1];

      std::copy( argument.begin(), argument.end(), buffer);
      buffer[ argument.size()] = '\0';

      long size = 0;
      int cd1 = tpacall( "casual_test1", buffer, 0, 0);
      int cd2 = tpacall( "casual_test2", buffer, 0, 0);

      tpgetrply( &cd1, &buffer, &size, 0);
      std::cout << std::endl << "reply1: " << buffer << std::endl;

      tpgetrply( &cd2, &buffer, &size, 0);
      std::cout << std::endl << "reply2: " << buffer << std::endl;

      tpfree( buffer);
	}
	else
	{
	   std::cout << tperrnostring(tperrno) << std::endl;
	}
    * */
}
