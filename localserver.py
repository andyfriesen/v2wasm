#!/usr/bin/env python3

import sys

if sys.version_info < (3, 0):
    print('Upgrade to Python 3.x already!!!')
    sys.exit(1)

import http.server
import socketserver

PORT = 8000

class Handler(http.server.SimpleHTTPRequestHandler):
    pass

Handler.extensions_map['.wasm'] = 'application/wasm'

httpd = socketserver.TCPServer(("", PORT), Handler)

print("serving at port {0}".format(PORT))
httpd.serve_forever()
