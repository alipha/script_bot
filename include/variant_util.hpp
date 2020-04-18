#ifndef LIPH_VARIANT_UTIL_HPP
#define LIPH_VARIANT_UTIL_HPP

#include <variant>


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


template<typename To, typename From>
To to_variant(From &&from) { 
    return std::visit([](auto &&arg) -> To { 
        return std::forward<decltype(arg)>(arg); 
    }, std::forward<From>(from)); 
}


#endif
