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
    ssize_t n = headerBuf.ReadFd(fd_, 1024); 

    if (n < 0) {
        std::cout << "recv error: " << strerror(errno);
        return -1;
    }

    if (n == 0) {
        std::cout << "peer closed connection\n"; 
        closeConnection();
        return 1;
    }

    int rc = ProcessRequestLine(headerBuf, request_);

    if (rc == OK) {
        /* the request line has been parsed successfully */

        /* test case */

        ProcessRequestHeaders(headerBuf, request_);
        
        std::cout << "----- request line -----\n"; 
        std::cout << "method: " << request_->Method() << std::endl
            << "uri: " << request_->URI() << std::endl
            << "HTTP version: " << request_->HTTPVersion() << std::endl;
        
        std::cout << "----- request headers -----\n";
        for(std::map<std::string, std::string>::const_iterator it
                = request_->Headers().begin();
            it != request_->Headers().end();
            it++) {

            std::cout << it->first << ": " << it->second << std::endl;
        }       
        return OK;
    } else if (rc >= 10){
        std::cout << "Bad request.\n";
        //TODO send 4xx response.
        ::send(fd_, "400 Bad Request\r\n", sizeof("400 Bad Request\r\n"), 0);
        return -1;
    } else {
        std::cout << "parse request line failed.\n";
        return -1;
    } 
}

void Connection::handleWrite()
{
    //send(fd_, "gotcha!\n", sizeof("gotcha!\n"), 0);
}
