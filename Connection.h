#ifndef CONNECTION_H
#define CONNECTION_H
#include <boost/core/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <sys/socket.h>
#include <iostream>
#include <cerrno>

class Channel;
class EventLoop;
class Buffer;
class Acceptor;
class Request;

class Connection : boost::noncopyable,
    public boost::enable_shared_from_this<Connection>
{
public:

    Connection(EventLoop* loop,
               Acceptor* acpt,
               int fd,
               int peerPort,
               boost::asio::ip::address peerAddr);

    void connEstablished();
    int handleRead();
    void handleWrite();
    int getRequest();
    void closeConnection();
    int fd() { return fd_; }

private:
    char buf_[1024];
    EventLoop* loop_;
    Acceptor* acpt_;
    int fd_;
    int peerPort_;
    boost::asio::ip::address peerAddr_;

    boost::scoped_ptr<Channel> channel_;
    boost::shared_ptr<Request> request_;

    bool active_;

    boost::shared_ptr<Connection> getShared()
    { return shared_from_this(); }
};

#endif
