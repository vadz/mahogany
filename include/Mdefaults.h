/*-*- c++ -*-********************************************************
 * Mdefaults.h : all default defines for readEntry() config entries *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef MDEFAULTS_H
#define MDEFAULTS_H

#include "Mcallbacks.h"
#include "Mversion.h"

// under Windows 12pt font looks too huge, letters are like in a book for
// children
#ifdef OS_WIN
#  define DEFAULT_FONT_SIZE 10l
#else
#  define DEFAULT_FONT_SIZE 12l
#endif

/** @name The sections of the configuration file. */
///@{

/** The section in the global configuration file used for storing
    profiles (trailing '/' required).
*/
#ifndef M_PROFILE_CONFIG_SECTION
#  define   M_PROFILE_CONFIG_SECTION_UNIX   "/M/Profiles"
#  define   M_IDENTITY_CONFIG_SECTION_UNIX   "/M/Ids"
#  define   M_FILTERS_CONFIG_SECTION_UNIX   "/M/Filters"
#  ifdef OS_WIN
#     define   M_PROFILE_CONFIG_SECTION   "/Profiles"
#     define   M_IDENTITY_CONFIG_SECTION   "/Ids"
#     define   M_FILTERS_CONFIG_SECTION   "/Filters"
#  else  // Unix
#     define   M_PROFILE_CONFIG_SECTION   M_PROFILE_CONFIG_SECTION_UNIX
#     define   M_IDENTITY_CONFIG_SECTION  M_IDENTITY_CONFIG_SECTION_UNIX
#     define   M_FILTERS_CONFIG_SECTION   M_FILTERS_CONFIG_SECTION_UNIX
#  endif // Unix/Win
#endif

/** The section in the global configuration file used for storing
    folder profiles (trailing '/' required).
*/
#ifndef M_FOLDER_CONFIG_SECTION
#  define   M_FOLDER_CONFIG_SECTION   M_PROFILE_CONFIG_SECTION
#endif

// The trailing slashes in the following defines are important, otherwise
// Profile.cpp will get broken!

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
#  define   M_SETTINGS_CONFIG_SECTION       "Settings/"
#  else  // Unix
#     define   M_SETTINGS_CONFIG_SECTION    "Settings/"
#  endif // Unix/Win
#endif

/// the subgroup for custom headers values
#define M_CUSTOM_HEADERS_CONFIG_SECTION "CustomHeaders"

/// the subgroup for message templates (should have the trailing slash)
#define M_TEMPLATE_SECTION "Template/"
#define M_TEMPLATES_SECTION "Templates/"

/** @name Keys where the template for messages of given type is stored */
//@{
#define MP_TEMPLATE_NEWMESSAGE   "NewMessage"
#define MP_TEMPLATE_NEWARTICLE   "NewArticle"
#define MP_TEMPLATE_REPLY        "Reply"
#define MP_TEMPLATE_FOLLOWUP     "Followup"
#define MP_TEMPLATE_FORWARD      "Forward"
//@}

//@}

/** @name User level: novice, intermidiate, expert */
//@{
/// minimal user level, prefer simplicity over flexibility
#define M_USERLEVEL_NOVICE 0L
/// advanced user, more configurability features available
#define M_USERLEVEL_ADVANCED 1L
/// expert user, all features available
#define M_USERLEVEL_EXPERT 2L
/// for the developers themselves
#define M_USERLEVEL_WIZARD 3L
//@}

/** @name Levels of  interaction, do something or not?

    NB: these values can't be changed as they are written to (and read from)
        the profile and so this would create a compatibility problem
 */
//@{
enum
{
   /// never do this action
   M_ACTION_NEVER,
   /// ask user if he wants it
   M_ACTION_PROMPT,
   /// don't ask user, do it
   M_ACTION_ALWAYS
};
//@}

/// the positions of the standard folders in the tree
enum MFolderIndex
{
   // system folders
   MFolderIndex_Inbox,
   MFolderIndex_NewMail,
   MFolderIndex_SentMail,
   MFolderIndex_Trash,
   MFolderIndex_Outbox,
   MFolderIndex_Draft,

   // initial servers
   MFolderIndex_IMAP,
   MFolderIndex_POP,
   MFolderIndex_NNTP,

   // anything else goes here
   MFolderIndex_Max
};

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
/// expand env vars in entries read from config?
#define   MP_EXPAND_ENV_VARS  "ExpandEnvVars"
/// default position x
#define   MP_XPOS            "XPos"
/// default position y
#define   MP_YPOS            "YPos"
/// window width
#define   MP_WIDTH         "Width"
/// window height
#define   MP_HEIGHT         "Height"
/// window iconisation status
#define   MP_ICONISED         "Iconised"
/// window maximized?
#define   MP_MAXIMISED         "Maximised"

/// show log window?
#define   MP_SHOWLOG          "ShowLog"
/// the file to save log messages to (if not empty)
#define   MP_LOGFILE          "LogFile"
/// debug protocols and folder access?
#define   MP_DEBUG_CCLIENT   "MailDebug"

/// open ADB editor on startup?
#define   MP_SHOWADBEDITOR    "ShowAdb"

/// show tips at startup?
#define MP_SHOWTIPS "ShowTips"
/// the index of the last tip which was shown
#define MP_LASTTIP "LastTip"

/// expand folder tree control?
#define   MP_EXPAND_TREECTRL   "ExpandTreeControl"
/// focus follows mouse?
#define MP_FOCUS_FOLLOWSMOUSE  "FocusFollowsMouse"
/// dockable menu bars?
#define   MP_DOCKABLE_MENUBARS "MenuBarsDockable"
/// dockable tool bars?
#define   MP_DOCKABLE_TOOLBARS   "ToolBarsDockable"
/// flat tool bars?
#define   MP_FLAT_TOOLBARS   "ToolBarsFlat"

/// help browser name
#define   MP_HELPBROWSER   "HelpBrowser"
/// is help browser of netscape type?
#define   MP_HELPBROWSER_ISNS   "HelpBrowserIsNetscape"
/// width of help frame
#define MP_HELPFRAME_WIDTH   "HelpFrameWidth"
/// height of help frame
#define MP_HELPFRAME_HEIGHT  "HelpFrameHeight"
/// xpos of help frame
#define MP_HELPFRAME_XPOS   "HelpFrameXpos"
/// ypos of help frame
#define MP_HELPFRAME_YPOS   "HelpFrameYpos"
/// the directory for mbox folders
#define   MP_MBOXDIR         "FolderDir"
/// the news spool directory
#define MP_NEWS_SPOOL_DIR "NewsSpool"
/// command to convert tiff faxes to postscript
#define   MP_TIFF2PS         "FaxToPS"
/// preferred intermediary image format in conversions (0=xpm,1=png,2=bmp,3=jpg)
#define MP_TMPGFXFORMAT      "ConvertGfxFormat"
/// the user's M directory
#   define   MP_USERDIR         "UserDirectory"
/// the acceptance status of the license
#define MP_LICENSE_ACCEPTED   "LicenseAccepted"

/// the complete path to the glocal M directory
#   define   MP_GLOBALDIR      "GlobalDir"

/// the directory containing the help files
#define MP_HELPDIR "HelpDir"

// Unix-only entries
#ifdef OS_UNIX
/// search paths for M's directory
#   define   MP_PATHLIST         "PathList"
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

