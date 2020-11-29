#ifndef LIPH_OPERAND_HPP
#define LIPH_OPERAND_HPP

#include "debug.hpp"
#include "gc.hpp"
#include "object.hpp"
#include <utility>
#include <variant>


struct operand {
    operand() {}
    operand(object v) : value(std::move(v)) {}
    operand(object c, object v): this_obj(std::move(c)), value(std::move(v)) {}

    void transverse(gc::action &act) { 
        act(this_obj.get());
        act(value.get());
    }

    object this_obj = object::type(std::monostate());
    object value = object::type(std::monostate());
};


inline gc::anchor<object> pop(std::vector<operand> &v) {
    if(debug && v.empty())
        debug_throw("calling pop() on empty vector<operand>!");

    gc::anchor<object> value = std::move(v.back().value);
    v.pop_back();
    return value;
}


#endif
