#ifndef LIPH_MEMORY_BUFFER_HPP
#define LIPH_MEMORY_BUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>


class memory_buffer {
public:
    memory_buffer() {}
    memory_buffer(std::vector<char> buf) : buf(std::move(buf)) {}

    template<typename T>
    T *read() {
        std::size_t misalignment = pos % alignof(T);
        if(misalignment)
            pos += alignof(T) - misalignment;
        // TODO: bounds checking?
        return std::launder(reinterpret_cast<T*>(&buf[pos]));
    }

    std::string_view read_str() {
        auto len = read<sd::uint16_t>();
        // TODO: bounds checking?
        std::string_view result(&buf[pos], len);
        pos += len;
        return result;
    }

    void append(const char *str, std::size_t len) {
        append(len);
        std::copy(str, str + len, &buf[pos]);
        pos += len;
    }

    template<typename T>
    void append(T &&value) {
        if constexpr(std::is_same_v<std::remove_cv<T>, std::string_view>) {
            append(value.data(), value.size());
        } else if constexpr(std::is_same_v<std::remove_cv<T>, std::string>) {
            append(value.data(), value.size());
        } else if constexpr(std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*>) {
            append(value, std::strlen(value));
        } else {
            std::size_t len = buf.size();
            std::size_t misalignment = len % alignof(T);
            if(misalignment)
                len += alignof(T) - misalignment;
            buf.resize(len + sizeof(T));
            new (&buf[len]) T(std::forward<T>(value));
        }
    }

    void seek_abs(std::size_t location) { pos = location; }
    void seek_rel(std::ptrdiff_t amount) { pos += amount; /* TODO: bounds checking? */ }

    const std::vector<char> &buffer() const { return buf; }

private:
    std::vector<char> buf;
    std::size_t pos;
};

#endif

