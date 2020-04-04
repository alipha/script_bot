#ifndef LIPH_OPERATION_TYPE_HPP
#define LIPH_OPERATION_TYPE_HPP

#include <cstdint>
#include <string_view>


enum class op_code : std::uint8_t {
    none = 0,
    pow,
    mul,
    div,
    mod,
    add,
    sub,
    shl,
    shr,
    lt,
    lte,
    gt,
    gte,
    eq,
    neq,
    bit_and,
    bit_xor,
    bit_or,
    logic_and,
    logic_or,
    negate,
    logic_not,
    bit_not,
    pre_inc, 
    pre_dec, 
    post_inc, 
    post_dec, 
    dot,
    assign,
    add_assign,
    sub_assign,
    mul_assign,
    div_assign,
    mod_assign,
    and_assign,
    xor_assign,
    or_assign,
    shl_assign,
    shr_assign,
    func_call,
    left_paren,
    right_paren,
    array_index,
    array_start,
    array_end,
    map_start,
    map_end,
    count,

    var,
    int_lit,
    float_lit,
    str_lit
};


inline bool operator<(op_code a, op_code b)  { return static_cast<int>(a) < static_cast<int>(b); }
inline bool operator<=(op_code a, op_code b) { return static_cast<int>(a) <= static_cast<int>(b); }
inline bool operator>(op_code a, op_code b)  { return static_cast<int>(a) > static_cast<int>(b); }
inline bool operator>=(op_code a, op_code b) { return static_cast<int>(a) >= static_cast<int>(b); }
inline bool operator!=(op_code a, op_code b) { return static_cast<int>(a) != static_cast<int>(b); }
inline bool operator==(op_code a, op_code b) { return static_cast<int>(a) == static_cast<int>(b); }


enum associative : std::uint8_t {
    left,
    right
};



struct operation_type {
    std::string_view symbol;
    bool in_binary_context;
    int operand_count;
    associative associativity;
    int precedence;
    op_code code;
};


operation_type lookup_operation(std::string_view symbol, bool in_binary_context);
operation_type lookup_operation(op_code code);


// TODO: add check in loading operation_type table
inline bool is_binary_op(op_code code) {
    return (code >= op_code::pow && code <= op_code::logic_or)
        || (code >= op_code::assign && code <= op_code::shr_assign)
        || (code == op_code::array_index);
}

inline bool is_unary_op(op_code code) {
    return code >= op_code::negate && code <= op_code::post_dec;
}

#endif

