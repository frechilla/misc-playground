#!/usr/bin/python
"""
usage: %(scriptName)s [options] FILE1 FILE2 [FILE(S)] -o OUTPUT_FILE

Merge two (or more) timestamped log files into one. 

A timestamped log should look something like:
[01/Jun/2012 12:29:17.953] INFO info message
[01/Jun/2012 12:29:17.983] WARNING warning message
...

There are two parameters that tell this script how to merge log messages:
  --regex  
    tells the script how to extract date and message from each log entry. The 
    regex must contain two groups, one labeled <date> which matches the date 
    string, and another group labeled <msg>, that parses the log string. See
    https://docs.python.org/2/library/re.html#regular-expression-syntax  to 
    learn more about regex expressions and groups
  --date-fmt
    Format of the date tag of each log entry. The <date> group will be analysed
    using the format described by this parameter to generete a datetime 
    python object using strftime. For more info on date formats see 
    https://docs.python.org/2/library/datetime.html#strftime-and-strptime-behavior
"""

# Copyright (C) 2014 Faustino Frechilla (frechilla@gmail.com)
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

import sys, os, time, threading, Queue
import re
import datetime
from optparse import OptionParser

# consts
#
LOG_REGEX_STR_DEFAULT = r'\[(?P<date>[^\]]+)\] (?P<msg>[^\n]+)'
DATE_FORMAT_DEFAULT = "%d/%b/%Y %H:%M:%S.%f"

def usage():
    """Prints the usage info text extracted from this script's documentation
    
    """
    print (__doc__ % {'scriptName' : sys.argv[0]}).lstrip()

def parse_ops():
    """The option parser. 
    
    """
    docstring = __doc__ % {'scriptName' : sys.argv[0]}
    usage = docstring.split("\n")[1]
    # Take the description from the docstring of this file
    desc = ''.join(docstring.split("\n")[3:4])
    parser = OptionParser(usage=usage, version="%prog 0.1", description = desc)

    # general ops
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="give more verbose output. Default is OFF")
    parser.add_option("-H", "--doc", dest="doc", action="store_true",
                      help="print this script documentation. It contains a bit more of help about the --regex and --date-fmt parameters")
    parser.add_option("-o", "--output", dest="outputfile", action="store",
                      type="string",
                      help="path to the file where the output will be written to. If it exists it will be truncated",
                      metavar="OUTPUT_FILE")
    parser.add_option("-r", "--regex", dest="regexp", action="store",
                      type="string", default=LOG_REGEX_STR_DEFAULT,
                      help="regular expression to extract dates and messages. Default is \'%s\'" % (LOG_REGEX_STR_DEFAULT),
                      metavar="REGEXP")
    parser.add_option("-d", "--date-fmt", dest="datefmt", action="store",
                      type="string", default=DATE_FORMAT_DEFAULT,
                      help="date format in log files. Used to extract the date from the regular expression of option \"--regex\". Default is \'%s\'" % (DATE_FORMAT_DEFAULT),
                      metavar="REGEXP")
    
    (parsed_ops, parsed_args) = parser.parse_args()
    return (parsed_ops.doc, parsed_ops.verbose, parsed_ops.outputfile, parsed_ops.regexp, parsed_ops.datefmt, parsed_args)

def read_next_msg(file_handler, regex_obj, date_format):
    """Returns the next tuple (date, msg)
    (None, None) if the end of the file was hit without any more message tuple
    """    
    while (True):
        line = file_handler.readline()
        if (line == ""):
            return (None, None) # end of file
        else:
            m = regex_obj.match(line.rstrip())
            if (m and m.group('date') and m.group('msg')):
                date_obj = datetime.datetime.strptime(m.group('date'), date_format)
                msg = m.group('msg')
                return (date_obj, msg)
    
    # How did you get to here?
    assert (False)
    return (None, None)


if __name__ == '__main__':
    
    # Parsing the arguments
    #
    (my_doc, my_verbose, my_output_file, my_regex, my_datefmt, my_paths) = parse_ops()
    if my_verbose:
        print "verbose: command line options parsed"
    
    if (my_doc):
        usage()
        sys.exit(0)

    # At least 2 log files
    #
    if len(my_paths) < 2:
        print "error: missing parameters"
        usage()
        sys.exit(1)
    
    # The output file name must be set
    #
    if (my_output_file is None):
        print "error: output file name missing"
        usage()
        sys.exit(2)
    
    # Process regular expression
    #
    regex_obj = None
    try:
        regex_obj = re.compile(my_regex)
    except Exception as e:
        print "error: regular expression \"%s\" can't be processed (%s)" % (my_regex, str(e))
        sys.exit(3)
    
    # open files to read
    #
    file_list = []
    for i, file in enumerate(my_paths):
        if not os.path.isfile(file):
            print "error: file %s (#%d) is not a file" % (file, i)
            usage()
            sys.exit(4)
        else:
            try:
                file_desc = open(file, 'r')
            except IOError as e:
                print "error: %s could not be opened (%s)" % (file, str(e))
                sys.exit(5)
            file_list.append(file_desc)                
            if my_verbose:
                print "verbose: file %s (#%d) opened" % (file, i)
    
    # open output file
    #
    try:
        output_handler = open(my_output_file, 'w')
    except IOError as e:
        print "error: %s could not be opened for writing (%s)" % (my_output_file, str(e))
    except Exception as e:
        print "error: unknown exception opening %s for writing (%s)" % (my_output_file, str(e))
    
    if my_verbose:
        print "verbose: regular expression set to '%s'" % my_regex
        print "verbose: date format '%s'" % my_datefmt
        print "verbose: output will be stored at %s" % my_output_file
    
    # working loop
    #
    try:
        dates_list = []
        msg_list = []
        for i, file_handler in enumerate(file_list):
            (date_obj, msg_str) = \
                read_next_msg(file_handler, regex_obj, my_datefmt)
            if (not date_obj is None):
                dates_list.append(date_obj)
                msg_list.append(msg_str)
        
        while (len(dates_list) > 0):
            min_index = dates_list.index(min(dates_list))
            
            output_handler.write("%s - %s\n" % (dates_list[min_index], msg_list[min_index]))
            #if (my_verbose):
            #    print "verbose: (#%d) %s -- %s" % \
            #        (min_index, dates_list[min_index], msg_list[min_index])
                
            (dates_list[min_index], msg_list[min_index]) = \
                read_next_msg(file_list[min_index], regex_obj, my_datefmt)
            if (dates_list[min_index] is None):
                if (my_verbose):
                    print "verbose: end of file %s (index #%d) reached" % \
                        (file_list[min_index].name, min_index)
                file_list[min_index].close()
                del file_list[min_index]
                del dates_list[min_index]
                del msg_list[min_index]
            
    except (KeyboardInterrupt, SystemExit) as e:
        raise
    except Exception as e:
        print("error: unexpected exception processing log files (%s)" % str(e))
        raise
    
    output_handler.close()
