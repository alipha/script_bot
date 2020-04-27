#include "compiler.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "memory_buffer.hpp"
#include "operation_type.hpp"
#include "tokenizer.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"
#include "util.hpp"

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>


using namespace std::string_literals;



class compiler_impl {
public:
    compiler_impl(memory *m, bool generate_tokenized) : mem(m), gen_tokenized(generate_tokenized) {}

    std::vector<char> compile(std::vector<std::string_view> token_list);
    
    const std::string &tokenized() const { return tokenized_result; }

private:
    static std::string parse_str_literal(std::string_view str);

    void accum(std::string_view token, op_code code) { accum(token, lookup_operation(op_code::none), code); }
    void accum(std::string_view token, const operation_type &op_type) { accum(token, op_type, op_type.code); }
    void accum(std::string_view token, const operation_type &op_type, op_code code);
    void accum_operand(std::string_view token);

    void pop_op_code(const operation_type &top);

    void handle_comma(op_code top_code, mut<std::size_t> index);
    void handle_pair(const operation_type &op_type, const operation_type &top_op_type);
    void handle_map_label(mut<std::size_t> index);

    void handle_ctrl_cond(mut<operation_type> op_type_ref, mut<bool> in_binary_context);
    void handle_ctrl_end(mut<operation_type> op_type_ref, mut<bool> in_binary_context, mut<std::size_t> index);
    
    bool pop_op_codes(std::string_view token, mut<operation_type> op_type_ref, mut<bool> in_binary_context, mut<std::size_t> index);

    std::uint8_t local_var_index(std::string_view name);


    void reset() {
        local_var_indexes.clear();
        jump_indexes = {};
        while_indexes = {};
        op_codes = {};
        tokens = {};
        result.clear();
        tokenized_result.clear();

        result.append(static_cast<std::uint8_t>(0));  // TODO: remove?
    }

    memory *mem;
    bool gen_tokenized;
    std::unordered_map<std::string, std::uint8_t> local_var_indexes;

    std::stack<std::size_t> jump_indexes;
    std::stack<std::size_t> while_indexes;
    std::stack<op_code> op_codes;
    std::vector<std::string_view> tokens;
    //std::vector<std::string_view>::iterator token_it;

    memory_buffer<debug> result;
    std::string tokenized_result;
};



compiler::compiler(memory *m, bool generate_tokenized) 
    : impl(std::make_unique<compiler_impl>(m, generate_tokenized)) {}

compiler::~compiler() {}

std::vector<char> compiler::compile(std::vector<std::string_view> token_list) {
    return impl->compile(std::move(token_list));
}

const std::string &compiler::tokenized() const { return impl->tokenized(); }

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


void compiler_impl::accum(std::string_view token, const operation_type &op_type, op_code code) {
    if(gen_tokenized) {
        tokenized_result += token;
        tokenized_result += " ";
    }

    //debug_out("accum: "s + token);

    if(!op_type.is_nop)
        result.append(code);

    switch(code) {
    case op_code::while_start:
        while_indexes.push(result.size());
        break;
    case op_code::else_start: {
        debug_out("else_start accum " + std::to_string(jump_indexes.size()));
        std::size_t jump_index = pop(jump_indexes);
        jump_indexes.push(result.append(static_cast<std::uint32_t>(0)));
        //debug_out("Else Patching: " + std::to_string(jump_index) + " with " + std::to_string(result.size()));
        result.patch(jump_index, static_cast<std::uint32_t>(result.size()));
        break;
    }
    case op_code::while_cond:
    case op_code::if_cond:
    case op_code::logic_and:
    case op_code::logic_or:
        jump_indexes.push(result.append(static_cast<std::uint32_t>(0)));
        break;
    case op_code::while_end: {
        std::size_t jump_index = pop(while_indexes);
        //debug_out("While Appending: " + std::to_string(jump_index));
        result.append(static_cast<std::uint32_t>(jump_index));

        jump_index = pop(jump_indexes);
        //debug_out("While Patching: " + std::to_string(jump_index) + " with " + std::to_string(result.size()));
        result.patch(jump_index, static_cast<std::uint32_t>(result.size()));
        break;
    }
    case op_code::if_end:
    case op_code::else_end:
    case op_code::logic_and_end:
    case op_code::logic_or_end: {
        std::size_t jump_index = pop(jump_indexes);
        //debug_out("If Patching: " + std::to_string(jump_index) + " with " + std::to_string(result.size()));
        result.patch(jump_index, static_cast<std::uint32_t>(result.size()));
        break;
    }
    case op_code::str_lit:
        result.append(parse_str_literal(token));
        break;
    case op_code::global_var:
        result.append(token);
        break;
    case op_code::local_var:
        result.append(local_var_index(token));
        break;
    case op_code::int_lit:
        result.append(static_cast<int64_t>(std::stoll(std::string(token))));  // TODO: from_chars?
        break;
    case op_code::uint_lit:
        result.append(static_cast<uint64_t>(std::stoull(std::string(token))));  // TODO: from_chars?
        break;
    case op_code::float_lit:
        result.append(std::stod(std::string(token)));
        break;
    default:
        ; //throw std::logic_error(token + " is currently not supported"s);
    }

    if(op_type.replace_with != op_code::none)
        op_codes.push(op_type.replace_with);
}


