#include "Request.h"
#include "UpstreamHandler.h"

int UpstreamHandler::Handle(Request& req, Response& res)
{
    CreateRequest();
    Connect();        
}

int UpstreamHandler::CreateRequest()
{
    typedef std::unordered_map<std::string, std::string> Header;

    Header          rh;
    size_t          len;
    std::string     method;
    std::string     requestLine;

    rh = request_->Headers();

    switch (request_->Method()) {
    case HTTP_GET:
        method = "GET";
        break;
    case HTTP_POST:
        method = "POST";
        break;
    case HTTP_HEAD:
        method = "HEAD";
        break;
    }
        
    // request line
    len = method.size() + 1 + request_->Uri().size() + 1
        + sizeof("HTTP/1.x") - 1;
    len += sizeof("\r\n") - 1;

    requestLine = method + " " + request_->Uri() + " " + "HTTP/1.1\r\n";

    Header::const_iterator it;

    // request header
    for (it = rh.begin(); it != rh.end(); it++) {
        len += it->first.size();
        len += sizeof(": ") - 1;
        len += it->second.size();
        len += sizeof("\r\n") - 1;
    }
    len += sizeof("\r\n") - 1;

    buf_.reset(new Buffer(len));
    buf_->SetLast(cpystr(buf_, requestLine.c_str(), requestLine.size()));

    for (it = rh.begin(); it != rh.end(); it++) {
        buf_->SetLast(cpystr(buf_, it->first.c_str(), it->first.size()));
        buf_->SetLast(cpystr(buf_, ": ", sizeof(": ") - 1));
        buf_->SetLast(cpystr(buf_, it->second.c_str(), it->second.size()));
        buf_->SetLast(cpystr(buf_, "\r\n", sizeof("\r\n") - 1));
    }

    buf_->SetLast(cpystr(buf_, "\r\n", sizeof("\r\n") - 1));

    u_char* b = buf_->Start();
    std::cout << "create request:\n";
    for (; b != buf_->Last(); b++) {
        std::cout << *b;
    }
}

int UpstreamHandler::Connect()
{
    int                             rc, sockfd;
    uint32_t                        event;
    struct sockaddr_in              uaddr;
    boost::shared_ptr<Connection>   c;

    sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, 0);
    if (sockfd == -1) {
        std::cout << "socket(2) error: "
            << strerror(errno) << std::endl;
        return ERROR;
    }

    // old-fashion way - setting sockaddr_in manually

    uaddr.sin_family = AF_INET;
    uaddr.sin_port   = htons(upstreamPort_);
    inet_pton(AF_INET, upstreamServer_.c_str(), &uaddr.sin_addr);
    boost::asio::ip::address upstreamAddr = 
        boost::asio::ip::address::from_string(upstreamServer_.c_str());

    c = boost::make_shared<Connection>(
            PhaseHandler::GetEventLoop(),
            nullptr,
            this,
            sockfd,
            upstreamPort_,
            upstreamAddr);
    
    // don't forget to add 'this' pointer when binding member function
    c->SetWriteCallback(
            boost::bind(&UpstreamHandler::UpstreamSendRequest, this, c.get()));
    c->SetReadCallback(
            boost::bind(&UpstreamHandler::UpstreamProcessHeader, this, c.get()));

    upstreamConnections_[sockfd] = c;
    c->GetRequest(); 
    c->ConnEstablished(false, true);    // enable EPOLLOUT.

    rc = connect(sockfd, (struct sockaddr*)&uaddr, sizeof(uaddr));

    if (rc == -1) {
        if (errno != EINPROGRESS) {
            std::cout << "connect(2) failed: "
                << strerror(errno) << std::endl;
            return ERROR;
        }

        std::cout << "connect(2): " << strerror(errno) << std::endl;
        return AGAIN;
    }
    
    if (rc == 0) {
        // connection established
        std::cout << "connection established\n";

        UpstreamSendRequest(c.get());
    }
}

int UpstreamHandler::UpstreamSendRequest(Connection* c)
{
    if (UpstreamTestConnection(c) != OK) {
        std::cout << "Connection closed\n";
        UpstreamCloseConnection(c);
        return OK;
    }

    std::cout << "sending request to upstream server\n";

    //TODO send request body

    size_t expected = buf_->Last() - buf_->Pos();
    size_t n = buf_->Write(c->fd());

    if (n < 0) {
        if (errno == EAGAIN) {
            std::cout << "write not ready\n";
            return AGAIN;
        } else {
            std::cout << "write(2) error: " 
                << strerror(errno) << std::endl;
            return ERROR;
        }
    }

    if (n < expected) {
        std::cout << "write less than expected.\n";
        return AGAIN;
    }
    
    std::cout << "send completed\n";
    // disable write
    c->Disable(); c->EnableReading(true); 

    return UpstreamProcessHeader(c);
}

int UpstreamHandler::UpstreamProcessHeader(Connection* c)
{
    uint32_t rc;

    std::cout << "reading response from upstream\n";

    out_.reset(new Buffer(4096));    //default value from nginx

    if (UpstreamTestConnection(c) != OK) {
        std::cout << "upstream process header: connection closed\n";
        UpstreamCloseConnection(c);
        return OK;
    }
    
    ssize_t n = out_->ReadFd(c->fd(), out_->End() - out_->Start());

    if (n == AGAIN) {
        return AGAIN;
    } 

    if (n == 0) {
        std::cout << "peer closed connection prematurely\n";
        UpstreamCloseConnection(c);
        return OK;
    }

    std::cout << "recv: \n";
    u_char* a = out_->Pos();
    for( ; a != out_->Last(); a++ ) {
        std::cout << *a;
    }

    return OK;
}

int UpstreamHandler::UpstreamTestConnection(Connection* c)
{
    int          err;
    socklen_t    len;

    err = 0;
    len = sizeof(int);

    if (getsockopt(c->fd(), SOL_SOCKET, SO_ERROR, (void*)&err, &len)
            == 0)
    {
        if (err) {
            std::cout << "connect(2) failed: " 
                << strerror(err) << std::endl;
            return ERROR;
        }
    }

    return OK;
}
