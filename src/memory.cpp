#include "debug.hpp"
#include "gc.hpp"
#include "memory.hpp"
#include "object.hpp"

#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>


void memory::push_frame(std::size_t size) {
    for(std::size_t i = 0; i < size; i++)
        local_var_stack.push_back(make_lvalue());
    frames.push(frame(size, temps_stack.size()));
}


void memory::pop_frame() {
    if(debug) {
        if(frames.empty())
            debug_throw("pop_frame: no stack frames!");
        if(frames.top().size > local_var_stack.size())
            debug_throw("top frame is bigger than the stack! frame: "
                    + std::to_string(frames.top().size) + ", local vars: " 
                    + std::to_string(local_var_stack.size()));
    }

    local_var_stack.resize(local_var_stack.size() - frames.top().size);
    temps_stack.resize(frames.top().temps_start);
    frames.pop();
}


var_ref memory::get_local_var(std::size_t index) {
    if(debug) {
        if(frames.empty())
            debug_throw("get_local_var: no stack frames!");
        if(index == 0 || index > frames.top().size)
            debug_throw("invalid local_var index: " + std::to_string(index)
                    + ", top frame size: " + std::to_string(frames.top().size));
        if(index > local_var_stack.size())
            debug_throw("index greater than local_var_stack. index: "
                + std::to_string(index) + ", local_var size: " + std::to_string(local_var_stack.size()));
    }

    return local_var_stack[local_var_stack.size() - index];
}

