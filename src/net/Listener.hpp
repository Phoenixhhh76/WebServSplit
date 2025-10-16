
#ifndef LISTENER_HPP
#define LISTENER_HPP
#include "core/Fd.hpp"
#include <string>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>

// Non-blocking listening socket
struct Listener {
    Fd          fd;
    std::string host;
    int         port;
    std::vector<size_t> serverIdx; // servers bound to this host:port

    static int set_nonblock(int fd);
    bool open_and_listen(const std::string& h, int p); // socket/bind/listen + O_NONBLOCK
};
#endif
