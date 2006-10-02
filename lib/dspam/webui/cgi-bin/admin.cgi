#!/usr/bin/perl

# $Id: admin.cgi,v 1.12 2006/05/13 01:13:01 jonz Exp $
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

use strict;
use Time::Local;
use vars qw { %CONFIG %DATA %FORM };
require "ctime.pl";

# Read configuration parameters common to all CGI scripts
require "configure.pl";

#
# The current CGI script
#

$CONFIG{'ME'}		    = "admin.cgi";

#
# Check Permissions
#
do {
  my($admin) = 0;
  open(FILE, "<./admins");
  while(<FILE>) {
    chomp;
    if ($_ eq $ENV{'REMOTE_USER'}) {
      $admin = 1;
    }
  }
  close(FILE);

  if (!$admin) {
    &error("Access Denied");
  }
};

$| = 1;

#
# Determine which extensions are available
#
                                                                                
if ($CONFIG{'AUTODETECT'} == 1 || $CONFIG{'AUTODETECT'} eq "") {
  $CONFIG{'PREFERENCES_EXTENSION'} = 0;
  $CONFIG{'LARGE_SCALE'} = 0;
  $CONFIG{'DOMAIN_SCALE'} = 0;
  do {
    my $x = `$CONFIG{'DSPAM'} --version`;
    if ($x =~ /--enable-preferences-extension/) {
      $CONFIG{'PREFERENCES_EXTENSION'} = 1;
    }
    if ($x =~ /--enable-large-scale/) {
      $CONFIG{'LARGE_SCALE'} = 1;
    }
    if ($x =~ /--enable-domain-scale/) {
      $CONFIG{'DOMAIN_SCALE'} = 1;
    }
  };
}

#
# Parse Form Variables
#

%FORM = &ReadParse;

if ($ENV{'REMOTE_USER'} eq "") { 
  &error("System Error. I was unable to determine your identity.");
}

if ($FORM{'template'} eq "" || $FORM{'template'} !~ /^([A-Z0-9]*)$/i) {
  $FORM{'template'} = "status";
}

#
# Set up initial display variables
#
$DATA{'REMOTE_USER'} = $ENV{'REMOTE_USER'};

#
# Process Commands
#

# Status
if ($FORM{'template'} eq "status") {
  &DisplayStatus;

# User Statistics

} elsif ($FORM{'template'} eq "user") {
  &DisplayUserStatistics;

# Preferences
} elsif ($FORM{'template'} eq "preferences") {
  &DisplayPreferences;
}

&error("Invalid Command $FORM{'COMMAND'}");

#
# Preferences Functions
#

