#ifndef CONNECTION_H
#define CONNECTION_H

#include <cerrno>
#include <iostream>
#include <sys/socket.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "Channel.h"

class Channel;
class EventLoop;
class Buffer;
class Acceptor;
class Request;
class Response;
class UpstreamHandler;

class Connection : boost::noncopyable,
    public boost::enable_shared_from_this<Connection>
{
public:
    typedef boost::function<int()> Callback;

    Connection(EventLoop* loop,
               Acceptor* acpt,
               UpstreamHandler* cnct,
               int fd,
               int peerPort,
               boost::asio::ip::address peerAddr);

    void EnableReading(bool clear) { channel_->enableReading(clear); }
    void EnableWriting(bool clear) { channel_->enableWriting(clear); }
    void Disable()       { channel_->disableEvent(); }

    int GetRequest();
    void ConnEstablished(bool, bool);

    // default callback - handle request from client.
    void SetReadCallback() { 
        channel_->setReadCallback(
            boost::bind(&Connection::handleRead, this));
    }

    void SetReadCallback(const Callback& cb) { channel_->setReadCallback(cb); }
    void SetWriteCallback(const Callback& cb) { channel_->setWriteCallback(cb); }
    const int fd() const { return fd_; }

private:
    EventLoop* loop_;
    Acceptor* acpt_;
    UpstreamHandler* cnct_;
    int fd_;
    int peerPort_;
    boost::asio::ip::address peerAddr_;

    boost::scoped_ptr<Channel> channel_;
    boost::scoped_ptr<Buffer> buf_;

    boost::shared_ptr<Request> request_;
    boost::shared_ptr<Response> response_;

    bool active_;

    int handleRead();
    void handleWrite();
    int handleHeaders();
    void CloseConnection();    //this function should be called inside class only
    void ProcessRequest(const boost::shared_ptr<Request>&,
                        const boost::shared_ptr<Response>&);
};

#endif
