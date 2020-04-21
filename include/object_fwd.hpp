#ifndef LIPH_OBJECT_FWD_HPP
#define LIPH_OBJECT_FWD_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


class object;


using string_ref = std::shared_ptr<std::string>;
using array_ref = std::shared_ptr<std::vector<object>>;
using map_ref = std::shared_ptr<std::unordered_map<std::string, object>>;
using var_ref = std::shared_ptr<object>;    // a named variable (a global variable or capture, or currently, a local variable)
using lvalue_ref = object*;    // a pointer to an assignable object (e.g., the result of x[i]). In the future, possibly local variables.


struct func_type {
    std::vector<char> code;     // TODO: share the code among func_types?
    std::vector<var_ref> captures;
};

using func_ref = std::shared_ptr<func_type>;


#endif
