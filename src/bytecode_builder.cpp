#include "bytecode_builder.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "memory_buffer.hpp"
#include "object_fwd.hpp"
#include "operation_type.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"
#include "tokenizer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::string_literals;

class memory;


constexpr std::uint8_t capture_index_start = 128;


struct capture_mapping {
    std::uint8_t parent_index;
    std::uint8_t my_index;
};


class builder_impl {
public:
    builder_impl(memory *m, std::size_t src_start_index, std::deque<bytecode_builder> *builders, std::stack<op_code> *op_codes_ptr, bool generate_tokenized, const std::vector<std::string_view> &params) 
        : mem(m),
          src_start_pos(src_start_index),
          parents(builders), 
          op_codes(op_codes_ptr), 
          gen_tokenized(generate_tokenized)
    { reset(params); }
    
    // non-movable because the parents pointer will probably end up pointing to the wrong one
    builder_impl(const builder_impl &) = delete;
    builder_impl(builder_impl &&) = delete;
    builder_impl &operator=(const builder_impl &) = delete;
    builder_impl &operator=(builder_impl &&) = delete;

    void append(std::string_view token, op_code code) {
        append(token, lookup_operation(op_code::none), code); 
    }
    void append(std::string_view token, const operation_type &op_type) { 
        append(token, op_type, op_type.code);
    }
    void append(std::string_view token, const operation_type &op_type, op_code code);

    void append_operand(std::string_view token);
    void append_operand(std::shared_ptr<func_def> func, const std::string &func_tokens);

    void reset(const std::vector<std::string_view> &params);
    
    const std::string &tokenized() const { return tokenized_result; }

    std::size_t source_start_pos() const { return src_start_pos; }

    std::shared_ptr<func_def> finalize_bytecode(std::string_view source_text);

private:
    static std::string parse_str_literal(std::string_view str);

    std::uint8_t add_capture(std::string_view name, std::uint8_t parent_index);
    std::uint8_t get_or_add_index(std::string_view name);
    std::optional<std::uint8_t> get_my_index(std::string_view name) const;


    memory *mem;
    std::size_t src_start_pos;
    std::deque<bytecode_builder> *parents;
    std::stack<op_code> *op_codes;
    bool gen_tokenized;

    gcvector<std::shared_ptr<func_def>> func_lits;
    std::unordered_map<std::string_view, std::uint8_t> local_var_indexes;
    std::unordered_map<std::string_view, capture_mapping> capture_indexes;

    std::stack<std::size_t> jump_indexes;
    std::stack<std::size_t> while_indexes;

    memory_buffer<debug> result;
    std::string tokenized_result;
};



bytecode_builder::bytecode_builder(memory *m, std::size_t src_start_index, std::deque<bytecode_builder> *builders, std::stack<op_code> *op_codes, bool generate_tokenized, const std::vector<std::string_view> &params)
    : impl(std::make_unique<builder_impl>(m, src_start_index, builders, op_codes, generate_tokenized, params)) {}

bytecode_builder::~bytecode_builder() {}

void bytecode_builder::append(std::string_view token, op_code code) { 
    impl->append(token, code); 
}

void bytecode_builder::append(std::string_view token, const operation_type &op_type) { 
    impl->append(token, op_type); 
}

void bytecode_builder::append(std::string_view token, const operation_type &op_type, op_code code) {
    impl->append(token, op_type, code);
}

void bytecode_builder::append_operand(std::string_view token) {
    impl->append_operand(token);
}
    
void bytecode_builder::append_operand(std::shared_ptr<func_def> func, const std::string &func_tokens) {
    impl->append_operand(std::move(func), func_tokens); 
}

void bytecode_builder::reset(const std::vector<std::string_view> &params) { impl->reset(params); }

const std::string &bytecode_builder::tokenized() const { return impl->tokenized(); }
    
std::size_t bytecode_builder::source_start_pos() const { return impl->source_start_pos(); }

std::shared_ptr<func_def> bytecode_builder::finalize_bytecode(std::string_view source_text) { 
    return impl->finalize_bytecode(source_text); 
}



