#include "tokenizer.hpp"

#include <cctype>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>


void tokenizer::tokenize() {
    src += ' ';
    char *token_begin = &src[0];
    char *token_end = &src[0];
    bool finish_token = false;
    bool backslash = false;
    stage token_stage = stage::whitespace;

    toks.clear();
    char prev_ch = '\0';

    for(auto it = src.begin(); it != src.end(); ++it) {
        char ch = *it;

        if(ch == '"' && token_stage != stage::single_quote && !backslash) {
            if(token_stage == stage::double_quote) {
                token_stage = stage::finished;
            } else {
                token_stage = stage::double_quote;
                finish_token = true;
            }
        } else if(ch == '\'' && token_stage != stage::double_quote && !backslash) {
            if(token_stage == stage::single_quote) {
                token_stage = stage::finished;
            } else {
                token_stage = stage::single_quote;
                finish_token = true;
            }
        } else if(token_stage == stage::single_quote || token_stage == stage::double_quote) {
            backslash = (ch == '\\' && !backslash);
        } else if(std::isspace(ch)) {
            if(token_stage != stage::whitespace) {
                token_stage = stage::whitespace;
                finish_token = true;
            }
        } else if((ch >= '0' && ch <= '9') || 
                (token_stage == stage::number && (ch == '.' || ch == 'u' || ch == 'U'))) {
            if(token_stage != stage::identifier && token_stage != stage::number) {
                token_stage = stage::number;
                finish_token = true;
            }
        } else if(is_identifier(ch)) {
            if(token_stage != stage::identifier) {
                token_stage = stage::identifier;
                finish_token = true;
            }
        } else if(std::strchr("(),.;:[]{}", ch)) {
            token_stage = stage::finished;
            finish_token = true;
        } else {
            if(token_stage != stage::oper) {
                token_stage = stage::oper;
                finish_token = true;
            }

            if(std::strchr("?*&-+|<>", ch)) {
                if(prev_ch != ch)
                    finish_token = true;
            } else if(ch == '=') {
                if(token_end - token_begin == 1 && std::strchr("!%^&*-+|<>/=", ch)) {
                    /* do nothing */
                } else if(token_end - token_begin == 2) {
                    std::string_view token(token_begin, token_end - token_begin);
                    if(token != "<<" && token != ">>" && token != "**") {
                        finish_token = true;
                    }
                } else {
                    finish_token = true;
                }
            } else {
                finish_token = true;
            }
        }

        if(finish_token) {
            if(token_end != token_begin) {
                std::string_view token(token_begin, token_end - token_begin);
                toks.emplace_back(token, token_begin - src.begin());
            }
            token_begin = token_end;
            finish_token = false;
        }

        if(token_stage == stage::whitespace) {
            token_begin++;
        }

        token_end++;
        prev_ch = ch;
    }
}

