#ifndef BUFFER_H
#define BUFFER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <cerrno>

class Buffer
{
public:
    Buffer(size_t size)
        : buf_(new u_char[size]),
          start_(buf_),
          end_(buf_+size),
          last_(buf_),
          pos_(buf_)
    {}

    ~Buffer(){ delete [] buf_; }

    u_char* Start() const  { return start_; }
    u_char* End() const { return end_; }
    u_char* Last() const { return last_; }
    u_char* Pos() const { return pos_; }

    void SetPos(u_char* pos) { pos_ = pos; }

    ssize_t ReadFd(int fd, size_t size);

private:
    u_char* buf_;
    u_char* start_;
    u_char* end_;
    u_char* last_;
    u_char* pos_;    //current position

};

#endif
