#include "executor.hpp"
#include "conversion.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "operation_type.hpp"

#include <string>
#include <stdexcept>
#include <type_traits>
#include <variant>


using namespace std::string_literals;


namespace executor {


object index_op(memory *mem, const object &left, const object &right) {
    bool temp = !std::holds_alternative<lvalue_ref>(left.get()) 
        && !std::holds_alternative<var_ref>(left.get()); 
    if(temp)
        mem->push_temp(left);

    return object::type(std::visit([](auto &&l, auto &&r) -> lvalue_ref {
        using L = std::decay_t<decltype(l)>;
        using R = std::decay_t<decltype(r)>;

        if constexpr(std::is_same_v<L, array_ref>) {
            std::int64_t index = to_int(r);
            if(index < 0 || static_cast<std::size_t>(index) >= l->size())
                throw std::runtime_error("array index out of bounds: " + std::to_string(index) 
                        + ", size: " + std::to_string(l->size()));
            return &(*l)[index];
        } else if constexpr(std::is_same_v<L, map_ref>) {
            if constexpr(std::is_integral_v<R> || std::is_same_v<R, string_ref>) {
                return &l->try_emplace(to_str(r), std::monostate()).first->second;
            } else {
                throw std::runtime_error("map index with non-string/non-int type");
            }
        //} else if constexpr(std::is_same_v<L, string_ref>) {  // TODO: string indexing
        } else {
            throw std::runtime_error("object does not support []");
        }
    }, left.non_null_value(), right.non_null_value()));
}


}  // namespace executor
