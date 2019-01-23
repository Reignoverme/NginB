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
        serverName_("NginB/0.0.1")
    {}

    void SetStatusLine(const u_char*, const u_char*);
    void SetHeaders (const u_char*, const u_char*, const u_char*, const u_char*);

    void SetHeaders(const std::string& field,
            const std::string& value)       { headers_[field] = value; }
    void SetFd(const int fd)                { fd_ = fd; }
    void SetState(uint8_t state)            { state_ = state; }
    void SetStatus (const uint32_t status)  { status_ = status; }
    void SetHTTPVersion(uint32_t version)   { httpVersion_ = version; }

    uint32_t& Status()                      { return status_; }
    uint32_t  State() const                 { return state_; }
    const int fd() const                    { return fd_; }
    const Header& Headers() const           { return headers_; }
    ConnectionPt GetConnection() const      { return connection_.lock(); }
    const std::string& ServerName() const   { return serverName_; }


private:
    boost::weak_ptr<Connection> connection_;

    int fd_;
    uint32_t status_;
    uint8_t state_;
    std::string statusLine_;
    uint32_t httpVersion_;
    std::string serverName_;
    Header headers_;
};

#endif
