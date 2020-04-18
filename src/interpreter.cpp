#include "interpreter.hpp"
#include "debug.hpp"
#include "conversion.hpp"
#include "memory.hpp"
#include "memory_buffer.hpp"
#include "object.hpp"
#include "operation_type.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <sstream>
#include <type_traits>
#include <variant>
#include <vector>


using namespace std::string_literals;

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

/*
template<typename Int>
auto to_optional_int(Int &&x) {
    using T = std::decay_t<Int>;
    if constexpr(std::is_same_v<T, std::monostate>)
        return std::optional<std::int64_t>();
    else
        return std::optional<T>(x);
}
*/



template<typename T, typename U>
bool do_binary_comp(op_code code, const std::optional<T> &l, const std::optional<U> &r) {
    switch(code) {
    case op_code::lt:  return l < r;
    case op_code::lte: return l <= r;
    case op_code::gt:  return l > r;
    case op_code::gte: return l >= r;
    case op_code::eq:  return l == r;
    case op_code::neq: return l != r;
    default:
        throw std::logic_error("Invalid binary_comp op: "s + lookup_operation(code).symbol);
    }
}


bool binary_comp(op_code code, const object &left, const object &right) {

    switch(code) {
    case op_code::logic_and: return left.to_bool() && right.to_bool();
    case op_code::logic_or:  return left.to_bool() || right.to_bool();
    default:
        return std::visit([code](auto &&left, auto &&right) -> bool {
            using L = std::decay_t<decltype(left)>;
            using R = std::decay_t<decltype(right)>;

            if constexpr(std::is_same_v<L, std::monostate> && std::is_same_v<R, std::monostate>) {
                return do_binary_comp(code, std::optional<int>(), std::optional<int>());
            } else if constexpr(std::is_same_v<L, string_ref> || std::is_same_v<R, string_ref>) {
                return do_binary_comp(code, to_optional_string(left), to_optional_string(right));
            } else if constexpr(std::is_same_v<L, std::monostate>) {
                return do_binary_comp(code, std::optional<R>(), std::optional<R>(right));
            } else if constexpr(std::is_same_v<R, std::monostate>) {
                return do_binary_comp(code, std::optional<L>(left), std::optional<L>());
            // TODO: deep comparison of array_ref? (and map_ref?)
            } else if constexpr((std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) || std::is_same_v<L, R>) {
                return do_binary_comp(code, std::optional<L>(left), std::optional<R>(right));
            } else {
                throw std::runtime_error("Performing "s + lookup_operation(code).symbol + " between different types");
            }
        }, left.value(), right.value());
    }
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
            throw std::logic_error("Invalid binary_int_op: "s + lookup_operation(code).symbol);
        }
    }, left.to_int(), right.to_int());
}


object binary_arithmetic(op_code code, const object &left, const object &right) {
    // TODO: operator+ for arrays?
    if (std::holds_alternative<string_ref>(left.value()) 
            || std::holds_alternative<string_ref>(right.value())) {
        if(code == op_code::add) {
            return object::type(std::make_shared<std::string>(left.to_string() + right.to_string()));
        } else {
            throw std::runtime_error("String does not support "s + lookup_operation(code).symbol);
        }
    } else {
        return std::visit([code](auto &&l, auto &&r) -> object::type {
            using LeftT = std::decay_t<decltype(l)>;
            using RightT = std::decay_t<decltype(r)>;

            if constexpr(!std::is_arithmetic_v<LeftT> || !std::is_arithmetic_v<RightT>) {
                throw std::runtime_error("Performing "s + lookup_operation(code).symbol + " on non-arithmetic type type");
            } else {
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
                    throw std::logic_error("Invalid binary_arithmetic op: "s + lookup_operation(code).symbol);
                }
            }
        }, left.non_null_value(), right.non_null_value());
    }
}


void execute_control_statement(memory_buffer<debug> &buffer, std::stack<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_if_statement with zero operands. op_code: "s
                    + lookup_operation(code).symbol);
        }
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(!pop(operands).to_bool())
        buffer.seek_abs(jump_pos);
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
        case op_code::colon:
        case op_code::block_end:
        case op_code::while_cond:
            break;
        case op_code::while_block:
            pop(operands);
            buffer.seek_abs(*buffer.read<std::uint32_t>());
            break;
        case op_code::if_block:
            pop(operands);
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
            operands.push(object(std::make_shared<std::string>(buffer.read_str())));
            break;
        case op_code::array_start:
            operands.push(object(mem->make_array()));
            break;
        case op_code::map_start:
            operands.push(object(mem->make_map()));
            break;
        case op_code::array_add:
        case op_code::array_end:
            array_add(operands);
            break;
        case op_code::map_add:
        case op_code::map_end:
            map_add(operands);
            break;
        case op_code::if_start:
        case op_code::while_start:
            execute_control_statement(buffer, operands, code);
            break;
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
    
    std::string result = operands.top().to_string();
    mem->pop_frame();
    return result;
}


