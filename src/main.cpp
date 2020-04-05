#include "tokenizer.hpp"
#include "compiler.hpp"
#include "interpreter.hpp"

#include <boost/asio.hpp>
#include <iostream>


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


int main() {
    while(true) {
        std::string line;
        std::cout << "Input: ";
        std::getline(std::cin, line);

        tokenizer t(std::move(line));
        for(std::string_view token : t.tokens())
            std::cout << '"' << token << '"' << std::endl;

        std::cout << std::endl;
        std::cout << to_postfix(t.tokens()) << std::endl;
        std::cout << to_hex(compile(t.tokens())) << std::endl;
        std::cout << "Result: " << execute(compile(t.tokens())) << std::endl;
    }

    return 0;
}
