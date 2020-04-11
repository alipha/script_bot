#ifndef LIPH_OBJECT_HPP
#define LIPH_OBJECT_HPP

#include "util.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>



template<typename To, typename From>
To to_variant(From &&from) { return std::visit([](auto &&arg) -> To { return arg; }, std::forward<From>(from)); }



struct object;



using string_ref = std::shared_ptr<std::string>;
using array_ref = std::shared_ptr<std::vector<object>>;
using map_ref = std::shared_ptr<std::unordered_map<std::string, object>>;
using var_ref = std::shared_ptr<object>;
using lvalue_ref = object*;


struct func_type {
    std::vector<char> code;
    std::vector<var_ref> captures;
};

using func_ref = std::shared_ptr<func_type>;



template<typename Int>
std::int64_t to_int(Int &&arg) {
    using T = std::decay_t<Int>;
    if constexpr(std::is_arithmetic_v<T>)
        return static_cast<std::int64_t>(arg);
    else
        throw std::runtime_error("expected arithmetic value"); 
}

inline std::uint64_t to_int(std::uint64_t arg) { return arg; }



inline std::optional<std::string> to_optional_string(std::monostate) { return {}; }
inline std::optional<std::string> to_optional_string(string_ref s) { return {*s}; }
inline std::optional<std::string> to_optional_string(func_ref) { return {"<function>"}; }
inline std::optional<std::string> to_optional_string(std::uint64_t v) { return {std::to_string(v)}; }
inline std::optional<std::string> to_optional_string(std::int64_t v) { return {std::to_string(v)}; }

inline std::optional<std::string> to_optional_string(double v) {
    thread_local std::stringstream ss;
    ss.str("");
    ss.precision(15);
    ss << v;
    return ss.str();
}

inline std::optional<std::string> to_optional_string(array_ref ref);
inline std::optional<std::string> to_optional_string(map_ref ref);

template<typename T>
inline std::string to_str(const T &v) { return to_optional_string(v).value_or("null"); }



class object {
public:
    using non_null_type = std::variant<std::int64_t, std::uint64_t, double, 
          string_ref, array_ref, map_ref, func_ref>;
    using value_type = variant_push_t<std::monostate, non_null_type>;
    using type = variant_push_t<value_type, var_ref>;
    using int_type = std::variant<std::int64_t, std::uint64_t>; 
    //using nullable_int_type = variant_push_t<std::monostate, int_type>;

    object() : val() {}
    object(type v) : val(std::move(v)) {}

    object &operator=(type v) {
        val = std::move(v);
        return *this;
    }

    const type &get() const { return val; }

    type &get() { return val; }

    
    std::optional<std::string> to_optional_string() const {
        return std::visit([](auto &&v) { return ::to_optional_string(v); }, value());
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
            return ::to_int(arg);
        }, non_null_value());
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


inline std::optional<std::string> to_optional_string(array_ref ref) {
    if(ref->empty())
        return {"[]"};

    std::string out("[");

    for(object &obj : *ref)
        out += obj.to_string() + ", "; // TODO: to_formatted_string()

    out.pop_back();
    out.back() = ']';
    return {out};
}

inline std::optional<std::string> to_optional_string(map_ref ref) {
    if(ref->empty())
        return {"{}"};

    std::string out("{");

    for(const auto &keypair : *ref)
        out += keypair.first + ": " + keypair.second.to_string() + ", "; // TODO: to_formatted_string()

    out.pop_back();
    out.back() = '}';
    return {out};
}


#endif

