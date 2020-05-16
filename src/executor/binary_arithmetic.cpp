#include "executor.hpp"
#include "object.hpp"
#include "operation_type.hpp"
#include "string_util.hpp"

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <variant>


using namespace std::string_literals;


namespace executor {


object binary_arithmetic(op_code code, const object &left, const object &right) {
    // TODO: operator+ for arrays?
    if (std::holds_alternative<string_ref>(left.value()) 
            || std::holds_alternative<string_ref>(right.value())) {
        if(code == op_code::add) {
            return object::type(make_string(left.to_string() + right.to_string()));
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


}  // namespace executor

