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

#include "MFolder.h"    // for Folder_xxx constants

/** @name The sections of the configuration file. */
//@{
/** The section in the global configuration file used for storing
    profiles (trailing '/' required).
*/
#ifndef M_PROFILE_CONFIG_SECTION
#  ifdef OS_WIN
#     define   M_PROFILE_CONFIG_SECTION   "/Profiles"
#  else  // Unix
#     define   M_PROFILE_CONFIG_SECTION   "/M/Profiles"
#  endif // Unix/Win
#endif

/** The section in the global configuration file used for storing
    window positions (trailing '/' required).
*/
#ifndef M_FRAMES_CONFIG_SECTION
#  ifdef OS_WIN
#  define   M_FRAMES_CONFIG_SECTION       "/Frames"
#  else  // Unix
#     define   M_FRAMES_CONFIG_SECTION    "/M/Frames"
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
#define   MC_VERSION          "Version"
/// are we running for the first time?
#define   MC_FIRSTRUN         "FirstRun"
/// shall we record default values in configuration files
#define   MC_RECORDDEFAULTS      "RecordDefaults"
/// default position x
#define   MC_XPOS            "XPos"
/// default position y
#define   MC_YPOS            "YPos"
/// window width
#define   MC_WIDTH         "Width"
/// window height   
#define   MC_HEIGHT         "Height"

/// show log window?
#define   MC_SHOWLOG          "ShowLog"

/// help browser name
#define   MC_HELPBROWSER   "HelpBrowser"
/// is help browser of netscape type?
#define   MC_HELPBROWSER_ISNS   "HelpBrowserIsNetscape"
/// the directory for mbox folders
#define   MC_MBOXDIR         "FolderDir"

// Unix-only entries
#ifdef OS_UNIX
/// search paths for M's directory
#   define   MC_PATHLIST         "PathList"
/// path to M directory
#   define   MC_ROOTPATH   "GlobalDirectory"
/// the name of M's root directory
#   define   MC_ROOTDIRNAME         "RootDirectoryName"
/// the user's M directory
#   define   MC_USERDIR         "UserDirectory"
/// the name of the M directory 
#   define   MC_USER_MDIR         "MDirName"
/// the path where to find .afm files
#   define   MC_AFMPATH         "AfmPath"
/// the path to the /etc directories (configuration files)
#   define   MC_ETCPATH         "ConfigPath"
#endif //Unix

/// the default icon for frames
#define   MC_ICON_MFRAME         "MFrameIcon"
/// the icon for the main frame
#define   MC_ICON_MAINFRAME      "MainFrameIcon"
/// the icon directory
#define   MC_ICONPATH         "IconDirectory"
/// the path for finding profiles
#define   MC_PROFILE_PATH         "ProfilePath"
/// the extension to use for profile files
#define   MC_PROFILE_EXTENSION      "ProfileExtension"
/// the name of the mailcap file
#define   MC_MAILCAP         "MailCap"
/// the name of the mime types file
#define   MC_MIMETYPES         "MimeTypes"
/// the label for the From: field
#define   MC_FROM_LABEL         "FromLabel"
/// the label for the To: field
#define   MC_TO_LABEL         "ToLabel"
/// the label for the CC: field
#define   MC_CC_LABEL         "CcLabel"
/// the label for the BCC: field
#define   MC_BCC_LABEL         "BccLabel"
/// the printf() format for dates
#define   MC_DATE_FMT         "DateFormat"
/// show console window
#define   MC_SHOWCONSOLE      "ShowConsole"  
/// name of address database
#define   MC_ADBFILE         "AddressBook"
/// names of folders to open at startup (semicolon separated list)
#define   MC_OPENFOLDERS         "OpenFolders"
/// name of folder to open in mainframe
#define   MC_MAINFOLDER          "MainFolder"
/// path for Python
#define   MC_PYTHONPATH        "PythonPath"
/// is Python enabled (this is a run-time option)?
#define   MC_USEPYTHON         "UsePython"
/// start-up script to run
#define   MC_STARTUPSCRIPT     "StartupScript"
/// show splash screen on startup?
#define   MC_SHOWSPLASH        "ShowSplash"
/// how long should splash screen stay (0 disables timeout)?
#define   MC_SPLASHDELAY       "SplashDelay"
/// ask user if he really wants to exit?
#define   MC_CONFIRMEXIT       "ConfirmExit"
/// create .profile files?
#define   MC_CREATE_PROFILES   "CreateProfileFiles"
/**@name For Profiles: */
//@{
/// the user's full name
#define   MP_PERSONALNAME         "PersonalName"
/// the username/login
#define   MP_USERNAME         "UserName"
/// the user's hostname
#define   MP_HOSTNAME         "HostName"
/// the username for returned mail
#define   MP_RETURN_USERNAME      "ReturnUserName"
/// the hostname for returned mail
#define   MP_RETURN_HOSTNAME      "ReturnHostName"
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
/// the folder type for a mailbox (see FolderType enum)
#define   MP_FOLDER_TYPE         "Type"
/// the filename for a mailbox
#define   MP_FOLDER_PATH         "Path"
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
/// keep copies of outgoing mail?
#define   MP_USEOUTGOINGFOLDER   "KeepCopies"
/// write outgoing mail to folder:
#define   MP_OUTGOINGFOLDER      "SentMailFolder"
/// Show all message headers?
#define   MP_SHOWHEADERS         "ShowHeaders"
/// Autocollect email addresses? 0=no 1=ask 2=always
#define   MP_AUTOCOLLECT         "AutoCollect"
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
/// the wildcard for save dialog
//@}
//@}

