#!/usr/bin/perl -w

# Purpose: removes the duplicate translation from a .po file
# Usage:   give it the output (with errors about duplicate translations) from
#          mergecat or msgfmt -v on stdin
# Author:  VZ
# Created: 19.05.99
# Version: $Id$

use strict;

my $filename;       # the .po file we're working with
my (@dup1, @dup2);  # the list of original/duplicate lines

# first get the list of all locations of duplicate definitions
while (<STDIN>) {
    my ($file, $line, $msg) = split(':');
    if ( $filename eq "" ) {
        $filename = $file
    }
    elsif ( $file ne $filename ) {
        die "Can only process one file at a time, aborting.\n"
    }

    if ( $msg =~ "^duplicate message" ) {
        push @dup2, $line
    }
    elsif ( $msg =~ "location of the first definition" ) {
        push @dup1, $line
    }
    else {
        warn "Unknown message '" . $msg . "' at line " . $. . " ignored.\n"
    }
}

# preserve the original copy in .po.orig
my $filenameOld = $filename . ".orig";
rename $filename, $filenameOld or die "Error renaming file: $!\n";

# slurp in the whole file and process it from bottom to top to avoid line
# shift which would happen if we deleted the lines from top to bottom
open(PO_FILE, $filenameOld) or die "Error opening file: $!\n";
my @filelines = <PO_FILE>;

for ( my $i = 0; $i < $#dup1; $i++ ) {
    my $numLines = 0;

    NEXT_LINE: while ( 1 ) {
        $_ = @filelines[$dup2[$i]];

    }

    # do remove lines
    splice @filelines, $dup2[$i], $numLines;
}
