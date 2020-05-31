#ifndef LIPH_TOKENIZER_HPP
#define LIPH_TOKENIZER_HPP

#include <cstddef>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


struct symbol {
    symbol(std::string_view t, std::size_t p) : token(t), pos(p) {}

    std::string_view token;
    std::size_t pos;
};


class tokenizer {
public:
    tokenizer(std::string source) : src(std::move(source)) { tokenize(); }

    tokenizer(const tokenizer &) = delete;
    tokenizer(tokenizer &&) = delete;
    tokenizer &operator=(const tokenizer &) = delete;
    tokenizer &operator=(tokenizer &&) = delete;

    const std::string &source() const { return src; }
    const std::vector<token> &tokens() const { return toks; }

    static bool is_identifier(char ch) { return std::isalpha(ch) || ch == '_' || ch == '$'; }

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
    std::vector<token> toks;
};

#endif
