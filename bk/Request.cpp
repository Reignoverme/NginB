#include "Request.h"


void Request::SetUri(const u_char* start, const u_char* end)
{
    size_t size = end - start;
    uri_ = std::string(reinterpret_cast<const char*>(start), size);
}

void Request::SetHeaders(const u_char* nameStart,
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
