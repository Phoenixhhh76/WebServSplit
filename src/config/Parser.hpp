#ifndef PARSER_HPP
#define PARSER_HPP

#include "Config.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <istream>

// comments in English: a tiny tokenizer and recursive-descent parser for an nginx-like subset.
// Grammar (simplified):
//   config   := { server_block }+
//   server_block := 'server' '{' { server_directive | location_block } '}'
//   location_block := 'location' <path> '{' { location_directive } '}'
//   server_directive := listen | server_name | root | index | error_page | client_max_body_size
//   location_directive := allowed_methods | autoindex | return | cgi | upload_store | root | index
//
// Tokens: IDENT/STRING, '{', '}', ';'
//
// Limitations:
//  - Quoted strings supported with "..." (no escapes).
//  - 'listen' accepts "host:port" or "port".
//  - client_max_body_size supports K/M/G suffix.
//  - error_page: "<code> <path>";
//  - return: "<location>";
//  - cgi: ".ext";
//  - allowed_methods: list of idents.
//
// Throw std::runtime_error on syntax error.

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg): std::runtime_error(msg) {}
};

struct Token {
    enum Type { IDENT, STRING, LBRACE, RBRACE, SEMICOLON, END } type;
    std::string text;
    int line;
    Token(): type(END), line(0) {}
    Token(Type t, const std::string& s, int ln): type(t), text(s), line(ln) {}
};

class Lexer {
public:
    explicit Lexer(std::istream& in);
    Token next();
    Token peek();
private:
    std::istream& _in;
    int _line;
    bool _hasPeek;
    Token _peekTok;
    void skipSpacesAndComments();
    Token readString();
    Token readIdent();
};

class Parser {
public:
    explicit Parser(std::istream& in);
    Config parse();

private:
    Lexer _lex;
    Token _tok;

    // advance helpers
    void advance();
    bool accept(Token::Type t);
    void expect(Token::Type t, const char* msg);

    // grammar
    void parseServer(Config& cfg);
    void parseServerDirective(ServerConfig& srv);
    void parseLocation(ServerConfig& srv);
    void parseLocationDirective(LocationConfig& loc);

    // server directives
    void dir_listen(ServerConfig& srv);
    void dir_server_name(ServerConfig& srv);
    void dir_root(ServerConfig& srv);
    void dir_index(ServerConfig& srv);
    void dir_error_page(ServerConfig& srv);
    void dir_client_max_body_size(ServerConfig& srv);

    // location directives
    void dir_allowed_methods(LocationConfig& loc);
    void dir_autoindex(LocationConfig& loc);
    void dir_return(LocationConfig& loc);
    void dir_cgi(LocationConfig& loc);
    void dir_upload_store(LocationConfig& loc);
    void dir_loc_root(LocationConfig& loc);
    void dir_loc_index(LocationConfig& loc);

    // utilities
    static size_t parseSizeWithUnit(const std::string& s); // e.g. 10M, 512K, 1G
    static bool isTokenIdent(const Token& t, const char* kw);
    static bool isValidHttpMethod(const std::string& m);
    static void splitHostPort(const std::string& s, std::string& h, int& p);
};

#endif // PARSER_HPP
