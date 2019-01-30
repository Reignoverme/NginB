#ifndef PHASE_HANDLER_H
#define PHASE_HANDLER_H

#include <cerrno>
#include <cstring>
#include <sys/uio.h>
#include <sys/sendfile.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "Buffer.h"
#include "Request.h"
#include "Response.h"
#include "HTTPCore.h"
#include "EventLoop.h"

#define cpystr(b, str, size) ((u_char*)memcpy(b.Last(), str, size) + size)

class PhaseHandler: boost::noncopyable
{
public:
    typedef boost::function<int(/*TODO*/)> Handler;

    PhaseHandler(EventLoop* loop):
        loop_(loop)
        {}

    virtual ~PhaseHandler() {};
    virtual int Handle(boost::shared_ptr<Request>&,
            boost::shared_ptr<Response>&) = 0;

    EventLoop* GetEventLoop() const { return loop_; }

    int SendBody(boost::shared_ptr<Response>&, size_t);

    int SendHeaders(int, Response&, Response::Chain&);
    int SendHeaders(Connection::RequestPtr&,
            Connection::ResponsePtr&);    // send headers to downstream client.

    int             WriteFilter(int, Response&, Response::Chain&);
private:
    EventLoop* loop_;

    int             WritevChain(int, Response::Chain&);
    void            UpdateChainAfterSend(Response::Chain&, ssize_t);
    size_t          OutputChainToIovec(Response::Iovec&, Response::Chain&);
    ssize_t         Writev(int, Response::Iovec&);

};


#endif
