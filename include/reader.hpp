#ifndef LIPH_READER_HPP
#define LIPH_READER_HPP

#include "endian.hpp"
#include "object.hpp"
#include "serializer.hpp"

#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>


class reader {
public:
    reader() = default;

    reader(const reader &) = delete;
    reader(reader &&) = delete;
    reader &operator=(const reader &) = delete;
    reader &operator=(reader &&) = delete;
    
    void open(const std::string &filename);
    void close();
    
    object get_object(std::uintptr_t addr);
    void add_object_ref(std::uintptr_t addr, object &&obj);

    std::pair<std::uintptr_t, lvalue_ref> next_to_load();

    bool has_needs_loading() const { return !needs_loading.empty(); }

    template<typename T>
    std::optional<T> read() {
        char bytes[sizeof(T)];
        if(!is.read(bytes, sizeof bytes))
            return std::nullopt;
        else if(is.gcount() != sizeof bytes)
            throw std::runtime_error("error: only partially read value from file");
        else
            return {from_little_endian<T>(bytes)};
    }

    template<typename T>
    T read_or_throw() {
        if(std::optional<T> value = read<T>(); value)
            return *value;
        else
            throw std::runtime_error("unexpected end of file");
    }

    template<typename String>
    String read_str() {
        std::uint32_t size = read_or_throw<std::uint32_t>();
        String str(size, '\0');
        if(!is.read(str.data(), size) || is.gcount() != size)
            throw std::runtime_error("unable to read string of size " + std::to_string(size));
        return str;
    }

    void read_ref_to(lvalue_ref dest);
    
    object read_object();

private:
    object_type read_type();
    
    std::ifstream is;
    std::unordered_map<std::uintptr_t, object> objects_by_addr;
    std::unordered_multimap<std::uintptr_t, lvalue_ref> needs_loading;
};

#endif
