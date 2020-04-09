#include "interpreter.hpp"
#include "memory_buffer.hpp"
#include "object.hpp"
#include "operation_type.hpp"
#include "util.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <sstream>
#include <variant>
#include <vector>


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

bool binary_comp(op_code code, const object &left, const object &right) {
    auto l = left.to_optional_int();
    auto r = right.to_optional_int();

    switch(code) {
    case op_code::lt:  return l < r;
    case op_code::lte: return l <= r;
    case op_code::gt:  return l > r;
    case op_code::gte: return l >= r;
    case op_code::eq:  return l == r;
    case op_code::neq: return l != r;
    case op_code::logic_and: return l.value_or(0) && r.value_or(0);
    case op_code::logic_or:  return l.value_or(0) || r.value_or(0);
    default:
        throw std::logic_error("Invalid binary_comp op: " + std::to_string(static_cast<int>(code)));
    }
}


std::int32_t binary_int_op(op_code code, const object &left, const object &right) {
    std::int32_t l = left.to_int();
    std::int32_t r = right.to_int();
    
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


object binary_arithmetic(op_code code, const object &left, const object &right) {
    return std::visit([code](auto &&l, auto &&r) -> object::type {
        using LeftT = std::remove_reference_t<decltype(l)>;
        using RightT = std::remove_reference_t<decltype(r)>;

        switch(code) {
        case op_code::pow: return std::pow(l, r);
        case op_code::add: return l + r;
        case op_code::sub: return l - r;
        case op_code::mul: return l * r;
        case op_code::div: 
            if constexpr(std::is_same_v<LeftT, std::int32_t> && std::is_same_v<RightT, std::int32_t>) {
                if(r == 0)
                    throw std::runtime_error("Division by zero");
                if(l == std::numeric_limits<std::int32_t>::min() && r == -1)
                    throw std::runtime_error("Overflow computing: " 
                            + std::to_string(std::numeric_limits<std::int32_t>::min()) + " / -1");
            }
            return l / r;
        default:
            throw std::logic_error("Invalid binary_arithmetic op: " + std::to_string(static_cast<int>(code)));
        }
    }, left.non_null_value(), right.non_null_value());
}



std::string interpreter::execute(const std::vector<char> &program) {
    memory_buffer<true> buffer(program);    // TODO: false?
    std::stack<object> operands;

    while(buffer.position() < buffer.size()) {
        op_code code = *buffer.read<op_code>();
       
        switch(code) {
        case op_code::left_paren:
        case op_code::right_paren:
            break;
        case op_code::var:
            throw std::logic_error("var is currently unsupported");
        case op_code::int_lit:
            operands.push(object(*buffer.read<std::int32_t>()));
            break;
        case op_code::float_lit:
            operands.push(object(*buffer.read<double>()));
            break;
        case op_code::null_lit:
            operands.push(object());
            break;
        case op_code::str_lit:
            throw std::logic_error("str_lit is currently unsupported");
        default:
            if(is_binary_op(code)) {
                execute_binary_op(operands, code);
            } else {
                execute_unary_op(operands, code);
            }
        }
    }

    if(operands.size() != 1) {  // TODO: remove?
        throw std::logic_error("Expected one operand in stack. size: " + std::to_string(operands.size()));
    }

    return std::visit([](auto &&value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr(std::is_same_v<T, std::monostate>) {
            return "null";
        } else {
            std::stringstream ss;
            ss << value;
            return ss.str(); 
        }
    }, operands.top().value());
}


void interpreter::execute_binary_op(std::stack<object> &operands, op_code code) {
    if(operands.size() < 2) {  // TODO: remove?
        throw std::logic_error("execute_binary_op with " + std::to_string(operands.size()) + " operands. op_code: "
                + std::to_string(static_cast<int>(code)));
    }

    object right = pop(operands);
    object left = pop(operands);
    object result;

    if(code == op_code::semicolon)
        result = right;
    else if(is_binary_comp(code))
        result = static_cast<std::int32_t>(binary_comp(code, left, right));
    else if(is_binary_int_op(code))
        result = binary_int_op(code, left, right);
    else if(is_binary_arithmetic(code))
        result = binary_arithmetic(code, left, right);
    else
        throw std::logic_error("Currently unspported binary op: " + std::to_string(static_cast<int>(code)));

    operands.push(result);
}


void interpreter::execute_unary_op(std::stack<object> &operands, op_code code) {
    if(operands.empty()) {  // TODO: remove?
        throw std::logic_error("execute_unary_op with zero operands. op_code: "
                + std::to_string(static_cast<int>(code)));
    }

    object operand = pop(operands);

    object result = std::visit([code](auto &&v) -> object::type {
        switch(code) {
        case op_code::negate:    return -v;
        case op_code::bit_not:   return ~static_cast<std::int32_t>(v);
        case op_code::logic_not: return static_cast<std::int32_t>(!v);
        default:
            throw std::logic_error("Currently unsupported unary op: " + std::to_string(static_cast<int>(code)));
        }
    }, operand.non_null_value());

    operands.push(result);
}

