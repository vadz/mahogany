File: readme_win.txt, the README file for MS Windows version
Date: 16.12.01
Version: the information in this file applies to alpha 0.64

0. Requirements
--------------

 a) Win95/98/NT 4.0/2000/XP. Mahogany has been mostly tested under NT 4.0, but
    it is also known to work under Windows 95/98, 2000 and XP. It does not run
    under Windows 3.1.

 b) You need a POP3 or IMAP4 (recommended, especially for slow connection!)
    server to read e-mail and an SMTP server to send it. Reading and sending
    USENET news is also supported (although this alpha version doesn't have
    the "advanced" newsreader features) and you will need a NNTP server if you
    want to try it out.

 c) If you plan to use Python scripting with Mahogany, you need to have Python
    1.5, 2.0 (preferred) or 2.1 installed on your system, please refer to
    http://www.python.org/ for details. Note that in this case you will have
    to rebuild the program with Python support as the official binaries for
    this version don't include it.

1. Installation
---------------

 a) the preferred installation method is to use the provided installation
    program which will guide you through the installation process and will
    allow you to uninstall the program automatically later (by going into the
    "Add/Remove programs" applet in the Control panel).

    Uninstallation note: all the mail boxes files created by the program will
    be left even if you uninstall it. As by default they're created under the
    installation directory, at least the SentMail and NewMail folders will be
    left there and so it will not be deleted during uninstallation. Please
    delete these files manually if you really don't want to keep them.

 b) you can also install Mahogany manually. This involves copying the main
    executable M.exe to any directory on your hard drive. The registry entries
    necessary for the program will be created automatically under the key
    HKCU\Software\wxWindows\M, so they will have to be deleted manually if you
    later want to uninstall the program.

 c) upgrading: don't uninstall Mahogany 0.63 if you want to keep your existing
    settings. You may install 0.64 either into the same directory as 0.63 or
    in another one (in which case you should move your existing mailbox files
    to the new directory before running the program if you wish to continue
    using them)

2. Miscellaneous remarks
------------------------

 a) locale support: like the Unix version, Mahogany for Windows supports many
    different languages (please see our WWW page for the up-to-date list - new
    translations are added quite often). However, they're not included in this
    distribution and you'll have to fetch them yourself from the WWW page (see
    the address above) and put them into the locale directory to make it work.

 b) documentation: sorry, this preliminary Windows version comes without
    context-sensitive help. The help file is included in the distribution
    though - it's a translation to Windows help format of Mahogany HTML
    documentation which may always be found at

       http://mahogany.sourceforge.net/doc/Manual/Manual.html

    The context-sensitive help will work in the future versions.

    Generally speaking, documentation is far from complete, so if you have any
    questions please feel free to ask them on the Mahogany mailing lists (see
    the help file for details on how subscribe to/access them)

3. Differences from the Unix version
------------------------------------

 The features enumerated below work in the Unix version, but are not yet
implemented under Windows:

 a) no Palm support (synchronization with hand held devices)
 b) no support for local MTA, mail can be sent only using SMTP

 Note that encrypted communications using SSL are now supported in Windows
version provided you didn't unselect "SSL Support" item during installation

