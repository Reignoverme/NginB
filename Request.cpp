#include "Request.h"

void Request::SetField(const u_char* start, const u_char* end, uint8_t field)
{
    size_t size = end - start;

    switch (field) {
    case URI_FIELD:
        uri_ = std::string(reinterpret_cast<const char*>(start), size);
        break;
    case EXT_FIELD:
        ext_ = std::string(reinterpret_cast<const char*>(start), size);
        break;
    default:
        std::cout << "invalid extension.\n";
    }
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
