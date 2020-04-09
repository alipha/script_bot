#include "compiler.hpp"
#include "memory_buffer.hpp"
#include "operation_type.hpp"
#include "tokenizer.hpp"
#include "util.hpp"

#include <cstring>
#include <stack>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

using namespace std::string_literals;


std::string compiler::to_postfix(std::vector<std::string_view> token_list) {
    return to_postfix_impl<std::string>(std::move(token_list), 
            [](std::string &result, std::string_view token, op_code, bool) {
                result += token;
                result += " ";
            });
}


std::vector<char> compiler::compile(std::vector<std::string_view> token_list) { 
    memory_buffer result = to_postfix_impl<memory_buffer<true>>(std::move(token_list),
            [](memory_buffer<true> &buf, std::string_view token, op_code code, bool is_operator) {
                buf.append(code);

                if(is_operator)
                    return;

                switch(code) {
                case op_code::str_lit:
                    buf.append(parse_str_literal(token));
                    break;
                case op_code::var:
                    buf.append(token);
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


template<typename ResultType, typename Accumulator>
ResultType compiler::to_postfix_impl(std::vector<std::string_view> token_list, Accumulator accum) {
    std::stack<std::string_view> token_pairs;
    std::stack<op_code> op_codes;
    bool in_binary_context = false;
    
    ResultType result;

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
                accum(result, token, op_code::var, false);
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

    if(op_codes.size() != 1 || op_codes.top() != op_code::semicolon) {
        throw std::logic_error("Expected stack to only contain op_code::semicolon. Size: " 
                + std::to_string(op_codes.size()) + ", code: " + std::to_string(static_cast<int>(op_codes.top())));
    }

    if(!token_pairs.empty()) {
        throw std::logic_error("Mismatched "s + pop(token_pairs));
    }

    return result;
}

