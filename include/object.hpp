#ifndef LIPH_OBJECT_HPP
#define LIPH_OBJECT_HPP

#include "gc.hpp"
#include "memory_buffer.hpp"
#include "object_fwd.hpp"
#include "variant_util.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>


struct func_def {
    func_def(memory_buffer<debug> &&c, gcvector<std::shared_ptr<func_def>> &&funcs, gcstring &&text)
        : code(std::move(c)), func_lits(std::move(funcs)), source_text(std::move(text)) {}

    memory_buffer<debug> code;
    gcvector<std::shared_ptr<func_def>> func_lits;
    gcstring source_text;
};


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
    return gc::make_ptr<object>(std::move(value));
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


template<typename T>
constexpr bool is_non_ref() {
    if constexpr(std::is_same_v<T, std::monostate> 
            || std::is_same_v<T, std::int64_t> 
            || std::is_same_v<T, std::uint64_t>
            || std::is_same_v<T, double>)
        return true;
    else
        return false;
}

template<typename T>
constexpr bool is_ref() {
    if constexpr(std::is_same_v<T, string_ref> 
            || std::is_same_v<T, array_ref>
            || std::is_same_v<T, map_ref>
            || std::is_same_v<T, func_ref>)
        return true;
    else
        return false;
}


#endif

