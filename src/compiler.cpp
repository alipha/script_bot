#include "compiler.hpp"
#include "memory_buffer.hpp"
#include "operation_type.hpp"
#include "tokenizer.hpp"
#include "util.hpp"

#include <cstdint>
#include <cstring>
#include <stack>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

using namespace std::string_literals;


std::string compiler::to_postfix(std::vector<std::string_view> token_list) {
    std::string result;
    to_postfix_impl<std::string>(std::move(token_list), result,
            [](std::string &result, std::string_view token, op_code, bool) {
                result += token;
                result += " ";
            });
    return result;
}


std::vector<char> compiler::compile(std::vector<std::string_view> token_list) {
    memory_buffer<debug> result;
    result.append(static_cast<std::uint8_t>(0));

    std::size_t local_var_count = to_postfix_impl<memory_buffer<debug>>(std::move(token_list), result,
        [this](memory_buffer<debug> &buf, std::string_view token, op_code code, bool is_operator) {
            buf.append(code);

            if(is_operator)
                return;

            switch(code) {
            case op_code::str_lit:
                buf.append(parse_str_literal(token));
                break;
            case op_code::global_var:
                buf.append(token);
                break;
            case op_code::local_var:
                buf.append(this->local_var_index(token));
                break;
            case op_code::int_lit:
                buf.append(std::stoi(std::string(token)));  // TODO: from_chars?
                break;
            case op_code::float_lit:
                buf.append(std::stod(std::string(token)));
                break;
            default:
                throw std::logic_error(token + " is currently not supported"s);
            }
        });

    result.buffer()[0] = local_var_count;
    return result.buffer();
}


std::string compiler::parse_str_literal(std::string_view str) {
    std::string result;
    bool backslash = false;

    for(std::size_t i = 1; i < str.size() - 1; ++i) {
        if(!backslash && str[i] == '\\') {
            backslash = true;
        } else {
            backslash = false;
            result += str[i];
        }
    }

    return result;
}


bool compiler::has_lower_precedence(op_code current_code, op_code top_code) {
    operation_type current = lookup_operation(current_code);
    operation_type top = lookup_operation(top_code);

    return current.precedence < top.precedence 
        || (current.precedence == top.precedence && top.associativity == associative::left);
}


std::uint8_t compiler::local_var_index(std::string_view name) {
    std::size_t next_index = local_var_indexes.size() + 1;
    return local_var_indexes.emplace(name, next_index).first->second;
}


template<typename ResultType, typename Accumulator>
std::size_t compiler::to_postfix_impl(std::vector<std::string_view> token_list, ResultType &result, Accumulator accum) {
    std::stack<std::string_view> token_pairs;
    std::stack<op_code> op_codes;
    bool in_binary_context = false;

    local_var_indexes.clear();

    auto tokens = std::move(token_list);
    if(tokens.back() != ";")
        tokens.push_back(";");

    for(auto token : tokens) {
        operation_type op_type = lookup_operation(token, in_binary_context);

        if(op_type.code == op_code::none && !token.empty()) {
            if(in_binary_context)
                throw std::runtime_error("`"s + token + "` was not expected at this point.");

            if(token == "null")
                accum(result, token, op_code::null_lit, true);
            else if(tokenizer::is_identifier(token[0]))
                accum(result, token, op_code::local_var, false);    // TODO: global_var
            else if(token[0] == '"' || token[0] == '\'') // TODO: char?
                accum(result, token, op_code::str_lit, false);
            else if(token.find_first_not_of("0123456789") == std::string_view::npos)
                accum(result, token, op_code::int_lit, false);
            else if(std::isdigit(token[0]))
                accum(result, token, op_code::float_lit, false);
            else
                throw std::runtime_error("Invalid operand: "s + token);
                
            in_binary_context = !in_binary_context;
            continue;
        }

        if(op_type.operand_count == 2)
            in_binary_context = !in_binary_context;

        switch(op_type.code) {
        case op_code::func_call:
        case op_code::left_paren:
        case op_code::array_index:
        case op_code::array_start:
        case op_code::map_start:
            token_pairs.push(token);
            op_codes.push(op_type.code);
            continue;
        case op_code::right_paren:
            if(token_pairs.empty() || pop(token_pairs) != "(")
                throw std::runtime_error("Mismatched )");
            break;
        case op_code::array_end:
            if(token_pairs.empty() || pop(token_pairs) != "[")
                throw std::runtime_error("Mismatched ]");
            break;
        case op_code::map_end:
            if(token_pairs.empty() || pop(token_pairs) != "{")
                throw std::runtime_error("Mismatched }");
            break;
        default: break;
        }
        
        op_code top_code;
        while(!op_codes.empty() && has_lower_precedence(op_type.code, top_code = op_codes.top())) {
            accum(result, lookup_operation(top_code).symbol, top_code, true);
            op_codes.pop();
        }

        op_codes.push(op_type.code);
    }

    if constexpr(debug) {
        if(op_codes.size() != 1 || op_codes.top() != op_code::semicolon) {
            throw std::logic_error("Expected stack to only contain op_code::semicolon. Size: " 
                    + std::to_string(op_codes.size()) + ", code: " 
                    + std::to_string(static_cast<int>(op_codes.top())));
        }
    }

    if(!token_pairs.empty()) {
        throw std::runtime_error("Mismatched "s + pop(token_pairs));
    }

    std::size_t local_var_count = local_var_indexes.size();
    local_var_indexes.clear();

    if(local_var_count > 255) {
        throw std::runtime_error("Too many local variables: " + std::to_string(local_var_count));
    }

    return local_var_count;
}

