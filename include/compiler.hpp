#ifndef LIPH_COMPILER_HPP
#define LIPH_COMPILER_HPP

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

    std::vector<char> compile(std::vector<symbol> token_list, const std::string &source);

    const std::string &tokenized() const;

private:
    std::unique_ptr<compiler_impl> impl;
};


#endif

