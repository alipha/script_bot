#ifndef LIPH_UTIL_HPP
#define LIPH_UTIL_HPP

#include <utility>


template<typename T>
struct mut {
    explicit mut(T &t) : ref(t) {}

    T &get() { return ref; }

private:
    T &ref;
};


template<typename Func>
struct on_scope_end {
    on_scope_end(Func func) : f(std::move(func)) {}

    on_scope_end(const on_scope_end &) = delete;
    on_scope_end(on_scope_end &&) = delete;
    on_scope_end &operator=(const on_scope_end &) = delete;
    on_scope_end &operator=(on_scope_end &&) = delete;

    ~on_scope_end() noexcept(noexcept(std::declval<Func>()())) { f(); }

private:
    Func f;
};

#endif
