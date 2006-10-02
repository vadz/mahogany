#!/usr/bin/perl

# train.pl
# This tool trains a corpus of messages (a directory containing a nonspam and
# a spam subdirectory, each with messages in individual files) to a specific
# user and reports on errors. 
#
# You can use four different training paradigms; the three stock training modes
# included with dspam (teft, toe, tum) or tune (train until no errors) by 
# setting toe mode training and rerunning this script until little or no errors
# are generated.

use strict;
use vars qw { $USER $PATH $REPORTING_WINDOW $CORPUS $TRAINING_MODE };

$REPORTING_WINDOW  = 250;			# How often to summarize
$PATH              = "/usr/local/dspam/bin";	# Path to dspam binaries
$TRAINING_MODE     = "teft";			# Training mode

### DO NOT CONFIGURE BELOW THIS LINE ###

$USER = shift;
$CORPUS = shift;

if ($CORPUS eq "") {
  die "Usage: $0 [username] [corpus]";
}

&Train("$CORPUS/nonspam", "$CORPUS/spam");

sub Train {
  my($nonspam, $spam) = @_;
  my(@nonspam_corpus, @spam_corpus);
  my($ic, $sc, $fp, $sm, $num);

  print "Training $nonspam / $spam corpora...\n";
  @nonspam_corpus = GetFiles($nonspam);
  @spam_corpus = GetFiles($spam); 

  $ic = $sc = $fp = $sm = $num = 0; 

  while($#nonspam_corpus && $#spam_corpus) {
    my($nonspam_msg) = shift(@nonspam_corpus);
    my($spam_msg) = shift(@spam_corpus);
    my($cmd, $response);
 
    # Process one nonspam

    $cmd = "$PATH/dspam --user $USER --mode=$TRAINING_MODE --deliver=stdout < $nonspam/$nonspam_msg";
    $response = `$cmd`;
    if ($response =~ /X-DSPAM-Result: (Innocent|Whitelisted)/i) {
      $ic++;
    } else {
      $fp++;
      print "FP: $nonspam_msg\n";
      open(TRAIN, "|$PATH/dspam --user $USER --mode=$TRAINING_MODE --class=innocent --source=error");
      print TRAIN $response;
      close(TRAIN);
    }

    # Process one spam

    $cmd = "$PATH/dspam --user $USER --mode=$TRAINING_MODE --deliver=stdout < $spam/$spam_msg";
    $response = `$cmd`;
    if ($response =~ /X-DSPAM-Result: Spam/i) {
      $sc++;
    } else {
      print "SM: $spam_msg\n";
      $sm++;
      open(TRAIN, "|$PATH/dspam --user $USER --mode=$TRAINING_MODE --class=spam --source=error");
      print TRAIN $response;
      close(TRAIN);
    }
    $num+=2;

    if ($num % $REPORTING_WINDOW == 0) {
      print "Spam Correct   : $sc\n";
      print "Spam Missed    : $sm\n";
      print "Nonspam Correct: $ic\n";
      print "Nonspam Missed : $fp\n"; 
      print "--------------------\n";
      $sc = $sm = $ic = $fp = $num = 0;
    }
  }

  print "Spam Correct   : $sc\n";
  print "Spam Missed    : $sm\n";
  print "Nonspam Correct: $ic\n";
  print "Nonspam Missed : $fp\n\n";

  system("$PATH/dspam_stats -S $USER");
}

sub GetFiles {
  my($corpus) = @_;
  my(@files);

  opendir(DIR, "./$corpus") || die $!;
  @files = grep(!/^\./, readdir(DIR));
  closedir(DIR);
  return @files;
} 