/// Which encryption algorithm to use : 0 = simple builtin, 1 = twofish
#define MP_CRYPTALGO    "CryptAlgo"
/// DoesTwoFish work? (-1 unknown, 0 no, 1 yes)
#define MP_CRYPT_TWOFISH_OK "TwoFishOk"
/// some test data
#define MP_CRYPT_TESTDATA "CryptData"
/// the locale for translation to national languages
#define   MP_LOCALE               "Locale"
/// the default character set
#define MP_CHARSET      "CharSet"
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
/// the key for identity redirection
#define MP_PROFILE_IDENTITY "ProfileId"
/// the name of the mailcap file
#define   MP_MAILCAP         "MailCap"
/// the name of the mime types file
#define   MP_MIMETYPES         "MimeTypes"
/// the strftime() format for dates
#define   MP_DATE_FMT         "DateFormat"
/// display all dates as GMT?
#define   MP_DATE_GMT         "GMTDate"
/// show console window
#define   MP_SHOWCONSOLE      "ShowConsole"
/// name of address database
#define   MP_ADBFILE         "AddressBook"
/// open any folders at all on startup?
#define   MP_DONTOPENSTARTUP   "DontOpenAtStartup"
/// names of folders to open at startup (semicolon separated list)
#define   MP_OPENFOLDERS         "OpenFolders"
/// reopen the last opened folder in the main frame
#define   MP_REOPENLASTFOLDER "ReopenLastFolder"
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
/// how often should we check for incoming mail (secs, 0 to disable)?
#define   MP_POLLINCOMINGDELAY       "PollIncomingDelay"
/// collect all new mail at startup?
#define   MP_COLLECTATSTARTUP "CollectAtStartup"
/// ask user if he really wants to exit?
#define   MP_CONFIRMEXIT       "ConfirmExit"
/// open folders when they're clicked (otherwise - double clicked)
#define   MP_OPEN_ON_CLICK     "ClickToOpen"
/// show all folders (even hidden ones) in the folder tree?
#define   MP_SHOW_HIDDEN_FOLDERS "ShowHiddenFolders"
/// create .profile files?
#define   MP_CREATE_PROFILES   "CreateProfileFiles"
/// umask setting for normal files
#define   MP_UMASK               "Umask"
/// automatically show first message in folderview?
#define   MP_AUTOSHOW_FIRSTMESSAGE "AutoShowFirstMessage"
/// automatically show first unread message in folderview?
#define   MP_AUTOSHOW_FIRSTUNREADMESSAGE "AutoShowFirstUnread"
/// open messages when they're clicked (otherwise - double clicked)
#define   MP_PREVIEW_ON_SELECT     "PreviewOnSelect"
/// program used to convert image files?
#define   MP_CONVERTPROGRAM      "ImageConverter"
/// list of modules to load at startup
#define MP_MODULES               "Modules"
/// the user path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_USER   "CompooseTemplatePathUser"
/// the global path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_GLOBAL   "CompooseTemplatePath"

/// the format string for the folder tree display
#define MP_FOLDERSTATUS_TREE "TreeViewFmt"
/// the format string for status bar folder status display
#define MP_FOLDERSTATUS_STATBAR "StatusBarFmt"
/// the format string for title bar folder status display
#define MP_FOLDERSTATUS_TITLEBAR "TitleBarFmt"

/**@name Printer settings */
//@{
/// Command
#define MP_PRINT_COMMAND   "PrintCommand"
/// Options
#define MP_PRINT_OPTIONS   "PrintOptions"
/// Orientation
#define MP_PRINT_ORIENTATION "PrintOrientation"
/// print mode
#define MP_PRINT_MODE      "PrintMode"
/// paper name
#define MP_PRINT_PAPER   "PrintPaperType"
/// paper name
#define MP_PRINT_FILE   "PrintFilenname"
/// print in colour?
#define MP_PRINT_COLOUR "PrintUseColour"
/// top margin
#define MP_PRINT_TOPMARGIN_X    "PrintMarginLeft"
/// left margin
#define MP_PRINT_TOPMARGIN_Y   "PrintMarginTop"
/// bottom margin
#define MP_PRINT_BOTTOMMARGIN_X    "PrintMarginRight"
/// right margin
#define MP_PRINT_BOTTOMMARGIN_Y   "PrintMarginBottom"
/// zoom level in print preview
#define MP_PRINT_PREVIEWZOOM   "PrintPreviewZoom"
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
/// the current user identity
#define   MP_CURRENT_IDENTITY  "Identity"
/// the user's full name
#define   MP_PERSONALNAME     "PersonalName"
/// the user's qualification
#define   MP_USERLEVEL        "Userlevel"
/// the username/login
#define   MP_USERNAME         "UserName"
/// the user's hostname
#define   MP_HOSTNAME         "HostName"
/// Add this hostname for addresses without hostname?
#define   MP_ADD_DEFAULT_HOSTNAME   "AddDefaultHostName"
/// (the username for returned mail) E-mail address
#define   MP_FROM_ADDRESS      "ReturnAddress"
/// Reply address
#define   MP_REPLY_ADDRESS      "ReplyAddress"
/// the default POP3 host
#define   MP_POPHOST          "Pop3Host"
/// the default IMAP4 host
#define   MP_IMAPHOST          "Imap4Host"
/// the mail host
#define   MP_SMTPHOST         "MailHost"
/// use the specified sender value or guess it automatically?
#define   MP_GUESS_SENDER       "GuessSender"
/// the smtp sender value
#define   MP_SENDER           "Sender"
/// the smtp host user-id
#define   MP_SMTPHOST_LOGIN   "MailHostLogin"
/// the smtp host password
#define   MP_SMTPHOST_PASSWORD  "MailHostPw"
/// the news server
#define   MP_NNTPHOST         "NewsHost"
/// the news host user-id
#define   MP_NNTPHOST_LOGIN   "NewsHostLogin"
/// the news host password
#define   MP_NNTPHOST_PASSWORD  "NewsHostPw"
/// use SSL?
#define   MP_SMTPHOST_USE_SSL         "MailHostSSL"
/// check ssl-certs for SMTP connections?
#define   MP_SMTPHOST_USE_SSL_UNSIGNED   "MailHostSSLUnsigned"
/// sendmail command
#define MP_SENDMAILCMD "SendmailCmd"
/// use sendmail?
#define MP_USE_SENDMAIL "UseSendmail"
/// use SSL?
#define   MP_NNTPHOST_USE_SSL         "NewsHostSSL"
/// check ssl-certs for NNTP connections?
#define   MP_NNTPHOST_USE_SSL_UNSIGNED   "NewsHostSSLUnsigned"
/// the beacon host to test for net connection
#define   MP_BEACONHOST      "BeaconHost"
/// does Mahogany control dial-up networking?
#define MP_DIALUP_SUPPORT   "DialUpNetSupport"

/// should we attach vCard to outgoing messages?
#define MP_USEVCARD "UseVCard"
/// the vCard to use
#define MP_VCARD "VCard"

#if defined(OS_WIN)
/// the RAS connection to use
#define MP_NET_CONNECTION "RasConnection"
#elif defined(OS_UNIX)
/// the command to go online
#define MP_NET_ON_COMMAND   "NetOffCommand"
/// the command to go offline
#define MP_NET_OFF_COMMAND   "NetOnCommand"
#endif // platform

/// show CC field in message composition?
#define   MP_SHOWCC         "ShowCC"
/// show BCC field in message composition?
#define   MP_SHOWBCC         "ShowBCC"
/// login for mailbox
#define   MP_FOLDER_LOGIN      "Login"
/// password for mailbox
#define   MP_FOLDER_PASSWORD      "Password"
/// log level
#define   MP_LOGLEVEL      "LogLevel"
/// threshold for displaying mailfolder progress dialog
#define   MP_FOLDERPROGRESS_THRESHOLD   "FolderProgressThreshold"
/// size threshold for displaying message retrieval progress dialog
#define   MP_MESSAGEPROGRESS_THRESHOLD_SIZE   "MsgProgressMinSize"
/// time threshold for displaying message retrieval progress dialog
#define   MP_MESSAGEPROGRESS_THRESHOLD_TIME   "MsgProgressDelay"
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
/// detect and remove signature when replying?
#define MP_REPLY_DETECT_SIG "DetectSig"
#if wxUSE_REGEX
/// a regex to detect signature
#define MP_REPLY_SIG_SEPARATOR "SigSeparator"
#endif
/// include the original message in the reply [no,ask,yes]
#define MP_REPLY_QUOTE_ORIG "ReplyQuoteInsert" 

