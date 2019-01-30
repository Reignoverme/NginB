#include "Connection.h"
#include "PhaseHandler.h"

int PhaseHandler::SendHeaders(
        Connection::RequestPtr& req,
        Connection::ResponsePtr& res)
{
    typedef std::unordered_map<std::string, std::string> Headers;

    size_t                           len, size;
    Headers                          headers;
    std::string                      value, statusLine;
    Response::Chain                  out;
    boost::shared_ptr<Connection>    c;

    len = 0;

    c = req->GetConnection();
    if (!c) {
        std::cout << "connection closed\n";
        return ERROR;
    }

    if (res->IsSet()) {

        headers = res->Headers();  
        Headers::const_iterator it; 
        
        headers["Server"] = "NginB/0.0.1";
        
        if (!headers["Date"].size()) {
            char tmp[128];
            time_t now = time(0);
            struct tm tm  = *gmtime(&now);
            strftime(tmp, sizeof(tmp), "%a, %d %b %Y %H:%M:%S %Z", &tm);
            std::string tptm(tmp);
            headers["Date"] = tptm;
        }

        // status line
        len += sizeof("HTTP/1.1 ") - 1;
        len += res->StatusLine().size();
        len += sizeof("\r\n") - 1;

        // headers
        for (it = headers.begin();
             it != headers.end();
             it ++) {
            len += it->first.size();
            len += sizeof(": ") - 1;
            len += it->second.size();
            len += sizeof("\r\n") - 1;
        }

        len += sizeof("\r\n") - 1;

        Buffer b = Buffer(len); 

        b.SetLast(cpystr(b, "HTTP/1.1 ", sizeof("HTTP/1.1 ") - 1));
        b.SetLast(cpystr(b, res->StatusLine().c_str(), res->StatusLine().size()));
        b.SetLast(cpystr(b, "\r\n", sizeof("\r\n") - 1));

        for (it = headers.begin();
             it != headers.end();
             it++) {
            b.SetLast(cpystr(b, it->first.c_str(), it->first.size()));
            b.SetLast(cpystr(b, ": ", sizeof(": ") - 1));
            b.SetLast(cpystr(b, it->second.c_str(), it->second.size()));
            b.SetLast(cpystr(b, "\r\n", sizeof("\r\n") - 1));
        }

        b.SetLast(cpystr(b, "\r\n", sizeof("\r\n") - 1));

        //std::cout << "------ send back headers ------\n";
        //u_char *a = b.Start();

        //for( ; a!=b.Last(); a++ ) {
        //    std::cout << *a;
        //}

        out.push_back(b);

    } else {

        len = sizeof("http/1.x ") - 1 + sizeof("\r\n") - 1
              + sizeof("\r\n") - 1;

        switch (res->StatusCode()) {
        case HTTP_OK:
            statusLine = "200 OK"; 
            len += statusLine.size();
            if (req->Ext().size()) {
                value = types[req->Ext()]; 
            }

            if (value.size() != 0) {
                res->SetHeaders(std::string("Content-type"), value);
            } else {
                res->SetHeaders(std::string("Content-type"),
                        std::string("text/plain"));
            }
            
            len += sizeof("Content-type: ") - 1 + value.size() + 2;

            break;

        // on error, return default error page.
        case HTTP_NOT_FOUND:
            statusLine = "404 Not Found";
            len += statusLine.size();
            value = types["html"];
            res->SetHeaders(std::string("Content-type"), value);
            len += sizeof("Content-type: ") - 1 + value.size() + 2;
            break;

        case HTTP_INTERNAL_SERVER_ERROR:
            statusLine = "500 Internal Server Error";
            len += statusLine.size();
            value = types["html"];
            res->SetHeaders(std::string("Content-type"), value);
            len += sizeof("Content-type: ") - 1 + value.size() + 2;
            break;
        }

        if (res->ServerName().size()) {
            res->SetHeaders(std::string("Server"), res->ServerName());
            len += sizeof("Server: ") - 1 + res->ServerName().size();
        }

        len += sizeof("Date: Mon, 28 Sep 1970 06:00:00 GMT\r\n") - 1; 

        headers = res->Headers();
        
        if (headers["Content-length"].size()) {
            len += sizeof("Content-length: ") - 1 +
                sizeof("-9,223,372,036,854,775,808") + 1;
        }

        Buffer b = Buffer(len); 
        
        // HTTP version
        size = sizeof("HTTP/1.1 ") - 1;
        b.SetLast((u_char*)memcpy(b.Last(), "HTTP/1.1 ", size) + size);

        // Status line
        size = statusLine.size();
        b.SetLast(cpystr(b, statusLine.c_str(), size));
        b.SetLast(cpystr(b, "\r\n", 2));

        // Server
        if (headers["Server"].size()) {
            size = res->ServerName().size();
            b.SetLast(cpystr(b, "Server: ", sizeof("Server: ") - 1));
            b.SetLast(cpystr(b, res->ServerName().c_str(), size));
            b.SetLast(cpystr(b, "\r\n", 2));
        }

        // Content-type
        if (headers["Content-type"].size()) {
            size = headers["Content-type"].size();
            b.SetLast(cpystr(b, "Content-type: ", sizeof("Content-type: ") - 1));
            b.SetLast(cpystr(b, headers["Content-type"].c_str(), size));
            b.SetLast(cpystr(b, "\r\n", 2));
        }

        // Content-length
        if (headers["Content-length"].size()) {
            size = headers["Content-length"].size();
            b.SetLast(cpystr(b, "Content-length: ", sizeof("Content-length: ") - 1));
            b.SetLast(cpystr(b, headers["Content-length"].c_str(), size));
            b.SetLast(cpystr(b, "\r\n", 2));
        }

        // Date
        char tmp[128];
        time_t now = time(0);
        struct tm tm  = *gmtime(&now);
        strftime(tmp, sizeof(tmp), "%a, %d %b %Y %H:%M:%S %Z", &tm);
        std::string tptm(tmp);
        b.SetLast(cpystr(b, "Date: ", sizeof("Date: ") - 1));
        b.SetLast(cpystr(b, tptm.c_str(), tptm.size()));
        b.SetLast(cpystr(b, "\r\n\r\n", 4));

        //std::cout << "------ send back headers ------\n";
        //u_char *a = b.Start();

        //for( ; a!=b.Last(); a++ ) {
        //    std::cout << *a;
        //}

        out.push_back(b);
    }

    
    int rc = WriteFilter(c->fd(), *res, out);
    if (rc == OK) {
        c->EnableReading(true);
    }

    return rc;
}

