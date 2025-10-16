#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP
#include <string>
#include <map>
// Minimal HTTP/1.1 request head (no body parsing yet).
struct HttpRequest {
    std::string method, target, version;
    std::map<std::string,std::string> headers; // lowercase keys
    std::string rawHead;

    bool parse_head(const std::string& in);     // fill fields when CRLFCRLF found
    static std::map<std::string,std::string> parse_headers(const std::string& req);
};
#endif
