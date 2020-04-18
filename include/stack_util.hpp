#ifndef LIPH_STACK_UTIL_HPP
#define LIPH_STACK_UTIL_HPP

#include "debug.hpp"
#include <stack>


template<typename T>
T pop(std::stack<T> &s) {
    if(debug && s.empty())
        debug_throw("calling pop() on empty stack!");

    T value = s.top();
    s.pop();
    return value;
}


#endif
