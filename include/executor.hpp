#ifndef LIPH_EXECUTOR_HPP
#define LIPH_EXECUTOR_HPP

#include "object.hpp"
#include "operation_type.hpp"

#include <stack>


class memory;


namespace executor {

bool binary_comp(op_code code, const object &left, const object &right);
object binary_int_op(op_code code, const object &left, const object &right);
object binary_arithmetic(op_code code, const object &left, const object &right);

object index_op(memory *mem, const object &left, const object &right);

void unary_op(std::stack<object> &operands, op_code code);

}

#endif
