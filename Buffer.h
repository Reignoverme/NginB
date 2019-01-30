#ifndef BUFFER_H
#define BUFFER_H
#include <cerrno>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>

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

    Buffer(const Buffer& rhl) {
        size_t size = rhl.End() - rhl.Start();
        buf_ = new u_char[size];
        start_ = buf_;
        end_  = buf_ + size;
        last_ = buf_ + (rhl.Last() - rhl.Start());
        pos_ = buf_ + (rhl.Pos() - rhl.Start());

        memcpy(start_, rhl.Start(), size);
    }

    ~Buffer(){ delete [] buf_; }

    u_char* Start() const  { return start_; }
    u_char* End() const { return end_; }
    u_char* Last() const { return last_; }
    u_char* Pos() const { return pos_; }

    size_t  Size() const { return last_ - start_; }

    void SetPos(u_char* pos) { pos_ = pos; }
    void SetLast(u_char* last) { last_ = last; }
    void Reset() {
        pos_ = start_;
        last_ = start_;
    }

    ssize_t ReadFd(int fd, size_t size);
    ssize_t Write(int fd);

private:
    u_char* buf_;
    u_char* start_;
    u_char* end_;
    u_char* last_;
    u_char* pos_;    //current position

};

#endif
