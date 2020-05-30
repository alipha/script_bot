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

#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>


using namespace std::string_literals;



class interpreter_impl {
public:
    interpreter_impl(memory *m) : mem(m) {}
    
    std::string execute(const std::vector<char> &program);

private:
    func_ref make_func(std::string_view bytecode); 
    void array_add(std::vector<object> &operands);
    void map_add(std::vector<object> &operands);
    void execute_binary_op(std::vector<object> &operands, op_code code);
    void execute_unary_op(std::vector<object> &operands, op_code code);
    void execute_control_statement(memory_buffer<debug> &buffer, std::vector<object> &operands, op_code code);
    void execute_short_circuit(memory_buffer<debug> &buffer, std::vector<object> &operands, op_code code, bool jump_value);
    void execute_coalesce(memory_buffer<debug> &buffer, std::vector<object> &operands, op_code code);

    memory *mem;
    gc::anchor<object> last_value;
};



interpreter::interpreter(memory *m) : impl(std::make_unique<interpreter_impl>(m)) {}

interpreter::~interpreter() {}

std::string interpreter::execute(const std::vector<char> &program) {
    return impl->execute(program);
}


void interpreter_impl::execute_control_statement(memory_buffer<debug> &buffer, std::vector<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_control_statement with zero operands. op_code: "s
                    + lookup_operation(code).symbol);
        }
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(!pop(operands)->to_bool())
        buffer.seek_abs(jump_pos);
}


void interpreter_impl::execute_short_circuit(memory_buffer<debug> &buffer, std::vector<object> &operands, op_code code, bool jump_value) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_short_circuit with zero operands. op_code: "s
                    + lookup_operation(code).symbol);
        }
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(operands.back().to_bool() == jump_value)
        buffer.seek_abs(jump_pos);
    else
        operands.pop_back();
}


void interpreter_impl::execute_coalesce(memory_buffer<debug> &buffer, std::vector<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_coalesce with zero operands. op_code: "s
                    + lookup_operation(code).symbol);
        }
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(!std::holds_alternative<std::monostate>(operands.back().value()))
        buffer.seek_abs(jump_pos);
    else
        operands.pop_back();
}


std::string interpreter_impl::execute(const std::vector<char> &program) {
    std::time_t start = std::time(nullptr);
    memory_buffer<debug> buffer(program);
    gc::anchor<std::vector<object>> operands;
    last_value = object::type(std::monostate());

    mem->clear_stack();
    // TODO: params
    std::uint8_t param_count = *buffer.read<std::uint8_t>();
    (void)param_count;
    mem->push_frame(*buffer.read<std::uint8_t>() - param_count);
    
    std::uint8_t capture_count = *buffer.read<std::uint8_t>();
    std::size_t code_size = buffer.size() - capture_count;
    int loops = 1000;

    while(buffer.position() < code_size) {
        if(--loops == 0) {
            if(std::time(nullptr) - start > 30)
                throw std::runtime_error("Execution terminated after 30 seconds");
            loops = 1000;
        }

        op_code code = *buffer.read<op_code>();
      
        switch(code) { 
        case op_code::else_start:
        case op_code::while_end:
            if(!operands->empty())
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
            operands->push_back(object(make_func(buffer.read_str())));
            break;
        case op_code::func_call:
        case op_code::array_start:
            operands->push_back(object(make_array()));
            break;
        case op_code::map_start:
            operands->push_back(object(make_map()));
            break;
        case op_code::param_add:
        case op_code::array_add:
        case op_code::array_end:
            array_add(*operands);
            break;
        case op_code::map_add:
        case op_code::map_end:
            map_add(*operands);
            break;
        case op_code::func_call_end:
            operands->push_back(object(make_array()));
            break;
        case op_code::if_cond:
        case op_code::while_cond:
            execute_control_statement(buffer, *operands, code);
            break;
        case op_code::logic_and:
        case op_code::logic_or:
            execute_short_circuit(buffer, *operands, code, code == op_code::logic_or);
            break;
        case op_code::coalesce:
            execute_coalesce(buffer, *operands, code);
            break;
        default:
            if constexpr(debug) {
                if(lookup_operation(code).is_nop)
                    throw std::logic_error("Unexpected "s + lookup_operation(code).symbol + " in program");
            }

            if(is_binary_op(code)) {
                execute_binary_op(*operands, code);
            } else {
                executor::unary_op(last_value, *operands, code);
            }
        }
    }

    if constexpr(debug) {
        if(!operands->empty()) {
            throw std::logic_error("Expected no operands in stack. size: " + std::to_string(operands->size()));
        }
    }
    
    std::string result = to_std_string(last_value->to_string());
    last_value = object();
    mem->pop_frame();
    return result;
}

    
func_ref interpreter_impl::make_func(std::string_view bytecode) {
    if constexpr(debug) {
        if(bytecode.size() < 3)
            throw std::logic_error("make_func with bytecode length " 
                    + std::to_string(bytecode.size()) + " < 3");
        std::size_t min_size = static_cast<std::uint8_t>(bytecode[2]) + 3;
        if(bytecode.size() < min_size)
            throw std::logic_error("make_func: no room for captures with bytecode length "
                    + std::to_string(bytecode.size()) + " < " + std::to_string(min_size));
    }

    gcvector<var_ref> captures(bytecode[2]);
    std::size_t capture_index = bytecode.size() - captures.size();

    for(var_ref &capture : captures)
        capture = mem->get_local_var(bytecode[capture_index++]);

    // TODO: remove the captures from the bytecode
    return gc::make_ptr<func_type>(bytecode.begin(), bytecode.end(), std::move(captures));
}


void interpreter_impl::array_add(std::vector<object> &operands) {
    if constexpr(debug) {
        if(operands.size() < 2)
            throw std::logic_error("execute array_add with " + std::to_string(operands.size()) + " operands");
    }

    object &array = *(operands.end() - 2);
    std::get<array_ref>(array.value())->push_back(operands.back());
    operands.pop_back();
}


void interpreter_impl::map_add(std::vector<object> &operands) {
    if constexpr(debug) {
        if(operands.size() < 3)
            throw std::logic_error("execute map_add with " + std::to_string(operands.size()) + " operands");
    }

    object &value = operands.back();
    object &key = *(operands.end() - 2);
    object &map = *(operands.end() - 3);
    std::get<map_ref>(map.value())->try_emplace(key.to_string(), value);
    operands.pop_back();
    operands.pop_back();
}


void interpreter_impl::execute_binary_op(std::vector<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.size() < 2) {
            throw std::logic_error("execute_binary_op with " + std::to_string(operands.size()) 
                    + " operands. op_code: " + lookup_operation(code).symbol);
        }
    }

    bool is_assign = is_binary_assignment(code);
    // TODO: only use anchors when the op is something that allocates memory?
    gc::anchor<object> right = pop(operands);
    gc::anchor<object> left = is_assign ? gc::anchor<object>(operands.back()) : pop(operands);
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
        operands.push_back(std::move(result));
    }
}



