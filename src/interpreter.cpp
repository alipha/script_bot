#include "interpreter.hpp"
#include "memory_buffer.hpp"
#include "operation_type.hpp"
#include "util.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <sstream>
#include <variant>
#include <vector>


std::int32_t to_int(const value_type &value) {
    return std::visit([](auto &&arg) { return static_cast<std::int32_t>(arg); }, value);
}

/*
template<typename Func>
value_type do_with_doubles(const value_type &left, const value_type &right, Func) {
    return std::visit([](auto &&left, auto &&right) {
        using leftT = std::decay_t<decltype(left)>;
        using rightT = std::decay_t<decltype(right)>;

        if constexpr(std::is_same_v<leftT, double> || std::is_same_v<rightT, double>) {
            return Func(static_cast<double>(left), static_cast<double>(right));
        } else {
            return Func(left, right);
        }
    });
}
*/

void execute_binary_op(std::stack<value_type> &operands, op_code code);
void execute_unary_op(std::stack<value_type> &operands, op_code code);

/*
// pow
// mod
// shl
// shr
//
template<typename T>
struct arithmetic_ops {
    static T mul(T a, T b) { return a * b; }
    static T div(T a, T b) { return a / b; }
    static T add(T a, T b) { return a + b; }
    static T sub(T a, T b) { return a - b; }
    static std::int32_t  lt(T a, T b) { return a < b; }
    static std::int32_t lte(T a, T b) { return a <= b; }
    static std::int32_t  gt(T a, T b) { return a > b; }
    static std::int32_t gte(T a, T b) { return a >= b; }
    static std::int32_t  eq(T a, T b) { return a == b; }
    static std::int32_t neq(T a, T b) { return a != b; }
    static T negate(T a) { return -a; }
};
*/

std::string execute(const std::vector<char> &program) {
    memory_buffer<true> buffer(program);    // TODO: false?
    std::stack<value_type> operands;

    while(buffer.position() < buffer.size()) {
        op_code code = *buffer.read<op_code>();
       
        if(code == op_code::left_paren || code == op_code::right_paren) { 
            /* do nothing */
        } else if(code == op_code::var) {
            throw std::logic_error("var is currently unsupported");
        } else if(code == op_code::int_lit) {
            operands.push(*buffer.read<std::int32_t>());
        } else if(code == op_code::float_lit) {
            operands.push(*buffer.read<double>());
        } else if(code == op_code::str_lit) {
            throw std::logic_error("str_lit is currently unsupported");
        } else if(is_binary_op(code)) {
            execute_binary_op(operands, code);
        } else {
            execute_unary_op(operands, code);
        }
    }

    if(operands.size() != 1) {
        throw std::logic_error("Expected only 1 ooperand in stack. size: " + std::to_string(operands.size()));
    }

    return std::visit([](auto &&value) {
        std::stringstream ss;
        ss << value;
        return ss.str(); 
    }, operands.top());
}


template<typename Left, typename Right>
bool binary_comp(op_code code, Left &&l, Right &&r) {
    switch(code) {
    case op_code::lt:  return l < r;
    case op_code::lte: return l <= r;
    case op_code::gt:  return l > r;
    case op_code::gte: return l >= r;
    case op_code::eq:  return l == r;
    case op_code::neq: return l != r;
    case op_code::logic_and: return l && r;
    case op_code::logic_or:  return l || r;
    default:
        throw std::logic_error("Invalid binary_comp op: " + std::to_string(static_cast<int>(code)));
    }
}


template<typename Left, typename Right>
std::int32_t binary_int_op(op_code code, Left &&left, Right &&right) {
    auto l = static_cast<std::int32_t>(left);
    auto r = static_cast<std::int32_t>(right);
    
    switch(code) {
    case op_code::mod: return l % r;
    case op_code::bit_and: return l & r;
    case op_code::bit_xor: return l ^ r;
    case op_code::bit_or: return l | r;
    case op_code::shl: return l << r;
    case op_code::shr: return l >> r;
    default:
        throw std::logic_error("Invalid binary_int_op: " + std::to_string(static_cast<int>(code)));
    }
}


