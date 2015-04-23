##############################################################################
#                                                                            #
#   this file contains a few command line hacks to do miscellaneous things   #
#                                                                            #
##############################################################################

# update last modification date of all files which were last
# modified more than 2 years ago (730 days)
# it-ll get update to 'now'
#
find . -type f -mtime +730 -exec touch '{}' \;

# print all files which were last modified less than 2 days ago
#
find . -type f -mtime -2 -print
