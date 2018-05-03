#!/usr/bin/python
"""
usage: %(scriptName)s FILE1 -o OUTPUT_FILE
Takes a file with values from 0 to 255 (separated by new line characters) and converts
them to (-1, 1)
An input file should look something like:
255
0
127
128
126

Output would look like:
1.0000
-1.0000
-0.0039
0.0039
-0.0118

"""

import sys
import os

def usage():
    """Prints the usage info text extracted from this script's documentation
    
    """
    print ((__doc__ % {'scriptName' : sys.argv[0]}).lstrip())

if __name__ == '__main__':

    if len(sys.argv) != 3:
        print("Bad parameters!!!")
        usage()
        sys.exit(1)
    
    if not os.path.isfile(sys.argv[1]):
        print("error: file %s can't be opened" % (sys.argv[1]))
        sys.exit(2)

    try:
        file = open(sys.argv[1], 'r')
    except IOError as e:
        print ("error: %s could not be opened for reading" % (sys.argv[1]))
        sys.exit(3)
        
    try:
        filew = open(sys.argv[2], 'w')
    except IOError as e:
        print ("error: %s could not be opened for writing" % (sys.argv[2]))
        sys.exit(4)
    
    try:
        for line in file:
            try:
                datastr = line.strip().split(' ', 1)[0]
                n = int(datastr)
                if (n >= 0 and n <= 255):
                    filew.write ("%.4f\n" % ((n*2/255)-1))
                else:
                    print("Bad number %d! Ignoring..." % n)
            except ValueError:
                print("Could not convert data into integer %s" % datastr)
            except Exception as e:
                print("Unexpected error:", sys.exc_info()[0])
                print(e)
                
    except:
        print("I/O error hadling %s:" % (sys.argv[1]), sys.exc_info()[0])
    
    file.close()
    filew.close()
    