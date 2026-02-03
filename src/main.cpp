#include <filesystem>
#include "kvstore/kvstore.h"

#include <iostream>
#include <sstream>
#include <string>

static void PrintHelp() {
  std::cout << "Commands:\n"
            << "  PUT <key> <value>\n"
            << "  GET <key>\n"
            << "  DEL <key>\n"
            << "  HELP\n"
            << "  EXIT\n";
}

int main() {
  std::filesystem::create_directories("data");
  kv::KVStore store("data/kv.aof");
  PrintHelp();

  std::string line;
  while (true) {
    std::cout << "> " << std::flush;
    if (!std::getline(std::cin, line)) break;

    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd.empty()) continue;

    if (cmd == "EXIT" || cmd == "quit" || cmd == "Quit") {
      break;
    } else if (cmd == "HELP") {
      PrintHelp();
    } else if (cmd == "PUT") {
      std::string key;
      iss >> key;
      std::string value;
      std::getline(iss, value);  // remainder (may include spaces)

      if (key.empty() || value.empty()) {
        std::cout << "ERR usage: PUT <key> <value>\n";
        continue;
      }

      // Trim leading space in value (from getline)
      if (!value.empty() && value[0] == ' ') value.erase(0, 1);

      store.Put(key, value);
      std::cout << "OK\n";
    } else if (cmd == "GET") {
      std::string key;
      iss >> key;
      if (key.empty()) {
        std::cout << "ERR usage: GET <key>\n";
        continue;
      }
      auto v = store.Get(key);
      if (!v) {
        std::cout << "(nil)\n";
      } else {
        std::cout << *v << "\n";
      }
    } else if (cmd == "DEL") {
      std::string key;
      iss >> key;
      if (key.empty()) {
        std::cout << "ERR usage: DEL <key>\n";
        continue;
      }
      bool deleted = store.Del(key);
      std::cout << (deleted ? "1\n" : "0\n");
    } else {
      std::cout << "ERR unknown command. Type HELP.\n";
    }
  }

  return 0;
}
