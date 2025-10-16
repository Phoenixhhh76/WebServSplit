#include "Parser.hpp"
#include <cctype>
#include <sstream>
#include <cstdlib>

// ------------------------ Lexer ------------------------

Lexer::Lexer(std::istream& in): _in(in), _line(1), _hasPeek(false) {}

void Lexer::skipSpacesAndComments() {
    while (_in.good()) {
        int c = _in.peek();
        if (c == '\n') { _line++; _in.get(); continue; }
        if (isspace(c)) { _in.get(); continue; }
        if (c == '#') { // comment till end of line
            while (_in.good() && _in.get() != '\n') {}
            _line++;
            continue;
        }
        break;
    }
}

Token Lexer::readString() {
    // assumes starting quote already consumed
    std::string out;
    while (_in.good()) {
        int c = _in.get();
        if (c == std::char_traits<char>::eof()) break;
        if (c == '"') break;
        // no escape handling for skeleton; keep it simple
        out.push_back((char)c);
    }
    return Token(Token::STRING, out, _line);
}

Token Lexer::readIdent() {
    std::string out;
    while (_in.good()) {
        int c = _in.peek();
        if (c == std::char_traits<char>::eof()) break; //if (c == ) break;
        if (isspace(c) || c=='{' || c=='}' || c==';' || c=='"') break;
        out.push_back((char)_in.get());
    }
    return Token(Token::IDENT, out, _line);
}

Token Lexer::next() {
    if (_hasPeek) { _hasPeek=false; return _peekTok; }
    skipSpacesAndComments();
    int c = _in.peek();
    if (!_in.good() || c == std::char_traits<char>::eof()) return Token(Token::END,"",_line);
    if (c=='{') { _in.get(); return Token(Token::LBRACE,"{",_line); }
    if (c=='}') { _in.get(); return Token(Token::RBRACE,"}",_line); }
    if (c==';') { _in.get(); return Token(Token::SEMICOLON,";",_line); }
    if (c=='"') { _in.get(); return readString(); }
    return readIdent();
}

Token Lexer::peek() {
    if (!_hasPeek) { _peekTok = next(); _hasPeek=true; }
    return _peekTok;
}

// ------------------------ Parser ------------------------

Parser::Parser(std::istream& in): _lex(in) { _tok = _lex.next(); }

void Parser::advance() { _tok = _lex.next(); }

bool Parser::accept(Token::Type t) {
    if (_tok.type == t) { advance(); return true; }
    return false;
}

void Parser::expect(Token::Type t, const char* msg) {
    if (!accept(t)) {
        std::ostringstream oss;
        oss << "Parse error at line " << _tok.line << ": expected " << msg;
        throw ParseError(oss.str());
    }
}

bool Parser::isTokenIdent(const Token& t, const char* kw) {
    return t.type == Token::IDENT && t.text == kw;
}

bool Parser::isValidHttpMethod(const std::string& m) {
    return m=="GET" || m=="POST" || m=="DELETE";
}

size_t Parser::parseSizeWithUnit(const std::string& s) {
    // e.g. "10M", "512K", "1G", or plain "12345"
    if (s.empty()) return 0;
    char unit = s[s.size()-1];
    long mult = 1;
    std::string num = s;
    if (unit=='K' || unit=='k') { mult = 1024; num = s.substr(0,s.size()-1); }
    else if (unit=='M' || unit=='m') { mult = 1024L*1024L; num = s.substr(0,s.size()-1); }
    else if (unit=='G' || unit=='g') { mult = 1024L*1024L*1024L; num = s.substr(0,s.size()-1); }
    // very naive strtol; no overflow checks for skeleton
    long n = std::strtol(num.c_str(), NULL, 10);
    if (n < 0) n = 0;
    return (size_t)(n * mult);
}

void Parser::splitHostPort(const std::string& s, std::string& h, int& p) {
    // accept "host:port" or "port"
    size_t pos = s.find(':');
    if (pos == std::string::npos) {
        h = "0.0.0.0";
        p = std::atoi(s.c_str());
    } else {
        h = s.substr(0,pos);
        p = std::atoi(s.substr(pos+1).c_str());
    }
}

// Entry point
Config Parser::parse() {
    Config cfg;
    while (_tok.type != Token::END) {
        if (!isTokenIdent(_tok, "server"))
            throw ParseError("Expected 'server' block");
        parseServer(cfg);
    }
    return cfg;
}

void Parser::parseServer(Config& cfg) {
    advance(); // consume 'server'
    expect(Token::LBRACE, "'{' after server");
    ServerConfig srv;

    while (!accept(Token::RBRACE)) {
        if (isTokenIdent(_tok,"location")) {
            parseLocation(srv);
            continue;
        }
        parseServerDirective(srv);
    }

    cfg.servers.push_back(srv);
}

