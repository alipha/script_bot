#ifndef LIPH_COMPILER_HPP
#define LIPH_COMPILER_HPP

#include "operation_type.hpp"

#include <string>
#include <string_view>
#include <vector>


class memory;


class compiler {
public:
    compiler(memory *m) : mem(m) {}

    std::string to_postfix(std::vector<std::string_view> token_list);
    std::vector<char> compile(std::vector<std::string_view> token_list);

private:
    static std::string parse_str_literal(std::string_view str);
    static bool has_lower_precedence(op_code current_code, op_code top_code);

    template<typename ResultType, typename Accumulator>
    static ResultType to_postfix_impl(std::vector<std::string_view> token_list, Accumulator accum);

    memory *mem;
};


#endif