/** @name default values of configuration entries */
//@{
/// our version
#define   MC_VERSION_D          ""
/// are we running for the first time?
#define   MC_FIRSTRUN_D         1
/// shall we record default values in configuration files
#define   MC_RECORDDEFAULTS_D      0
/// default window position x
#define   MC_XPOS_D        20
/// default window position y
#define   MC_YPOS_D        50
/// default window width
#define   MC_WIDTH_D   600
/// default window height
#define   MC_HEIGHT_D   400
/// show log window?
#define   MC_SHOWLOG_D  1
/// help browser name
#define   MC_HELPBROWSER_D   "netscape"
/// is help browser of netscape type?
#define   MC_HELPBROWSER_ISNS_D   1
/// the directory for mbox folders
#define   MC_MBOXDIR_D         ""

// Unix-only entries
#ifdef OS_UNIX
/// path list for M's directory
#define   MC_PATHLIST_D M_PREFIX":/usr/local:/usr/:/opt:/opt/local:/usr/opt:/usr/local/opt"
/// path to M directory
#define   MC_ROOTPATH_D   M_PREFIX"/share/M"
/// the name of M's root directory
#define   MC_ROOTDIRNAME_D   "M"
/// the user's M directory
#define   MC_USERDIR_D         ""
/// the name of the M directory 
#define   MC_USER_MDIR_D         ".M"
/// the path where to find .afm files
#define   MC_AFMPATH_D "/usr/share:/usr/lib:/usr/local/share:/usr/local/lib:/opt/ghostscript:/opt/enscript"
/// the path to the /etc directories (configuration files)
#define   MC_ETCPATH_D "/etc:/usr/etc:/usr/local/etc:/opt/etc:/usr/share/etc:/usr/local/share/etc"
#endif // Unix

