#include "executor.hpp"
#include "conversion.hpp"
#include "debug.hpp"
#include "object.hpp"
#include "operation_type.hpp"
#include "string_util.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>


using namespace std::string_literals;


namespace executor {


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


}  // namespace executor

