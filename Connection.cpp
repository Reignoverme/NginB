#include "Buffer.h"
#include "Request.h"
#include "Response.h"
#include "Acceptor.h"
#include "Connection.h"
#include "StaticHandler.h"
#include "UpstreamHandler.h"
#include "HTTPRequestHandler.h"

Connection::Connection(EventLoop* loop,
                       Acceptor* acpt,
                       UpstreamHandler* cnct,
                       int fd,
                       int peerPort,
                       boost::asio::ip::address peerAddr)
    :loop_(loop), 
     acpt_(acpt),
     cnct_(cnct),
     fd_(fd),
     peerPort_(peerPort),
     peerAddr_(peerAddr),
     channel_(new Channel(fd_, loop)),
     buf_(new Buffer(1024)),
     active_(true)
{
    //channel_->setReadCallback(
    //    boost::bind(&Connection::handleRead, this));

    //channel_->setWriteCallback(
    //    boost::bind(&Connection::handleWrite, this));
}

void Connection::ConnEstablished(bool er, bool ew)
{
    if (er) {
        channel_->enableReading(false);
    }

    if (ew) {
        channel_->enableWriting(false);
    }
}

void Connection::CloseConnection()
{
    channel_->disableEvent();
    active_ = false;
    ::close(fd_);
    if (acpt_) {
        acpt_->CloseConnection(this);
    }

    if (cnct_) {
        cnct_->CloseConnection(this);
    }
}

int Connection::GetRequest()
{
    request_ = boost::make_shared<Request>(shared_from_this());
    if (request_.get() == NULL) {
        std::cout << "create request object failed\n";
        abort();
    }

    response_ = boost::make_shared<Response>(shared_from_this());
    if (response_.get() == NULL) {
        std::cout << "create response object failed\n";
        abort();
    }

    return 1;
}

void Connection::ProcessRequest(const boost::shared_ptr<Request>& req,
                            const boost::shared_ptr<Response>& res)
{
    /* TODO
     * check uri, dispatch this request to
     * StaticHandler or
     * UpstreamHandler
     */

    //StaticHandler sh(loop_);
    //sh.Handle(*req, *res); 
    //


    UpstreamHandler *up = new UpstreamHandler(loop_, req, "/", "127.0.0.1", 8000);

    up->Handle(*req, *res);
}

int Connection::handleHeaders()
{
    int rc;

    ssize_t n = buf_->ReadFd(fd_, buf_->End() - buf_->Last()); 

    rc = ProcessRequestHeaders(buf_, request_);
   
    if (rc == OK)
    {
        std::cout << "----- request line -----\n"; 
        std::cout << "method: " << request_->Method() << std::endl
            << "uri: " << request_->Uri() << std::endl
            << "HTTP version: " << request_->HTTPVersion() << std::endl;

        std::cout << "----- request headers -----\n";
        for(std::unordered_map<std::string, std::string>::const_iterator
                it = request_->Headers().begin();
                it != request_->Headers().end();
                it++) {
            std::cout << it->first << ": " << it->second << std::endl;
        }       

        std::cout << "----- processing request -----\n";
        ProcessRequest(request_, response_);

        if (request_->Headers()["Connection"] == "close") {
            std::cout << "closing connection\n";
            CloseConnection();
        }

        buf_->Reset();
        channel_->setReadCallback(
                boost::bind(&Connection::handleRead, this));
    }

    return rc;
}

/*
 * Handle read events on connection socket.
 * @return    
 */
int Connection::handleRead()
{
    /*
     * TODO
     * if (cnct_) call cnct_->handleRead()
     * else if (acpt_) go on processing
     *
     */
    ssize_t n = buf_->ReadFd(fd_, buf_->End() - buf_->Last()); 

    if (n < 0) {
        std::cout << "recv error: " << strerror(errno);
        return -1;
    }

    if (n == 0) {
        std::cout << "peer closed connection\n"; 
        CloseConnection();
        return 1;
    }

    int rc = ProcessRequestLine(buf_, request_);

    if (rc == OK) {
        /* the request line has been parsed successfully */

        channel_->setReadCallback(boost::bind(
                    &Connection::handleHeaders, this));
        rc = handleHeaders(); 

        return rc;

    } else if (rc == AGAIN) {
        std::cout << "returning again\n";
        //buf_->Reset();
        return AGAIN;
    } else if (rc >= 10) {
        std::cout << "Bad request.\n";
        //TODO send 4xx response.
        buf_->Reset();
        ::send(fd_, "400 Bad Request\r\n", sizeof("400 Bad Request\r\n"), 0);
        return -1;
    }
    return ERROR;
}

void Connection::handleWrite()
{
    //send(fd_, "gotcha!\n", sizeof("gotcha!\n"), 0);
}
