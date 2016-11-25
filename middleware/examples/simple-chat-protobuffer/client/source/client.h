#ifndef CLIENT_INCLUDED
#define CLIENT_INCLUDED

#include <vector>
#include <string>

namespace simple_chat_protobuffer {
   enum Command { None, Help, Nick, CreateChatRoom, EnterChatRoom, ListChatRoom, Message, Quit };

   class Client {
   private:
      Command command = Command::None;
      std::map <std::string, Command> command_map;
      std::string command_str = "";
      std::string nick = "";
      std::string chat_room = "";
      bool connected = false;
   public:
      Client(std::vector< std::string> arguments);
      void run(void);
      void command_help(void);
      void command_nick(std::vector<std::string> tokens);
      void command_create(std::vector<std::string> tokens);
      void command_enter(std::vector<std::string> tokens);
      void command_list(void);
   };

}

#endif