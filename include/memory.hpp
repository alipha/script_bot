#ifndef LIPH_MEMORY_HPP
#define LIPH_MEMORY_HPP

#include "object.hpp"

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>


class memory {
public:
    array_ref make_array() {
        auto array = std::make_shared<std::vector<object>>();
        arrays.push_front(array);
        return array;
    }

    lvalue_ref make_lvalue(object::type value) {
        auto lvalue = std::make_shared<object>(std::move(value));
        lvalues.push_front(lvalue);
        return lvalue;
    }

private:
    std::list<std::weak_ptr<std::vector<object>> arrays;
    std::list<std::weak_ptr<object>> lvalues;

    std::unordered_map<std::string, lvalue_ref> globals;
};

#endif

