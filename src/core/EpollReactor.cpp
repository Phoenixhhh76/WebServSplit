#include "EpollReactor.hpp"

bool EpollReactor::open(){ _epfd = epoll_create1(0); return _epfd>=0; }

bool EpollReactor::add(int fd, uint32_t ev){
    struct epoll_event e; std::memset(&e,0,sizeof(e));
    e.events=ev; e.data.fd=fd;
    return epoll_ctl(_epfd,EPOLL_CTL_ADD,fd,&e)==0;
}

bool EpollReactor::mod(int fd, uint32_t ev){
    struct epoll_event e; std::memset(&e,0,sizeof(e));
    e.events=ev; e.data.fd=fd;
    return epoll_ctl(_epfd,EPOLL_CTL_MOD,fd,&e)==0;
}

bool EpollReactor::del(int fd){
    return epoll_ctl(_epfd,EPOLL_CTL_DEL,fd,0)==0;
}

int EpollReactor::wait(struct epoll_event* out, int max, int timeout_ms){
    return epoll_wait(_epfd,out,max,timeout_ms);
}
