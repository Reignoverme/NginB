#include "Request.h"
#include "UpstreamHandler.h"
#include "HTTPRequestHandler.h"

int UpstreamHandler::Handle(Connection::RequestPtr& req,
        Connection::ResponsePtr& res)
{
    int rc;
    rc = CreateRequest();
    // request body not complete
    if (rc == AGAIN) {
        return AGAIN;
    }

    Connect();        
    return OK;
}

int UpstreamHandler::CreateRequest()
{
    typedef std::unordered_map<std::string, std::string> Header;

    Header          rh;
    size_t          len;
    std::string     method;
    std::string     requestLine;

    // TODO
    // see if rh["content-lenth"] exists
    // if so, read client body and send to upstream srever

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

    Buffer buff_(len);
    buff_.SetLast(cpystr(buff_, requestLine.c_str(), requestLine.size()));

    for (it = rh.begin(); it != rh.end(); it++) {
        buff_.SetLast(cpystr(buff_, it->first.c_str(), it->first.size()));
        buff_.SetLast(cpystr(buff_, ": ", sizeof(": ") - 1));
        buff_.SetLast(cpystr(buff_, it->second.c_str(), it->second.size()));
        buff_.SetLast(cpystr(buff_, "\r\n", sizeof("\r\n") - 1));
    }

    buff_.SetLast(cpystr(buff_, "\r\n", sizeof("\r\n") - 1));

    in_.push_back(buff_);

    //u_char* b = buf_->Start();
    //std::cout << "------ header send to server ------\n";
    //for (; b != buf_->Last(); b++) {
    //    std::cout << *b;
    //}
    //std::cout << "-----------------------------------\n";
    
    // this request has http body
    if (rh["content-length"].size()) {
        std::cout << "<<<<< reading client body: "
            << rh["content-length"] << " bytes >>>>>\n";

        // use buf_ temporarily as body buffer
        buf_.reset(new Buffer(std::stoi(rh["content-length"])));    

        int rc = ReadClientBody(request_, std::stoi(rh["content-length"]), false);
        if (rc == AGAIN) {
            return AGAIN;
        }
    }

    return OK;
}

int UpstreamHandler::ReadClientBody(const RequestPtr& r, int expected, bool rr)
{
    int                     n, sockfd; 
    Request::ConnectionPt   c;

    c = r->GetConnection();
    if (!c) {
        std::cout << "Connection closed\n";
        UpstreamCloseConnection(c.get());
    }

    boost::scoped_ptr<Buffer>& buf= c->buf();
    sockfd = c->fd();

    n = buf->ReadFd(sockfd, buf->End() - buf->Last());

    if (n < 0) {
        switch (n){
        case AGAIN:
            c->SetReadCallback(boost::bind(&UpstreamHandler::ReadClientBody,
                    this, r, expected, rr)); 
            return AGAIN;

        case ERROR:
            std::cout << "readbuf error: "
                << strerror(errno) << std::endl;
            c->CloseConnection();
            return ERROR;
        }
    }

    if (n == 0) {
        std::cout << "readclientbody: connection closed\n";
        c->CloseConnection();
    }

    // there is part of body left in buffer
    if (n < expected) {
        std::cout << "read client body not complete.\n";
        c->SetReadCallback(boost::bind(&UpstreamHandler::ReadRestOfBody,
                    this, r, expected - n, rr)); 

        // copy what's left to temp buffer
        int i = 0;
        u_char *a = buf_->Last();
        for ( ; i < n; a++, i++ ) {
            *a = *(buf->Pos());
            buf->SetPos(buf->Pos() + 1);
        }

        buf_->SetLast(buf_->Last() + n);
        buf_->SetPos(buf_->Last());
        buf->SetPos(buf->Last());

        return AGAIN;
    }

    // client body all in buffer
    std::cout << "read client body complete: " << buf->Last() - buf->Pos()
        << " bytes\n";
    
    Buffer buff_(buf->Last() - buf->Pos());
    u_char *a = buff_.Last();
    int i = 0;
    for ( ; i < n; a++, i++ ) {
        *a = *(buf->Pos());
        buf->SetPos(buf->Pos() + 1);
    }
    buff_.SetLast(buff_.End());

    in_.push_back(buff_);
    return OK;
}

