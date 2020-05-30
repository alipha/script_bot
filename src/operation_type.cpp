#include "operation_type.hpp"
#include "debug.hpp"
#include "string_util.hpp"

#include <cstddef>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>


using namespace std::string_literals;

using assoc = associative;
using cat = op_category;
using code = op_code;


operation_type operation_types[] = {
//  symbol   bin? bin_next? ops   assoc.     prec  nop?  stmt?     category         code            replace_with    left_pair? empty?    primary_right    other_right
    {"",      false, true,  0, assoc::left,    0, false, false, cat::other,      code::none,         code::none,         false, false, code::none,         code::none},
    {".",     true,  false, 2, assoc::left,  900, false, false, cat::other,      code::dot,          code::none,         false, false, code::none,         code::none},
    {"?.",    true,  false, 2, assoc::left,  900, false, false, cat::other,      code::null_dot,     code::null_dot_end, false, false, code::none,         code::none},
    {"/?.",   true,  false, 2, assoc::left,  850, true,  false, cat::other,      code::null_dot_end, code::none,         false, false, code::none,         code::none},
    {"[",     true,  false, 2, assoc::left,  900, false, false, cat::other,      code::index,        code::none,         false, false, code::index_end,    code::array_end},
    {"?[",    true,  false, 2, assoc::left,  900, false, false, cat::other,      code::null_index,   code::null_index_end,false,false, code::index_end,    code::array_end},
    {"/?[",   true,  false, 2, assoc::left,  850, true,  false, cat::other,      code::null_index_end,code::none,        false, false, code::none,         code::none},
    {"<",     true,  false, 2, assoc::left,  450, false, false, cat::comparison, code::lt,           code::none,         false, false, code::none,         code::none},
    {"<=",    true,  false, 2, assoc::left,  450, false, false, cat::comparison, code::lte,          code::none,         false, false, code::none,         code::none},
    {">",     true,  false, 2, assoc::left,  450, false, false, cat::comparison, code::gt,           code::none,         false, false, code::none,         code::none},
    {">=",    true,  false, 2, assoc::left,  450, false, false, cat::comparison, code::gte,          code::none,         false, false, code::none,         code::none},
    {"==",    true,  false, 2, assoc::left,  450, false, false, cat::comparison, code::eq,           code::none,         false, false, code::none,         code::none},
    {"!=",    true,  false, 2, assoc::left,  450, false, false, cat::comparison, code::neq,          code::none,         false, false, code::none,         code::none},
    {"%",     true,  false, 2, assoc::left,  550, false, false, cat::integer,    code::mod,          code::none,         false, false, code::none,         code::none},
    {"&",     true,  false, 2, assoc::left,  375, false, false, cat::integer,    code::bit_and,      code::none,         false, false, code::none,         code::none},
    {"^",     true,  false, 2, assoc::left,  350, false, false, cat::integer,    code::bit_xor,      code::none,         false, false, code::none,         code::none},
    {"|",     true,  false, 2, assoc::left,  325, false, false, cat::integer,    code::bit_or,       code::none,         false, false, code::none,         code::none},
    {"<<",    true,  false, 2, assoc::left,  475, false, false, cat::integer,    code::shl,          code::none,         false, false, code::none,         code::none},
    {">>",    true,  false, 2, assoc::left,  475, false, false, cat::integer,    code::shr,          code::none,         false, false, code::none,         code::none},
    {"**",    true,  false, 2, assoc::left,  575, false, false, cat::arithmetic, code::pow,          code::none,         false, false, code::none,         code::none},
    {"+",     true,  false, 2, assoc::left,  500, false, false, cat::arithmetic, code::add,          code::none,         false, false, code::none,         code::none},
    {"-",     true,  false, 2, assoc::left,  500, false, false, cat::arithmetic, code::sub,          code::none,         false, false, code::none,         code::none},
    {"*",     true,  false, 2, assoc::left,  550, false, false, cat::arithmetic, code::mul,          code::none,         false, false, code::none,         code::none},
    {"/",     true,  false, 2, assoc::left,  550, false, false, cat::arithmetic, code::div,          code::none,         false, false, code::none,         code::none},
    {"=",     true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::assign,       code::none,         false, false, code::none,         code::none},
    {"%=",    true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::mod_assign,   code::none,         false, false, code::none,         code::none},
    {"&=",    true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::and_assign,   code::none,         false, false, code::none,         code::none},
    {"^=",    true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::xor_assign,   code::none,         false, false, code::none,         code::none},
    {"|=",    true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::or_assign,    code::none,         false, false, code::none,         code::none},
    {"<<=",   true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::shl_assign,   code::none,         false, false, code::none,         code::none},
    {">>=",   true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::shr_assign,   code::none,         false, false, code::none,         code::none},
    {"**=",   true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::pow_assign,   code::none,         false, false, code::none,         code::none},
    {"+=",    true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::add_assign,   code::none,         false, false, code::none,         code::none},
    {"-=",    true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::sub_assign,   code::none,         false, false, code::none,         code::none},
    {"*=",    true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::mul_assign,   code::none,         false, false, code::none,         code::none},
    {"/=",    true,  false, 2, assoc::right, 200, false, false, cat::assignment, code::div_assign,   code::none,         false, false, code::none,         code::none},

//  symbol   bin? bin_next? ops   assoc.     prec  nop?  stmt?     category         code            replace_with    left_pair? empty?    primary_right    other_right
    {"++",    false, false, 1, assoc::right, 600, false, false, cat::assignment, code::pre_inc,      code::none,         false, false, code::none,         code::none},
    {"--",    false, false, 1, assoc::right, 600, false, false, cat::assignment, code::pre_dec,      code::none,         false, false, code::none,         code::none},
    {"++",    true,  true,  1, assoc::left,  700, false, false, cat::assignment, code::post_inc,     code::none,         false, false, code::none,         code::none},
    {"--",    true,  true,  1, assoc::left,  700, false, false, cat::assignment, code::post_dec,     code::none,         false, false, code::none,         code::none},
    {"~",     false, false, 1, assoc::right, 600, false, false, cat::integer,    code::bit_not,      code::none,         false, false, code::none,         code::none},
    {"!",     false, false, 1, assoc::right, 600, false, false, cat::comparison, code::logic_not,    code::none,         false, false, code::none,         code::none},
    {"+",     false, false, 1, assoc::right, 600, false, false, cat::integer,    code::plus,         code::none,         false, false, code::none,         code::none},
    {"-",     false, false, 1, assoc::right, 600, false, false, cat::arithmetic, code::negate,       code::none,         false, false, code::none,         code::none},
    {"&&",    true,  false, 1, assoc::left,  300, false, false, cat::comparison, code::logic_and,    code::logic_and_end,false, false, code::none,         code::none},
    {"/&&",   true,  false, 1, assoc::left,  300, true,  false, cat::comparison, code::logic_and_end,code::none,         false, false, code::none,         code::none},
    {"||",    true,  false, 1, assoc::left,  275, false, false, cat::comparison, code::logic_or,     code::logic_or_end, false, false, code::none,         code::none},
    {"/||",   true,  false, 1, assoc::left,  275, true,  false, cat::comparison, code::logic_or_end, code::none,         false, false, code::none,         code::none},
    {"??",    true,  false, 1, assoc::left,  270, false, false, cat::comparison, code::coalesce,     code::coalesce_end, false, false, code::none,         code::none},
    {"/??",   true,  false, 1, assoc::left,  270, true,  false, cat::comparison, code::coalesce_end, code::none,         false, false, code::none,         code::none},
    {",",     true,  false, 1, assoc::left,  150, false, false, cat::other,      code::array_add,    code::none,         false, false, code::none,         code::none},
    {",",     true,  false, 1, assoc::left,  150, false, false, cat::other,      code::map_add,      code::none,         false, false, code::none,         code::none},
    {",",     true,  false, 1, assoc::left,  150, false, false, cat::other,      code::param_add,    code::none,         false, false, code::none,         code::none},
    {",",     true,  false, 2, assoc::left,  150, false, false, cat::other,      code::comma,        code::none,         false, false, code::none,         code::none},
    {"?",     true,  false, 1, assoc::left,  170, false, false, cat::other,      code::ternary_start,code::colon,        false, false, code::none,         code::none},
    {":",     true,  false, 1, assoc::left,  170, false, false, cat::other,      code::colon,        code::none,         false, false, code::none,         code::none},    // TODO: next: ternary_end?
    {"/?:",   true,  false, 1, assoc::left,  170, true,  false, cat::other,      code::ternary_end,  code::none,         false, false, code::none,         code::none},
    {";",     true,  false, 1, assoc::left,   50, false, false, cat::other,      code::semicolon,    code::none,         false, false, code::none,         code::none},  // TODO: this precedence should be different?
    {"(",     true,  false, 0, assoc::left,  900, false, false, cat::other,      code::func_call,    code::none,         false, true,  code::func_call_end,code::right_paren},
  //{"?(",    true,  false, 0, assoc::left,  900, false, false, cat::other,      code::null_call,    code::none,         false, true,  code::null_call_end,code::right_paren},
  //{"/?(",   true,  false, 0, assoc::left,  850, true,  false, cat::other,      code::null_call_end,code::none,         true,  false, code::none,         code::none},
    {"(",     false, false, 0, assoc::right,   0, true,  false, cat::other,      code::left_paren,   code::none,         false, false, code::right_paren,  code::func_call_end},
    {")",     true,  true,  1, assoc::left,  900, false, false, cat::other,      code::func_call_end,code::none,         true,  false, code::none,         code::none},
    {")",     true,  true,  1, assoc::left,    0, true,  false, cat::other,      code::right_paren,  code::none,         true,  false, code::none,         code::none},
    {"[",     false, false, 0, assoc::right,   0, false, false, cat::other,      code::array_start,  code::none,         false, true,  code::array_end,    code::index_end},
    {"]",     true,  true,  1, assoc::left,  900, true,  false, cat::other,      code::index_end,    code::none,         true,  false, code::none,         code::none},
    {"]",     true,  true,  1, assoc::left,    0, false, false, cat::other,      code::array_end,    code::none,         true,  false, code::none,         code::none},
    {"{",     false, false, 0, assoc::right,   0, true,  false, cat::other,      code::block_start,  code::none,         false, true,  code::block_end,    code::none},  // removed code::map_end as other
    {"{",     false, false, 0, assoc::right,   0, true,  false, cat::other,      code::func_start,   code::none,         false, true,  code::func_end,     code::block_end},
    {"{",     false, false, 0, assoc::right,   0, false, false, cat::other,      code::map_start,    code::none,         false, true,  code::map_end,      code::block_end},
    {"}",     false, false, 1, assoc::left,    0, true,  true,  cat::other,      code::func_end,     code::none,         true,  false, code::none,         code::none},
    {"}",     false, false, 1, assoc::left,    0, true,  true,  cat::other,      code::block_end,    code::none,         true,  false, code::none,         code::none},
    {"}",     true,  true,  1, assoc::left,    0, false, false, cat::other,      code::map_end,      code::none,         true,  false, code::none,         code::none},
    {"if" ,   false, false, 1, assoc::left,   50, true,  true,  cat::ctrl_start, code::if_start,     code::if_cond,      false, false, code::none,         code::none},
    {"if?",   false, false, 1, assoc::left,   50, false, false, cat::ctrl_cond,  code::if_cond,      code::if_end,       false, false, code::none,         code::none},
    {"/if",   false, false, 1, assoc::right,  50, true,  false, cat::ctrl_end,   code::if_end,       code::none,         false, false, code::none,         code::none}, 
    {"else",  false, false, 1, assoc::left,   50, false, false, cat::ctrl_cond,  code::else_start,   code::none,         false, false, code::none,         code::none},
    {"/else", false, false, 1, assoc::right,  50, true,  false, cat::ctrl_end,   code::else_end,     code::none,         false, false, code::none,         code::none}, 
    {"while", false, false, 1, assoc::left,   50, true,  true,  cat::ctrl_start, code::while_start,  code::while_cond,   false, false, code::none,         code::none},
    {"while?",false, false, 1, assoc::left,   50, false, false, cat::ctrl_cond,  code::while_cond,   code::while_end,    false, false, code::none,         code::none},
    {"/while",false, false, 1, assoc::right,  50, false, false, cat::ctrl_end,   code::while_end,    code::none,         false, false, code::none,         code::none},
    {"fn",    false, true,  0, assoc::right,   0, false, false, cat::other,      code::func_lit,     code::none,         false, false, code::none,         code::none},
    {"return",false, false, 1, assoc::right,  50, false, true,  cat::other,      code::ret,          code::none,         false, false, code::none,         code::none}
    //{"return",false,false,1, assoc::left,   50, true,  false, cat::ctrl_cond,  code::return_start, code::return_end,   false, false, code::none,         code::none},
    //{"/return",false,false,1,assoc::right,  50, false, false, cat::ctrl_end,   code::return_end,   code::none,         false, false, code::none,         code::none}
};


