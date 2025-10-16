
#include "HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse(): status(200), reason("OK"), contentType("text/html") {}

std::string HttpResponse::serialize_close() const {
    std::ostringstream o;
    o << "HTTP/1.1 " << status << " " << reason << "\r\n"
      << "Content-Type: " << contentType << "\r\n"
      << "Content-Length: " << body.size() << "\r\n";
    for (std::map<std::string,std::string>::const_iterator it=extra.begin(); it!=extra.end(); ++it){
        o << it->first << ": " << it->second << "\r\n";
    }
    o << "Connection: close\r\n\r\n" << body;
    return o.str();
}
