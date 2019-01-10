#include "EventLoop.h"
#include "Channel.h"
#include "Epoller.h"
#include "Acceptor.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <boost/bind.hpp>
#include <cerrno>

EventLoop::EventLoop(int listenport,
                     boost::asio::ip::address listenaddr)
     :listenport_(listenport),
      listenaddr_(listenaddr),
      poller_(new Epoller(this)),
      acceptor_(new Acceptor(this, listenport_, listenaddr_))
{}

void EventLoop::startListen()
{
    acceptor_->listen();
}

void EventLoop::updateChannel(Channel* c)
{
    poller_->updateChannel(c); 
}

void EventLoop::disableChannel(Channel* c)
{
    poller_->disableChannel(c);
}

void EventLoop::loop()
{
    while(true)
    {
        activeChannels_.clear();
        poller_->poll(&activeChannels_);
        for( ChannelList::iterator it = activeChannels_.begin();
             it != activeChannels_.end();
             ++it)
        {
            (*it)->handleEvent();
        }
    }
}
