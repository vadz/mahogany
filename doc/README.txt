

M - Release Notes 

Karsten Ballüder (Ballueder@usa.net, http://Ballueder.home.ml.org)

Abstract

This document is a set of notes about how to compile and install M.
It also covers some frequently asked questions. 

\tableofcontents

1 Introduction 

M is intended to be a powerful mail and news client. The following
points were significant in the development of it 

* it should have an easy to use graphical user interface 

* it should be powerful, being able to replace awkward tools such as
  procmail, thus it must have a built in scripting language 

* it should have full support for a wide range of standards, including
  full MIME support 

* it should run on a wide variety of systems </enum>

2 Compilation notes

2.1 Operating systems specific

2.1.1 Linux

If compiling with a non-default compiler like egcs, make sure that
/usr/include is not in the include path, neither should /usr/lib be
explicitly listed.

2.1.2 Solaris/SunOS

M has been successfully compiled with gcc-2.8.0 on Solaris. Currently
it does not compile with the standard C++ compiler.

2.1.3 Microsoft Windows 

M can be compiled under Windows, using wxWindows Version 2.0 and Microsoft
Visual C++.

FIXME: VADIM?

2.2 Other issues/libraries

2.2.1 STL 

M uses the STL, so you need a sufficiently recent compiler, i.e. egcs
or gcc-2.8.

2.2.2 C-client library

The c-client library is required and is available from ftp://ftp.cac.washington.edu/imap/imap.tar.Z.
Before compiling it with M, you need to patch it. 

Unpack the archive in the main M directory, then change into the IMAP
directory and try a first make, i.e. a make linux or make gso. This
will create a source directory c-client with lots of links to other
source files in it. 

Then install the rfc822.c.patch on c-client's rfc822.c file from the
extra/patches subdirectory of M and run the c-client++ script from
the extra/scripts subdirectory in the c-client source directory, which
will rename variables in the c-client code to make it C++ compliant.
If there is any problem, it helps to edit the CCTYPE or CFLAGS files.
After creation of the library c-client.a, all object files can be
deleted.

2.2.3 Python 

configure looks for Python in /usr/local/src/Python-1.5. If your Python
is installed in a different location, change the variable PYTHON_PATH
at the beginning of configure. 

2.2.4 XFaces 

If you have the compface library and header file installed, it will
be used to support XFaces. To install it, unpack it under the main
M directory and apply the patch compface.patch from the extra/patches
directory to its sources. Compile it and link its header and library
to the extra/include and extra/lib directories.

3 Installation

3.1 Configuration and Compilation

Follow these steps: 

1. Edit configure so it will find your installation of Python. If you
  do not have it, just skip this step.

2. Run configure to create the include/config.h and makeopts file. It
  may be required to edit makeopts by hand.

3. Run make dep and cd src and make to compile and link M. Compiling
  some of the source files will take an enormous amount of memory,
  so make sure you have enough virtual memory. 

4 Configuration and Testing

4.1 Configuration settings

Under Unix all configuration settings are stored in ~/.M/config under
Windows in the registry. To get an overview over all possible configuration
options and their default values, set the value RecordDefaults=1.
Under Unix, do this by creating a new ~/.M/config file containig the
lines





After running M, this file will then contain all default settings.
Most of them are easily understood. Otherwise, the file include/Mdefaults.h
contains them all with some short comments.

4.2 Reading mail/news

To read news, you need to open a folder. The default incoming mail
folder has the name INBOX. Any other name will be interpreted as a
filename relative to the folder directory. 

4.3 Writing mail/news

Before being able to send mail, you need to configure the MailHost
setting to tell it where to send the mail. 

5 Further Information

* You can download the latest version of M from http://Ballueder.home.ml.org/M/ 

* You can also get up-to-date information on M from the M WWW Page:
  http://Ballueder.home.ml.org/M/

* wxWindows is available fromhttp://web.ukonline.co.uk/julian.smart/

6 FAQ