void compiler_impl::accum_operand(std::string_view token) {
    if(token == "null")
        accum(token, op_code::null_lit);
    else if(tokenizer::is_identifier(token[0]))
        accum(token, op_code::local_var);    // TODO: global_var
    else if(token[0] == '"' || token[0] == '\'') // TODO: char?
        accum(token, op_code::str_lit);
    else if(std::isdigit(token[0]) && (token.back() == 'u' || token.back() == 'U'))
        accum(token, op_code::uint_lit);
    else if(token.find_first_not_of("0123456789") == std::string_view::npos)
        accum(token, op_code::int_lit);
    else if(std::isdigit(token[0]))
        accum(token, op_code::float_lit);
    else
        throw std::runtime_error("Invalid operand: "s + token);
}


std::string compiler_impl::parse_str_literal(std::string_view str) {
    std::string result;
    bool backslash = false;

    if(str.empty())
        return result;

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


void compiler_impl::pop_op_code(const operation_type &top) {
    op_codes.pop();
    if(top.operand_count != 0)
        accum(top.symbol, top);
}


void compiler_impl::handle_comma(op_code top_code, mut<std::size_t> index) {
    operation_type op_type;

    if(top_code == op_code::map_start) {
        op_type = lookup_operation(op_code::map_add);
    } else if(top_code == op_code::array_start) {
        op_type = lookup_operation(op_code::array_add);
    } else if(top_code == op_code::func_call) {
         op_type = lookup_operation(op_code::param_add);
    } else {
        throw std::runtime_error("Comma not within (), [] or {}");
    }

    accum(op_type.symbol, op_type);

    if(op_type.code == op_code::map_add)
        handle_map_label(index);
}


void compiler_impl::handle_pair(const operation_type &op_type, const operation_type &top_op_type) {
    op_code code = op_type.code;

    if(top_op_type.primary_right_pair != code && top_op_type.other_right_pair != code)
        throw std::runtime_error("Mismatched "s + top_op_type.symbol);
    pop_op_code(top_op_type);
    code = top_op_type.primary_right_pair;

    accum(op_type.symbol, lookup_operation(code));
}


void compiler_impl::handle_map_label(mut<std::size_t> index) {
	std::size_t &i = index.get();

    std::string_view token = tokens[++i];
    operation_type op_type = lookup_operation(token, false);

    if(op_type.code != op_code::none)
        throw std::runtime_error("`"s + token + "` is not a valid map label");

    if(token[0] == '"' || token[0] == '\'') {
        accum(token, op_code::str_lit);
    } else {
        std::string t = "\""s + token + "\"";
        accum({t.data(), t.size()}, op_code::str_lit);
    }

    token = tokens[++i];
    if(token != ":")
        throw std::runtime_error("Expected colon after map label, not "s + token);

    //op_codes.push(op_code::colon); 
}


void compiler_impl::handle_ctrl_cond(mut<operation_type> op_type_ref, mut<bool> in_binary_context) {
	auto &op_type = op_type_ref.get();
    auto &in_binary = in_binary_context.get();

    if(!op_codes.empty()) {
        operation_type top_op_type = lookup_operation(op_codes.top());
        
        if(top_op_type.category == op_category::ctrl_cond) {
            pop_op_code(top_op_type);
            op_type = top_op_type;
            in_binary = false;
        }
    } 
}


void compiler_impl::handle_ctrl_end(mut<operation_type> op_type_ref, mut<bool> in_binary_context, mut<std::size_t> index) {
	auto &op_type = op_type_ref.get();
    auto &in_binary = in_binary_context.get();
    auto &i = index.get();

    operation_type top_op_type;
    
    while(!op_codes.empty() && (top_op_type = lookup_operation(op_codes.top())).category == op_category::ctrl_end) {
        in_binary = false;

        if(top_op_type.code == op_code::if_end && tokens[i + 1] == "else") {
            op_codes.top() = op_code::else_end;
            op_type = lookup_operation(tokens[++i], false);
            accum(op_type.symbol, op_type);
            return;
        }

        pop_op_code(top_op_type);
        op_type = top_op_type;
    }
}


bool compiler_impl::pop_op_codes(std::string_view token, mut<operation_type> op_type_ref, mut<bool> in_binary_context, mut<std::size_t> index) {
    auto &op_type = op_type_ref.get();
    op_code top_code;

    while(!op_codes.empty() && has_lower_precedence(op_type.code, top_code = op_codes.top())) {
        operation_type top_op_type = lookup_operation(top_code);
       
        if(op_type.has_left_pair && top_op_type.primary_right_pair != op_code::none) {
            handle_pair(op_type, top_op_type);

            if(top_op_type.code == op_code::left_paren) {
                handle_ctrl_cond(op_type_ref, in_binary_context);
                //debug_out("left paren " + std::to_string(in_binary_context.get()));
            } else if(top_op_type.code == op_code::block_start) {
                handle_ctrl_end(op_type_ref, in_binary_context, index);
                //debug_out("block start " + std::to_string(in_binary_context.get()));
            }

            return true;
        }

        pop_op_code(top_op_type);
    }

    if(op_type.has_left_pair)
        throw std::runtime_error("Mismatched "s + token);

    return false;
}


std::uint8_t compiler_impl::local_var_index(std::string_view name) {
    std::size_t next_index = local_var_indexes.size() + 1;
    return local_var_indexes.emplace(name, next_index).first->second;
}


std::vector<char> compiler_impl::compile(std::vector<std::string_view> token_list) {
    reset();

    tokens = std::move(token_list);
    tokens.push_back(";");

    bool in_binary_context = false;
    operation_type op_type = lookup_operation(op_code::semicolon);

    for(std::size_t i = 0; i < tokens.size(); ++i) {
        std::string_view token = tokens[i];
        
        op_code last_code = op_type.code; 
        operation_type last_type = lookup_operation(last_code);

        if(token == ";") {
            op_type = lookup_operation(op_code::semicolon);
            in_binary_context = true;
        } else {
            op_type = lookup_operation(token, in_binary_context);
        }

        if(op_type.code == op_code::colon || op_type.code == op_code::else_start) {
            throw std::runtime_error("`"s + token + "` was not expected here");
        }

        //debug_out("last_type: "s + last_type.symbol);

        if(op_type.code == op_code::map_start && last_type.category == op_category::ctrl_cond) {
            op_type = lookup_operation(op_code::block_start);
        }

        if((op_type.category == op_category::ctrl_start || op_type.code == op_code::block_end) 
                && last_code != op_code::semicolon && last_code != op_code::block_start 
                && last_type.category != op_category::ctrl_cond && last_type.category != op_category::ctrl_end) {
            throw std::runtime_error(token + " should be at the start of a statement"s);
        }

        if(op_type.category == op_category::ctrl_start && tokens[i + 1] != "(") {
            throw std::runtime_error("( should follow "s + token);
        }
      
        if(op_type.code == op_code::none && !token.empty()) {
            if(in_binary_context)
                throw std::runtime_error("`"s + token + "` was not expected at this point.");

            accum_operand(token);
            in_binary_context = true;
            continue;
        }

        in_binary_context = op_type.in_binary_context;
        if(op_type.operand_count == 2 || op_type.code == op_code::semicolon)
            in_binary_context = !in_binary_context;

        if(op_type.operand_count == 0) {
            if constexpr(debug) {
                if(op_type.category == op_category::ctrl_cond || op_type.category == op_category::ctrl_end)
                    throw std::logic_error("`"s + op_type.symbol + "` shouldn't get here");
            }

            accum(token, op_type);
            op_codes.push(op_type.code);

            if(op_type.code == op_code::map_start)
                handle_map_label(mut(i));
            
            continue;
        }
        

        if(pop_op_codes(token, mut(op_type), mut(in_binary_context), mut(i)))
            continue;
        

        if(op_type.code == op_code::comma && !op_codes.empty()) {
            handle_comma(op_codes.top(), mut(i));
            continue;
        }

        if(op_type.operand_count == 1 && op_type.associativity == associative::left) {
            accum(op_type.symbol, op_type);

            if(op_type.code == op_code::semicolon) {
                handle_ctrl_end(mut(op_type), mut(in_binary_context), mut(i));

                if(!op_codes.empty() && op_codes.top() != op_code::block_start 
                        && lookup_operation(op_codes.top()).category != op_category::ctrl_end)
                    throw std::runtime_error(token + " should be at the statement level"s);
                //if(last_code == op_code::semicolon)
                    //continue;
            }
        } else {
            op_codes.push(op_type.code);
        }

        if(op_type.category == op_category::ctrl_start && tokens[i + 1] != "(")
            throw std::runtime_error("Expected ( to follow "s + token);
    }

    if constexpr(debug) {
        if(!op_codes.empty()) {
            throw std::logic_error("Expected stack to be empty. Size: " + op_codes.size());
        }
    }
/*        if(op_codes.size() != 1 || op_codes.top() != op_code::semicolon) {
            throw std::logic_error("Expected stack to only contain op_code::semicolon. Size: " 
                    + std::to_string(op_codes.size()) + ", code: " 
                    + std::to_string(op_codes.empty() ? 0 : static_cast<int>(op_codes.top())));
        }
        */

    std::size_t local_var_count = local_var_indexes.size();
    //local_var_indexes.clear();

    if(local_var_count > 255) {
        throw std::runtime_error("Too many local variables: " + std::to_string(local_var_count));
    }

    result.buffer()[0] = static_cast<char>(local_var_count);
    return std::move(result).buffer();
}

