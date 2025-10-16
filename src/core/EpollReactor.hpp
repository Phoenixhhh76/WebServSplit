#ifndef EPOLL_REACTOR_HPP
#define EPOLL_REACTOR_HPP
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>

class EpollReactor {
    int _epfd;
    EpollReactor(const EpollReactor&); EpollReactor& operator=(const EpollReactor&);
public:
    EpollReactor(): _epfd(-1) {}
    bool open();                       // epoll_create1
    bool add(int fd, uint32_t ev);     // EPOLL_CTL_ADD
    bool mod(int fd, uint32_t ev);     // EPOLL_CTL_MOD
    bool del(int fd);                  // EPOLL_CTL_DEL
    int  wait(struct epoll_event* out, int max, int timeout_ms);
    int  fd() const { return _epfd; }
};

#endif