#include "Buffer.h"
#include "HTTPCore.h"

ssize_t Buffer::ReadFd(int fd, size_t size)
{
    ssize_t n;

    if ((last_ - pos_) > 0) {
        std::cout << "buffer remains " << last_ - pos_ << " bytes.\n";
        return last_ - pos_;
    }

    n = ::recv(fd, last_, size, 0);

    if(n < 0){
        if (errno == EAGAIN || errno == EINTR) {
            std::cout << "recv(2) not ready\n";
            return AGAIN;
        }
        return ERROR;
    } else {
        std::cout << "buffer read " << n << " bytes.\n";
        last_ += n;
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
