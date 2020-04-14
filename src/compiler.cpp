#include "compiler.hpp"
#include "memory_buffer.hpp"
#include "operation_type.hpp"
#include "tokenizer.hpp"
#include "util.hpp"

#include <cstdint>
#include <cstring>
#include <limits>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <iostream>

using namespace std::string_literals;


/*
unsigned stou(std::string const & str, size_t * idx = 0, int base = 10) {
    unsigned long result = std::stoul(str, idx, base);
    if (result > std::numeric_limits<unsigned>::max()) {
        throw std::out_of_range("stou");
    }
    return result;
}


template<typename Int>
Int string_to(std::string const & str, size_t * idx = 0, int base = 10) {
    static_assert(std::is_integral_v<Int>, "string_to only works with integer types");
    if constexpr(std::is_unsigned_v<Int>) {
        if constexpr(sizeof(Int) == sizeof(unsigned int))
            return std::stou(str, idx, base);
        else if constexpr(sizeof(Int) == sizeof(unsigned long))
            return std::stoul(str, idx, base);
        else if constexpr(sizeof(Int) == sizeof(unsigned long long))
            return std::stoull(str, idx, base);
    } else {
        if constexpr(sizeof(Int) == sizeof(int))
            return std::stoi(str, idx, base);
        else if constexpr(sizeof(Int) == sizeof(long))
            return std::stol(str, idx, base);
        else if constexpr(sizeof(Int) == sizeof(long long))
            return std::stoll(str, idx, base);
    }
}
*/


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
    std::stack<std::size_t> jump_indexes;

    memory_buffer<debug> result;
    result.append(static_cast<std::uint8_t>(0));

    std::size_t local_var_count = to_postfix_impl<memory_buffer<debug>>(std::move(token_list), result,
        [this, &jump_indexes](memory_buffer<debug> &buf, std::string_view token, op_code code, bool is_operator) {
            buf.append(code);

            switch(code) {
            case op_code::if_start:
                jump_indexes.push(buf.append(static_cast<std::uint32_t>(0)));
                break;
            case op_code::if_block: {
                std::size_t jump_index = pop(jump_indexes);
                std::cout << "Patching: " << jump_index << " with " << buf.size() << std::endl;
                buf.patch(jump_index, static_cast<std::uint32_t>(buf.size()));
                break;
            }
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
                buf.append(static_cast<int64_t>(std::stoll(std::string(token))));  // TODO: from_chars?
                break;
            case op_code::uint_lit:
                buf.append(static_cast<uint64_t>(std::stoull(std::string(token))));  // TODO: from_chars?
                break;
            case op_code::float_lit:
                buf.append(std::stod(std::string(token)));
                break;
            default:
                if(!is_operator)
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
void find_pair(std::stack<op_code> &token_pairs, std::stack<op_code> &op_codes, operation_type current, op_code target, op_code target2, ResultType &result, Accumulator accum) {
    op_code top_code;

    if(token_pairs.empty() || ((top_code = pop(token_pairs)) != target && top_code != target2))
        throw std::runtime_error("find_pair: Mismatched "s + lookup_operation(target).symbol);

    while((top_code = pop(op_codes)) != target && top_code != target2) {
        accum(result, lookup_operation(top_code).symbol, top_code, true);
    }

    operation_type top_type = lookup_operation(top_code);
    if(top_type.operand_count == 2)
        accum(result, top_type.symbol, top_code, true);
    else
        accum(result, current.symbol, current.code, true);
}


template<typename ResultType, typename Accumulator>
std::size_t compiler::to_postfix_impl(std::vector<std::string_view> token_list, ResultType &result, Accumulator accum) {
    std::stack<op_code> token_pairs;
    std::stack<op_code> op_codes;
    op_code last_code = op_code::none;
    bool in_binary_context = false;
    std::string_view expect_token = "";

    local_var_indexes.clear();

    auto tokens = std::move(token_list);
    if(tokens.back() != ";")
        tokens.push_back(";");

    for(auto token : tokens) {
        if(expect_token != "") {
            if(expect_token == "{") {
                if(token == "{") {
                    op_codes.push(op_code::block_start);
                    token_pairs.push(op_code::block_start);
                    in_binary_context = false;
                    continue;
                }
            } else if(token != expect_token) {
                throw std::runtime_error("Expected `"s + expect_token + "` not " + token);
            }

            expect_token = "";
            if(token == ":")
                continue;
        }

        if((last_code == op_code::array_start && token == "]")
                || (last_code == op_code::map_start && token == "}")) {
//                || (last_code == op_code::func_call && token == ")")) {
            op_codes.pop();
            token_pairs.pop();
            in_binary_context = !in_binary_context;
            last_code = op_code::none;
            continue;
        }

        operation_type op_type = lookup_operation(token, in_binary_context);

        if(last_code == op_code::map_start || (last_code == op_code::comma && token_pairs.top() == op_code::map_start)) {
            if(op_type.code != op_code::none || token.empty() || token == "null")
                throw std::runtime_error("`"s + token + "` is not a valid map key.");

            if(token[0] == '"' || token[0] == '\'') {
                accum(result, token, op_code::str_lit, false);
            } else {
                std::string t = "\""s + token + "\"";
                accum(result, {t.data(), t.size()}, op_code::str_lit, false);
            }

            expect_token = ":";
            last_code = op_type.code;
            continue;
        }

        expect_token = "";
        last_code = op_type.code;

        if(op_type.code == op_code::none && !token.empty()) {
            if(in_binary_context)
                throw std::runtime_error("`"s + token + "` was not expected at this point.");

            if(token == "null")
                accum(result, token, op_code::null_lit, true);
            else if(tokenizer::is_identifier(token[0]))
                accum(result, token, op_code::local_var, false);    // TODO: global_var
            else if(token[0] == '"' || token[0] == '\'') // TODO: char?
                accum(result, token, op_code::str_lit, false);
            else if(std::isdigit(token[0]) && (token.back() == 'u' || token.back() == 'U'))
                accum(result, token, op_code::uint_lit, false);
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
        case op_code::colon:
            throw std::runtime_error("`:` was not expected here");
        case op_code::if_start:
            if(!token_pairs.empty() && token_pairs.top() != op_code::block_start)
                throw std::runtime_error(token + " is not expected here"s);
            token_pairs.push(op_type.code);
            expect_token = "(";
            break;
        case op_code::semicolon:
            if(!token_pairs.empty() && token_pairs.top() == op_code::if_block) {
                find_pair(token_pairs, op_codes, lookup_operation(op_code::if_block), op_code::if_block, op_code::none, result, accum);
                continue;
            }
            break;
        case op_code::func_call:
        case op_code::left_paren:
        case op_code::index:
        case op_code::array_start:
        case op_code::map_start:
            //if(op_type.code == op_code::array_start || op_type.code == op_code::map_start)
            if(op_type.operand_count == 1)
                accum(result, op_type.symbol, op_type.code, true);
            token_pairs.push(op_type.code);
            op_codes.push(op_type.code);
            continue;
        case op_code::right_paren:
            find_pair(token_pairs, op_codes, op_type, op_code::func_call, op_code::left_paren, result, accum);
            if(!token_pairs.empty() && token_pairs.top() == op_code::if_start) {
                accum(result, lookup_operation(op_code::if_start).symbol, op_code::if_start, false);
                token_pairs.top() = op_code::if_block;
                op_codes.push(op_code::if_block);
                expect_token = "{";
                in_binary_context = false;
            }
            continue;
        case op_code::array_end:
            find_pair(token_pairs, op_codes, op_type, op_code::index, op_code::array_start, result, accum);
            continue;
        case op_code::map_end:
            find_pair(token_pairs, op_codes, op_type, op_code::map_start, op_code::block_start, result, accum);
            if(!token_pairs.empty() && token_pairs.top() == op_code::if_block) { 
                accum(result, lookup_operation(op_code::if_block).symbol, op_code::if_block, true);
                token_pairs.pop();
                if(pop(op_codes) != op_code::if_block)      // TODO: remove?
                    throw std::logic_error("expected if_block");
            }
            continue;
        default: 
            break;
        }
        
        op_code top_code;

        while(!op_codes.empty() && has_lower_precedence(op_type.code, top_code = op_codes.top())) {
            switch(top_code) {
            case op_code::func_call:    // TODO: remove?
            case op_code::left_paren:
            case op_code::index:
            case op_code::array_start:
            case op_code::map_start:
                throw std::logic_error("Unexpected symbol: "s + lookup_operation(top_code).symbol);
            default:
                accum(result, lookup_operation(top_code).symbol, top_code, true);
            }
            op_codes.pop();
        }

        if(op_type.code == op_code::if_start) {
            /* do nothing */
        } else if(op_type.code == op_code::comma) {
            if(token_pairs.empty())
                throw std::runtime_error("Comma not within (), [] or {}");

            op_code code = op_code::comma;

            if(token_pairs.top() == op_code::array_start)
                code = op_code::array_add;
            else if(token_pairs.top() == op_code::map_start)
                code = op_code::map_add;

            accum(result, ",", code, true);
            //op_codes.push(op_code::array_add);
        } else {    
            op_codes.push(op_type.code);
        }
    }

    /*
    if constexpr(debug) {
        if(op_codes.size() != 1 || op_codes.top() != op_code::semicolon) {
            throw std::logic_error("Expected stack to only contain op_code::semicolon. Size: " 
                    + std::to_string(op_codes.size()) + ", code: " 
                    + std::to_string(op_codes.empty() ? 0 : static_cast<int>(op_codes.top())));
        }
    }
    */

    if(!token_pairs.empty()) {
        throw std::runtime_error("Mismatched "s + lookup_operation(pop(token_pairs)).symbol);
    }
    
    std::size_t local_var_count = local_var_indexes.size();
    local_var_indexes.clear();

    if(local_var_count > 255) {
        throw std::runtime_error("Too many local variables: " + std::to_string(local_var_count));
    }

    return local_var_count;
}