int UpstreamHandler::ReadRestOfBody(const RequestPtr& r, int expected, bool rr)
{
    int sockfd;
    ssize_t n;
    Request::ConnectionPt c;

    c = r->GetConnection();
    if (!c) {
        std::cout << "connection closed\n";
        c->CloseConnection();
        return OK;
    }

    sockfd = c->fd();

    n = buf_->ReadFd(sockfd, expected);

    if (n < 0) {
        return n;
    }

    if (n == 0) {
        std::cout << "readrestofbody: connection closed\n";
        c->CloseConnection();
        return OK;
    }

    if (n == expected) {
        std::cout << "read rest of body complete\n";
        buf_->SetPos(buf_->Start());    // set this for PhaseHandler::SendHeader
        in_.push_back(*buf_);
        buf_->SetPos(buf_->Last());     // set this for future read

        if (!rr) {
            Connect();
        } else {
            // this returns to Channel::HandleEvent
            // so we have to send response to client here
            PhaseHandler::WriteFilter(request_->GetConnection()->fd(), *response_, in_);
        }
        c->SetReadCallback(boost::bind(&Connection::handleRead,
                    c.get()));

        return OK;

    } else {
        std::cout << "read rest of body not complete, remaining "
            << expected - n << " bytes\n";
        c->SetReadCallback(boost::bind(&UpstreamHandler::ReadRestOfBody,
                    this, r, expected - n, rr));
        buf_->SetPos(buf_->Pos() + n);
        if (buf_->Pos() == buf_->End()) {
            std::cout << "buffer ran out, use a new one\n";
            buf_->SetPos(buf_->Start());    // set this for PhaseHandler::
            in_.push_back(*buf_);
            buf_.reset(new Buffer(8192));
        }
        return OK;
    }
}

int UpstreamHandler::Connect()
{
    int                             rc, sockfd;
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

    c->GetRequest();    // get REQUEST send to upstream server
                        // get RESPONSE for upstream server response

    c->ConnEstablished(false, true);    // enable EPOLLOUT
    upstreamConnections_[sockfd] = c;

    // don't forget to add 'this' pointer when binding member function
    c->SetWriteCallback(
            boost::bind(&UpstreamHandler::UpstreamSendRequest,
                this,
                std::ref(upstreamConnections_[sockfd])));
    c->SetReadCallback(
            boost::bind(&UpstreamHandler::UpstreamProcessHeader,
                this,
                std::ref(upstreamConnections_[sockfd])));


    response_ = boost::make_shared<Response>(c);    // used to send back to client

    std::cout << sockfd << " <------------> "
        << request_->GetConnection()->fd() << std::endl;
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

        UpstreamSendRequest(c);
    }

    return OK;
}

int UpstreamHandler::UpstreamSendRequest(Response::ConnectionPtr& c)
{
    if (UpstreamTestConnection(c) != OK) {
        std::cout << "Connection closed\n";
        UpstreamCloseConnection(c.get());
        return OK;
    }

    std::cout << "sending request to upstream server\n";

    //TODO
    //1.send request body
    //2.change sending approach,
    //use PhaseHandler::SendHeaders
    //
    //1 done:  2019/1/29
    //2 done:  2019/1/29

    int rc = PhaseHandler::SendHeaders(c->fd(), *(c->response()), in_);

    if (rc == OK) {
        std::cout << "send completed\n";
        // disable write
        c->Disable(); c->EnableReading(true); 

        buf_.reset();
        in_.clear();
        return UpstreamProcessHeader(c);
    }

    return AGAIN;
}

