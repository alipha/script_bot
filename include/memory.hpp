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


constexpr std::uint8_t capture_index_start = 128;


struct frame {
    std::size_t parent_pos;
    std::size_t parent_operand_count;
    std::size_t local_var_count;
    std::size_t temps_start;
    func_ref func;
    std::size_t code_size;
    array_ref params;
    std::size_t param_count;
    
    void transverse(gc::action &act) {
        act(func);
        act(params);
    }
};


class memory {
public:
    void push_frame(std::size_t current_pos, std::size_t current_operand_count, func_ref func, array_ref params);
    std::size_t pop_frame();
    void clear_stack();
   
    frame &current_frame() { return frame_stack->back(); }

    var_ref get_local_var(std::size_t index);

    void push_temp(object temp) { temps_stack->push_back(std::move(temp)); }

private:
    gc::anchor<std::unordered_map<std::string, var_ref>> globals;
    gc::anchor<std::vector<object>> temps_stack;
    gc::anchor<std::vector<var_ref>> local_var_stack;
    gc::anchor<std::vector<frame>> frame_stack;
};

#endif

