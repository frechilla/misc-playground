# The poor man's profiler...
# all credit goes to poormansprofiler.org
#

#!/bin/bash

function usage
{
    echo "$0 APPLICATION [SAMPLES_COUNT] [SLEEPTIME]"
}

if [[ $# -lt 1 ]]; then
    # application name is compulsory...
    usage
    exit 1
fi

if [[ $# -gt 1 ]]; then
    nsamples=$2
else
    nsamples=1
fi
if [[ $# -gt 2 ]]; then
    sleeptime=$3
else
    sleeptime=0
fi

pid=`pidof $1`
if [[ $? -ne 0 ]]; then
    echo "application $1 is not running"
    exit 2
fi

echo "profiling pid $pid ($1) $nsamples time(s) (sleeptime: $sleeptime)..."

for x in `seq 1 $nsamples`
do
    gdb -ex "set pagination 0" -ex "thread apply all bt" -batch -p $pid
    # for gdb 6.3 and older...
    #
    # (echo "set pagination 0";
    #  echo "thread apply all bt;
    #  echo "quit"; cat /dev/zero ) | gdb -p $(pid)
    sleep $sleeptime
done | \
awk '
  BEGIN { s = ""; }
  /^Thread/ {print s; s = ""; }
  /^\#/ { if (s != "" ) { s = s "," $4} else { s = $4 } }
  END { print s }' | \
sort | uniq -c | sort -r -n -k 1,1