/// use XFace in composition?
#define   MP_COMPOSE_USE_XFACE   "UseXFaces"
/// Name from where to read XFace
#define   MP_COMPOSE_XFACE_FILE   "XFace"
/// the folder type for a mailbox (see enum in MFolder class)
#define   MP_FOLDER_TYPE         "Type"
/// should we try to create the folder before opening it?
#define   MP_FOLDER_TRY_CREATE "TryCreate"
/// the folder icon for a mailbox (see icon functions in FolderType.h)
#define   MP_FOLDER_ICON         "Icon"
/// the position of the folder in the tree
#define MP_FOLDER_TREEINDEX "Index"
/// Where to store all new mail (obsolete)
#define MP_NEWMAIL_FOLDER      "NewMailFolder"
/// Where to store outgoing mail
#define MP_OUTBOX_NAME       "OutBoxName"
/// Use outbox?
#define MP_USE_OUTBOX          "UseOutBox"
/// Name of Trash folder?
#define MP_TRASH_FOLDER      "TrashFolder"
/// Use a trash folder?
#define MP_USE_TRASH_FOLDER   "UseTrash"
/// the filename for a mailbox
#define   MP_FOLDER_PATH         "Path"
/// comment
#define   MP_FOLDER_COMMENT      "Comment"
/// update interval for folders in seconds
#define   MP_UPDATEINTERVAL      "UpdateInterval"
/// close of folders delayed by
#define MP_FOLDER_CLOSE_DELAY   "FolderCloseDelay"
/// do automatic word wrap?
#define MP_AUTOMATIC_WORDWRAP   "AutoWrap"
/// wrapmargin for composition (set to -1 to disable it)
#define   MP_WRAPMARGIN      "WrapMargin"
/// do automatic word wrap in message view?
#define MP_VIEW_AUTOMATIC_WORDWRAP   "ViewAutoWrap"
/// wrapmargin for message view (set to -1 to disable it)
#define   MP_VIEW_WRAPMARGIN      "ViewWrapMargin"
/// show TEXT/PLAIN as inlined text?
#define   MP_PLAIN_IS_TEXT      "PlainIsText"
/// show MESSAGE/RFC822 as inlined text?
#define   MP_RFC822_IS_TEXT      "Rfc822IsText"
/// prefix for subject in replies
#define   MP_REPLY_PREFIX         "ReplyPrefix"
/// collapse reply prefixes? 0=no, 1=replace "Re"s with one, 2=use reply level
#define   MP_REPLY_COLLAPSE_PREFIX "CollapseReplyPrefix"
/// prefix for text in replies
#define   MP_REPLY_MSGPREFIX      "ReplyQuote"
/// prepend the initials of the sender to thereply prefix?
#define   MP_REPLY_MSGPREFIX_FROM_SENDER "ReplyQuoteUseSender"
/// quote the empty lines when replying?
#define   MP_REPLY_QUOTE_EMPTY      "ReplyQuoteEmpty"
/// set reply string from To: field?
#define MP_SET_REPLY_FROM_TO   "ReplyEqualsTo"
/// prefix for subject in forwards
#define   MP_FORWARD_PREFIX         "ForwardPrefix"
/// show XFaces?
#define   MP_SHOW_XFACES         "ShowXFaces"
/// show graphics inline
#define   MP_INLINE_GFX         "InlineGraphics"
/// limit size for inline graphics
#define   MP_INLINE_GFX_SIZE     "InlineGraphicsSize"
/// which viewer to use in the message view?
#define MP_MSGVIEW_VIEWER "MsgViewer"
/// which headers to show in the message view?
#define   MP_MSGVIEW_HEADERS     "MsgViewHeaders"
/// all headers we know about
#define   MP_MSGVIEW_ALL_HEADERS     "MsgViewAllHeaders"
/// should we auto detect the message encoding?
#define  MP_MSGVIEW_AUTO_ENCODING   "MsgViewAutoDetectEnc"
/// the default encoding for the viewer/composer
#define  MP_MSGVIEW_DEFAULT_ENCODING   "DefEncoding"
/// the type of the last created folder
#define   MP_LAST_CREATED_FOLDER_TYPE  "LastFolderType"
/// the filter program to apply (OBSOLETE)
#define MP_FILTER_RULE   "Filter"
/// the filters to use for this folder
#define MP_FOLDER_FILTERS   "Filters"
/// the default folder file format
#define MP_FOLDER_FILE_DRIVER   "MailboxFileFormat"
/// the background colour for the folder tree
#define MP_FOLDER_BGCOLOUR "FTreeBgColour"
/**@name  Font settings for message view */
//@{
/// message view title
#define   MP_MVIEW_TITLE_FMT   "MViewTitleFmt"
/// which font to use
#define   MP_MVIEW_FONT         "MViewFont"
/// which font size
#define   MP_MVIEW_FONT_SIZE         "MViewFontSize"
// which foreground colour for the font
#define   MP_MVIEW_FGCOLOUR      "MViewFgColour"
// which background colour for the font
#define   MP_MVIEW_BGCOLOUR      "MViewBgColour"
// which colour for URLS
#define   MP_MVIEW_URLCOLOUR      "MViewUrlColour"
// colour for attachment labels
#define   MP_MVIEW_ATTCOLOUR      "MViewAttColour"
// perform quoted text colourization?
#define   MP_MVIEW_QUOTED_COLOURIZE   "MViewQuotedColourized"
// cycle colours?
#define   MP_MVIEW_QUOTED_CYCLE_COLOURS   "MViewQuotedCycleColours"
// which colour for quoted text
#define   MP_MVIEW_QUOTED_COLOUR1      "MViewQuotedColour1"
// which colour for quoted text, second level
#define   MP_MVIEW_QUOTED_COLOUR2      "MViewQuotedColour2"
// which colour for quoted text, third level
#define   MP_MVIEW_QUOTED_COLOUR3      "MViewQuotedColour3"
/// the maximum number of whitespaces prepending >
#define   MP_MVIEW_QUOTED_MAXWHITESPACE    "MViewQuotedMaxWhitespace"
/// the maximum number of A-Z prepending >
#define   MP_MVIEW_QUOTED_MAXALPHA    "MViewQuotedMaxAlpha"
/// the colour for header names in the message view
#define   MP_MVIEW_HEADER_NAMES_COLOUR  "MViewHeaderNamesColour"
/// the colour for header values in the message view
#define   MP_MVIEW_HEADER_VALUES_COLOUR  "MViewHeaderValuesColour"
//@}
/**@name  Font settings for folder view */
//@{
/// which font to use
#define   MP_FVIEW_FONT         "FViewFont"
/// which font size
#define   MP_FVIEW_FONT_SIZE         "FViewFontSize"
/// don't show full e-mail, only sender's name
#define   MP_FVIEW_NAMES_ONLY         "FViewNamesOnly"
/// which foreground colour for the font
#define   MP_FVIEW_FGCOLOUR      "FViewFgColour"
/// which background colour for the font
#define   MP_FVIEW_BGCOLOUR      "FViewBgColour"
/// colour for deleted messages
#define   MP_FVIEW_DELETEDCOLOUR      "FViewDeletedColour"
/// colour for new messages
#define   MP_FVIEW_NEWCOLOUR      "FViewNewColour"
/// colour for recent messages
#define   MP_FVIEW_RECENTCOLOUR      "FViewRecentColour"
/// colour for unread messages
#define   MP_FVIEW_UNREADCOLOUR      "FViewUnreadColour"
/// colour for flagged messages
#define   MP_FVIEW_FLAGGEDCOLOUR      "FViewFlaggedColour"
/// update the folder view status bar to show the msg info?
#define   MP_FVIEW_STATUS_UPDATE "FViewStatUpdate"
/// folder view status bar string
#define   MP_FVIEW_STATUS_FMT  "FViewStatFmt"
// how to show the size (MessageSizeShow enum value)
#define MP_FVIEW_SIZE_FORMAT   "SizeFormat"
/// replace "From" address with "To" in messages from oneself?
#define MP_FVIEW_FROM_REPLACE "ReplaceFrom"
/// the ':' separated list of addresses which are "from oneself"
#define MP_FROM_REPLACE_ADDRESSES "ReplaceFromAdr"
//@}
/**@name  Font settings for folder tree */
//@{
/// format for the folder tree entries
#define MP_FTREE_FORMAT "FTreeFormat"
/// reflect the folder status in its parent
#define MP_FTREE_PROPAGATE "FTreePropagate"
//@}
/**@name Font settings for compose view */
//@{
/// which font to use
#define   MP_CVIEW_FONT         "CViewFont"
/// which font size
#define   MP_CVIEW_FONT_SIZE         "CViewFontSize"
// which foreground colour for the font
#define   MP_CVIEW_FGCOLOUR      "CViewFGColour"
// which background colour for the font
#define   MP_CVIEW_BGCOLOUR      "CViewBGColout"
//@}
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS      "HighlightURL"
/// sort criterium for folder listing
#define MP_MSGS_SORTBY         "SortMessagesBy"
/// re-sort messages on status change?
#define MP_MSGS_RESORT_ON_CHANGE         "ReSortMessagesOnChange"
/// use threading
#define MP_MSGS_USE_THREADING  "ThreadMessages"

