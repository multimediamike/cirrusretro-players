#!/usr/bin/python

import BaseHTTPServer
import getopt
import os
import socket
import sys

DEFAULT_HTTP_PORT = 8000

INDEX_PAGE = "play.html"

class CRTestServerHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    def do_GET(self):
        response = 200
        gzip = False
        contentType = None
        page = None

        # remove leading '/'
        path = self.path[1:]

        if path == '':
            # serve up the base static page
            contentType = "text/html"
            page = INDEX_PAGE
        elif path.endswith(".js"):
            contentType = "application/javascript"
            page = path
        elif path.endswith(".jsgz"):
            contentType = "application/javascript"
            gzip = True
            page = path
        elif os.path.exists(path):
            contextType = "application/octet-stream"
            page = path
        else:
            response = 404

        self.send_response(response)
        if response == 200:
            if gzip:
                self.send_header("Content-encoding", "gzip")
            self.send_header("Content-type", contentType)
            self.end_headers()
            self.wfile.write(open(page).read())
        else:
            self.end_headers()

def run(port,
        server_class=BaseHTTPServer.HTTPServer,
        handler_class=CRTestServerHandler):
    server_address = ('', port)
    try:
        httpd = server_class(server_address, handler_class)
    except socket.error:
        print "port %d is already is use" % (port)
        return
    print "Cirrus Retro test server running on port %d... (Ctrl-C to exit)" % (port)
    httpd.serve_forever()


def usage():
    print """USAGE: ghettorss-server.py <options>
  -h, --help: Print this message
  -p, --port=[number]: Port on which to run the HTTP server
"""

if __name__ == '__main__':

    port = DEFAULT_HTTP_PORT
    # process command line arguments
    opts, args = getopt.getopt(sys.argv[1:], "hp:", ["help", "port="])

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit()
        elif opt in ("-p", "--port"):
            try:
                port = int(arg)
            except ValueError:
                print "Invalid port number"
                sys.exit()

    print """
*** WARNING! Listening on all interfaces!
*** This will expose parts of your filesystem to anyone who cares to
*** access it with a web browser!
"""

    # kick off the server
    run(port)
