#include "Connection.h"
#include "PhaseHandler.h"

void PhaseHandler::FinalizeRequest(const Request& req, const Response& res)
{
    
}

int PhaseHandler::SendHeaders(Request& req, Response& res)
{
    int                              connfd;
    size_t                           len, size;
    Buffer*                          b;
    std::string                      value, statusLine;
    boost::shared_ptr<Connection>    c;
    std::unordered_map
        <std::string, std::string>   headers;

    c = req.GetConnection();
    if( !c ) {
        std::cout << "Connection has been closed\n";
        return ERROR;
    }

    len = sizeof("http/1.x ") - 1 + sizeof("\r\n") - 1
          + sizeof("\r\n") - 1;

    switch (res.Status()) {
    case HTTP_OK:
        statusLine = "200 OK"; 
        len += statusLine.size();
        if (req.Ext().size()) {
            value = types[req.Ext()]; 
        }

        if (value.size() != 0) {
            res.SetHeaders(std::string("Content-type"), value);
        } else {
            res.SetHeaders(std::string("Content-type"),
                    std::string("text/plain"));
        }
        
        len += sizeof("Content-type: ") - 1 + value.size() + 2;

        break;

    // on error, return default error page.
    case HTTP_NOT_FOUND:
        statusLine = "404 Not Found";
        len += statusLine.size();
        value = types["html"];
        res.SetHeaders(std::string("Content-type"), value);
        len += sizeof("Content-type: ") - 1 + value.size() + 2;
        break;

    case HTTP_INTERNAL_SERVER_ERROR:
        statusLine = "500 Internal Server Error";
        len += statusLine.size();
        value = types["html"];
        res.SetHeaders(std::string("Content-type"), value);
        len += sizeof("Content-type: ") - 1 + value.size() + 2;
        break;
    }

    if (res.ServerName().size()) {
        res.SetHeaders(std::string("Server"), res.ServerName());
        len += sizeof("Server: ") - 1 + res.ServerName().size();
    }

    len += sizeof("Date: Mon, 28 Sep 1970 06:00:00 GMT\r\n") - 1; 

    headers = res.Headers();
    
    if (headers["Content-length"].size()) {
        len += sizeof("Content-length: ") - 1 +
            sizeof("-9,223,372,036,854,775,808") + 1;
    }

    b = new Buffer(len); 
    
    // HTTP version
    size = sizeof("HTTP/1.1 ") - 1;
    b->SetLast((u_char*)memcpy(b->Last(), "HTTP/1.1 ", size) + size);

    // Status line
    size = statusLine.size();
    b->SetLast(cpystr(b, statusLine.c_str(), size));
    b->SetLast(cpystr(b, "\r\n", 2));

    // Server
    if (headers["Server"].size()) {
        size = res.ServerName().size();
        b->SetLast(cpystr(b, "Server: ", sizeof("Server: ") - 1));
        b->SetLast(cpystr(b, res.ServerName().c_str(), size));
        b->SetLast(cpystr(b, "\r\n", 2));
    }

    // Content-type
    if (headers["Content-type"].size()) {
        size = headers["Content-type"].size();
        b->SetLast(cpystr(b, "Content-type: ", sizeof("Content-type: ") - 1));
        b->SetLast(cpystr(b, headers["Content-type"].c_str(), size));
        b->SetLast(cpystr(b, "\r\n", 2));
    }

    // Content-length
    if (headers["Content-length"].size()) {
        size = headers["Content-length"].size();
        b->SetLast(cpystr(b, "Content-length: ", sizeof("Content-length: ") - 1));
        b->SetLast(cpystr(b, headers["Content-length"].c_str(), size));
        b->SetLast(cpystr(b, "\r\n", 2));
    }

    // Date
    char tmp[128];
    time_t now = time(0);
    struct tm tm  = *gmtime(&now);
    strftime(tmp, sizeof(tmp), "%a, %d %b %Y %H:%M:%S %Z", &tm);
    std::string tptm(tmp);
    b->SetLast(cpystr(b, "Date: ", sizeof("Date: ") - 1));
    b->SetLast(cpystr(b, tptm.c_str(), tptm.size()));
    b->SetLast(cpystr(b, "\r\n\r\n", 4));

    // send header
    ssize_t n = b->Write(c->fd()); 
    if (n < 0) {
        if (n == AGAIN) {
            //TODO 
        }

        if (n == ERROR) {
            //TODO
        }
    }

    delete b;
    std::cout << "sent " << n << " bytes header\n";
}

int PhaseHandler::SendBody(Request& req, Response& res, size_t size)
{
    ssize_t n;
    boost::shared_ptr<Connection> c;
    c = req.GetConnection();

    if (!c) {
        std::cout << "connection closed.\n";
        return ERROR;
    }

    if (res.fd()) {
        n = ::sendfile(c->fd(), res.fd(), 0, size);
    }

    if(n < 0) {
        std::cout << "sendfile error: " << strerror(errno) << std::endl;
        return ERROR;
    }

    return n;
}

