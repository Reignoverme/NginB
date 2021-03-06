#include "EventLoop.h"
#include <boost/asio.hpp>

int main()
{
    boost::asio::ip::address ad = 
        boost::asio::ip::address::from_string("192.168.72.131");
    int port = 9999;
    EventLoop loop(port, ad);
    loop.startListen();
    loop.loop();
}
