#include "Router.hpp"

const ServerConfig& Router::select_server(const Config& cfg,
                                          const Listener& ln,
                                          const HttpRequest& req){
    std::string onlyHost;
    std::map<std::string,std::string>::const_iterator it = req.headers.find("host");
    if (it != req.headers.end()){
        onlyHost = it->second;
        size_t c = onlyHost.find(':'); if (c!=std::string::npos) onlyHost = onlyHost.substr(0,c);
    }
    for (size_t i=0;i<ln.serverIdx.size();++i){
        const ServerConfig& s = cfg.servers[ ln.serverIdx[i] ];
        if (!s.serverName.empty() && s.serverName == onlyHost) return s;
    }
    return cfg.servers[ ln.serverIdx[0] ];
}

const LocationConfig* Router::match_location(const ServerConfig& srv, const std::string& uriPath){
    const LocationConfig* best = 0; size_t bestLen=0;
    for (size_t i=0;i<srv.locations.size();++i){
        const LocationConfig& L = srv.locations[i];
        const std::string& p = L.path;
        if (p.empty()) continue;
        if (uriPath.size() < p.size()) continue;
        if (uriPath.compare(0,p.size(),p)!=0) continue;
        if (uriPath.size()>p.size() && uriPath[p.size()]!='/') continue;
        if (p.size() > bestLen){ best=&L; bestLen=p.size(); }
    }
    return best;
}

bool Router::method_allowed(const LocationConfig* loc, const std::string& method){
    if (!loc) return method=="GET";
    if (loc->allowedMethods.empty()) return method=="GET";
    return loc->allowedMethods.count(method)!=0;
}
