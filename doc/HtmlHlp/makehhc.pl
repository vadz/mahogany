#!/usr/bin/perl -w

# Purpose: creates the .hhc file used for CHM file contents from HTML
# Usage:   launch it in the directory containing Manual.html
#          (produced using "latex2html -split 0 -info 0 -no_navigation")
# Author:  VZ
# Created: 05.03.01
# Version: $Id$

use strict;

open(IN_HTML, "<Manual.html") or die "manual file not found: $!\n";
open(OUT_HHC, ">Manual.hhc") or die "failed to open CHM file: $!\n";

# are we inside the TOC?
my $in_toc = 0;

# are we inside a TOC entry?
my $in_entry = 0;

while (<IN_HTML>) {
	if ( !$in_toc ) {
		if ( /^<!--Table of Contents-->$/ ) {
			$in_toc = 1;

			# write CHM preamble
			print OUT_HHC <<EOF
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<meta name="GENERATOR" content="Microsoft&reg; HTML Help Workshop 4.1">
<!-- Sitemap 1.0 -->
</HEAD><BODY>
<OBJECT type="text/site properties">
 <param name="ImageType" value="Folder">
</OBJECT>
EOF
		}
	}
	elsif ( /^<!--End of Table of Contents-->$/ ) {
		# ignore the rest of the file
		last;
	}
	else {
		# we're inside TOC
		if ( /^<LI><A NAME=/ ) {
			# entry start
			$in_entry = 1;
		}
		elsif ( /(<.*UL)( CLASS="TofC")*>/ ) {
			# leave ULs as is but remove the optional class, HTML
			# Help Workshop doesn't like it
			print OUT_HHC "$1>"
		}
		elsif ( /^ +HREF="([^"]+)">(.+)<\/A>$/ ) {
			# entry body
			if ( $in_entry ) {
				# latex2html may generate either just the anchor in the link
				# or the file name with the anchor but we always need the
				# latter for HTML Help Workshop to work
				my ($link, $value) = ($1, $2);
				$link = "Manual.html$link" unless $link =~ /^Manual.html/;
				print OUT_HHC <<EOF
\t<LI> <OBJECT type=\"text/sitemap\">
<param name=\"Local\" value=\"$link\">
<param name=\"Name\" value=\"$value\">
</OBJECT>
EOF
			}
			else {
				warn "Line $.: unexpected entry body ignored.\n";
			}
		}
		else {
			# what's this?
			warn "Line $.: '$_' ignored.\n" unless /^$/ or /^<BR>$/
		}
	}
}

if ( !$in_toc ) {
		warn "TOC not found.\n";
}
else {
	print OUT_HHC "</BODY></HTML>"
}