int PhaseHandler::SendHeaders(int fd, Response& res, Response::Chain& in)
{
    int rc = WriteFilter(fd, res, in);
    return rc;
}

int PhaseHandler::WriteFilter(int fd, Response& res, Response::Chain& in)
{
    
    int                                rc;
    boost::shared_ptr<Connection>      c;
    Response::Chain::const_iterator    cl, ln;

    Response::Chain& out = res.out();

    c = res.GetConnection();
    if (!c) {
        std::cout << "connection closed\n";
        return ERROR;
    }

    for ( cl = out.begin(); cl != out.end(); cl++ ) {
        std::cout << "write old buf" 
            << "pos: " << cl->Pos()
            << "size: " << cl->Last() - cl->Pos() << std::endl;
    }

    for ( ln = in.begin(); ln != in.end(); ln++ ) {
        out.push_back(*ln);

        std::cout << "write new buf\n"
            << "pos: " << ln->Pos() - ln->Start() << std::endl
            << "size: " << ln->Last() - ln->Pos() << std::endl;
        //for (u_char* a = ln->Pos(); a < ln->Last(); a++) {
        //    std::cout << *a;
        //}
    }

    rc = WritevChain(fd, out);

    return rc; 
}

int PhaseHandler::WritevChain(int fd, Response::Chain& out)
{
    off_t                   send, prevSend;
    ssize_t                 n, sent;
    Response::Iovec         iovs;

    send = 0;

    for ( ;; ) {
        prevSend = send;

        send += OutputChainToIovec(iovs, out); 

        n = Writev(fd, iovs); 

        if (n == ERROR) {
            return ERROR;
        }

        sent = (n == AGAIN) ? 0 : n;

        UpdateChainAfterSend(out, sent);

        if (send - prevSend != sent) {
            std::cout << "send: " << send
                << " prev send: " << prevSend
                << " sent: " << send << std::endl;

            return AGAIN;
        }

        if (out.size() == 0) {
            std::cout << "writev " << n << " bytes\n";
            std::cout << "out.size == 0\n";

            return OK;
        }
    }
}

