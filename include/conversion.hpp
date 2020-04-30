#ifndef LIPH_CONVERSION_HPP
#define LIPH_CONVERSION_HPP

#include <cstdint>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

#include "object_fwd.hpp"


template<typename Int>
std::int64_t to_int(const Int &arg) {
    using T = std::decay_t<decltype(arg)>;

    if constexpr(std::is_arithmetic_v<T>)
        return static_cast<std::int64_t>(arg);
    else
        throw std::runtime_error("expected arithmetic value"); 
}

inline std::uint64_t to_int(std::uint64_t arg) { return arg; }



inline std::optional<std::string> to_optional_string(std::monostate,   std::size_t = 0, std::size_t* = nullptr, bool = false) { 
    return {};
}

inline std::optional<std::string> to_optional_string(const func_ref &, std::size_t = 0, std::size_t* = nullptr, bool = false) { 
    return {"<function>"}; 
}

inline std::optional<std::string> to_optional_string(std::uint64_t v,  std::size_t = 0, std::size_t* = nullptr, bool = false) { 
    return {std::to_string(v)};
}

inline std::optional<std::string> to_optional_string(std::int64_t v,   std::size_t = 0, std::size_t* = nullptr, bool = false) { 
    return {std::to_string(v)};
}

std::optional<std::string> to_optional_string(const string_ref &s,     std::size_t = 0, std::size_t* = nullptr, bool format = false);
std::optional<std::string> to_optional_string(double v,                std::size_t = 0, std::size_t* = nullptr, bool = false);

std::optional<std::string> to_optional_string(const array_ref &ref,    std::size_t = 0, std::size_t* = nullptr, bool = false);
std::optional<std::string> to_optional_string(const map_ref &ref,      std::size_t = 0, std::size_t* = nullptr, bool = false);



template<typename T>
inline std::string to_str(const T &v) { return to_optional_string(v).value_or("null"); }


#endif
