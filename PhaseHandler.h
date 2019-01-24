#ifndef PHASE_HANDLER_H
#define PHASE_HANDLER_H

#include <cerrno>
#include <cstring>
#include <sys/sendfile.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "Buffer.h"
#include "Request.h"
#include "Response.h"
#include "HTTPCore.h"
#include "EventLoop.h"

#define cpystr(b, str, size) ((u_char*)memcpy(b->Last(), str, size) + size)

class PhaseHandler: boost::noncopyable
{
public:
    typedef boost::function<int(/*TODO*/)> Handler;

    PhaseHandler(EventLoop* loop):
        loop_(loop)
        {}

    virtual ~PhaseHandler() {};
    virtual int Handle(Request&, Response&) = 0;

    int SendHeaders(Request&, Response&);
    int SendBody(Request&, Response&, size_t);
    EventLoop* GetEventLoop() const { return loop_; }

private:
    EventLoop* loop_;
};


#endif
