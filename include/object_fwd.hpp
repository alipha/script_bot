#ifndef LIPH_OBJECT_FWD_HPP
#define LIPH_OBJECT_FWD_HPP

#include "gc.hpp"
#include <cstddef>
#include <memory>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>


class object;
class func_def;


using gcstring = std::basic_string<char, std::char_traits<char>, gc::allocator<char>>;


namespace std {
    template<>
    struct hash<gcstring> {
        std::size_t operator()(const gcstring &str) const {
            return hash<std::string_view>()(std::string_view(str.data(), str.size()));
        }
    };
}


template<typename T>
using gcvector = std::vector<T, gc::allocator<T>>;

using gcmap = std::unordered_map<gcstring, object>;

using string_ref = std::shared_ptr<gcstring>;
using array_ref = gc::ptr<gcvector<object>>;
using map_ref = gc::ptr<gcmap>;
using var_ref = gc::ptr<object>;    // a named variable (a global variable or capture, or currently, a local variable)
using lvalue_ref = object*;    // a pointer to an assignable object (e.g., the result of x[i]). In the future, possibly local variables.


struct func_type {
    func_type(std::shared_ptr<func_def> def, gcvector<var_ref> &&caps) :
        definition(std::move(def)), captures(std::move(caps)) {}
   
    void transverse(gc::action &act) { act(captures); }

    std::shared_ptr<func_def> definition;
    gcvector<var_ref> captures;
};

using func_ref = gc::ptr<func_type>;


#endif
