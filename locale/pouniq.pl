#!/usr/bin/perl -w

# Purpose: removes the duplicate translation from a .po file
# Usage:   give the filenames to process on the command line, output will go
#          to the files with the same name but .uniq suffix
# Author:  VZ
# Created: 19.05.99
# Version: $Id$

use strict;

my $filename = "";  # name of the current file
my $msgid;          # current msgid
my %messages;       # the lines of msgids
my $skip = 0;       # true if skipping current (duplicate) msgid
my $hold = "";      # the accumulator for the output
my $inmsgid = 0;    # are we inside msgid?
my $dupFound = 0;   # were there any duplicates?

while (<>) {
    if ( $ARGV ne $filename ) {
        if ( $filename && !$dupFound ) {
            print "No duplicates found in ", $filename, ".\n";
            unlink $filename . ".uniq" or
                warn "Couldn't delete temp file: $!\n";

            $dupFound = 0;
        }

        $filename = $ARGV;
        open(OUT, ">" . $filename . ".uniq") or
            die "Error opening output file: $!\n";

        print "Processing file ", $filename, "...\n";
    }

    if ( /^msgid "(.*)"$/ ) {               # start of msgid
        $msgid = $1;
        $hold = $_;
        $skip = 0;
        $inmsgid = 1;
    }
    elsif ( $inmsgid ) {
        if ( /^\s*"(.*)"$/ ) {              # continuation of msgid
            $msgid .= $1;
            $hold .= $_;
        }
        else {                              # end of msgid
            if ( exists $messages{$msgid} ) {
                $dupFound = 1;
                printf "Duplicate entry found: msgid = '%s', first occurrence " .
                       "at line %d, another at line %d\n",
                       $msgid, $messages{$msgid}, $. - 1;
                $skip = 1;
            }
            else {
                $messages{$msgid} = $. - 1;

                print OUT $hold;
                print OUT
            }

            # reinit
            $hold = "";
            $msgid = "";
            $inmsgid = 0;
        }
    }
    else {                                  # not msgid at all
        if ( $_ eq "" or /^#/ ) {
            # comment or empty line is the entry separator
            $skip = 0;
        }

        print OUT unless $skip
    }
}

if ( !$dupFound ) {
    print "No duplicates found in ", $filename, ".\n";
    unlink $filename . ".uniq" or
        warn "Couldn't delete temp file: $!\n";
}
