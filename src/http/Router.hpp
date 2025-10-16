#ifndef ROUTER_HPP
#define ROUTER_HPP
#include "config/Config.hpp"
#include "HttpRequest.hpp"
#include "net/Listener.hpp"

class Router {
public:
    static const ServerConfig& select_server(const Config& cfg,
                                             const Listener& ln,
                                             const HttpRequest& req);
    static const LocationConfig* match_location(const ServerConfig& srv, const std::string& uriPath);
    static bool method_allowed(const LocationConfig* loc, const std::string& method);
};
#endif
