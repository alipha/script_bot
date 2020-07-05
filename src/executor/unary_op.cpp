#include "executor.hpp"
#include "conversion.hpp"
#include "debug.hpp"
#include "gc.hpp"
#include "object.hpp"
#include "operation_type.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"
#include "variant_util.hpp"

#include <string>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>


using namespace std::string_literals;


namespace executor {


void unary_op(gc::anchor<object> &last_value, std::vector<object> &operands, std::size_t parent_operand_count, op_code code) {
    if(code == op_code::semicolon) {
        //std::cout << "semicolon: " << operands.size() << ", " << parent_operand_count << std::endl;
        if(operands.size() > parent_operand_count)
            last_value = pop(operands);
        //operands.clear();   // TODO: should i do this?
        return;
    }

    if(debug && operands.size() <= parent_operand_count) {
        throw std::logic_error("execute_unary_op with zero operands. op_code: "s
                + lookup_operation(code).symbol);
    }

    bool is_pre = (code == op_code::pre_inc || code == op_code::pre_dec);
    bool is_post = (code == op_code::post_inc || code == op_code::post_dec);
    object operand = is_pre ? operands.back() : *pop(operands);
    
    if(is_post)
        operands.push_back(to_variant<object::type>(operand.value()));

    if(code == op_code::logic_not) {
        operands.push_back(object(static_cast<std::int64_t>(!operand.to_bool())));
        return;
    }

    object result = object(std::visit([code](auto &&v) -> object::type {
        using T = std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T, string_ref>) {
            if(code == op_code::plus) {
                if(!v->empty() && v->front() == '-')
                    return static_cast<std::int64_t>(std::stoll(to_std_string(*v)));
                else
                    return static_cast<std::uint64_t>(std::stoull(to_std_string(*v)));
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
            case op_code::pre_inc:   return v + 1;
            case op_code::pre_dec:   return v - 1;
            case op_code::post_inc:  return v + 1;
            case op_code::post_dec:  return v - 1;
            default:
                throw std::logic_error("Currently unsupported unary op: "s + lookup_operation(code).symbol);
            }
        }
    }, operand.non_null_value()));

    if(is_pre || is_post) {
        if(var_ref *var = std::get_if<var_ref>(&operand.get())) {
            **var = to_variant<object::type>(result.value());
        } else if(lvalue_ref *lvalue = std::get_if<lvalue_ref>(&operand.get())) {
            **lvalue = to_variant<object::type>(result.value()); 
        } else {
            throw std::runtime_error("left of "s + lookup_operation(code).symbol + " is not assignable");
        }
    } else {
        operands.push_back(std::move(result));
    }
}


} // namespace executor

