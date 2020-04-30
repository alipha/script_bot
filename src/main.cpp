#include "tokenizer.hpp"
#include "compiler.hpp"
#include "interpreter.hpp"
#include "irc.hpp"
#include "memory.hpp"
#include "string_util.hpp"

#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <boost/asio.hpp>


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


std::string run(compiler &c, interpreter &i, std::string_view code) {
    try {
        tokenizer t(std::string(code.data(), code.size()));
        return i.execute(c.compile(t.tokens()));
    } catch(std::exception &e) {
        return "Error: "s + e.what();
    }
}


void run_irc(compiler &c, interpreter &i) {
    irc_client irc;
    irc.login();

    while(true) {
        irc_message msg = irc.read();

        if(msg.action() != "PRIVMSG")
            continue;

        if(msg.sender_nick() == "Alipha" && msg.target() == "LiphBot") {
            irc.write(msg.message());
            if(starts_with(msg.message(), "QUIT"))
                return;
        } else if(starts_with(msg.message(), "!calc ")) {
            irc.write("PRIVMSG "s + msg.target() + " :" + msg.sender_nick() + ": " + 
                    run(c, i, msg.message().substr(6)));
        }
    }
}


int main(int argc, char* argv[]) {
    memory m;
    compiler c(&m, true);
    interpreter i(&m);

    if(argc > 1 && argv[1] == std::string_view("irc")) {
        std::time_t start_time = std::time(nullptr);
        int delay = 10;
        while(true) {
            try {
                start_time = std::time(nullptr);
                run_irc(c, i);
                return 0;
            } catch(boost::system::system_error &e) {
                std::cerr << "boost exception: " << e.what() << std::endl;
            }
            if(std::time(nullptr) - start_time > 600)
                delay = 10;
            std::this_thread::sleep_for(std::chrono::seconds(delay));
            delay *= 2;
        }
    }

    while(true) {
        try {
            std::string line;
            std::cout << "Input: ";
            std::getline(std::cin, line);

            if(line == "quit")
                return 0;

            tokenizer t(std::move(line));
            for(std::string_view token : t.tokens())
                std::cout << '"' << token << '"' << std::endl;

            std::cout << std::endl;

            std::vector<char> code = c.compile(t.tokens());

            std::cout << c.tokenized() << std::endl;
            std::cout << to_hex(code) << std::endl;
            std::cout << "Result: " << i.execute(code) << std::endl;
        } catch(std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

