#include "Buffer.h"

ssize_t Buffer::ReadFd(int fd, size_t size)
{
    ssize_t n;

    n = ::recv(fd, last_, size, 0);

    if(n <= 0){
        return n;
    } else {
        std::cout << "read " << n << " bytes.\n";
        last_ += n;
        return n;
    }
}
