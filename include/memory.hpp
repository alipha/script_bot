#ifndef LIPH_MEMORY_HPP
#define LIPH_MEMORY_HPP

#include "gc.hpp"
#include "object.hpp"

#include <cstddef>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>


struct frame {
    std::size_t parent_pos;
    std::size_t local_var_count;
    std::size_t temps_start;
    std::shared_ptr<func_def> func;
    std::size_t code_size;
    gcvector<object> params;
    std::size_t param_count;
};


class memory {
public:
    void push_frame(std::size_t current_pos, std::shared_ptr<func_def> func, gcvector<object> params);
    std::size_t pop_frame();
    void clear_stack();
   
    frame &current_frame() { return frame_stack.back(); }

    var_ref get_local_var(std::size_t index);

    void push_temp(object temp) { temps_stack->push_back(std::move(temp)); }

private:
    gc::anchor<std::unordered_map<std::string, var_ref>> globals;
    gc::anchor<std::vector<object>> temps_stack;
    gc::anchor<std::vector<var_ref>> local_var_stack;
    std::vector<frame> frame_stack;
};

#endif