#if defined(EXPERIMENTAL_JWZ_THREADING)

/// Gather messages with same subject in one thread
#define MP_MSGS_GATHER_SUBJECTS "GatherSubjectsWhenThreading"
/// break thread when subject changes
#define MP_MSGS_BREAK_THREAD "BreakThreadIfSubjectChanges"
/// Indent messages when common ancestor is missing
#define MP_MSGS_INDENT_IF_DUMMY "IndentIfDummy"

#if wxUSE_REGEX
#   define MP_MSGS_SIMPLIFYING_REGEX "SimplifyingRegex"
#   define MP_MSGS_REPLACEMENT_STRING "ReplacementString"
#else // wxUSE_REGEX
    /// Remove list prefix when comparing message's subject to gather them
#   define MP_MSGS_REMOVE_LIST_PREFIX_GATHERING "RemoveListPrefixWhenGathering"
    /// Remove list prefix when comparing message's subject to break threads
#   define MP_MSGS_REMOVE_LIST_PREFIX_BREAKING "RemoveListPrefixWhenBreaking"
#endif // wxUSE_REGEX

#endif // EXPERIMENTAL_JWZ_THREADING

/// search criterium for searching in folders
#define MP_MSGS_SEARCH_CRIT   "SearchCriterium"
/// search argument
#define MP_MSGS_SEARCH_ARG    "SearchArgument"
/// open URLs with
#define   MP_BROWSER         "Browser"
/// Browser is netscape variant
#define   MP_BROWSER_ISNS    "BrowserIsNetscape"
/// Open netscape in new window
#define   MP_BROWSER_INNW    "BrowserInNewWindow"
/// external editor to use for message composition (use %s for filename)
#define MP_EXTERNALEDITOR    "ExternalEditor"
/// start external editor automatically?
#define MP_ALWAYS_USE_EXTERNALEDITOR    "AlwaysUseExtEditor"
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
/// support efax style incoming faxes
#define MP_INCFAX_SUPPORT      "IncomingFaxSupport"
/// domains from which to support faxes, semicolon delimited
#define MP_INCFAX_DOMAINS      "IncomingFaxDomains"
/// Default name for the SSL library
#define MP_SSL_DLL_SSL      "SSLDll"
/// Default name for the SSL/crypto library
#define MP_SSL_DLL_CRYPTO "CryptoDll"

/// Use substrings in address expansion?
#define   MP_ADB_SUBSTRINGEXPANSION   "ExpandWithSubstring"

/** @name maximal amounts of data to retrieve from remote servers */
//@{
/// ask confirmation before retrieveing messages bigger than this (in Kb)
#define MP_MAX_MESSAGE_SIZE   "MaxMsgSize"
/// ask confirmation before retrieveing more headers than this
#define MP_MAX_HEADERS_NUM    "MaxHeadersNum"
/// never download more than that many messages
#define MP_MAX_HEADERS_NUM_HARD "HardHeadersLimit"
//@}

/// setting this prevents the filters from expuning the msgs automatically
#define MP_SAFE_FILTERS "SafeFilters"

/** @name timeout values for c-client mail library */
//@{
/// IMAP lookahead value
#define MP_IMAP_LOOKAHEAD "IMAPlookahead"
/// TCP/IP open timeout in seconds.
#define MP_TCP_OPENTIMEOUT "TCPOpenTimeout"
#if 0 // no longer used
/// TCP/IP read timeout in seconds.
#define MP_TCP_READTIMEOUT "TCPReadTimeout"
/// TCP/IP write timeout in seconds.
#define  MP_TCP_WRITETIMEOUT "TCPWriteTimeout"
/// TCP/IP close timeout in seconds.
#define  MP_TCP_CLOSETIMEOUT "TCPCloseTimeout"
#endif // 0
/// rsh connection timeout in seconds.
#define MP_TCP_RSHTIMEOUT "TCPRshTimeout"
/// ssh connection timeout in seconds.
#define MP_TCP_SSHTIMEOUT "TCPSshTimeout"
/// the path to rsh
#define MP_RSH_PATH "RshPath"
/// the path to ssh
#define MP_SSH_PATH "SshPath"
//@}
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
/// Do we remote synchronise configuration settings?
#define MP_SYNC_REMOTE   "SyncRemote"
/// IMAP folder to sync to
#define MP_SYNC_FOLDER   "SyncFolder"
/// our last sync date
#define MP_SYNC_DATE      "SyncDate"
/// sync filters?
#define MP_SYNC_FILTERS   "SyncFilters"
/// sync filters?
#define MP_SYNC_IDS   "SyncIds"
/// sync folders?
#define MP_SYNC_FOLDERS   "SyncFolders"
/// sync folder tree
#define MP_SYNC_FOLDERGROUP   "SyncFolderGroup"

/** @name sending */
//@{
/// confirm sending the message?
#define MP_CONFIRM_SEND "ConfirmSend"
/// preview the message being sent?
#define MP_PREVIEW_SEND "PreviewSend"
//@}

/** @name away mode */
//@{
/// automatically enter the away mode after that many minutes (0 to disable)
#define MP_AWAY_AUTO_ENTER "AutoAway"
/// automaticlly exit from the away mode when the user chooses menu command
#define MP_AWAY_AUTO_EXIT "AutoExitAway"
/// keep the value of away mode for the next run (otherwise always reset it)
#define MP_AWAY_REMEMBER "RememberAway"
/// the saved value of away status (only written if MP_AWAY_REMEMBER is set)
#define MP_AWAY_STATUS "AwayStatus"
//@}

/** @name names of obsolete configuration entries, for upgrade routines */
//@{
#define MP_OLD_FOLDER_HOST "HostName"
//@}
//@}

/** @name default values of configuration entries */
//@{
/// The Profile Type.
#define   MP_PROFILE_TYPE_D      0l
/// our version
#define   MP_VERSION_D          M_EMPTYSTRING
/// are we running for the first time?
#define   MP_FIRSTRUN_D         1
/// shall we record default values in configuration files
#define   MP_RECORDDEFAULTS_D      0l
/// expand env vars in entries read from config?
#ifdef OS_WIN
#define   MP_EXPAND_ENV_VARS_D 0l
#else
#define   MP_EXPAND_ENV_VARS_D 1l
#endif
/// default window position x
#define   MP_XPOS_D        20
/// default window position y
#define   MP_YPOS_D        50
/// default window width
#define   MP_WIDTH_D   600
/// default window height
#define   MP_HEIGHT_D   400
/// window iconisation status
#define   MP_ICONISED_D 0l
/// window maximized?
#define   MP_MAXIMISED_D 0l
/// show log window?
#define   MP_SHOWLOG_D  1
/// the file to save log messages to (if not empty)
#define   MP_LOGFILE_D ""
/// debug protocols and folder access?
#define   MP_DEBUG_CCLIENT_D   0l
/// open ADB editor on startup?
#define   MP_SHOWADBEDITOR_D 0L
/// show tips at startup?
#define MP_SHOWTIPS_D 1L
/// the index of the last tip which was shown
#define MP_LASTTIP_D 0L
/// help browser name
#define   MP_HELPBROWSER_D   "netscape"
/// is help browser of netscape type?
#define   MP_HELPBROWSER_ISNS_D   1
/// width of help frame
#define MP_HELPFRAME_WIDTH_D 560l
/// height of help frame
#define MP_HELPFRAME_HEIGHT_D    500l
/// xpos of help frame
#define MP_HELPFRAME_XPOS_D 40l
/// ypos of help frame
#define MP_HELPFRAME_YPOS_D  40l
/// the directory for mbox folders
#define   MP_MBOXDIR_D         M_EMPTYSTRING
/// the news spool directory
#define MP_NEWS_SPOOL_DIR_D M_EMPTYSTRING
/// command to convert tiff faxes to postscript
#define   MP_TIFF2PS_D         "convert %s %s"
/// preferred intermediary image format in conversions
#define MP_TMPGFXFORMAT_D      2
/// expand folder tree control?
#define   MP_EXPAND_TREECTRL_D   1
/// focus follows mouse?
#define MP_FOCUS_FOLLOWSMOUSE_D    1l
/// dockable menu bars?
#define   MP_DOCKABLE_MENUBARS_D   1l
/// dockable tool bars?
#define   MP_DOCKABLE_TOOLBARS_D   1l
/// flat tool bars?
#define   MP_FLAT_TOOLBARS_D      1l

