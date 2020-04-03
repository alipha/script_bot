#ifndef LIPH_TOKENIZER_HPP
#define LIPH_TOKENIZER_HPP

#include <string>
#include <string_view>
#include <utility>
#include <vector>


class tokenizer {
public:
    tokenizer(std::string source) : src(std::move(source)) { tokenize(); }

    tokenizer(const tokenizer &) = delete;
    tokenizer(tokenizer &&) = delete;
    tokenizer &operator=(const tokenizer &) = delete;
    tokenizer &operator=(tokenizer &&) = delete;

    const std::string &source() const { return src; }
    const std::vector<std::string_view> &tokens() const { return toks; }

private:
    enum class stage {
        whitespace,
        identifier,
        oper,
        number,
        double_quote,
        single_quote,
        finished
    };

    void tokenize();

    std::string src;
    std::vector<std::string_view> toks;
};

#endif
