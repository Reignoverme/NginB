#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <stddef.h>

#define str3_cmp(m, c0, c1, c2)  \
        m[0] == c0 && m[1] == c1 && m[2] == c2

#define str4_cmp(m, c0, c1, c2, c3)  \
        m[0] == c0 && m[1] == c1 && m[2] == c2 && m[3] == c3

class Buffer;
class Request;
class Response;

int ProcessRequestLine(const boost::scoped_ptr<Buffer>&,
        const boost::shared_ptr<Request>&);

template <class T>
int ProcessRequestHeaders(const boost::scoped_ptr<Buffer>&,
        const T&);

int ParseStatusLine(const boost::scoped_ptr<Buffer>&,
        const boost::shared_ptr<Response>&);

ssize_t httpReadRequestHeader(const Buffer&);

#endif