void validate(const operation_type &type, const std::set<op_code> &primary_right_pairs) {
    op_code code = type.code;
    if(type.operand_count == 2 && !is_binary_op(code) && code != op_code::comma)
        throw std::logic_error(type.symbol + " has 2 operands but !is_binary_op"s);
    if(type.operand_count != 2 && is_binary_op(code)) 
        throw std::logic_error(type.symbol + " doesn't have 2 operands but is_binary_op"s);
    if(type.operand_count == 1 && !is_unary_op(code))
        throw std::logic_error(type.symbol + " has 1 operand but !is_unary_op"s);
    if(type.operand_count == 2 && is_unary_op(code) && code != op_code::comma)
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

    operation_type primary_right = operation_types[static_cast<int>(type.primary_right_pair)];
    operation_type other_right = operation_types[static_cast<int>(type.other_right_pair)];

    if(type.primary_right_pair != op_code::none && type.primary_right_pair == type.other_right_pair)
        throw std::logic_error(type.symbol + " has the same primary and other right pairs"s);
    if(type.primary_right_pair == op_code::none && type.other_right_pair != op_code::none)
        throw std::logic_error(type.symbol + " has other right pair "s + other_right.symbol + " but no primary right pair");
    if(type.primary_right_pair != op_code::none && !primary_right.has_left_pair)
        throw std::logic_error(type.symbol + " has a right pair of "s + primary_right.symbol + " but that right pair's has_left_pair is false");
    if(type.other_right_pair != op_code::none && !other_right.has_left_pair)
        throw std::logic_error(type.symbol + " has an other right pair of "s + other_right.symbol + " but that right pair's has_left_pair is false");
    if(type.primary_right_pair != op_code::none && type.other_right_pair != op_code::none 
            && primary_right.symbol != other_right.symbol)
        throw std::logic_error(type.symbol + " has right pairs "s + primary_right.symbol + " and " + other_right.symbol + " with different symbols");
    if(type.primary_right_pair != op_code::none && type.precedence != primary_right.precedence)
        throw std::logic_error(type.symbol + " and "s + primary_right.symbol + " have different precedences");
    if(type.has_left_pair && primary_right_pairs.find(type.code) == primary_right_pairs.end())
        throw std::logic_error(type.symbol + " has_left_pair, but no symbols has it as a right_pair"s);
    if(!type.has_left_pair && primary_right_pairs.find(type.code) != primary_right_pairs.end())
        throw std::logic_error(type.symbol + " does not has_left_pair, but a symbol has it as a right_pair"s);
}


