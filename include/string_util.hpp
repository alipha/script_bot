#ifndef LIPH_STRING_UTIL_HPP
#define LIPH_STRING_UTIL_HPP

#include <string>
#include <string_view>


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


inline bool starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;       
}


#endif