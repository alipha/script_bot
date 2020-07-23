#include "compiler.hpp"
#include "bytecode_builder.hpp"
#include "cleanup.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "operation_type.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"
#include "tokenizer.hpp"
#include "util.hpp"

#include <cstring>
#include <deque>
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
    compiler_impl(memory *m, bool generate_tokenized) : mem(m), gen_tokenized(generate_tokenized), last_code(op_code::none), last_type() {}

    // non-movable because the builders' parents pointers will end up pointing to the wrong parents
    compiler_impl(const compiler_impl &) = delete;
    compiler_impl(compiler_impl &&) = delete;
    compiler_impl &operator=(const compiler_impl &) = delete;
    compiler_impl &operator=(compiler_impl &&) = delete;

    std::shared_ptr<func_def> compile(std::vector<symbol> token_list, const std::string &source, bool persist_vars);
    
    const std::string &tokenized() const { return builders.front().tokenized(); }

private:
    void pop_op_code(const operation_type &top);

    op_code handle_comma(op_code top_code, mut<std::size_t> index);
    void handle_pair(const operation_type &op_type, const operation_type &top_op_type);
    op_code handle_map_label(mut<std::size_t> index, bool map_start);

    void handle_ctrl_cond(mut<operation_type> op_type_ref);
    void handle_ctrl_end(mut<operation_type> op_type_ref, mut<std::size_t> index);
    void handle_func_lit(mut<std::size_t> index);
    void handle_func_end(mut<operation_type> op_type_ref, std::size_t index);
    
    bool pop_op_codes(std::string_view token, mut<operation_type> op_type_ref, mut<std::size_t> index);


    void reset() {
        op_codes = {};
        last_code = op_code::none;
        last_type = {};
        tokens.clear();
        source_text.clear();
        builders.clear();
    }

    memory *mem;
    bool gen_tokenized;
    std::stack<op_code> op_codes;
    op_code last_code;
    operation_type last_type;
    std::vector<symbol> tokens;
    std::string source_text;
    std::deque<bytecode_builder> builders;
};



compiler::compiler(memory *m, bool generate_tokenized) 
    : impl(std::make_unique<compiler_impl>(m, generate_tokenized)) {}

compiler::~compiler() {}

std::shared_ptr<func_def> compiler::compile(std::vector<symbol> token_list, const std::string &source, bool persist_vars) {
    return impl->compile(std::move(token_list), source, persist_vars);
}

const std::string &compiler::tokenized() const { return impl->tokenized(); }



void compiler_impl::pop_op_code(const operation_type &top) {
    op_codes.pop();
    if(top.operand_count != 0)
        builders.front().append(top.symbol, top);
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

    builders.front().append(op_type.symbol, op_type);

    if(op_type.code == op_code::map_add)
        return handle_map_label(index, false);
    return op_type.code;
}


void compiler_impl::handle_pair(const operation_type &op_type, const operation_type &top_op_type) {
    op_code code = op_type.code;

    if(top_op_type.primary_right_pair != code && top_op_type.other_right_pair != code)
        throw std::runtime_error("Mismatched "s + top_op_type.symbol);

    if(top_op_type.code == op_code::func_call && last_code != op_code::func_call)
        builders.front().append(",", lookup_operation(op_code::param_add));

    pop_op_code(top_op_type);
    code = top_op_type.primary_right_pair;

    builders.front().append(op_type.symbol, lookup_operation(code));
}


op_code compiler_impl::handle_map_label(mut<std::size_t> index, bool start_map) {
	std::size_t &i = index.get();

    std::string_view token = tokens[++i].token;                             
    if(start_map && token == "}") {
        op_codes.pop();
        return op_code::map_end;
    }

    operation_type op_type = lookup_operation(token, false);

    if(op_type.code != op_code::none)
        throw std::runtime_error("`"s + token + "` is not a valid map label");

    if(token[0] == '"' || token[0] == '\'') {
        builders.front().append(token, op_code::str_lit);
    } else {
        std::string t = "\""s + token + "\"";
        builders.front().append({t.data(), t.size()}, op_code::str_lit);
    }

    token = tokens[++i].token;
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

        if(top_op_type.code == op_code::if_end && tokens[i + 1].token == "else") {
            op_codes.top() = op_code::else_end;
            op_type = lookup_operation(tokens[++i].token, false);
            builders.front().append(op_type.symbol, op_type);
            return;
        }

        pop_op_code(top_op_type);
        op_type = top_op_type;
    }
}


