#include "debug.hpp"
#include "gc.hpp"
#include "memory.hpp"
#include "object.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>


void memory::push_frame(std::size_t size) {
    for(std::size_t i = 0; i < size; i++)
        local_var_stack->push_back(make_lvalue());
    frame_stack.push_back(frame(size, temps_stack->size()));
}


void memory::pop_frame() {
    if(debug) {
        if(frame_stack.empty())
            debug_throw("pop_frame: no stack frame_stack!");
        if(frame_stack.back().size > local_var_stack->size())
            debug_throw("top frame is bigger than the stack! frame: "
                    + std::to_string(frame_stack.back().size) + ", local vars: " 
                    + std::to_string(local_var_stack->size()));
    }

    local_var_stack->resize(local_var_stack->size() - frame_stack.back().size);
    temps_stack->resize(frame_stack.back().temps_start);
    frame_stack.pop_back();
}


void memory::clear_stack() {
    temps_stack->clear();
    local_var_stack->clear();
    frame_stack.clear();
}


var_ref memory::get_local_var(std::size_t index) {
    if(debug) {
        if(frame_stack.empty())
            debug_throw("get_local_var: no stack frame_stack!");
        if(index == 0 || index > frame_stack.back().size)
            debug_throw("invalid local_var index: " + std::to_string(index)
                    + ", top frame size: " + std::to_string(frame_stack.back().size));
        if(index > local_var_stack->size())
            debug_throw("index greater than local_var_stack. index: "
                + std::to_string(index) + ", local_var size: " + std::to_string(local_var_stack->size()));
    }

    return (*local_var_stack)[local_var_stack->size() - index];
}

