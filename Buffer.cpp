#include "Buffer.h"
#include "HTTPCore.h"

ssize_t Buffer::ReadFd(int fd, size_t size)
{
    ssize_t n;

    size = size > (size_t)(end_ - last_) ? (end_ - last_) : size;

    if ((last_ - pos_) > 0) {
        std::cout << "buffer remains " << last_ - pos_ << " bytes.\n";
        //std::cout << "what remains:\n";
        //u_char *a = pos_;
        //for (; a < last_; a++) {
        //    std::cout << *a;
        //}
        n = last_ - pos_;
        return n;
    }

    if (last_ == end_) {
        //TODO
    }

    n = ::recv(fd, last_, size, 0);

    if(n < 0){
        if (errno == EAGAIN || errno == EINTR) {
            std::cout << fd << " recv(2) not ready\n";
            return AGAIN;
        }
        return ERROR;
    } else {
        std::cout << fd << " buffer read " << n << " bytes.\n";
        last_ += n;
        std::cout << fd << " buffer empty: " << end_ - last_ << " bytes.\n";
        return n;
    }
}

ssize_t Buffer::Write(int fd)
{
    ssize_t n;

    n = ::write(fd, pos_, last_ - pos_);

    if (n < 0) {
        switch (errno) {
        case EAGAIN:
            std::cout << "write not ready.\n";
            break;

        default:
            std::cout << "write failed.\n";
            break;
        }
    }

    pos_ += n;
    return n;
}
