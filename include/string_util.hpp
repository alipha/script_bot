#ifndef LIPH_STRING_UTIL_HPP
#define LIPH_STRING_UTIL_HPP

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

// yes, overloading operators for std::string and std::string_view combinations is not allowed, but
// i don't care
template<typename Char, typename Traits, typename Alloc>
std::basic_string<Char, Traits, Alloc> operator+(std::basic_string<Char, Traits, Alloc> left, std::basic_string_view<Char, Traits> right) {
    left.append(right);
    return left;
}


template<typename Char, typename Traits, typename Alloc>
std::basic_string<Char, Traits, Alloc> operator+(std::basic_string_view<Char, Traits> left, std::basic_string<Char, Traits, Alloc> right) {
    right.insert(0, left);
    return right;
}


template<typename Char, typename Traits, typename Alloc>
bool operator<(const std::basic_string<Char, Traits, Alloc> &left, std::basic_string_view<Char, Traits> right) {
    return std::string_view(left) < right;
}


template<typename Char, typename Traits, typename Alloc>
bool operator<(std::basic_string_view<Char, Traits> left, const std::basic_string<Char, Traits, Alloc> &right) {
    return left < std::string_view(right);
}


inline bool starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;       
}


inline std::string to_lower(std::string str) {
    for(std::size_t i = 0, len = str.size(); i < len; ++i)
        str[i] = std::tolower(str[i]);

    return str;
}


inline std::string to_lower(std::string_view str) { return to_lower(std::string(str)); }


inline std::string trim(const std::string &str) {
    std::size_t first_letter = str.find_first_not_of(' ');
    if(first_letter == std::string::npos)
        return "";

    return str.substr(first_letter, str.find_last_not_of(' ') - first_letter + 1);
}

#endif
