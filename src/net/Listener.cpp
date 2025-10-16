#include "Listener.hpp"

int Listener::set_nonblock(int fd){
    int flags = fcntl(fd,F_GETFL,0);
    if (flags<0) return -1;
    return fcntl(fd,F_SETFL,flags|O_NONBLOCK);
}

bool Listener::open_and_listen(const std::string& h, int p){
    host=h; port=p;
    int ls = ::socket(AF_INET, SOCK_STREAM, 0);
    if (ls < 0) return false;
    int yes=1; ::setsockopt(ls,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    sockaddr_in addr; std::memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) { ::close(ls); return false; }
    addr.sin_port = htons(port);
    if (::bind(ls,(sockaddr*)&addr,sizeof(addr))<0){ ::close(ls); return false; }
    if (::listen(ls,128)<0){ ::close(ls); return false; }
    if (set_nonblock(ls)<0){ ::close(ls); return false; }
    fd.reset(ls);
    return true;
}
