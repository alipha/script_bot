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


operation_type operation_types[] = {
//  symbol bin? bin_next? ops        assoc.     prec  nop?       category                code                  replace_with    left_pair? empty?    primary_right         other_right
    {"",    false, true,  0, associative::left,    0, false, op_category::other,      op_code::none,         op_code::none,         false, false, op_code::none,         op_code::none},
    {".",   true,  false, 2, associative::left,  900, false, op_category::other,      op_code::dot,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"?.",  true,  false, 2, associative::left,  900, false, op_category::other,      op_code::null_dot,     op_code::null_dot_end, false, false, op_code::none,         op_code::none},
    {"/?.", true,  false, 2, associative::left,  850, true,  op_category::other,      op_code::null_dot_end, op_code::none,         false, false, op_code::none,         op_code::none},
    {"(",   true,  false, 2, associative::left,  900, false, op_category::other,      op_code::func_call,    op_code::none,         false, true,  op_code::func_call_end,op_code::right_paren},
  //{"?(",  true,  false, 2, associative::left,  900, false, op_category::other,      op_code::null_call,    op_code::none,         false, true,  op_code::null_call_end,op_code::right_paren},
  //{"/?(", true,  false, 2, associative::left,  850, true,  op_category::other,      op_code::null_call_end,op_code::none,         true,  false, op_code::none,         op_code::none},
    {"[",   true,  false, 2, associative::left,  900, false, op_category::other,      op_code::index,        op_code::none,         false, false, op_code::index_end,    op_code::array_end},
    {"?[",  true,  false, 2, associative::left,  900, false, op_category::other,      op_code::null_index,   op_code::null_index_end,false,false, op_code::index_end,    op_code::array_end},
    {"/?[", true,  false, 2, associative::left,  850, true,  op_category::other,      op_code::null_index_end,op_code::none,        false, false, op_code::none,         op_code::none},
    {"<",   true,  false, 2, associative::left,  450, false, op_category::comparison, op_code::lt,           op_code::none,         false, false, op_code::none,         op_code::none},
    {"<=",  true,  false, 2, associative::left,  450, false, op_category::comparison, op_code::lte,          op_code::none,         false, false, op_code::none,         op_code::none},
    {">",   true,  false, 2, associative::left,  450, false, op_category::comparison, op_code::gt,           op_code::none,         false, false, op_code::none,         op_code::none},
    {">=",  true,  false, 2, associative::left,  450, false, op_category::comparison, op_code::gte,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"==",  true,  false, 2, associative::left,  450, false, op_category::comparison, op_code::eq,           op_code::none,         false, false, op_code::none,         op_code::none},
    {"!=",  true,  false, 2, associative::left,  450, false, op_category::comparison, op_code::neq,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"%",   true,  false, 2, associative::left,  550, false, op_category::integer,    op_code::mod,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"&",   true,  false, 2, associative::left,  375, false, op_category::integer,    op_code::bit_and,      op_code::none,         false, false, op_code::none,         op_code::none},
    {"^",   true,  false, 2, associative::left,  350, false, op_category::integer,    op_code::bit_xor,      op_code::none,         false, false, op_code::none,         op_code::none},
    {"|",   true,  false, 2, associative::left,  325, false, op_category::integer,    op_code::bit_or,       op_code::none,         false, false, op_code::none,         op_code::none},
    {"<<",  true,  false, 2, associative::left,  475, false, op_category::integer,    op_code::shl,          op_code::none,         false, false, op_code::none,         op_code::none},
    {">>",  true,  false, 2, associative::left,  475, false, op_category::integer,    op_code::shr,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"**",  true,  false, 2, associative::left,  575, false, op_category::arithmetic, op_code::pow,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"+",   true,  false, 2, associative::left,  500, false, op_category::arithmetic, op_code::add,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"-",   true,  false, 2, associative::left,  500, false, op_category::arithmetic, op_code::sub,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"*",   true,  false, 2, associative::left,  550, false, op_category::arithmetic, op_code::mul,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"/",   true,  false, 2, associative::left,  550, false, op_category::arithmetic, op_code::div,          op_code::none,         false, false, op_code::none,         op_code::none},
    {"=",   true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::assign,       op_code::none,         false, false, op_code::none,         op_code::none},
    {"%=",  true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::mod_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"&=",  true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::and_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"^=",  true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::xor_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"|=",  true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::or_assign,    op_code::none,         false, false, op_code::none,         op_code::none},
    {"<<=", true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::shl_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {">>=", true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::shr_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"**=", true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::pow_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"+=",  true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::add_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"-=",  true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::sub_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"*=",  true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::mul_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"/=",  true,  false, 2, associative::right, 200, false, op_category::assignment, op_code::div_assign,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"++",  false, false, 1, associative::right, 600, false, op_category::assignment, op_code::pre_inc,      op_code::none,         false, false, op_code::none,         op_code::none},
    {"--",  false, false, 1, associative::right, 600, false, op_category::assignment, op_code::pre_dec,      op_code::none,         false, false, op_code::none,         op_code::none},
    {"++",  true,  true,  1, associative::left,  700, false, op_category::assignment, op_code::post_inc,     op_code::none,         false, false, op_code::none,         op_code::none},
    {"--",  true,  true,  1, associative::left,  700, false, op_category::assignment, op_code::post_dec,     op_code::none,         false, false, op_code::none,         op_code::none},
    {"~",   false, false, 1, associative::right, 600, false, op_category::integer,    op_code::bit_not,      op_code::none,         false, false, op_code::none,         op_code::none},
    {"!",   false, false, 1, associative::right, 600, false, op_category::comparison, op_code::logic_not,    op_code::none,         false, false, op_code::none,         op_code::none},
    {"+",   false, false, 1, associative::right, 600, false, op_category::integer,    op_code::plus,         op_code::none,         false, false, op_code::none,         op_code::none},
    {"-",   false, false, 1, associative::right, 600, false, op_category::arithmetic, op_code::negate,       op_code::none,         false, false, op_code::none,         op_code::none},
    {"&&",  true,  false, 1, associative::left,  300, false, op_category::comparison, op_code::logic_and,    op_code::logic_and_end,false, false, op_code::none,         op_code::none},
    {"/&&", true,  false, 1, associative::left,  300, true,  op_category::comparison, op_code::logic_and_end,op_code::none,         false, false, op_code::none,         op_code::none},
    {"||",  true,  false, 1, associative::left,  275, false, op_category::comparison, op_code::logic_or,     op_code::logic_or_end, false, false, op_code::none,         op_code::none},
    {"/||", true,  false, 1, associative::left,  275, true,  op_category::comparison, op_code::logic_or_end, op_code::none,         false, false, op_code::none,         op_code::none},
    {"??",  true,  false, 1, associative::left,  270, false, op_category::comparison, op_code::coalesce,     op_code::coalesce_end, false, false, op_code::none,         op_code::none},
    {"/??", true,  false, 1, associative::left,  270, true,  op_category::comparison, op_code::coalesce_end, op_code::none,         false, false, op_code::none,         op_code::none},
    {",",   true,  false, 1, associative::left,  150, false, op_category::other,      op_code::array_add,    op_code::none,         false, false, op_code::none,         op_code::none},
    {",",   true,  false, 1, associative::left,  150, false, op_category::other,      op_code::map_add,      op_code::none,         false, false, op_code::none,         op_code::none},
    {",",   true,  false, 1, associative::left,  150, false, op_category::other,      op_code::param_add,    op_code::none,         false, false, op_code::none,         op_code::none},
    {",",   true,  false, 2, associative::left,  150, false, op_category::other,      op_code::comma,        op_code::none,         false, false, op_code::none,         op_code::none},
    {"?",   true,  false, 1, associative::left,  170, false, op_category::other,      op_code::ternary_start,op_code::colon,        false, false, op_code::none,         op_code::none},
    {":",   true,  false, 1, associative::left,  170, false, op_category::other,      op_code::colon,        op_code::none,         false, false, op_code::none,         op_code::none},    // TODO: next: ternary_end?
    {"/?:", true,  false, 1, associative::left,  170, true,  op_category::other,      op_code::ternary_end,  op_code::none,         false, false, op_code::none,         op_code::none},
    {";",   true,  false, 1, associative::left,   50, false, op_category::other,      op_code::semicolon,    op_code::none,         false, false, op_code::none,         op_code::none},  // TODO: this precedence should be different?
    {"(",   false, false, 0, associative::right,   0, true,  op_category::other,      op_code::left_paren,   op_code::none,         false, false, op_code::right_paren,  op_code::func_call_end},
    {")",   true,  true,  1, associative::left,  900, false, op_category::other,      op_code::func_call_end,op_code::none,         true,  false, op_code::none,         op_code::none},
    {")",   true,  true,  1, associative::left,    0, true,  op_category::other,      op_code::right_paren,  op_code::none,         true,  false, op_code::none,         op_code::none},
    {"[",   false, false, 0, associative::right,   0, false, op_category::other,      op_code::array_start,  op_code::none,         false, true,  op_code::array_end,    op_code::index_end},
    {"]",   true,  true,  1, associative::left,  900, true,  op_category::other,      op_code::index_end,    op_code::none,         true,  false, op_code::none,         op_code::none},
    {"]",   true,  true,  1, associative::left,    0, false, op_category::other,      op_code::array_end,    op_code::none,         true,  false, op_code::none,         op_code::none},
    {"{",   false, false, 0, associative::right,   0, true,  op_category::other,      op_code::block_start,  op_code::none,         false, true,  op_code::block_end,    op_code::none},  // removed op_code::map_end as other
    {"{",   false, false, 0, associative::right,   0, false, op_category::other,      op_code::map_start,    op_code::none,         false, true,  op_code::map_end,      op_code::block_end},
    {"}",   false, false, 1, associative::left,    0, true,  op_category::other,      op_code::block_end,    op_code::none,         true,  false, op_code::none,         op_code::none},
    {"}",   true,  true,  1, associative::left,    0, false, op_category::other,      op_code::map_end,      op_code::none,         true,  false, op_code::none,         op_code::none},
    {"if" , false, false, 1, associative::left,   50, true,  op_category::ctrl_start, op_code::if_start,     op_code::if_cond,      false, false, op_code::none,         op_code::none},
    {"if?", false, false, 1, associative::left,   50, false, op_category::ctrl_cond,  op_code::if_cond,      op_code::if_end,       false, false, op_code::none,         op_code::none},
    {"/if", false, false, 1, associative::right,  50, true,  op_category::ctrl_end,   op_code::if_end,       op_code::none,         false, false, op_code::none,         op_code::none}, 
    {"else",false, false, 1, associative::left,   50, false, op_category::ctrl_cond,  op_code::else_start,   op_code::none,         false, false, op_code::none,         op_code::none},
    {"/else",false,false, 1, associative::right,  50, true,  op_category::ctrl_end,   op_code::else_end,     op_code::none,         false, false, op_code::none,         op_code::none}, 
    {"while",false,false, 1, associative::left,   50, true,  op_category::ctrl_start, op_code::while_start,  op_code::while_cond,   false, false, op_code::none,         op_code::none},
    {"while?",false,false,1, associative::left,   50, false, op_category::ctrl_cond,  op_code::while_cond,   op_code::while_end,    false, false, op_code::none,         op_code::none},
    {"/while",false,false,1, associative::right,  50, false, op_category::ctrl_end,   op_code::while_end,    op_code::none,         false, false, op_code::none,         op_code::none}
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
                return it->second;

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

