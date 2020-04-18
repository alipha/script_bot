#ifndef LIPH_MEMORY_HPP
#define LIPH_MEMORY_HPP

#include "object.hpp"

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>


struct frame {
    frame(std::size_t s, std::size_t t) : size(s), temps_start(t) {}
    std::size_t size;
    std::size_t temps_start;
};


class memory {
public:
    array_ref make_array();
    map_ref make_map();
    var_ref make_lvalue(object::type value = std::monostate());

    void push_frame(std::size_t size);
    void pop_frame();

    var_ref get_local_var(std::size_t index);

    void push_temp(object temp);

private:
    std::vector<std::weak_ptr<std::vector<object>>> arrays;
    std::vector<std::weak_ptr<std::unordered_map<std::string, object>>> maps;
    std::vector<std::weak_ptr<object>> lvalues;

    std::unordered_map<std::string, var_ref> globals;
    std::vector<object> temps_stack;
    std::vector<var_ref> local_var_stack;
    std::stack<frame> frames;
};

#endif

