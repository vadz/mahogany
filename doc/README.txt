

M - README

Karsten Ballüder \\(Ballueder@usa.net)\\Contact us: m-developers@makelistcom

Abstract

These are the release notes for M. Some more detailed information on
how to compile, install und use it are in the Information files.

Abstract

This information relates to the first public alpha release, 17. August
1998.

1 WARNING - this is (pre-?)alpha

When you do a large free software project, you have two choices: either
continue hacking without releases and risk to never get it finished,
or set yourself a release date, no matter what the situation is. As
we got several requests from people who want to play with it, we decided
on a release date and did our best to get it into a working shape.
BUT THIS SOFTWARE IS STILL UNDER DEVELOPMENT! This means

* it is incomplete and awkward to use

* it may crash occasionally or often or be completely unusable

M is not ready for the end user yet, but we present it in its current
state to give you an impression of what it is going to be. We also
hope to attract a bit of attention and maybe even some outside help
for it.

2 Which features are implemented?

Quite some already, but more are still missing. What we have so far:

* Cross-platform. M compiles on a variety of Unix systems and on Microsoft
  Windows. Use one mail client, no matter what system you use. The
  source and binary for Windows 95/98/NT are available on request
  (m-developers@makelist.com). Mailbox file formats are the same on
  both platforms.

* Based on the c-client library from the University of Washington,
  therefore full access to a wide range of protocols and file formats,
  including SMTP, MAP, POP3, NNTP and several mailbox formats.

* Wide (extreme?) user configurability. Whatever makes sense to override
  or change, can be changed by the user. Configuration supports several
  configuration files on Unix, with special administrator support
  for making entries immutable, and the registry on Windows.

* Scriptable and extendable. M includes an embedded Python interpreter
  with full access to its object hierarchy. Write object-oriented
  scripts to extend and control M.

* Easy MIME support. Text and other content can be freely mixed and
  different filetypes are represented by icons.

* Inline displaying of images, clickable URLs, XFace support.

* Multiple mail folders.

* Powerful address database and contact manager 

* Printing of nicely formatted messages.

* Full internationalisation support, M speaks multiple languages, but
  no translations yet.

3 Known bugs

* selection in the folder view sometimes behaves strange, selecting
  all messages doesn't work

* message and composition view don't automatically scroll to the cursor

* tab traversal in dialogs doesn't work (wxGTK problem)

4 TODO, features to implement

This is a list of features on our TODO list that we are currently working
on. Before adding new features, we'll clean up a few things:

* First comes a rewrite of the class hierarchy. For better modularisation
  and CORBA support (Python will profit from this, too.), we will
  clean up header files and remove some interdependencies. GUI and
  non-GUI code will be better separated, class implementations and
  interface definitions will be sorted out more clearly. This includes
  a common base object with reference couting.

* Plug the (very few remaining) memory holes.

Then we fix some GUI issues. Many of these depend on wxGTK which is
still evolving speedily. 

* add keyboard accelerators and proper tab traversal

* add a context sensitive help system

* add more dialogs and a tree control for folder selection

After that we reach the list of serious improvements:

* Better Python support. We have some callbacks in place, but after
  the class hierarchy rewrite we have to generate new interface files
  for the complete class hierarchy. Also by this time wxPython might
  be integrated, so we can actually write some of the configuration
  dialogs in python which should speed things up. Help welcome.

* Full Drag and Drop interaction with filemanagers of Windows and Gnome
  (will be added real soon, easy).

* Easy to use filtering system for mails.

* Support for V-cards.

* Nested mail folder hierarchy.

* Spam-Ex spam fighting/auto-complaint function.

* Richt-text editing and HTML mail support

* Support for PGP and GNU Privacy Guard to encrypt mails.

* Threading of messages and proper usenet news support.

* Compression of mail folders.

* Delay-Folder to keep mails and re-present them at a later date.

* Context sensitive help system (HTML based).

* Translations to German, French and Italian.

* Wide character (Unicode) support and other character sets.

* Import, export and synchronisation with other programs' address databases.

* Voice mail.

* More Python support through wxPython.

* Support for Drag and Drop interaction with KDE, once that wxQt is
  available.

* CORBA support, possible cooperation with PINN project.

* ANY OTHER SUGGESTION

4.1 Help Needed

As you can see, we have big plans for M. To achieve all this, we need
some help. Areas where we would use some help are

* Python 

* support for further mail protocols, LDAP

* The wxQt project, a port of wxWindows to the Qt toolkit, will also
  be happy for any help. We are not directly involved in this, but
  being involved with wxWindows, we are happy to support that port.

* If you have access to other systems apart from Linux/Solaris/Windows,
  you are very welcome to help us port M to those platforms, or to
  other hardware than Intel.

5 Online resources

* M has a homepage at http://Ballueder.home.ml.org/M/ 

* The wxWindows homepage is http://web.ukonline.co.uk/julian.smart/

* wxGTK, the GTK port of wxWindows, is available from http://www.freiburg.linux.de/~wxxt/

6 FAQ

There will be some after this release - surely.
