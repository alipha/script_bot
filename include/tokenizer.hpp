#ifndef LIPH_TOKENIZER_HPP
#define LIPH_TOKENIZER_HPP

#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


struct token_value {
    std::string_view token;
    std::size_t pos;
};


class tokenizer {
public:
    using value_type = token_value;

    tokenizer() {}
    tokenizer(std::string source) : src(std::move(source)), pos(0) {}

    const std::string &source() const { return src; }

    static bool is_identifier(char ch) { return std::isalpha(ch) || ch == '_' || ch == '$'; }

    class iterator {
    public:
        token_value 
    private:
        tokenizer *t;
        char *token_begin;
        char *token_end;
        stage next_stage;
    };

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
    std::size_t pos;
};

#endif
