#include "Channel.h"
#include "EventLoop.h"

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::disable()
{
    loop_->disableChannel(this);
}

void Channel::enableReading(bool clear)
{ 
    ractive_ = true; 
    if (clear) {
        events_ &= 0;
    }
    events_ |= EPOLLIN | EPOLLRDHUP;
    update();
}

void Channel::enableWriting(bool clear)
{ 
    ractive_ = true;
    if (clear) {
        events_ &= 0;
    }
    events_ |= EPOLLOUT;
    update();
}

void Channel::handleEvent()
{
    if(revents_ & (EPOLLIN|EPOLLRDHUP))
    {
        std::cout << "handle read event on: " << fd_ << std::endl;
        if(readCallback_)
        {
            readCallback_();
        } else {
            std::cout << "no read callback." << std::endl;
            abort();
        }
    } 

    if (revents_ & ( EPOLLOUT )) {
        std::cout << "handle write event on: " << fd_ << std::endl;
        if(writeCallback_)
        {
            writeCallback_();
        } else {
            std::cout << "no write callback\n";
            abort();
        }
    }

}