void PhaseHandler::UpdateChainAfterSend(Response::Chain& out,
        ssize_t sent)
{
    off_t                       size;
    Response::Chain             in;
    Response::Chain::iterator   it;

    for (it = out.begin(); it != out.end(); it++) {
        if (sent == 0) {
            break;
        }

        size = it->Last() - it->Pos();

        if (sent >= size) {
            sent -= size;
            continue;
        }

        it->SetPos(it->Pos() + (size_t)sent);
        break;
    }

    out.erase(out.begin(), it);
}

ssize_t PhaseHandler::Writev(int fd, Response::Iovec& iovs)
{
    ssize_t n;

intr:
    n = writev(fd, &iovs[0], iovs.size());

    if (n == -1) {
        switch (errno) {
        case EAGAIN:
            std::cout << "writev(2) not ready\n";
            return AGAIN;

        case EINTR:
            std::cout << "writev(2) was interrupted\n";
            goto intr;
        
        default:
            std::cout << "write(2) failed: "
                << strerror(errno) << std::endl;
            return ERROR;
        }
    }

    return n;
}

size_t PhaseHandler::OutputChainToIovec(Response::Iovec& iovs,
        Response::Chain& out)
{
    int                                n;
    u_char                             *prev;
    size_t                             size, total;
    struct iovec                       iov;
    Response::Chain::iterator    it;

    n = 0;
    total = 0;
    iov = {};
    prev = nullptr;

    for ( it = out.begin(); it != out.end(); it++ ) {
        size = it->Last() - it->Pos(); 

        if (prev == it->Pos()) {
            std::cout << "it occurs\n";
            iov.iov_len += size;

        } else {
            if (n == 16) {
                break;
            }

            iov.iov_base = (void *)it->Pos();
            iov.iov_len = size;

            iovs.push_back(iov);
        }

        prev = it->Pos() + size;
        total += size;
    }

    return total;
}

int PhaseHandler::SendBody(Connection::ResponsePtr& res, size_t size)
{
    //TODO!!!!!
    //
    // add write event to epoll
    // done: 2019/1/28
    
    off_t   offset;
    ssize_t n;
    boost::shared_ptr<Connection> c;
    c = res->GetConnection();

    n = 0;
    offset = 0;

    if (!c) {
        std::cout << "connection closed.\n";
        return ERROR;
    }

    if (res->fd()) {
        n = ::sendfile(c->fd(), res->fd(), 0, size);

        if(n < 0) {
            if (errno == EAGAIN) {
                std::cout << "sendfile(2) not ready.\n";
                c->EnableWriting(false);
                return AGAIN;
            }

            std::cout << "sendfile error: " << strerror(errno) << std::endl;
            return ERROR;
        }

        if ((size_t)n < size) {
            std::cout << c->fd() <<" sendfile(2) not complete\n";
            std::cout << "sendfild(2) send " << n << " bytes, remaining "
                << size - n << " bytes\n";
            c->EnableWriting(false);
            c->SetWriteCallback(boost::bind(&PhaseHandler::SendBody,
                        this, res, size - n));
            return AGAIN;
        }

        std::cout << c->fd() << " sendfile(2) complete, send " << n << " bytes\n";
        c->Disable(); c->EnableReading(true);
        c->buf()->Reset();
        c->SetReadCallback(boost::bind(&Connection::handleRead, c.get()));
        return OK;

    } else {
        std::cout << "sendbody: file not open\n";
        return ERROR;
    }
}

