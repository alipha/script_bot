#ifndef LIPH_OBJECT_HPP
#define LIPH_OBJECT_HPP

#include "util.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>


struct object;

using string_ref = std::shared_ptr<std::string>;
using array_ref = std::shared_ptr<std::vector<object>>;
//using map_ref = std::shared_ptr<std::unordered_map<std::string, object>>;
using var_ref = std::shared_ptr<object>;
using lvalue_ref = object*;  // or std::size_t?


template<typename To, typename From>
To to_variant(From &&from) { return std::visit([](auto &&arg) -> To { return arg; }, std::forward<From>(from)); }


class object {
public:
    using non_null_type = std::variant<std::int64_t, std::uint64_t, double, string_ref/*, array_ref*/>;
    using value_type = variant_push_t<std::monostate, non_null_type>;
    using type = variant_push_t<value_type, var_ref>;
    using int_type = std::variant<std::int64_t, std::uint64_t>; 
    using nullable_int_type = variant_push_t<std::monostate, int_type>;

    object() : val() {}
    object(type v) : val(std::move(v)) {}

    object &operator=(type v) {
        val = std::move(v);
        return *this;
    }

    const type &get() const { return val; }

    type &get() { return val; }

   
    std::optional<std::string> to_optional_string() const {
        return std::visit([](auto &&v) -> std::optional<std::string> {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T, std::monostate>)
                return {};
            else if constexpr(std::is_same_v<T, string_ref>)
                return {*v};
            else
                return {std::to_string(v)};
        }, value());
    }

    std::string to_string() const {
        return to_optional_string().value_or("null");
    }

    bool to_bool() const {
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

    int_type to_int() const {
        return std::visit([](auto &&arg) -> int_type { 
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, std::uint64_t>)
                return arg;
            else if constexpr(std::is_arithmetic_v<T>)
                return static_cast<std::int64_t>(arg);
            else
                throw std::runtime_error("expected arithmetic value"); 
        }, non_null_value());
    }

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

    non_null_type non_null_value() const {
        return std::visit([](auto &&v) -> non_null_type {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T, std::monostate>)
                throw std::runtime_error("unexpected null value");
            else
                return v;
        }, value());
    }

    value_type value() const {
        return std::visit([](auto &&v) -> value_type {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T, var_ref> /*|| std::is_same_v<T, lvalue_ref>*/)
                return v->value();
            else
                return v;
        }, val);
    }
private:
    type val;
};


#endif

