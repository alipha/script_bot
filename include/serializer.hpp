#ifndef LIPH_SERIALIZER_HPP
#define LIPH_SERIALIZER_HPP

#include <memory>
#include <string>


class memory;
class serializer_impl;


enum class object_type : std::uint8_t {
    nul,
    int64,
    uint64,
    float8,
    string,
    array,
    map,
    func,
    count
};


class serializer {
public:
    serializer(memory *m);
    ~serializer();

    serializer(const serializer &) = delete;
    serializer(serializer &&) = delete;
    serializer &operator=(const serializer &) = delete;
    serializer &operator=(serializer &&) = delete;

    void serialize(const std::string &filename);
    void deserialize(const std::string &filename);

private:
    std::unique_ptr<serializer_impl> impl;
};


#endif
