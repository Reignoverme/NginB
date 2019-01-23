#include "Response.h"
#include "Connection.h"

std::unordered_map<std::string, std::string> types =
    {
        { "html", "text/html" },
        { "htm" , "text/html" },
        { "css" , "text/css" },
        { "js"  , "application/javascript" },
        { "jpg" , "image/jpeg" },
        { "jpeg", "image/jpeg" },
        { "png" , "image/png" },
        { "gif" , "image/gif" },
        { "ico" , "image/x-icon" }
    };

void Response::SetHeaders(const u_char* nameStart,
                         const u_char* nameEnd,
                         const u_char* valueStart,
                         const u_char* valueEnd)
{
    std::string name(reinterpret_cast<const char*>(nameStart),
            nameEnd - nameStart);
    std::string value(reinterpret_cast<const char*>(valueStart),
            valueEnd - valueStart);

    headers_[name] = value;
}

void Response::SetStatusLine(const u_char* start,
                             const u_char* end)
{
    statusLine_ = std::string(reinterpret_cast<const char*>(start),
            end - start);
}
