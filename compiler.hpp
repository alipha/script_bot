#ifndef LIPH_COMPILER_HPP
#define LIPH_COMPILER_HPP

#include <string_view>
#include <vector>


std::string to_postfix(std::vector<std::string_view> token_list);
std::vector<char> compile(std::vector<std::string_view> token_list);


#endif

