// Serve static files or build autoindex HTML. No blocking beyond filesystem syscalls.

#ifndef STATIC_HANDLER_HPP
#define STATIC_HANDLER_HPP
#include "config/Config.hpp"
#include "HttpResponse.hpp"
#include <string>

struct StaticHandler {
    static bool path_exists(const std::string& p);
    static bool is_dir(const std::string& p);
    static bool read_file_ok(const std::string& p, std::string& out);
    static const char* guess_mime(const std::string& path);
    static std::string build_autoindex(const std::string& uriPath, const std::string& fsDir);
    static std::string normalize_noindex(const std::string& root, const std::string& uri);
    static void resolve_fs(const ServerConfig& srv, const LocationConfig* loc,
                           std::string& root, std::string& index, std::string& err404);
    static void handle_get(const ServerConfig& srv,
                           const LocationConfig* loc,
                           const std::string& target,
                           HttpResponse& resp); // fills status/reason/body/ctype/extra
};
#endif
