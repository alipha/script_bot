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
        auto len = *read<std::uint16_t>();
        bounds_check(len);

        std::string_view result(&buf[pos], len);
        pos += len;
        return result;
    }

    std::size_t append(const char *str, std::size_t len) {
        if(len > std::numeric_limits<std::uint16_t>::max())
            throw std::runtime_error("text is too long. len = " + std::to_string(len));
        std::size_t loc = append(static_cast<std::uint16_t>(len));

        std::size_t current_len = buf.size();
        buf.resize(current_len + len);   // TODO: check for overflow?
        std::copy(str, str + len, &buf[current_len]);
        return loc;
    }

    template<typename T>
    std::size_t append(T &&value) {    // TODO: emplace_back?
        using TObj = std::decay_t<T>;

        if constexpr(std::is_same_v<TObj, std::string_view>) {
            return append(value.data(), value.size());
        } else if constexpr(std::is_same_v<TObj, std::string>) {
            return append(value.data(), value.size());
        } else if constexpr(std::is_same_v<TObj, const char*> || std::is_same_v<TObj, char*>) {
            return append(value, std::strlen(value));
        } else {
            std::size_t len = buf.size();
            std::size_t misalignment = len % alignof(TObj);
            if(misalignment)
                len += alignof(TObj) - misalignment;
            buf.resize(len + sizeof(TObj)); // TODO: check for overflow? and for misalignment above?
            new (&buf[len]) TObj(std::forward<T>(value));
            return len;
        }
    }

    template<typename T>
    void patch(std::size_t location, T &&value) {
        using TObj = std::decay_t<T>;
        if(BoundsCheck && location + sizeof(TObj) > buf.size())
            throw std::logic_error("out of bounds patch with location=" + std::to_string(location) + 
                    ", len=" + std::to_string(sizeof(TObj)) + ", size=" + std::to_string(buf.size())); // TODO: runtime_error?
        new (&buf[location]) TObj(std::forward<T>(value));
    }

    void extend(std::size_t amount) { buf.resize(buf.size() + amount); }
    
    void seek_abs(std::size_t location) { pos = location; }
    
    void seek_rel(std::ptrdiff_t amount) { 
        pos += amount;  // TODO: check for overflow?
        bounds_check(0); 
    }

    void clear() {
        buf.clear();
        pos = 0;
    }
    
    std::vector<char> buffer() && {
        std::vector<char> ret(std::move(buf));
        clear();
        return ret;
    }

    std::vector<char> &buffer() & { return buf; }
    const std::vector<char> &buffer() const & { return buf; }

    std::size_t position() const { return pos; }
    std::size_t size() const { return buf.size(); }

private:
    std::vector<char> buf;
    std::size_t pos;
};

#endif