void compiler_impl::handle_func_lit(mut<std::size_t> index) {
    auto &i = index.get();
    std::vector<std::string_view> params;
    std::size_t start_pos = tokens[i].pos;

    std::string_view token = tokens[++i].token;
    if(token != "(")
        throw std::runtime_error("Expected ( after fn. Got: "s + token);

    token = tokens[++i].token;
    //builders.front().append(token, op_code::none); // TODO: valid?
    if(tokenizer::is_identifier(token[0]))
        params.push_back(token);
    else if(token != ")")
        throw std::runtime_error("Unexpected token "s + token + " in parameter list");

    if(token != ")") {
        while((token = tokens[++i].token) == ",") {
            // TODO: valid? for tokenized_result
            //builders.front().append(",", op_code::none);
            token = tokens[++i].token;
            //builders.front().append(token, op_code::none); // TODO: valid?
            if(tokenizer::is_identifier(token[0]))
                params.push_back(token);
            else
                throw std::runtime_error("Invalid parameter name: "s + token); 
        }
    }

    if(token != ")")
        throw std::runtime_error("Unexpected token "s + token + " in parameter list. Expected , or )");

    if(tokens[++i].token != "{")
        throw std::runtime_error("Expected { after fn parameter list. Got: "s + token);

    builders.front().append("{", lookup_operation(op_code::func_start), op_code::func_start);
    op_codes.push(op_code::func_start);

    builders.emplace_front(mem, start_pos, &builders, &op_codes, gen_tokenized, params);
}


void compiler_impl::handle_func_end(mut<operation_type> op_type_ref, std::size_t index) {
    auto &op_type = op_type_ref.get();
    op_type.code = op_code::func_lit;

    gcstring src_text(source_text.begin() + builders.front().source_start_pos(),
            source_text.begin() + tokens[index + 1].pos);

    std::shared_ptr<func_def> func = builders.front().finalize_bytecode(src_text);
    std::string tokenized = builders.front().tokenized();
    builders.pop_front();
    builders.front().append_operand(func, tokenized);
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
            } else if(top_op_type.code == op_code::func_start) {
                handle_func_end(op_type_ref, index.get());
            }

            return true;
        }

        pop_op_code(top_op_type);
    }

    if(op_type.has_left_pair)
        throw std::runtime_error("Mismatched "s + token);

    return false;
}


std::shared_ptr<func_def> compiler_impl::compile(std::vector<symbol> token_list, const std::string &source, bool persist_vars) {
    cleanup c = [this]() { this->reset(); };
    builders.emplace_front(mem, 0, &builders, &op_codes, gen_tokenized, std::vector<std::string_view>());

    source_text = source + ';';
    tokens = std::move(token_list);
    tokens.push_back({";", source_text.size() - 1});

    bool in_binary_context = false;
    operation_type op_type = lookup_operation(op_code::semicolon);

    for(std::size_t i = 0; i < tokens.size(); ++i) {
        std::string_view token = tokens[i].token;
        
        last_code = op_type.code; 
        last_type = lookup_operation(last_code);
        in_binary_context = last_type.is_binary_next;

        if(token == ";" && (last_type.category == op_category::ctrl_cond || last_type.category == op_category::ctrl_end || last_code == op_code::block_start || last_code == op_code::block_end || last_code == op_code::semicolon)) {
            op_type = lookup_operation(op_code::semicolon);
        } else {
            op_type = lookup_operation(token, in_binary_context, last_type);
            if(last_type.code == op_code::array_start && is_empty_pair(last_type, op_type)) {
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
                && last_code != op_code::func_start 
                && last_type.category != op_category::ctrl_cond && last_type.category != op_category::ctrl_end) {
            throw std::runtime_error(token + " should be at the start of a statement"s);
        }

        if(op_type.category == op_category::ctrl_start && tokens[i + 1].token != "(") {
            throw std::runtime_error("( should follow "s + token);
        }
      
        if(op_type.code == op_code::none && !token.empty()) {
            if(in_binary_context)
                throw std::runtime_error("`"s + token + "` was not expected at this point.");

            bool is_global = (persist_vars && builders.size() == 1) 
                || mem->has_global(std::string(token));
            builders.front().append_operand(token, is_global);
            continue;
        }

        if(op_type.code == op_code::func_lit) {
            handle_func_lit(mut(i));
            op_type.code = op_code::func_start;
            continue;
        }

        if(op_type.operand_count == 0) {
            if constexpr(debug) {
                if(op_type.category == op_category::ctrl_cond || op_type.category == op_category::ctrl_end)
                    throw std::logic_error("`"s + op_type.symbol + "` shouldn't get here");
            }

            builders.front().append(token, op_type);
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
            builders.front().append(op_type.symbol, op_type);

            if(op_type.code == op_code::semicolon) {
                handle_ctrl_end(mut(op_type), mut(i));

                if(!op_codes.empty() && op_codes.top() != op_code::block_start 
                        && op_codes.top() != op_code::func_start
                        && lookup_operation(op_codes.top()).category != op_category::ctrl_end)
                    throw std::runtime_error(token + " should be at the statement level"s);
                //if(last_code == op_code::semicolon)
                    //continue;
            }
        } else {
            op_codes.push(op_type.code);
        }

        if(op_type.category == op_category::ctrl_start && tokens[i + 1].token != "(")
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

    return builders.front().finalize_bytecode(source_text);
}

