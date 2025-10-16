#!/usr/bin/env python3
import os, sys

# comments in English: minimal CGI echo; read stdin by CONTENT_LENGTH
method = os.getenv('REQUEST_METHOD', 'GET')
clen = int(os.getenv('CONTENT_LENGTH', '0') or '0')
body = sys.stdin.read(clen) if clen > 0 else ''

# Output CGI headers + body
print("Status: 200 OK")
print("Content-Type: text/plain")
print()
print("method=", method)
print("query=", os.getenv('QUERY_STRING', ''))
print("path=", os.getenv('PATH_INFO', ''))
print("body=", body)
