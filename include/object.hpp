#ifndef LIPH_OBJECT_HPP
#define LIPH_OBJECT_HPP

#include "object_fwd.hpp"
#include "variant_util.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>


class object {
public:
    using non_null_type = std::variant<std::int64_t, std::uint64_t, double, 
          string_ref, array_ref, map_ref, func_ref>;
    using value_type = variant_push_t<std::monostate, non_null_type>;
    using type = variant_push_t<variant_push_t<value_type, var_ref>, lvalue_ref>;
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

    
    value_type value() const;

    non_null_type non_null_value() const;

    std::optional<std::string> to_optional_string(std::size_t depth = 0, bool format = false) const;

    std::string to_string(std::size_t depth = 0, bool format = false) const;

    int_type to_int() const;

    bool to_bool() const;

private:
    type val;
};


#endif

