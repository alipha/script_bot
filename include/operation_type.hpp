#ifndef LIPH_OPERATION_TYPE_HPP
#define LIPH_OPERATION_TYPE_HPP

#include <cstdint>
#include <string_view>

//
enum class op_category : std::uint8_t {
    integer,
    comparison,
    arithmetic,
    assignment,
    other
};


enum class op_code : std::uint8_t {
    none = 0,
    dot,
    func_call,
    index,
    lt,        // 04
    lte,
    gt,
    gte,
    eq,        // 08
    neq,
    logic_and,
    logic_or,
    array_add,  // 0c
    map_add,
    comma,
    colon,
    semicolon,  // 10
    mod,
    bit_and,
    bit_xor,
    bit_or,     // 14
    shl,
    shr,
    pow,
    add,        // 18
    sub,
    mul,
    div,
    assign,     // 1c
    mod_assign,
    and_assign,
    xor_assign,
    or_assign,  // 20
    shl_assign,
    shr_assign,
    pow_assign,
    add_assign, // 24
    sub_assign,
    mul_assign,
    div_assign,
    pre_inc,    // 28
    pre_dec,
    post_inc,
    post_dec,
    bit_not,    // 2c
    logic_not,
    plus,
    negate,
    left_paren,  // 30
    right_paren,
    array_start,
    array_end,
    block_start, // 34
    map_start,
    block_end,
    map_end,
    if_start,    // 38
    if_block,
    while_start,
    while_block,
    while_cond,  // 3c
    count,

    global_var,
    local_var,
    int_lit,     // 40
    uint_lit,
    float_lit,
    str_lit,
    null_lit     // 44
};


inline bool operator<(op_code a, op_code b)  { return static_cast<int>(a) < static_cast<int>(b); }
inline bool operator<=(op_code a, op_code b) { return static_cast<int>(a) <= static_cast<int>(b); }
inline bool operator>(op_code a, op_code b)  { return static_cast<int>(a) > static_cast<int>(b); }
inline bool operator>=(op_code a, op_code b) { return static_cast<int>(a) >= static_cast<int>(b); }
inline bool operator!=(op_code a, op_code b) { return static_cast<int>(a) != static_cast<int>(b); }
inline bool operator==(op_code a, op_code b) { return static_cast<int>(a) == static_cast<int>(b); }


constexpr int assign_ops_offset = static_cast<int>(op_code::mod_assign) - static_cast<int>(op_code::mod);


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
    op_category category;
    op_code code;
};


operation_type lookup_operation(std::string_view symbol, bool in_binary_context);
operation_type lookup_operation(op_code code);

bool has_lower_precedence(op_code current_code, op_code top_code);


inline bool is_binary_op(op_code code) {
    return code >= op_code::dot && code <= op_code::div_assign;
}

inline bool is_unary_op(op_code code) {
    return code >= op_code::pre_inc && code <= op_code::while_cond;
}

inline bool is_binary_comp(op_code code) {
    return code >= op_code::lt && code <= op_code::logic_or;
}

inline bool is_binary_int_op(op_code code) {
    return code >= op_code::mod && code <= op_code::shr;
}

inline bool is_binary_int_result(op_code code) {
    return is_binary_comp(code) || is_binary_int_op(code);
}

inline bool is_binary_arithmetic(op_code code) {
    return code >= op_code::pow && code <= op_code::div;
}

inline bool is_binary_assignment(op_code code) {
    return code >= op_code::assign && code <= op_code::div_assign; 
}

inline bool is_increment(op_code code) {
    return code >= op_code::pre_inc && code <= op_code::post_dec;
}

inline bool requires_lvalue(op_code code) {
    return is_binary_assignment(code) || is_increment(code);
}

#endif

