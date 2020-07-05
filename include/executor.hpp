#ifndef LIPH_EXECUTOR_HPP
#define LIPH_EXECUTOR_HPP

#include "gc.hpp"
#include "object.hpp"
#include "operation_type.hpp"

#include <vector>


class memory;


namespace executor {

bool binary_comp(op_code code, const object &left, const object &right);
object binary_int_op(op_code code, const object &left, const object &right);
object binary_arithmetic(op_code code, const object &left, const object &right);

object index_op(memory *mem, const object &left, const object &right);

void unary_op(gc::anchor<object> &last_value, std::vector<object> &operands, std::size_t parent_operand_count, op_code code);

}

#endif
