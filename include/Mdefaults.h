/*-*- c++ -*-********************************************************
 * Mdefaults.h : all default defines for readEntry() config entries *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/



#ifndef MDEFAULTS_H
#define   MDEFAULTS_H

#include "Mcallbacks.h"
#include "Mversion.h"

/** @name The sections of the configuration file. */
//@{

// The trailing slashes in the following defines are important,
// otherwise Profile.cpp will get broken!

/** The section in the global configuration file used for storing
    profiles (trailing '/' required).
*/
#ifndef M_PROFILE_CONFIG_SECTION
#  ifdef OS_WIN
#     define   M_PROFILE_CONFIG_SECTION   "/Profiles/"
#  else  // Unix
#     define   M_PROFILE_CONFIG_SECTION   "/M/Profiles/"
#  endif // Unix/Win
#endif

/** The section in the global configuration file used for storing
    folder profiles (trailing '/' required).
*/
#ifndef M_FOLDER_CONFIG_SECTION
#  define   M_FOLDER_CONFIG_SECTION   M_PROFILE_CONFIG_SECTION "Folders/"
#endif

/** The section in the global configuration file used for storing
    window positions (trailing '/' required).
*/
#ifndef M_FRAMES_CONFIG_SECTION
#  ifdef OS_WIN
#  define   M_FRAMES_CONFIG_SECTION       "/Frames/"
#  else  // Unix
#     define   M_FRAMES_CONFIG_SECTION    "/M/Frames/"
#  endif // Unix/Win
#endif
/** The section in the global configuration file used for storing
    control settings.
*/
#ifndef M_SETTINGS_CONFIG_SECTION
#  ifdef OS_WIN
#  define   M_SETTINGS_CONFIG_SECTION       "/Settings/"
#  else  // Unix
#     define   M_SETTINGS_CONFIG_SECTION    "/Settings/"
#  endif // Unix/Win
#endif
//@}

/** @name Levels of  interaction, do something or not? */
//@{
/// never do this action
#define   M_ACTION_NEVER   0
/// ask user if he wants it
#define   M_ACTION_PROMPT   1
/// don't ask user, do it
#define   M_ACTION_ALWAYS   2
//@}

/**
 NOTICE: the distinction between MP_xxx and MP_xxx config entries no
 longer exists. They are now treated identically.
**/
/** @name built-in icon names */
//@{
/// for hyperlinks/http
#define   M_ICON_HLINK_HTTP   "M-HTTPLINK"
/// for hyperlinks/ftp
#define   M_ICON_HLINK_FTP   "M-FTPLINK"
/// unknown icon
#define   M_ICON_UNKNOWN      "UNKNOWN"
//@}
/** @name names of configuration entries */
//@{
/// our version
#define   MP_VERSION          "Version"
/// are we running for the first time?
#define   MP_FIRSTRUN         "FirstRun"
/// shall we record default values in configuration files
#define   MP_RECORDDEFAULTS      "RecordDefaults"
/// default position x
#define   MP_XPOS            "XPos"
/// default position y
#define   MP_YPOS            "YPos"
/// window width
#define   MP_WIDTH         "Width"
/// window height
#define   MP_HEIGHT         "Height"

/// show log window?
#define   MP_SHOWLOG          "ShowLog"

/// expand folder tree control?
#define   MP_EXPAND_TREECTRL   "ExpandTreeControl"
/// help browser name
#define   MP_HELPBROWSER   "HelpBrowser"
/// is help browser of netscape type?
#define   MP_HELPBROWSER_ISNS   "HelpBrowserIsNetscape"
/// the directory for mbox folders
#define   MP_MBOXDIR         "FolderDir"

/// the user's M directory
#   define   MP_USERDIR         "UserDirectory"

// Unix-only entries
#ifdef OS_UNIX
/// search paths for M's directory
#   define   MP_PATHLIST         "PathList"
/// path to M directory
#   define   MP_ROOTPATH   "GlobalDirectory"
/// the name of M's root directory
#   define   MP_ROOTDIRNAME         "RootDirectoryName"
/// the name of the M directory
#   define   MP_USER_MDIR         "MDirName"
/// the path where to find .afm files
#   define   MP_AFMPATH         "AfmPath"
/// the path to the /etc directories (configuration files)
#   define   MP_ETCPATH         "ConfigPath"
/// the path to the M directory, e.g. /usr/
#   define   MP_PREFIXPATH         "PrefixPath"
#endif //Unix

