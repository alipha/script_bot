#include "tokenizer.hpp"
#include "compiler.hpp"
#include "interpreter.hpp"
#include "irc.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "settings.hpp"
#include "string_util.hpp"

#include <chrono>
#include <ctime>
#include <iostream>
#include <memory>
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
        return i.execute(c.compile(t.tokens(), t.source()));
    } catch(std::exception &e) {
        return "Error: "s + e.what();
    }
}


void run_irc(settings &s, compiler &c, interpreter &i) {
    irc_client irc(s);
    irc.login();

    while(true) {
        irc_message msg = irc.read();

        if(msg.action() != "PRIVMSG")
            continue;

        if(msg.is_admin_msg()) {
            irc.write(msg.message());
            if(starts_with(msg.message(), "QUIT"))
                return;
        } else if(starts_with(msg.message(), "!calc ")) {
            irc.write("PRIVMSG "s + msg.target() + " :" + msg.sender_nick() + ": " + 
                    run(c, i, msg.message().substr(6)));
            std::size_t mem_used = gc::get_memory_used();
            if(mem_used > 0)
                std::cout << "Memory Used: " << mem_used << std::endl;
        }
    }
}


int main(int argc, char* argv[]) {
    settings setting("settings.txt");
    memory m;
    compiler c(&m, true);
    interpreter i(&m);

    std::string_view max_memory = setting.first("max_memory").value_or("100000000");
    gc::set_memory_limit(std::stoul(std::string(max_memory)));

    if(argc > 1 && argv[1] == std::string_view("irc")) {
        std::time_t start_time = std::time(nullptr);
        int delay = 10;
        while(true) {
            try {
                start_time = std::time(nullptr);
                run_irc(setting, c, i);
                return 0;
            } catch(boost::system::system_error &e) {
                std::cerr << "boost exception: " << e.what() << std::endl;
            }
            if(std::time(nullptr) - start_time > 600)
                delay = 9;
            std::this_thread::sleep_for(std::chrono::seconds(delay));
            delay *= 2;
        }
    }

    while(true) {
        try {
            //gc::collect();
            std::string line;
            std::cout << "Input: ";
            std::getline(std::cin, line);

            if(line == "quit")
                return 0;

            tokenizer t(std::move(line));
            for(symbol token : t.tokens())
                std::cout << '"' << token.token << '"' << std::endl;

            std::cout << std::endl;

            std::shared_ptr<func_def> code = c.compile(t.tokens(), t.source());

            std::cout << c.tokenized() << std::endl;
            std::cout << to_hex(code->code) << std::endl;
            std::cout << "Result: " << i.execute(std::move(code)) << std::endl;
        } catch(std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

