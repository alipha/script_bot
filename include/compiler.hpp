#ifndef LIPH_COMPILER_HPP
#define LIPH_COMPILER_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>


class memory;
class tokenizer;
class compiler_impl;


class compiler {
public:
    compiler(memory *m, tokenizer *t, bool generate_tokenized = false);
    ~compiler();

    std::vector<char> compile(std::string program);

    const std::string &tokenized() const;

private:
    std::unique_ptr<compiler_impl> impl;
};


#endif