/// the user's M directory
#define   MP_USERDIR_D         M_EMPTYSTRING

/// the acceptance status of the license
#define MP_LICENSE_ACCEPTED_D   0l
// Unix-only entries
#ifdef OS_UNIX
/// path list for M's directory
#  define   MP_PATHLIST_D M_PREFIX ":/usr/local:/usr/:/opt:/opt/local:/usr/opt:/usr/local/opt"
/// the complete path to the glocal M directory
#  define   MP_GLOBALDIR_D      M_BASEDIR
/// the name of M's root directory
#  define   MP_ROOTDIRNAME_D   "Mahogany"
/// the name of the M directory
#  define   MP_USER_MDIR_D         ".M"
/// the path where to find .afm files
#  define   MP_AFMPATH_D M_BASEDIR "/afm:/usr/share:/usr/lib:/usr/local/share:/usr/local/lib:/opt/ghostscript:/opt/enscript"
/// the path to the /etc directories (configuration files)
#  define   MP_ETCPATH_D "/etc:/usr/etc:/usr/local/etc:/opt/etc:/usr/share/etc:/usr/local/share/etc"
/// the path to the m directory
#  define   MP_PREFIXPATH_D "/usr:/usr/local:/opt:/usr/share:/usr/local/share:/opt/share:/usr/local/opt:/usr/local/opt/share:/tmp"
#else // !Unix
/// the complete path to the glocal M directory
#  define   MP_GLOBALDIR_D  ""
#endif // Unix/!Unix

/// the directory containing the help files
#define MP_HELPDIR_D ""

/// Which encryption algorithm to use : 0 = simple builtin, 1 = twofish
#define MP_CRYPTALGO_D   0L
/// DoesTwoFish work? (-1 unknown, 0 no, 1 yes)
#define MP_CRYPT_TWOFISH_OK_D -1
/// some test data
#define MP_CRYPT_TESTDATA_D ""
/// the locale for translation to national languages
#define   MP_LOCALE_D               M_EMPTYSTRING
/// the default character set
#define MP_CHARSET_D               "ISO-8859-1"
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
/// the key for identity redirection
#define MP_PROFILE_IDENTITY_D  ""
/// the name of the mailcap file
#define   MP_MAILCAP_D         "mailcap"
/// the name of the mime types file
#define   MP_MIMETYPES_D         "mime.types"
/// the strftime() format for dates
#define   MP_DATE_FMT_D         "%c"
/// display all dates as GMT?
#define   MP_DATE_GMT_D         0l
/// show console window
#define   MP_SHOWCONSOLE_D      1
/// open any folders at all on startup?
#define   MP_DONTOPENSTARTUP_D   0l
/// names of folders to open at startup
#define   MP_OPENFOLDERS_D      M_EMPTYSTRING
/// reopen the last opened folder in the main frame
#define   MP_REOPENLASTFOLDER_D 1
/// name of folder to open in mainframe
#define   MP_MAINFOLDER_D        ""
/// path for Python
#define   MP_PYTHONPATH_D         M_EMPTYSTRING
#ifdef OS_WIN
// under Windows, the setup program will create the appropriate registry key
#define   MP_USEPYTHON_D         0l
#else
/// is Python enabled (this is a run-time option)?
#define   MP_USEPYTHON_D         1
#endif
/// start-up script to run
#define     MP_STARTUPSCRIPT_D   ""
/// show splash screen on startup?
#define     MP_SHOWSPLASH_D      1
/// how long should splash screen stay (0 disables timeout)?
#define MP_SPLASHDELAY_D        5
/// how often should we autosave the profile settings (0 to disable)?
#define   MP_AUTOSAVEDELAY_D       60
/// how often should we check for incoming mail (secs, 0 to disable)?
#define   MP_POLLINCOMINGDELAY_D       300
/// collect all new mail at startup?
#define   MP_COLLECTATSTARTUP_D 0l
/// ask user if he really wants to exit?
#define   MP_CONFIRMEXIT_D      1l
/// open folders when they're clicked (otherwise - double clicked)
#define   MP_OPEN_ON_CLICK_D     0l
/// show all folders (even hidden ones) in the folder tree?
#define   MP_SHOW_HIDDEN_FOLDERS_D 0l
/// create .profile files?
#define   MP_CREATE_PROFILES_D   0l
/// umask setting for normal files
#define   MP_UMASK_D               022
/// automatically show first message in folderview?
#define   MP_AUTOSHOW_FIRSTMESSAGE_D 0l
/// automatically show first unread message in folderview?
#define   MP_AUTOSHOW_FIRSTUNREADMESSAGE_D 1l
/// open messages when they're clicked (otherwise - double clicked)
#define   MP_PREVIEW_ON_SELECT_D     1l
/// program used to convert image files?
#define   MP_CONVERTPROGRAM_D      "convert %s -compress None %s"
/// list of modules to load at startup
#define MP_MODULES_D   "Filters"

/// the format string for the folder tree display
#define MP_FOLDERSTATUS_TREE_D _("%f (%t, %u)")
/// the format string for status bar folder status display
#define MP_FOLDERSTATUS_STATBAR_D _("%f (%t messages, %u unread, %n new)")
/// the format string for title bar folder status display
#define MP_FOLDERSTATUS_TITLEBAR_D _("%f (%u unread, %n new)")