sub DisplayPreferences {
  my(%PREFS, $FILE, $USER);

  $DATA{'USERNAME'} = $FORM{'username'};
  $USER = $FORM{'username'};

  if ($FORM{'username'} eq "") {
    $FILE = "./default.prefs";
    if ($CONFIG{'PREFERENCES_EXTENSION'} != 1 && ! -l "$CONFIG{'DSPAM_HOME'}/default.prefs") {
      $DATA{'ERROR'} = "<em>WARNING:</em> " .
        "These default preferences will not be loaded by DSPAM, but only by ".
        " the CGI interface when a user initially sets up their preferences. ".
        "To have DSPAM override its configuration with these default ".
        "preferences, symlink $CONFIG{'DSPAM_HOME'}/default.prefs to the ".
        "default.prefs file in the CGI directory.<BR><BR>";
    }
  } else {
    $FILE = GetPath($FORM{'username'}) . ".prefs";
  }

  if ($FORM{'submit'} ne "") {

    if ($FORM{'enableBNR'} ne "on") {
      $FORM{'enableBNR'} = "off";
    }
                                                                                
    if ($FORM{'optIn'} ne "on") {
      $FORM{'optIn'} = "off";
    }

    if ($FORM{'optOut'} ne "on") {
      $FORM{'optOut'} = "off";
    }

    if ($FORM{'enableWhitelist'} ne "on") {
      $FORM{'enableWhitelist'} = "off";
    }

    if ($FORM{'showFactors'} ne "on") {
      $FORM{'showFactors'} = "off";
    }

    if ($CONFIG{'PREFERENCES_EXTENSION'} == 1) {

      if ($FORM{'spamSubject'} eq "") {
        $FORM{'spamSubject'} = "''";
      } else {
        $FORM{'spamSubject'} = quotemeta($FORM{'spamSubject'});
      }

      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' trainingMode " . quotemeta($FORM{'trainingMode'}) . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' spamAction " . quotemeta($FORM{'spamAction'}) . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' signatureLocation "
        . quotemeta($FORM{'signatureLocation'}) . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' spamSubject " . $FORM{'spamSubject'} . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' statisticalSedation "
        . quotemeta($FORM{'statisticalSedation'}) . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' enableBNR "
        . quotemeta($FORM{'enableBNR'}) . "> /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' optOut "
        . quotemeta($FORM{'optOut'}) . ">/dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' optIn "
        . quotemeta($FORM{'optIn'}) . ">/dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' showFactors "
        . quotemeta($FORM{'showFactors'}) . "> /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref '".quotemeta($USER).
        "' enableWhitelist "
        . quotemeta($FORM{'enableWhitelist'}) . "> /dev/null");

    } else {
      open(FILE, ">$FILE") || do { &error("Unable to write preferences: $!"); };
      print FILE <<_END;
trainingMode=$FORM{'trainingMode'}
spamAction=$FORM{'spamAction'}
spamSubject=$FORM{'spamSubject'}
statisticalSedation=$FORM{'statisticalSedation'}
enableBNR=$FORM{'enableBNR'}
enableWhitelist=$FORM{'enableWhitelist'}
signatureLocation=$FORM{'signatureLocation'}
showFactors=$FORM{'showFactors'}
_END
      close(FILE);
    }
  }

  if (! -e $FILE) {
    %PREFS = GetPrefs($USER, "./default.prefs");
  } else {
    %PREFS = GetPrefs($USER, $FILE);
  }

  $DATA{"SEDATION_$PREFS{'statisticalSedation'}"} = "CHECKED";
  $DATA{"S_".$PREFS{'trainingMode'}} = "CHECKED";
  $DATA{"S_LOC_".uc($PREFS{'signatureLocation'})} = "CHECKED";
  $DATA{"S_ACTION_".uc($PREFS{'spamAction'})} = "CHECKED"; 
  $DATA{"SPAM_SUBJECT"} = $PREFS{'spamSubject'};
  if ($PREFS{'optIn'} eq "on") {
    $DATA{'C_OPTIN'} = "CHECKED";
  }
  if ($PREFS{'optOut'} eq "on") {
    $DATA{'C_OPTOUT'} = "CHECKED";
  }

  if ($PREFS{"enableBNR"} eq "on") {
    $DATA{"C_BNR"} = "CHECKED";
  }
  if ($PREFS{"showFactors"} eq "on") {
    $DATA{"C_FACTORS"} = "CHECKED";
  }
  if ($PREFS{"enableWhitelist"} eq "on") {
    $DATA{"C_WHITELIST"} = "CHECKED";
  }

  &output(%DATA);
}

#
# User Statistics
#

