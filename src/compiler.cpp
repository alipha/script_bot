#include "compiler.hpp"
#include "bytecode_builder.hpp"
#include "debug.hpp"
#include "operation_type.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"
#include "util.hpp"

#include <cstring>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>


using namespace std::string_literals;



class compiler_impl {
public:
    compiler_impl(memory *m, bool generate_tokenized) : builder(m, &op_codes, generate_tokenized) {}

    std::vector<char> compile(std::vector<std::string_view> token_list);
    
    const std::string &tokenized() const { return builder.tokenized(); }

private:
    void pop_op_code(const operation_type &top);

    op_code handle_comma(op_code top_code, mut<std::size_t> index);
    void handle_pair(const operation_type &op_type, const operation_type &top_op_type);
    op_code handle_map_label(mut<std::size_t> index, bool map_start);

    void handle_ctrl_cond(mut<operation_type> op_type_ref);
    void handle_ctrl_end(mut<operation_type> op_type_ref, mut<std::size_t> index);
    
    bool pop_op_codes(std::string_view token, mut<operation_type> op_type_ref, mut<std::size_t> index);


    void reset() {
        op_codes = {};
        last_code = op_code::none;
        last_type = {};
        tokens = {};
        builder.reset();
    }

    std::stack<op_code> op_codes;
    op_code last_code;
    operation_type last_type;
    std::vector<std::string_view> tokens;
    bytecode_builder builder;
    //std::vector<std::string_view>::iterator token_it;
};



compiler::compiler(memory *m, bool generate_tokenized) 
    : impl(std::make_unique<compiler_impl>(m, generate_tokenized)) {}

compiler::~compiler() {}

std::vector<char> compiler::compile(std::vector<std::string_view> token_list) {
    return impl->compile(std::move(token_list));
}

const std::string &compiler::tokenized() const { return impl->tokenized(); }



void compiler_impl::pop_op_code(const operation_type &top) {
    op_codes.pop();
    if(top.operand_count != 0)
        builder.append(top.symbol, top);
}


op_code compiler_impl::handle_comma(op_code top_code, mut<std::size_t> index) {
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

    builder.append(op_type.symbol, op_type);

    if(op_type.code == op_code::map_add)
        return handle_map_label(index, false);
    return op_type.code;
}


void compiler_impl::handle_pair(const operation_type &op_type, const operation_type &top_op_type) {
    op_code code = op_type.code;

    if(top_op_type.primary_right_pair != code && top_op_type.other_right_pair != code)
        throw std::runtime_error("Mismatched "s + top_op_type.symbol);

    pop_op_code(top_op_type);
    code = top_op_type.primary_right_pair;

    builder.append(op_type.symbol, lookup_operation(code));
}


op_code compiler_impl::handle_map_label(mut<std::size_t> index, bool start_map) {
	std::size_t &i = index.get();

    std::string_view token = tokens[++i];                             
    if(start_map && token == "}") {
        op_codes.pop();
        return op_code::map_end;
    }

    operation_type op_type = lookup_operation(token, false);

    if(op_type.code != op_code::none)
        throw std::runtime_error("`"s + token + "` is not a valid map label");

    if(token[0] == '"' || token[0] == '\'') {
        builder.append(token, op_code::str_lit);
    } else {
        std::string t = "\""s + token + "\"";
        builder.append({t.data(), t.size()}, op_code::str_lit);
    }

    token = tokens[++i];
    if(token != ":")
        throw std::runtime_error("Expected colon after map label, not "s + token);

    return op_code::colon;
    //op_codes.push(op_code::colon); 
}


void compiler_impl::handle_ctrl_cond(mut<operation_type> op_type_ref) {
	auto &op_type = op_type_ref.get();

    if(!op_codes.empty()) {
        operation_type top_op_type = lookup_operation(op_codes.top());
        
        if(top_op_type.category == op_category::ctrl_cond) {
            pop_op_code(top_op_type);
            op_type = top_op_type;
        }
    } 
}


