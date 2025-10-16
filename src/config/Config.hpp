#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

// comments in English: minimal immutable-like POJOs for parsed configuration.
// Keep fields public for simplicity in the skeleton; in real code expose getters only.

struct LocationConfig {
    std::string path;                     // e.g. "/upload/" (must start with '/')
    std::set<std::string> allowedMethods; // e.g. {"GET","POST","DELETE"}
    std::string root;                     // location override root (optional)
    std::string index;                    // location override index (optional)
    bool        autoindex;                // default false
    std::string returnRedirect;           // e.g. "/files/" (empty means none)
    std::string cgiExtension;             // e.g. ".py" (empty means none)
    std::string uploadStore;              // folder to persist uploads (optional)

    LocationConfig(): autoindex(false) {}
};

struct ServerConfig {
    std::string host;                     // default "0.0.0.0"
    int         port;                     // default 80
    std::string serverName;               // optional
    std::string root;                     // default root
    std::string index;                    // default index file name
    size_t      clientMaxBodySize;        // in bytes; default e.g. 1MB
    std::map<int, std::string> errorPages; // e.g. {404: "/errors/404.html"}
    std::vector<LocationConfig> locations;

    ServerConfig(): host("0.0.0.0"), port(80), clientMaxBodySize(1024*1024) {}
};

struct Config {
    std::vector<ServerConfig> servers;
    // comments in English: helper lookups (very naive for skeleton)
    const ServerConfig* findServer(const std::string& h, int p) const {
        for (size_t i=0;i<servers.size();++i) {
            if (servers[i].host == h && servers[i].port == p) return &servers[i];
        }
        // if none match, return first (default per host:port concept is simplified here)
        return servers.empty() ? NULL : &servers[0];
    }
};

#endif // CONFIG_HPP
