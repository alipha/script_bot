#include "debug.hpp"

#ifdef DEBUG
#include <iostream>
#include <stdexcept>

void debug_throw(const std::string &message) { throw std::logic_error(message); }
void debug_out(const std::string &message) { std::cerr << "DEBUG: " << message << std::endl; }

#else

void debug_throw(const std::string &) {}
void debug_out(const std::string &) {}

#endif
