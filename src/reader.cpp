#include "reader.hpp"
#include "debug.hpp"
#include "object.hpp"
#include "serializer.hpp"

#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

    
void reader::open(const std::string &filename) {
    is.open(filename);
    if(!is)
        throw std::runtime_error("unable to open file " + filename);
}
    

void reader::close() {
    needs_loading = {};
    objects_by_addr = {};
    is.close();
}


object reader::get_object(std::uintptr_t addr) {
    if(debug && objects_by_addr.find(addr) == objects_by_addr.end())
        throw std::logic_error("objects_by_addr doesn't contain addr");
    return objects_by_addr[addr];
}


void reader::add_object_ref(std::uintptr_t addr, object &&obj) {
    if(debug && objects_by_addr.find(addr) != objects_by_addr.end())
        throw std::logic_error("objects_by_addr already contains addr");
    objects_by_addr.try_emplace(addr, std::move(obj));
}
    

std::pair<std::uintptr_t, lvalue_ref> reader::next_to_load() {
    auto addr_ref = *needs_loading.begin();
    needs_loading.erase(needs_loading.begin());
    return addr_ref;
}


object_type reader::read_type() {
    std::uint8_t type_value = read_or_throw<std::uint8_t>();
    if(type_value >= static_cast<std::uint8_t>(object_type::count))
        throw std::runtime_error("invalid type: " + std::to_string(type_value));
    return static_cast<object_type>(type_value);
}


void reader::read_ref_to(lvalue_ref dest) {
    object_type type = read_type();

    switch(type) {
    case object_type::nul:    *dest = object(std::monostate()); break;
    case object_type::int64:  *dest = object(read_or_throw<std::int64_t>()); break;
    case object_type::uint64: *dest = object(read_or_throw<std::uint64_t>()); break;
    case object_type::float8: *dest = object(read_or_throw<double>()); break;
    case object_type::func:
        /* todo */
    default: {
            std::uintptr_t addr = read_or_throw<std::uintptr_t>();
            needs_loading.emplace(addr, dest);
        }
    }
}


object reader::read_object() {
    object_type type = read_type();
    std::uint32_t size;

    switch(type) {
    case object_type::string: return object::type(make_string(read_str<gcstring>()));
    case object_type::array: {
        size = read_or_throw<std::uint32_t>();

        array_ref array = make_array();
        array->resize(size);

        for(object &obj : *array)
            read_ref_to(&obj);
            
        return object::type(array);
    }
    case object_type::map: {
        size = read_or_throw<std::uint32_t>();

        map_ref map = make_map();
        for(std::size_t i = 0; i < size; ++i) {
            gcstring name = read_str<gcstring>();
            auto it = map->try_emplace(name, std::monostate()).first;
            read_ref_to(&it->second);
        }

        return object::type(map);
    }
    case object_type::func: {
        /* todo */
    }
    default:
        throw std::runtime_error("type " 
            + std::to_string(static_cast<std::uint8_t>(type)) + " should not be here.");
    }
}