sub DisplayUserStatistics {
  my($b) = "rowEven";

  my ($sl_total) = 0;
  my ($il_total) = 0;
  my ($sm_total) = 0;
  my ($fp_total) = 0;
  my ($sc_total) = 0;
  my ($ic_total) = 0;
  my ($mailbox_total) = 0;
  
  open(IN, "$CONFIG{'DSPAM_STATS'}|");
  while(<IN>) {
    chomp;
    if ($b eq "rowEven") {
      $b = "rowOdd";
    } else {
      $b = "rowEven";
    }
    s/:/ /g;
    my($username, $sl, $il, $fp, $sm, $sc, $ic) = (split(/\s+/))[0,2,4,6,8,10,12]; 
    if ($sl eq "") {
      $_ = <IN>;
      s/:/ /g;
      ($sl, $il, $fp, $sm, $sc, $ic) = (split(/\s+/))[2,4,6,8,10,12];
    }

    my(%PREFS) = GetPrefs($username, GetPath($username).".prefs");
    $PREFS{'enableBNR'} = "OFF" if ($PREFS{'enableBNR'} ne "on");
    $PREFS{'enableWhitelist'} = "OFF" if ($PREFS{'enableWhitelist'} ne "on");
    $PREFS{'spamAction'} = ucfirst($PREFS{'spamAction'});
    $PREFS{'enableBNR'} = uc($PREFS{'enableBNR'});
    $PREFS{'enableWhitelist'} = uc($PREFS{'enableWhitelist'});
     
    my($mailbox) = GetPath($username).".mbox";
    my($mailbox_size, $mailbox_display);
    if ( -e $mailbox ) {
    	$mailbox_size = -s $mailbox;
    	$mailbox_display = sprintf("%2.1f KB", ($mailbox_size / 1024));
    	$mailbox_total += $mailbox_size;
    }
    else {
    	$mailbox_display = "--";
    }
    
    $sl_total += $sl;
    $il_total += $il;
    $sm_total += $sm;
    $fp_total += $fp;
    $sc_total += $sc;
    $ic_total += $ic;
    
    $DATA{'TABLE'} .= qq!<tr><td class=\"$b\"><a href="$CONFIG{'DSPAM_CGI'}?user=$username">$username</A></td>!.
                      "	<td class=\"$b rowDivider\" align=\"right\">$mailbox_display</td>".
                      "	<td class=\"$b rowDivider\">$sl</td>".
                      "	<td class=\"$b\">$il</td>".
                      " <td class=\"$b\">$fp</td>".
                      "	<td class=\"$b\">$sm</td>".
                      " <td class=\"$b\">$sc</td>".
                      "	<td class=\"$b\">$ic</td>".
                      "	<td class=\"$b rowDivider\">$PREFS{'trainingMode'}</td>".
                      "	<td class=\"$b\">$PREFS{'spamAction'}</td>".
                      "	<td class=\"$b\">$PREFS{'enableBNR'}</td>".
                      "	<td class=\"$b\">$PREFS{'enableWhitelist'}</td>".
                      "	<td class=\"$b\">$PREFS{'statisticalSedation'}</td>".
                      "	<td class=\"$b\">$PREFS{'signatureLocation'}</td>".
                      "</tr>\n";
  }
  close(IN);

  my($mailbox_total_display) = sprintf("%2.1f KB", ($mailbox_total / 1024));

  $b = "rowHighlight";
  $DATA{'TABLE'} .= "<tr><td class=\"$b\">Total</td>".
                    "	<td class=\"$b rowDivider\" align=\"right\">$mailbox_total_display</td>".
                    "	<td class=\"$b rowDivider\">$sl_total</td>".
                    "	<td class=\"$b\">$il_total</td>".
                    "	<td class=\"$b\">$sm_total</td>".
                    "	<td class=\"$b\">$fp_total</td>".
                    "	<td class=\"$b\">$sc_total</td>".
                    "	<td class=\"$b\">$ic_total</td>".
                    "	<td class=\"$b rowDivider\">&nbsp;</td>".
                    "	<td class=\"$b\">&nbsp;</td>".
                    "	<td class=\"$b\">&nbsp;</td>".
                    "	<td class=\"$b\">&nbsp;</td>".
                    "	<td class=\"$b\">&nbsp;</td>".
                    "	<td class=\"$b\">&nbsp;</td>".
                   "</tr>\n";

  &output(%DATA);
}

#
# Status Functions
#