int UpstreamHandler::UpstreamProcessHeader(Response::ConnectionPtr& c)
{
    int32_t rc;

    std::cout << "reading response from upstream\n";

    boost::scoped_ptr<Buffer>&  buf = c->buf();
    //out_.reset(new Buffer(4096));    //default value from nginx

    if (UpstreamTestConnection(c) != OK) {
        std::cout << "upstream process header: connection closed\n";
        UpstreamCloseConnection(c.get());
        return OK;
    }
    
    ssize_t n = buf->ReadFd(c->fd(), buf->End() - buf->Start());
    //ssize_t n = out_->ReadFd(c->fd(), out_->End() - out_->Start());

    if (n == AGAIN) {
        return AGAIN;
    } 

    if (n == 0) {
        std::cout << "upstream closed connection\n";
        UpstreamCloseConnection(c.get());
        return OK;
    }

    std::cout << "------ upstream response ------\n";
    u_char* a = buf->Start();
    for( ;a != buf->Last(); a++ ) {
        std::cout << *a;
    }
    std::cout << "-------------------------------\n";

    if (response_->IsSet()) {    // first read event goes to 'else', 
                                 // following read event goes here
        // send body
        // first read response body

        boost::shared_ptr<Connection> ct = request_->GetConnection();
        
        buf_.reset(new Buffer(8192));
        int expected = std::stoi(response_->Headers()["Content-Length"]);
        rc = ReadClientBody(c->request(), expected, true);

        if (rc == OK) {
            std::cout << "send response body to client\n";
            rc = WriteFilter(ct->fd(), *response_, in_);
            if (rc == OK) {
                buf_.reset();
            }
        }
        
        //if (ct.get()) {
        //    // TODO use PhaseHandler::WriteFilter()
        //    n = out_->Write(request_->GetConnection()->fd());
        //    std::cout << request_->GetConnection()->fd()<< " write " << n << " bytes\n";
        //} else {
        //    std::cout << "request closed\n";
        //}

    } else {    // first read event from upstream server
        //rc = ParseStatusLine(out_, response_);
        std::cout << "parsing upstream response\n";
        rc = ParseStatusLine(buf, response_);
        if (rc == AGAIN) {
            return rc;
        }

        if (rc == ERROR) {
            std::cout << "upstream server sent invalid header\n";
            return ERROR;
        }

        rc = ProcessRequestHeaders<ResponsePtr>(buf, response_);
        std::unordered_map<std::string, std::string> h;
        h = response_->Headers();

        if (rc == OK) {
            Request::ConnectionPt ct = request_->GetConnection();

            response_->Set();
            std::cout << "sending headers to client\n";

            rc = PhaseHandler::SendHeaders(request_, response_);
            if (rc == AGAIN) {
                std::cout << "send headers failed.\n";
                ct->EnableWriting(false);
                ct->SetWriteCallback(boost::bind(
                            &PhaseHandler::SendHeaders, this, request_, response_));
                return AGAIN;
            }

            // there is response body in buffer
            if (buf->Pos() != buf->Last() && h["Content-Length"].size()) {

                std::cout << "^^^^^^^^^^^^^^^^ there is response body in buffer\n";

                buf_.reset(new Buffer(8192));
                rc = ReadClientBody(c->request(), std::stoi(h["Content-Length"]), true);
                
                if (rc == OK) {
                    Request::ConnectionPt ct = request_->GetConnection();
                    if (!ct) {
                        std::cout << "Connection closed\n";
                        return OK;
                    }

                    rc = PhaseHandler::WriteFilter(ct->fd(), *response_, in_);
                }
            }

            buf->Reset();
        }

        if (rc == AGAIN) {

        }
    }
    
    out_.reset();
    return OK;
}

int UpstreamHandler::UpstreamTestConnection(Response::ConnectionPtr& c)
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
