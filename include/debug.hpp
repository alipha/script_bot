#ifndef LIPH_DEBUG_HPP
#define LIPH_DEBUG_HPP

#include <string>


constexpr bool debug = false
#ifdef DEBUG
    || true
#endif
    ;


void debug_throw(const std::string &message);
void debug_out(const std::string &message);

#endif

