File: readme_win.txt, the README file for MS Windows version
Date: 10.03.01
Version: the information in this file applies to alpha 0.62

0. Requirments
--------------

 a) Win95/98/NT 4.0. Mahogany has been tested the most under NT 4.0, but it
    also works (with very minor shortcomings) under Windows 9x.

 b) You need a POP3 or IMAP4 (recommended, especially for slow connection!)
    server to read e-mail and an SMTP server to send it. Reading and sending
    USENET news is also supported (although this alpha version doesn't have
    the "advanced" newsreader features) and you will need a NNTP server if you
    want to try it out.

 c) If you plan to use Python scripting with Mahogany, you need to have Python
    2.0 installed on your system, please refer to http://www.python.org/ for
    details.

1. Installation
---------------

 a) the preferred installation method is to use the provided setup.exe program
    which will guide you through the installation process and will allow you to
    uninstall the program automatically later (by going into the "Add/Remove
    programs" applet in the Control panel).

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

 c) installation over previous versions: if you're one of courageous people
    who have tried pre-alpha 0.23, please delete/uninstall it before
    installing this version. The registry settings will be updated
    automatically and so may be left, but you should not try to install the
    program in the same directory.

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

 All of features enumerated below work in the Unix version, but are not
yet implemented under Windows:

 a) there is no XFace support yet

 b) Windows version doesn't support encrypted communications using SSL

 c) no Palm support (synchronization with hand held devices)
