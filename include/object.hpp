#ifndef LIPH_OBJECT_HPP
#define LIPH_OBJECT_HPP

#include "gc.hpp"
#include "object_fwd.hpp"
#include "variant_util.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
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

    std::optional<gcstring> to_optional_string(std::size_t depth = 0, std::size_t *count = nullptr, bool format = false) const;

    gcstring to_string(std::size_t depth = 0, std::size_t *count = nullptr, bool format = false) const;

    int_type to_int() const;

    bool to_bool() const;


    void transverse(gc::action &act) { act(val); }

private:
    type val;
};


inline array_ref make_array() { return gc::make_ptr<gcvector<object>>(); }

inline map_ref make_map() { return gc::make_ptr<gcmap>(); }

inline var_ref make_lvalue(object::type value = std::monostate()) {
    auto p = gc::make_ptr<object>();
    *p = object(std::move(value));
    return p;
}

inline string_ref make_string(gcstring str) { 
    return std::allocate_shared<gcstring>(gc::allocator<gcstring>(), std::move(str)); 
}

inline string_ref make_string(std::string_view str) {
    return std::allocate_shared<gcstring>(gc::allocator<gcstring>(), str.begin(), str.end());
}



inline gc::anchor<object> pop(std::vector<object> &v) {
    if(debug && v.empty())
        debug_throw("calling pop() on empty vector<object>!");

    gc::anchor<object> value = std::move(v.back());
    v.pop_back();
    return value;
}


#endif

