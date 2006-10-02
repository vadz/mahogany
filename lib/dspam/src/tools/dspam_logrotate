#!/usr/bin/perl

# dspam_logrotate - Removed old entries from DSPAM log files.
# Steve Pellegrin <spellegrin@convoglio.com>
#
# Patched for working on dspam installation with thousends of users. 
# Norman Maurer <nm@byteaction.de>
#
# Usage:
#    dspam_logrotate -v -a days logfile ...
#      -v: Print verbose output
#      -a days: All log entries older than 'days' will be removed.
#      -l logfile: A list of one or more files to process.
#      -d dspamdir: The home directory of dspam.

###########################################################
#
# Print usage info
#
sub usage {
	print "Usage: " . $0 . " -a age [-v] -l logfiles\n";
	print "or\n";
	print "Usage: " . $0 . " -a age [-v] -d /var/dspam\n";
}
###########################################################


###########################################################
#
# "Rotate" one log file
#
sub rotate {
    # Give names to input args.
    my($filename);
    my($cutoffTimestamp);
    my($printStats);
    ($filename, $cutoffTimestamp, $printStats) = @_;

    # Generate names for the temporary files.
    my($tempInputFile) = $filename . ".in";
    my($tempOutputFile) = $filename . ".out";

    # Rename the log file to the temporary input file name.
    rename $filename, $tempInputFile;

    # Open the temporary input and output files.
    open INFILE, "< $tempInputFile"
        or die "Cannot open input file: $tempInputFile\n$!";
    open OUTFILE, "> $tempOutputFile"
        or die "Cannot open output file: $tempOutputFile\n$!";

    # Read the input file and copy eligible records to the output.
    # Count the number of delete records in case printStats is true.
    my($linesDeleted) = 0;
    while (defined($thisLine = <INFILE>)) {
        # Get this line's timestamp.
	my($lineTimestamp) = substr($thisLine, 0, index($thisLine, "\t"));

	# Write lines with newer timestamps to the output file.
	if ($lineTimestamp >= $cutoffTimestamp) {
	    print OUTFILE $thisLine;
	} else {
	    $linesDeleted++;
	}
    }
    close INFILE;

    # It is possible that records have been written to the log file while
    # we have been processing the temporary files. If so, append them to 
    # our temporary output file.
    if (-e $filename) {
        open INFILE, "< $filename"
            or die "Cannot open log file: $filename\n$!";
        while (defined($thisLine = <INFILE>)) {
	    print OUTFILE $thisLine;
        }
        close INFILE;
    }

    # Rename our temporary output file to the original log file name.
    close OUTFILE;
    rename $tempOutputFile, $filename;

    # Remove our temporary input file.
    unlink $tempInputFile;

    # Print statistics, if desired.
    if ($printStats != 0) {
        print "Deleted $linesDeleted lines from $filename \n";
    }
}
###########################################################


###########################################################
#
# Mainline
#
###########################################################

# Extract the command line arguments
#    -a days: All log entries older than 'days' will be removed.
#    -v: Print verbose output
#    logfile: A list of one or more files to process.
#
my($ageDays) = undef;
my($logfiles);
my($help) = 0;
my($verbose) = 0;

while ($arg = shift(@ARGV)) {
    if ($arg eq "-a") {
        $ageDays = shift(@ARGV);
    } elsif ($arg eq "-v") {
        $verbose = 1;
    } elsif ($arg eq "-l") {
        @logfiles = @ARGV;
    } elsif ($arg eq "-d") {
    	my $dspamdir = shift(@ARGV);
        @logfiles = split(/\n/, `find $dspamdir -name \"*.log\"`);
    } elsif ($arg eq "-h") {
        $help = 1;
    }
}

#
# Quit now if the command line looks screwy.
#
if (!defined($ageDays) || (scalar @logfiles == 0) || $help == 1) {
    usage();
    exit(-1);
}

#
# Determine the earliest timestamp allowed to stay in the file.
#
my($minimumTimestamp) = (time - ($ageDays * 60 * 60 * 24));

#
# Rotate each logfile specified on the command line.
#
foreach $logfile (@logfiles) {
    rotate($logfile, $minimumTimestamp, $verbose);
}
