#include "debug.hpp"
#include "gc.hpp"
#include "memory.hpp"
#include "object.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>


void memory::push_frame(std::size_t current_pos, std::size_t current_operand_count, func_ref func, array_ref params) {
    gc::anchor_ptr<func_type> func_anchor = func;
    gc::anchor_ptr<gcvector<object>> params_anchor = params;

    memory_buffer<debug> &buffer = func->definition->code;
    buffer.seek_abs(0);
    std::uint8_t param_count = *buffer.read<std::uint8_t>();
    std::size_t local_var_count = *buffer.read<std::uint8_t>() - param_count;
    
    std::uint8_t capture_count = *buffer.read<std::uint8_t>();
    std::size_t code_size = buffer.size() - capture_count;

    for(std::size_t i = params->size(); i < param_count; ++i) {
        gc::anchor_ptr<object> param = make_lvalue();
        params->push_back(object(param));
    }

    for(std::size_t i = 0; i < local_var_count; ++i)
        local_var_stack->push_back(make_lvalue());

    frame f = {current_pos, current_operand_count, local_var_count, 
        temps_stack->size(), std::move(func), code_size,
        std::move(params), param_count};
    frame_stack->push_back(std::move(f));
}


std::size_t memory::pop_frame() {
    if(debug) {
        if(frame_stack->empty())
            debug_throw("pop_frame: frame_stack empty!");
        if(frame_stack->back().local_var_count > local_var_stack->size())
            debug_throw("top frame is bigger than the stack! frame: "
                    + std::to_string(frame_stack->back().local_var_count) + ", local vars: " 
                    + std::to_string(local_var_stack->size()));
    }

    std::size_t parent_pos = frame_stack->back().parent_pos;
    local_var_stack->resize(local_var_stack->size() - frame_stack->back().local_var_count);
    temps_stack->resize(frame_stack->back().temps_start);
    frame_stack->pop_back();
    return parent_pos;
}


void memory::clear_stack() {
    temps_stack->clear();
    local_var_stack->clear();
    frame_stack->clear();
}


var_ref memory::get_local_var(std::size_t index) const {
    if(debug && frame_stack->empty())
        debug_throw("get_local_var: frame_stack empty!");

    if(index >= capture_index_start) {
        gcvector<var_ref> &captures = frame_stack->back().func->captures;
        std::size_t capture_index = index - capture_index_start;

        if(debug && capture_index >= captures.size()) {
           debug_throw("capture_index >= captures.size(): " + std::to_string(capture_index)
                   + " >= " + std::to_string(captures.size()));
        }

        return captures[capture_index];

    } else if(index <= frame_stack->back().param_count) {
        if(debug && (index == 0 || index > frame_stack->back().params->size())) {
            debug_throw("local_var index > params.size(): " + std::to_string(index)
                    + " > " + std::to_string(frame_stack->back().params->size()));
        }

        return std::get<var_ref>((*frame_stack->back().params)[index - 1].get());

    } else {
        index -= frame_stack->back().param_count;

        if(debug) {
            if(index == 0 || index > frame_stack->back().local_var_count)
                debug_throw("invalid local_var index: " + std::to_string(index)
                        + ", top frame size: " + std::to_string(frame_stack->back().local_var_count));
            if(index > local_var_stack->size())
                debug_throw("index greater than local_var_stack. index: "
                    + std::to_string(index) + ", local_var size: " + std::to_string(local_var_stack->size()));
        }

        return (*local_var_stack)[local_var_stack->size() - index];
    }
}


var_ref memory::get_or_add_global(const std::string &name) {
    if(auto it = globals->find(name); it != globals->end())
        return it->second;
    else
        return globals->try_emplace(name, make_lvalue()).first->second;
}


bool memory::has_global(const std::string &name) const {
    return globals->count(name);
}
