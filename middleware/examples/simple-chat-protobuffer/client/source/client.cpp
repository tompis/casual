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

enum Command { None, Help, Nick, CreateChatRoom, ConnectChatRoom, ListChatRoom, Message, Quit };

int main( int argc, char** argv)
{
   Command command = Command::None;
	std::vector< std::string> arguments;
   std::map <std::string, Command> command_map;
   std::string command_str = "";
   std::string nick = "";
   std::string chat_room = "";
   bool connected = false;

   command_map["help"] = Command::Help;
   command_map["nick"] = Command::Nick;
   command_map["create"] = Command::CreateChatRoom;
   command_map["connect"] = Command::ConnectChatRoom;
   command_map["list"] = Command::ListChatRoom;
   command_map["quit"] = Command::Quit;
   command_map["message"] = Command::Message;

	std::copy(
		argv,
		argv + argc,
		std::back_inserter( arguments));

	if( arguments.size() < 1)
	{
	   std::cerr << "need at least 0 argument" << std::endl;
	   return 1;
	}
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
                  std::cout << "Commands:\n";
                  std::cout << " help\n";
                  std::cout << " nick <nickname>\n";
                  std::cout << " create <chatroom>\n";
                  std::cout << " connect [chatroom]\n";
                  std::cout << " message <message>\n";
                  std::cout << " quit\n";
                  break;
               case Command::Nick:
                  if ( tokens.size() < 2 ) {
                     std::cout << "Nick command needs a name \n";
                     break;
                  }
                  nick = tokens[1];
                  std::cout << "Nick is " << nick << "\n";
                  break;
               case Command::CreateChatRoom:
               {
                  if ( nick.size() == 0 ) {
                     std::cout << "First you need to get a nick \n";
                     break;
                  }
                  if ( tokens.size() < 2 ) {
                     std::cout << "Create chat room needs a room name \n";
                     break;
                  }
                  chat_room = tokens[1];
                  std::cout << "Chat room " << chat_room << " created\n";
                  break;
               }
               case Command::ConnectChatRoom:
               {
                  if ( nick.size() == 0 ) {
                     std::cout << "First you need to get a nick \n";
                     break;
                  }
                  if ( chat_room.size() == 0 && tokens.size() == 1 ) {
                     std::cout << "You have not created a chat room in this session or given a name to the connect commmand \n";
                     break;                  
                  }
                  if ( tokens.size() > 1 ) 
                     chat_room = tokens[1];
                  std::cout << "Chat room " << chat_room << " connected\n";
                  connected = true;
                  break;
               }
               case Command::ListChatRoom:
               {
                  std::cout << "List chat rooms\n";
                  break;
               }
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
