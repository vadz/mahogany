File: readme_win.txt, the README file for MS Windows version
Date: 21.08.98
Version: the information in this file applies to pre-alpha 0.01

             *********************************************************
             *** please see file README.txt for main documentation ***
             *********************************************************

0. Requirments
--------------

 a) Win95/98/NT 4.0. At this moment M hadn't been yet tested with Windows 98, please
    write us if you experience some specific problems with this version of Windows
    (installation or otherwise).

    M hasn't been tested with neither Win32s nor Windows NT 3.51 and earlier, but
    probably could be made to work with them. However, future plans do not include
    supporting these platforms and future versions will make usage of 95/NT 4.0
    specific features.

 b) POP3/IMAP server to read mail from (you might also use local files in Unix
    mbox format as well as several others, but it's not very exciting, is it?).
    NNTP servers are also supported, but M is not really a news reader (yet)

1. Installation
---------------

 a) the preferred installation method is to use the provided setup.exe program
    which will guide you through the installation process and will allow you to
    uninstall the program automatically later.

 b) you can also install M manually. This involves copying M files to any directory
    on your hard drive. The registry entries necessary for the program will be
    created automatically under the key HKCU\Software\wxWindows\M.

 c) python usage: if you already have python15.dll installed on your system, you
    can use it instead of provided one saving something lke 450Kb of disk space.

2. Differences from the Unix version
------------------------------------

 All of features enumerated below already work in the Unix version, but are not
yet implemented under Windows.

 a) handling of MIME attachements is (more) buggy and the MIME dialog is truly ugly

 b) there is no XFace support yet

 c) Python support is flaky, especially under 95. For NT, it works ok for me, but with
    95 there are some problems.