sub DisplayStatus {
  my($LOG) = "$CONFIG{'DSPAM_HOME'}/system.log";
  my(@spam_daily, @nonspam_daily, @period_daily, @fp_daily, @sm_daily, 
     @inoc_daily, @whitelist_daily);
  my(@spam_weekly, @nonspam_weekly, @period_weekly, @fp_weekly, @sm_weekly,
     @inoc_weekly, @whitelist_weekly);
  my(%msgpersecond);
  my($keycount_exectime);
  my($last_message);
  my(%classes);

  my ($min, $hour, $mday, $mon, $year) = (localtime(time))[1,2,3,4,5];
  my ($hmstart) = time - 60;
  my ($daystart) = timelocal(0, 0, 0, $mday, $mon, $year);
  my ($periodstart) = $daystart - (3600*24*24); # 2 Weeks ago
  my ($dailystart) = time - (3600*23);
  my ($c_weekly) = 0; # Cursor to most recent time slot
  my ($c_daily) = 0; 

  if (! -e $LOG) {
    &error("No historical data is available.");
  }

  # Initialize each individual time period

  for(0..23) {
    my($h) = To12Hour($hour-(23-$_));
    $period_daily[$_]	= $h;
    $spam_daily[$_]	= 0;
    $nonspam_daily[$_]	= 0;
    $sm_daily[$_]	= 0;
    $fp_daily[$_]	= 0;
    $inoc_daily[$_]	= 0;
  }

  for(0..24) {
    my($d) = $daystart - (3600*24*(24-$_));
    my ($lday, $lmon, $lyear) = (localtime($d))[3,4,5];
    $lmon++;
    $lyear += 1900;
    $period_weekly[$_]	= "$lmon/$lday/$lyear";
    $spam_weekly[$_]	= 0;
    $nonspam_weekly[$_]	= 0;
    $sm_weekly[$_]	= 0;
    $fp_weekly[$_]	= 0;
    $inoc_weekly[$_]	= 0;
  }

  open(LOG, "<$LOG") || &error("Unable to open logfile: $!");
  while(<LOG>) {
    my($t_log, $c_log, $signature, $e_log) = (split(/\t/))[0,1,3,5];
    next if ($t_log > time);

    $last_message = $t_log;

    # Only Parse Log Data in our Time Period
    if ($t_log>=$periodstart) {
      my($tmin, $thour, $tday, $tmon, $tyear) = (localtime($t_log))[1,2,3,4,5];
      $tmon++;
      $tyear += 1900;

      # Weekly Graph
      $c_weekly = 0;
      while($period_weekly[$c_weekly] ne "$tmon/$tday/$tyear" && $c_weekly<24) {
        $c_weekly++;
      }

      if ($c_log eq "E") {
        if ($classes{$signature} eq "S") {
          $spam_weekly[$c_weekly]--;
          $spam_weekly[$c_weekly] = 0 if ($spam_weekly[$c_weekly]<0);
        } elsif ($classes{$signature} eq "I") {
          $nonspam_weekly[$c_weekly]--;
          $nonspam_weekly[$c_weekly] = 0 if ($nonspam_weekly[$c_weekly]<0);
        } elsif ($classes{$signature} eq "W") {
          $whitelist_weekly[$c_weekly]--;
          $whitelist_weekly[$c_weekly] = 0 if ($whitelist_weekly[$c_weekly]<0);
        } elsif ($classes{$signature} eq "F") {
          $spam_weekly[$c_weekly]++; $fp_weekly[$c_weekly]--;
          $fp_weekly[$c_weekly] = 0 if ($fp_weekly[$c_weekly]<0);
        } elsif ($classes{$signature} eq "M") {
          $sm_weekly[$c_weekly]--; $nonspam_weekly[$c_weekly]++;
          $sm_weekly[$c_weekly] = 0 if ($sm_weekly[$c_weekly]<0);
        } elsif ($classes{$signature} eq "N") {
          $inoc_weekly[$c_weekly]--;
          $inoc_weekly[$c_weekly] = 0 if ($inoc_weekly[$c_weekly]<0);
        }
      } else {
       $classes{$signature} = $c_log;
      }

      if ($c_log eq "S") { $spam_weekly[$c_weekly]++; }
      if ($c_log eq "I") { $nonspam_weekly[$c_weekly]++; }
      if ($c_log eq "W") { $whitelist_weekly[$c_weekly]++; }
      if ($c_log eq "F")
        { $spam_weekly[$c_weekly]--; $fp_weekly[$c_weekly]++;
          $spam_weekly[$c_weekly] = 0 if ($spam_weekly[$c_weekly]<0); }
      if ($c_log eq "M")
        { $sm_weekly[$c_weekly]++; $nonspam_weekly[$c_weekly]--;
          $nonspam_weekly[$c_weekly] = 0 if ($nonspam_weekly[$c_weekly]<0); }
      if ($c_log eq "N") { $inoc_weekly[$c_weekly]++; }


      # Daily Graph
      if ($t_log>=$dailystart) {
        while($period_daily[$c_daily] ne To12Hour($thour) && $c_daily<24) {
          $c_daily++;
        }


        if ($c_log eq "E") {
          if ($classes{$signature} eq "S") {
            $spam_daily[$c_daily]--;
            $spam_daily[$c_daily] = 0 if ($spam_daily[$c_daily]<0);
          } elsif ($classes{$signature} eq "I") {
            $nonspam_daily[$c_daily]--;
            $nonspam_daily[$c_daily] = 0 if ($nonspam_daily[$c_daily]<0);
          } elsif ($classes{$signature} eq "W") {
            $whitelist_daily[$c_daily]--;
            $whitelist_daily[$c_daily] = 0 if ($whitelist_daily[$c_daily]<0);
          } elsif ($classes{$signature} eq "F") {
            $spam_daily[$c_daily]++; $fp_daily[$c_daily]--;
            $fp_daily[$c_daily] = 0 if ($fp_daily[$c_daily]<0);
          } elsif ($classes{$signature} eq "M") {
            $sm_daily[$c_daily]--; $nonspam_daily[$c_daily]++;
            $sm_daily[$c_daily] = 0 if ($sm_daily[$c_daily]<0);
          } elsif ($classes{$signature} eq "N") {
            $inoc_daily[$c_daily]--;
            $inoc_daily[$c_daily] = 0 if ($inoc_daily[$c_daily]<0);
          }
        }

        if ($c_log eq "S") { $spam_daily[$c_daily]++; }
        if ($c_log eq "I") { $nonspam_daily[$c_daily]++; }
        if ($c_log eq "W") { $whitelist_daily[$c_daily]++; }
        if ($c_log eq "F")
          { $spam_daily[$c_daily]--; $fp_daily[$c_daily]++;
            $spam_daily[$c_daily] = 0 if ($spam_daily[$c_daily]<0); }
        if ($c_log eq "M")
          { $sm_daily[$c_daily]++; $nonspam_daily[$c_daily]--;
            $nonspam_daily[$c_daily] = 0 if ($nonspam_daily[$c_daily]<0); }
        if ($c_log eq "N") { $inoc_daily[$c_daily]++; }
      }

      # Last Half-Minute
      if ($t_log>=$hmstart) {
        $msgpersecond{$t_log}++;
        $DATA{'AVG_PROCESSING_TIME'} += $e_log;
        $keycount_exectime++;
      }
    }
  }

  close(LOG);

  # Calculate Avg. Messages Per Second
  foreach(values(%msgpersecond)) {
    $DATA{'AVG_MSG_PER_SECOND'} += $_;
  }
  $DATA{'AVG_MSG_PER_SECOND'} /= 60;
  $DATA{'AVG_MSG_PER_SECOND'} = sprintf("%2.2f", $DATA{'AVG_MSG_PER_SECOND'});

  # Calculate Avg. Processing Time
  if ($keycount_exectime == 0) {
    $DATA{'AVG_PROCESSING_TIME'} = 0;
  } else {
    $DATA{'AVG_PROCESSING_TIME'} /= $keycount_exectime;
  }
  $DATA{'AVG_PROCESSING_TIME'} = sprintf("%01.6f", $DATA{'AVG_PROCESSING_TIME'});

  # Calculate Number of processes, Uptime and Mail Queue length
  $DATA{'DSPAM_PROCESSES'} = `$CONFIG{'ALL_PROCS'} | grep dspam | grep -v grep | grep -v cgi | grep -v sock | wc -l`;
  $DATA{'UPTIME'} = `uptime`;
  $DATA{'MAIL_QUEUE'} = `$CONFIG{'MAIL_QUEUE'}`;
  
  # Calculate Graphs
  $DATA{'SPAM_TODAY'}               = $spam_weekly[24];
  $DATA{'NONSPAM_TODAY'}            = $nonspam_weekly[24];
  $DATA{'SM_TODAY'}                 = $sm_weekly[24];
  $DATA{'FP_TODAY'}                 = $fp_weekly[24];
  $DATA{'INOC_TODAY'}               = $inoc_weekly[24];
  $DATA{'TOTAL_TODAY'}              = $DATA{'SPAM_TODAY'} + $DATA{'NONSPAM_TODAY'} + $DATA{'SM_TODAY'}
                                      + $DATA{'FP_TODAY'} + $DATA{'INOC_TODAY'};

  $DATA{'SPAM_THIS_HOUR'}           = $spam_daily[23];
  $DATA{'NONSPAM_THIS_HOUR'}        = $nonspam_daily[23];
  $DATA{'SM_THIS_HOUR'}             = $sm_daily[23];
  $DATA{'FP_THIS_HOUR'}             = $fp_daily[23];
  $DATA{'INOC_THIS_HOUR'}           = $inoc_daily[23];
  $DATA{'TOTAL_THIS_HOUR'}          = $DATA{'SPAM_THIS_HOUR'} +
                                      + $DATA{'NONSPAM_THIS_HOUR'} 
                                      + $DATA{'SM_THIS_HOUR'}
                                      + $DATA{'FP_THIS_HOUR'} 
                                      + $DATA{'INOC_THIS_HOUR'};

  $DATA{'DATA_DAILY'} = join(",", @spam_daily)
                      . "_"
                      . join(",", @nonspam_daily)
                      . "_"
                      . join(",", @sm_daily)
                      . "_"
                      . join(",", @fp_daily)
                      . "_"
                      . join(",", @inoc_daily)
                      . "_"
                      . join(",", @whitelist_daily)
                      . "_"
                      . join(",", @period_daily); 

  $DATA{'DATA_WEEKLY'} = join(",", @spam_weekly)
                      . "_"
                      . join(",", @nonspam_weekly)
                      . "_"
                      . join(",", @sm_weekly)
                      . "_"
                      . join(",", @fp_weekly)
                      . "_"
                      . join(",", @inoc_weekly)
                      . "_"
                      . join(",", @whitelist_weekly)
                      . "_"
                      . join(",", @period_weekly);

  foreach(@spam_daily)		{ $DATA{'TS_DAILY'} += $_; };
  foreach(@nonspam_daily)	{ $DATA{'TI_DAILY'} += $_; }
  foreach(@sm_daily)		{ $DATA{'SM_DAILY'} += $_; }
  foreach(@fp_daily)		{ $DATA{'FP_DAILY'} += $_; }
  foreach(@inoc_daily)		{ $DATA{'INOC_DAILY'} += $_; }
  foreach(@whitelist_daily)	{ $DATA{'TI_DAILY'} += $_; }

  foreach(@spam_weekly)		{ $DATA{'TS_WEEKLY'} += $_; }
  foreach(@nonspam_weekly)	{ $DATA{'TI_WEEKLY'} += $_; }
  foreach(@sm_weekly)		{ $DATA{'SM_WEEKLY'} += $_; }
  foreach(@fp_weekly)		{ $DATA{'FP_WEEKLY'} += $_; }
  foreach(@inoc_weekly)		{ $DATA{'INOC_WEEKLY'} += $_; }
  foreach(@whitelist_weekly)	{ $DATA{'TI_WEEKLY'} += $_; }

  &output(%DATA);
}

