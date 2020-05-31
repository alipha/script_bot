#ifndef LIPH_BYTECODE_BUILDER_HPP
#define LIPH_BYTECODE_BUILDER_HPP

#include "object_fwd.hpp"
#include "operation_type.hpp"

#include <cstddef>
#include <deque>
#include <memory>
#include <stack>
#include <string>
#include <string_view>
#include <vector>


class builder_impl;
class memory;


class bytecode_builder {
public:
    bytecode_builder(memory *m, std::size_t src_start_index, std::deque<bytecode_builder> *builders, std::stack<op_code> *op_codes, bool generate_tokenized, const std::vector<std::string_view> &params);
    ~bytecode_builder();
   
    bytecode_builder(const bytecode_builder &) = delete;
    bytecode_builder(bytecode_builder &&) = delete;
    bytecode_builder &operator=(const bytecode_builder &) = delete;
    bytecode_builder &operator=(bytecode_builder &&) = delete;

    void append(std::string_view token, op_code code);
    void append(std::string_view token, const operation_type &op_type);
    void append(std::string_view token, const operation_type &op_type, op_code code);
    void append(std::string_view token, const operation_type &op_type, op_code code, std::string_view func_tokens);
    void append_operand(std::string_view token);
    void append_operand(const std::vector<char> &bytecode, const std::string &func_tokens);

    void reset(const std::vector<std::string_view> &params);
    
    const std::string &tokenized() const;

    std::size_t source_start_pos() const;
    
    std::shared_ptr<func_def> finalize_bytecode(std::string_view source_text);
    
private:
    friend class builder_impl;
    
    std::unique_ptr<builder_impl> impl;
};


#endif
