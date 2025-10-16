#include "HttpRequest.hpp"
#include <sstream>

std::map<std::string,std::string> HttpRequest::parse_headers(const std::string& req){
    std::map<std::string,std::string> h;
    size_t pos = req.find("\r\n"); if (pos==std::string::npos) return h;
    size_t i = pos+2;
    while (true){
        size_t e = req.find("\r\n", i);
        if (e==std::string::npos) break;
        if (e==i) break;
        std::string line = req.substr(i,e-i);
        size_t c = line.find(':');
        if (c!=std::string::npos){
            std::string k = line.substr(0,c);
            std::string v = line.substr(c+1);
            while(!v.empty() && (v[0]==' '||v[0]=='\t')) v.erase(0,1);
            for (size_t j=0;j<k.size();++j) if(k[j]>='A'&&k[j]<='Z') k[j]=char(k[j]-'A'+'a');
            h[k]=v;
        }
        i = e+2;
    }
    return h;
}

bool HttpRequest::parse_head(const std::string& in){
    size_t hdrEnd = in.find("\r\n\r\n");
    if (hdrEnd==std::string::npos) return false;
    rawHead = in.substr(0,hdrEnd+4);
    size_t crlf = rawHead.find("\r\n");
    if (crlf==std::string::npos) return false;
    std::istringstream l(rawHead.substr(0,crlf));
    if (!(l >> method >> target >> version)) return false;
    headers = parse_headers(rawHead);
    return true;
}