#ifndef INDEX_HANDLER_H
#define INDEX_HANDLER_H
#include "PhaseHandler.h"

class IndexHandler: public PhaseHandler
{
public:
    IndexHandler()
    virtual int Handle();
    std::string MapUriToPath(const std::string&);
private:
    std::list<std::string> indexs_ {"index.html"};
}
