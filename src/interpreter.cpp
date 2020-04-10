#include "interpreter.hpp"
#include "memory.hpp"
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


template<typename Int>
auto to_optional_int(Int &&x) {
    using T = std::decay_t<Int>;
    if constexpr(std::is_same_v<T, std::monostate>)
        return std::optional<std::int64_t>();
    else
        return std::optional<T>(x);
}

template<typename Int>
auto to_int(Int &&x) {
    using T = std::decay_t<Int>;
    if constexpr(std::is_same_v<T, std::uint64_t>)
        return x;
    else
        return static_cast<std::int64_t>(x);
}


bool binary_comp(op_code code, const object &left, const object &right) {
    auto l = left.to_nullable_int();
    auto r = right.to_nullable_int();

    return std::visit([code](auto &&left, auto &&right) {
        auto l = to_optional_int(left);
        auto r = to_optional_int(right);

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
    }, left.to_nullable_int(), right.to_nullable_int());
}


object binary_int_op(op_code code, const object &left, const object &right) {
    
    return std::visit([code](auto &&l, auto &&r) -> object::type {
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
    }, left.to_int(), right.to_int());
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
            if constexpr(std::is_integral_v<LeftT> && std::is_integral_v<RightT>) {
                if(r == 0)
                    throw std::runtime_error("Division by zero");
            }
            if constexpr(std::is_same_v<LeftT, std::int64_t> && std::is_same_v<RightT, std::int64_t>) {
                if(l == std::numeric_limits<std::int64_t>::min() && r == -1)
                    throw std::runtime_error("Overflow computing: " 
                            + std::to_string(std::numeric_limits<std::int64_t>::min()) + " / -1");
            }
            return l / r;
        default:
            throw std::logic_error("Invalid binary_arithmetic op: " + std::to_string(static_cast<int>(code)));
        }
    }, left.non_null_value(), right.non_null_value());
}



std::string interpreter::execute(const std::vector<char> &program) {
    memory_buffer<debug> buffer(program);
    std::stack<object> operands;

    mem->push_frame(*buffer.read<std::uint8_t>());

    while(buffer.position() < buffer.size()) {
        op_code code = *buffer.read<op_code>();
       
        switch(code) {
        case op_code::left_paren:
        case op_code::right_paren:
            break;
        case op_code::global_var:
            throw std::logic_error("global_var is currently unsupported");
        case op_code::local_var:
            operands.push(object(mem->get_local_var(*buffer.read<std::uint8_t>())));
            break;
        case op_code::int_lit:
            operands.push(object(*buffer.read<std::int64_t>()));
            break;
        case op_code::uint_lit:
            operands.push(object(*buffer.read<std::uint64_t>()));
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

    if constexpr(debug) {
        if(operands.size() != 1) {
            throw std::logic_error("Expected one operand in stack. size: " + std::to_string(operands.size()));
        }
    }

    std::string result = std::visit([](auto &&value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr(std::is_same_v<T, std::monostate>) {
            return "null";
        } else {
            std::stringstream ss;
            ss << value;
            return ss.str(); 
        }
    }, operands.top().value());

    mem->pop_frame();
    return result;
}


void interpreter::execute_binary_op(std::stack<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.size() < 2) {
            throw std::logic_error("execute_binary_op with " + std::to_string(operands.size()) 
                    + " operands. op_code: " + std::to_string(static_cast<int>(code)));
        }
    }

    bool is_assign = is_binary_assignment(code);
    object right = pop(operands);
    object left = is_assign ? operands.top() : pop(operands);
    object result;

    if(is_assign) {
        code = static_cast<op_code>(static_cast<int>(code) - assign_ops_offset);
    }

    if(code == op_code::semicolon)
        result = right;
    else if(is_binary_comp(code))
        result = static_cast<std::int64_t>(binary_comp(code, left, right));
    else if(is_binary_int_op(code))
        result = binary_int_op(code, left, right);
    else if(is_binary_arithmetic(code))
        result = binary_arithmetic(code, left, right);
    else
        throw std::logic_error("Currently unspported binary op: " + std::to_string(static_cast<int>(code)));

    if(is_assign) {
        *std::get<lvalue_ref>(left.get()) = to_variant<object::type>(result.value());
    } else {
        operands.push(result);
    }
}


void interpreter::execute_unary_op(std::stack<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_unary_op with zero operands. op_code: "
                    + std::to_string(static_cast<int>(code)));
        }
    }

    object operand = pop(operands);

    object result = std::visit([code](auto &&v) -> object::type {
        switch(code) {
        case op_code::plus:      return static_cast<std::int64_t>(v);
        case op_code::negate:    return -v;
        case op_code::bit_not:   return ~to_int(v);
        case op_code::logic_not: return static_cast<std::int64_t>(!v);
        default:
            throw std::logic_error("Currently unsupported unary op: " + std::to_string(static_cast<int>(code)));
        }
    }, operand.non_null_value());

    operands.push(result);
}

