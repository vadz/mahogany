File: readme_win.txt, the README file for MS Windows version
Date: April 8, 2003
Version: the information in this file applies to version 0.65

0. Requirements
--------------

 a) Any Win32 OS: Mahogany should work on all Win32 systems, i.e.
    Windows 95/98/ME/NT 4/2000/XP but SSL/TLS might be not supoprted
    on older systems without Internet Explorer. Mahogany does not
    run under Windows 3.1.

 b) You need a POP3 or IMAP4 (recommended, especially for slow
    connection!) server to read e-mail and an SMTP server to send
    it. Reading and sending USENET news is also supported (although
    barely usable in this version) and you will need a NNTP server
    if you want to try it out.

 c) If you plan to use Python scripting with Mahogany, you need to
    have Python 1.5, 2.0 or 2.1 installed on your system (2.2 is not
    supported yet), please refer to http://www.python.org/ for
    details. Note that in this case you will have to rebuild the
    program with Python support as the official binaries for this
    version don't include it.

1. Installation
---------------

 a) the preferred installation method is to use the provided
    installation program which will guide you through the
    installation process and will allow you to uninstall the program
    automatically later (by going into the "Add/Remove programs"
    applet in the Control panel).

 b) you can also install Mahogany manually. This involves copying
    the main executable M.exe to any directory on your hard drive.
    The registry entries necessary for the program will be created
    automatically under the key HKCU\Software\Mahogany-Team\M, so
    they will have to be deleted manually if you later want to
    uninstall the program.

 c) upgrading: don't uninstall the previous Mahogany version if you
    want to keep your existing settings. You may install the new
    version either into the same directory as the old one or in
    another one (in which case you should move your existing mailbox
    files to the new directory before running the program if you
    wish to continue using them)

2. Uninstallation
-----------------

 a) if you have installed Mahogany using the provided installer you
    can uninstall it either using the corresponding item in the
    "Start" menu or, for the systems supporting it, from the Control
    Panel applet as usual.

    Note that all the mail boxes files created by the program will
    be left even if you uninstall it. As by default they're created
    under the installation directory, at least the SentMail and
    NewMail folders will be left there and so it will not be deleted
    during uninstallation. Please delete these files manually if you
    really don't want to keep them.

 b) to uninstall Mahogany manually, delete the directory where you
    installed it and the HKCU\Software\Mahogany-Team\M. Mahogany
    doesn't install any files in the Windows system directory.

