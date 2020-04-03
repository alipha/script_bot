#include "tokenizer.hpp"
#include "compiler.hpp"
#include <iostream>


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
    }

    return 0;
}
