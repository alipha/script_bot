#ifndef LIPH_WRITER_HPP
#define LIPH_WRITER_HPP

#include "object.hpp"
#include "serializer.hpp"

#include <cstdint>
#include <deque>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>


class writer {
public:
    writer() = default;

    writer(const writer &) = delete;
    writer(writer &&) = delete;
    writer &operator=(const writer &) = delete;
    writer &operator=(writer &&) = delete;
    
    void open(const std::string &filename);
    void close();

    bool has_needs_saving() const { return !needs_saving.empty(); }
    bool has_funcs_need_saving() const { return !funcs_need_saving.empty(); }

    std::pair<std::uintptr_t, object&> next_to_save();
    const func_def *next_func_to_save();


    template<typename T>
    void write(const T &value) {
        static_assert(std::is_arithmetic_v<T>);
        if(!os.write(reinterpret_cast<const char*>(&value), sizeof value))
            throw std::runtime_error("error writing to file");
    }

    template<typename... Args>
    void write(const std::basic_string<Args...> &str) {
        write(static_cast<std::uint32_t>(str.size()));
        if(!os.write(str.data(), str.size()))
            throw std::runtime_error("error writing string to file");
    }

    template<typename T>
    void write_addr(const void *ptr, const T value) {
        std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
        if(auto [it, inserted] = objects_by_addr.try_emplace(addr, value); inserted)
            needs_saving.push_back(addr);

        write(addr);
    }

    template<typename T>
    void write_ref(const T &value, const object *object_addr = nullptr) {
        (void)object_addr;

        if constexpr(std::is_same_v<T, std::monostate>) {
            if(object_addr)
                write_addr(object_addr, value);
            // else do nothing
        } else if constexpr(is_non_ref<T>()) {
            if(object_addr)
                write_addr(object_addr, value);
            else
                write(value);
        } else if constexpr(is_ref<T>()) { 
            write_addr(value.get(), value);
        } else if constexpr(std::is_same_v<T, var_ref> || std::is_same_v<T, lvalue_ref>) {
            throw std::logic_error("write_ref: value shouldn't be var_ref or lvalue_ref");
        } else {
            static_assert(sizeof(T) && false, "unknown object variant type");
        }
    }

    void write(const string_ref &str);
    void write(const array_ref &array);
    void write(const map_ref &map);
    void write(const func_ref &func);
    void write(const std::monostate &nul);
    void write(const object &obj);

    void write_func_def(const func_def &func);
    void write_func_def_ref(const func_def &func);

    void write_ref(const object &obj, bool force_ref = false);

private:
    object_type get_type(const object &obj);

    std::ofstream os;
    std::deque<std::uintptr_t> needs_saving;
    std::unordered_map<std::uintptr_t, object> objects_by_addr;

    std::deque<const func_def*> funcs_need_saving;
    std::unordered_set<const func_def*> func_defs;
};

#endif
