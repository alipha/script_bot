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
    void array_add(std::stack<object> &operands);
    void map_add(std::stack<object> &operands);
    void execute_binary_op(std::stack<object> &operands, op_code code);
    void execute_unary_op(std::stack<object> &operands, op_code code);
    void execute_control_statement(memory_buffer<debug> &buffer, std::stack<object> &operands, op_code code);
    void execute_short_circuit(memory_buffer<debug> &buffer, std::stack<object> &operands, op_code code, bool jump_value);
    void execute_coalesce(memory_buffer<debug> &buffer, std::stack<object> &operands, op_code code);

    memory *mem;
    object last_value;
};



interpreter::interpreter(memory *m) : impl(std::make_unique<interpreter_impl>(m)) {}

interpreter::~interpreter() {}

std::string interpreter::execute(const std::vector<char> &program) {
    return impl->execute(program);
}


void interpreter_impl::execute_control_statement(memory_buffer<debug> &buffer, std::stack<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_control_statement with zero operands. op_code: "s
                    + lookup_operation(code).symbol);
        }
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(!pop(operands).to_bool())
        buffer.seek_abs(jump_pos);
}


void interpreter_impl::execute_short_circuit(memory_buffer<debug> &buffer, std::stack<object> &operands, op_code code, bool jump_value) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_short_circuit with zero operands. op_code: "s
                    + lookup_operation(code).symbol);
        }
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(operands.top().to_bool() == jump_value)
        buffer.seek_abs(jump_pos);
    else
        operands.pop();
}


void interpreter_impl::execute_coalesce(memory_buffer<debug> &buffer, std::stack<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.empty()) {
            throw std::logic_error("execute_coalesce with zero operands. op_code: "s
                    + lookup_operation(code).symbol);
        }
    }

    std::uint32_t jump_pos = *buffer.read<std::uint32_t>();

    if(!std::holds_alternative<std::monostate>(operands.top().value()))
        buffer.seek_abs(jump_pos);
    else
        operands.pop();
}


std::string interpreter_impl::execute(const std::vector<char> &program) {
    std::time_t start = std::time(nullptr);
    memory_buffer<debug> buffer(program);
    std::stack<object> operands;
    last_value = object::type(std::monostate());

    mem->push_frame(*buffer.read<std::uint8_t>());
    int loops = 1000;

    while(buffer.position() < buffer.size()) {
        if(--loops == 0) {
            if(std::time(nullptr) - start > 30)
                throw std::runtime_error("Execution terminated after 30 seconds");
            loops = 1000;
        }

        op_code code = *buffer.read<op_code>();
      
        switch(code) { 
        case op_code::else_start:
        case op_code::while_end:
            if(!operands.empty())
                operands.pop();
            buffer.seek_abs(*buffer.read<std::uint32_t>());
            break;
        /*case op_code::if_end:
            if(!operands.empty())
                operands.pop();
            break;*/
        case op_code::global_var:
            throw std::logic_error("global_var is currently unsupported");
        case op_code::local_var:
            operands.push(object(mem->get_local_var(*buffer.read<std::uint8_t>())));
            break;
        case op_code::int_lit:
            operands.push(object(*buffer.read<std::int64_t>()));
            break;
        case op_code::uint_lit:
            operands.push(object(*buffer.read<std::uint64_t>()));
            break;
        case op_code::float_lit:
            operands.push(object(*buffer.read<double>()));
            break;
        case op_code::null_lit:
            operands.push(object());
            break;
        case op_code::str_lit:
            operands.push(object(make_string(buffer.read_str())));
            break;
        case op_code::array_start:
            operands.push(object(make_array()));
            break;
        case op_code::map_start:
            operands.push(object(make_map()));
            break;
        case op_code::array_add:
        case op_code::array_end:
            array_add(operands);
            break;
        case op_code::map_add:
        case op_code::map_end:
            map_add(operands);
            break;
        case op_code::if_cond:
        case op_code::while_cond:
            execute_control_statement(buffer, operands, code);
            break;
        case op_code::logic_and:
        case op_code::logic_or:
            execute_short_circuit(buffer, operands, code, code == op_code::logic_or);
            break;
        case op_code::coalesce:
            execute_coalesce(buffer, operands, code);
            break;
        default:
            if constexpr(debug) {
                if(lookup_operation(code).is_nop)
                    throw std::logic_error("Unexpected "s + lookup_operation(code).symbol + " in program");
            }

            if(is_binary_op(code)) {
                execute_binary_op(operands, code);
            } else {
                executor::unary_op(last_value, operands, code);
            }
        }
    }

    if constexpr(debug) {
        if(!operands.empty()) {
            throw std::logic_error("Expected no operands in stack. size: " + std::to_string(operands.size()));
        }
    }
    
    std::string result = to_std_string(last_value.to_string());
    mem->pop_frame();
    return result;
}


void interpreter_impl::array_add(std::stack<object> &operands) {
    if constexpr(debug) {
        if(operands.size() < 2)
            throw std::logic_error("execute array_add with " + std::to_string(operands.size()) + " operands");
    }

    object element = pop(operands);
    std::get<array_ref>(operands.top().value())->push_back(element);
}


void interpreter_impl::map_add(std::stack<object> &operands) {
    if constexpr(debug) {
        if(operands.size() < 3)
            throw std::logic_error("execute map_add with " + std::to_string(operands.size()) + " operands");
    }

    object value = pop(operands);
    object key = pop(operands);
    std::get<map_ref>(operands.top().value())->try_emplace(key.to_string(), value);
}


void interpreter_impl::execute_binary_op(std::stack<object> &operands, op_code code) {
    if constexpr(debug) {
        if(operands.size() < 2) {
            throw std::logic_error("execute_binary_op with " + std::to_string(operands.size()) 
                    + " operands. op_code: " + lookup_operation(code).symbol);
        }
    }

    bool is_assign = is_binary_assignment(code);
    object right = pop(operands);
    object left = is_assign ? operands.top() : pop(operands);
    object result;

    if(is_assign && code != op_code::assign) {
        code = static_cast<op_code>(static_cast<int>(code) - assign_ops_offset);
    }

    if(code == op_code::assign)
        result = right;
    else if(code == op_code::index)
        result = executor::index_op(mem, left, right);
    else if(is_binary_comp(code))
        result = static_cast<std::int64_t>(executor::binary_comp(code, left, right));
    else if(is_binary_int_op(code))
        result = executor::binary_int_op(code, left, right);
    else if(is_binary_arithmetic(code))
        result = executor::binary_arithmetic(code, left, right);
    else
        throw std::logic_error("currently unspported binary op: "s + lookup_operation(code).symbol);

    if(is_assign) {
        if(var_ref *var = std::get_if<var_ref>(&left.get())) {
            **var = to_variant<object::type>(result.value());
        } else if(lvalue_ref *lvalue = std::get_if<lvalue_ref>(&left.get())) {
            **lvalue = to_variant<object::type>(result.value()); 
        } else {
            throw std::runtime_error("left of "s + lookup_operation(code).symbol + " is not assignable");
        }
    } else {
        operands.push(result);
    }
}



