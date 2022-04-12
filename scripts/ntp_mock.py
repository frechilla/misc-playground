#!/usr/bin/python
"""
Usage: %(scriptName)s IP:PORT
Simulate an NTP server on IP and PORT. Sends NTP responses to NTP queries 
"""

import socket
import sys
import ctypes
import struct
import datetime
import math
import time

# system epoch (1970 in unix)
_SYSTEM_EPOCH = datetime.date(*time.gmtime(0)[0:3])
# NTP epoch starts off 1900
_NTP_EPOCH = datetime.date(1900, 1, 1)
# delta to convert system epoch to NTP epoch
NTP_DELTA = (_SYSTEM_EPOCH - _NTP_EPOCH).days * 24 * 3600

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

    # Create a UDP socket and bind the socket to the port
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_address = (ip, port)
    print('starting up on {} port {}'.format(*server_address))
    sock.bind(server_address)

    counter = 0
    while True:
        print('\nwaiting to receive message')
        data, address = sock.recvfrom(4096)

        print('received {} bytes from {}'.format(
            len(data), address))
        
        if data:
            # get NTP parameters from the client message
            (flags, stratum, interval, precision, root_delay, root_dispersion, ref_id, ref_t, orig_t, rcv_t, xmit_t) = struct.unpack('>BBBBIIIQQQQ', data)
            #print (flags, stratum,interval,precision,root_delay,root_dispersion,ref_id,ref_t, orig_t, rcv_t, xmit_t)

            # calculate NTP timestamp based on the system time
            secs = (datetime.datetime.utcnow() - datetime.datetime(1970,1,1)).total_seconds() + NTP_DELTA
            
            # separate integer and floating part for correctly formatting NTP response
            (float_part, ntp_secs) = math.modf(secs)
            
            # floating part is coded in units of 1/(2^32)
            ntp_float = 4294967296 * float_part

            # fill in NTP header as server, stratum 1. 
            #   Reference ID is 'GPGL'
            #   Reference timestamp is set to the integer part of the system timestamp
            #   Origin timestamp is set to the original timestamp sent by the client
            data_to_send = struct.pack('>BBBBIIIIIQIIII', 0x24, 1, 3, 0xe9, 0, 0, 0x4750474c, ntp_secs, 0, xmit_t, ntp_secs, ntp_float, ntp_secs, ntp_float)

            #############################################################################################################
            ######## This hack extracts an NTP payload from a binary file, and generates a response based on the ########
            ######## timestamp stored in thefile and the original xmit timestamp sent by the client              ########
            #ntpdumpfile = 'ntp_aurizon_' + str(counter).zfill(2) + '.bin'
            #with open(ntpdumpfile, "rb") as f:
            #    print ntpdumpfile
            #    data = f.read()
            #    counter = counter + 1
            #    (flags, stratum,interval,precision,root_delay,root_dispersion,ref_id,ref_t, NOTIMPORTANT, rcv_t, xmit_cap) = struct.unpack('>BBBBIIIQQQQ', data)
            #    data_to_send = struct.pack('>BBBBIIIQQQQ', flags, stratum, interval, precision, root_delay, root_dispersion, ref_id, ref_t, xmit_t, rcv_t, xmit_cap)
            ##############################################################################################################

            # reply back!
            sent = sock.sendto(data_to_send, address)
            print('sent {} bytes back to {}'.format(sent, address))
