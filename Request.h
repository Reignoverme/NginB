#ifndef REQUEST_H
#define REQUEST_H
#include <boost/core/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <map>
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
    typedef std::map<std::string, std::string> Header;

    Request(boost::shared_ptr<Connection> c)
        :connection_(c),
         state_(0)
    {}

    uint8_t State() { return state_; }
    uint32_t Method() { return method_; }
    std::string URI() { return uri_; }
    uint32_t HTTPVersion() { return httpVersion_; } 

    void SetMethod(uint32_t method) { method_ = method; }
    void SetState(uint8_t state) { state_ = state; }
    void SetHTTPVersion(uint32_t version) { httpVersion_ = version; }
    void SetUri(const u_char*, const u_char*);
    void SetHeaders(const u_char*, const u_char*, const u_char*, const u_char*);

    Header Headers() { return headers_; }

private:

    boost::weak_ptr<Connection> connection_;

    uint8_t state_;    // parsing state

    uint32_t method_;
    std::string uri_;
    uint32_t httpVersion_;
    std::string requestLine_;
    Header headers_;
};


#endif