void interpreter::array_add(std::stack<object> &operands) {
    if constexpr(debug) {
        if(operands.size() < 2)
            throw std::logic_error("execute array_add with " + std::to_string(operands.size()) + "operands");
    }

    object element = pop(operands);
    std::get<array_ref>(operands.top().value())->push_back(element);
}


void interpreter::map_add(std::stack<object> &operands) {
    if constexpr(debug) {
        if(operands.size() < 3)
            throw std::logic_error("execute map_add with " + std::to_string(operands.size()) + "operands");
    }

    object value = pop(operands);
    object key = pop(operands);
    std::get<map_ref>(operands.top().value())->try_emplace(key.to_string(), value);
}


object interpreter::index_op(const object &left, const object &right) {
    bool temp = !std::holds_alternative<lvalue_ref>(left.get()) 
        && !std::holds_alternative<var_ref>(left.get()); 
    if(temp)
        mem->push_temp(left);

    return object::type(std::visit([](auto &&l, auto &&r) -> lvalue_ref {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr(std::is_same_v<L, array_ref>) {
            std::int64_t index = to_int(r);
            if(index < 0 || static_cast<std::size_t>(index) >= l->size())
                throw std::runtime_error("array index out of bounds: " + std::to_string(index) 
                        + ", size: " + std::to_string(l->size()));
            return &(*l)[index];
        } else if constexpr(std::is_same_v<L, map_ref>) {
            if constexpr(std::is_integral_v<R> || std::is_same_v<R, string_ref>) {
                return &l->try_emplace(to_str(r), std::monostate()).first->second;
            } else {
                throw std::runtime_error("map index with non-string/non-int type");
            }
        //} else if constexpr(std::is_same_v<L, string_ref>) {  // TODO: string indexing
        } else {
            throw std::runtime_error("object does not support []");
        }
    }, left.non_null_value(), right.non_null_value()));
}


void interpreter::execute_binary_op(std::stack<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.size() < 2) {
            throw std::logic_error("execute_binary_op with " + std::to_string(operands.size()) 
                    + " operands. op_code: " + lookup_operation(code).symbol);
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
    else if(code == op_code::index)
        result = index_op(left, right);
    else if(is_binary_comp(code))
        result = static_cast<std::int64_t>(binary_comp(code, left, right));
    else if(is_binary_int_op(code))
        result = binary_int_op(code, left, right);
    else if(is_binary_arithmetic(code))
        result = binary_arithmetic(code, left, right);
    else
        throw std::logic_error("currently unspported binary op: "s + lookup_operation(code).symbol);

    if(is_assign) {
        if(var_ref *var = std::get_if<var_ref>(&left.get()))
            **var = to_variant<object::type>(result.value());
        else if(lvalue_ref *lvalue = std::get_if<lvalue_ref>(&left.get()))
            **lvalue = to_variant<object::type>(result.value()); 
        else
            throw std::runtime_error("left of "s + lookup_operation(code).symbol + " is not assignable");
    } else {
        operands.push(result);
    }
}


void interpreter::execute_unary_op(std::stack<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_unary_op with zero operands. op_code: "s
                    + lookup_operation(code).symbol);
        }
    }

    object operand = pop(operands);

    if(code == op_code::logic_not) {
        operands.push(object(static_cast<std::int64_t>(!operand.to_bool())));
        return;
    }

    object result = std::visit([code](auto &&v) -> object::type {
        using T = std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T, string_ref>) {
            if(code == op_code::plus) {
                if(!v->empty() && v->front() == '-')
                    return static_cast<std::int64_t>(std::stoll(*v));
                else
                    return static_cast<std::uint64_t>(std::stoull(*v));
            } else {
                throw std::runtime_error("String does not support: "s + lookup_operation(code).symbol);
            }
        } else if constexpr(!std::is_arithmetic_v<T>) {
                throw std::runtime_error("Performing "s + lookup_operation(code).symbol + " on non-arithmetic type type");
        } else {
            switch(code) {
            case op_code::plus:      return static_cast<std::int64_t>(v);
            case op_code::negate:    return -v;
            case op_code::bit_not:   return ~to_int(v);
            //case op_code::logic_not: return static_cast<std::int64_t>(!v);
            default:
                throw std::logic_error("Currently unsupported unary op: "s + lookup_operation(code).symbol);
            }
        }
    }, operand.non_null_value());

    operands.push(result);
}

