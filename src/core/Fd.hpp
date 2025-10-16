#ifndef FD_HPP
#define FD_HPP
#include <unistd.h>

// RAII wrapper for file descriptors (C++98: non-copyable)
class Fd {
    int _fd;
    Fd(const Fd&);            // non-copyable
    Fd& operator=(const Fd&);
public:
    Fd(): _fd(-1) {}
    explicit Fd(int fd): _fd(fd) {}
    ~Fd(){ if (_fd>=0) ::close(_fd); }
    int  get() const { return _fd; }
    bool valid() const { return _fd>=0; }
    void reset(int fd=-1){ if (_fd>=0) ::close(_fd); _fd=fd; }
};
#endif
