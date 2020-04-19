#include "operation_type.hpp"
#include "debug.hpp"
#include "string_util.hpp"

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>


using namespace std::string_literals;


operation_type operation_types[] = {
    {"",    true,  0, associative::left,    0, op_category::other,      op_code::none},
    {".",   true,  2, associative::left,  900, op_category::other,      op_code::dot},
    {"(",   true,  2, associative::left,  100, op_category::other,      op_code::func_call},
    {"[",   true,  2, associative::left,  100, op_category::other,      op_code::index},
    {"<",   true,  2, associative::left,  450, op_category::comparison, op_code::lt},
    {"<=",  true,  2, associative::left,  450, op_category::comparison, op_code::lte},
    {">",   true,  2, associative::left,  450, op_category::comparison, op_code::gt},
    {">=",  true,  2, associative::left,  450, op_category::comparison, op_code::gte},
    {"==",  true,  2, associative::left,  450, op_category::comparison, op_code::eq},
    {"!=",  true,  2, associative::left,  450, op_category::comparison, op_code::neq},
    {"&&",  true,  2, associative::left,  300, op_category::comparison, op_code::logic_and},
    {"||",  true,  2, associative::left,  275, op_category::comparison, op_code::logic_or},
    {",",   true,  2, associative::left,  150, op_category::other,      op_code::array_add},
    {",",   true,  2, associative::left,  150, op_category::other,      op_code::map_add},
    {",",   true,  2, associative::left,  150, op_category::other,      op_code::comma},
    {":",   true,  2, associative::left,  170, op_category::other,      op_code::colon},
    {";",   true,  2, associative::left,   10, op_category::other,      op_code::semicolon},
    {"%",   true,  2, associative::left,  550, op_category::integer,    op_code::mod},
    {"&",   true,  2, associative::left,  375, op_category::integer,    op_code::bit_and},
    {"^",   true,  2, associative::left,  350, op_category::integer,    op_code::bit_xor},
    {"|",   true,  2, associative::left,  325, op_category::integer,    op_code::bit_or},
    {"<<",  true,  2, associative::left,  475, op_category::integer,    op_code::shl},
    {">>",  true,  2, associative::left,  475, op_category::integer,    op_code::shr},
    {"**",  true,  2, associative::left,  575, op_category::arithmetic, op_code::pow},
    {"+",   true,  2, associative::left,  500, op_category::arithmetic, op_code::add},
    {"-",   true,  2, associative::left,  500, op_category::arithmetic, op_code::sub},
    {"*",   true,  2, associative::left,  550, op_category::arithmetic, op_code::mul},
    {"/",   true,  2, associative::left,  550, op_category::arithmetic, op_code::div},
    {"=",   true,  2, associative::right, 200, op_category::assignment, op_code::assign},
    {"%=",  true,  2, associative::right, 200, op_category::assignment, op_code::mod_assign},
    {"&=",  true,  2, associative::right, 200, op_category::assignment, op_code::and_assign},
    {"^=",  true,  2, associative::right, 200, op_category::assignment, op_code::xor_assign},
    {"|=",  true,  2, associative::right, 200, op_category::assignment, op_code::or_assign},
    {"<<=", true,  2, associative::right, 200, op_category::assignment, op_code::shl_assign},
    {">>=", true,  2, associative::right, 200, op_category::assignment, op_code::shr_assign},
    {"**=", true,  2, associative::right, 200, op_category::assignment, op_code::pow_assign},
    {"+=",  true,  2, associative::right, 200, op_category::assignment, op_code::add_assign},
    {"-=",  true,  2, associative::right, 200, op_category::assignment, op_code::sub_assign},
    {"*=",  true,  2, associative::right, 200, op_category::assignment, op_code::mul_assign},
    {"/=",  true,  2, associative::right, 200, op_category::assignment, op_code::div_assign},
    {"++",  false, 1, associative::right, 600, op_category::assignment, op_code::pre_inc},
    {"--",  false, 1, associative::right, 600, op_category::assignment, op_code::pre_dec},
    {"++",  true,  1, associative::left,  700, op_category::assignment, op_code::post_inc},
    {"--",  true,  1, associative::left,  700, op_category::assignment, op_code::post_dec},
    {"~",   false, 1, associative::right, 600, op_category::integer,    op_code::bit_not},
    {"!",   false, 1, associative::right, 600, op_category::comparison, op_code::logic_not},
    {"+",   false, 1, associative::right, 600, op_category::integer,    op_code::plus},
    {"-",   false, 1, associative::right, 600, op_category::arithmetic, op_code::negate},
    {"(",   false, 1, associative::left,  100, op_category::other,      op_code::left_paren},
    {")",   true,  1, associative::left,  100, op_category::other,      op_code::right_paren},
    {"[",   false, 1, associative::left,  100, op_category::other,      op_code::array_start},
    {"]",   true,  1, associative::left,  100, op_category::other,      op_code::array_end},
    {"{",   false, 1, associative::left,  100, op_category::other,      op_code::block_start},
    {"{",   false, 1, associative::left,  100, op_category::other,      op_code::map_start},
    {"}",   true,  1, associative::left,  100, op_category::other,      op_code::block_end},
    {"}",   true,  1, associative::left,  100, op_category::other,      op_code::map_end},
    {"if",  true,  1, associative::left,    5, op_category::other,      op_code::if_start},    // TODO: changed binary context
    {"/if", true,  1, associative::left,    5, op_category::other,      op_code::if_block},
    {"while/",true,1, associative::left,    5, op_category::other,      op_code::while_start}, // TODO: changed binary context
    {"/while",true,1, associative::left,    5, op_category::other,      op_code::while_block},
    {"while", true,1, associative::left,    5, op_category::other,      op_code::while_cond}
};


