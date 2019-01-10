#ifndef REQUEST_H
#define REQUEST_H
#include <boost/core/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <iostream>

#define HTTP_UNKNOWN                   0x0001
#define HTTP_GET                       0x0002
#define HTTP_HEAD                      0x0004
#define HTTP_POST                      0x0008

#define HTTP_PARSE_HEADER_DONE         1

#define HTTP_CLIENT_ERROR              10
#define HTTP_PARSE_INVALID_METHOD      10
#define HTTP_PARSE_INVALID_REQUEST     11
#define HTTP_PARSE_INVALID_VERSION     12
#define HTTP_PARSE_INVALID_09_METHOD   13

#define HTTP_PARSE_INVALID_HEADER      14

class Connection;
class Buffer;

class Request : boost::noncopyable
{
public:
    Request(boost::shared_ptr<Connection> c)
        :connection_(c),
         state_(0)
    {}

    uint8_t state() { return state_; }
    uint32_t method() { return method_; }
    std::string uri() { return uri_; }
    uint32_t httpVersion() { return httpVersion_; } 

    void setMethod(uint32_t method) { method_ = method; }
    void setState(uint8_t state) { state_ = state; }
    void setHTTPVersion(uint32_t version) { httpVersion_ = version; }
    void setUri(const u_char* start, const u_char* end)
    {
        size_t size = end - start;
        uri_ = std::string(reinterpret_cast<const char*>(start), size);
    }

private:
    boost::weak_ptr<Connection> connection_;

    uint8_t state_;    // parsing state

    uint32_t method_;
    std::string uri_;
    uint32_t httpVersion_;
    std::string requestLine_;
};


#endif
