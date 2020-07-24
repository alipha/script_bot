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
    if(debug && obj.get().index() >= static_cast<std::size_t>(object_type::count))
        debug_throw("write: unexpected variant value: " + std::to_string(obj.get().index()));
    write(obj.get().index());
    std::visit([this](const auto &value) { write(value); }, obj.get());
}


void writer::write_ref(const object &obj) {
    if(debug && obj.get().index() >= static_cast<std::size_t>(object_type::count))
        debug_throw("write_ref: unexpected variant value: " + std::to_string(obj.get().index()));
    write(obj.get().index());
    std::visit([this](const auto &value) { write_ref(value); }, obj.get());
}
    
