#ifndef LIPH_COMPILER_HPP
#define LIPH_COMPILER_HPP

#include "operation_type.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
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

    std::uint8_t local_var_index(std::string_view name);

    template<typename ResultType, typename Accumulator>
    std::size_t to_postfix_impl(std::vector<std::string_view> token_list, ResultType &result, Accumulator accum);

    memory *mem;
    std::unordered_map<std::string, std::uint8_t> local_var_indexes;
};


#endif

