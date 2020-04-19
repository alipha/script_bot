#ifndef LIPH_COMPILER_HPP
#define LIPH_COMPILER_HPP

#include <memory>
#include <string_view>
#include <vector>


class memory;
class compiler_impl;


class compiler {
public:
    compiler(memory *m, bool generate_tokenized = false);
    ~compiler();

    std::vector<char> compile(std::vector<std::string_view> token_list);

    const std::string &tokenized() const;

private:
    std::unique_ptr<compiler_impl> impl;
};


#endif

