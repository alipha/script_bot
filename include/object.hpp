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
using lvalue_ref = std::shared_ptr<object>;
using local_ref = object*;


class object {
public:
    using non_null_type = std::variant<std::int32_t, double/*, string_ref, array_ref*/>;
    using value_type = variant_push_t<std::monostate, non_null_type>;
    using type = variant_push_t<value_type, lvalue_ref>;

    object() : val() {}
    object(type v) : val(std::move(v)) {}

    object &operator=(type v) {
        val = std::move(v);
        return *this;
    }

    const type &get() const { return val; }

    type &get() { return val; }

    
    std::int32_t to_int() const {
        return std::visit([](auto &&arg) { return static_cast<std::int32_t>(arg); }, non_null_value());
    }

    std::optional<std::int32_t> to_optional_int() const {
        return std::visit([](auto &&v) -> std::optional<std::int32_t> {
            using T = std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T, std::monostate>)
                return {};
            else
                return static_cast<std::int32_t>(v);
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
            if constexpr(std::is_same_v<T, lvalue_ref> /*|| std::is_same_v<T, local_ref>*/)
                return v->value();
            else
                return v;
        }, val);
    }
private:
    type val;
};


#endif

