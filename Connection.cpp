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
     active_(true),
     closed_(false)
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
    closed_ = true;

    channel_->disableEvent();
    active_ = false;
    std::cout << "closing " << fd_ << std::endl;
    ::close(fd_);
    if (acpt_) {
    //    std::cout << "closing acpt connection" << fd_ << std::endl;
        acpt_->CloseConnection(this);
    }

    if (cnct_) {
     //   std::cout << "closing cnct connection" << fd_ << std::endl;
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

int Connection::ProcessRequest(RequestPtr& req, ResponsePtr& res)
{
    int rc;

    if (req->Uri().compare(0, 7, "/static") == 0) {
        StaticHandler sh(loop_);
        rc = sh.Handle(req, res); 
    } else {
        // TODO manage life time of Upstream objects.
        UpstreamHandler *up = new UpstreamHandler(loop_, req, "/", "127.0.0.1", 8000);
        rc = up->Handle(req, res);
    }

    return rc;
}

int Connection::handleHeaders()
{
    int rc;

    //u_char* a = buf_->Last();

    ssize_t n = buf_->ReadFd(fd_, buf_->End() - buf_->Last()); 

    if (n < 0) {
       return n;      
    }

    if (n == 0) {
        std::cout << "handleheaders: connection closed\n";
        CloseConnection();
        return OK;
    }

    //std::cout << "****** handle headers recved: ******\n";
    //for ( ; a!=buf_->Last(); a++ ) {
    //    std::cout << *a;
    //}

    rc = ProcessRequestHeaders<RequestPtr>(buf_, request_);
   
    if (rc == OK)
    {
        std::cout << "----- request line -----\n"; 
        std::cout << "method: " << request_->Method() << std::endl
            << "uri: " << request_->Uri() << std::endl
            << "HTTP version: " << request_->HTTPVersion() << std::endl;

        //std::cout << "----- request headers -----\n";
        //for(std::unordered_map<std::string, std::string>::const_iterator
        //        it = request_->Headers().begin();
        //        it != request_->Headers().end();
        //        it++) {
        //    std::cout << it->first << ": " << it->second << std::endl;
        //}       

        std::cout << "----- processing request -----\n";
        rc = ProcessRequest(request_, response_);
        if ( rc == AGAIN || rc == ERROR ) {
            return rc;
        }

        if (request_->Headers()["Connection"] == "close") {
            std::cout << "closing connection\n";
            CloseConnection();
        }

        //buf_->Reset();
        //channel_->setReadCallback(
        //        boost::bind(&Connection::handleRead, this));
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
        CloseConnection();
        return ERROR;
    }

    if (n == 0) {
        std::cout << "peer closed connection\n"; 
        CloseConnection();
        return OK;
    }

    //std::cout << "****** recved: ******\n";
    //u_char* a = buf_->Start();
    //for ( ; a!=buf_->Last(); a++ ) {
    //    std::cout << *a;
    //}

    std::cout << "------ process request line ------\n";
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
        ::send(fd_, "HTTP/1.1 400 Bad Request\r\n\r\n",
                sizeof("HTTP/1.1 400 Bad Request\r\n\r\n")-1, 0);
        return -1;
    }
    return ERROR;
}

void Connection::handleWrite()
{
    //send(fd_, "gotcha!\n", sizeof("gotcha!\n"), 0);
}
