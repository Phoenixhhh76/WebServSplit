#include "Connection.hpp"
#include <sstream>
#include <cctype>

void Connection::attach(int cfd, int lfd){
    _fd=cfd; _listen_fd=lfd; _st=RECV_HEADERS; _in.clear(); _out.clear();
}

void Connection::on_readable(const Config& cfg, const std::map<int,Listener*>& fd2listener){
    char buf[4096];
    for(;;){
        ssize_t r = ::recv(_fd, buf, sizeof(buf), 0);
        if (r > 0){ _in.append(buf, r); continue; }
        if (r == 0){ _st=CLOSED; break; }
        // r < 0: per subject, do not inspect errno; stop for now
        break;
    }
    if (_st==CLOSED) return;

    if (_st==RECV_HEADERS){
        HttpRequest req;
        if (!req.parse_head(_in)) return; // not complete yet

        // HTTP/1.1 must have Host
        if (req.version=="HTTP/1.1" && req.headers.find("host")==req.headers.end()){
            HttpResponse res; res.status=400; res.reason="Bad Request"; res.body="<h1>400 Bad Request</h1>";
            _out = res.serialize_close(); _st=SEND_RESPONSE; return;
        }

        const Listener* ln = fd2listener.find(_listen_fd)->second;
        const ServerConfig& srv = Router::select_server(cfg, *ln, req);
        std::string pathOnly = req.target;
        size_t q = pathOnly.find('?'); if (q!=std::string::npos) pathOnly=pathOnly.substr(0,q);
        const LocationConfig* loc = Router::match_location(srv, pathOnly);

        // return redirect if present
        if (loc && !loc->returnRedirect.empty()){
            int code=302; std::string url=loc->returnRedirect;
            if (!url.empty() && std::isdigit(url[0])){
                std::istringstream ss(url); ss>>code; ss>>url; if (url.empty()) url="/";
            }
            HttpResponse res; res.status=code; res.reason=(code==301?"Moved Permanently":"Found");
            res.extra["Location"]=url; res.body="";
            _out = res.serialize_close(); _st=SEND_RESPONSE; return;
        }

        // method allowed?
        if (!Router::method_allowed(loc, req.method)){
            HttpResponse res; res.status=405; res.reason="Method Not Allowed";
            std::set<std::string> allow;
            if (loc && !loc->allowedMethods.empty()) allow=loc->allowedMethods; else allow.insert("GET");
            std::ostringstream ah; for (std::set<std::string>::iterator it=allow.begin(); it!=allow.end();){
                ah<<*it; ++it; if (it!=allow.end()) ah<<", ";
            }
            res.extra["Allow"]=ah.str();
            res.body="<h1>405 Method Not Allowed</h1>";
            _out = res.serialize_close(); _st=SEND_RESPONSE; return;
        }

        if (req.method!="GET"){
            HttpResponse res; res.status=501; res.reason="Not Implemented";
            res.body="<h1>501 Not Implemented</h1>";
            _out = res.serialize_close(); _st=SEND_RESPONSE; return;
        }

        // Static GET / autoindex
        HttpResponse res;
        StaticHandler::handle_get(srv, loc, req.target, res);
        _out = res.serialize_close(); _st=SEND_RESPONSE;
    }
}

void Connection::on_writable(){
    if (_out.empty()) return;
    ssize_t w = ::send(_fd, _out.data(), _out.size(), 0);
    if (w > 0) _out.erase(0, (size_t)w);
    else {
        // w < 0: per subject, do not inspect errno; stop for now
        return;
    }
    if (_out.empty()) _st=CLOSED; // close-after-response
}
