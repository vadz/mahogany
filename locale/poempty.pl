#!/usr/bin/perl -w

# Purpose: finds missing translations in .po files
# Usage:   give the filenames to process on the command line
# Author:  VZ
# Created: 19.05.99
# Version: $Id$

use strict;

sub report
{
    my ($file, $n) = @_;

    if ( $n ) {
        printf "%d missing translations in file '%s'\n", $n, $file
    }
    else {
        printf "file '%s' doesn't contain any empty translations\n", $file
    }
}

my $filename = "";  # name of the current file
my $msgid;          # current msgid
my $msgstr;         # current msgstr
my $inmsgid = 0;    # are we inside a msgid?
my $inmsgstr = 0;   # are we inside a msgstr?
my $line;           # line where last msgid started
my $empty = 0;      # number of missing translations

while (<>) {
    if ( $ARGV ne $filename ) {
        if ( $filename ) {
            &report($filename, $empty);
        }

        $filename = $ARGV;
        $empty = 0;
    }

    if ( /^msgid "(.*)"$/ ) {               # start of msgid
        $msgid = $1;
        $line = $.;
        $inmsgid = 1;
    }
    elsif ( $inmsgid ) {
        if ( /^\s*"(.*)"$/ ) {              # continuation of msgid
            $msgid .= $1;
        }
        else {                              # end of msgid
            $inmsgid = 0;
        }
    }

    if ( /^msgstr "(.*)"$/ ) {              # start of msgstr
        $msgstr = $1;
        $inmsgstr = 1;
    }
    elsif ( $inmsgstr ) {
        if ( /^\s*"(.*)"$/ ) {              # continuation of msgstr
            $msgstr .= $1;
        }
        else {                              # end of msgstr
            $inmsgstr = 0;
        }
    }

    if ( !$inmsgid && !$inmsgstr ) {
        if ( $msgid && !$msgstr ) {
            printf "No translation for '%s' at line %d\n",
                   $msgid, $line;
            $empty++;
        }

        $msgid = $msgstr = "";
    }
}

&report($filename, $empty);
