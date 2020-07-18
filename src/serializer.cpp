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

    void write_ref(std::ostream &os, const object &obj);

    template<typename T>
    std::optional<T> read(std::istream &is);

    template<typename T>
    T read_or_throw(std::istream &is);

    template<typename String>
    String read_str(std::istream &is);

    object_type read_type(std::istream &is);
    
    void read_ref_to(std::istream &is, lvalue_ref dest);
    
    object read_object(std::istream &is);


    memory *mem;
    std::deque<std::uintptr_t> needs_saving;
    std::unordered_map<std::uintptr_t, object> objects_by_addr;
    std::unordered_multimap<std::uintptr_t, lvalue_ref> needs_loading;
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
    needs_saving.clear();
    objects_by_addr.clear();

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

    needs_saving = {};
    objects_by_addr = {};
}


void serializer_impl::deserialize(const std::string &filename) {
    objects_by_addr.clear();
    needs_loading.clear();

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

    objects_by_addr = {};
    needs_loading = {};
}


template<typename T>
void serializer_impl::write(std::ostream &os, const T &value) {
    if(!os.write(reinterpret_cast<const char*>(&value), sizeof value))
        throw std::runtime_error("error writing to file");
}

template<typename... Args>
void serializer_impl::write(std::ostream &os, const std::basic_string<Args...> &str) {
    write(os, static_cast<std::uint32_t>(str.size()));
    if(!os.write(str.data(), str.size()))
        throw std::runtime_error("error writing string to file");
}

void serializer_impl::write(std::ostream &os, const string_ref &str) {
    write(os, *str);
}

void serializer_impl::write(std::ostream &os, const array_ref &array) {
    write(os, static_cast<std::uint32_t>(array->size()));
    for(object &obj : *array)
        write_ref(os, obj);
}

void serializer_impl::write(std::ostream &os, const map_ref &map) {
    write(os, static_cast<std::uint32_t>(map->size()));
    for(auto &[name, obj] : *map) {
        write(os, name);
        write_ref(os, obj);
    }
}

void serializer_impl::write(std::ostream &os, const func_ref &func) {
    (void)os;
    (void)func;
    /*gcvector<std::uint8_t> &code = func->code.buffer();
    write(os, static_cast<std::uint32_t>(code.size()));
    if(!os.write(reinterpret_cast<char*>(code.data()), code.size()))
        throw std::runtime_error("error writing function to file");
*/    // TODO: func_defs are shared
}

void serializer_impl::write(std::ostream &, const std::monostate &) {}
    

template<typename T>
void serializer_impl::write_ref(std::ostream &os, const T &value) {
    if constexpr(std::is_same_v<T, std::monostate>) {
        // do nothing
    } else if constexpr(std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint64_t>
            || std::is_same_v<T, double>) {
        write(os, value);
    } else if constexpr(std::is_same_v<T, string_ref> || std::is_same_v<T, array_ref>
            || std::is_same_v<T, map_ref> || std::is_same_v<T, func_ref>) { 
        std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(value.get());
        if(auto [it, inserted] = objects_by_addr.try_emplace(addr, value); inserted)
            needs_saving.push_back(addr);

        write(os, addr);
    } else if constexpr(std::is_same_v<T, var_ref> || std::is_same_v<T, lvalue_ref>) {
        throw std::logic_error("write_ref: value shouldn't be var_ref or lvalue_ref");
    } else {
        static_assert(sizeof(T) && false, "unknown object variant type");
    }
}
    
void serializer_impl::write(std::ostream &os, const object &obj) {
    if(debug && obj.get().index() >= static_cast<std::size_t>(object_type::count))
        debug_throw("write: unexpected variant value: " + std::to_string(obj.get().index()));
    write(os, obj.get().index());
    std::visit([this, &os](const auto &value) { write(os, value); }, obj.get());
}

void serializer_impl::write_ref(std::ostream &os, const object &obj) {
    if(debug && obj.get().index() >= static_cast<std::size_t>(object_type::count))
        debug_throw("write_ref: unexpected variant value: " + std::to_string(obj.get().index()));
    write(os, obj.get().index());
    std::visit([this, &os](const auto &value) { write_ref(os, value); }, obj.get());
}


template<typename T>
std::optional<T> serializer_impl::read(std::istream &is) {
    T value;
    if(!is.read(reinterpret_cast<char*>(&value), sizeof value))
        return std::nullopt;
    else if(is.gcount() != sizeof value)
        throw std::runtime_error("error: only partially read value from file");
    else
        return {value};
}

template<typename T>
T serializer_impl::read_or_throw(std::istream &is) {
    if(std::optional<T> value = read<T>(is); value)
        return *value;
    else
        throw std::runtime_error("unexpected end of file");
}
    

template<typename String>
String serializer_impl::read_str(std::istream &is) {
    std::uint32_t size = read_or_throw<std::uint32_t>(is);
    String str(size, '\0');
    if(!is.read(str.data(), size) || is.gcount() != size)
        throw std::runtime_error("unable to read string of size " + std::to_string(size));
    return str;
}


object_type serializer_impl::read_type(std::istream &is) {
    std::uint8_t type_value = read_or_throw<std::uint8_t>(is);
    if(type_value >= static_cast<std::uint8_t>(object_type::count))
        throw std::runtime_error("invalid type: " + std::to_string(type_value));
    return static_cast<object_type>(type_value);
}


void serializer_impl::read_ref_to(std::istream &is, lvalue_ref dest) {
    object_type type = read_type(is);

    switch(type) {
    case object_type::nul:    *dest = object(std::monostate()); break;
    case object_type::int64:  *dest = object(read_or_throw<std::int64_t>(is)); break;
    case object_type::uint64: *dest = object(read_or_throw<std::uint64_t>(is)); break;
    case object_type::float8: *dest = object(read_or_throw<double>(is)); break;
    case object_type::func:
        /* todo */
    default: {
            std::uintptr_t addr = read_or_throw<std::uintptr_t>(is);
            needs_loading.emplace(addr, dest);
        }
    }
}


object serializer_impl::read_object(std::istream &is) {
    object_type type = read_type(is);
    std::uint32_t size;

    switch(type) {
    case object_type::string: return object::type(make_string(read_str<gcstring>(is)));
    case object_type::array: {
        size = read_or_throw<std::uint32_t>(is);

        array_ref array = make_array();
        array->resize(size);

        for(object &obj : *array)
            read_ref_to(is, &obj);
            
        return object::type(array);
    }
    case object_type::map: {
        size = read_or_throw<std::uint32_t>(is);

        map_ref map = make_map();
        for(std::size_t i = 0; i < size; ++i) {
            gcstring name = read_str<gcstring>(is);
            auto it = map->try_emplace(name, std::monostate()).first;
            read_ref_to(is, &it->second);
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


