#!/usr/bin/perl

# $Id: dspam.cgi,v 1.26 2006/06/01 19:14:14 jonz Exp $
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
use vars qw { %CONFIG %DATA %FORM $MAILBOX $CURRENT_USER $USER $TMPFILE};
use vars qw { $CURRENT_STORE };
require "ctime.pl";

# Read configuration parameters common to all CGI scripts
require "configure.pl";

if($CONFIG{"DATE_FORMAT"}) {
  use POSIX qw(strftime);
}

#
# The current CGI script
#

$CONFIG{'ME'} = "dspam.cgi";

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
# Determine admin status
#

$CONFIG{'ADMIN'} = 0;
if ($ENV{'REMOTE_USER'} ne "") {
  open(FILE, "<./admins");
  while(<FILE>) {
    chomp;
    if ($_ eq $ENV{'REMOTE_USER'}) {
      $CONFIG{'ADMIN'} = 1;
    }
  }
  close(FILE);
}

#
# Configure Filesystem
#

%FORM = &ReadParse;

$CURRENT_USER = $ENV{'REMOTE_USER'};

if ($FORM{'user'} ne "" && $CONFIG{'ADMIN'} == 1) {
  $CURRENT_USER = $FORM{'user'};
} else {
  $FORM{'user'} = $CURRENT_USER ;
}

$CONFIG{'DSPAM_ARGS'} =~ s/%CURRENT_USER%/$CURRENT_USER/g;

# Current Store
do {
  my(%PREF) = GetPrefs($CURRENT_USER);
  $CURRENT_STORE = $PREF{"localStore"};
  if ($CURRENT_STORE eq "") { $CURRENT_STORE = $CURRENT_USER; }
};

$USER = GetPath($CURRENT_STORE);
$MAILBOX = $USER . ".mbox";
$TMPFILE = $USER . ".tmp";

if ($CURRENT_USER eq "") {
  &error("System Error. I was unable to determine your identity.");
}

if ($FORM{'template'} eq "" || $FORM{'template'} !~ /^([A-Z0-9]*)$/i) {
  $FORM{'template'} = "performance";
}

my($MYURL);
$MYURL = "$CONFIG{'ME'}?user=$FORM{'user'}&template=$FORM{'template'}";

#
# Set up initial display variables
#
&CheckQuarantine;
$DATA{'REMOTE_USER'} = $CURRENT_USER;

#
# Process Commands
#

# Performance
if ($FORM{'template'} eq "performance") {
  if ($FORM{'command'} eq "resetStats") {
    &ResetStats;
    redirect($MYURL);
  } elsif ($FORM{'command'} eq "tweak") {
    &Tweak;
    redirect($MYURL);
  } else {
    &DisplayIndex;
  }

# Quarantine
} elsif ($FORM{'template'} eq "quarantine") {
  if ($FORM{'command'} eq "viewMessage") {
    &Quarantine_ViewMessage;
  } else {
    $MYURL .= "&sortby=$FORM{'sortby'}" if ($FORM{'sortby'} ne "");
    if ($FORM{'command'} eq "processQuarantine") {
      &ProcessQuarantine;
      redirect($MYURL);
    } elsif ($FORM{'command'} eq "processFalsePositive") {
      &ProcessFalsePositive;
      redirect($MYURL);
    } else {
      &DisplayQuarantine;
    }
  }

# Alerts
} elsif ($FORM{'template'} eq "alerts") {
  if ($FORM{'command'} eq "addAlert") {
    &AddAlert;
    redirect($MYURL);
  } elsif ($FORM{'command'} eq "deleteAlert") {
    &DeleteAlert;
    redirect($MYURL);
  } else {
    &DisplayAlerts;
  }

# Preferences
} elsif ($FORM{'template'} eq "preferences") {
  &DisplayPreferences;

# Analysis
} elsif ($FORM{'template'} eq "analysis") {
  &DisplayAnalysis;

# History
} elsif ($FORM{'template'} eq "history") {
  &DisplayHistory;
} elsif ($FORM{'template'} eq "fragment") {
  &DisplayFragment;
} else {
  &error("Invalid Command $FORM{'COMMAND'}");
}

#
# History Functions
# 

sub DisplayFragment {
  $FORM{'signatureID'} =~ s/\///g;
  $DATA{'FROM'} = $FORM{'from'};
  $DATA{'SUBJECT'} = $FORM{'subject'};
  $DATA{'INFO'} = $FORM{'info'};
  $DATA{'TIME'} = $FORM{'time'};
  open(FILE, "<$USER.frag/$FORM{'signatureID'}.frag") || &error($!);
  while(<FILE>) {
    s/</&lt\;/g;
    s/>/&gt\;/g;
    $DATA{'MESSAGE'} .= $_;
  }
  close(FILE);
  &output(%DATA);
}

