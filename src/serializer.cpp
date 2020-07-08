#include "serializer.hpp"
#include "debug.hpp"
#include "gc.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "variant_util.hpp"

#include <cstdint>
#include <deque>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>


enum class object_type : std::uint8_t {
    nul,
    int64,
    uint64,
    float8,
    string,
    array,
    map,
    func,
    count
};


class serializer_impl {
public:
    serializer_impl(memory *m) : mem(m) {}
    
    serializer_impl(const serializer_impl &) = delete;
    serializer_impl(serializer_impl &&) = delete;
    serializer_impl &operator=(const serializer_impl &) = delete;
    serializer_impl &operator=(serializer_impl &&) = delete;
    
    void serialize(const std::string &filename);
    void deserialize(const std::string &filename);
    
private:
    template<typename T>
    void write(std::ostream &os, const T &value);

    template<typename... Args>
    void write(std::ostream &os, const std::basic_string<Args...> &str);

    void write(std::ostream &os, const string_ref &str);
    void write(std::ostream &os, const array_ref &array);
    void write(std::ostream &os, const map_ref &map);
    void write(std::ostream &os, const func_ref &func);
    void write(std::ostream &os, const std::monostate &nul);
    void write(std::ostream &os, const object &obj);

    template<typename T>
    void write_ref(std::ostream &os, const T &value);

    memory *m;
    std::deque<std::uintptr_t> needs_saving;
    std::unordered_map<std::uintptr_t, object> objects_by_addr;
    std::unordered_map<lvalue_ref, std::uintptr_t> needs_loading;
};
    


serializer::serializer(memory *m) : impl(std::make_unique<serializer_impl>(m)) {}

serializer::~serializer() {}

void serializer::serialize(const std::string &filename) {
    impl->serialize(filename);
}

void serializer::deserialize(const std::string &filename) {
    impl->deserialize(filename);
}



void serializer_impl::serialize(const std::string &filename) {
    std::ofstream file(filename);
    if(!file)
        throw std::runtime_error("unable to open file " + filename);

    for(auto &[name, obj_ref] : *globals) {
        write(os, name);
        write(os, *obj_ref);
    }

    while(!needs_saving.empty()) {

}

void serializer_impl::deserialize(const std::string &filename) {
}


template<typename T>
void serializer_impl::write(std::ostream &os, const T &value) {
    if(!os.write(reinterpret_cast<char*>(&value), sizeof value))
        throw std::runtime_error("error writing to file");
}

template<typename... Args>
void serializer_impl::write(std::ostream &os, const std::basic_string<Args...> &str) {
    write(os, str.size());
    if(!os.write(str.data(), str.size()))
        throw std::runtime_error("error writing string to file");
}

void serializer_impl::write(std::ostream &os, const string_ref &str) {
    write(os, *str);
}

void serializer_impl::write(std::ostream &os, const array_ref &array) {
    write(os, array->size());
    for(object &obj : *array)
        write_ref(os, obj);
}

void serializer_impl::write(std::ostream &os, const map_ref &map) {
    write(os, map->size());
    for(auto &[name, obj] : *map) {
        write(os, name);
        write_ref(os, obj);
    }
}

void serializer_impl::write(std::ostream &os, const func_ref &func) {
    gcvector<std::uint8_t> &code = func->code.buffer();
    write(os, code.size());
    if(!os.write(code.data(), code.size()))
        throw std::runtime_error("error writing function to file");
    // TODO: func_defs are shared
}

void serializer_impl::write(std::ostream &os, const std::monostate &nul) {}
    

template<typename T>
void serializer_impl::write_ref(std::ostream &os, const T &value) {
    if constexpr(std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint64_t>
            || std::is_same_v<T, double>) {
        write(os, value);
    } else if constexpr(!std::is_same_v<T, std::monostate>) {
        std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(&*value);
        if(auto [it, inserted] = objects_by_addr.try_emplace(addr, value); inserted)
            needs_saving.push_back(addr);

        write(os, addr);
    }
}
    
void serializer_impl::write(std::ostream &os, const object &obj) {
    if(debug && obj.get().index() >= object_type::count)
        debug_throw("write: unexpected variant value: " + std::to_string(obj.get().index()));
    write(os, obj.get().index());
    std::visit([](auto &value) { write(os, value); }, obj);
}

void serializer_impl::write_ref(std::ostream &os, const object &obj) {
    if(debug && obj.get().index() >= object_type::count)
        debug_throw("write_ref: unexpected variant value: " + std::to_string(obj.get().index()));
    write(os, obj.get().index());
    std::visit([](auto &value) { write_ref(os, value); }, obj);
}

