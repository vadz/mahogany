File: readme_win.txt, the README file for MS Windows version
Date: 20.06.99
Version: the information in this file applies to alpha 0.23

0. Requirments
--------------

 a) Win95/98/NT 4.0. Mahogany has been tested the most under NT 4.0, but it
    also works (with very minor shortcomings) under Windows 9x.

 b) You need a POP3 or IMAP4 (recommended, especially for slow connection!)
    server to read e-mail and an SMTP server to send it. Reading and sending
    USENET news is also supported (although this alpha version doesn't have
    the "advanced" newsreader features).

1. Installation
---------------

 a) the preferred installation method is to use the provided setup.exe program
    which will guide you through the installation process and will allow you to
    uninstall the program automatically later (by going into the "Add/Remove
    programs" applet in the Control panel).

    Uninstallation note: all the mail boxes files created by the program will
    be left even if you uninstall it. This means that at least the SentMail
    and NewMail folders will be left in the installation directory and so it
    will not be deleted during uninstallation. Please delete these files
    manually if you really don't want to keep them.

 b) you can also install Mahogany manually. This involves copying the main
    executable M.exe to any directory on your hard drive. The registry entries
    necessary for the program will be created automatically under the key
    HKCU\Software\wxWindows\M, so they will have to be deleted manually if you
    later want to uninstall the program.

 c) installation over previous versions: if you're one of courageous people
    who have tried pre-alpha 0.10, please delete/uninstall it before
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
       http://www.phy.hw.ac.uk/~karsten/Mahogany/doc/Manual/index.html.

    The future versions of the program will include help in MS HTML format.

 c) working off-line: there are no special support for off-line mode in
    Mahogany yet, but you may use it as off-line reader too.

3. Differences from the Unix version
------------------------------------

 All of features enumerated below already work in the Unix version, but are not
yet implemented under Windows.

 a) there is no XFace support yet.

 b) this release comes without Python support due to lack of its usefullness
    so far.

 c) there is unfortunately no context-sensitive help yet.

4. Items from our TODO list
---------------------------

 In addition to the common TODO list which may be consulted at Mahogany Web
page, the following items are scheduled for implementation under Windows in
the near future:

 a) import of "foreign" address books from Eudora, MS Outlook and The Bat!
    programs.

 b) import of folder information from the same programs.

 c) converting of the online HTML help into Microsoft HTML help format.

 d) better support for working off-line
