#!/usr/bin/perl

# FIXME: This script should be made more portable using parameters
# FIXME: Forward declarations don't really work
# FIXME: Find unterminated USE_PCH blocks
# FIXME: Stronger checking for "// for wxSomeClass" comments

use File::Find;
use File::Temp qw/ tempfile /;
use File::Basename;

$base = '/home/main/store/cvs-track/mahogany/source';
$build = '/home/main/store/cvs-track/mahogany/build';
$compiler =
"c++ -fsyntax-only -c -I$build/include -I$base/include -DDEBUG"
." -DDEBUG_main -I/usr/lib/wx/include/gtkd-2.4 -D__WXDEBUG__ -D__WXGTK__"
." -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -I$base/extra/include"
." -I$build/extra/src/c-client -I$base/extra/src/compface"
." -I$base/src/wx/vcard -fno-exceptions -fno-rtti -fno-operator-names"
." -O0 -Wall -x c++ -Werror";
@includepath = ("$build/include", "$base/include",
   "/usr/lib/wx/include/gtkd-2.4", "$base/extra/include",
   "$build/extra/src/c-client", "$base/extra/src/compface",
   "$base/src/wx/vcard", "/usr/include", "/usr/include/linux");

@precompiled = FindIncludes("include/Mpch.h");
$precompiled{$_} = 1 foreach (@precompiled);
@precompiled = FindIncludes("/usr/include/wx/wx.h");
$precompiled{$_} = 1 foreach (@precompiled);