/// the locale for translation to national languages
#define   MP_LOCALE               "Locale"
/// the default icon for frames
#define   MP_ICON_MFRAME         "MFrameIcon"
/// the icon for the main frame
#define   MP_ICON_MAINFRAME      "MainFrameIcon"
/// the icon directory
#define   MP_ICONPATH         "IconDirectory"
/// the path for finding profiles
#define   MP_PROFILE_PATH         "ProfilePath"
/// the extension to use for profile files
#define   MP_PROFILE_EXTENSION      "ProfileExtension"
/// the name of the mailcap file
#define   MP_MAILCAP         "MailCap"
/// the name of the mime types file
#define   MP_MIMETYPES         "MimeTypes"
/// the printf() format for dates
#define   MP_DATE_FMT         "DateFormat"
/// show console window
#define   MP_SHOWCONSOLE      "ShowConsole"
/// name of address database
#define   MP_ADBFILE         "AddressBook"
/// names of folders to open at startup (semicolon separated list)
#define   MP_OPENFOLDERS         "OpenFolders"
/// name of folder to open in mainframe
#define   MP_MAINFOLDER          "MainFolder"
/// path for Python
#define   MP_PYTHONPATH        "PythonPath"
/// is Python enabled (this is a run-time option)?
#define   MP_USEPYTHON         "UsePython"
/// start-up script to run
#define   MP_STARTUPSCRIPT     "StartupScript"
/// show splash screen on startup?
#define   MP_SHOWSPLASH        "ShowSplash"
/// how long should splash screen stay (0 disables timeout)?
#define   MP_SPLASHDELAY       "SplashDelay"
/// how often should we autosave the profile settings (0 to disable)?
#define   MP_AUTOSAVEDELAY       "AutoSaveDelay"
/// ask user if he really wants to exit?
#define   MP_CONFIRMEXIT       "ConfirmExit"
/// open folders when they're clicked (otherwise - double clicked)
#define   MP_OPEN_ON_CLICK     "ClickToOpen"
/// create .profile files?
#define   MP_CREATE_PROFILES   "CreateProfileFiles"
/// umask setting for normal files
#define   MP_UMASK               "Umask"
/**@name Printer settings */
//@{
/// Command
#define MP_PRINT_COMMAND   "PrintCommand"
/// Orientation
#define MP_PRINT_ORIENTATION "PrintOrientation"
/// Scaling
#define MP_PRINT_SCALING   "PrintScaling"
/// Preview command
#define MP_PRINT_PREVIEWCMD   "PrintPreviewCommand"
/// Printout file
#define MP_PRINT_FILE         "PrintFile"
/// Translation X
#define MP_PRINT_TRANS_X      "PrintTranslationX"
/// Translation Y
#define MP_PRINT_TRANS_Y      "PrintTranslationY"
/// FIXME What about paper format?
//@}
/**@name for BBDB address book support */
//@{
/// generate unique names
#define   MP_BBDB_GENERATEUNIQUENAMES   "BbdbGenerateUniqueNames"
/// ignore entries without names
#define   MP_BBDB_IGNOREANONYMOUS      "BbdbIgnoreAnonymous"
/// name for anonymous entries, when neither first nor family name are set
#define   MP_BBDB_ANONYMOUS         "BbdbAnonymousName"
/// save on exit, 0=no, 1=ask, 2=always
#define   MP_BBDB_SAVEONEXIT  "BbdbSaveOnExit"
//@}
/**@name For Profiles: */
//@{
/// The Profile Type.
#define   MP_PROFILE_TYPE      "ProfileType"
/// the user's full name
#define   MP_PERSONALNAME         "PersonalName"
/// the username/login
#define   MP_USERNAME         "UserName"
/// the user's hostname
#define   MP_HOSTNAME         "HostName"
/// the username for returned mail
#define   MP_RETURN_ADDRESS      "ReturnAddress"
/// the mail host
#define   MP_SMTPHOST         "MailHost"
/// the news server
#define   MP_NNTPHOST         "NewsHost"
/// show CC field in message composition?
#define   MP_SHOWCC         "ShowCC"
/// show BCC field in message composition?
#define   MP_SHOWBCC         "ShowBCC"
/// hostname for mailbox
#define   MP_POP_HOST      "HostName"
/// login for mailbox
#define   MP_POP_LOGIN      "Login"
/// password for mailbox
#define   MP_POP_PASSWORD      "Password"
/// log level
#define   MP_LOGLEVEL      "LogLevel"
/// add extra headers
#define   MP_ADD_EXTRAHEADERS   "AddExtraHeaders"
/// list of extra headers, semicolon separated name=value
#define   MP_EXTRAHEADERS      "ExtraHeaders"
/// the default path for saving files
#define   MP_DEFAULT_SAVE_PATH      "SavePath"
/// the default filename for saving files
#define   MP_DEFAULT_SAVE_FILENAME   "SaveFileName"
/// the default extension for saving files
#define   MP_DEFAULT_SAVE_EXTENSION   "SaveExtension"
/// the wildcard for save dialog
#define   MP_DEFAULT_SAVE_WILDCARD   "SaveWildcard"
/// the default path for saving files
#define   MP_DEFAULT_LOAD_PATH      "LoadPath"
/// the default filename for saving files
#define   MP_DEFAULT_LOAD_FILENAME   "LoadFileName"
/// the default extension for saving files
#define   MP_DEFAULT_LOAD_EXTENSION   "LoadExtension"
/// the wildcard for save dialog
#define   MP_DEFAULT_LOAD_WILDCARD   "LoadWildcard"
/// default value for To: field in composition
#define   MP_COMPOSE_TO         "ComposeToDefault"
/// default value for Cc: field in composition
#define   MP_COMPOSE_CC         "ComposeCcDefault"
/// default value for Bcc: field in composition
#define   MP_COMPOSE_BCC         "ComposeBccDefault"
/// use signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE   "ComposeInsertSignature"
/// use "--" to separate signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_SEPARATOR   "ComposeSeparateSignature"
/// filename of signature file
#define   MP_COMPOSE_SIGNATURE      "SignatureFile"
/// use XFace in composition?
#define   MP_COMPOSE_USE_XFACE   "UseXFaces"
/// Name from where to read XFace
#define   MP_COMPOSE_XFACE_FILE   "XFace"
/// the folder type for a mailbox (see enum in MFolder class)
#define   MP_FOLDER_TYPE         "Type"
/// the folder flags (see MFolder)
#define   MP_FOLDER_FLAGS        "Flags"
/// the filename for a mailbox
#define   MP_FOLDER_PATH         "Path"
/// comment
#define   MP_FOLDER_COMMENT      "Comment"
/// update interval for folders in seconds
#define   MP_UPDATEINTERVAL      "UpdateInterval"
/// wrapmargin for composition view (set to -1 to disable it)
#define   MP_COMPOSE_WRAPMARGIN      "WrapMargin"
/// show MESSAGE/RFC822 as text?
#define   MP_RFC822_IS_TEXT      "Rfc822IsText"
/// prefix for subject in replies
#define   MP_REPLY_PREFIX         "ReplyPrefix"
/// prefix for text in replies
#define   MP_REPLY_MSGPREFIX      "ReplyQuote"
/// prefix for subject in forwards
#define   MP_FORWARD_PREFIX         "ForwardPrefix"
/// show XFaces?
#define   MP_SHOW_XFACES         "ShowXFaces"
// show graphics inline
#define   MP_INLINE_GFX         "InlineGraphics"
/// which font to use
#define   MP_FTEXT_FONT         "FontFamily"
/// which font size
#define   MP_FTEXT_SIZE         "FontSize"
/// which font style
#define   MP_FTEXT_STYLE         "FontStyle"
/// which font weight
#define   MP_FTEXT_WEIGHT         "FontWeight"
// which foreground colour for the font
#define   MP_FTEXT_FGCOLOUR      "FontForeGround"
// which background colour for the font
#define   MP_FTEXT_BGCOLOUR      "FontBackGround"
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS      "HighlightURL"
/// open URLs with
#define   MP_BROWSER         "Browser"
/// Browser is netscape variant
#define   MP_BROWSER_ISNS    "BrowserIsNetscape"
/// external editor to use for message composition (use %s for filename)
#define MP_EXTERNALEDITOR    "ExternalEditor"
/// execute a command when new mail arrives?
#define   MP_USE_NEWMAILCOMMAND      "CommandOnNewMail"
/// command to execute when new mail arrives
#define   MP_NEWMAILCOMMAND      "OnNewMail"
/// show new mail messages?
#define   MP_SHOW_NEWMAILMSG      "ShowNewMail"
/// show detailed info about how many new mail messages?
#define   MP_SHOW_NEWMAILINFO      "ShowNewMailInfo"
/// keep copies of outgoing mail?
#define   MP_USEOUTGOINGFOLDER   "KeepCopies"
/// write outgoing mail to folder:
#define   MP_OUTGOINGFOLDER      "SentMailFolder"
/// Show all message headers?
#define   MP_SHOWHEADERS         "ShowHeaders"
/// Autocollect email addresses? 0=no 1=ask 2=always
#define   MP_AUTOCOLLECT         "AutoCollect"
/// Name of the address books for autocollected addresses
#define   MP_AUTOCOLLECT_ADB     "AutoCollectAdb"
/// Autocollect entries with names only?
#define   MP_AUTOCOLLECT_NAMED "AutoCollectNamed"
/** @name for folder list ctrls: ratios of the width to use for
    columns */
