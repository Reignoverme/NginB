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

void Channel::enableReading()
{ 
    ractive_ = true; 
    events_ |= EPOLLIN | EPOLLRDHUP;
    update();
}

void Channel::enableWriting()
{ 
    ractive_ = true;
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
        }
    } else if(revents_ & ( EPOLLOUT )) {
        std::cout << "write event\n";
        //std::cout << "handle write event on: " << fd_ << std::endl;
        //if(writeCallback_)
        //{
        //    writeCallback_();
        //}
    } else {
        std::cout << "unknow event: " << revents_ << std::endl;
    }
}
