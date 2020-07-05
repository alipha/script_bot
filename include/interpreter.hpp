#ifndef LIPH_INTERPRETER_HPP
#define LIPH_INTERPRETER_HPP

#include "object_fwd.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>


class memory;
class interpreter_impl;


class interpreter {
public:
    interpreter(memory *m, std::size_t max_call_depth);
    ~interpreter();
    
    std::string execute(std::shared_ptr<func_def> program);

private:
    std::unique_ptr<interpreter_impl> impl;
};


#endif