std::map<std::pair<std::string_view, bool>, operation_type> load_operation_types() {
    auto &types = operation_types;

    static_assert(sizeof(types) / sizeof(*types) == static_cast<std::size_t>(op_code::count), 
            "Missing op_code to operation_type mapping");

    std::set<op_code> primary_right_pairs;
    std::map<std::pair<std::string_view, bool>, operation_type> type_map;
    int code_index = 0;

    for(auto &t : types) {
        if(t.code != static_cast<op_code>(code_index))
            throw std::logic_error(t.symbol + " is not in the correct order in operation_types. Index: "s + std::to_string(code_index));
        
        if(t.primary_right_pair != op_code::none) {
            auto[it, success] = primary_right_pairs.insert(t.primary_right_pair);
            if(!success && t.code != op_code::null_index)
                throw std::logic_error(std::to_string(static_cast<int>(t.primary_right_pair)) + " is aleady a primary right pair"s);
        }

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

    for(auto &t : types) {
        validate(t, primary_right_pairs);
    }

    return type_map;
}


std::map<std::pair<std::string_view, bool>, operation_type> symbol_operation_map = load_operation_types();


operation_type lookup_operation(std::string_view symbol, bool in_binary_context, const operation_type &last_type) {
    auto it = symbol_operation_map.find(std::pair(symbol, in_binary_context));

    if(it == symbol_operation_map.end()) {
        auto it2 = symbol_operation_map.find(std::pair(symbol, !in_binary_context));

        if(it2 != symbol_operation_map.end()) {
            if(is_empty_pair(last_type, it2->second))
                return it2->second;

            throw std::runtime_error(it2->second.symbol + " was not expected in "s 
                    + (in_binary_context ? "binary" : "unary") + " context");
        }

        return operation_types[static_cast<int>(op_code::none)];
    } else {
        return it->second;
    }
}


operation_type lookup_operation(std::string_view symbol, bool in_binary_context) {
    return lookup_operation(symbol, in_binary_context, lookup_operation(op_code::none));
}


operation_type lookup_operation(op_code code) {
    int code_index = static_cast<int>(code);

    if constexpr(debug) {
        if(code >= op_code::count)
            throw std::out_of_range("op_code >= op_code::count. Value: " + std::to_string(code_index));
    }
    return operation_types[code_index];
}


bool is_empty_pair(const operation_type &left, const operation_type &right) {
    return left.allow_empty_pair && ((left.primary_right_pair != op_code::none && left.primary_right_pair == right.code)
            || (left.other_right_pair != op_code::none && left.other_right_pair == right.code));
}


bool has_lower_precedence(op_code current_code, op_code top_code) {
    operation_type current = lookup_operation(current_code);
    operation_type top = lookup_operation(top_code);

    if(current.has_left_pair)
        return true;
    if(top.primary_right_pair != op_code::none)
        return false;
    
    return current.precedence < top.precedence 
        || (current.precedence == top.precedence && top.associativity == associative::left);
}

