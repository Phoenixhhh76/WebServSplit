#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include "core/EpollReactor.hpp"
#include "net/Listener.hpp"
#include "net/Connection.hpp"
#include "config/Parser.hpp"

#include <map>
#include <vector>
#include <iostream>

class HttpServer {
    EpollReactor _reactor;
    std::map<int, Listener*> _fd2listener;
    std::map<int, Connection> _conns;

public:
    // NOTE: vector of pointers to avoid copying Listener (Fd is non-copyable)
    bool start_listeners(std::vector<Listener*>& ls);
    void loop(const Config& cfg);
};

#endif // HTTP_SERVER_HPP
