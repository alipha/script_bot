#include "conversion.hpp"
#include "gc.hpp"
#include "object.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <sstream>


thread_local std::basic_stringstream<char, std::char_traits<char>, gc::allocator<char>> ss;


std::optional<gcstring> to_optional_string(const string_ref &s, std::size_t, std::size_t *, bool format) {
    if(!format) 
        return {*s};

    gcstring result("\"");
    for(char ch : *s) {
        if(ch == '\"')
            result += "\\\"";
        else if(ch == '\\')
            result += "\\\\";
        else
            result += ch;
    }
    result += "\"";
    return {std::move(result)}; 
}


std::optional<gcstring> to_optional_string(double v, std::size_t, std::size_t *, bool) {
    ss.str("");
    ss.precision(15);
    ss << v;
    return ss.str();
}


std::optional<gcstring> to_optional_string(const array_ref &ref, std::size_t depth, std::size_t *count, bool) {
    if(ref->empty())
        return {"[]"};
    if(depth >= 20)
        return "...";

    std::size_t count_value = 0;
    if(!count)
        count = &count_value;

    if(*count > 1000)
        return "...";

    gcstring out("[");

    for(const object &obj : *ref)
        out += obj.to_string(depth+1, &++*count, true) + ", ";

    out.pop_back();
    out.back() = ']';
    return {std::move(out)};
}


std::optional<gcstring> to_optional_string(const map_ref &ref, std::size_t depth, std::size_t *count, bool) {
    if(ref->empty())
        return {"{}"};
    if(depth >= 20)
        return "...";

    std::size_t count_value = 0;
    if(!count)
        count = &count_value;

    if(*count > 1000)
        return "...";
                  
    gcstring out("{");

    for(const auto &keypair : *ref)
        out += keypair.first + ": " + keypair.second.to_string(depth+1, &++*count, true) + ", ";

    out.pop_back();
    out.back() = '}';
    return {std::move(out)};
}

