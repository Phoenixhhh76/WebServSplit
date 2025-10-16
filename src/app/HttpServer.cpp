// comments in English only inside code.
#include "HttpServer.hpp"
#include <sys/socket.h>
#include <cerrno>
#include <cstdio>

bool HttpServer::start_listeners(std::vector<Listener*>& ls){
    if (!_reactor.open()) { std::perror("epoll_create1"); return false; }
    for (size_t i=0;i<ls.size();++i){
        if (!ls[i] || !ls[i]->fd.valid()) return false;
        _fd2listener[ls[i]->fd.get()] = ls[i];
        if (!_reactor.add(ls[i]->fd.get(), EPOLLIN)) { std::perror("epoll_ctl ADD"); return false; }
    }
    return true;
}

void HttpServer::loop(const Config& cfg){
    std::vector<struct epoll_event> evs(1024);
    while(true){
        int n = _reactor.wait(&evs[0], (int)evs.size(), -1);
        if (n < 0){
            if (errno == EINTR) continue;
            std::perror("epoll_wait");
            break;
        }
        for (int i=0;i<n;++i){
            int fd = evs[i].data.fd;
            uint32_t ee = evs[i].events;

            // Accept phase
            if (_fd2listener.count(fd)){
                for(;;){
                    sockaddr_in cli; socklen_t cl=sizeof(cli);
                    int cfd = ::accept(fd, (sockaddr*)&cli, &cl);
                    if (cfd < 0){
                        // Per subject: do not inspect errno after read/write; stop for now.
                        break;
                    }
                    Listener::set_nonblock(cfd);
                    Connection cx; cx.attach(cfd, fd);
                    _conns[cfd] = cx;
                    _reactor.add(cfd, EPOLLIN);
                }
                continue;
            }

            // Connection I/O
            Connection &cx = _conns[fd];
            if (ee & EPOLLIN)  cx.on_readable(cfg, _fd2listener);
            if (ee & EPOLLOUT) cx.on_writable();

            if (cx.closed()){
                _reactor.del(fd);
                ::close(fd);
                _conns.erase(fd);
            } else {
                uint32_t want = EPOLLIN | (cx.want_write()? EPOLLOUT : 0);
                _reactor.mod(fd, want);
            }
        }
    }
}

