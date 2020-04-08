#ifndef LIPH_IRC_HPP
#define LIPH_IRC_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>


using pos_size_pair = std::pair<std::size_t, std::size_t>;


class irc_message {
public:
    irc_message() {}
    irc_message(std::string line) : raw_line(line) { parse(); }

    const std::string &raw() const { return raw_line; }
    std::string_view sender() const { return make_sv(s); }
    std::string_view sender_nick() const { return make_sv(nick); }
    std::string_view action() const { return make_sv(act); }
    std::string_view target() const { return make_sv(t); }
    std::string_view message() const { return make_sv(msg); }

private:

    std::string_view make_sv(pos_size_pair p) const { 
        return std::string_view(raw_line.data() + p.first, p.second);
    }

    pos_size_pair tokenize(std::size_t &pos);
    void parse();

    std::string raw_line;
    pos_size_pair s;
    pos_size_pair nick;
    pos_size_pair act;
    pos_size_pair t;
    pos_size_pair msg;
};


class irc_client {
public:
    irc_client() : context(), sock(context), sock_buf(), sock_stream(&sock_buf) {}

    void login();

    void write(std::string_view message);
    void write(const std::string &message) { write(std::string_view(message.data(), message.size())); }
    void write(const char *message) { write(std::string_view(message)); }

    irc_message read();
    
private:
    boost::asio::io_service context;
    boost::asio::ip::tcp::socket sock;
    boost::asio::streambuf sock_buf;
    std::istream sock_stream;
};


#endif

