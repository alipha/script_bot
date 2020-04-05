#ifndef LIPH_OPERAND_HPP
#define LIPH_OPERAND_HPP


struct operand {

};



struct operation {

};


enum associative {
    left,
    right
};


struct operation_type {
    std::string_view symbol;
    int operand_count;
    associative associativity;
    int precedence;
};


struct operation {

};


struct statement {
    std::vector<char> buffer;
};



std::map<std::pair<std::string_view, int>, operation_type> load_operation_types() {
    operation_type types[] = {
        {"+", 2, associative.left, 100},

    };

    std::map<std::pair<std::string_view, int>, operation_type> type_map;

    for(auto &t : types) {
        type_map[std::pair(t.symbol, t.operand_count)] = t;
    }

    return type_map;
}


std::map<std::pair<std::string_view, int>, operation_type> symbol_operation_map = load_operation_types();


operation_type lookup_operation(std::string_view symbol, int operand_count) {
    return symbol_operation_map.at(std::pair(symbol, operand_count));
}

#endif

