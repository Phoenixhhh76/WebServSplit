#include "StaticHandler.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>

bool StaticHandler::path_exists(const std::string& p){ struct stat st; return ::stat(p.c_str(),&st)==0; }
bool StaticHandler::is_dir(const std::string& p){ struct stat st; if(::stat(p.c_str(),&st)!=0) return false; return S_ISDIR(st.st_mode); }

bool StaticHandler::read_file_ok(const std::string& p, std::string& out){
    std::ifstream in(p.c_str(), std::ios::in|std::ios::binary); if(!in) return false;
    std::ostringstream ss; ss<<in.rdbuf(); out=ss.str(); return true;
}

const char* StaticHandler::guess_mime(const std::string& path){
    std::string::size_type dot = path.rfind('.');
    if (dot==std::string::npos) return "application/octet-stream";
    std::string ext = path.substr(dot+1);
    if (ext=="html"||ext=="htm") return "text/html";
    if (ext=="css")  return "text/css";
    if (ext=="js")   return "application/javascript";
    if (ext=="json") return "application/json";
    if (ext=="png")  return "image/png";
    if (ext=="jpg"||ext=="jpeg") return "image/jpeg";
    if (ext=="gif")  return "image/gif";
    if (ext=="svg")  return "image/svg+xml";
    if (ext=="ico")  return "image/x-icon";
    if (ext=="txt")  return "text/plain";
    return "application/octet-stream";
}

std::string StaticHandler::build_autoindex(const std::string& uriPath, const std::string& fsDir){
    std::ostringstream html;
    html<<"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Index of "
        <<uriPath<<"</title></head><body><h1>Index of "<<uriPath<<"</h1><ul>";
    DIR* d = ::opendir(fsDir.c_str());
    if (d){
        struct dirent* de;
        while ((de=::readdir(d))!=0){
            const char* name = de->d_name;
            if (std::strcmp(name,".")==0) continue;
            std::string link = uriPath;
            if (link.empty() || link[link.size()-1]!='/') link+="/";
            link += name;
            html<<"<li><a href=\""<<link<<"\">"<<name<<"</a></li>";
        }
        ::closedir(d);
    }
    html<<"</ul></body></html>";
    return html.str();
}

std::string StaticHandler::normalize_noindex(const std::string& root, const std::string& uri){
    std::string p=uri;
    size_t q = p.find('?'); if (q!=std::string::npos) p=p.substr(0,q);
    if (p.empty() || p[0]!='/') p="/"+p;
    std::vector<std::string> segs;
    for (size_t i=0;i<p.size();){
        while(i<p.size() && p[i]=='/') ++i;
        size_t j=i; while(j<p.size() && p[j]!='/') ++j;
        if (j>i){
            std::string s = p.substr(i,j-i);
            if (s==".."){ if(!segs.empty()) segs.pop_back(); }
            else if (s!=".") segs.push_back(s);
        }
        i=j;
    }
    std::string path=root;
    for(size_t k=0;k<segs.size();++k){ path+="/"; path+=segs[k]; }
    return path;
}

void StaticHandler::resolve_fs(const ServerConfig& srv, const LocationConfig* loc,
                               std::string& root, std::string& index, std::string& err404){
    root  = (loc && !loc->root.empty()) ? loc->root : srv.root;
    index = (loc && !loc->index.empty())? loc->index : srv.index;
    std::map<int,std::string>::const_iterator e = srv.errorPages.find(404);
    if (e!=srv.errorPages.end()) err404 = srv.root + e->second;
    else err404 = "";
}

void StaticHandler::handle_get(const ServerConfig& srv,
                               const LocationConfig* loc,
                               const std::string& target,
                               HttpResponse& resp){
    std::string root, index, err404;
    resolve_fs(srv,loc,root,index,err404);
    if (root.empty()) root=".";
    if (index.empty()) index="index.html";

    std::string fsBase = normalize_noindex(root, target);
    if (is_dir(fsBase)){
        std::string fsIndex = fsBase;
        if (!fsIndex.empty() && fsIndex[fsIndex.size()-1]!='/') fsIndex+="/";
        fsIndex += index;

        std::string body;
        if (read_file_ok(fsIndex, body)){
            resp.status=200; resp.reason="OK"; resp.body=body; resp.contentType=guess_mime(fsIndex);
        } else {
            bool ai = (loc && loc->autoindex);
            if (ai){
                std::string uri = target; size_t q=uri.find('?'); if(q!=std::string::npos) uri=uri.substr(0,q);
                if (uri.empty() || uri[uri.size()-1]!='/') uri+="/";
                resp.status=200; resp.reason="OK";
                resp.contentType="text/html";
                resp.body = build_autoindex(uri, fsBase);
            } else {
                std::string epage;
                if (!err404.empty()) read_file_ok(err404, epage);
                if (epage.empty()) epage = "<h1>404 Not Found</h1>";
                resp.status=404; resp.reason="Not Found"; resp.body=epage; resp.contentType="text/html";
            }
        }
    } else {
        std::string body;
        if (!read_file_ok(fsBase, body)){
            std::string epage;
            if (!err404.empty()) read_file_ok(err404, epage);
            if (epage.empty()) epage = "<h1>404 Not Found</h1>";
            resp.status=404; resp.reason="Not Found"; resp.body=epage; resp.contentType="text/html";
        } else {
            resp.status=200; resp.reason="OK"; resp.body=body; resp.contentType=guess_mime(fsBase);
        }
    }
}