/**@name Printer settings */
//@{
/// Command
#define MP_PRINT_COMMAND_D "lpr"
/// Options
#define MP_PRINT_OPTIONS_D   ""
/// Orientation
#define MP_PRINT_ORIENTATION_D 1  /* wxPORTRAIT */
/// print mode
#define MP_PRINT_MODE_D 0  /* PS_PRINTER */
/// paper name
#define MP_PRINT_PAPER_D 0L
/// paper name
#define MP_PRINT_FILE_D "mahogany.ps"
/// print in colour?
#define MP_PRINT_COLOUR_D 1
/// top margin
#define MP_PRINT_TOPMARGIN_X_D    0l
/// left margin
#define MP_PRINT_TOPMARGIN_Y_D   0l
/// bottom margin
#define MP_PRINT_BOTTOMMARGIN_X_D   0l
/// right margin
#define MP_PRINT_BOTTOMMARGIN_Y_D 0l
/// zoom level in print preview
#define MP_PRINT_PREVIEWZOOM_D   100l
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
/// the current user identity
#define   MP_CURRENT_IDENTITY_D M_EMPTYSTRING
/// the user's qualification (see M_USERLEVEL_XXX)
#define   MP_USERLEVEL_D        M_USERLEVEL_NOVICE
/// the username/login
#define   MP_USERNAME_D         M_EMPTYSTRING
/// the user's hostname
#define   MP_HOSTNAME_D         M_EMPTYSTRING
/// Add this hostname for addresses without hostname?
#define   MP_ADD_DEFAULT_HOSTNAME_D 1L
/// the default POP3 host
#define   MP_POPHOST_D          "pop"
/// the default IMAP4 host
#define   MP_IMAPHOST_D          "imap"
/// the mail host
#define   MP_SMTPHOST_D         "mail"
/// use the specified sender value or guess it automatically?
#define   MP_GUESS_SENDER_D  1l
/// the smtp sender value
#define   MP_SENDER_D           ""
/// the mail host user-id
#define   MP_SMTPHOST_LOGIN_D   ""
/// the mail host password
#define   MP_SMTPHOST_PASSWORD_D   ""
/// use SSL?
#define   MP_SMTPHOST_USE_SSL_D   0l
/// check ssl-certs for SMTP connections?
#define   MP_SMTPHOST_USE_SSL_UNSIGNED_D   0l
/// sendmail command  FIXME - should be detected by configure
#ifdef OS_LINUX
#  define MP_SENDMAILCMD_D "/usr/sbin/sendmail -t"
#else
#  define MP_SENDMAILCMD_D "/usr/lib/sendmail -t"
#endif
/// use sendmail?
#ifdef OS_UNIX
#  define MP_USE_SENDMAIL_D 1l
#else
#  define MP_USE_SENDMAIL_D 0l
#endif
/// tyhe mail server fallback
#define   MP_SMTPHOST_FB        "localhost"
/// the news server
#define   MP_NNTPHOST_D      "news"
/// the news server user-id
#define   MP_NNTPHOST_LOGIN_D      ""
/// the news server password
#define   MP_NNTPHOST_PASSWORD_D      ""
/// use SSL?
#define   MP_NNTPHOST_USE_SSL_D   0l
/// check ssl-certs for NNTP connections?
#define   MP_NNTPHOST_USE_SSL_UNSIGNED_D   0l
/// the beacon host to test for net connection
#define   MP_BEACONHOST_D      ""
/// does Mahogany control dial-up networking?
#define MP_DIALUP_SUPPORT_D   0L

/// should we attach vCard to outgoing messages?
#define MP_USEVCARD_D 0l
/// the vCard to use
#define MP_VCARD_D "vcard.vcf"

#if defined(OS_WIN)
/// the RAS connection to use
#define MP_NET_CONNECTION_D "RasConnection"
#elif defined(OS_UNIX)
/// the command to go online
#define MP_NET_ON_COMMAND_D   "wvdial"
/// the command to go offline
#define MP_NET_OFF_COMMAND_D   "killall pppd"
#endif // platform

/// the news server fallback
#define   MP_NNTPHOST_FB        "news"
/// (the username for returned mail) E-mail address
#define   MP_FROM_ADDRESS_D      M_EMPTYSTRING
/// Reply address
#define   MP_REPLY_ADDRESS_D      M_EMPTYSTRING
/// show CC field in message composition?
#define   MP_SHOWCC_D         1
/// show BCC field in message composition?
#define   MP_SHOWBCC_D         1
/// login for mailbox
#define   MP_FOLDER_LOGIN_D      M_EMPTYSTRING
/// passwor for mailbox
#define   MP_FOLDER_PASSWORD_D      M_EMPTYSTRING
/// log level
#define   MP_LOGLEVEL_D         0l
/// threshold for displaying mailfolder progress dialog
#define   MP_FOLDERPROGRESS_THRESHOLD_D 20L
/// threshold for displaying message retrieval progress dialog (kbytes)
#define   MP_MESSAGEPROGRESS_THRESHOLD_SIZE_D  40l
/// threshold for displaying message retrieval progress dialog (seconds)
#define   MP_MESSAGEPROGRESS_THRESHOLD_TIME_D  1l
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
/// detect and remove signature when replying?
#define MP_REPLY_DETECT_SIG_D   1
#if wxUSE_REGEX
/// a regex to detect signature
#define MP_REPLY_SIG_SEPARATOR_D "((_____*)|(-- ?))"
#endif
/// include the original message in the reply [no,ask,yes]
#define MP_REPLY_QUOTE_ORIG_D M_ACTION_ALWAYS

/// use XFace in composition?
#define   MP_COMPOSE_USE_XFACE_D   1
/// Name from where to read XFace
#define   MP_COMPOSE_XFACE_FILE_D   "$HOME/.xface"
/// the folder type for a mailbox
#define   MP_FOLDER_TYPE_D         (long)(0x00ff)  // MF_ILLEGAL
/// should we try to create the folder before opening it?
#define   MP_FOLDER_TRY_CREATE_D 0L
/// the folder icon for a mailbox (see icon functions in FolderType.h)
#define   MP_FOLDER_ICON_D         (long)-1        // no special icon
/// the position of the folder in the tree
#define MP_FOLDER_TREEINDEX_D      (long)-1        // see enum MFolderIndex
/// Where to store all new mail
#define MP_NEWMAIL_FOLDER_D      "" // obsolete
/// Which folder to use as Outbox
#define MP_OUTBOX_NAME_D         ""
/// Use Outbox?
#define MP_USE_OUTBOX_D            0l
/// Name of Trash folder?
#define MP_TRASH_FOLDER_D      ""
/// Use a trash folder?
#define MP_USE_TRASH_FOLDER_D   1l
/// the filename for a mailbox
#define   MP_FOLDER_PATH_D      ((const char *)NULL) // don't change this!
/// comment
#define   MP_FOLDER_COMMENT_D      M_EMPTYSTRING
/// update interval for folders in seconds
#define   MP_UPDATEINTERVAL_D      180
/// close of folders delayed by
#define MP_FOLDER_CLOSE_DELAY_D    30
/// do automatic word wrap?
#define MP_AUTOMATIC_WORDWRAP_D   1l
/// wrapmargin for composition (set to -1 to disable it)
#define   MP_WRAPMARGIN_D      75
/// do automatic word wrap in message view?
#define MP_VIEW_AUTOMATIC_WORDWRAP_D   1l
/// wrapmargin for message view (set to -1 to disable it)
#define   MP_VIEW_WRAPMARGIN_D      100
/// show TEXT/PLAIN as inlined text?
#define   MP_PLAIN_IS_TEXT_D      1l
/// show MESSAGE/RFC822 as text?
#define   MP_RFC822_IS_TEXT_D      0l
/// prefix for subject in replies
#define   MP_REPLY_PREFIX_D      "Re: "
/// collapse reply prefixes? 0=no, 1=replace "Re"s with one, 2=use reply level
#define   MP_REPLY_COLLAPSE_PREFIX_D 2l
/// prefix for text in replies
#define   MP_REPLY_MSGPREFIX_D      "> "
/// prepend the initials of the sender to thereply prefix?
#define   MP_REPLY_MSGPREFIX_FROM_SENDER_D 0l
/// quote the empty lines when replying?
#define   MP_REPLY_QUOTE_EMPTY_D      1l
/// set reply string from To: field?
#define MP_SET_REPLY_FROM_TO_D   0l
/// prefix for subject in forwards
#define   MP_FORWARD_PREFIX_D      _("Forwarded message: ")
/// show XFaces?
#define   MP_SHOW_XFACES_D      1
/// show graphics inline
#define   MP_INLINE_GFX_D       1
/// limit size for inline graphics in Kb (0 for no limit)
#define   MP_INLINE_GFX_SIZE_D  40
/// which viewer to use in the message view?
#define MP_MSGVIEW_VIEWER_D "LayoutViewer"
/// which headers to show in the message view?
#define   MP_MSGVIEW_HEADERS_D     \
          "Date:" \
          "From:" \
          "Subject:" \
          "Newsgroups:" \
          "To:" \
          "Cc:" \
          "Bcc:" \
          "Reply-To:"
