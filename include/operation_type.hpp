#ifndef LIPH_OPERATION_TYPE_HPP
#define LIPH_OPERATION_TYPE_HPP

#include <cstdint>
#include <string_view>


enum class op_category : std::uint8_t {
    integer,
    comparison,
    arithmetic,
    assignment,
    control,
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
    mod,
    bit_and,
    bit_xor,   // 0c
    bit_or,
    shl,
    shr,
    pow,       // 10
    add,
    sub,
    mul,
    div,       // 14
    assign,
    mod_assign,
    and_assign,
    xor_assign, // 18
    or_assign,  
    shl_assign,
    shr_assign,
    pow_assign, // 1c
    add_assign,
    sub_assign,
    mul_assign,
    div_assign, // 20
    pre_inc,
    pre_dec,
    post_inc,
    post_dec,   // 24
    bit_not,
    logic_not,
    plus,
    negate,     // 28
    logic_and,
    logic_and_end,
    logic_or,
    logic_or_end, // 2c
    array_add,
    map_add,
    comma,
    colon,       // 30
    semicolon,
    left_paren,
    right_paren,
    array_start, // 34
    array_end,
    block_start,
    map_start,
    block_end,   // 38
    map_end,
    if_start,
    if_cond,
    if_end,      // 3c
    while_start,
    while_cond,
    while_end,
    count,       // 40

    global_var,
    local_var,
    int_lit,
    uint_lit,   // 44
    float_lit,
    str_lit,
    null_lit
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
    op_code replace_with;
    bool has_left_pair;
    op_code right_pair;
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

