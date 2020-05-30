#ifndef LIPH_OPERATION_TYPE_HPP
#define LIPH_OPERATION_TYPE_HPP

#include <cstdint>
#include <string_view>


enum class op_category : std::uint8_t {
    integer,
    comparison,
    arithmetic,
    assignment,
    ctrl_start,
    ctrl_cond,
    ctrl_end,
    other
};


enum class op_code : std::uint8_t {
    none = 0,
    dot,
    null_dot,
    null_dot_end,
    index,
    null_index,
    null_index_end,
    lt,
    lte,       // 08
    gt,
    gte,
    eq,
    neq,       // 0c
    mod,
    bit_and,
    bit_xor,
    bit_or,    // 10
    shl,
    shr,
    pow,
    add,       // 14
    sub,
    mul,
    div,
    assign,     // 18
    mod_assign,
    and_assign,
    xor_assign,
    or_assign,  // 1c  
    shl_assign,
    shr_assign,
    pow_assign,
    add_assign, // 20
    sub_assign,
    mul_assign,
    div_assign,
    pre_inc,    // 24
    pre_dec,
    post_inc,
    post_dec,
    bit_not,    // 28
    logic_not,
    plus,
    negate,
    logic_and,     // 2c
    logic_and_end,
    logic_or,
    logic_or_end,
    coalesce,     // 30
    coalesce_end,
    array_add,
    map_add,
    param_add,   // 34
    comma,
    ternary_start,
    colon,
    ternary_end, // 38
    semicolon,
    func_call,
    left_paren,
    func_call_end, // 3c
    right_paren,
    array_start,
    index_end,
    array_end,   // 40
    block_start,
    func_start,
    map_start,
    func_end,   // 44
    block_end,
    map_end,
    if_start,
    if_cond,     // 48
    if_end,
    else_start,
    else_end,
    while_start, // 4c
    while_cond,
    while_end,
    func_lit,
    ret,         // 50
    count,

    global_var,
    local_var,
    int_lit,     // 54
    uint_lit,
    float_lit,
    str_lit,
    null_lit     // 58
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
    bool is_binary_next;
    int operand_count;
    associative associativity;
    int precedence;
    bool is_nop;
    bool is_statement_level;
    op_category category;
    op_code code;
    op_code replace_with;
    bool has_left_pair;
    bool allow_empty_pair;
    op_code primary_right_pair;
    op_code other_right_pair;
};


operation_type lookup_operation(std::string_view symbol, bool in_binary_context, const operation_type &last_type);
operation_type lookup_operation(std::string_view symbol, bool in_binary_context);
operation_type lookup_operation(op_code code);

bool is_empty_pair(const operation_type &left, const operation_type &right);

bool has_lower_precedence(op_code current_code, op_code top_code);


inline bool is_binary_op(op_code code) {
    return code >= op_code::dot && code <= op_code::div_assign;
}

inline bool is_unary_op(op_code code) {
    return code >= op_code::pre_inc && code <= op_code::ret;
}

inline bool is_binary_comp(op_code code) {
    return code >= op_code::lt && code <= op_code::neq;
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

