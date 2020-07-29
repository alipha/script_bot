#include "writer.hpp"
#include "debug.hpp"
#include "object.hpp"
#include "serializer.hpp"

#include <cstdint>
#include <cstddef>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <utility>

    
void writer::open(const std::string &filename) {
    os.open(filename);
    if(!os)
        throw std::runtime_error("writer: unable to open file " + filename);
}


void writer::close() {
    needs_saving = {};
    objects_by_addr = {};
    os.close();
}

    
std::pair<std::uintptr_t, object&> writer::next_to_save() {
    std::uintptr_t addr = needs_saving.front();
    needs_saving.pop_front();

    if(debug && objects_by_addr.find(addr) == objects_by_addr.end())
        throw std::logic_error("addr is not in objects_by_addr");

   return {addr, objects_by_addr[addr]}; 
}


void writer::write(const string_ref &str) {
    write(*str);
}


void writer::write(const array_ref &array) {
    write(static_cast<std::uint32_t>(array->size()));
    for(object &obj : *array)
        write_ref(obj);
}


void writer::write(const map_ref &map) {
    write(static_cast<std::uint32_t>(map->size()));
    for(auto &[name, obj] : *map) {
        write(name);
        write_ref(obj);
    }
}


void writer::write(const func_ref &func) {
    (void)os;
    (void)func;
    /*gcvector<std::uint8_t> &code = func->code.buffer();
    write(os, static_cast<std::uint32_t>(code.size()));
    if(!os.write(reinterpret_cast<char*>(code.data()), code.size()))
        throw std::runtime_error("error writing function to file");
*/    // TODO: func_defs are shared
}


void writer::write(const std::monostate &) {}


void writer::write(const object &obj) {
    write(static_cast<std::uint8_t>(get_type(obj)));
    std::visit([this](const auto &value) { write(value); }, obj.value());
}


void writer::write_ref(const object &obj) {
    write(static_cast<std::uint8_t>(get_type(obj)));
    std::visit([this](const auto &value) { write_ref(value); }, obj.value());
}
    

object_type writer::get_type(const object &obj) {
    return std::visit([](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr(std::is_same_v<T, std::monostate>) {
            return object_type::nul;
        } else if constexpr(std::is_same_v<T, std::int64_t>) {
            return object_type::int64;
        } else if constexpr(std::is_same_v<T, std::uint64_t>) {
            return object_type::uint64;
        } else if constexpr(std::is_same_v<T, double>) {
            return object_type::float8;
        } else if constexpr(std::is_same_v<T, string_ref>) {
            return object_type::string;
        } else if constexpr(std::is_same_v<T, array_ref>) {
            return object_type::array;
        } else if constexpr(std::is_same_v<T, map_ref>) {
            return object_type::map;
        } else if constexpr(std::is_same_v<T, func_ref>) {
            return object_type::func;
        } else {
            static_assert(sizeof(T) && false, "missing variant type");
        }
    }, obj.value());
}
    
