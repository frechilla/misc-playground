#!/usr/bin/python
"""
Usage: %(scriptName)s [options] IP:PORT

Send UDP packets random in content and size to an IP+PORT location. 
Compatible with Python >= 2.6

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

import struct
import socket
import sys
import random
import array
import time
from optparse import OptionParser, OptionGroup

##############################################################################
# Some constants that could be turned into parameters
#
# For the moment the SRC address is set by the kernel. Can't make it work
DEFAULT_SRC_IP = '0.0.0.0' # SRC_IP won't be used (set to the sender's IP)
DEFAULT_SRC_PORT = 0 # If it is 0 it'll be set to DST_PORT when using raw sockets

IP_HEADER_SIZE = 20
UDP_HEADER_SIZE = 8
DEFAULT_MAX_SIZE = 65535
DEFAULT_MTU = 1500
# print a status msg every DEFAULT_PRINT_OUT_PERIOD messages have been sent
DEFAULT_PRINT_OUT_PERIOD = 1
DEFAULT_USECS_INTERVAL = 0 # default to no wait between messages
USEC_IN_A_SEC = 1000000.0



##############################################################################
# Functions 
#

def build_ip_header(src_ip, dst_ip):
    """Builds a valid IP header and returns it

    Parameters:
      - src_ip: A string with a valid IP address which will be used as 
                SRC IP
      - dst_ip: A string with a valid IP address where the packets will be
                sent to
    
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Version|  IHL  |Type of Service|          Total Length         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |         Identification        |Flags|      Fragment Offset    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Time to Live |    Protocol   |         Header Checksum       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       Source Address                          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                    Destination Address                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                    Options                    |    Padding    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    """

    ihl = 5
    version = 4
    ip_ihl_ver = (version << 4) | ihl
    ip_tos = 0
    ip_tot_len = 0  # kernel will fill the correct total length
    ip_id = 0xbeef  # Id of this packet
    ip_frag_off = 0
    ip_ttl = 255
    ip_proto = socket.IPPROTO_UDP
    ip_check = 0    # kernel will fill the correct checksum
    ip_saddr = socket.inet_aton (src_ip)  # Spoof the src IP if you want to
    ip_daddr = socket.inet_aton (dst_ip)

    # the ! in the pack format string means network order
    #   see http://docs.python.org/2/library/struct.html#format-characters
    ip_header = struct.pack('!BBHHHBBH4s4s' , 
        ip_ihl_ver, ip_tos, ip_tot_len, ip_id, ip_frag_off, ip_ttl, ip_proto, 
        ip_check, ip_saddr, ip_daddr)
    return ip_header


def build_udp_header(src_port, dst_port, length):
    """Builds a valid UDP header and returns it

    Parameters:
      - src_port: A uint16 which will be used as source port for the UDP
                  header
      - dst_port: A uint16 which will be used as destination port for the
                  UDP header
      - length: Length of the data that will be sent in the UDP package. 
                The actual length field in the UDP package will be 8 bytes
                longer to make room for the UDP header itself 
 
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |            Source port        |         Destination port      |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |            Length             |            Checksum           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    """

    if (src_port == DEFAULT_SRC_PORT):
        src_port = dst_port

    # error-checking of header AND data. If no checksum is generated set the
    # value all-zeros
    checksum = 0
    udp_header = struct.pack('!HHHH', 
        src_port, dst_port, (length + 8), checksum);

    return udp_header


def parse_ops():
    """The option parser. 

    A looooong ugly function, you've been warned

    """

    docstring = __doc__ % {'scriptName' : sys.argv[0]}
    usage = docstring.split("\n")[1]
    # Take the description from the docstring of this file
    desc = ''.join(docstring.split("\n")[2:])
    parser = OptionParser(usage=usage, version="%prog 0.2", description = desc)

    # general ops
    parser.add_option("-u", "--usec-interval", dest="usec", action="store",
                      type="int", default="0",
                      help="microseconds between packets sent. Default is 0",
                      metavar="USEC")
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="give more verbose output. Default is OFF")

    # plain socket ops
    group = OptionGroup(parser, "Plain socket options", 
                        "Options available when working with plain sockets (without the \"--raw\" option).")
    group.add_option("-m", "--max", dest="max", action="store", type="int", default=DEFAULT_MAX_SIZE,
                      help="maximum size of every UDP packet sent (default set to 65535). If it is bigger than the MTU of your network IP fragmentation may happen", 
                      metavar="MAX_SIZE")
    parser.add_option_group(group)

    # RAW socket ops
    group = OptionGroup(parser, "RAW socket options", 
                        "Options available to work with RAW sockets.")
    group.add_option("-r", "--raw", dest="raw", action="store_true",
                      help="Activate RAW sockets (instead of plain sockets). Default is OFF")
    group.add_option("-t", "--mtu", dest="mtu", action="store", type="int", default=DEFAULT_MTU,
                      help="MTU of your network (default set to 1500). This value will be used as the maximum size of the packets when working with RAW sockets", 
                      metavar="MTU")
    group.add_option("-p", "--src-port", dest="srcport", action="store", type="int",
                      help="use SRCPORT as the source port in the UDP packets. Default is set to the destination PORT", 
                      metavar="SRCPORT")
    parser.add_option_group(group)

    (parsed_ops, parsed_args) = parser.parse_args()

    # IP:PORT is a mandatory argument
    #
    if (len(parsed_args) == 0) or (len(parsed_args) > 1):
        print "IP:PORT is a mandatory argument"
        print usage
        sys.exit()

    # checking for a valid destination (IP:PORT)
    #
    my_dst_port = ""
    my_dst_ip = ""
    try:
        (my_dst_ip, my_dst_port) = parsed_args[0].split(':', 2)
    except:
        print "Invalid IP:PORT pair"
        print usage
        sys.exit()

    try:
        my_dst_port = int(my_dst_port)
    except:
        print "Invalid PORT in IP:PORT pair"
        print usage
        sys.exit()
        
    if ((my_dst_port <= 0) or (my_dst_port > 0xffff)):
        print "PORT must be a valid uint16 number (bigger than 0)"
        print usage
        sys.exit()

    if ((len(my_dst_ip) < 8) or (len(my_dst_ip) > 15)):
        print "IP isn't valid"
        print usage
        sys.exit()
    else:
        try:
            socket.inet_aton(my_dst_ip)
            # legal IP
        except socket.error:
            print "IP isn't valid"
            print usage
            sys.exit()

    # checking for a valid microsencs interval
    my_usec = DEFAULT_USECS_INTERVAL
    if (parsed_ops.usec):
        try:
            my_usec = int(parsed_ops.usec)
        except:
            print "Invalid microseconds interval"
            print usage
            sys.exit()

    # Checking plain socket options
    #
    my_max = DEFAULT_MAX_SIZE
    if (parsed_ops.max):
        try:
            my_max = int(parsed_ops.max)
        except:
            print "Invalid Maximum size"
            print usage
            sys.exit()

        if ((my_max <= 0) or (my_max > 0xffff)):
            print "Maximum size for plain sockets must be a valid uint16 number (bigger than 0)"
            print usage
            sys.exit()

    # time now for RAW socket options
    #
    my_mtu = DEFAULT_MTU
    my_src_port = DEFAULT_SRC_PORT
    if (parsed_ops.raw):
        # checking for a valid MTU
        if (parsed_ops.mtu):
            try:
                my_mtu = int(parsed_ops.mtu)
            except:
                print "Invalid MTU"
                print usage
                sys.exit()

            if ((my_mtu <= 0) or (my_mtu > 0xffff)):
                print "MTU must be a valid uint16 number (bigger than 0)"
                print usage
                sys.exit()

        # checking for a valid source port
        # if it is not present it will be set to DEFAULT_SRC_PORT
        if (parsed_ops.srcport):
            try:
                my_src_port = int(parsed_ops.srcport)
            except:
                print "Invalid SRCPORT number"
                print usage
                sys.exit()

            if ((my_src_port <= 0) or (my_src_port > 0xffff)):
                print "SRCPORT must be a valid uint16 number (bigger than 0)"
                print usage
                sys.exit()

    return (my_dst_port, my_dst_ip, parsed_ops.verbose, my_usec, my_max, parsed_ops.raw, my_src_port, my_mtu)



##############################################################################
# MAIN
#

# Parsing the arguments
#
(my_dst_port, my_dst_ip, my_verbose, my_interval, 
 my_plain_max, my_use_raw_socket, my_src_port, my_mtu) = parse_ops()

# transform my_interval from usecs into secs
my_interval /= USEC_IN_A_SEC

# create the socket that will be used to send the random data
#
if my_use_raw_socket:
    if my_verbose:
        print "Creating RAW socket. MTU set to %d" % (my_mtu)
        if (my_plain_max != DEFAULT_MAX_SIZE):
            print "max size of packets is set with \"--max\". Did you mean to use \"--mtu\"?"
    try:
        # http://www.binarytides.com/raw-socket-programming-in-python-linux/
        # 
        s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW)
        #s.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, int(1))
    except socket.error, msg:
        print 'Raw socket could not be created. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
        print 'Are you running as root?'
        sys.exit()
else:
    if my_verbose:
        print "Creating plain socket. Maximum length of random packets set to %d" % (my_plain_max)
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    except socket.error, msg:
        print 'Plain socket could not be created. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
        sys.exit()
    try:
        s.bind(('', 0))
    except socket.error, msg:
        print 'Plain socket could not bind. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
        sys.exit()
    

# random generator initilisation
#
rangen = None
try:
    rangen = random.SystemRandom()
except NotImplementedError, msg:
    print 'Warning: random.SystemRandom is not implemented in this OS'
    rangen = random
finally:
    # Initialize the basic random number generator with current system time
    # no matter which random generator we are using
    rangen.seed()

if my_use_raw_socket:
    # the IP header. It will be constant throught the execution of the script
    #
    ip_header = build_ip_header(DEFAULT_SRC_IP, my_dst_ip)

latest_time = 0
current_time = 0
packet_num = 0
while True:
    # length of this packet. 
    #
    if my_use_raw_socket:
        length = rangen.randint(1, my_mtu - IP_HEADER_SIZE - UDP_HEADER_SIZE)
    else:
        length = rangen.randint(1, my_plain_max - IP_HEADER_SIZE - UDP_HEADER_SIZE)

    # can only be used in python >= 2.6
    #message = bytearray(rangen.getrandbits(8) for i in xrange(length)) # python >= 2.6
    # cant use it because arrays and packed strings cant be concatenated together
    #message = array.array('B', (rangen.getrandbits(8) for i in xrange(length)))

    # a random stream of bytes of size 'length'
    # 
    message = struct.pack('B' * length, *(rangen.getrandbits(8) for i in xrange(length)))

    packet_num += 1
    if (my_verbose and (packet_num % DEFAULT_PRINT_OUT_PERIOD == 0)):
        print "%d: Sending %d bytes of data to %s:%d..." % (packet_num, length, my_dst_ip, my_dst_port)

    current_time = time.time()
    if ((latest_time != 0) and (latest_time + my_interval) > current_time):
        time.sleep((latest_time + my_interval) - current_time)

    try:
        if my_use_raw_socket:
            # Send the UDP packet using a raw socket. Concatenate IP_HDR + UDP_HDR + DATA
            #
            udp_header = build_udp_header(my_src_port, my_dst_port, length)
            s.sendto(ip_header + udp_header + message, (my_dst_ip, 0))
        else:
            # Send the UDP packet using a plain UDP socket    
            s.sendto(message, (my_dst_ip, my_dst_port))
            
    except socket.error, msg:
        print '%d: SendTo failed on a message %d bytes long to %s:%d' % (packet_num, length, my_dst_ip, my_dst_port)
        print '  Error Code : ' + str(msg[0]) + '. Message: ' + msg[1]
        #sys.exit()

    latest_time = time.time()