if( $#ARGV >= 0 )
{
   if( $ARGV[0] eq '--eval' )
   {
      $program = $ARGV[1];
      print eval($program),"\n",$@;
   }
   if( $ARGV[0] eq '--one' )
   {
      $header_mode='.cpp';
      $header_mode='.h' if $ARGV[1] =~ /\.h/;
      chdir(dirname($ARGV[1]));
      if( $#ARGV >= 2 )
      {
         TestInclude(basename($ARGV[1]),$ARGV[2]);
      }
      else
      {
         CheckFile(basename($ARGV[1]));
      }
   }
   if( $ARGV[0] eq '--statistics' )
   {
      CalculateStatistics();
   }
   if( $ARGV[0] eq '--fast' )
   {
      $fast = 1;
      Run();
   }
   exit;
}

$fast = 0;
Run();

sub Run
{
   $header_mode='.h';
   find { preprocess => \&SortFiles, wanted => \&AllIncludes }, '.';
   $header_mode='.cpp';
   find { preprocess => \&SortFiles, wanted => \&AllFiles }, '.';
}

sub CompileFile
{
   my ($source) = shift(@_);
   my ($verbose) = shift(@_);
   return CompileInclude($source,$verbose) if $header_mode eq '.h';
   my ($output,$outputfile) = tempfile();
   close $output or die;
   system "$compiler $source".($verbose ? "" : " >$outputfile 2>&1");
   my ($result) = $?;
   unlink $outputfile or die;
   return $result;
}

sub FindIncludes
{
   my(@includes);
   local *SOURCE;
   my ($precompile_section) = 0;
   
   open(SOURCE,shift(@_)) or die;
   while(<SOURCE>)
   {
      $precompile_section = 1 if /# *ifndef +((USE_PCH)|(WX_PRECOMP))/;
      $precompile_section = 0 if m%# *endif *// *((USE_PCH)|(WX_PRECOMP))%;
      next if ! /^ *# *include/;
      /["<]([^"]*)[">]/ or die;
      push @includes,$1;
      $precompile_include = $precompiled{$1} ? 1 : 0;
      if( $precompile_section != $precompile_include )
      {
         warn " $1 - precompile mismatch\n";
      }
      if( $header_mode eq '.h' )
      {
         if( $1 eq 'Mpch.h' || $1 eq 'Mcommon.h' || $1 eq 'Mconfig.h' )
         {
            warn " $1 - should be in cpp file\n";
         }
      }
   }
   close(SOURCE) or die;
   @includes;
}

sub ScanForward
{
   my ($include) = shift(@_);
   local *SOURCE;
   my (@classes);
   
   return if $include =~ /^std/;
   return if $include =~ m#^wx/#;
   open(SOURCE,$include) or open(SOURCE,"$base/include/$include")
      or return;
   while(<SOURCE>)
   {
      push @classes,"class $2;\n" if /^class +(WXDLLEXPORT +)?(\w+)/;
      push @classes,"$1\n" if /^(#include *[<"].*[">])/;
   }
   close(SOURCE) or die;
   
   return @classes;
}

sub RemoveInclude
{
   my ($source) = shift(@_);
   my ($include) = shift(@_);
   my ($replace) = shift(@_);
   local *SOURCE;

   my ($scratch,$scratchname) = tempfile();
   print $scratch ScanForward($include);
   open(SOURCE,$source) or die;
   while(<SOURCE>)
   {
      if( /^ *# *include *["<]$include[">]/ )
      {
         print $scratch $replace;
         next;
      }
      print $scratch $_;
   }
   close(SOURCE) or die;
   close($scratch) or die;
   
   $scratchname;
}

sub BaseInclude
{
   my($include)=shift(@_);
   $include eq 'Mpch.h' || $include eq 'Mcommon.h'
   || $include eq 'wx/wxprec.h' || $include eq 'windows.h'
   || $include eq 'wx/msw/winundef.h';
}

sub ValidateInclude
{
   my($source)=shift(@_);
   my($include)=shift(@_);
   return 0 if BaseInclude $include;
   return 0 if basename($source,'.cpp') eq basename($include,'.h');
   return 0 if $include =~ /.c$/ || $include =~ /.cpp$/;
   return 1;
}

sub IsConditional
{
   my($source)=shift(@_);
   my($include)=shift(@_);
   my($scratchname) = RemoveInclude($source,$include,'#error IsCompiledIn');
   my($result) = CompileFile($scratchname,0) == 0;
   unlink $scratchname or die;
   $result;
}

sub FindForProtection
{
   my($source)=shift(@_);
   my($include)=shift(@_);
   my($protected)='';
   open(SOURCE,$source) or die;
   while(<SOURCE>)
   {
      if( /^ *# *include *["<]$include[">]/ )
      {
         m#// *[Ff]or +(\w+)#;
         $protected = $1;
      }
   }
   close(SOURCE) or die;
   return $protected;
}

sub IsFairlyForProtected
{
   my($source)=shift(@_);
   my($include)=shift(@_);
   my($protected)=FindForProtection($source,$include);
   my($fair)=0;
   return 0 if $protected eq '';
   open(SOURCE,$source) or die;
   while(<SOURCE>)
   {
      $fair = $fair || /$protected/;
   }
   close(SOURCE) or die;
   return $fair;
}

sub TestInclude
{
   my($source)=shift(@_);
   my($include)=shift(@_);
   return if !ValidateInclude $source,$include;
   my($scratchname) = RemoveInclude $source,$include;
   if( CompileFile($scratchname,0) == 0 )
   {
      my ($conditional) = IsConditional($source,$include);
      warn " $include - conditional\n" if $conditional;
      my ($protected) = IsFairlyForProtected($source,$include);
      warn " $include - protected\n" if $protected;
      warn " $include\n" if !$conditional && !$protected;
   }
   else { warn " $include - essential\n"; }
   unlink $scratchname or die;
}

sub AllFiles
{
   return if ! /.cpp$/;
   my($source)=$_;
   CheckFile($source);
}

sub SortFiles
{
   my(@sorted)=sort { $a cmp $b } @_; # Swap $a and $b to reverse order
   return @sorted;
}

sub WrapInclude
{
   my ($include) = shift(@_);
   my ($scratch,$scratchname) = tempfile();
   print $scratch "#include \"Mpch.h\"\n";
   print $scratch "#include \"Mcommon.h\"\n";
   my ($dir) = "";
   if( !( $include =~ m#^/# ) )
   {
      my ($basedir) = $File::Find::dir;
      $basedir = dirname($ARGV[1]) if $#ARGV >= 1;
      $dir = "$2/" if $basedir =~ m#^(\./)?include/(.*)#;
   }
   print $scratch "#include \"$dir$include\"\n";
   close($scratch) or die;
   return $scratchname;
}

sub CompileInclude
{
   my ($include) = shift(@_);
   my ($verbose) = shift(@_);
   local $header_mode = '.cpp';
   my ($scratchname) = WrapInclude($include);
   my ($result) = CompileFile($scratchname,$verbose);
   unlink $scratchname or die;
   return $result;
}

sub AllIncludes
{
   return if ! /\.h$/;
   return if $File::Find::dir =~ m#/extra/#;
   my($include)=$_;
   CheckFile($include);
}

sub CheckFile
{
   my ($file) = shift(@_);
   warn "$File::Find::dir $file\n";
   if( !$fast )
   {
      my ($status) = CompileFile($file,1);
      return if $status != 0;
   }
   my (@includes) = FindIncludes $file;
   if( !$fast )
   {
      foreach $include (@includes)
      {
         TestInclude $file,$include;
      }
   }
}

sub CollectExtensionFiles
{
   return if substr($_,-length($extension)) ne $extension;
   push @cppfiles,$File::Find::name;
}

sub FindExtensionFiles
{
   local $extension = shift(@_);
   local @cppfiles;
   find { preprocess => \&SortFiles,
      wanted => \&CollectExtensionFiles }, '.';
   return @cppfiles;
}

sub IncludePath
{
   my ($parent) = shift(@_);
   my ($file) = shift(@_);
   foreach(@includepath)
   {
      return "$_/$file" if -e "$_/$file";
   }
   return dirname($parent)."/$file";
}

sub IgnoreInclude
{
   my ($file) = shift(@_);
   #return 1 if $file eq 'stdarg.h'; # Virtual file
   return 1 if $file =~ m#^/usr/#; # Don't look at system files at all
   return 1 if $file =~ m#wx/\w+/#; # We don't have MSW headers on Linux
   #return 1 if $file =~ m#windows.h#; # No such file on Linux
   #return 1 if $file =~ m#wx/listbook.h#; # wxWindows 2.5
   #return 1 if # C++ headers are placed randomely
#      $file eq 'string' || $file eq 'iostream.h' || $file eq 'fstream.h'
#      || $file eq 'iostream' || $file eq 'fstream';
   return 1 if $file =~ m#shortsym.h#; # C-client mess
#   return 1 if $file =~ m#/wx/#; # Our wxWindows extensions
   return 1 if $file =~ m#\.\./classes/MObject.cpp#; # Filters.cpp TEST hack
   return 1 if $file =~ m#libmal.h#; # PalmOS
   return 1 if $file =~ m#wxllist.h#; # More wxWindows extensions
   return 1 if $file =~ m#wxlparser.h#; # More wxWindows extensions
   return 1 if $file =~ m#wxlwindow.h#; # More wxWindows extensions
   return 1 if $file =~ m#Mpch.h#; # Don't count precompiled headers
   return 0;
}

sub FindIncludesRecursivelyHelper
{
   my ($file) = shift(@_);
   local *SOURCE;

   return if IgnoreInclude($file);
   open(SOURCE,$file) or die $file;
   while(<SOURCE>)
   {
      next if ! /^ *# *include *[<"]([^"]*)[">]/; # Count system includes too
      my ($include) = $1;
      if( $dependencies{$include} != 1 )
      {
         $dependencies{$include} = 1;
         FindIncludesRecursivelyHelper(IncludePath($file,$include))
            if /^ *# *include *"([^"]*)"/; # Don't follow system includes
      }
   }
   close(SOURCE) or die;
}

sub FindIncludesRecursively
{
   my ($file) = shift(@_);
   local %dependencies;
   FindIncludesRecursivelyHelper($file);
   return keys %dependencies;
}

sub StatisticsForExtension
{
   my ($extension) = shift(@_);
   my (@filelist) = FindExtensionFiles($extension);
   my (%depcount);
   foreach my $file (@filelist)
   {
      $depcount{$file} = scalar(FindIncludesRecursively($file));
   }
   @filelist = sort { $depcount{$a} <=> $depcount{$b} } @filelist;
   foreach my $file (@filelist)
   {
      printf "%3d $file\n",$depcount{$file};
   }
}

sub InvertedDependencies
{
   my (@filelist) = FindExtensionFiles(".cpp");
   my (%eachinclude);
   foreach my $file (@filelist)
   {
      foreach my $include (FindIncludesRecursively($file))
      {
         $eachinclude{$include}{$file} = 1;
      }
   }
   my (%depcount);
   $depcount{$_} = scalar(keys %{$eachinclude{$_}})
      foreach (keys %eachinclude);
   @sortedlist = sort { $depcount{$a} <=> $depcount{$b} } keys %eachinclude;
   foreach my $include (@sortedlist)
   {
      printf "%3d $include\n",$depcount{$include};
   }
}

sub CalculateStatistics
{
   print "Recursive include counts for sources\n";
   StatisticsForExtension(".cpp");
   print "Recursive include counts for headers\n";
   StatisticsForExtension(".h");
   print "Recursive usage counts for headers\n";
   InvertedDependencies();
   if( -e $ARGV[1] )
   {
      print "One file dependencies\n";
      print join("\n",FindIncludesRecursively($ARGV[1])),"\n";
   }
}
