#include "object.hpp"
#include "conversion.hpp"

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>


object::value_type object::value() const {
    return std::visit([](auto &&v) -> value_type {
        using T = std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T, var_ref> || std::is_same_v<T, lvalue_ref>)
            return v->value();
        else
            return v;
    }, val);
}


object::non_null_type object::non_null_value() const {
    return std::visit([](auto &&v) -> non_null_type {
        using T = std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T, std::monostate>)
            throw std::runtime_error("unexpected null value");
        else
            return std::forward<T>(v);
    }, value());
}


std::optional<gcstring> object::to_optional_string(std::size_t depth, std::size_t *count, bool format) const {
    return std::visit([=](auto &&v) { 
        return ::to_optional_string(std::forward<decltype(v)>(v), depth, count, format); 
    }, value());
}

gcstring object::to_string(std::size_t depth, std::size_t *count, bool format) const {
    return to_optional_string(depth, count, format).value_or("null");
}


object::int_type object::to_int() const {
    return std::visit([](auto &&arg) -> int_type { 
        return ::to_int(std::forward<decltype(arg)>(arg)); 
    }, non_null_value());
}

bool object::to_bool() const {
    return std::visit([](auto &&v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T, std::monostate>)
            return false;
        else if constexpr(std::is_same_v<T, string_ref>)
            return !v->empty();
        else
            return !!v;
    }, value());
}

/*
nullable_int_type to_nullable_int() const {
    return std::visit([](auto &&v) -> nullable_int_type {
        using T = std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T, std::monostate> || std::is_same_v<T, std::uint64_t>)
            return v;
        else if constexpr(std::is_arithmetic_v<T>)
            return static_cast<std::int64_t>(v);
        else
            throw std::runtime_error("expected arithmetic value"); 
    }, value());
}
*/