//@{
/// status
#define   MP_FLC_STATUSWIDTH   "ColumnWidthStatus"
/// subject
#define   MP_FLC_SUBJECTWIDTH   "ColumnWidthSubject"
/// from
#define   MP_FLC_FROMWIDTH   "ColumnWidthFrom"
/// date
#define   MP_FLC_DATEWIDTH   "ColumnWidthDate"
/// size
#define   MP_FLC_SIZEWIDTH   "ColumnWidthSize"
//@}
/** @name for folder list ctrls: column numbers */
//@{
/// status
#define   MP_FLC_STATUSCOL   "ColumnStatus"
/// subject
#define   MP_FLC_SUBJECTCOL   "ColumnSubject"
/// from
#define   MP_FLC_FROMCOL   "ColumnFrom"
/// date
#define   MP_FLC_DATECOL   "ColumnDate"
/// size
#define   MP_FLC_SIZECOL   "ColumnSize"
//@}
//@}
/// an entry used for testing
#define MP_TESTENTRY      "TestEntry"
//@}

/** @name default values of configuration entries */
//@{
/// The Profile Type.
#define   MP_PROFILE_TYPE_D      0
/// our version
#define   MP_VERSION_D          M_EMPTYSTRING
/// are we running for the first time?
#define   MP_FIRSTRUN_D         1
/// shall we record default values in configuration files
#define   MP_RECORDDEFAULTS_D      0
/// default window position x
#define   MP_XPOS_D        20
/// default window position y
#define   MP_YPOS_D        50
/// default window width
#define   MP_WIDTH_D   600
/// default window height
#define   MP_HEIGHT_D   400
/// show log window?
#define   MP_SHOWLOG_D  1
/// help browser name
#define   MP_HELPBROWSER_D   "netscape"
/// is help browser of netscape type?
#define   MP_HELPBROWSER_ISNS_D   1
/// the directory for mbox folders
#define   MP_MBOXDIR_D         M_EMPTYSTRING
/// expand folder tree control?
#define   MP_EXPAND_TREECTRL_D   1

