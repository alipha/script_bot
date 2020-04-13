#ifndef LIPH_MEMORY_HPP
#define LIPH_MEMORY_HPP

#include "object.hpp"
#include "util.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>


struct frame {
    frame(std::size_t s, std::size_t t) : size(s), temps_start(t) {}
    std::size_t size;
    std::size_t temps_start;
};


class memory {
public:
    array_ref make_array() {
        auto array = std::make_shared<std::vector<object>>();
        arrays.push_back(array);
        return array;
    }

    map_ref make_map() {
        auto map = std::make_shared<std::unordered_map<std::string, object>>();
        maps.push_back(map);
        return map;
    }

    void push_temp(object temp) {
        temps_stack.push_back(std::move(temp));
    }

    var_ref make_lvalue(object::type value = std::monostate()) {
        auto lvalue = std::make_shared<object>(std::move(value));
        lvalues.push_back(lvalue);
        return lvalue;
    }

    void push_frame(std::size_t size) {
        for(std::size_t i = 0; i < size; i++)
            local_var_stack.push_back(make_lvalue());
        frames.push(frame(size, temps_stack.size()));
    }

    void pop_frame() {
        if constexpr(debug) {
            if(frames.empty())
                throw std::logic_error("pop_frame: no stack frames!");
            if(frames.top().size > local_var_stack.size())
                throw std::logic_error("top frame is bigger than the stack! frame: "
                        + std::to_string(frames.top().size) + ", local vars: " 
                        + std::to_string(local_var_stack.size()));
        }

        local_var_stack.resize(local_var_stack.size() - frames.top().size);
        temps_stack.resize(frames.top().temps_start);
        frames.pop();
    }

    var_ref get_local_var(std::size_t index) {
        if constexpr(debug) {
            if(frames.empty())
                throw std::logic_error("get_local_var: no stack frames!");
            if(index == 0 || index > frames.top().size)
                throw std::logic_error("invalid local_var index: " + std::to_string(index)
                        + ", top frame size: " + std::to_string(frames.top().size));
            if(index > local_var_stack.size())
                throw std::logic_error("index greater than local_var_stack. index: "
                    + std::to_string(index) + ", local_var size: " + std::to_string(local_var_stack.size()));
        }

        return local_var_stack[local_var_stack.size() - index];
    }

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