void Parser::parseServerDirective(ServerConfig& srv) {
    if (isTokenIdent(_tok,"listen")) { dir_listen(srv); return; }
    if (isTokenIdent(_tok,"server_name")) { dir_server_name(srv); return; }
    if (isTokenIdent(_tok,"root")) { dir_root(srv); return; }
    if (isTokenIdent(_tok,"index")) { dir_index(srv); return; }
    if (isTokenIdent(_tok,"error_page")) { dir_error_page(srv); return; }
    if (isTokenIdent(_tok,"client_max_body_size")) { dir_client_max_body_size(srv); return; }

    std::ostringstream oss; oss << "Unknown server directive: " << _tok.text;
    throw ParseError(oss.str());
}

// server directives impl
void Parser::dir_listen(ServerConfig& srv) {
    advance(); // consume 'listen'
    if (_tok.type != Token::IDENT) throw ParseError("listen expects 'host:port' or 'port'");
    std::string host; int port=0; splitHostPort(_tok.text, host, port);
    if (port<=0) throw ParseError("invalid listen port");
    srv.host = host; srv.port = port;
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_server_name(ServerConfig& srv) {
    advance();
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("server_name expects a token");
    srv.serverName = _tok.text;
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_root(ServerConfig& srv) {
    advance();
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("root expects a path");
    srv.root = _tok.text;
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_index(ServerConfig& srv) {
    advance();
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("index expects a file name");
    srv.index = _tok.text;
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_error_page(ServerConfig& srv) {
    advance();
    if (_tok.type != Token::IDENT) throw ParseError("error_page expects code and path");
    int code = std::atoi(_tok.text.c_str());
    advance();
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("error_page expects path");
    srv.errorPages[code] = _tok.text;
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_client_max_body_size(ServerConfig& srv) {
    advance();
    if (_tok.type != Token::IDENT) throw ParseError("client_max_body_size expects size");
    srv.clientMaxBodySize = parseSizeWithUnit(_tok.text);
    advance();
    expect(Token::SEMICOLON, "';'");
}

// location parsing
void Parser::parseLocation(ServerConfig& srv) {
    advance(); // consume 'location'
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("location expects a path");
    LocationConfig loc;
    loc.path = _tok.text;
    advance();
    expect(Token::LBRACE, "'{' after location path");

    while (!accept(Token::RBRACE)) {
        parseLocationDirective(loc);
    }
    srv.locations.push_back(loc);
}

void Parser::parseLocationDirective(LocationConfig& loc) {
    if (isTokenIdent(_tok,"allowed_methods")) { dir_allowed_methods(loc); return; }
    if (isTokenIdent(_tok,"autoindex")) { dir_autoindex(loc); return; }
    if (isTokenIdent(_tok,"return")) { dir_return(loc); return; }
    if (isTokenIdent(_tok,"cgi")) { dir_cgi(loc); return; }
    if (isTokenIdent(_tok,"upload_store")) { dir_upload_store(loc); return; }
    if (isTokenIdent(_tok,"root")) { dir_loc_root(loc); return; }
    if (isTokenIdent(_tok,"index")) { dir_loc_index(loc); return; }

    std::ostringstream oss; oss << "Unknown location directive: " << _tok.text;
    throw ParseError(oss.str());
}

void Parser::dir_allowed_methods(LocationConfig& loc) {
    advance(); // consume keyword
    // read until ';'
    while (_tok.type == Token::IDENT) {
        if (!isValidHttpMethod(_tok.text))
            throw ParseError("allowed_methods: invalid HTTP method");
        loc.allowedMethods.insert(_tok.text);
        advance();
    }
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_autoindex(LocationConfig& loc) {
    advance();
    if (!(_tok.type==Token::IDENT) ) throw ParseError("autoindex on|off");
    if (_tok.text == "on") loc.autoindex = true;
    else if (_tok.text == "off") loc.autoindex = false;
    else throw ParseError("autoindex expects 'on' or 'off'");
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_return(LocationConfig& loc) {
    advance(); // consume 'return'
    // comments in English: support optional status code before path
    std::string first;
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("return expects code or url");
    first = _tok.text;
    advance();

    // if next token looks like a path (starts with '/' or not numeric)
    std::string target;
    if (_tok.type == Token::IDENT || _tok.type == Token::STRING) {
        target = _tok.text;
        advance();
    } else {
        // only one token (no target)
        target = "";
    }

    // store combined string "code path" or only path
    if (!target.empty())
        loc.returnRedirect = first + " " + target;
    else
        loc.returnRedirect = first;

    expect(Token::SEMICOLON, "';'");
}


//this one is not support code path so it's why I add dir_return
void Parser::dir_cgi(LocationConfig& loc) {
    advance();
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("cgi expects extension like .py");
    loc.cgiExtension = _tok.text;
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_upload_store(LocationConfig& loc) {
    advance();
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("upload_store expects a folder path");
    loc.uploadStore = _tok.text;
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_loc_root(LocationConfig& loc) {
    advance();
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("root expects a path");
    loc.root = _tok.text;
    advance();
    expect(Token::SEMICOLON, "';'");
}

void Parser::dir_loc_index(LocationConfig& loc) {
    advance();
    if (_tok.type != Token::IDENT && _tok.type != Token::STRING)
        throw ParseError("index expects a file name");
    loc.index = _tok.text;
    advance();
    expect(Token::SEMICOLON, "';'");
}
