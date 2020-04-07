#include "irc.hpp"
#include "util.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <boost/asio.hpp>


using boost::asio::ip::tcp;
using namespace std::string_literals;


pos_size_pair irc_message::tokenize(std::size_t &pos) {
    std::size_t start = pos;

    if(start >= raw_line.size())
        return {raw_line.size(), 0};

    /*
    if(raw_line[start] == ':') {
        pos = raw_line.size();
        return {start + 1, raw_line.size() - start - 1};
    }
    */

    std::size_t space = raw_line.find(' ', start);

    if(space == std::string::npos) {
        pos = raw_line.size();
        return {start, raw_line.size() - start};
    }

    pos = space + 1;
    return {start, space - start};
}


void irc_message::parse() {
    s = act = t = msg = pos_size_pair(0, 0);
    if(raw_line.empty())
        return;

    std::size_t pos = 0;
    if(raw_line[0] == ':') {
        s = tokenize(pos = 1);

        std::string_view send = sender();
        std::size_t bang = send.find('!');
        if(bang == std::string_view::npos) {
            nick = {1, send.size()};
        } else {
            nick = {1, bang};
        }
    }
    
    act = tokenize(pos);
    
    if(pos < raw_line.size() && raw_line[pos] != ':')
        t = tokenize(pos);

    if(pos < raw_line.size() && raw_line[pos] == ':')
        pos++;

    msg = {pos, raw_line.size() - pos};
}


void irc_client::login() {
    tcp::resolver resolver(context);
    tcp::resolver::query query("irc.freenode.net", "6667", tcp::resolver::query::numeric_service);
    
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    // Try each endpoint until we successfully establish a connection.
    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
        sock.close();
        sock.connect(*endpoint_iterator++, error);
    }

    if (error)
        throw boost::system::system_error(error);

    write("NICK LiphBot");
    write("USER LiphBot * * :AliphaBot 2.0");

    while(read().action() != "376") {}  // read until end of MOTD

    write("JOIN #aliphaBot");
}


void irc_client::write(std::string_view message) {
    std::cout << '>' << message << std::endl;
    boost::asio::write(sock, boost::asio::buffer(message.data(), message.size()));
    boost::asio::write(sock, boost::asio::buffer("\r\n", 2));
}


irc_message irc_client::read() {    
    irc_message msg;
    std::string line;

    do {
        boost::asio::read_until(sock, sock_buf, "\r\n");
        std::getline(sock_stream, line);

        if(line.empty() || line.front() == '\r')
            continue;

        if(line.back() == '\r')
            line.pop_back();

        std::cout << line << std::endl;
        msg = irc_message(std::move(line));

        if(msg.action() == "PING")
            write("PONG :"s + msg.target() + msg.message());
    } while(msg.action() == "PING");

    return msg;
}

