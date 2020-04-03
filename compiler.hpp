#ifndef LIPH_COMPILER_HPP
#define LIPH_COMPILER_HPP

#include "operation_type.hpp"
#include "util.hpp"

#include <stack>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>


using token = std::variant<std::string, op_code>;


bool has_lower_precedence(op_code current_code, op_code top_code) {
    operation_type current = lookup_operation(current_code);
    operation_type top = lookup_operation(top_code);

    return current.precedence < top.precedence 
        || (current.precedence == top.precedence && top.associativity == associative::left);
}


std::string to_postfix(std::vector<std::string_view> token_list) {
    std::stack<op_code> op_codes;
    bool in_binary_context = false;
    std::string result;
    auto tokens = std::move(token_list);
    tokens.push_back("");

    for(auto token : tokens) {
        operation_type op_type = lookup_operation(token, in_binary_context);

        if(token == "(") {

        }
        
        if(op_type.code == op_code::none && !token.empty()) {
            if(in_binary_context)
                throw std::runtime_error(std::string("`") + token + "` was not expected at this point.");

            // TODO: number, text, or variable?
            result += token;
            result += " ";
            in_binary_context = !in_binary_context;
            continue;
        }

        if(op_type.operand_count == 2)
            in_binary_context = !in_binary_context;

        op_code top_code;
        while(!op_codes.empty() && has_lower_precedence(op_type.code, top_code = op_codes.top())) {
            result += lookup_operation(top_code).symbol;
            result += " ";
            op_codes.pop();
        }

        op_codes.push(op_type.code);
    }

    if(op_codes.size() != 1 || op_codes.top() != op_code::none) {
        throw std::logic_error("Expected stack to only contain op_code::none. Size: " 
                + std::to_string(op_codes.size()) + ", code: " + std::to_string(static_cast<int>(op_codes.top())));
    }
    /*
    while(!op_codes.empty()) {
        result += lookup_operation(op_codes.top()).symbol;
        result += " ";
        op_codes.pop();
    }
    */

    return result;
}


std::vector<char> compile(const std::vector<std::string_view> &) {//tokens) {
    
    return {};
}

#endif

