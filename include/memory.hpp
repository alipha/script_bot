#ifndef LIPH_MEMORY_HPP
#define LIPH_MEMORY_HPP

#include "object.hpp"
#include "util.hpp"

#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>


struct frame {
    explicit frame(std::size_t s) : size(s) {}
    std::size_t size;
};


class memory {
public:
    array_ref make_array() {
        auto array = std::make_shared<std::vector<object>>();
        arrays.push_front(array);
        return array;
    }

    var_ref make_lvalue(object::type value = std::monostate()) {
        auto lvalue = std::make_shared<object>(std::move(value));
        lvalues.push_front(lvalue);
        return lvalue;
    }

    void push_frame(std::size_t size) {
        for(std::size_t i = 0; i < size; i++)
            local_var_stack.push_back(make_lvalue());
        frames.push(frame(size));
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
    std::list<std::weak_ptr<std::vector<object>>> arrays;
    std::list<std::weak_ptr<object>> lvalues;

    std::unordered_map<std::string, var_ref> globals;
    std::vector<var_ref> local_var_stack;
    std::stack<frame> frames;
};

#endif

