#include "EventLoop.h"
#include <boost/asio.hpp>

int main()
{
    boost::asio::ip::address ad = 
        boost::asio::ip::address::from_string("127.0.0.1");
    int port = 9999;
    EventLoop loop(port, ad);
    loop.startListen();
    loop.loop();
}
