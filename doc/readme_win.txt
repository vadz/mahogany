File: readme_win.txt, the README file for MS Windows version
Date: August 20, 2012
Version: the information in this file applies to version 0.68

0. Requirements
--------------

 a) Windows XP or later. Mahogany doesn't run under previous Windows
    versions.

 b) You need a POP3 or IMAP4 (recommended, especially for slow
    connection!) server to read e-mail and an SMTP server to send
    it. Reading and sending USENET news is also supported (although
    barely usable in this version) and you will need a NNTP server
    if you want to try it out.

 c) If you plan to use Python scripting with Mahogany, you need to
    have Python installed on your system. All Python 2 versions are
    normally supported but 2.6 or 2.7 is recommended. Please get it from
    http://www.python.org/.

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
    be left even if you uninstall it. As under Windows 9x they're
    created under the installation directory, at least the SentMail
    and NewMail folders will be left there and so it will not be
    deleted during uninstallation. Please delete these files manually
    if you really don't want to keep them.

    Also note that the program options are left in the registry after
    uninstallation, if you want to get rid of them as well please
    remove the HKCU\Software\Mahogany-Team\M key (with all its
    subkeys) manually.

 b) to uninstall Mahogany manually, delete the directory where you
    installed it and the HKCU\Software\Mahogany-Team\M. Mahogany
    doesn't install any files in the Windows system directory.

