#ifndef LIPH_UTIL_HPP
#define LIPH_UTIL_HPP

#include <stack>
#include <string>
#include <string_view>
#include <variant>


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


template<typename T>
T pop(std::stack<T> &s) {
    T value = s.top();
    s.pop();
    return value;
}


inline bool starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;       
}


template<typename X, typename Y>
struct variant_push {};

template<typename T, typename... Types>
struct variant_push<std::variant<Types...>, T> {
    using type = std::variant<Types..., T>;
};

template<typename T, typename... Types>
struct variant_push<T, std::variant<Types...>> {
    using type = std::variant<T, Types...>;
};

template<typename Variant, typename T>
using variant_push_t = typename variant_push<Variant, T>::type;

template<typename T, typename Variant>
using variant_push_t = typename variant_push<T, Variant>::type;


#endif