void validate(const operation_type &type) {
    op_code code = type.code;
    if(type.operand_count == 2 && !is_binary_op(code))
        throw std::logic_error(type.symbol + " has 2 operands but !is_binary_op"s);
    if(type.operand_count != 2 && is_binary_op(code)) 
        throw std::logic_error(type.symbol + " doesn't have 2 operands but is_binary_op"s);
    if(type.operand_count == 1 && !is_unary_op(code))
        throw std::logic_error(type.symbol + " has 1 operand but !is_unary_op"s);
    if(type.operand_count != 1 && is_unary_op(code))
        throw std::logic_error(type.symbol + " doesn't have 1 operand but is_unary_op"s);
    if(type.operand_count == 2 && type.category == op_category::comparison && !is_binary_comp(code))
        throw std::logic_error(type.symbol + " is binary and is comparison category but !is_binary_comp"s);
    if((type.operand_count != 2 || type.category != op_category::comparison) && is_binary_comp(code))
        throw std::logic_error(type.symbol + " is not binary or is not comparison category but is_binary_comp"s);
    if(type.operand_count == 2 && type.category == op_category::integer && !is_binary_int_op(code))
        throw std::logic_error(type.symbol + " is binary and is integer category but !is_binary_int_op"s);
    if((type.operand_count != 2 || type.category != op_category::integer) && is_binary_int_op(code))
        throw std::logic_error(type.symbol + " is not binary or is not integer category but is_binary_int_op"s);
    if(type.operand_count == 2 && type.category == op_category::arithmetic && !is_binary_arithmetic(code))
        throw std::logic_error(type.symbol + " is binary and is arithmetic category but !is_binary_arithmetic"s); 
    if((type.operand_count != 2 || type.category != op_category::arithmetic) && is_binary_arithmetic(code))
        throw std::logic_error(type.symbol + " is not binary or is not arithmetic category but is_binary_arithmetic"s);
    if(type.operand_count == 2 && type.category == op_category::assignment && !is_binary_assignment(code))
        throw std::logic_error(type.symbol + " is binary and is assignment category but !is_binary_assignment"s); 
    if((type.operand_count != 2 || type.category != op_category::assignment) && is_binary_assignment(code))
        throw std::logic_error(type.symbol + " is not binary or is not assignment category but is_binary_assignment"s);
    if(type.operand_count == 1 && type.category == op_category::assignment && !is_increment(code))
        throw std::logic_error(type.symbol + " is unary and is assignment category but !is_increment"s); 
    if((type.operand_count != 1 || type.category != op_category::assignment) && is_increment(code))
        throw std::logic_error(type.symbol + " is not unary or is not assignment category but is_increment"s);  
}


std::map<std::pair<std::string_view, bool>, operation_type> load_operation_types() {
    auto &types = operation_types;

    static_assert(sizeof(types) / sizeof(*types) == static_cast<std::size_t>(op_code::count), 
            "Missing op_code to operation_type mapping");

    std::map<std::pair<std::string_view, bool>, operation_type> type_map;
    int code_index = 0;

    for(auto &t : types) {
        if(t.code != static_cast<op_code>(code_index))
            throw std::logic_error(t.symbol + " is not in the correct order in operation_types. Index: "s + std::to_string(code_index));
        validate(t);

        if(t.category == op_category::assignment && t.operand_count == 2 && t.code != op_code::assign) {
            std::string_view symbol = t.symbol;
            symbol.remove_suffix(1);
            if(types[code_index - assign_ops_offset].symbol != symbol)
                throw std::logic_error(t.symbol + " is not "s + std::to_string(assign_ops_offset) 
                        + " op_codes after " + symbol);
        }

        type_map[std::pair(t.symbol, t.in_binary_context)] = t;
        code_index++;
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

    if constexpr(debug) {
        if(code >= op_code::count)
            throw std::out_of_range("op_code >= op_code::count. Value: " + std::to_string(code_index));
    }
    return operation_types[code_index];
}


bool has_lower_precedence(op_code current_code, op_code top_code) {
    operation_type current = lookup_operation(current_code);
    operation_type top = lookup_operation(top_code);

    return current.precedence < top.precedence 
        || (current.precedence == top.precedence && top.associativity == associative::left);
}

