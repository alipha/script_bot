#include "executor.hpp"
#include "conversion.hpp"
#include "debug.hpp"
#include "object.hpp"
#include "operation_type.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"

#include <stack>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <variant>


using namespace std::string_literals;


namespace executor {


void unary_op(std::stack<object> &operands, op_code code) {
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
			case op_code::negate:    return -static_cast<std::int64_t>(v);
            case op_code::bit_not:   return ~to_int(v);
            //case op_code::logic_not: return static_cast<std::int64_t>(!v);
            default:
                throw std::logic_error("Currently unsupported unary op: "s + lookup_operation(code).symbol);
            }
        }
    }, operand.non_null_value());

    operands.push(result);
}


} // namespace executor

