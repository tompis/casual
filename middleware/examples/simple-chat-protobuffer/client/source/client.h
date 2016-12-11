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
      int room_id = -1;
      int message_id = -1;
      bool connected = false;
      void command_help(void);
      void command_nick(std::vector<std::string> tokens);
      void command_create(std::vector<std::string> tokens);
      void command_enter(std::vector<std::string> tokens);
      void command_list(void);
      void command_message(void);
      void get_messages(void);
      void write_prompt(void);
      int recive_messages_reply(int& cd);
   public:
      Client(std::vector< std::string> arguments);
      void run(void);
   };

}

#endif