/// all headers we know about
#define   MP_MSGVIEW_ALL_HEADERS_D \
          MP_MSGVIEW_HEADERS_D \
          "Approved:" \
          "Arrival-Date:" \
          "Auto-Submitted:" \
          "Bounces-to:" \
          "Charset:" \
          "Content-Description:" \
          "Content-Disposition:" \
          "Content-ID:" \
          "Content-Length:" \
          "Content-Transfer-Encoding:" \
          "Content-Type:" \
          "Delivered-To:" \
          "Diagnostic-Code:" \
          "Disclaimer:" \
          "Errors-To:" \
          "Final-Recipient:" \
          "Full-name:" \
          "Importance:" \
          "In-Reply-To:" \
          "Keywords:" \
          "Last-Attempt-Date:" \
          "Lines:" \
          "List-Archive:" \
          "List-Help:" \
          "List-Id:" \
          "List-Post:" \
          "List-Subscribe:" \
          "List-Unsubscribe:" \
          "Mail-Copies-To:" \
          "Mail-Followup-To:" \
          "Mailing-List:" \
          "Message-Id:" \
          "MIME-Version:" \
          "NNTP-Posting-Host:" \
          "Note:" \
          "Old-Return-Path:" \
          "Organization:" \
          "Path:" \
          "Precedence:" \
          "Priority:" \
          "Received:" \
          "Received-From:" \
          "Received-From-MTA:" \
          "References:" \
          "Remote-MTA:" \
          "Reporting-MTA:" \
          "Resent-Date:" \
          "Resent-From:" \
          "Resent-Message-ID:" \
          "Resent-Sender:" \
          "Resent-To:" \
          "Return-Path:" \
          "Return-Receipt-To:" \
          "Sender:" \
          "Sent:" \
          "State-Changed-By:" \
          "State-Changed-From-To:" \
          "State-Changed-When:" \
          "State-Changed-Why:" \
          "Status:" \
          "User-Agent:" \
          "Version:" \
          "Xref:" \
          "X-Accept-Language:" \
          "X-Amazon-Track:" \
          "X-Article-Creation-Date:" \
          "X-Attachments:" \
          "X-Authentication-Warning:" \
          "X-BeenThere:" \
          "X-Disclaimer:" \
          "X-Dispatcher:" \
          "X-eGroups-From:" \
          "X-eGroups-Return:" \
          "X-Envelope-To:" \
          "X-Expiry-Date:" \
          "X-Face:" \
          "X-HTTP-Proxy:" \
          "X-HTTP-User-Agent:" \
          "X-Image-URL:" \
          "X-IMAP:" \
          "X-Juno-Att:" \
          "X-Juno-Line-Breaks:" \
          "X-Juno-RefParts:" \
          "X-Keywords:" \
          "X-Loop:" \
          "X-LSV-ListID:" \
          "X-Mailer:" \
          "X-Mailing-List:" \
          "X-Mailman-Version:" \
          "X-MIME-Autoconverted:" \
          "X-MIMEOLE:" \
          "X-MIMETrack:" \
          "X-Mozilla-Status2:" \
          "X-MSMail-Priority:" \
          "X-MyDeja-Info:" \
          "X-Newsreader:" \
          "X-Original-Date:" \
          "X-Originating-IP:" \
          "X-Priority:" \
          "X-Sender:" \
          "X-Species:" \
          "X-Status:" \
          "X-Trace:" \
          "X-UID:" \
          "X-UIDL:" \
          "X-URL:" \
          "X-WM-Posted-At:"
/// should we auto detect the message encoding?
#define  MP_MSGVIEW_AUTO_ENCODING_D   1
/// the default encoding for the viewer/composer
#define  MP_MSGVIEW_DEFAULT_ENCODING_D (long)wxFONTENCODING_DEFAULT
/// the type of the last created folder
#define   MP_LAST_CREATED_FOLDER_TYPE_D  (int)File
/// the filter program to apply (OBSOLETE)
#define MP_FILTER_RULE_D   ""
/// the filters to use for this folder
#define MP_FOLDER_FILTERS_D   ""
/// the default folder file format
#define MP_FOLDER_FILE_DRIVER_D 0L
/* format: mbx:unix:mmdf:tenex defined in MailFolderCC.cpp */
/// the background colour for the folder tree
#define MP_FOLDER_BGCOLOUR_D ""
/**@name  Font settings for message view */
//@{
/// message view title
#define   MP_MVIEW_TITLE_FMT_D   _("from $from about \"$subject\"")
/// which font to use
#define   MP_MVIEW_FONT_D         6L
/// which font size
#define   MP_MVIEW_FONT_SIZE_D         DEFAULT_FONT_SIZE
// which foreground colour for the font
#define   MP_MVIEW_FGCOLOUR_D      "black"
// which background colour for the font
#define   MP_MVIEW_BGCOLOUR_D      "white"
// which colour for URLS
#define   MP_MVIEW_URLCOLOUR_D     "blue"
// colour for attachment labels
#define   MP_MVIEW_ATTCOLOUR_D     "dark olive green"

// colorize quoted text?
#define   MP_MVIEW_QUOTED_COLOURIZE_D 1L
// cycle colours?
#define   MP_MVIEW_QUOTED_CYCLE_COLOURS_D  0L
// which colour for quoted text
#define   MP_MVIEW_QUOTED_COLOUR1_D     "gray"
// which colour for quoted text, seconds level
#define   MP_MVIEW_QUOTED_COLOUR2_D     "gray"
// which colour for quoted text, seconds level
#define   MP_MVIEW_QUOTED_COLOUR3_D     "gray"
/// the maximum number of whitespaces prepending >
#define   MP_MVIEW_QUOTED_MAXWHITESPACE_D  2L
/// the maximum number of A-Z prepending >
#define   MP_MVIEW_QUOTED_MAXALPHA_D    3L
/// the colour for header names in the message view
#define   MP_MVIEW_HEADER_NAMES_COLOUR_D  "blue"
/// the colour for header values in the message view
#define   MP_MVIEW_HEADER_VALUES_COLOUR_D "black"
//@}
/**@name  Font settings for message view */
//@{
/// which font to use
#define   MP_FVIEW_FONT_D         4L
/// which font size
#define   MP_FVIEW_FONT_SIZE_D         DEFAULT_FONT_SIZE
/// don't show full e-mail, only sender's name
#define   MP_FVIEW_NAMES_ONLY_D         0L
/// which foreground colour for the font
#define   MP_FVIEW_FGCOLOUR_D      "black"
/// which background colour for the font
#define   MP_FVIEW_BGCOLOUR_D      "white"
/// colour for deleted messages
#define   MP_FVIEW_DELETEDCOLOUR_D      "grey"
/// colour for new messages
#define   MP_FVIEW_NEWCOLOUR_D      "orange"
/// colour for recent messages
#define   MP_FVIEW_RECENTCOLOUR_D      "gold"
/// colour for unread messages
#define   MP_FVIEW_UNREADCOLOUR_D      "blue"
/// colour for flagged messages
#define   MP_FVIEW_FLAGGEDCOLOUR_D      "purple"
/// update the folder view status bar to show the msg info?
#define   MP_FVIEW_STATUS_UPDATE_D 0L
/// folder view status bar string
#define   MP_FVIEW_STATUS_FMT_D _("Date: $date, Subject: $subject, From: $from")
//@}
/**@name  Font settings for folder tree */
//@{
/// format for the folder tree entries
#define MP_FTREE_FORMAT_D " (%t, %u)"
/// reflect the folder status in its parent?
#define MP_FTREE_PROPAGATE_D 1L
//@}
/**@name Font settings for compose view */
//@{
/// which font to use
#define   MP_CVIEW_FONT_D         6L
/// which font size
#define   MP_CVIEW_FONT_SIZE_D    DEFAULT_FONT_SIZE
// which foreground colour for the font
#define   MP_CVIEW_FGCOLOUR_D      "black"
// which background colour for the font
#define   MP_CVIEW_BGCOLOUR_D      "white"
//@}
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS_D      1
/// sort criterium for folder listing (== MSO_DATE)
#define MP_MSGS_SORTBY_D         2l
/// re-sort messages on status change?
#define MP_MSGS_RESORT_ON_CHANGE_D 0l
/// use threading
#define MP_MSGS_USE_THREADING_D  1l

#if defined(EXPERIMENTAL_JWZ_THREADING)

