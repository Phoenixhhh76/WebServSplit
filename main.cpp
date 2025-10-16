// comments in English only inside code.
#include "config/Parser.hpp"
#include "config/Config.hpp"
#include "app/HttpServer.hpp"
#include "net/Listener.hpp"

#include <map>
#include <sstream>
#include <fstream>
#include <iostream>

// Plan struct without Fd (copyable)
struct ListenPlan {
    std::string host;
    int         port;
    std::vector<size_t> serverIdx;
};

int main(int argc, char** argv){
    if (argc < 2){
        std::cerr<<"Usage: "<<(argc>0?argv[0]:"webserv")<<" <config.conf>\n";
        return 1;
    }
    std::ifstream in(argv[1]);
    if (!in){ std::cerr<<"Cannot open config file: "<<argv[1]<<"\n"; return 1; }

    Config cfg;
    try { Parser p(in); cfg = p.parse(); }
    catch (const ParseError& e){ std::cerr<<"Config parse error: "<<e.what()<<"\n"; return 2; }

    if (cfg.servers.empty()){ std::cerr<<"No server blocks defined in configuration.\n"; return 1; }

    // group unique host:port into copyable plans (no Fd here)
    std::map<std::string, ListenPlan> plans;
    for (size_t i=0;i<cfg.servers.size();++i){
        const ServerConfig& s = cfg.servers[i];
        std::ostringstream key; key<<s.host<<":"<<s.port;
        if (plans.find(key.str())==plans.end()){
            ListenPlan pl; pl.host=s.host; pl.port=s.port;
            plans[key.str()] = pl; // copyable
        }
        plans[key.str()].serverIdx.push_back(i);
    }

    // open listeners as pointers to avoid copying RAII Fd
    std::vector<Listener*> listeners;
    listeners.reserve(plans.size());
    for (std::map<std::string,ListenPlan>::iterator it=plans.begin(); it!=plans.end(); ++it){
        ListenPlan &pl = it->second;
        Listener* ln = new Listener();
        if (!ln->open_and_listen(pl.host, pl.port)){ std::perror("listen"); delete ln; return 1; }
        ln->serverIdx = pl.serverIdx;
        listeners.push_back(ln);
    }

    // info
    std::cout<<"[INFO] Listening sockets:\n";
    for (size_t i=0;i<listeners.size();++i){
        std::cout<<"  - "<<listeners[i]->host<<":"<<listeners[i]->port
                 <<" (servers="<<listeners[i]->serverIdx.size()<<")\n";
    }

    // start server loop
    HttpServer srv;
    if (!srv.start_listeners(listeners)) return 1;
    srv.loop(cfg);

    // not reached in this MVP; if you add a stop path, remember to delete
    for (size_t i=0;i<listeners.size();++i) delete listeners[i];
    return 0;
}

