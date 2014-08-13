#!/usr/bin/python
"""
Usage: %(scriptName)s IP:PORT

Receive UDP packets on a IP+PORT location and print the size received.
"""

# Copyright (C) 2013 Faustino Frechilla (frechilla@gmail.com)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import socket, sys

UDP_MAX_LENGTH = 65507

def usage():
    print __doc__ % {'scriptName' : sys.argv[0]}

if __name__ == "__main__":

    if (len(sys.argv) < 2):
        usage()
        sys.exit(-1)

    # parse ip:port tuple
    try:
        (ip, port_str) = sys.argv[1].strip().split(':')
    except:
        usage()
        sys.exit(-2)

    # validate port
    try:
        port = int(port_str)
    except:
        usage()
        sys.exit(-3)

    # validate IP
    try:
        socket.inet_aton(ip)
    except:
        usage()
        sys.exit(-4)

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((ip, port))
    print "Listening on %s:%d" % (ip, port)
    while True:
        (msg, (remote_addr, remote_port)) = s.recvfrom(UDP_MAX_LENGTH)
        print "Received %d bytes of data from %s:%s" % (len(msg), remote_addr, remote_port)
