#include "serializer.hpp"
#include "cleanup.hpp"
#include "gc.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include <cstdint>
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

/*
template<typename T, typename MakeExceptFunc>
T value_or_throw(const std::optional<T> &opt, MakeExceptFunc make_except) {
    if(opt)
        return *opt;
    throw make_except();
}
*/

class serializer_impl {
public:
    serializer_impl(memory *m) : mem(m), r(), w() {}
    
    serializer_impl(const serializer_impl &) = delete;
    serializer_impl(serializer_impl &&) = delete;
    serializer_impl &operator=(const serializer_impl &) = delete;
    serializer_impl &operator=(serializer_impl &&) = delete;
    
    void serialize(const std::string &filename);
    void deserialize(const std::string &filename);
    
private:
    memory *mem;
    reader r;
    writer w;
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
	cleanup c = [this]() { this->w.close(); };

    auto &globals = mem->get_globals();
    w.open(filename);

    w.write<std::uint32_t>(globals.size());
    for(auto &[name, obj_ref] : globals) {
        w.write(name);
        w.write_ref(*obj_ref);
    }

    while(w.has_needs_saving()) {
        const auto& [addr, object] = w.next_to_save();
        w.write(addr);
        w.write(object);
    }

    w.write(static_cast<std::uintptr_t>(0));
    
    while(w.has_funcs_need_saving()) {
        const func_def *func = w.next_func_to_save();
        w.write(reinterpret_cast<std::uintptr_t>(func));
        w.write_func_def(*func);
    }
}


void serializer_impl::deserialize(const std::string &filename) {
	cleanup c = [this]() { this->r.close(); };

    r.open(filename);

    std::size_t global_count = r.read_or_throw<std::uint32_t>();
    mem->reset();
    gc::collect();

    for(std::size_t i = 0; i < global_count; ++i) {
        std::string name = r.read_str<std::string>();
        var_ref lvalue = mem->get_or_add_global(name);

        r.read_ref_to(lvalue.get());
    } 

    while(std::uintptr_t addr = r.read_or_throw<std::uintptr_t>()) {
        object obj = r.read_object();
        r.add_object_ref(addr, std::move(obj));
    }

    while(r.has_needs_loading()) {
        auto [addr, ref] = r.next_to_load();
        *ref = r.get_object(addr);
    }

    // TODO: populate funcs_need_loading

    /*
    while(r.has_funcs_need_loading()) {
        auto [addr, ref] = r.next_func_to_load();
        *ref = r.get_func_def(addr);
    }
    */
}
 