/// the user's M directory
#define   MP_USERDIR_D         M_EMPTYSTRING

// Unix-only entries
#ifdef OS_UNIX
/// path list for M's directory
#define   MP_PATHLIST_D M_PREFIX ":/usr/local:/usr/:/opt:/opt/local:/usr/opt:/usr/local/opt"
/// path to M directory
#define   MP_ROOTPATH_D   M_PREFIX "/share/M"
/// the name of M's root directory
#define   MP_ROOTDIRNAME_D   "M"
/// the name of the M directory
#define   MP_USER_MDIR_D         ".M"
/// the path where to find .afm files
#define   MP_AFMPATH_D M_BASEDIR "/afm:/usr/share:/usr/lib:/usr/local/share:/usr/local/lib:/opt/ghostscript:/opt/enscript"
/// the path to the /etc directories (configuration files)
#define   MP_ETCPATH_D "/etc:/usr/etc:/usr/local/etc:/opt/etc:/usr/share/etc:/usr/local/share/etc"
/// the path to the m directory
#define   MP_PREFIXPATH_D "/usr:/usr/local:/opt:/usr/share:/usr/local/share:/opt/share:/usr/local/opt:/usr/local/opt/share:/tmp"
#endif // Unix

/// the locale for translation to national languages
#define   MP_LOCALE_D               M_EMPTYSTRING
/// the default icon for frames
#define   MP_ICON_MFRAME_D      "mframe.xpm"
/// the icon for the main frame
#define   MP_ICON_MAINFRAME_D      "mainframe.xpm"
/// the icon directoy
#define   MP_ICONPATH_D         "icons"
/// the profile path
#define   MP_PROFILE_PATH_D      "."
/// the extension to use for profile files
#define   MP_PROFILE_EXTENSION_D      ".profile"
/// the name of the mailcap file
#define   MP_MAILCAP_D         "mailcap"
/// the name of the mime types file
#define   MP_MIMETYPES_D         "mime.types"
/// the label for the From: field
/// the printf() format for dates
#define   MP_DATE_FMT_D         "%2u.%2u.%4u"
/// show console window
#define   MP_SHOWCONSOLE_D      1
/// names of folders to open at startup
#define   MP_OPENFOLDERS_D      M_EMPTYSTRING
/// name of folder to open in mainframe
#define   MP_MAINFOLDER_D        "INBOX"
/// path for Python
#define   MP_PYTHONPATH_D         M_EMPTYSTRING
/// is Python enabled (this is a run-time option)?
#define   MP_USEPYTHON_D         1
/// start-up script to run
#define     MP_STARTUPSCRIPT_D            "Minit"
/// show splash screen on startup?
#define     MP_SHOWSPLASH_D      1
/// how long should splash screen stay (0 disables timeout)?
#define MP_SPLASHDELAY_D        5000
/// how often should we autosave the profile settings (0 to disable)?
#define   MP_AUTOSAVEDELAY_D       60000
/// ask user if he really wants to exit?
#define   MP_CONFIRMEXIT_D      1
/// open folders when they're clicked (otherwise - double clicked)
#define   MP_OPEN_ON_CLICK_D     1
/// create .profile files?
#define   MP_CREATE_PROFILES_D   0
/// umask setting for normal files
#define   MP_UMASK_D               022
/**@name Printer settings */
//@{
/// Command
#define MP_PRINT_COMMAND   "PrintCommand"
/// Orientation
#define MP_PRINT_ORIENTATION "PrintOrientation"
/// Scaling
#define MP_PRINT_SCALING   "PrintScaling"
/// Preview command
#define MP_PRINT_PREVIEWCMD   "PrintPreviewCommand"
/// Printout file
#define MP_PRINT_FILE         "PrintFile"
/// Translation X
#define MP_PRINT_TRANS_X      "PrintTranslationX"
/// Translation Y
#define MP_PRINT_TRANS_Y      "PrintTranslationY"
/// FIXME What about paper format?
//@}
/**@name for BBDB address book support */
//@{
/// generate unique names
#define   MP_BBDB_GENERATEUNIQUENAMES_D   0L
/// ignore entries without names
#define   MP_BBDB_IGNOREANONYMOUS_D      0L
/// name for anonymous entries, when neither first nor family name are set
#define   MP_BBDB_ANONYMOUS_D   "anonymous"
/// save on exit, 0=no, 1=ask, 2=always
#define   MP_BBDB_SAVEONEXIT_D  M_ACTION_PROMPT
//@}
/**@name For Profiles: */
//@{
//@}
/// the user's full name
#define   MP_PERSONALNAME_D      M_EMPTYSTRING
/// the username/login
#define   MP_USERNAME_D         M_EMPTYSTRING
/// the user's hostname
#define   MP_HOSTNAME_D         M_EMPTYSTRING
/// the mail host
#define   MP_SMTPHOST_D         ""
/// tyhe mail server fallback
#define   MP_SMTPHOST_FB        "localhost"
/// the news server
#define   MP_NNTPHOST_D         ""
/// the news server fallback
#define   MP_NNTPHOST_FB        "news"
/// the username for returned mail
#define   MP_RETURN_ADDRESS_D      M_EMPTYSTRING
/// show CC field in message composition?
#define   MP_SHOWCC_D         1
/// show BCC field in message composition?
#define   MP_SHOWBCC_D         1
/// hostname for mailbox
#define   MP_POP_HOST_D      "localhost"
/// login for mailbox
#define   MP_POP_LOGIN_D      M_EMPTYSTRING
/// passwor for mailbox
#define   MP_POP_PASSWORD_D      M_EMPTYSTRING
/// log level
#define   MP_LOGLEVEL_D         0
/// add extra headers
#define   MP_ADD_EXTRAHEADERS_D      0
/// list of extra headers, semicolon separated name=value
#define   MP_EXTRAHEADERS_D      M_EMPTYSTRING
/// the default path for saving files
#define   MP_DEFAULT_SAVE_PATH_D      M_EMPTYSTRING
/// the default filename for saving files
#define   MP_DEFAULT_SAVE_FILENAME_D   "*"
/// the default extension for saving files
#define   MP_DEFAULT_SAVE_EXTENSION_D   M_EMPTYSTRING
/// the default path for saving files
#define   MP_DEFAULT_LOAD_PATH_D      M_EMPTYSTRING
/// the default filename for saving files
#define   MP_DEFAULT_LOAD_FILENAME_D   "*"
/// the default extension for saving files
#define   MP_DEFAULT_LOAD_EXTENSION_D   M_EMPTYSTRING
/// default value for To: field in composition
#define   MP_COMPOSE_TO_D         M_EMPTYSTRING
/// default value for Cc: field in composition
#define   MP_COMPOSE_CC_D         M_EMPTYSTRING
/// default value for Bcc: field in composition
#define   MP_COMPOSE_BCC_D      M_EMPTYSTRING
/// use signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_D   1
/// use "--" to separate signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_SEPARATOR_D   1
/// filename of signature file
#ifdef OS_WIN
#  define   MP_COMPOSE_SIGNATURE_D      M_EMPTYSTRING
#else
#  define   MP_COMPOSE_SIGNATURE_D      "$HOME/.signature"
#endif
/// use XFace in composition?
#define   MP_COMPOSE_USE_XFACE_D   1
/// Name from where to read XFace
#define   MP_COMPOSE_XFACE_FILE_D   "$HOME/.xface"
/// the folder type for a mailbox
#define   MP_FOLDER_TYPE_D         (int)(1)  // MF_FILE
/// the folder flags (see MFolder)
#define   MP_FOLDER_FLAGS_D       0
/// the filename for a mailbox
#define   MP_FOLDER_PATH_D      ((const char *)NULL) // don't change this!
/// comment
#define   MP_FOLDER_COMMENT_D      M_EMPTYSTRING
/// update interval for folders in seconds
#define   MP_UPDATEINTERVAL_D      60
/// wrapmargin for composition view (set to -1 to disable it)
#define   MP_COMPOSE_WRAPMARGIN_D      60
/// show MESSAGE/RFC822 as text?
#define   MP_RFC822_IS_TEXT_D      0
/// prefix for subject in replies
#define   MP_REPLY_PREFIX_D      "Re:"
/// prefix for text in replies
#define   MP_REPLY_MSGPREFIX_D      " > "
/// prefix for subject in forwards
#define   MP_FORWARD_PREFIX_D      "Forwarded message: "
/// show XFaces?
#define   MP_SHOW_XFACES_D      1
// show graphics inline
#define   MP_INLINE_GFX_D       1
/// which font to use
#define   MP_FTEXT_FONT_D         (int)wxTELETYPE
/// which font size
#define   MP_FTEXT_SIZE_D         14
/// which font style
#define   MP_FTEXT_STYLE_D      (int)wxNORMAL
/// which font weight
#define   MP_FTEXT_WEIGHT_D     (int)wxNORMAL
// which foreground colour for the font
#define   MP_FTEXT_FGCOLOUR_D      "black"
// which background colour for the font
#define   MP_FTEXT_BGCOLOUR_D      "white"
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS_D      1