#
# Global Functions
#

sub output {
  if ($FORM{'template'} eq "" || $FORM{'template'} !~ /^([A-Z0-9]*)$/i) {
    $FORM{'template'} = "performance";
  }
  print "Expires: now\n";
  print "Pragma: no-cache\n";
  print "Cache-control: no-cache\n";
  print "Content-type: text/html\n\n";
  my(%DATA) = @_;
  $DATA{'WEB_ROOT'} = $CONFIG{'WEB_ROOT'};

  open(FILE, "<$CONFIG{'TEMPLATES'}/nav_admin_$FORM{'template'}.html");
  while(<FILE>) { 
    s/\$CGI\$/$CONFIG{'ME'}/g;
    s/\$([A-Z0-9_]*)\$/$DATA{$1}/g; 
    print;
  }
  close(FILE);
  exit;
}

sub SafeVars {
  my(%PAIRS) = @_;
  my($url, $key);
  foreach $key (keys(%PAIRS)) {
    my($value) = $PAIRS{$key};
    $value =~ s/([^A-Z0-9])/sprintf("%%%x", ord($1))/gie;
    $url .= "$key=$value&";
  }
  $url =~ s/\&$//;
  return $url;
}

sub error {
  my($error) = @_;
  $FORM{'template'} = "error";
  $DATA{'MESSAGE'} = <<_end;
The following error occured while trying to process your request: <BR>
<B>$error</B><BR>
<BR>
If this problem persists, please contact your administrator.
_end
  &output(%DATA);
}

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

