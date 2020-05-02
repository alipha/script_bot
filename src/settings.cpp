#include "settings.hpp"
#include "string_util.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>


settings::settings(const std::string &filename) {
    std::ifstream file(filename);
    if(!file) {
        std::cerr << "Using default settings because unable to open settings file: " << filename << std::endl;
        return;
    }

    std::string line;
    while(std::getline(file, line)) {
        line = trim(line);
        if(line.empty() || line[0] == '#')
            continue;

        std::size_t pos = line.find('=');
        if(pos == std::string::npos)
            throw std::runtime_error("Expected = in settings file line: " + line);

        std::string key = to_lower(trim(line.substr(0, pos)));
        std::string value = trim(line.substr(pos + 1));

        if(key.empty())
            throw std::runtime_error("Invalid blank key on line: " + line);

        std::vector<std::string> &values = data[key];  // create the vector even if we don't push to it
        if(!value.empty())
            values.push_back(value);
    }
}


std::optional<std::string_view> settings::first(const std::string &key) const {
    auto it = data.find(to_lower(trim(key)));
    if(it != data.end() && !it->second.empty())
        return it->second.front();
    else
        return {};
}


const std::vector<std::string> &settings::all(const std::string &key) const {
    static std::vector<std::string> empty_vector;

    auto it = data.find(to_lower(trim(key)));
    if(it != data.end())
        return it->second;
    else
        return empty_vector;
}


bool settings::is_set(const std::string &key) const {
    return data.find(to_lower(trim(key))) != data.end();
}

