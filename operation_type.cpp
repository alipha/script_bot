#include "operation_type.hpp"
#include "util.hpp"

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>


operation_type operation_types[] = {
    {"",    true,  0, associative::left,    0, op_code::none},
    {"**",  true,  2, associative::left,  575, op_code::pow},
    {"*",   true,  2, associative::left,  550, op_code::mul},
    {"/",   true,  2, associative::left,  550, op_code::div},
    {"%",   true,  2, associative::left,  550, op_code::mod},
    {"+",   true,  2, associative::left,  500, op_code::add},
    {"-",   true,  2, associative::left,  500, op_code::sub},
    {"<<",  true,  2, associative::left,  475, op_code::shl},
    {">>",  true,  2, associative::left,  475, op_code::shr},
    {"<",   true,  2, associative::left,  450, op_code::lt},
    {"<=",  true,  2, associative::left,  450, op_code::lte},
    {">",   true,  2, associative::left,  450, op_code::gt},
    {">=",  true,  2, associative::left,  450, op_code::gte},
    {"==",  true,  2, associative::left,  450, op_code::eq},
    {"!=",  true,  2, associative::left,  450, op_code::neq},
    {"&",   true,  2, associative::left,  375, op_code::bit_and},
    {"^",   true,  2, associative::left,  350, op_code::bit_xor},
    {"|",   true,  2, associative::left,  325, op_code::bit_or},
    {"&&",  true,  2, associative::left,  300, op_code::logic_and},
    {"||",  true,  2, associative::left,  275, op_code::logic_or},
    {"-",   false, 1, associative::right, 600, op_code::negate},
    {"!",   false, 1, associative::right, 600, op_code::logic_not},
    {"~",   false, 1, associative::right, 600, op_code::bit_not},
    {"++",  false, 1, associative::right, 600, op_code::pre_inc},
    {"--",  false, 1, associative::right, 600, op_code::pre_dec},
    {"++",  true,  1, associative::left,  700, op_code::post_inc},
    {"--",  true,  1, associative::left,  700, op_code::post_dec},
    {".",   true,  2, associative::left,  900, op_code::dot},
    {"=",   true,  2, associative::right, 200, op_code::assign},
    {"+=",  true,  2, associative::right, 200, op_code::add_assign},
    {"-=",  true,  2, associative::right, 200, op_code::sub_assign},
    {"*=",  true,  2, associative::right, 200, op_code::mul_assign},
    {"/=",  true,  2, associative::right, 200, op_code::div_assign},
    {"%=",  true,  2, associative::right, 200, op_code::mod_assign},
    {"&=",  true,  2, associative::right, 200, op_code::and_assign},
    {"^=",  true,  2, associative::right, 200, op_code::xor_assign},
    {"|=",  true,  2, associative::right, 200, op_code::or_assign},
    {"<<=", true,  2, associative::right, 200, op_code::shl_assign},
    {">>=", true,  2, associative::right, 200, op_code::shr_assign},
    {"(",   true,  2, associative::left,  100, op_code::func_call},
    {"(",   false, 1, associative::left,  100, op_code::left_paren},
    {")",   true,  1, associative::left,  101, op_code::right_paren},
    {"[",   true,  2, associative::left,  100, op_code::array_index},
    {"[",   false, 1, associative::left,  100, op_code::array_start},
    {"]",   true,  1, associative::left,  101, op_code::array_end},
    {"{",   false, 1, associative::left,  100, op_code::map_start},
    {"}",   true,  1, associative::left,  101, op_code::map_end}
};


std::map<std::pair<std::string_view, bool>, operation_type> load_operation_types() {
    auto &types = operation_types;

    static_assert(sizeof(types) / sizeof(*types) == static_cast<std::size_t>(op_code::count), 
            "Missing op_code to operation_type mapping");

    std::map<std::pair<std::string_view, bool>, operation_type> type_map;
    int code_index = 0;

    for(auto &t : types) {
        if(t.code != static_cast<op_code>(code_index++))
            throw std::logic_error(t.symbol + std::string(" is not in the correct order in operation_types. Index: " + std::to_string(code_index - 1)));

        type_map[std::pair(t.symbol, t.in_binary_context)] = t;
    }

    return type_map;
}


std::map<std::pair<std::string_view, bool>, operation_type> symbol_operation_map = load_operation_types();


operation_type lookup_operation(std::string_view symbol, bool in_binary_context) {
    auto it = symbol_operation_map.find(std::pair(symbol, in_binary_context));
    if(it == symbol_operation_map.end())
        return operation_types[static_cast<int>(op_code::none)];
    else
        return it->second;
}


operation_type lookup_operation(op_code code) {
    int code_index = static_cast<int>(code);
    if(code_index >= static_cast<int>(op_code::count))
        throw std::out_of_range("op_code >= op_code::count. Value: " + std::to_string(code_index));
    return operation_types[code_index];
}

