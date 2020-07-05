#include "interpreter.hpp"
#include "debug.hpp"
#include "conversion.hpp"
#include "executor.hpp"
#include "gc.hpp"
#include "memory.hpp"
#include "memory_buffer.hpp"
#include "object.hpp"
#include "operation_type.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"
#include "variant_util.hpp"

#include <cstddef>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>


using namespace std::string_literals;



class interpreter_impl {
private:
    static constexpr std::size_t no_parent = ~0;
    static constexpr std::size_t loop_count = 1000;

    struct program_state {
        std::time_t start_time;
        memory_buffer<debug> *buffer;
        std::size_t code_size;
        std::size_t loops;
    };
    
public:
    interpreter_impl(memory *m, std::size_t max_call_depth) 
        : mem(m), max_depth(max_call_depth), last_value(), operands(), parent_operand_count(0) {}
    
    std::string execute(std::shared_ptr<func_def> program);

private:
    void execute_op_code(program_state &state);

    func_ref make_func(std::uint8_t func_index);
    void array_add();
    void param_add();
    void map_add();
    void call_func(program_state &state);
    void execute_binary_op(op_code code);
    void execute_unary_op(op_code code);
    void execute_control_statement(memory_buffer<debug> &buffer, op_code code);
    void execute_short_circuit(memory_buffer<debug> &buffer, op_code code, bool jump_value);
    void execute_coalesce(memory_buffer<debug> &buffer, op_code code);

    memory *mem;
    std::size_t max_depth;
    gc::anchor<object> last_value;
    gc::anchor<std::vector<object>> operands;
    std::size_t parent_operand_count;
};



interpreter::interpreter(memory *m, std::size_t max_call_depth) 
    : impl(std::make_unique<interpreter_impl>(m, max_call_depth)) {}

interpreter::~interpreter() {}

std::string interpreter::execute(std::shared_ptr<func_def> program) {
    return impl->execute(std::move(program));
}


