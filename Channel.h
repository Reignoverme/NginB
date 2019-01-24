#ifndef CHANNEL_H
#define CHANNEL_H
#include <boost/core/noncopyable.hpp>
#include <boost/function.hpp>
#include <sys/epoll.h>
#include <iostream>

class EventLoop;

class Channel : boost::noncopyable
{
public:
    typedef boost::function<int()> EventCallback;

    Channel(int fd, EventLoop* loop): loop_(loop),
                                      fd_(fd),
                                      events_(0),
                                      ractive_(false)
    {}

    void handleEvent();
    void enableReading(bool);
    void enableWriting(bool);

    void setReadCallback(const EventCallback& cb)
    {   
        if (cb) {
//            std::cout << "binding read cb on " << fd_ << std::endl;
            readCallback_ = cb;
        } else {
            std::cout << "read cb error!\n";
        }
    }
    void setWriteCallback(const EventCallback& cb)
    {
        if (cb) {
 //           std::cout << "binding write cb on " << fd_ << std::endl;
            writeCallback_ = cb;
        } else {
            std::cout << "write cb error!\n";
        }
    }
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

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_;
    bool ractive_;
};

#endif