void builder_impl::append(std::string_view token, const operation_type &op_type, op_code code) {
    if(gen_tokenized) {
        tokenized_result += token;
        tokenized_result += ' ';
    }

    //debug_out("append: "s + token);

    if(!op_type.is_nop)
        result.append(code);

    switch(code) {
    case op_code::while_start:
        while_indexes.push(result.size());
        break;
    case op_code::else_start: {
        debug_out("else_start append " + std::to_string(jump_indexes.size()));
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
    case op_code::coalesce:
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
    case op_code::logic_or_end: 
    case op_code::coalesce_end: {
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
        result.append(get_or_add_index(token));
        break;
    case op_code::int_lit:
        result.append(static_cast<std::int64_t>(std::stoll(std::string(token))));  // TODO: from_chars?
        break;
    case op_code::uint_lit:
        result.append(static_cast<std::uint64_t>(std::stoull(std::string(token))));  // TODO: from_chars?
        break;
    case op_code::float_lit:
        result.append(std::stod(std::string(token)));
        break;
    default:
        ; //throw std::logic_error(token + " is currently not supported"s);
    }

    if(op_type.replace_with != op_code::none)
        op_codes->push(op_type.replace_with);
}


void builder_impl::append_operand(std::string_view token) {
    if(token == "null")
        append(token, op_code::null_lit);
    else if(tokenizer::is_identifier(token[0]))
        append(token, op_code::local_var);    // TODO: global_var
    else if(token[0] == '"' || token[0] == '\'') // TODO: char?
        append(token, op_code::str_lit);
    else if(std::isdigit(token[0]) && (token.back() == 'u' || token.back() == 'U'))
        append(token, op_code::uint_lit);
    else if(token.find_first_not_of("0123456789") == std::string_view::npos)
        append(token, op_code::int_lit);
    else if(std::isdigit(token[0]))
        append(token, op_code::float_lit);
    else
        throw std::runtime_error("Invalid operand: "s + token);
}


void builder_impl::append_operand(std::shared_ptr<func_def> func, const std::string &func_tokens) {
    if(gen_tokenized)
        tokenized_result += func_tokens + ' ';
   
    result.append(op_code::func_lit);
    result.append(static_cast<std::uint8_t>(func_lits.size()));
    func_lits.push_back(std::move(func));
}


// TODO: remove and put the appends in the ctor?
void builder_impl::reset(const std::vector<std::string_view> &params) {
    func_lits.clear();
    local_var_indexes.clear();
    capture_indexes.clear();
    jump_indexes = {};
    while_indexes = {};
    result.clear();
    tokenized_result.clear();

    result.append(static_cast<std::uint8_t>(params.size()));
    result.append(static_cast<std::uint8_t>(0));
    result.append(static_cast<std::uint8_t>(0));

    std::uint8_t index = 0;
    for(std::string_view param : params)
        local_var_indexes[param] = ++index;
}


std::shared_ptr<func_def> builder_impl::finalize_bytecode(std::string_view source_text) {
    std::size_t local_var_count = local_var_indexes.size();
    std::size_t capture_count = capture_indexes.size();

    if(local_var_count >= 127)
        throw std::runtime_error("Too many local variables: " + std::to_string(local_var_count));
    if(capture_count >= 127)
        throw std::runtime_error("Too many captures: " + std::to_string(capture_count));
    
    std::size_t capture_start = result.size();
    result.extend(capture_indexes.size());

    for(auto &capture : capture_indexes)
        result.patch(capture_start + capture.second.my_index - capture_index_start, capture.second.parent_index);

    result.patch(1, static_cast<std::uint8_t>(local_var_count));
    result.patch(2, static_cast<std::uint8_t>(capture_count));
    return std::make_shared<func_def>(std::move(result), std::move(func_lits), gcstring(source_text.begin(), source_text.end()));
}


std::string builder_impl::parse_str_literal(std::string_view str) {
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


std::uint8_t builder_impl::add_capture(std::string_view name, std::uint8_t parent_index) {
    std::uint8_t next_index = capture_indexes.size() + capture_index_start;
    capture_indexes[name] = capture_mapping{parent_index, next_index};
    return next_index;
}


std::uint8_t builder_impl::get_or_add_index(std::string_view name) {

    // if this builder has the local variable or the capture already, return it
    std::optional<std::uint8_t> index = get_my_index(name);
    if(index)
        return *index;

    // find this builder in the builder list so that we're only looking at
    // builder parents and not builder children
    // (though I think *this would always be the front() builder...)
    auto this_it = std::find_if(parents->begin(), parents->end(), 
            [this](auto &parent) { return parent.impl.get() == this; });

    if constexpr(debug) {
        if(this_it == parents->end())
            throw std::logic_error("builder is not in parents!");
    }

    // find which parent (or grandparent, etc) builder has the local variable or capture
    auto it = std::find_if(std::next(this_it), parents->end(),
            [&index, name](auto &parent) { 
                index = parent.impl->get_my_index(name);
                return index.has_value();
            });

    // if the variable doesn't exist, create it
    if(it == parents->end()) {
        std::size_t next_index = local_var_indexes.size() + 1;
        return local_var_indexes[name] = next_index;
    }

    if constexpr(debug) {
        if(!index.has_value())
            throw std::logic_error("index expected to have a value");
    }

    // add the variable as a capture for each child
    while(it != this_it) {
        --it;
        index = it->impl->add_capture(name, *index);
    }

    return *index;
}
    

std::optional<std::uint8_t> builder_impl::get_my_index(std::string_view name) const {
    
    auto it = local_var_indexes.find(name);
    if(it != local_var_indexes.end())
        return it->second;

    auto capture_it = capture_indexes.find(name);
    if(capture_it != capture_indexes.end())
        return capture_it->second.my_index;

    return {};
}

