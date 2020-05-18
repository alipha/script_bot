#include "irc.hpp"
#include "settings.hpp"
#include "string_util.hpp"

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <boost/asio.hpp>


using boost::asio::ip::tcp;
using namespace std::string_literals;



class irc_client_impl {
public:
    irc_client_impl(settings &s) 
        : setting(s), context(), sock(context), sock_buf(), sock_stream(&sock_buf) {}

    void login();
    void write(std::string_view message);

    irc_message read();
    
private:
    void auth();
    bool is_admin(std::string_view nickname) const;
    
    std::string bot_nick() const { return std::string(setting.first("nick").value_or("LiphBot")); }

    settings &setting;
    boost::asio::io_service context;
    boost::asio::ip::tcp::socket sock;
    boost::asio::streambuf sock_buf;
    std::istream sock_stream;
};



pos_size_pair irc_message::tokenize(std::size_t &pos) {
    std::size_t start = pos;

    if(start >= raw_line.size())
        return {raw_line.size(), 0};

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


irc_client::irc_client(settings &setting) 
    : impl(std::make_unique<irc_client_impl>(setting)) {}

irc_client::~irc_client() {}

void irc_client::login() { impl->login(); }

void irc_client::write(std::string_view message) { impl->write(message); }

irc_message irc_client::read() { return impl->read(); }



void irc_client_impl::login() {
    tcp::resolver resolver(context);
    tcp::resolver::query query(
            std::string(setting.first("server").value_or("irc.freenode.net")),
            std::string(setting.first("port")  .value_or("6667")), 
            tcp::resolver::query::numeric_service);
    
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

    auth();

    while(read().action() != "376") {}  // read until end of MOTD

    if(setting.is_set("on_connect")) {
        for(const std::string &line : setting.all("on_connect"))
            write(line);
    } else {
        write("JOIN #aliphaBot");
    }
}


void irc_client_impl::write(std::string_view message) {
    if(message.size() > 425)
        message = message.substr(0, 425);
    std::cout << '>' << message << std::endl;
    boost::asio::write(sock, boost::asio::buffer(message.data(), message.size()));
    boost::asio::write(sock, boost::asio::buffer("\r\n", 2));
}


irc_message irc_client_impl::read() {    
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


    msg.set_is_admin_msg(
            to_lower(msg.target()) == to_lower(bot_nick())
            && is_admin(msg.sender_nick())
    );

    return msg;
}
    

void irc_client_impl::auth() {
    bool sent_nick = false;
    bool sent_user = false;
    for(const std::string &line : setting.all("auth")) {
        write(line);
        if(starts_with(to_lower(line), "nick "))
            sent_nick = true;
        if(starts_with(to_lower(line), "user "))
            sent_user = true;
    }

    if(!sent_nick)
        write("NICK " + bot_nick());
    if(!sent_user)
        write("USER " + bot_nick() + " * * :AliphaBot 2.0");
}


bool irc_client_impl::is_admin(std::string_view nickname) const {
    std::string nick = to_lower(nickname);

    if(auto &&admins = setting.all("admin"); !admins.empty()) {
        for(const std::string &admin : admins)
            if(to_lower(admin) == nick)
                return true;
        return false;
    } else {
        return nick == "alipha";
    }
}

