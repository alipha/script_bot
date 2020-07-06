#ifndef LIPH_COMPILER_HPP
#define LIPH_COMPILER_HPP

#include "object_fwd.hpp"
#include <memory>
#include <string_view>
#include <vector>


class memory;
class symbol;
class compiler_impl;


class compiler {
public:
    compiler(memory *m, bool generate_tokenized = false);
    ~compiler();

    compiler(const compiler &) = delete;
    compiler(compiler &&) = delete;
    compiler &operator=(const compiler &) = delete;
    compiler &operator=(compiler &&) = delete;

    std::shared_ptr<func_def> compile(std::vector<symbol> token_list, const std::string &source, bool persist_vars);

    const std::string &tokenized() const;

private:
    std::unique_ptr<compiler_impl> impl;
};


#endif