template<typename Left, typename Right>
auto binary_arithmetic(op_code code, Left &&l, Right &&r) {
    using LeftT = std::decay_t<Left>;
    using RightT = std::decay_t<Right>;

    switch(code) {
    case op_code::add: return l + r;
    case op_code::sub: return l - r;
    case op_code::mul: return l * r;
    case op_code::div: 
        if constexpr(std::is_same_v<LeftT, std::int32_t> && std::is_same_v<RightT, std::int32_t>) {
            if(r == 0)
                throw std::runtime_error("Division by zero");
        }
        return l / r;
    default:
        throw std::logic_error("Invalid binary_arithmetic op: " + std::to_string(static_cast<int>(code)));
    }
}


void execute_binary_op(std::stack<value_type> &operands, op_code code) {
    if(operands.size() < 2) {  // TODO: remove?
        throw std::logic_error("execute_binary_op with " + std::to_string(operands.size()) + " operands. op_code: "
                + std::to_string(static_cast<int>(code)));
    }

    value_type right = pop(operands);
    value_type left = pop(operands);

    value_type result = std::visit([code](auto &&l, auto &&r) -> value_type {
        if(is_binary_comp(code))
            return static_cast<std::int32_t>(binary_comp(code, l, r));
        if(is_binary_int_op(code))
            return binary_int_op(code, l, r);
        if(is_binary_arithmetic(code))
            return binary_arithmetic(code, l, r);
        throw std::logic_error("Currently unspported binary op: " + std::to_string(static_cast<int>(code)));
    }, left, right);

    operands.push(result);

        /*
    if(is_binary_int_op(code)) {
        std::int32_t l = to_int(left);
        std::int32_t r = to_int(right);

    }
    using aops = arithmetic_ops<int>;

    switch(code) {
    case op_code::mul: operands.push(aops::mul(left, right)); break;
    case op_code::div: operands.push(aops::div(left, right)); break;
    case op_code::mod: operands.push(left % right); break;
    case op_code::add: operands.push(aops::add(left, right)); break;
    case op_code::sub: operands.push(aops::sub(left, right)); break;
    case op_code::shl: operands.push(left << right); break;
    case op_code::shr: operands.push(left >> right); break;
    case op_code::lt:  operands.push(aops::lt(left, right)); break;
    case op_code::lte: operands.push(aops::lte(left, right)); break;
    case op_code::gt:  operands.push(aops::gt(left, right)); break;
    case op_code::gte: operands.push(aops::gte(left, right)); break;
    case op_code::eq:  operands.push(aops::eq(left, right)); break;
    case op_code::neq: operands.push(aops::neq(left, right)); break;
    case op_code::bit_and: operands.push(left & right); break;
    case op_code::bit_xor: operands.push(left ^ right); break;
    case op_code::bit_or:  operands.push(left | right); break;
    case op_code::logic_and: operands.push(left && right); break;
    case op_code::logic_or:  operands.push(left || right); break;
    default:
        throw std::logic_error("Currently unspported binary op: " + std::to_string(static_cast<int>(code)));
    }
    */
}


void execute_unary_op(std::stack<value_type> &operands, op_code code) {
    if(operands.empty()) {  // TODO: remove?
        throw std::logic_error("execute_unary_op with zero operands. op_code: "
                + std::to_string(static_cast<int>(code)));
    }

    value_type operand = pop(operands);

    value_type result = std::visit([code](auto &&v) -> value_type {
        switch(code) {
        case op_code::negate:    return -v;
        case op_code::bit_not:   return ~static_cast<std::int32_t>(v);
        case op_code::logic_not: return static_cast<std::int32_t>(!v);
        default:
            throw std::logic_error("Currently unsupported unary op: " + std::to_string(static_cast<int>(code)));
        }
    }, operand);

    operands.push(result);
}

