#ifndef REQUEST_H
#define REQUEST_H
#include <boost/core/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <unordered_map>
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

#define URI_FIELD 0x01
#define EXT_FIELD 0x02

class Connection;
class Buffer;

class Request : boost::noncopyable
{
public:
    typedef std::unordered_map<std::string, std::string> Header;
    typedef boost::shared_ptr<Connection> ConnectionPt;

    Request(ConnectionPt c)
        :connection_(c),
         state_(0)
    {}

    const std::string& Uri() const { return uri_; }
    const std::string& Ext() const { return ext_; }
    uint8_t     State() const { return state_; }
    uint32_t    Method() const { return method_; }
    uint32_t    HTTPVersion() const { return httpVersion_; } 
    uint32_t    Phase() const { return phase_; }
    Header& Headers()            { return headers_; }

    void SetMethod(const uint32_t method) { method_ = method; }
    void SetState(const uint8_t state) { state_ = state; }
    void SetHTTPVersion(const uint32_t version) { httpVersion_ = version; }
    void SetPhase(const uint32_t phase) { phase_ = phase; }
    void SetField(const u_char*, const u_char*, uint8_t filed);
    void SetHeaders (const u_char*, const u_char*, const u_char*, const u_char*);
 
#if 0
    void SetReadEventCallback(const boost::function<void(/*TODO*/)>& cb)
    { readcallback_ = cb; }

    void SetWriteEventCallback(const boost::function<void(/*TODO*/)>& cb)
    { writecallback_ = cb; }
#endif

    ConnectionPt GetConnection() const { return connection_.lock(); }

private:
    boost::weak_ptr<Connection> connection_;

    uint8_t state_;    // parsing state

    uint32_t method_;
    std::string uri_;
    uint32_t httpVersion_;
    std::string ext_;
    Header headers_;
    uint32_t phase_;

    void toLower(std::basic_string<char>&);
};

#endif