/// open URLs with
#ifdef  OS_UNIX
#  define   MP_BROWSER_D         "netscape"
#  define   MP_BROWSER_ISNS_D    1
#else  // under Windows, we know better...
#  define   MP_BROWSER_D         M_EMPTYSTRING
#  define   MP_BROWSER_ISNS_D    0l
#endif // Unix/Win

/// external editor to use for message composition (use %s for filename)
#ifdef OS_UNIX
#  define   MP_EXTERNALEDITOR_D  "gvim %s"
#else
#  define   MP_EXTERNALEDITOR_D  "notepad %s"
#endif // Unix/Win

/// command to execute when new mail arrives
#ifdef  OS_UNIX
#   define   MP_USE_NEWMAILCOMMAND_D   1
#   define   MP_NEWMAILCOMMAND_D   "/usr/bin/play "M_BASEDIR"/newmail.wav"
#else
#   define   MP_USE_NEWMAILCOMMAND_D   0
#   define   MP_NEWMAILCOMMAND_D   M_EMPTYSTRING
#endif
/// show new mail messages?
#define   MP_SHOW_NEWMAILMSG_D      1
/// show detailed info about how many new mail messages?
#define   MP_SHOW_NEWMAILINFO_D      10
/// keep copies of outgoing mail?
#define   MP_USEOUTGOINGFOLDER_D  1
/// write outgoing mail to folder:
#define   MP_OUTGOINGFOLDER_D  "SentMail"
/// Show all message headers?
#define   MP_SHOWHEADERS_D         0l
/// Autocollect email addresses? 0=no 1=ask 2=always
#define   MP_AUTOCOLLECT_D         2l
/// Name of the address books for autocollected addresses
#define   MP_AUTOCOLLECT_ADB_D    "autocollect.adb"
/// Autocollect entries with names only?
#define   MP_AUTOCOLLECT_NAMED_D 0l

