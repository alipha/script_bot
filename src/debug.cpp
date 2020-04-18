#include "debug.hpp"

#ifdef DEBUG
#include <stdexcept>

void debug_throw(const std::string &message) { throw std::logic_error(message); }

#else

void debug_throw(const std::string &message) {}

#endif
