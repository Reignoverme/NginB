#ifndef EPOLLER_H
#define EPOLLER_H
#include <sys/epoll.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <map>
#include "EventLoop.h"

class Channel;

class Epoller
{
public:
    typedef std::vector<Channel*> ChannelList;
    Epoller(EventLoop* loop);
    void poll(ChannelList *ac);
    void updateChannel(Channel* c);
    void disableChannel(Channel* c);

private:
    typedef boost::shared_ptr<struct epoll_event[10]> EventList;
    typedef std::map<int, Channel*> ChannelMap;

    int epfd_;
    ChannelMap channels_;
    EventLoop* loop_;

    void fillActiveChannels(int nevents, EventList ev, ChannelList* ac);
};

#endif
