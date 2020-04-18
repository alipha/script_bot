#include "conversion.hpp"
#include "object.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <sstream>


thread_local std::stringstream ss;


std::optional<std::string> to_optional_string(const string_ref &s, std::size_t, bool format) {
    if(!format) 
        return {*s};

    std::string result("\"");
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


std::optional<std::string> to_optional_string(double v, std::size_t, bool) {
    ss.str("");
    ss.precision(15);
    ss << v;
    return ss.str();
}


std::optional<std::string> to_optional_string(const array_ref &ref, std::size_t depth, bool) {
    if(ref->empty())
        return {"[]"};
    if(depth >= 20)
        return "...";

    std::string out("[");

    for(const object &obj : *ref)
        out += obj.to_string(depth+1, true) + ", ";

    out.pop_back();
    out.back() = ']';
    return {std::move(out)};
}


std::optional<std::string> to_optional_string(const map_ref &ref, std::size_t depth, bool) {
    if(ref->empty())
        return {"{}"};
    if(depth >= 20)
        return "...";

    std::string out("{");

    for(const auto &keypair : *ref)
        out += keypair.first + ": " + keypair.second.to_string(depth+1, true) + ", ";

    out.pop_back();
    out.back() = '}';
    return {std::move(out)};
}