void compiler_impl::handle_ctrl_end(mut<operation_type> op_type_ref, mut<std::size_t> index) {
	auto &op_type = op_type_ref.get();
    auto &i = index.get();

    operation_type top_op_type;
    
    while(!op_codes.empty() && (top_op_type = lookup_operation(op_codes.top())).category == op_category::ctrl_end) {

        if(top_op_type.code == op_code::if_end && tokens[i + 1] == "else") {
            op_codes.top() = op_code::else_end;
            op_type = lookup_operation(tokens[++i], false);
            builder.append(op_type.symbol, op_type);
            return;
        }

        pop_op_code(top_op_type);
        op_type = top_op_type;
    }
}


bool compiler_impl::pop_op_codes(std::string_view token, mut<operation_type> op_type_ref, mut<std::size_t> index) {
    auto &op_type = op_type_ref.get();
    op_code top_code;

    while(!op_codes.empty() && has_lower_precedence(op_type.code, top_code = op_codes.top())) {
        operation_type top_op_type = lookup_operation(top_code);
       
        if(op_type.has_left_pair && top_op_type.primary_right_pair != op_code::none) {
            handle_pair(op_type, top_op_type);

            if(top_op_type.code == op_code::left_paren) {
                handle_ctrl_cond(op_type_ref);
                //debug_out("left paren " + std::to_string(in_binary_context.get()));
            } else if(top_op_type.code == op_code::block_start) {
                handle_ctrl_end(op_type_ref, index);
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


std::vector<char> compiler_impl::compile(std::vector<std::string_view> token_list) {
    reset();

    tokens = std::move(token_list);
    tokens.push_back(";");

    bool in_binary_context = false;
    operation_type op_type = lookup_operation(op_code::semicolon);

    for(std::size_t i = 0; i < tokens.size(); ++i) {
        std::string_view token = tokens[i];
        
        last_code = op_type.code; 
        last_type = lookup_operation(last_code);
        in_binary_context = last_type.is_binary_next;

        if(token == ";" && (last_type.category == op_category::ctrl_cond || last_type.category == op_category::ctrl_end || last_code == op_code::block_start || last_code == op_code::block_end || last_code == op_code::semicolon)) {
            op_type = lookup_operation(op_code::semicolon);
        } else {
            op_type = lookup_operation(token, in_binary_context, last_type);
            if(is_empty_pair(last_type, op_type)) {
                pop_op_code(last_type);
                op_type.code = last_type.primary_right_pair;
                continue;
            }
           //std::cout << "here " << op_type.symbol << ' ' << op_type.precedence << ' ' << last_type.symbol << ' ' << last_type.operand_count << std::endl;
        }

        if(op_type.code == op_code::colon || op_type.code == op_code::else_start) {
            throw std::runtime_error("`"s + token + "` was not expected here");
        }

        //debug_out("last_type: "s + last_type.symbol);

        if(op_type.code == op_code::map_start && last_type.category == op_category::ctrl_cond) {
            op_type = lookup_operation(op_code::block_start);
        }

        if(op_type.is_statement_level
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

            builder.append_operand(token);
            continue;
        }

        if(op_type.operand_count == 0) {
            if constexpr(debug) {
                if(op_type.category == op_category::ctrl_cond || op_type.category == op_category::ctrl_end)
                    throw std::logic_error("`"s + op_type.symbol + "` shouldn't get here");
            }

            builder.append(token, op_type);
            op_codes.push(op_type.code);

            if(op_type.code == op_code::map_start)
                op_type.code = handle_map_label(mut(i), true);
            
            continue;
        }
        

        if(pop_op_codes(token, mut(op_type), mut(i)))
            continue;
        

        if(op_type.code == op_code::comma && !op_codes.empty()) {
            op_type.code = handle_comma(op_codes.top(), mut(i));
            continue;
        }

        if(op_type.operand_count == 1 && op_type.associativity == associative::left) {
            builder.append(op_type.symbol, op_type);

            if(op_type.code == op_code::semicolon) {
                handle_ctrl_end(mut(op_type), mut(i));

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

    if(!op_codes.empty())
        throw std::logic_error("Unmatched: "s + lookup_operation(op_codes.top()).symbol);

/*        if(op_codes.size() != 1 || op_codes.top() != op_code::semicolon) {
            throw std::logic_error("Expected stack to only contain op_code::semicolon. Size: " 
                    + std::to_string(op_codes.size()) + ", code: " 
                    + std::to_string(op_codes.empty() ? 0 : static_cast<int>(op_codes.top())));
        }
        */

    return builder.finalize_bytecode();
}

