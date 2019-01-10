#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <boost/shared_ptr.hpp>
#include <stddef.h>

#define OK         0
#define ERROR     -1
#define AGAIN     -2
#define DONE      -3
#define DECLINED  -4
#define ABORT     -5

#define str3_cmp(m, c0, c1, c2)  \
        m[0] == c0 && m[1] == c1 && m[2] == c2

#define str4_cmp(m, c0, c1, c2, c3)  \
        m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3

class Buffer;
class Request;

int processRequestLine(Buffer&, const boost::shared_ptr<Request>&);

ssize_t httpReadRequestHeader(const Buffer&);

#endif
