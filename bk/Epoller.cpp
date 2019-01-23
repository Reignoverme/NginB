#include "Epoller.h"
#include "Channel.h"
#include <cerrno>

Epoller::Epoller(EventLoop* loop)
    :loop_(loop)
{
    epfd_ = epoll_create(42);
}

void Epoller::updateChannel(Channel* c)
{
    /* must initialize ev, or ev.events gets 
     * unspecified values.
     */
    struct epoll_event ev;
    bzero(&ev, sizeof ev);

    ev.data.fd = c->fd();
    ev.events |= c->events();
    int err = epoll_ctl(epfd_, EPOLL_CTL_ADD, c->fd(), &ev);
    if(err < 0)
    {
        std::cout << "epoll_ctl: " << strerror(errno) << std::endl;
    }
    channels_[c->fd()] = c;
}

void Epoller::disableChannel(Channel* ch)
{
    epoll_ctl(epfd_, EPOLL_CTL_DEL, ch->fd(), NULL);
    channels_.erase(ch->fd());
}

void Epoller::poll(ChannelList& ac)
{
    EventList ev(new struct epoll_event[10]);
    
    int nevents = epoll_wait(epfd_, ev.get(), 10, -1); 
    if(nevents > 0)
        fillActiveChannels(nevents, ev, ac);
    else if(nevents < 0)
    {
        std::cout << "epoll_wait: " << strerror(errno) << std::endl;
    }
}

void Epoller::fillActiveChannels(int nevents, EventList ev, ChannelList& ac)
{
    int i;
    for(i=0; i<nevents; i++)
    {
        ChannelMap::iterator it = channels_.find(ev[i].data.fd);
        assert(it != channels_.end());
        Channel* c = it->second;
        std::cout << "fd: " << c->fd() << " ready ";
        std::cout << "events: " << std::hex << ev[i].events 
            << std::dec << std::endl;
        if(ev[i].events & (EPOLLIN|EPOLLOUT|EPOLLRDHUP))
        {
            c->setRevents(ev[i].events);
            ac.push_back(c);
        }
    }
}
