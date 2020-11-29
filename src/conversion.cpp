#include "conversion.hpp"
#include "gc.hpp"
#include "object.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <sstream>
#include <utility>
#include <variant>


thread_local std::basic_stringstream<char, std::char_traits<char>, gc::allocator<char>> ss;


std::optional<gcstring> check_bounds(std::size_t depth, std::size_t *&count, std::size_t &count_value) {
    if(depth >= 20)
        return {"..."};

    if(!count)
        count = &count_value;

    if(*count > 1000)
        return {"..."};

    return {};
}


std::optional<gcstring> close_list(gcstring &out, char ch) {
    out.pop_back();
    out.back() = ch;
    return {std::move(out)};
}


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

    std::size_t count_value = 0;
    if(std::optional<gcstring> ret = check_bounds(depth, count, count_value))
        return ret;

    gcstring out("[");

    for(const object &obj : *ref)
        out += obj.to_string(depth+1, &++*count, true) + ", ";

    return close_list(out, ']');
}


std::optional<gcstring> to_optional_string(const map_ref &ref, std::size_t depth, std::size_t *count, bool) {
    if(ref->empty())
        return {"{}"};

    std::size_t count_value = 0;
    if(std::optional<gcstring> ret = check_bounds(depth, count, count_value))
        return ret;
                  
    gcstring out("{");

    for(const auto &[key, value] : *ref)
        out += key + ": " + value.to_string(depth+1, &++*count, true) + ", ";

    return close_list(out, '}');
}


std::optional<gcstring> to_optional_string(const class_ref &ref, std::size_t depth, std::size_t *count, bool) {
    std::size_t count_value = 0;
    if(std::optional<gcstring> ret = check_bounds(depth, count, count_value))
        return ret;

    gcstring out("class(owner=" + ref->owner + ") {");
    if(ref->attrs.empty())
        return {out + "}"};

    for(const auto &[key, value] : ref->attrs) {
        if(std::holds_alternative<func_ref>(value.get()))
            out += key + ", ";
        else
            out += key + ": " + value.to_string(depth+1, &++*count, true) + ", ";
    }

    return close_list(out, '}');
}


std::optional<gcstring> to_optional_string(const object_ref &ref, std::size_t depth, std::size_t *count, bool) {
    std::size_t count_value = 0;
    if(std::optional<gcstring> ret = check_bounds(depth, count, count_value))
        return ret;

    gcstring out("obj(owner=" + ref->owner + ") {");
    if(ref->attrs.empty())
        return {out + "}"};

    for(const auto &[key, value] : ref->attrs) {
        out += key + ": " + value.to_string(depth+1, &++*count, true) + ", ";
    }

    return close_list(out, '}');
}


std::optional<gcstring> to_optional_string(const func_ref &ref, std::size_t depth, std::size_t *count, bool) {
    std::size_t count_value = 0;
    if(std::optional<gcstring> ret = check_bounds(depth, count, count_value))
        return ret;

    gcstring out = ref->definition->source_text;

    if(!ref->captures.empty()) {
        out += " with [";

        for(const auto &capture : ref->captures) {
            out += capture->to_string(depth+1, &++*count, true) + ", ";
        }

        out.pop_back();
        out.back() = ']';
    }

    return {std::move(out)};
}

