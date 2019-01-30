#ifndef INDEX_HANDLER_H
#define INDEX_HANDLER_H

#include <list>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "PhaseHandler.h"

#define HTTP_404_PAGE "/var/www/404.html"
#define HTTP_500_PAGE "/var/www/400.html"

class Response;

class StaticHandler: public PhaseHandler
{
public:
    StaticHandler(EventLoop* loop):
        PhaseHandler(loop),
        root_("/var/www"),
        indexes_({"index.html"})
    {}

    virtual int Handle(Connection::RequestPtr&,
            Connection::ResponsePtr&);

private:
    //TODO put this in config file.
    std::string root_;
    std::list<std::string> indexes_;

    // private methods
    int OpenFile(const std::string&, struct stat&, int, int);
    int FileStat(const std::string&, struct stat&);
    std::string MapUriToPath(const std::string&);

    void SendHeaders(const Request&, const Response&);
};

#endif
