#!/usr/bin/perl

# FIXME: This script should be made more portable using parameters

use File::Find;
use File::Temp qw/ tempfile /;

$source = '/home/main/store/cvs-track/mahogany/source';
$build = '/home/main/store/cvs-track/mahogany/build';
$compiler =
"c++ -fsyntax-only -c -I$build/include -I$source/include -DDEBUG"
." -DDEBUG_main -I/usr/lib/wx/include/gtkd-2.4 -D__WXDEBUG__ -D__WXGTK__"
." -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$source/extra/include"
." -I$build/extra/src/c-client -I$source/extra/src/compface"
." -I$source/src/wx/vcard -fno-exceptions -fno-rtti -fno-operator-names"
." -O0 -Wall -x c++ -Werror";

find \&AllFiles, '.';
#$_ = 'wxFiltersDialog.cpp';
#chdir 'src/gui';
#AllFiles();

sub CompileFile
{
   my ($source) = shift(@_);
   my ($output,$outputfile) = tempfile();
   close $output or die;
   system "$compiler $source >$outputfile 2>&1";
   unlink $outputfile or die;
}

sub FindIncludes
{
   my(@includes);
   local *SOURCE;
   
   open(SOURCE,shift(@_)) or die;
   while(<SOURCE>)
   {
      next if ! /^ *# *include/;
      /["<](.*)[">]/ or die;
      push @includes,$1;
   }
   close(SOURCE) or die;
   @includes;
}

sub RemoveInclude
{
   my ($source) = shift(@_);
   my ($include) = shift(@_);
   local *SOURCE;
   
   my ($scratch,$scratchname) = tempfile();
   open(SOURCE,$source) or die;
   while(<SOURCE>)
   {
      next if /^ *# *include *["<]$include[">]/;
      print $scratch $_;
   }
   close(SOURCE) or die;
   close($scratch) or die;
   
   $scratchname;
}

sub AllFiles
{
   return if ! /.cpp$/;
   my($source)=$_;
   warn "$File::Find::dir $source\n";
   CompileFile $source;
   if($? != 0)
   {
      warn " Compilation without change failed\n";
      return;
   }
   @includes = FindIncludes $source;
   foreach $include (@includes)
   {
      next if $include eq 'Mpch.h' || $include eq 'Mcommon.h';
      $scratchname = RemoveInclude $source,$include;
      CompileFile $scratchname;
      unlink $scratchname or die;
      warn " $include\n" if $? == 0;
   }
}
