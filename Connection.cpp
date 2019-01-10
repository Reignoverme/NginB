#include "Connection.h"
#include "Channel.h"
#include "Acceptor.h"
#include "Buffer.h"
#include "Request.h"
#include "HTTPRequestHandler.h"

Connection::Connection(EventLoop* loop,
                       Acceptor* acpt,
                       int fd,
                       int peerPort,
                       boost::asio::ip::address peerAddr)
    :loop_(loop), 
     acpt_(acpt),
     fd_(fd),
     peerPort_(peerPort),
     peerAddr_(peerAddr),
     channel_(new Channel(fd_, loop)),
//     request_(new Request(shared_from_this())),
     active_(true)
{
    channel_->setReadCallback(
        boost::bind(&Connection::handleRead, this));
    //channel_->setWriteCallback(
    //    boost::bind(&Connection::handleWrite, this));
}

void Connection::connEstablished()
{
    channel_->enableReading();
}

void Connection::closeConnection()
{
    channel_->disableEvent();
    active_ = false;
    ::close(fd_);
    acpt_->closeConnection(this);
}

int Connection::getRequest()
{
    request_ = boost::make_shared<Request>(shared_from_this());
    if (request_ == NULL) {
        return -1;
    }
    return 1;
}

int Connection::handleRead()
{
    /*
     * Handle read events on connection socket.
     */
    size_t clientHeaderBufferSize = 1024;    //size = 1k by default - nginx.
                                             //TODO put this in config file.
    Buffer headerBuf(clientHeaderBufferSize); 
    ssize_t n = headerBuf.readFd(fd_, 1024); 

    if (n < 0) {
        std::cout << "recv error: " << strerror(errno);
        return -1;
    }

    if (n == 0) {
        std::cout << "peer closed connection\n"; 
        closeConnection();
        return 1;
    }

    int rc = processRequestLine(headerBuf, request_);

    if (rc == OK) {
        /* the request line has been parsed successfully */

        /* test case */
        std::cout << "method: " << request_->method() << std::endl
            << "uri: " << request_->uri() << std::endl
            << "HTTP version: " << request_->httpVersion() << std::endl;

        return OK;
    } else {
        std::cout << "process request line failed.\n";
        return -1;
    } 
}

void Connection::handleWrite()
{
    //send(fd_, "gotcha!\n", sizeof("gotcha!\n"), 0);
}