sub DisplayHistory {
  my($all_lines , $begin, $history_pages, $rec, $history_page);
  unless ($history_page = $FORM{'history_page'}) { $history_page = 1;}

  my(@buffer, @history, $line, %rec);
  my($rowclass) = "rowEven";

  if ($CONFIG{'HISTORY_PER_PAGE'} == 0) {
    $CONFIG{'HISTORY_PER_PAGE'} = 50;
  }

  if ($FORM{'command'} eq "retrainChecked") {
    foreach my $i (0 .. $#{ $FORM{retrain_checked} }) {
        my ($retrain, $signature) = split(/:/, $FORM{retrain_checked}[$i]);
        if ($retrain eq "innocent") {
          $FORM{'signatureID'} = quotemeta($signature);
          &ProcessFalsePositive();
          undef $FORM{'signatureID'};
         } elsif ($retrain eq "innocent" or $retrain eq "spam") {
           system("$CONFIG{'DSPAM'} --source=error --class=" . quotemeta($retrain) . " --signature=" . quotemeta($signature) . " --user " . quotemeta("$CURRENT_USER"));
         }
    }
  redirect("$MYURL&history_page=$history_page");
  } else {
    if ($FORM{'retrain'} ne "") {
       if ($FORM{'retrain'} eq "innocent") {
         &ProcessFalsePositive();
       } else {
         system("$CONFIG{'DSPAM'} --source=error --class=" . quotemeta($FORM{'retrain'}) . " --signature=" . quotemeta($FORM{'signatureID'}) . " --user " . quotemeta("$CURRENT_USER"));
       }
    redirect("$MYURL&history_page=$history_page");
    }
  }

  my($LOG) = "$USER.log";
  if (! -e $LOG) {
    &error("No historical data is available");
  }

  # Preseed retraining information and delivery errors
 
  open(LOG, "< $LOG") or die "Can't open log file $LOG";
  while(<LOG>) {
    my($time, $class, $from, $signature, $subject, $info, $messageid) 
      = split(/\t/, $_);
    next if ($signature eq "");
    if ($class eq "M" || $class eq "F" || $class eq "E") { 
      if ($class eq "E") {
        $rec{$signature}->{'info'} = $info;
      } elsif ($class eq "F" || $class eq "M") {
        $rec{$signature}->{'class'} = $class;
        $rec{$signature}->{'count'}++;
        $rec{$signature}->{'info'} = $info 
          if ($rec{$signature}->{'info'} eq "");
      }
      # filter out resents if there are any.  Since it's the same
      # message we only allow retraining on the 1st occurence of it.
    } elsif ($messageid == ''
	     || $rec{$signature}->{'messageid'} != $messageid
	     || $CONFIG{'HISTORY_DUPLICATES'} ne "no") {
      $rec{$signature}->{'time'} = $time;
      $rec{$signature}->{'class'} = $class;
      $rec{$signature}->{'from'} = $from;
      $rec{$signature}->{'signature'} = $signature;
      $rec{$signature}->{'subject'} = $subject;
      $rec{$signature}->{'info'} = $info;
      $rec{$signature}->{'messageid'} = $messageid;

      unshift(@buffer, $rec{$signature});
    }
  }
  close(LOG);

  if($CONFIG{'HISTORY_SIZE'} < ($#buffer+1)) {
    $history_pages = int($CONFIG{'HISTORY_SIZE'} / $CONFIG{'HISTORY_PER_PAGE'});
    $history_pages += 1 if($CONFIG{'HISTORY_SIZE'} % $CONFIG{'HISTORY_PER_PAGE'});
  } else {
    $history_pages = int( ($#buffer+1) / $CONFIG{'HISTORY_PER_PAGE'});
    $history_pages += 1 if(($#buffer+1) % $CONFIG{'HISTORY_PER_PAGE'});
  }
  $begin = int(($history_page - 1) * $CONFIG{'HISTORY_PER_PAGE'}) ;

  # Now lets just keep the information that we really need. 
  @buffer = splice(@buffer, $begin,$CONFIG{'HISTORY_PER_PAGE'});

  my $retrain_checked_msg_no = 0;
  while ($rec = pop@buffer) {
    my($time, $class, $from, $signature, $subject, $info, $messageid);

    $time = $rec->{'time'};
    $class = $rec->{'class'};
    $from = $rec->{'from'};
    $signature = $rec->{'signature'};
    $subject = $rec->{'subject'};
    $info = $rec->{'info'};
    $messageid = $rec->{'messageid'};

    next if ($signature eq "");
    next if ($rec{$signature}->{'displayed'} ne "");
    next if ($class eq "E");
    $rec{$signature}->{'displayed'} = 1;

    # Resends of retrained messages will need the original from/subject line
    if ($messageid ne "") {
      $from = $rec{$messageid}->{'from'} 
        if ($from eq "<None Specified>");
      $subject = $rec{$messageid}->{'subject'} 
        if ($subject eq "<None Specified>");

      $rec{$messageid}->{'from'} = $from 
        if ($rec{$messageid}->{'from'} eq "");
      $rec{$messageid}->{'subject'} = $subject 
        if ($rec{$messageid}->{'subject'} eq "");
    }

    $from = "<None Specified>" if ($from eq "");
    $subject = "<None Specified>" if ($subject eq "");

    my $ctime;
    if($CONFIG{"DATE_FORMAT"}) {
      $ctime = strftime($CONFIG{"DATE_FORMAT"}, localtime($time));
    } else {
      $ctime = ctime($time);
      my(@t) = split(/\:/, (split(/\s+/, $ctime))[3]);
      my($x) = (split(/\s+/, $ctime))[0];
      my($m) = "a";
      if ($t[0]>=12) { $t[0] -= 12; $m = "p"; }
      if ($t[0] == 0) { $t[0] = 12; }
      $ctime = "$x $t[0]:$t[1]$m";
    }

    # Set the appropriate type and label for this message

    my($cl, $cllabel);
    $class = $rec{$signature}->{'class'} if ($rec{$signature}->{'class'} ne "");
    if ($class eq "S") { $cl = "spam"; $cllabel="SPAM"; }
    elsif ($class eq "I") { $cl = "innocent"; $cllabel="Good"; }
    elsif ($class eq "F") {
      if ($rec{$signature}->{'count'} % 2 != 0) {
        $cl = "false"; $cllabel="Miss"; 
      } else {
        $cl = "innocent"; $cllabel="Good"; 
      }
    }
    elsif ($class eq "M") { 
      if ($rec{$signature}->{'count'} % 2 != 0) {
          $cl = "missed"; $cllabel="Miss"; 
      } else {
          $cl = "spam"; $cllabel="SPAM";
      }
    }
    elsif ($class eq "N") { $cl = "inoculation"; $cllabel="Spam"; }
    elsif ($class eq "C") { $cl = "blacklisted"; $cllabel="RBL"; }
    elsif ($class eq "W") { $cl = "whitelisted"; $cllabel="Whitelist"; }
    if ($messageid ne "") {
      if ($rec{$messageid}->{'resend'} ne "") {
        $cl = "relay";
        $cllabel = "Resend";
      }
      $rec{$messageid}->{'resend'} = $signature;
    }

    $info = $rec{$signature}->{'info'} if ($rec{$signature}->{'info'} ne "");

    $from =~ s/</&lt;/g;
    $from =~ s/>/&gt;/g;
    $subject =~ s/</&lt;/g;
    $subject =~ s/>/&gt;/g;

    $from = substr($from, 0, $CONFIG{'MAX_COL_LEN'}) . "..." if (length($from)>$CONFIG{'MAX_COL_LEN'});
    $subject = substr($subject, 0, $CONFIG{'MAX_COL_LEN'}) . "..." if (length($subject)>$CONFIG{'MAX_COL_LEN'});

    my($rclass);
    $rclass = "spam" if ($class eq "I" || $class eq "W" || $class eq "F");
    $rclass = "innocent" if ($class eq "S" || $class eq "M");

    my($retrain);
    if ($rec{$signature}->{'class'} =~ /^(M|F)$/ && $rec{$signature}->{'count'} % 2 != 0) {
      $retrain = "<b>Retrained</b>";
    } 

    if ($retrain eq "") {
      $retrain = qq!<A HREF="$CONFIG{'ME'}?template=$FORM{'template'}&history_page=$history_page&user=$FORM{'user'}&retrain=$rclass&signatureID=$signature">As ! . ucfirst($rclass) . "</A>";
    } else {
      $retrain .= qq! (<A HREF="$CONFIG{'ME'}?template=$FORM{'template'}&history_page=$history_page&user=$FORM{'user'}&retrain=$rclass&signatureID=$signature">Undo</A>)!;
    }

    my($path) = "$USER.frag/$signature.frag";
    if (-e $path) {
      my(%pairs);
      $pairs{'template'} = "fragment";
      $pairs{'signatureID'} = $signature;
      my($sub) = $subject;
      $sub =~ s/#//g;
      $sub =~ s/(['])/\\$1/g;
      $pairs{'subject'} = $sub;
      $pairs{'from'} = $from;
      $pairs{'info'} = $info;
      $pairs{'time'} = $ctime;
      $pairs{'user'} = $FORM{'user'};
      my($url) = &SafeVars(%pairs);
      $from = qq!<a href="javascript:openwin(580,400,1,'$CONFIG{'ME'}?$url')">$from</a>!;
    }

    my($entry) = <<_END;
<tr>
	<td class="$cl $rowclass" nowrap="true"><small>$cllabel</td>
        <td class="$rowclass" nowrap="true"><small>
	 <input name="msgid$retrain_checked_msg_no" type="checkbox" value="$rclass:$signature">
	 $retrain</td>
	<td class="$rowclass" nowrap="true"><small>$ctime</td>
	<td class="$rowclass" nowrap="true"><small>$from</td>
	<td class="$rowclass" nowrap="true"><small>$subject</td>
	<td class="$rowclass" nowrap="true"><small>$info</td>
</tr>
_END
    $retrain_checked_msg_no++;
    push(@history, $entry);

    if ($rowclass eq "rowEven") {
      $rowclass = "rowOdd";
    } else {
      $rowclass = "rowEven";
    }

  }

  my $entry = <<_END;
	<input name="history_page" type="hidden" value="$history_page">
_END
  push(@history, $entry);

  while($line = pop(@history)) { $DATA{'HISTORY'} .= $line; }

  if ($CONFIG{'HISTORY_PER_PAGE'} > 0) {
    $DATA{'HISTORY'} .= "<center>[";
    if (($history_pages > 1) && ($history_page > 1)) {
      my $i = $history_page-1;
      $DATA{'HISTORY'} .= "<a href=\"$CONFIG{'DSPAM_CGI'}?user=$FORM{'user'}&template=$FORM{'template'}&history_page=$i\">&nbsp;&lt;&nbsp;</a>";
    }
    for(my $i = 1; $i <= $history_pages; $i++) {
  
      if ($i == $history_page) {
        $DATA{'HISTORY'} .= "<a href=\"$CONFIG{'DSPAM_CGI'}?user=$FORM{'user'}&template=$FORM{'template'}&history_page=$i\"><big><strong>&nbsp;$i&nbsp;</strong></big></a>";
      } else {
       $DATA{'HISTORY'} .= "<a href=\"$CONFIG{'DSPAM_CGI'}?user=$FORM{'user'}&template=$FORM{'template'}&history_page=$i\">&nbsp;$i&nbsp;</a>";
      }
    }
    if (($history_pages > 1) && ($history_page < $history_pages)) {
      my $i = $history_page+1;
      $DATA{'HISTORY'} .= "<a href=\"$CONFIG{'DSPAM_CGI'}?user=$FORM{'user'}&template=$FORM{'template'}&history_page=$i\">&nbsp;&gt;&nbsp;</a>";
    }
    $DATA{'HISTORY'} .= "]</center><BR>";
  }
  &output(%DATA);
}

#
# Analysis Functions
#

sub DisplayAnalysis {
  my($LOG) = "$USER.log";

  my %Stats=(
    daily	=> {},
    weekly	=> {},
  );

  my ($min, $hour, $mday, $mon, $year) = (localtime(time))[1,2,3,4,5];
  my ($daystart) = timelocal(0, 0, 0, $mday, $mon, $year);
  my ($periodstart) = $daystart - (3600*24*13); # 2 Weeks ago
  my ($dailystart) = time - (3600*23);

  if (! -e $LOG) {
    &error("No historical data is available.");
  }

  open(LOG, "<$LOG") || &error("Unable to open logfile: $!");
  while(<LOG>) {
    my($t_log, $c_log) = split(/\t/);

    # Only Parse Log Data in our Time Period
    if ($t_log>=$periodstart) {
      my($tmin, $thour, $tday, $tmon) = (localtime($t_log))[1,2,3,4];
      $tmon++;

      foreach my $period (qw( weekly daily )) {
        my $idx;
        if ($period eq "weekly") {
          $idx="$tmon/$tday";
        } else { 
          ($t_log>=$dailystart) || next;
          $idx=To12Hour($thour);
        }
        if (!exists $Stats{$period}->{$idx}) {
          $Stats{$period}->{$idx}={
		nonspam	=>	0,
		spam	=>	0,
		title	=>	$idx,
		idx	=>	$t_log,
          };
        }
        my $hr=$Stats{$period}->{$idx};
        if ($c_log eq "S") {
          $hr->{spam}++;
        }
        if ($c_log eq "I" || $c_log eq "W") {
          $hr->{nonspam}++;
        }
        if ($c_log eq "F") {
          $hr->{spam}--;
          ($hr->{spam}<0) && ($hr->{spam}=0);
          $hr->{nonspam}++;
        }
        if ($c_log eq "M") {
          $hr->{nonspam}--;
          ($hr->{nonspam}<0) && ($hr->{nonspam}=0);
          $hr->{spam}++;
        }
      }
    }
  }
  close(LOG);

  foreach my $period (qw( daily weekly )) {
    my $uc_period=uc($period);
    my $hk="DATA_$uc_period";
    my %lst=();
    foreach my $hr (sort {$a->{idx}<=>$b->{idx}} (values %{$Stats{$period}})) {
      foreach my $type (qw( spam nonspam title )) {
        (exists $lst{$type}) || ($lst{$type}=[]);
        push(@{$lst{$type}},$hr->{$type});
        my $totk="";
        if ($type eq "spam") { $totk="S"; }
        elsif ($type eq "nonspam") { $totk="I"; }
        ($totk eq "") && next;
        my $sk="T${totk}_$uc_period";
        (exists $DATA{$sk}) || ($DATA{$sk}=0);
        $DATA{$sk}+=$hr->{$type};
      }
    }
    $DATA{$hk}=join("_",
		join(",",@{$lst{spam}}),
		join(",",@{$lst{nonspam}}),
		join(",",@{$lst{title}}),
	);
  }

  &output(%DATA);
}

#
# Preferences Functions
#

sub DisplayPreferences {
  my(%PREFS);
  my($FILE) = "$USER.prefs";

  my $username = $CURRENT_USER;

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

    if ($FORM{'showFactors'} ne "on") {
      $FORM{'showFactors'} = "off";
    }
                                                                                
    if ($FORM{'enableWhitelist'} ne "on") {
      $FORM{'enableWhitelist'} = "off";
    }

    if ($CONFIG{'PREFERENCES_EXTENSION'} == 1) {

      if ($FORM{'spamSubject'} eq "") {
        $FORM{'spamSubject'} = "''";
      } else {
        $FORM{'spamSubject'} = quotemeta($FORM{'spamSubject'});
      }

      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " trainingMode " . quotemeta($FORM{'trainingMode'}) . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " spamAction " . quotemeta($FORM{'spamAction'}) . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " signatureLocation "
        . quotemeta($FORM{'signatureLocation'}) . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " spamSubject " . $FORM{'spamSubject'} . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " statisticalSedation "
        . quotemeta($FORM{'statisticalSedation'}) . " > /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " enableBNR "
        . quotemeta($FORM{'enableBNR'}) . "> /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " optOut "
        . quotemeta($FORM{'optOut'}) . ">/dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " optIn "
        . quotemeta($FORM{'optIn'}) . ">/dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " showFactors "
        . quotemeta($FORM{'showFactors'}) . "> /dev/null");
      system("$CONFIG{'DSPAM_BIN'}/dspam_admin ch pref ".quotemeta($CURRENT_USER).
        " enableWhitelist "
        . quotemeta($FORM{'enableWhitelist'}) . "> /dev/null");


    } else {
      open(FILE, ">$FILE") || do { &error("Unable to write preferences: $!"); };
      print FILE <<_END;
trainingMode=$FORM{'trainingMode'}
spamAction=$FORM{'spamAction'}
spamSubject=$FORM{'spamSubject'}
statisticalSedation=$FORM{'statisticalSedation'}
enableBNR=$FORM{'enableBNR'}
optIn=$FORM{'optIn'}
optOut=$FORM{'optOut'}
showFactors=$FORM{'showFactors'}
enableWhitelist=$FORM{'enableWhitelist'}
signatureLocation=$FORM{'signatureLocation'}
_END
      close(FILE);
    }
  redirect("$CONFIG{'ME'}?user=$FORM{'user'}&template=preferences");
  }

  %PREFS = GetPrefs();

  $DATA{"SEDATION_$PREFS{'statisticalSedation'}"} = "CHECKED";
  $DATA{"S_".$PREFS{'trainingMode'}} = "CHECKED";
  $DATA{"S_ACTION_".uc($PREFS{'spamAction'})} = "CHECKED"; 
  $DATA{"S_LOC_".uc($PREFS{'signatureLocation'})} = "CHECKED";
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

  if ($CONFIG{'OPTMODE'} eq "OUT") {
    $DATA{"OPTION"} = "<INPUT TYPE=CHECKBOX NAME=optOut " . $DATA{'C_OPTOUT'} . ">Disable DSPAM filtering<br>";
  } elsif ($CONFIG{'OPTMODE'} eq "IN") {
    $DATA{"OPTION"} = "<INPUT TYPE=CHECKBOX NAME=optIn " . $DATA{'C_OPTIN'} . ">Enable DSPAM filtering<br>";
  } else {
    $DATA{"OPTION"} = "";
  }

  &output(%DATA);
}

#
# Quarantine Functions
#

sub ProcessQuarantine {
  if ($FORM{'manyNotSpam'} ne "") {
    &Quarantine_ManyNotSpam;
  } else {
    &Quarantine_DeleteSpam;
  }
  &CheckQuarantine;
  return;
}

sub ProcessFalsePositive {
  my(@buffer, %head, $found);
  if ($FORM{'signatureID'} eq "") {
    &error("No Message ID Specified");
  }
  open(FILE, "<$MAILBOX");
  while(<FILE>) {
    chomp;
    push(@buffer, $_);
  }
  close(FILE);

  while($#buffer>=0) {
    my($buff, $mode, @temp);
    $mode = 0;
    @temp = ();
    while(($buff !~ /^From /) && ($#buffer>=0)) {
      $buff = $buffer[0];
      if ($buff =~ /^From /) {
        if ($mode == 0) { $mode = 1; }
        else { next; }
      }
      $buff = shift(@buffer);
      if ($buff !~ /^From /) {
        push(@temp, $buff);
      }
      next;
    }
    foreach(@temp) {
      last if ($_ eq "");
      my($key, $val) = split(/\: ?/, $_, 2);
      $head{$key} = $val;
    }
    if ($head{'X-DSPAM-Signature'} eq $FORM{'signatureID'}) {
      $found = 1;
      open(PIPE, "|$CONFIG{'DSPAM'} $CONFIG{'DSPAM_ARGS'}  >$TMPFILE 2>&1") || &error($!);
      foreach(@temp) {
        print PIPE "$_\n";
      }
      close(PIPE);
    }
  }

  # Couldn't find the message, so just retrain on signature
  if (!$found) {
    system("$CONFIG{'DSPAM'} --source=error --class=innocent --signature=" . quotemeta($FORM{'signatureID'}) . " --user " . quotemeta("$CURRENT_USER"));
  }

  if ($?) {
    my(@log);
    open(LOG, "<$TMPFILE");
    @log = <LOG>;
    close(LOG);
    unlink("$TMPFILE");
    &error("<PRE>".join('', @log)."</PRE>");
  }

  unlink("$TMPFILE");
  $FORM{$FORM{'signatureID'}} = "on";
  &Quarantine_DeleteSpam();
  return;
}

sub Quarantine_ManyNotSpam {
  my(@buffer, @errors);

  open(FILE, "<$MAILBOX");
  while(<FILE>) {
    chomp;
    push(@buffer, $_);
  }
  close(FILE);

  open(FILE, ">$MAILBOX") || &error($!);
  open(RETRAIN, ">>$USER.retrain.log");

  while($#buffer>=0) {
    my($buff, $mode, @temp, %head, $delivered);
    $mode = 0;
    while(($buff !~ /^From /) && ($#buffer>=0)) {
      $buff = $buffer[0];
      if ($buff =~ /^From /) {
        if ($mode == 0) {
          $mode = 1;
          $buff = shift(@buffer);
          push(@temp, $buff);
          $buff = "";
          next;
        } else {
          next;
        }
      }
      $buff = shift(@buffer);
      push(@temp, $buff);
      next;
    }
    foreach(@temp) {
      last if ($_ eq "");
      my($key, $val) = split(/\: ?/, $_, 2);
      $head{$key} = $val;
    }
    $delivered = 0;
    if ($FORM{$head{'X-DSPAM-Signature'}} ne "") {
      my($err);
      $err = &Deliver(@temp);
      if ($err eq "") {
        $delivered = 1;
      } else {
        push(@errors, $err);
      }
    }
    if (!$delivered) {
      foreach(@temp) {
        print FILE "$_\n";
      }
    } else {
      print RETRAIN time . "\t$head{'X-DSPAM-Signature'}\tinnocent\n";
    }
  }
  close(RETRAIN);
  close(FILE);
  if (@errors > 0) {
    &error(join("<BR>", @errors));
  }
  return;
}

sub Deliver {
  my(@temp) = @_;
  open(PIPE, "|$CONFIG{'DSPAM'} $CONFIG{'DSPAM_ARGS'}") || return $!;
  foreach(@temp) {
    print PIPE "$_\n" || return $!;
  }
  close(PIPE) || return $!;
  return "";
}

sub Quarantine_ViewMessage {
  my(@buffer);

  if ($FORM{'signatureID'} eq "") {
    &error("No Message ID Specified");
  }

  $DATA{'MESSAGE_ID'} = $FORM{'signatureID'};

  open(FILE, "<$MAILBOX");
  while(<FILE>) {
    chomp;
    push(@buffer, $_);
  }
  close(FILE);

  while($#buffer>=0) {
    my($buff, $mode, @temp, %head);
    $mode = 0;
    @temp = ();
    while(($buff !~ /^From /) && ($#buffer>=0)) {
      $buff = $buffer[0];
      if ($buff =~ /^From /) {
        if ($mode == 0) { $mode = 1; }
        else { next; }
      }
      $buff = shift(@buffer);
      if ($buff !~ /^From /) {
        push(@temp, $buff);
      }
      next;
    }
    foreach(@temp) {
      last if ($_ eq "");
      my($key, $val) = split(/\: ?/, $_, 2);
      $head{$key} = $val;
    }
    if ($head{'X-DSPAM-Signature'} eq $FORM{'signatureID'}) {
      foreach(@temp) {
        s/</\&lt\;/g;
        s/>/\&gt\;/g;
        $DATA{'MESSAGE'} .= "$_\n";
      }
    }
  }
  $FORM{'template'} = "viewmessage";
  &output(%DATA);
}

sub Quarantine_DeleteSpam {
  my(@buffer);
  if ($FORM{'deleteAll'} ne "") {
    my($sz);

    my($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
     $atime,$mtime,$ctime,$blksize,$blocks)
     = stat("$USER.mbox");
                                                                                
    open(FILE, "<$USER.mbox.size");
    $sz = <FILE>;
    close(FILE); 
    chomp($sz);

    if ($sz == $size) {
      open(FILE, ">$MAILBOX");
      close(FILE);
      unlink("$USER.mbox.size");
      unlink("$USER.mboxwarn");
    } else {
      return;
    }

    $FORM{'template'} = "performance";
    &CheckQuarantine;
    redirect("$CONFIG{'ME'}?user=$FORM{'user'}&template=$FORM{'template'}");
    return; 
  }
  open(FILE, "<$MAILBOX");
  while(<FILE>) {
    chomp;
    push(@buffer, $_);
  }
  close(FILE);
 

  open(FILE, ">$MAILBOX");
 
  while($#buffer>=0) {
    my($buff, $mode, @temp, %head);
    $mode = 0;
    while(($buff !~ /^From /) && ($#buffer>=0)) {
      $buff = $buffer[0];
      if ($buff =~ /^From /) {
        if ($mode == 0) { 
          $mode = 1; 
          $buff = shift(@buffer); 
          push(@temp, $buff); 
          $buff = ""; 
          next; 
        } else { 
          next; 
        }
      }
      $buff = shift(@buffer);
      push(@temp, $buff);
      next;
    }
    foreach(@temp) {
      last if ($_ eq "");
      my($key, $val) = split(/\: ?/, $_, 2);
      $head{$key} = $val;
    }
    if ($FORM{$head{'X-DSPAM-Signature'}} eq "") {
      foreach(@temp) {
        print FILE "$_\n";
      }
    }
  }
  close(FILE);
  return;
}

sub by_rating { $a->{'rating'} <=> $b->{'rating'} }
sub by_subject { lc($a->{'Subject'}) cmp lc($b->{'Subject'}) }
sub by_from { lc($a->{'From'}) cmp lc($b->{'From'}) }

sub DisplayQuarantine {
  my(@alerts);

  my($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
   $atime,$mtime,$ctime,$blksize,$blocks)
   = stat("$USER.mbox");

  open(FILE, ">$USER.mbox.size");
  print(FILE "$size");
  close(FILE);

  open(FILE, ">$MAILBOX.stamp");
  close(FILE);
  chmod 0660, "$MAILBOX.stamp";

  open(FILE, "<$USER.alerts");
  while(<FILE>) {
    chomp;
    push(@alerts, $_);
  }
  close(FILE);

  my($next, @buffer, $rowclass, $mode, $markclass, $marklabel, @headings);
  $rowclass="rowEven";
  open(FILE, "<$MAILBOX");
  while(<FILE>) {
    chomp;
    if ($_ ne "") {
      if ($mode eq "") { 
        if ($_ =~ /^From /) {
          $mode = 1;
        } else { 
          next; 
        } 
      }
      push(@buffer, $_);
      next;
    }
    next if ($mode eq "");

    my($new, $start, $alert);
    $alert = 0;
    $new = {};
    foreach(@buffer) {
      my($al);
      foreach $al (@alerts) {
        if (/$al/i) {
          $alert = 1;
        }
      }
      if ($_ =~ /^From /) {
        my(@a) = split(/ /, $_);
        my($x) = 2;
        for (0..$#a) { 
          if (($a[$_] =~ /\@|>/) && ($_>$x)) { 
            $x = $_ + 1;
          }
        }
        for(1..$x) { shift(@a); }
        $start = join(" ", @a);
      } else {
        my($key, $val) = split(/\: ?/, $_, 2);
        $new->{$key} = $val; 
      }
    }
    if ($rowclass eq "rowEven") { 
      $rowclass = "rowOdd";
    } else { 
      $rowclass = "rowEven";
    }

    $new->{'alert'} = $alert;

    if ($alert) { $rowclass="rowAlert"; }

    $new->{'Sub2'} = $new->{'X-DSPAM-Signature'};
    if (length($new->{'Subject'})>$CONFIG{'MAX_COL_LEN'}) {
      $new->{'Subject'} = substr($new->{'Subject'}, 0, $CONFIG{'MAX_COL_LEN'}) . "...";
    } 
 
    if (length($new->{'From'})>$CONFIG{'MAX_COL_LEN'}) {
      $new->{'From'} = substr($new->{'From'}, 0, $CONFIG{'MAX_COL_LEN'}) . "...";
    }

    if ($new->{'Subject'} eq "") {
      $new->{'Subject'} = "<None Specified>";
    }

#   $new->{'rating'} = $new->{'X-DSPAM-Probability'} * $new->{'X-DSPAM-Confidence'};
    $new->{'rating'} = $new->{'X-DSPAM-Confidence'};

    foreach(keys(%$new)) {
      next if ($_ eq "X-DSPAM-Signature");
      $new->{$_} =~ s/</\&lt\;/g;
      $new->{$_} =~ s/>/\&gt\;/g;
    }

    push(@headings, $new);

    @buffer = ();
    $mode = "";
    next;
  }

  my($sortBy) = $FORM{'sortby'};
  if ($sortBy eq "") {
    $sortBy = $CONFIG{'SORT_DEFAULT'};
  }
  if ($sortBy eq "Rating") {
    @headings = sort by_rating @headings;
  }
  if ($sortBy eq "Subject") {
    @headings = sort by_subject @headings;
  }
  if ($sortBy eq "From") {
    @headings = sort by_from @headings;
  }
  if ($sortBy eq "Date") {
    @headings = reverse @headings;
  }

  $DATA{'SORTBY'} = $sortBy;
  $DATA{'SORT_SELECTOR'} .=  "Sort by: <a href=\"$CONFIG{'ME'}?user=$FORM{'user'}&template=quarantine&sortby=Rating&user=$FORM{'user'}\">";
  if ($sortBy eq "Rating") {
    $DATA{'SORT_SELECTOR'} .= "<strong>Rating</strong>";
  } else {
    $DATA{'SORT_SELECTOR'} .= "Rating";
  }
  $DATA{'SORT_SELECTOR'} .=  "</a> | <a href=\"$CONFIG{'ME'}?user=$FORM{'user'}&template=quarantine&sortby=Date&user=$FORM{'user'}\">";
  if ($sortBy eq "Date") {
    $DATA{'SORT_SELECTOR'} .= "<strong>Date</strong>";
  } else {
    $DATA{'SORT_SELECTOR'} .= "Date";
  }
  $DATA{'SORT_SELECTOR'} .=  "</a> | <a href=\"$CONFIG{'ME'}?user=$FORM{'user'}&template=quarantine&sortby=Subject&user=$FORM{'user'}\">";
  if ($sortBy eq "Subject") {
    $DATA{'SORT_SELECTOR'} .= "<strong>Subject</strong>";
  } else {
    $DATA{'SORT_SELECTOR'} .= "Subject";
  }
  $DATA{'SORT_SELECTOR'} .=  "</a> | <a href=\"$CONFIG{'ME'}?user=$FORM{'user'}&template=quarantine&sortby=From&user=$FORM{'user'}\">";
  if ($sortBy eq "From") {
    $DATA{'SORT_SELECTOR'} .= "<strong>From</strong>";
  } else {
    $DATA{'SORT_SELECTOR'} .= "From";
  }
  $DATA{'SORT_SELECTOR'} .=  "</a>";


  my($row, $rowclass);
  $rowclass = "rowEven";
  for $row (@headings) {
    my($rating, $url, $markclass, $outclass);
    $rating = sprintf("%3.0f%%", $row->{'rating'} * 100.0);
    if ($row->{'rating'} > 0.8) {
	$markclass = "high";
    } else {
	if ($row->{'rating'} < 0.7) {
	    $markclass = "low";
	} else {
	    $markclass = "medium";
	}
    }

    my(%PAIRS);
    $PAIRS{'signatureID'} = $row->{'X-DSPAM-Signature'};
    $PAIRS{'command'}  = "viewMessage";
    $PAIRS{'user'} = $CURRENT_USER;
    $PAIRS{'template'} = "quarantine";
    $url = &SafeVars(%PAIRS);

    if ($row->{'alert'}) {
      $outclass = "rowAlert";
    } else {
      $outclass = $rowclass;
    }

    my(@ptfields) = split(/\s+/, $row->{'X-DSPAM-Processed'});
    my(@times) = split(/\:/, $ptfields[3]);
    my($ptime);
    if($CONFIG{"DATE_FORMAT"}) {
      my($month);
      $month->{'Jan'}=0;
      $month->{'Feb'}=1;
      $month->{'Mar'}=2;
      $month->{'Apr'}=3;
      $month->{'May'}=4;
      $month->{'Jun'}=5;
      $month->{'Jul'}=6;
      $month->{'Aug'}=7;
      $month->{'Sep'}=8;
      $month->{'Oct'}=9;
      $month->{'Nov'}=10;
      $month->{'Dec'}=11;
      $ptime = strftime($CONFIG{"DATE_FORMAT"}, $times[2],$times[1],$times[0],$ptfields[2],$month->{$ptfields[1]},$ptfields[4]-1900);
    } else {
      my($mer) = "a";
      if ($times[0] > 12) { $times[0] -= 12; $mer = "p"; }
      if ($times[0] == 0) { $times[0] = "12"; }
      $ptime = "$ptfields[1] $ptfields[2] $times[0]:$times[1]$mer";
      }

    $DATA{'QUARANTINE'} .= <<_END;
<tr>
	<td class="$outclass" nowrap="true"><input type="checkbox" name="$row->{'X-DSPAM-Signature'}"></td>
	<td class="$outclass $markclass" nowrap="true">$rating</td>
        <td class="$outclass" nowrap="true">$ptime</td>
	<td class="$outclass" nowrap="true">$row->{'From'}</td>
	<td class="$outclass" nowrap="true"><a href="$CONFIG{'ME'}?$url">$row->{'Subject'}</a></td>
</tr>
_END

    if ($rowclass eq "rowEven") {
      $rowclass = "rowOdd";
    } else {
      $rowclass = "rowEven";
    }
  }

  &output(%DATA);
}

#
# Performance Functions
#

sub ResetStats {
  my($ts, $ti, $tm, $fp, $sc, $ic);
  my($group);
  open(FILE, "<$USER.stats");
  chomp($ts = <FILE>);
  chomp($group = <FILE>); 
  close(FILE);
  ($ts, $ti, $tm, $fp, $sc, $ic) = split(/\,/, $ts);
  
  if ($group ne "") {
    my($GROUP) = GetPath($group) . ".stats";
    my($gts, $gti, $gtm, $gfp, $gsc, $gic);
    open(FILE, "<$GROUP");
    chomp($gts = <FILE>);
    close(FILE);
    ($gts, $gti, $gtm, $gfp, $gsc, $gic) = split(/\,/, $gts);
    $ts       -= $gts;
    $ti       -= $gti;
    $tm       -= $gtm;
    $fp       -= $gfp;
    $sc       -= $gsc;
    $ic       -= $gic;
  }

  open(FILE, ">$USER.rstats");
  print FILE "$ts" . "," . "$ti" . "," . "$tm" . "," .
             "$fp" . "," . "$sc" . "," . "$ic\n";
  close(FILE);
}

sub Tweak {
  my($ts, $ti, $tm, $fp, $sc, $ic);
  open(FILE, "<$USER.rstats");
  chomp($ts = <FILE>);
  close(FILE);
  ($ts, $ti, $tm, $fp, $sc, $ic) = split(/\,/, $ts);
  $tm++;
  open(FILE, ">$USER.rstats");
  print FILE "$ts,$ti,$tm,$fp,$sc,$ic\n";
  close(FILE);
}

sub DisplayIndex {
  my($spam, $innocent, $ratio, $fp, $misses);
  my($rts, $rti, $rtm, $rfp, $sc, $ic, $overall, $fpratio, $monthly, 
     $real_missed, $real_caught, $real_fp, $real_innocent);
  my($time) = ctime(time);
  my($group);

  open(FILE, "<$USER.stats");
  chomp($spam = <FILE>);
  chomp($group = <FILE>); 
  close(FILE);
  ($spam, $innocent, $misses, $fp, $sc, $ic) = split(/\,/, $spam);

  if ($group ne "") {
    my($GROUP) = GetPath($group) . ".stats";
    my($gspam, $ginnocent, $gmisses, $gfp, $gsc, $gic);
    open(FILE, "<$GROUP");
    chomp($gspam = <FILE>);
    close(FILE);
    ($gspam, $ginnocent, $gfp, $gmisses, $gsc, $gic) = split(/\,/, $gspam);
    $spam     -= $gspam;
    $innocent -= $ginnocent;
    $misses   -= $gmisses;
    $fp       -= $gfp;
    $sc       -= $gsc;
    $ic       -= $gic;
  }

  if ($spam+$innocent>0) { 
    $ratio = sprintf("%2.3f", 
      (($spam+$misses)/($spam+$misses+$fp+$innocent)*100)); 
  } else { 
    $ratio = 0; 
  }

  if (open(FILE, "<$USER.rstats")) {
    my($rstats);
   
    chomp($rstats = <FILE>);
    ($rts, $rti, $rtm, $rfp) = split(/\,/, $rstats);
    close(FILE);
    $real_missed = $misses-$rtm;
    $real_caught = $spam-$rts;
    $real_fp = $fp-$rfp;
    if ($real_fp < 0) { $real_fp = 0; }
    $real_innocent = $innocent-$rti; 
    if (($spam-$rts > 0) && ($spam-$rts + $misses-$rtm != 0) &&
        ($real_caught+$real_missed>0) && ($real_fp+$real_innocent>0)) {
      $monthly = sprintf("%2.3f", 
        (100.0-(($real_missed)/($real_caught+$real_missed))*100.0));
      $overall = sprintf("%2.3f", 
        (100-((($real_missed+$real_fp) / 
        ($real_fp+$real_innocent+$real_caught+$real_missed))*100)));
    } else {
      if ($real_caught == 0 && $real_missed > 0) {
        $monthly = 0;
        $overall = 0;
      } else {
        $monthly = 100;
        $overall = 100;
      }
    }

    if ($real_fp+$real_innocent>0) {
      $fpratio = sprintf("%2.3f", ($real_fp/($real_fp+$real_innocent)*100));
    } else {
      $fpratio = 0;
    }

  } else {
    $rts = $spam+$misses;
    $rti = $innocent;
    $rtm = $misses;
    $rfp = $fp;
    open(FILE, ">$USER.rstats");
    print FILE "$rts,$rti,$rtm,$rfp\n";
    close(FILE);
    $monthly = "N/A";
    $fpratio = "N/A";
    $overall = "N/A";
  }

  $DATA{'TIME'} = $time;
  $DATA{'TOTAL_SPAM_SCANNED'} = $spam;
  $DATA{'TOTAL_SPAM_LEARNED'} = $misses;
  $DATA{'TOTAL_NONSPAM_SCANNED'} = $innocent;
  $DATA{'TOTAL_NONSPAM_LEARNED'} = $fp;
  $DATA{'SPAM_RATIO'} = $ratio;
  $DATA{'SPAM_ACCURACY'} = $monthly;
  $DATA{'NONSPAM_ERROR_RATE'} = $fpratio;
  $DATA{'OVERALL_ACCURACY'} = $overall;
  $DATA{'TOTAL_SPAM_CORPUSFED'} = $sc;
  $DATA{'TOTAL_NONSPAM_CORPUSFED'} = $ic;
  $DATA{'TOTAL_SPAM_MISSED'} = $real_missed;
  $DATA{'TOTAL_SPAM_CAUGHT'} = $real_caught;
  $DATA{'TOTAL_NONSPAM_MISSED'} = $real_fp;
  $DATA{'TOTAL_NONSPAM_CAUGHT'} = $real_innocent;

  if ($CURRENT_USER !~ /\@/) {
    $DATA{'LOCAL_DOMAIN'} = "\@$CONFIG{'LOCAL_DOMAIN'}";
  }  

  &output(%DATA);
}

#
# Alert Functions
#

sub AddAlert {
  if ($FORM{'ALERT'} eq "") {
    &error("No Alert Specified");
  }
  open(FILE, ">>$USER.alerts");
  print FILE "$FORM{'ALERT'}\n";
  close(FILE);
  return;
}
                                                                                
sub DeleteAlert {
  my($line, @alerts);
  $line = 0;
  if ($FORM{'line'} eq "") {
    &Error("No Alert Specified");
  }
  open(FILE, "<$USER.alerts");
  while(<FILE>) {
    if ($line != $FORM{'line'}) {
      push(@alerts, $_);
    }
    $line++;
  }
  close(FILE);
                                                                                
  open(FILE, ">$USER.alerts");
  foreach(@alerts) { print FILE $_; }
  close(FILE);
  return;
}

sub DisplayAlerts {
  my($supp);
  
  $DATA{'ALERTS'} = <<_end;
<table border="0" cellspacing="0" cellpadding="2">
	<tr>
		<th>Alert Name</th>
		<th>&nbsp;</th>
	</tr>
_end

  my($rowclass);
  $rowclass = "rowEven";

  do {
    my($line) = 0;
    open(FILE, "<$USER.alerts");
    while(<FILE>) {
      s/</&lt;/g;
      s/>/&gt;/g;
      $DATA{'ALERTS'} .= qq!<tr><td class="$rowclass">$_</td><td class="$rowclass">[<a href="$CONFIG{'ME'}?command=deleteAlert&user=$FORM{'user'}&template=alerts&line=$line">Remove</a>]</td></tr>\n!;
      $line++;

      if ($rowclass eq "rowEven") {
	$rowclass = "rowOdd";
      } else {
	$rowclass = "rowEven";
      }
    }
  }; 

$DATA{'ALERTS'} .= <<_end;
</table>
_end

  &output(%DATA);
  exit;
}

#
# Global Functions
#

sub redirect {
  my($loc) = @_;
  print "Expires: now\n";
  print "Pragma: no-cache\n";
  print "Cache-control: no-cache\n";
  print "Location: $loc\n\n";
  exit(0);
  }

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

  # Check admin permissions
  do {
    if ($CONFIG{'ADMIN'} == 1) {
      $DATA{'NAV_ADMIN'} = qq!<li><a href="admin.cgi">Administrative Suite</a></li>!;
      $DATA{'FORM_USER'} = qq!<form action="$CONFIG{'ME'}"><input type=hidden name="template" value="$FORM{'template'}">Statistical SPAM Protection for <INPUT TYPE=TEXT NAME=user SIZE=16 value="$CURRENT_USER"> <input type=submit value="Change"></form>!;
    } else {
      $DATA{'NAV_ADMIN'} = '';
      $DATA{'FORM_USER'} = "Statistical SPAM Protection for <strong>$CURRENT_USER</strong>";
    }
  };

  open(FILE, "<$CONFIG{'TEMPLATES'}/nav_$FORM{'template'}.html");
  while(<FILE>) { 
    s/\$CGI\$/$CONFIG{'ME'}/g;
    if ($CONFIG{'ADMIN'} == 1 && $FORM{'user'}) {
      s/\$USER\$/user=$FORM{'user'}&/g;
    } else {
      s/\$USER\$//g;
    }
    s/\$([A-Z0-9_]*)\$/$DATA{$1}/g; 
    print;
  }
  close(FILE);
  exit(0);
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
  exit;
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

    if ($name =~ /^msgid[\d]+$/) {
	push(@{ $FORM{retrain_checked} }, $value);
    } else {
	$FORM{$name} = $value;
    }
  }
  return %FORM;
}

sub CheckQuarantine {
  my($f)=0;
  open(FILE, "<$MAILBOX");
  while(<FILE>) {
    next unless (/^From /);
    $f++;
  }
  close(FILE);   
  if ($f == 0) {
    $f = "Empty";
  }

  $DATA{'TOTAL_QUARANTINED_MESSAGES'} = $f;
}

sub To12Hour {
  my($h) = @_;
  if ($h < 0) { $h += 24; }
  if ($h>11) { $h -= 12 if ($h>12) ; $h .= "p"; }
  else { $h = "12" if ($h == 0); $h .= "a"; }
  return $h;
}

sub GetPath {
  my($USER) = @_;
  my($PATH);

  # Domain-scale
  if ($CONFIG{'DOMAIN_SCALE'} == 1) {
    my($VPOPUSERNAME, $VPOPDOMAIN);

    $VPOPUSERNAME = (split(/@/, $USER))[0];
    $VPOPDOMAIN = (split(/@/, $USER))[1];
    $VPOPDOMAIN = "local" if ($VPOPDOMAIN eq "");

    $PATH = "$CONFIG{'DSPAM_HOME'}/data/$VPOPDOMAIN/$VPOPUSERNAME/" .
            "$VPOPUSERNAME";
    return $PATH;

  # Normal scale
  } elsif ($CONFIG{'LARGE_SCALE'} == 0) {
    $PATH = "$CONFIG{'DSPAM_HOME'}/data/$USER/$USER";
    return $PATH;
                                                                                
  # Large-scale
  } else {
    if (length($USER)>1) {
      $PATH = "$CONFIG{'DSPAM_HOME'}/data/" . substr($USER, 0, 1) .
        "/". substr($USER, 1, 1) . "/$USER/$USER";
    } else {
      $PATH = "$CONFIG{'DSPAM_HOME'}/data/$USER/$USER";
    }
    return $PATH;
  }

  &error("Unable to determine filesystem scale");
}


sub GetPrefs {
  my(%PREFS);

  my($FILE) = "$USER.prefs";

  if ($CONFIG{'PREFERENCES_EXTENSION'} == 1) {
    open(PIPE, "$CONFIG{'DSPAM_BIN'}/dspam_admin agg pref " . quotemeta($CURRENT_USER) . "|");
    while(<PIPE>) {
      chomp;
      my($directive, $value) = split(/\=/);
      $PREFS{$directive} = $value;
    }
    close(PIPE);
  }

  if (keys(%PREFS) eq "0" || $CONFIG{'PREFERENCES_EXTENSION'} != 1) {

    if (! -e "./default.prefs") {
      &error("Unable to load default preferences");
    }
    open(FILE, "<./default.prefs");
    while(<FILE>) {
      chomp;
      my($directive, $value) = split(/\=/);
      $PREFS{$directive} = $value;
    }
    close(FILE);

    if( -e $FILE) {
      open(FILE, "<$FILE");
      while(<FILE>) {
	chomp;
	my($directive, $value) = split(/\=/);
	$PREFS{$directive} = $value;
      }
      close(FILE);
    }
  }
  return %PREFS
}
