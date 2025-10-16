#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP
#include <string>
#include <map>

struct HttpResponse {
    int         status;
    std::string reason;
    std::string contentType;
    std::string body;
    std::map<std::string,std::string> extra; // Location, Allow...

    HttpResponse();
    std::string serialize_close() const;     // build with Connection: close
};
#endif
