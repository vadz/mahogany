

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

* it should be as powerful and extensible as possible, being able to
  replace awkward tools such as procmail, thus it must have a built
  in scripting language 

* it should have full support for a wide range of standards, including
  full and intuitive MIME support 

* it should interact with other programs intuitively, sending MIME
  mails should be as easy as dropping files from a file manager in
  the message window

* it should run on a wide variety of systems 

2 Compilation notes

2.1 Operating systems specific

2.1.1 Linux

If compiling with a non-default compiler like egcs, make sure that
/usr/include is not in the include path, neither should /usr/lib be
explicitly listed. M has been compiled with egcs and gcc-2.8.x on
both, libc5 and glibc2 systems.

2.1.2 Solaris/SunOS

M has been successfully compiled with gcc-2.8.0 on Solaris. Currently
it does not compile with the standard C++ compiler.

2.1.3 Microsoft Windows 

M can be compiled under Windows, using wxWindows Version 2.0 and Microsoft
Visual C++.

2.2 Other issues/libraries

2.2.1 C-client library

A copy of the c-client library is required and is included with the
M sources. It is available separately from ftp://ftp.cac.washington.edu/imap/imap.tar.Z.
Before compiling it with M, you need to patch it. 

The following information only applies if you use a separate c-client
library source:

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

2.2.2 Python 

configure looks for Python in /usr/local/src/Python-1.5. If your Python
is installed in a different location, change the variable PYTHON_PATH
at the beginning of configure. 

2.2.3 XFaces 

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

5 Scripting and Python integration

5.1 Introduction

M uses Python as an embedded scripting language. A large number of
user definable callback functions are available. Scripts have access
most objects living in M.

5.2 Initialisation 

At startup, M will load a file called Minit.py and call the Minit()
function defined in there, without any arguments.

5.3 Callback Functions (Hooks)

There are a large number of callbacks available which will be called
from different places withing M. These are documented in Mcallbacks.h.
All of these callbacks are called with at least two arguments:

1. The name of the hook for which the function got called, e.g. FolderOpenHook

2. A pointer to the object from which it was called. E.g. for FolderOpenHook,
  this would be a pointer to a MailFolder object. This object does
  not carry a useable type with it and needs to be converted in the
  callback, e.g. if the argument is called arg and the object is a
  MailFolder, the object must either be used as MailFolder.MailFolder(arg)
  or be converted as mf = MailFolder.MailFolder(arg). 

3. Some callbacks have a third argument. This is either a single value
  or a tuple holding several values.

5.4 Namespaces

To avoid repeatedly typing in the name of the module (MailFolder in
this case), it can be imported into the global namespace with ``from~MailFolder~import~*''.
By default modules are not imported into the global namespace and
must be explicitly named.

5.5 List of Callbacks

+-----------------------+-------------+----------------------------+---------------------------------------+--------------------------------------------+
|Callback Name          | Object Type | Additional Arguments/Types | Return Value                          | Documentaion                               |
+-----------------------+-------------+----------------------------+---------------------------------------+--------------------------------------------+
+-----------------------+-------------+----------------------------+---------------------------------------+--------------------------------------------+
|FolderOpenHook         | MailFolder  |                            | void                                  | Called after a folder has been opened.     |
+-----------------------+-------------+----------------------------+---------------------------------------+--------------------------------------------+
|FolderUpdateHook       | MailFolder  |                            | void                                  | Called after a folder has been updated.    |
+-----------------------+-------------+----------------------------+---------------------------------------+--------------------------------------------+
|FolderSetMessageFlag   | MailFolder  | (long) index of message    | 1 if changing flags is ok,0 otherwise | Called before changing flags for a mesage. |
|                       |             | (string)name of flag       |                                       |                                            |
+-----------------------+-------------+----------------------------+---------------------------------------+--------------------------------------------+
|FolderClearMessageFlag | MailFolder  | (long) index of message    | 1 if changing flags is ok,0 otherwise | Called before changing flags for a mesage. |
|                       |             | (string) name of flag      |                                       |                                            |
+-----------------------+-------------+----------------------------+---------------------------------------+--------------------------------------------+
|FolderExpungeHook      | MailFolder  |                            | 1 to expunge, 0 to abort              | Called before expunging messages.          |
+-----------------------+-------------+----------------------------+---------------------------------------+--------------------------------------------+


6 Further Information

* You can download the latest version of M from http://Ballueder.home.ml.org/M/ 

* You can also get up-to-date information on M from the M WWW Page:
  http://Ballueder.home.ml.org/M/

* wxWindows is available fromhttp://web.ukonline.co.uk/julian.smart/

* The GTK port of wxWindows, wxGTK, is available from: http://www.freiburg.linux.de/~wxxt/

7 FAQ
