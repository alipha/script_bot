#ifndef LIPH_INTERPRETER_HPP
#define LIPH_INTERPRETER_HPP

#include "object.hpp"
#include "operation_type.hpp"

#include <cstdint>
#include <stack>
#include <string>
#include <variant>
#include <vector>


class memory;


class interpreter {
public:
    interpreter(memory *m) : mem(m) {}
    
    std::string execute(const std::vector<char> &program);

private:
    void execute_binary_op(std::stack<object> &operands, op_code code);
    void execute_unary_op(std::stack<object> &operands, op_code code);

    memory *mem;
};


#endif

