#ifndef CONNECTION_HPP
#define CONNECTION_HPP
#include "config/Config.hpp"
#include "net/Listener.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/Router.hpp"
#include "http/StaticHandler.hpp"

#include <sys/socket.h>
#include <map>
#include <string>

class Connection {
public:
    enum State { RECV_HEADERS, SEND_RESPONSE, CLOSED };
private:
    int         _fd;
    int         _listen_fd;
    State       _st;
    std::string _in, _out;

public:
    Connection(): _fd(-1), _listen_fd(-1), _st(RECV_HEADERS) {}
    void attach(int cfd, int lfd);
    int  fd() const { return _fd; }
    bool closed() const { return _st==CLOSED; }
    bool want_write() const { return _st==SEND_RESPONSE && !_out.empty(); }

    void on_readable(const Config& cfg,
                     const std::map<int,Listener*>& fd2listener); // recv + parse + route
    void on_writable();                                          // send outbuf
};
#endif
