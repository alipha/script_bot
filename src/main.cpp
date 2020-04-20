#include "tokenizer.hpp"
#include "compiler.hpp"
#include "interpreter.hpp"
#include "irc.hpp"
#include "memory.hpp"
#include "string_util.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>


using namespace std::string_literals;


std::string to_hex(const std::vector<char> &bytes) {
    const char *hex_chars = "0123456789abcdef";
    std::string result; //(bytes.size() * 2, '\0');

    for(auto ch : bytes) {
        result += hex_chars[(ch >> 4) & 0xf];
        result += hex_chars[ch & 0xf];
        result += ' ';
    }

    return result;
}


std::string run(compiler &c, interpreter &i, std::string_view program) {
    try {
        return i.execute(c.compile(program));
    } catch(std::exception &e) {
        return "Error: "s + e.what();
    }
}


int main(int argc, char* argv[]) {
    memory m;
    tokenizer t;
    compiler c(&m, &t, true);
    interpreter i(&m);

    if(argc > 1 && argv[1] == std::string_view("irc")) {
        irc_client irc;
        irc.login();

        while(true) {
            irc_message msg = irc.read();

            if(msg.action() != "PRIVMSG")
                continue;

            if(msg.sender_nick() == "Alipha" && msg.target() == "LiphBot") {
                irc.write(msg.message());
            } else if(starts_with(msg.message(), "!calc ")) {
                irc.write("PRIVMSG "s + msg.target() + " :" + msg.sender_nick() + ": " + 
                        run(c, i, msg.message().substr(6)));
            }
        }
    }

    while(true) {
        std::string line;
        std::cout << "Input: ";
        std::getline(std::cin, line);

        tokenizer t(line);
        for(auto [token, pos] : t)
            std::cout << '"' << token << '"' << std::endl;

        std::cout << std::endl;

        std::vector<char> program = c.compile(std::move(line));

        std::cout << c.tokenized() << std::endl;
        std::cout << to_hex(program) << std::endl;
        std::cout << "Result: " << i.execute(program) << std::endl;
    }

    return 0;
}