sub To12Hour {
  my($h) = @_;
  if ($h < 0) { $h += 24; }
  if ($h>11) { $h -= 12 if ($h>12) ; $h .= ":00pm"; }
  else { $h = "12" if ($h == 0); $h .= ":00am"; }
  return $h;
}


sub GetPath {
  my($USER, $VPOPUSERNAME, $VPOPDOMAIN);
  my($UN) = @_;

  # Domain-scale
  if ($CONFIG{'DOMAIN_SCALE'} == 1) {
    $VPOPUSERNAME = (split(/@/, $UN))[0];
    $VPOPDOMAIN = (split(/@/, $UN))[1];
    $USER = "$CONFIG{'DSPAM_HOME'}/data/$VPOPDOMAIN/$VPOPUSERNAME/$VPOPUSERNAME";

  # Normal scale
  } elsif ($CONFIG{'LARGE_SCALE'} == 0) {
    $USER = "$CONFIG{'DSPAM_HOME'}/data/$UN/$UN";

  # Large-scale
  } else {
    if (length($UN)>1) {
      $USER = "$CONFIG{'DSPAM_HOME'}/data/" . substr($UN, 0, 1) .
      "/". substr($UN, 1, 1) . "/$UN/$UN";
    } else {
      $USER = "$CONFIG{'DSPAM_HOME'}/data/$UN/$UN";
    }
  }

  return $USER;
}

sub GetPrefs {
  my(%PREFS);
  my($USER, $FILE) = @_;

  if ($CONFIG{'PREFERENCES_EXTENSION'} == 1) {
    open(PIPE, "$CONFIG{'DSPAM_BIN'}/dspam_admin agg pref ".quotemeta($USER)."|");
    while(<PIPE>) {
      chomp;
      my($directive, $value) = split(/\=/);
      $PREFS{$directive} = $value;
    }
    close(PIPE);
  } else {
    if (! -e $FILE) {
      $FILE = "./default.prefs";
    }
                                                                                
    if (! -e $FILE) {
      &error("Unable to load default preferences");
    }
                                                                                
    open(FILE, "<$FILE");
    while(<FILE>) {
      chomp;
      my($directive, $value) = split(/\=/);
      $PREFS{$directive} = $value;
    }
    close(FILE);
  }

  return %PREFS;
}
