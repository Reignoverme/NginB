#ifndef CHANNEL_H
#define CHANNEL_H
#include <boost/core/noncopyable.hpp>
#include <boost/function.hpp>
#include <sys/epoll.h>

class EventLoop;

class Channel : boost::noncopyable
{
public:
    typedef boost::function<void()> EventCallback;
    typedef boost::function<void()> ReadEventCallback;

    Channel(int fd, EventLoop* loop): loop_(loop),
                                      fd_(fd),
                                      events_(0),
                                      ractive_(false)
    {}

    void handleEvent();
    void enableReading();
    void enableWriting();

    void setReadCallback(const ReadEventCallback& cb)
    { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb)
    { writeCallback_ = cb; }

    void disableEvent()
    { ractive_ = false; disable(); }

    void setRevents(int events)
    { revents_ = events; }

    bool isRactive()
    { return ractive_; }

    int fd() const { return fd_; }
    int events() const { return events_; }

private:
    void update();
    void disable();

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_;
    bool ractive_;
};

#endif
