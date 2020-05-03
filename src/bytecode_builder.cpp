#include "bytecode_builder.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "memory_buffer.hpp"
#include "operation_type.hpp"
#include "stack_util.hpp"
#include "string_util.hpp"
#include "tokenizer.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace std::string_literals;

class memory;


class builder_impl {
public:
    builder_impl(memory *m, std::stack<op_code> *op_codes_ptr, bool generate_tokenized) 
        : mem(m), op_codes(op_codes_ptr), gen_tokenized(generate_tokenized) {}
    
    void append(std::string_view token, op_code code) { append(token, lookup_operation(op_code::none), code); }
    void append(std::string_view token, const operation_type &op_type) { append(token, op_type, op_type.code); }
    void append(std::string_view token, const operation_type &op_type, op_code code);
    void append_operand(std::string_view token);

    const std::string &tokenized() const { return tokenized_result; }

    void reset();
    
    std::vector<char> finalize_bytecode();

private:
    static std::string parse_str_literal(std::string_view str);

    std::uint8_t local_var_index(std::string_view name);


    memory *mem;
    std::stack<op_code> *op_codes;
    bool gen_tokenized;
    std::unordered_map<std::string, std::uint8_t> local_var_indexes;

    std::stack<std::size_t> jump_indexes;
    std::stack<std::size_t> while_indexes;

    memory_buffer<debug> result;
    std::string tokenized_result;
};



bytecode_builder::bytecode_builder(memory *m, std::stack<op_code> *op_codes, bool generate_tokenized)
    : impl(std::make_unique<builder_impl>(m, op_codes, generate_tokenized)) {}

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

void bytecode_builder::append_operand(std::string_view token) { impl->append_operand(token); }

void bytecode_builder::reset() { impl->reset(); }

const std::string &bytecode_builder::tokenized() const { return impl->tokenized(); }

std::vector<char> bytecode_builder::finalize_bytecode() { return impl->finalize_bytecode(); }


void builder_impl::append(std::string_view token, const operation_type &op_type, op_code code) {
    if(gen_tokenized) {
        tokenized_result += token;
        tokenized_result += " ";
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


void builder_impl::reset() {
        local_var_indexes.clear();
        jump_indexes = {};
        while_indexes = {};
        result.clear();
        tokenized_result.clear();

        result.append(static_cast<std::uint8_t>(0));
        //result.append(static_cast<std::uint8_t>(0));
}


std::vector<char> builder_impl::finalize_bytecode() {
    std::size_t local_var_count = local_var_indexes.size();
    //local_var_indexes.clear();

    if(local_var_count > 255) {
        throw std::runtime_error("Too many local variables: " + std::to_string(local_var_count));
    }

    result.buffer()[0] = static_cast<char>(local_var_count);
    return std::move(result).buffer();
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


std::uint8_t builder_impl::local_var_index(std::string_view name) {
    std::size_t next_index = local_var_indexes.size() + 1;
    return local_var_indexes.emplace(name, next_index).first->second;
}

