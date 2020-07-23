#include "serializer.hpp"
#include "cleanup.hpp"
#include "debug.hpp"
#include "gc.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "reader.hpp"
#include "variant_util.hpp"
#include "writer.hpp"

#include <cstdint>
#include <deque>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

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
	cleanup c = [this]() {
    	this->needs_saving = {};
    	this->objects_by_addr = {};
	};

    auto &globals = mem->get_globals();
    std::ofstream os(filename);
    if(!os)
        throw std::runtime_error("unable to open file for writing: " + filename);

    write<std::size_t>(os, globals.size());
    for(auto &[name, obj_ref] : globals) {
        write(os, name);
        write_ref(os, *obj_ref);
    }

    while(!needs_saving.empty()) {
        std::uintptr_t addr = needs_saving.front();
        needs_saving.pop_front();

        if(debug && objects_by_addr.find(addr) == objects_by_addr.end())
            throw std::logic_error("addr is not in objects_by_addr");
        write(os, addr);
        write(os, objects_by_addr[addr]);
    }
}


void serializer_impl::deserialize(const std::string &filename) {
	cleanup c = [this]() {
    	this->needs_loading = {};
    	this->objects_by_addr = {};
	};

    std::ifstream is(filename);
    if(!is)
        throw std::runtime_error("unable to open file for reading: " + filename);

    std::size_t global_count = read_or_throw<std::size_t>(is);
    mem->reset();
    gc::collect();

    for(std::size_t i = 0; i < global_count; ++i) {
        std::string name = read_str<std::string>(is);
        var_ref lvalue = mem->get_or_add_global(name);

        read_ref_to(is, lvalue.get());
    } 

    while(std::uintptr_t addr = read<std::uintptr_t>(is).value_or(0)) {
        object obj = read_object(is);
        if(debug && objects_by_addr.find(addr) != objects_by_addr.end())
            throw std::logic_error("objects_by_addr already contains addr");
        objects_by_addr[addr] = obj;
    }

    while(!needs_loading.empty()) {
        auto [addr, ref] = *needs_loading.begin();
        if(debug && objects_by_addr.find(addr) == objects_by_addr.end())
            throw std::logic_error("objects_by_addr doesn't contain addr");
        *ref = objects_by_addr[addr];
        needs_loading.erase(needs_loading.begin());
    }
}
 
