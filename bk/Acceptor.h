#ifndef ACCEPTOR_H
#define ACCEPTOR_H
#include <boost/core/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <iostream>
#include <mutex>
#include <map>
#include <fcntl.h>
#include "Channel.h"
#include "Connection.h"

class Acceptor : boost::noncopyable
{
public:
    typedef boost::function<void(/*TODO*/)> ConnectionCallback;

    Acceptor(EventLoop* loop,
             int port, 
             boost::asio::ip::address localaddr);

    void setConnectionCallback(ConnectionCallback& cb)
    { onConnection_ = cb; }
    
    void closeConnection(Connection* c)
    { connections_.erase(c->fd()); }  // erase calls dtor of boost::shared_ptr.

    int fd() { return sockfd_; }

    void bind();
    void listen();

private:
    typedef std::map<int, boost::shared_ptr<Connection>> ConnMap;
    void handleRead();
    
    ConnectionCallback onConnection_;
    std::mutex mutex_;
    EventLoop* loop_;
    int sockfd_; 
    int port_;
    boost::asio::ip::address localaddr_;

    ConnMap connections_;    //established connections 
    boost::scoped_ptr<Channel> acceptChannel_;    //accept channel lives forever 
};

#endif
