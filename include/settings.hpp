#ifndef LIPH_SETTINGS_HPP
#define LIPH_SETTINGS_HPP

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


class settings {
public:
    settings(const std::string &filename);

    std::optional<std::string_view> first(const std::string &key) const;
    const std::vector<std::string> &all(const std::string &key) const;
    bool is_set(const std::string &key) const;

private:
    std::map<std::string, std::vector<std::string>> data;
};

#endif
