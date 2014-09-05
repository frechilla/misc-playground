#!/usr/bin/perl

# This script gets all the files in a directory (passed as parameter)
# and rename them to 'default name' adding at the beginning a number(index),
# which by default starts as 001. it can also be specified as a parameter
#

use strict; # Declare strict checking on variable names, etc.
use File::Copy;

my $argc = $#ARGV + 1;
if (($argc != 2) && ($argc != 3))
{
    usage();
    exit();
}

# working directory
my $dir = $ARGV[0];
die("Specified Path '".$dir."' doesn't exist") if !(-d $dir);

my $defaultName = $ARGV[1];

# Index. It will be added at the start of the file
my $index = 1;
if ($argc == 3)
{
    $index = $ARGV[2];
}


my @listOfFiles = `ls -tr '$dir'`;

my $thisFile;
foreach $thisFile (@listOfFiles)
{
    # $thisFile contains a \n. Chomp removes it
    chomp($thisFile);
    
    # $dir/$thisFile might be a directory. We only want to rename files
    if (-f $dir."/".$thisFile)
    {
        # some regex magic. retrieve the extension of the file (extension is whatever there is
        # after the last '.' (extension in .tar.gz files will be ".tar.gz)
        my $extension;
        if ($thisFile =~ m/((\.tar)?\.[a-z]+)$/i)
        {
             $extension = $1;
        }
        else
        {
            die("not extension found in file '$dir"."/"."$thisFile'");
        }

        my $indexString = "$index";
        if ($index < 100)
        {
            $indexString = "0".$indexString;
        }
        if ($index < 10)
        {
            $indexString = "0".$indexString;
        }

        my $destFile = $dir."/".$indexString."-".$defaultName.$extension;

        print "renaming: ".$dir."/".$thisFile." --> ".$destFile."\n";

        # don't let the script rename the old file overwriting an existing file
        die ("destination file '".$destFile ."' exists. It won't be overwritten") if (-f $destFile);

        # WARNING
        # Uncomment these lines if you want the script to do what is meant to do
        # free advice: Run it first with this line commented out and have a look to the output 
        #              if you are happy with the output run it again with this line uncommented
         if (!move($dir."/".$thisFile, $destFile))
        {
            die ("shit! Error renaming that latest file");
        }
        $index++;
    }
}


###########################################################
# Print the usage options of the script
sub usage()
{
    # $^X: The name that Perl itself was executed as, from argv[0]
    print "Usage: perl " . $^X . " directory 'default name' [start index]\n";
    print qq[
    This script gets all the files in a directory (passed as parameter)
    and rename them to 'default name' adding at the beginning a number(index),
    which by default starts as 001. it can also be specified as a parameter

];
}

