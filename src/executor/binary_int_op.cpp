#include "executor.hpp"
#include "object.hpp"
#include "operation_type.hpp"
#include "string_util.hpp"

#include <stdexcept>
#include <string>
#include <variant>


using namespace std::string_literals;


namespace executor {


object binary_int_op(op_code code, const object &left, const object &right) {
    
    return std::visit([code](auto &&l, auto &&r) -> object::type {
        using LeftT = std::decay_t<decltype(l)>;
        using RightT = std::decay_t<decltype(r)>;

        switch(code) {
        case op_code::mod: {
            if(r == 0)
                throw std::runtime_error("Division by zero");
            
            if constexpr(std::is_same_v<LeftT, std::int64_t> && std::is_same_v<RightT, std::int64_t>) {
                if(l == std::numeric_limits<std::int64_t>::min() && r == -1)
                    throw std::runtime_error("Overflow computing: " 
                            + std::to_string(std::numeric_limits<std::int64_t>::min()) + " % -1");
            }
            return l % r;
        }
        case op_code::bit_and: return l & r;
        case op_code::bit_xor: return l ^ r;
        case op_code::bit_or: return l | r;
        case op_code::shl: return l << r;
        case op_code::shr: return l >> r;
        default:
            throw std::logic_error("Invalid binary_int_op: "s + lookup_operation(code).symbol);
        }
    }, left.to_int(), right.to_int());
}
    

} // namespace executor

