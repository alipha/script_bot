#ifndef LIPH_INTERPRETER_HPP
#define LIPH_INTERPRETER_HPP

#include <cstdint>
#include <variant>
#include <vector>
#include <string>

using value_type = std::variant<std::int32_t, double>;


std::string execute(const std::vector<char> &program);

#endif

