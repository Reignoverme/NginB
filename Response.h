#ifndef RESPONSE_H
#define RESPONSE_H

#define HTTP_OK                    200
#define HTTP_NOT_FOUND             404
#define HTTP_INTERNAL_SERVER_ERROR 500 

#include <unordered_map>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

extern std::unordered_map<std::string, std::string> types;

class Connection;

class Response
{
public:
    typedef std::unordered_map<std::string, std::string> Header;
    typedef boost::shared_ptr<Connection> ConnectionPt;

    Response(ConnectionPt c):
        connection_(c),
        state_(0),
        serverName_("NginB/0.0.1"),
        set_(false)
    {}

    void SetStatusLine(const u_char*, const u_char*);

    void Set()                              { set_ = true; }
    void SetFd(const int fd)                { fd_ = fd; }
    void SetState(uint8_t state)            { state_ = state; }
    void SetStatusCode (const uint32_t status)  { statusCode_ = status; }
    void SetHTTPVersion(uint32_t version)   { httpVersion_ = version; }
    void SetHeaders(const std::string& field,
            const std::string& value)       { headers_[field] = value; }
    void SetHeaders (const u_char*, const u_char*, const u_char*, const u_char*);

    int fd() const                          { return fd_; }
    bool IsSet() const                      { return set_; }
    uint32_t& StatusCode()                  { return statusCode_; }
    uint32_t  HTTPVersion()                 { return httpVersion_; }
    uint32_t  State() const                 { return state_; }
    std::string StatusLine()                { return statusLine_; }
    const Header& Headers() const           { return headers_; }
    ConnectionPt GetConnection() const      { return connection_.lock(); }
    const std::string& ServerName() const   { return serverName_; }


private:
    boost::weak_ptr<Connection> connection_;

    int fd_;
    uint32_t statusCode_;
    uint8_t state_;
    std::string statusLine_;
    uint32_t httpVersion_;
    std::string serverName_;
    Header headers_;

    bool set_;
};

#endif
