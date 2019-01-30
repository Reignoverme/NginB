#ifndef UPSTREAM_HANDLER_H
#define UPSTREAM_HANDLER_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>

#include "Connection.h"
#include "PhaseHandler.h"

class Connection;
class Request;

class UpstreamHandler : public PhaseHandler
{
public:
    typedef boost::shared_ptr<Request> RequestPtr;
    typedef boost::shared_ptr<Response> ResponsePtr;
    typedef std::unordered_map<std::string, std::string> Headers;
    typedef std::unordered_map<int,
            boost::shared_ptr<Connection>> Connections; 

    UpstreamHandler(EventLoop* loop,
                    RequestPtr  r,
                    std::string location,
                    std::string us,
                    unsigned short up):
        PhaseHandler(loop),
        request_(r),
        location_(location),
        upstreamServer_(us),
        upstreamPort_(up)
    {}

    //~UpstreamHandler() { upstreamConnections_.clear(); }

    /* 
     * Create request,
     * Connect upstream server,
     * send request,
     * recv response,
     * send back to client.
     *
     */
    virtual int Handle(Connection::RequestPtr&,
            Connection::ResponsePtr&);

    // Connect to upstream server
    int Connect();
    // Create request to upstream server
    int CreateRequest();

    int UpstreamSendRequest(Response::ConnectionPtr&);
    int UpstreamProcessHeader(Response::ConnectionPtr&);

    void CloseConnection(Connection* c) { UpstreamCloseConnection(c); }

private:
    RequestPtr                  request_;    // request from client
    ResponsePtr                 response_;   // response from upstream server
    std::string                 location_;
    std::string                 upstreamServer_;
    unsigned short              upstreamPort_;
    Response::Chain             in_;
    boost::scoped_ptr<Buffer>   buf_;
    boost::scoped_ptr<Buffer>   out_;

    Headers     proxyHeaders_;
    uint32_t    upstreamStatus_;
    Connections upstreamConnections_;

    int  ReadClientBody(const RequestPtr&, int, bool);
    int  ReadRestOfBody(const RequestPtr&, int, bool);
    int  UpstreamTestConnection(Response::ConnectionPtr&);
    void UpstreamCloseConnection(Connection* c) 
    { upstreamConnections_.erase(c->fd()); }
};

#endif
