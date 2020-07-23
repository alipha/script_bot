#ifndef LIPH_READER_HPP
#define LIPH_READER_HPP

#include "object.hpp"
#include "serializer.hpp"

#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>


class reader {
public:
    reader() = default;

    reader(const reader &) = delete;
    reader(reader &&) = delete;
    reader &operator=(const reader &) = delete;
    reader &operator=(reader &&) = delete;
    
    void open(const std::string &filename);
    void close() { is.close(); }

    template<typename T>
    std::optional<T> read() {
        T value;
        if(!is.read(reinterpret_cast<char*>(&value), sizeof value))
            return std::nullopt;
        else if(is.gcount() != sizeof value)
            throw std::runtime_error("error: only partially read value from file");
        else
            return {value};
    }

    template<typename T>
    T read_or_throw() {
        if(std::optional<T> value = read<T>(is); value)
            return *value;
        else
            throw std::runtime_error("unexpected end of file");
    }

    template<typename String>
    String read_str() {
        std::uint32_t size = read_or_throw<std::uint32_t>(is);
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
