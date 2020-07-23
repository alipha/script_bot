#include "reader.hpp"
#include "object.hpp"
#include "serializer.hpp"

#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

    
void writer::open(const std::string &filename) {
    is.open(filename);
    if(!is)
        throw std::runtime_error("unable to open file " + filename);
}


void writer::write(const string_ref &str) {
    write(os, *str);
}


void writer::write(const array_ref &array) {
    write(os, static_cast<std::uint32_t>(array->size()));
    for(object &obj : *array)
        write_ref(os, obj);
}


void writer::write(const map_ref &map) {
    write(os, static_cast<std::uint32_t>(map->size()));
    for(auto &[name, obj] : *map) {
        write(os, name);
        write_ref(os, obj);
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


void writer::write(std::ostream &, const std::monostate &) {}


void writer::write(const object &obj) {
    if(debug && obj.get().index() >= static_cast<std::size_t>(object_type::count))
        debug_throw("write: unexpected variant value: " + std::to_string(obj.get().index()));
    write(os, obj.get().index());
    std::visit([this, &os](const auto &value) { write(os, value); }, obj.get());
}


void writer::write_ref(const object &obj) {
    if(debug && obj.get().index() >= static_cast<std::size_t>(object_type::count))
        debug_throw("write_ref: unexpected variant value: " + std::to_string(obj.get().index()));
    write(os, obj.get().index());
    std::visit([this, &os](const auto &value) { write_ref(os, value); }, obj.get());
}
    
