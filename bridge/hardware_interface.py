#!/usr/bin/env python3

from http.server import (
    HTTPServer,
    BaseHTTPRequestHandler
)
import time

import serial

spacelock = serial.Serial('/dev/ttyS0', baudrate=9600)

def send_code(code):
    print("sending code: %r" % (code,))
    spacelock.write((code + '\n').encode())
    time.sleep(0.1)
    reply = spacelock.read(spacelock.in_waiting).decode(errors='replace')
    print("reply: %r" % (reply,))
    return reply

class RequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        result = ''
        try:
            if self.path.startswith('/send/'):
                result = send_code(self.path[6:])
        except BaseException as exc:
            result = str(exc)
            self.send_response(503)
        else:
            self.send_response(200)

        self.send_header("Content-type", "text")
        self.end_headers()
        self.wfile.write(result.encode('utf8', errors='replace'))

if __name__ == '__main__':
    httpd = HTTPServer(('', 8000), RequestHandler)
    httpd.serve_forever()

