#ifndef LIPH_MEMORY_BUFFER_HPP
#define LIPH_MEMORY_BUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>


template<bool BoundsCheck>
class memory_buffer {
public:
    memory_buffer() : pos(0) {}
    memory_buffer(std::vector<char> buf) : buf(std::move(buf)), pos(0) {}

    void bounds_check(std::size_t len) {
        if (BoundsCheck && (pos > buf.size() || buf.size() - pos < len)) {
            throw std::logic_error("out of bounds read with pos=" + std::to_string(pos) + 
                    ", len=" + std::to_string(len) + ", size=" + std::to_string(buf.size())); // TODO: runtime_error?
        }
    }

    template<typename T>
    T *read() {
        std::size_t misalignment = pos % alignof(T);
        if(misalignment)
            pos += alignof(T) - misalignment;
        
        bounds_check(sizeof(T));
        T *result = std::launder(reinterpret_cast<T*>(&buf[pos]));
        pos += sizeof(T);
        return result;
    }

    std::string_view read_str() {
        auto len = read<std::uint16_t>();
        bounds_check(len);

        std::string_view result(&buf[pos], len);
        pos += len;
        return result;
    }

    void append(const char *str, std::size_t len) {
        if(len > std::numeric_limits<std::uint16_t>::max())
            throw std::runtime_error("text is too long. len = " + std::to_string(len));
        append(static_cast<std::uint16_t>(len));

        std::size_t current_len = buf.size();
        buf.resize(current_len + len);   // TODO: check for overflow?
        std::copy(str, str + len, &buf[current_len]);
    }

    template<typename T>
    void append(T &&value) {    // TODO: emplace_back?
        using TObj = std::decay_t<T>;

        if constexpr(std::is_same_v<TObj, std::string_view>) {
            append(value.data(), value.size());
        } else if constexpr(std::is_same_v<TObj, std::string>) {
            append(value.data(), value.size());
        } else if constexpr(std::is_same_v<TObj, const char*> || std::is_same_v<TObj, char*>) {
            append(value, std::strlen(value));
        } else {
            std::size_t len = buf.size();
            std::size_t misalignment = len % alignof(TObj);
            if(misalignment)
                len += alignof(TObj) - misalignment;
            buf.resize(len + sizeof(TObj)); // TODO: check for overflow? and for misalignment above?
            new (&buf[len]) TObj(std::forward<T>(value));
        }
    }

    void seek_abs(std::size_t location) { pos = location; }
    
    void seek_rel(std::ptrdiff_t amount) { 
        pos += amount;  // TODO: check for overflow?
        bounds_check(0); 
    }
    
    std::vector<char> &buffer() { return buf; }
    const std::vector<char> &buffer() const { return buf; }

    std::size_t position() const { return pos; }
    std::size_t size() const { return buf.size(); }

private:
    std::vector<char> buf;
    std::size_t pos;
};

#endif

