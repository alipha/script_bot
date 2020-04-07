#include "interpreter.hpp"
#include "memory_buffer.hpp"
#include "operation_type.hpp"
#include "util.hpp"

#include <stdexcept>
#include <string>
#include <vector>


void execute_binary_op(std::stack<int> &operands, op_code code);
void execute_unary_op(std::stack<int> &operands, op_code code);

// pow
// mod
// shl
// shr
//
template<typename T>
struct arithmetic_ops {
    static T mul(T a, T b) { return a * b; }
    static T div(T a, T b) { return a / b; }
    static T add(T a, T b) { return a + b; }
    static T sub(T a, T b) { return a - b; }
    static int  lt(T a, T b) { return a < b; }
    static int lte(T a, T b) { return a <= b; }
    static int  gt(T a, T b) { return a > b; }
    static int gte(T a, T b) { return a >= b; }
    static int  eq(T a, T b) { return a == b; }
    static int neq(T a, T b) { return a != b; }
    static T negate(T a) { return -a; }
};


int execute(const std::vector<char> &program) {
    memory_buffer<true> buffer(program);    // TODO: false?
    std::stack<int> operands;

    while(buffer.position() < buffer.size()) {
        op_code code = *buffer.read<op_code>();
       
        if(code == op_code::left_paren || code == op_code::right_paren) { 
            /* do nothing */
        } else if(code == op_code::var) {
            throw std::logic_error("var is currently unsupported");
        } else if(code == op_code::int_lit) {
            operands.push(*buffer.read<int>());
        } else if(code == op_code::float_lit) {
            throw std::logic_error("float_lit is currently unsupported");
        } else if(code == op_code::str_lit) {
            throw std::logic_error("str_lit is currently unsupported");
        } else if(is_binary_op(code)) {
            execute_binary_op(operands, code);
        } else {
            execute_unary_op(operands, code);
        }
    }

    if(operands.size() != 1) {
        throw std::logic_error("Expected only 1 ooperand in stack. size: " + std::to_string(operands.size()));
    }

    return operands.top();
}


void execute_binary_op(std::stack<int> &operands, op_code code) {
    if(operands.size() < 2) {  // TODO: remove?
        throw std::logic_error("execute_binary_op with " + std::to_string(operands.size()) + " operands. op_code: "
                + std::to_string(static_cast<int>(code)));
    }

    int right = pop(operands);
    int left = pop(operands);
    using aops = arithmetic_ops<int>;

    switch(code) {
    case op_code::mul: operands.push(aops::mul(left, right)); break;
    case op_code::div: operands.push(aops::div(left, right)); break;
    case op_code::mod: operands.push(left % right); break;
    case op_code::add: operands.push(aops::add(left, right)); break;
    case op_code::sub: operands.push(aops::sub(left, right)); break;
    case op_code::shl: operands.push(left << right); break;
    case op_code::shr: operands.push(left >> right); break;
    case op_code::lt:  operands.push(aops::lt(left, right)); break;
    case op_code::lte: operands.push(aops::lte(left, right)); break;
    case op_code::gt:  operands.push(aops::gt(left, right)); break;
    case op_code::gte: operands.push(aops::gte(left, right)); break;
    case op_code::eq:  operands.push(aops::eq(left, right)); break;
    case op_code::neq: operands.push(aops::neq(left, right)); break;
    case op_code::bit_and: operands.push(left & right); break;
    case op_code::bit_xor: operands.push(left ^ right); break;
    case op_code::bit_or:  operands.push(left | right); break;
    case op_code::logic_and: operands.push(left && right); break;
    case op_code::logic_or:  operands.push(left || right); break;
    default:
        throw std::logic_error("Currently unspported binary op: " + std::to_string(static_cast<int>(code)));
    }
}


void execute_unary_op(std::stack<int> &operands, op_code code) {
    if(operands.empty()) {  // TODO: remove?
        throw std::logic_error("execute_unary_op with zero operands. op_code: "
                + std::to_string(static_cast<int>(code)));
    }

    int operand = pop(operands);

    switch(code) {
    case op_code::negate: operands.push(-operand); break;
    case op_code::bit_not: operands.push(~operand); break;
    case op_code::logic_not: operands.push(!operand); break;
    default:
        throw std::logic_error("Currently unsupported unary op: " + std::to_string(static_cast<int>(code)));
    }
}