void interpreter_impl::execute_control_statement(memory_buffer<debug> &buffer, op_code code) {
    if(debug && operands->size() <= parent_operand_count) {
        throw std::logic_error("execute_control_statement with zero operands-> op_code: "s
                + lookup_operation(code).symbol);
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(!pop(*operands)->to_bool())
        buffer.seek_abs(jump_pos);
}


void interpreter_impl::execute_short_circuit(memory_buffer<debug> &buffer, op_code code, bool jump_value) {
    if(debug && operands->size() <= parent_operand_count) {
        throw std::logic_error("execute_short_circuit with zero operands-> op_code: "s
                + lookup_operation(code).symbol);
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(operands->back().to_bool() == jump_value)
        buffer.seek_abs(jump_pos);
    else
        operands->pop_back();
}


void interpreter_impl::execute_coalesce(memory_buffer<debug> &buffer, op_code code) {
    if (debug && operands->size() <= parent_operand_count) {
        throw std::logic_error("execute_coalesce with zero operands-> op_code: "s
                + lookup_operation(code).symbol);
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(!std::holds_alternative<std::monostate>(operands->back().value()))
        buffer.seek_abs(jump_pos);
    else
        operands->pop_back();
}


std::string interpreter_impl::execute(std::shared_ptr<func_def> program) {
    std::size_t position = 0;
    func_ref func = gc::make_ptr<func_type>(std::move(program), gcvector<var_ref>());
    last_value = object::type(std::monostate());
   
    parent_operand_count = 0; 
    operands->clear();
    mem->clear_stack();
    mem->push_frame(no_parent, 0, func, make_array());

    program_state state = {
        std::time(nullptr),
        &mem->current_frame().func->definition->code,
        mem->current_frame().code_size,
        loop_count
    };

    while(true) {
        while(state.buffer->position() < state.code_size)
            execute_op_code(state);

        parent_operand_count = mem->current_frame().parent_operand_count;
        if(debug && parent_operand_count > operands->size())
            throw std::logic_error("parent_operand_count " + std::to_string(parent_operand_count)
                    + " > operands->size() " + std::to_string(operands->size()));

        operands->resize(parent_operand_count);
        position = mem->pop_frame();
        if(position == no_parent)
            break;

        //std::cout << "pushing back " << to_std_string(last_value->to_string()) << std::endl;
        operands->push_back(*last_value);
        parent_operand_count = mem->current_frame().parent_operand_count;
        state.buffer = &mem->current_frame().func->definition->code;
        state.code_size = mem->current_frame().code_size;
        state.buffer->seek_abs(position);
    }

    if(debug && !operands->empty()) {
        throw std::logic_error("Expected no operands in stack. size: " + std::to_string(operands->size()));
    }
    
    std::string result = to_std_string(last_value->to_string());
    last_value = object();
    //mem->pop_frame();
    return result;
}


void interpreter_impl::execute_op_code(program_state &state) {
    memory_buffer<debug> &buffer = *state.buffer;

    if(--state.loops == 0) {
        if(std::time(nullptr) - state.start_time > 30)
            throw std::runtime_error("Execution terminated after 30 seconds");
        state.loops = loop_count;
    }

    op_code code = *buffer.read<op_code>();
  
    switch(code) { 
    case op_code::else_start:
    case op_code::while_end:
        if(operands->size() > parent_operand_count)
            operands->pop_back();
        buffer.seek_abs(*buffer.read<std::uint32_t>());
        break;
    /*case op_code::if_end:
        if(!operands->empty())
            operands->pop();
        break;*/
    case op_code::global_var:
        throw std::logic_error("global_var is currently unsupported");
    case op_code::local_var:
        operands->push_back(object(mem->get_local_var(*buffer.read<std::uint8_t>())));
        break;
    case op_code::int_lit:
        operands->push_back(object(*buffer.read<std::int64_t>()));
        break;
    case op_code::uint_lit:
        operands->push_back(object(*buffer.read<std::uint64_t>()));
        break;
    case op_code::float_lit:
        operands->push_back(object(*buffer.read<double>()));
        break;
    case op_code::null_lit:
        operands->push_back(object());
        break;
    case op_code::str_lit:
        operands->push_back(object(make_string(buffer.read_str())));
        break;
    case op_code::func_lit:
        operands->push_back(object(make_func(*buffer.read<std::uint8_t>())));
        break;
    case op_code::func_call:
    case op_code::array_start:
        operands->push_back(object(make_array()));
        break;
    case op_code::map_start:
        operands->push_back(object(make_map()));
        break;
    case op_code::param_add:
        param_add();
        break;
    case op_code::array_add:
    case op_code::array_end:
        array_add();
        break;
    case op_code::map_add:
    case op_code::map_end:
        map_add();
        break;
    case op_code::func_call_end:
        call_func(state);
        break;
    case op_code::if_cond:
    case op_code::while_cond:
        execute_control_statement(buffer, code);
        break;
    case op_code::logic_and:
    case op_code::logic_or:
        execute_short_circuit(buffer, code, code == op_code::logic_or);
        break;
    case op_code::coalesce:
        execute_coalesce(buffer, code);
        break;
    default:
        if(debug && lookup_operation(code).is_nop)
            throw std::logic_error("Unexpected "s + lookup_operation(code).symbol + " in program");

        if(is_binary_op(code)) {
            execute_binary_op(code);
        } else {
            if(executor::unary_op(last_value, *operands, parent_operand_count, code))
                buffer.seek_abs(state.code_size);
        }
    }

    /*
    if(code < op_code::count)
        std::cout << lookup_operation(code).symbol;
    else
        std::cout << std::hex << static_cast<int>(code) << std::dec;
    std::cout << " : " << operands->size() << " " << parent_operand_count << " = ";
    for(auto &op : *operands) {
        if(std::holds_alternative<func_ref>(op.value()))
            std::cout << "func ";
        else
            std::cout << to_std_string(op.to_string()) << " ";
    }
    std::cout << std::endl;
    */
}


func_ref interpreter_impl::make_func(std::uint8_t func_index) {
    std::shared_ptr<func_def> &current_func = mem->current_frame().func->definition;

    if(debug && func_index >= current_func->func_lits.size())
        throw std::logic_error("make_func with func_index " + std::to_string(func_index) 
                + " >= " + std::to_string(current_func->func_lits.size()));

    std::shared_ptr<func_def> &new_func = current_func->func_lits[func_index];
    gcvector<std::uint8_t> &bytecode = new_func->code.buffer();

    gcvector<var_ref> captures(bytecode[2]);
    std::size_t capture_index = bytecode.size() - captures.size();

    for(var_ref &capture : captures)
        capture = mem->get_local_var(bytecode[capture_index++]);

    return gc::make_ptr<func_type>(new_func, std::move(captures));
}


void interpreter_impl::array_add() {
    if(debug && operands->size() < parent_operand_count + 2)
        throw std::logic_error("execute array_add with " + std::to_string(operands->size() - parent_operand_count) + " operands");

    object &array = *(operands->end() - 2);
    std::get<array_ref>(array.value())->push_back(operands->back());
    operands->pop_back();
}


void interpreter_impl::param_add() {
    if(debug && operands->size() < parent_operand_count + 2)
        throw std::logic_error("execute param_add with " + std::to_string(operands->size() - parent_operand_count) + " operands");

    object &array = *(operands->end() - 2);
    object::type param = to_variant<object::type>(operands->back().value());

    gc::anchor_ptr<object> param_lvalue = make_lvalue(std::move(param));
    //gc::anchor_ptr<object> param_anchor = param_lvalue;

    std::get<array_ref>(array.value())->push_back(object(param_lvalue));
    operands->pop_back();
}


void interpreter_impl::map_add() {
    if(debug && operands->size() < parent_operand_count + 3)
        throw std::logic_error("execute map_add with " + std::to_string(operands->size() - parent_operand_count) + " operands");

    object &value = operands->back();
    object &key = *(operands->end() - 2);
    object &map = *(operands->end() - 3);
    std::get<map_ref>(map.value())->try_emplace(key.to_string(), value);
    operands->pop_back();
    operands->pop_back();
}

    
void interpreter_impl::call_func(program_state &state) {
    if(debug && operands->size() < parent_operand_count + 2)
        throw std::logic_error("call_func with " + std::to_string(operands->size() - parent_operand_count) + " operands");

    if(mem->call_depth() > max_depth)
        throw std::runtime_error("stack overflow: max call depth of " + std::to_string(max_depth));

    array_ref &params = std::get<array_ref>(operands->back().get());
    object::value_type func_obj = (operands->end() - 2)->value();
    func_ref *func = std::get_if<func_ref>(&func_obj);
    if(!func)
        throw std::runtime_error("left of () is not a function");

    mem->push_frame(state.buffer->position(), operands->size() - 2, std::move(*func), std::move(params));
    state.buffer = &mem->current_frame().func->definition->code;
    state.code_size = mem->current_frame().code_size;
    operands->pop_back();
    operands->pop_back();
    parent_operand_count = operands->size();
}


void interpreter_impl::execute_binary_op(op_code code) {
    if(debug && operands->size() < parent_operand_count + 2) {
        throw std::logic_error("execute_binary_op with " + std::to_string(operands->size() - parent_operand_count) 
                + " operands. op_code: " + lookup_operation(code).symbol);
    }

    bool is_assign = is_binary_assignment(code);
    // TODO: only use anchors when the op is something that allocates memory?
    gc::anchor<object> right = pop(*operands);
    gc::anchor<object> left = is_assign ? gc::anchor<object>(operands->back()) : pop(*operands);
    object result;

    if(is_assign && code != op_code::assign) {
        code = static_cast<op_code>(static_cast<int>(code) - assign_ops_offset);
    }

    if(code == op_code::assign)
        result = *right;
    else if(code == op_code::index)
        result = executor::index_op(mem, *left, *right);
    else if(is_binary_comp(code))
        result = object::type(static_cast<std::int64_t>(executor::binary_comp(code, *left, *right)));
    else if(is_binary_int_op(code))
        result = executor::binary_int_op(code, *left, *right);
    else if(is_binary_arithmetic(code))
        result = executor::binary_arithmetic(code, *left, *right);
    else
        throw std::logic_error("currently unspported binary op: "s + lookup_operation(code).symbol);

    if(is_assign) {
        if(var_ref *var = std::get_if<var_ref>(&left->get())) {
            **var = to_variant<object::type>(result.value());
        } else if(lvalue_ref *lvalue = std::get_if<lvalue_ref>(&left->get())) {
            **lvalue = to_variant<object::type>(result.value()); 
        } else {
            throw std::runtime_error("left of "s + lookup_operation(code).symbol + " is not assignable");
        }
    } else {
        operands->push_back(std::move(result));
    }
}



