#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <vector>
#include "Acceptor.h"
#include "Epoller.h"

class Epoller;
class Channel;
class Acceptor;

class EventLoop
{
public:
    EventLoop(int listenport,
              boost::asio::ip::address listenaddr);

    void updateChannel(Channel* c);
    void disableChannel(Channel* c);
    void loop();
    void onConnection();
    void startListen();

private:
    typedef std::vector<Channel*> ChannelList;

    int fd_;
    int listenport_;
    boost::asio::ip::address listenaddr_;
    boost::scoped_ptr<Epoller> poller_;
    boost::scoped_ptr<Acceptor> acceptor_;
    ChannelList activeChannels_;

};

#endif
