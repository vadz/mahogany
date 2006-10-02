#!/usr/bin/perl

# $Id: graph.cgi,v 1.4 2006/05/13 01:13:01 jonz Exp $
# DSPAM
# COPYRIGHT (C) 2002-2006 JONATHAN A. ZDZIARSKI
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

use CGI ':standard';
use GD::Graph::lines3d;
use GD::Graph::lines;
use strict;
use vars qw { %CONFIG %FORM @spam_day @nonspam_day @period @data };

# Read configuration parameters common to all CGI scripts
require "configure.pl";

%FORM = &ReadParse();

GD::Graph::colour::read_rgb("rgb.txt"); 

do {
  my($spam, $nonspam, $period) = split(/\_/, $FORM{'data'});
  @spam_day = split(/\,/, $spam);
  @nonspam_day = split(/\,/, $nonspam);
  @period = split(/\,/, $period);
};

@data = ([@period], [@spam_day], [@nonspam_day]);
my $mygraph;
if ($CONFIG{'3D_GRAPHS'} == 1) {
  $mygraph = GD::Graph::lines3d->new(500, 200);
} else {
  $mygraph = GD::Graph::lines->new(500, 200);
}
$mygraph->set(
    x_label     => "$FORM{'x_label'}",
    y_label     => 'Number of Messages',
#   title       => "$FORM{'title'}",
    line_width   => 2,
    dclrs => [ qw(lred dgreen) ],
    fgclr => 'gray85',
    boxclr => 'gray95',
    textclr => 'black',
    legendclr => 'black',
    labelclr => 'gray60',
    axislabelclr => 'gray40',
    long_ticks  => 1,
    show_values => 0
) or warn $mygraph->error;

#         dclrs => [ qw( darkorchid2 mediumvioletred deeppink darkturquoise ) ],

$mygraph->set_legend_font(GD::gdMediumBoldFont);
$mygraph->set_legend('SPAM', 'Good');
my $myimage = $mygraph->plot(\@data) or die $mygraph->error;
                                                                                
print "Content-type: image/png\n\n";
print $myimage->png;

sub ReadParse {
  my(@pairs, %FORM);
  if ($ENV{'REQUEST_METHOD'} =~ /GET/i)
    { @pairs = split(/&/, $ENV{'QUERY_STRING'}); }
  else {
    my($buffer);
    read(STDIN, $buffer, $ENV{'CONTENT_LENGTH'});
    @pairs = split(/\&/, $buffer);
  }
  foreach(@pairs) {
    my($name, $value) = split(/\=/, $_);
                                                                                
    $name =~ tr/+/ /;
    $name =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
    $value =~ tr/+/ /;
    $value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
    $value =~ s/<!--(.|\n)*-->//g;
    $FORM{$name} = $value;
  }
  return %FORM;
}