/// the default icon for frames
#define   MC_ICON_MFRAME_D      "mframe.xpm"
/// the icon for the main frame
#define   MC_ICON_MAINFRAME_D      "mainframe.xpm"
/// the icon directoy
#define   MC_ICONPATH_D         "icons"
/// the profile path
#define   MC_PROFILE_PATH_D      "."
/// the extension to use for profile files
#define   MC_PROFILE_EXTENSION_D      ".profile"
/// the name of the mailcap file
#define   MC_MAILCAP_D         "mailcap"
/// the name of the mime types file
#define   MC_MIMETYPES_D         "mime.types"
/// the label for the From: field
#define   MC_FROM_LABEL_D         "From:"
/// the label for the To: field
#define   MC_TO_LABEL_D         "To:"
/// the label for the CC: field
#define   MC_CC_LABEL_D         "CC:"
/// the label for the BCC: field
#define   MC_BCC_LABEL_D       "BCC:"
/// the printf() format for dates
#define   MC_DATE_FMT_D         "%2u.%2u.%4u"
/// show console window
#define   MC_SHOWCONSOLE_D      1
/// names of folders to open at startup
#define   MC_OPENFOLDERS_D      ""
/// name of folder to open in mainframe
#define   MC_MAINFOLDER_D        "INBOX"
/// path for Python
#define   MC_PYTHONPATH_D         ""
/// is Python enabled (this is a run-time option)?
#define   MC_USEPYTHON_D         1
/// start-up script to run
#define     MC_STARTUPSCRIPT_D            "Minit"
/// show splash screen on startup?
#define     MC_SHOWSPLASH_D      1
/// how long should splash screen stay (0 disables timeout)?
#define MC_SPLASHDELAY_D        5000
/// ask user if he really wants to exit?
#define   MC_CONFIRMEXIT_D      1
/// create .profile files?
#define   MC_CREATE_PROFILES_D   0
/**@name For Profiles: */
//@{
//@}
/// the user's full name
#define   MP_PERSONALNAME_D      ""
/// the username/login
#define   MP_USERNAME_D         ""
/// the user's hostname
#define   MP_HOSTNAME_D         ""
/// the mail host
#define   MP_SMTPHOST_D         "localhost"
/// the news server
#define   MP_NNTPHOST_D         "news"
// the username for returned mail
//#define   MP_RETURN_USERNAME_D      "ReturnUserName"
// the hostname for returned mail
//#define   MP_RETURN_HOSTNAME_D      "ReturnHostName"
/// show CC field in message composition?
#define   MP_SHOWCC_D         1
/// show BCC field in message composition?
#define   MP_SHOWBCC_D         1
/// hostname for mailbox
#define   MP_POP_HOST_D      "localhost"
/// login for mailbox
#define   MP_POP_LOGIN_D      ""
/// passwor for mailbox   
#define   MP_POP_PASSWORD_D      ""
/// log level
#define   MP_LOGLEVEL_D         0
/// add extra headers
#define   MP_ADD_EXTRAHEADERS_D      0
/// list of extra headers, semicolon separated name=value
#define   MP_EXTRAHEADERS_D      ""
/// the default path for saving files
#define   MP_DEFAULT_SAVE_PATH_D      ""
/// the default filename for saving files
#define   MP_DEFAULT_SAVE_FILENAME_D   "*"
/// the default extension for saving files
#define   MP_DEFAULT_SAVE_EXTENSION_D   ""
/// the default path for saving files
#define   MP_DEFAULT_LOAD_PATH_D      ""
/// the default filename for saving files
#define   MP_DEFAULT_LOAD_FILENAME_D   "*"
/// the default extension for saving files
#define   MP_DEFAULT_LOAD_EXTENSION_D   ""
/// default value for To: field in composition
#define   MP_COMPOSE_TO_D         ""
/// default value for Cc: field in composition
#define   MP_COMPOSE_CC_D         ""
/// default value for Bcc: field in composition
#define   MP_COMPOSE_BCC_D      ""
/// use signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_D   1
/// use "--" to separate signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_SEPARATOR_D   1
/// filename of signature file
#ifdef OS_WIN
#  define   MP_COMPOSE_SIGNATURE_D      ""
#else
#  define   MP_COMPOSE_SIGNATURE_D      "$HOME/.signature"
#endif
/// the folder type for a mailbox
#define   MP_FOLDER_TYPE_D         MFolder::File
/// the filename for a mailbox
#define   MP_FOLDER_PATH_D      ((const char *)NULL) // don't change this!
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
#define   MP_FTEXT_FONT_D         wxTELETYPE
/// which font size
#define   MP_FTEXT_SIZE_D         14
/// which font style
#define   MP_FTEXT_STYLE_D      wxNORMAL
/// which font weight
#define   MP_FTEXT_WEIGHT_D     wxNORMAL
// which foreground colour for the font
#define   MP_FTEXT_FGCOLOUR_D      "black"
// which background colour for the font
#define   MP_FTEXT_BGCOLOUR_D      "white"
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS_D      1

/// open URLs with
#ifdef  OS_UNIX
#  define   MP_BROWSER_D         "Mnetscape"
#else  // under Windows, we know better...
#  define   MP_BROWSER_D         ""
#endif // Unix/Win

/// keep copies of outgoing mail?
#define   MP_USEOUTGOINGFOLDER_D  1
/// write outgoing mail to folder:
#define   MP_OUTGOINGFOLDER_D  "SentMail"
/// Show all message headers?
#define   MP_SHOWHEADERS_D         0
/// Autocollect email addresses? 0=no 1=ask 2=always
#define   MP_AUTOCOLLECT_D         2

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
//@}

/** @name other defines used in M */
//@{
/// title for main window
#define   M_TOPLEVELFRAME_TITLE   "M - Copyright 1998 by Karsten Ballüder"
/// do we want variable expansion for profiles?
#define   M_PROFILE_VAREXPAND      1
/// c-client lib needs a char buffer to write header data 
#define   CC_HEADERBUFLEN         1000000
/// file name prefix for lock files
#define   LOCK_PREFIX         "M-lock"

//@}
#endif
