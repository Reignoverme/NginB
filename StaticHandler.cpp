#include "Connection.h"
#include "StaticHandler.h"

int StaticHandler::Handle(Connection::RequestPtr& req,
        Connection::ResponsePtr& res)
{
    typedef std::list<std::string> strlist;

    int fd, rc;
    struct stat sb;
    std::string path;
    std::string uri = req->Uri();

    path = MapUriToPath(uri);

    std::cout << uri << std::endl;
    if (uri[uri.size()-1] == '/') {
        /* find index file under directory 'root'
         * there MUST be an index file under that dir
         * simply return the first index file we found
         */
        for (strlist::const_iterator it = indexes_.begin();
             it != indexes_.end();
             it++) {
            std::cout << "open file " << path+*it << std::endl;
            rc = OpenFile(path + *it, sb, 0, 1);

            if (rc < 0) {
                if (errno == ENOENT) {
                    continue;
                }
            }

            fd = OpenFile(path + *it, sb, O_RDONLY, 0);
            if (fd < 0) {
                std::cout << "open error: " << strerror(errno);
                return HTTP_NOT_FOUND;
            }

            res->SetHeaders("Content-length", std::to_string(sb.st_size));
            res->SetFd(fd);
        }

    } else {
        // find file
        fd = OpenFile(path.c_str(), sb, O_RDONLY, 0); 
        if (fd < 0) {
            // simply return 404

            std::cout << "error: "
                << strerror(errno) << std::endl;

            res->SetStatusCode(HTTP_NOT_FOUND);
            fd = OpenFile(HTTP_404_PAGE, sb, O_RDONLY, 0);

        } else {
            std::cout << "open file: " << path << std::endl;
            res->SetStatusCode(HTTP_OK);
        }
        res->SetFd(fd);
        res->SetHeaders("Content-length", std::to_string(sb.st_size));
    }

    PhaseHandler::SendHeaders(req, res);
    ssize_t n = PhaseHandler::SendBody(res, sb.st_size);
    
    if (n == OK) {
        ::close(fd);
    }

    if (n == AGAIN) {
        std::cout << "send later\n";
    }

    return n;
}

/*
 * @param: test    1: stat() only
 *                 0: open()
 */
int StaticHandler::OpenFile(const std::string& path, struct stat& sb,
        int mode, int test)
{
    int fd, rc;

    rc = stat(path.c_str(), &sb);

    if (test) {
        return rc;
    } else {
        fd = open(path.c_str(), mode, 0);

        if (fd == -1) {
            return -1;
        }

        return fd;
    }
}

std::string StaticHandler::MapUriToPath(const std::string& uri)
{ 
    //TODO a lot...
    //
    return root_ + uri;

}