/** @name for folder list ctrls: ratios of the width to use for
    columns */
//@{
/// status
#define   MP_FLC_STATUSWIDTH_D 10
/// subject
#define   MP_FLC_SUBJECTWIDTH_D  33
/// from
#define   MP_FLC_FROMWIDTH_D   20
/// date
#define   MP_FLC_DATEWIDTH_D   27
/// size
#define   MP_FLC_SIZEWIDTH_D   10
//@}
/** @name for folder list ctrls: column numbers */
//@{
/// status
#define   MP_FLC_STATUSCOL_D  0
/// subject
#define   MP_FLC_SUBJECTCOL_D 1
/// from
#define   MP_FLC_FROMCOL_D   2
/// date
#define   MP_FLC_DATECOL_D   3
/// size
#define   MP_FLC_SIZECOL_D   4
//@}

/// the wildcard for save dialog
#ifdef OS_UNIX
#  define   MP_DEFAULT_SAVE_WILDCARD_D   "*"
#  define   MP_DEFAULT_LOAD_WILDCARD_D   "*"
#else
#  define   MP_DEFAULT_SAVE_WILDCARD_D   "*.*"
#  define   MP_DEFAULT_LOAD_WILDCARD_D   "*.*"
#endif
/// an entry used for testing
#define MP_TESTENTRY_D      0L
//@}

/** @name other defines used in M */
//@{
/// title for main window
#define   M_TOPLEVELFRAME_TITLE   "M - Copyright 1998, 1999 by Karsten Ballüder"
/// do we want variable expansion for profiles?
#define   M_PROFILE_VAREXPAND      1
/// c-client lib needs a char buffer to write header data
#define   CC_HEADERBUFLEN         1000000
/// file name prefix for lock files
#define   LOCK_PREFIX         "M-lock"

//@}
#endif