/// Gather messages with same subject in one thread
#define MP_MSGS_GATHER_SUBJECTS_D 1l
/// break thread when subject changes
#define MP_MSGS_BREAK_THREAD_D 1l
/// Indent messages when common ancestor is missing
#define MP_MSGS_INDENT_IF_DUMMY_D 0l

#if wxUSE_REGEX
#  define MP_MSGS_SIMPLIFYING_REGEX_D "^ *(R[Ee](\\[[0-9]+\\])?: +)*(\\[[^][]+\\] +)?(R[Ee](\\[[0-9]+\\])?: +)*"
#  define MP_MSGS_REPLACEMENT_STRING_D "\\3"
#else // wxUSE_REGEX
   /// Remove list prefix when comparing message's subject to gather them
#  define MP_MSGS_REMOVE_LIST_PREFIX_GATHERING_D 1l
   /// Remove list prefix when comparing message's subject to break threads
#  define MP_MSGS_REMOVE_LIST_PREFIX_BREAKING_D 1l
#endif // wxUSE_REGEX

#endif // EXPERIMENTAL_JWZ_THREADING

// how to show the size (MessageSizeShow enum value)
#define MP_FVIEW_SIZE_FORMAT_D  0l
/// search criterium for searching in folders
#define MP_MSGS_SEARCH_CRIT_D   0l
/// search argument
#define MP_MSGS_SEARCH_ARG_D   ""
/// open URLs with
#ifdef  OS_UNIX
#  define   MP_BROWSER_D         "netscape"
#  define   MP_BROWSER_ISNS_D    1
#  define   MP_BROWSER_INNW_D    1
#else  // under Windows, we know better...
#  define   MP_BROWSER_D         M_EMPTYSTRING
#  define   MP_BROWSER_ISNS_D    0l
#  define   MP_BROWSER_INNW_D    1l
#endif // Unix/Win

/// external editor to use for message composition (use %s for filename)
#ifdef OS_UNIX
#  define   MP_EXTERNALEDITOR_D  "gvim %s"
#else
#  define   MP_EXTERNALEDITOR_D  "notepad %s"
#endif // Unix/Win

/// start external editor automatically?
#define MP_ALWAYS_USE_EXTERNALEDITOR_D    0l

/// command to execute when new mail arrives
#ifdef  OS_UNIX
#   define   MP_USE_NEWMAILCOMMAND_D   1
#   define   MP_NEWMAILCOMMAND_D   "/usr/bin/play "M_BASEDIR"/newmail.wav"
#else
#   define   MP_USE_NEWMAILCOMMAND_D   0l
#   define   MP_NEWMAILCOMMAND_D   M_EMPTYSTRING
#endif
/// show new mail messages?
#define   MP_SHOW_NEWMAILMSG_D      1
/// show detailed info about how many new mail messages?
#define   MP_SHOW_NEWMAILINFO_D      10
/// keep copies of outgoing mail?
#define   MP_USEOUTGOINGFOLDER_D  1
/// write outgoing mail to folder:
#define   MP_OUTGOINGFOLDER_D  "" // obsolete
/// Show all message headers?
#define   MP_SHOWHEADERS_D         0l
/// Autocollect email addresses?
#define   MP_AUTOCOLLECT_D     M_ACTION_ALWAYS
/// Name of the address books for autocollected addresses
#define   MP_AUTOCOLLECT_ADB_D    "autocollect.adb"
/// Autocollect entries with names only?
#define   MP_AUTOCOLLECT_NAMED_D 0l
#ifdef OS_UNIX
/// Default name for the SSL library
#define MP_SSL_DLL_SSL_D   "libssl.so.0"
/// Default name for the SSL/crypto library
#define MP_SSL_DLL_CRYPTO_D "libcrypto.so.0"
#else
/// Default name for the SSL library
#define MP_SSL_DLL_SSL_D   "libssl.dll"
/// Default name for the SSL/crypto library
#define MP_SSL_DLL_CRYPTO_D "libcrypto.dll"
#endif

/**@name message view settings */
//@{
/// which font to use
#define   MP_MV_FONT_FAMILY_D   6L
/// which font size
#define   MP_MV_FONT_SIZE_D     DEFAULT_FONT_SIZE
/// support efax style incoming faxes
#define MP_INCFAX_SUPPORT_D      1L
/// domains from which to support faxes, semicolon delimited
#define MP_INCFAX_DOMAINS_D      "efax.com"
//@}
/// Use substrings in address expansion?
#define   MP_ADB_SUBSTRINGEXPANSION_D 1l

/// replace "From" address with "To" in messages from oneself?
#define MP_FVIEW_FROM_REPLACE_D 0L
/// the ':' separated list of addresses which are "from oneself"
#define MP_FROM_REPLACE_ADDRESSES_D ""

/** @name maximal amounts of data to retrieve from remote servers */
//@{
/// ask confirmation before retrieveing messages bigger than this (in Kb)
#define MP_MAX_MESSAGE_SIZE_D 100l
/// ask confirmation before retrieveing more headers than this
#define MP_MAX_HEADERS_NUM_D  100l
/// never download more than that many messages
#define MP_MAX_HEADERS_NUM_HARD_D 0l
//@}
/// setting this prevents the filters from expuning the msgs automatically
#define MP_SAFE_FILTERS_D 0l
/** @name timeout values for c-client mail library */
//@{
/// IMAP lookahead value
#define MP_IMAP_LOOKAHEAD_D 0l
/// TCP/IP open timeout in seconds.
#define MP_TCP_OPENTIMEOUT_D      10l
#if 0 // obsolete
/// TCP/IP read timeout in seconds.
#define MP_TCP_READTIMEOUT_D       30l
/// TCP/IP write timeout in seconds.
#define  MP_TCP_WRITETIMEOUT_D    30l
/// TCP/IP close timeout in seconds.
#define  MP_TCP_CLOSETIMEOUT_D 60l
#endif // 0
/// rsh connection timeout in seconds.
#define MP_TCP_RSHTIMEOUT_D 0l
/// ssh connection timeout in seconds.
#define MP_TCP_SSHTIMEOUT_D 0l
/// the path to rsh
#define MP_RSH_PATH_D "rsh"
/// the path to ssh
#define MP_SSH_PATH_D "ssh"
//@}

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
#define   MP_FLC_STATUSCOL_D  0l
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
/// Do we remote synchronise configuration settings?
#define MP_SYNC_REMOTE_D   0L
/// IMAP folder to sync to
#define MP_SYNC_FOLDER_D   "MahoganySharedConfig"
/// our last sync date
#define MP_SYNC_DATE_D      0L
/// sync filters?
#define MP_SYNC_FILTERS_D   0L
/// sync filters?
#define MP_SYNC_IDS_D   0L
/// sync folders?
#define MP_SYNC_FOLDERS_D   0L
/// sync folder tree
#define MP_SYNC_FOLDERGROUP_D   ""

/** @name sending */
//@{
/// confirm sending the message?
#define MP_CONFIRM_SEND_D 0L
/// preview the message being sent?
#define MP_PREVIEW_SEND_D 0L
//@}

/** @name away mode */
//@{
/// automatically enter the away mode after that many minutes (0 to disable)
#define MP_AWAY_AUTO_ENTER_D 0L
/// automaticlly exit from the away mode when the user chooses menu command
#define MP_AWAY_AUTO_EXIT_D 1L
/// keep the value of away mode for the next run (otherwise always reset it)
#define MP_AWAY_REMEMBER_D 0L
/// the saved value of away status (only written if MP_AWAY_REMEMBER is set)
#define MP_AWAY_STATUS_D 0L
//@}

//@}

/** @name other defines used in M */
//@{
/// title for main window
#define   M_TOPLEVELFRAME_TITLE   _("Copyright (C) 1997-2001 The Mahogany Developers Team")
/// do we want variable expansion for profiles?
#define   M_PROFILE_VAREXPAND      1
/// c-client lib needs a char buffer to write header data
#define   CC_HEADERBUFLEN         1000000
/// file name prefix for lock files
#define   LOCK_PREFIX         "M-lock"
/// postfix for News outbox
#define M_NEWSOUTBOX_POSTFIX _("_News")
//@}

#endif // MDEFAULTS_H
