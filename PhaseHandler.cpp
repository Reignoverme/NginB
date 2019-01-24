#include "Connection.h"
#include "PhaseHandler.h"

int PhaseHandler::SendHeaders(Request& req, Response& res)
{
    typedef std::unordered_map<std::string, std::string> Headers;
    size_t                           len, size;
    Buffer*                          b;
    std::string                      value, statusLine;
    boost::shared_ptr<Connection>    c;
    Headers                          headers;

    len = 0;

    c = req.GetConnection();
    if (!c) {
        std::cout << "connection closed\n";
        return ERROR;
    }

    if (res.IsSet()) {

        headers = res.Headers();  
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
        len += res.StatusLine().size();
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

        b = new Buffer(len); 

        b->SetLast(cpystr(b, "HTTP/1.1 ", sizeof("HTTP/1.1 ") - 1));
        b->SetLast(cpystr(b, res.StatusLine().c_str(), res.StatusLine().size()));
        b->SetLast(cpystr(b, "\r\n", sizeof("\r\n") - 1));

        for (it = headers.begin();
             it != headers.end();
             it++) {
            b->SetLast(cpystr(b, it->first.c_str(), it->first.size()));
            b->SetLast(cpystr(b, ": ", sizeof(": ") - 1));
            b->SetLast(cpystr(b, it->second.c_str(), it->second.size()));
            b->SetLast(cpystr(b, "\r\n", sizeof("\r\n") - 1));
        }

        b->SetLast(cpystr(b, "\r\n", sizeof("\r\n") - 1));

    } else {

        len = sizeof("http/1.x ") - 1 + sizeof("\r\n") - 1
              + sizeof("\r\n") - 1;

        switch (res.StatusCode()) {
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

    }

    std::cout << "------ send back headers ------\n";
    u_char *a = b->Start();

    for( ; a!=b->Last(); a++ ) {
        std::cout << *a;
    }

    // send header
    ssize_t n = b->Write(c->fd()); 
    if (n < 0) {
        if (n == AGAIN) {
            //TODO 
            std::cout << "write(2) not ready\n";
            return AGAIN;
        }

        if (n == ERROR) {
            //TODO
            std::cout << "write(2) error\n";
            return ERROR;
        }
    }

    delete b;
    std::cout << c->fd() << " sent " << n << " bytes header\n";
    return OK;
}

int PhaseHandler::SendBody(Request& req, Response& res, size_t size)
{
    ssize_t n;
    boost::shared_ptr<Connection> c;
    c = req.GetConnection();

    n = 0;

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

