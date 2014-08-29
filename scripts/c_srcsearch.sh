#!/bin/bash

# this is a shortcut function to look for text in .h, .c, .cc and .cpp files 
# using the "find" utility
#
#usage: c_srcsearch [-i] [directory] string-to-search
#          -i: optional parameter to activate case insensitive search
#   directory: optional parameter. c_srcsearch will look for 
#              string-to-search in this directory
#
c_srcsearch()
{
  OPT_CASE=""
  OPT_DIR=""
  OPT_STR=""
  if [[ $# -eq 3 && $1 == "-i" && -d $2 ]]; then
    OPT_CASE="-i"
    OPT_DIR=$2
    OPT_STR=$3
  elif [[ $# -eq 2 && $1 == "-i" ]]; then
    OPT_CASE="-i"
    OPT_DIR="."
    OPT_STR=$2
  elif [[ $# -eq 2 && -d $1 ]]; then
    OPT_CASE=""
    OPT_DIR=$1
    OPT_STR=$2
  elif [[ $# -eq 1 && $1 != "-i" ]]; then
    OPT_CASE=""
    OPT_DIR="."
    OPT_STR=$1
  fi

  if [[ $OPT_CASE || $OPT_DIR || $OPT_STR ]]; then
    find "$OPT_DIR" \( -not -iwholename '*.svn/text-base*' -and \( -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.cc' \) \) -exec grep -Hn $OPT_CASE "$OPT_STR" '{}' \;

  else
    if [[ $# -eq 3 && ! -d $2 ]]; then
      echo "$2 is not a valid directory"
    elif [[ $# -eq 2  && ! -d $1 ]]; then
       echo "$1 is not a valid directory"
    fi

    echo "usage: c_srcsearch [-i] [directory] string-to-search"
    echo "          -i: optional parameter to activate case insensitive search"
    echo "   directory: optional parameter. c_srcsearch will look for "
    echo "              string-to-search in this directory"
  fi
}

#alias srcsearch=c_srcsearch
