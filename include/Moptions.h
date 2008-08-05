///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Moptions.h: Mahogany options data
// Purpose:     declares all MP_XXX_OPT (symbolic option identifiers), MP_XXX
//              (the option names in the profile) and MP_XXX_D (their default
//              values)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.08.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// all known options
// ----------------------------------------------------------------------------

class MOption;

extern const MOption MP_PROFILE_TYPE;
extern const MOption MP_CONFIG_SOURCE_TYPE;
extern const MOption MP_CONFIG_SOURCE_PRIO;
extern const MOption MP_VERSION;
extern const MOption MP_FIRSTRUN;
extern const MOption MP_RECORDDEFAULTS;
extern const MOption MP_EXPAND_ENV_VARS;
extern const MOption MP_XPOS;
extern const MOption MP_YPOS;
extern const MOption MP_WIDTH;
extern const MOption MP_HEIGHT;
extern const MOption MP_ICONISED;
extern const MOption MP_MAXIMISED;
extern const MOption MP_SHOW_TOOLBAR;
extern const MOption MP_SHOW_STATUSBAR;
extern const MOption MP_SHOW_FULLSCREEN;
extern const MOption MP_SHOWLOG;
extern const MOption MP_LOGFILE;
extern const MOption MP_DEBUG_CCLIENT;
extern const MOption MP_SHOWADBEDITOR;
extern const MOption MP_SHOWTIPS;
extern const MOption MP_LASTTIP;
extern const MOption MP_HELPBROWSER_KIND;
extern const MOption MP_HELPBROWSER;
extern const MOption MP_HELPBROWSER_ISNS;
extern const MOption MP_HELPFRAME_WIDTH;
extern const MOption MP_HELPFRAME_HEIGHT;
extern const MOption MP_HELPFRAME_XPOS;
extern const MOption MP_HELPFRAME_YPOS;
extern const MOption MP_MBOXDIR;
extern const MOption MP_NEWS_SPOOL_DIR;
extern const MOption MP_TIFF2PS;
extern const MOption MP_TMPGFXFORMAT;
extern const MOption MP_EXPAND_TREECTRL;
extern const MOption MP_FOCUS_FOLLOWSMOUSE;
extern const MOption MP_DOCKABLE_MENUBARS;
extern const MOption MP_DOCKABLE_TOOLBARS;
extern const MOption MP_FLAT_TOOLBARS;
extern const MOption MP_USERDIR;
extern const MOption MP_LICENSE_ACCEPTED;

extern const MOption MP_GLOBALDIR;
extern const MOption MP_RUNONEONLY;

extern const MOption MP_TBARIMAGES;

#ifdef OS_UNIX
extern const MOption MP_AFMPATH;
#endif // OS_UNIX

extern const MOption MP_HELPDIR;
extern const MOption MP_CRYPTALGO;
extern const MOption MP_CRYPT_TWOFISH_OK;
extern const MOption MP_CRYPT_TESTDATA;
extern const MOption MP_LOCALE;
extern const MOption MP_CHARSET;
extern const MOption MP_ICON_MFRAME;
extern const MOption MP_ICON_MAINFRAME;
extern const MOption MP_ICONPATH;
extern const MOption MP_PROFILE_PATH;
extern const MOption MP_PROFILE_EXTENSION;
extern const MOption MP_PROFILE_IDENTITY;
extern const MOption MP_MAILCAP;
extern const MOption MP_MIMETYPES;
extern const MOption MP_DATE_FMT;
extern const MOption MP_DATE_GMT;
extern const MOption MP_SHOWCONSOLE;
extern const MOption MP_DONTOPENSTARTUP;
extern const MOption MP_OPENFOLDERS;
extern const MOption MP_REOPENLASTFOLDER;
extern const MOption MP_MAINFOLDER;
extern const MOption MP_PYTHONPATH;
extern const MOption MP_PYTHONDLL;
extern const MOption MP_USEPYTHON;
extern const MOption MP_PYTHONMODULE_TO_LOAD;
extern const MOption MP_SHOWSPLASH;
extern const MOption MP_SPLASHDELAY;
extern const MOption MP_AUTOSAVEDELAY;
extern const MOption MP_POLLINCOMINGDELAY;
extern const MOption MP_POLL_OPENED_ONLY;
extern const MOption MP_COLLECTATSTARTUP;
extern const MOption MP_CONFIRMEXIT;
extern const MOption MP_OPEN_ON_CLICK;
extern const MOption MP_SHOW_HIDDEN_FOLDERS;
extern const MOption MP_CREATE_PROFILES;
extern const MOption MP_UMASK;
extern const MOption MP_LASTSELECTED_MESSAGE;
extern const MOption MP_AUTOSHOW_LASTSELECTED;
extern const MOption MP_AUTOSHOW_FIRSTMESSAGE;
extern const MOption MP_AUTOSHOW_FIRSTUNREADMESSAGE;
extern const MOption MP_PREVIEW_ON_SELECT;
extern const MOption MP_AUTOSHOW_SELECT;
extern const MOption MP_CONVERTPROGRAM;
extern const MOption MP_MODULES;
extern const MOption MP_MODULES_DONT_LOAD;
extern const MOption MP_COMPOSETEMPLATEPATH_USER;
extern const MOption MP_COMPOSETEMPLATEPATH_GLOBAL;

extern const MOption MP_FOLDERSTATUS_TREE;
extern const MOption MP_FOLDERSTATUS_STATBAR;
extern const MOption MP_FOLDERSTATUS_TITLEBAR;
extern const MOption MP_PRINT_COMMAND;
extern const MOption MP_PRINT_OPTIONS;
extern const MOption MP_PRINT_ORIENTATION;
extern const MOption MP_PRINT_MODE;
extern const MOption MP_PRINT_PAPER;
extern const MOption MP_PRINT_FILE;
extern const MOption MP_PRINT_COLOUR;
extern const MOption MP_PRINT_TOPMARGIN_X;
extern const MOption MP_PRINT_TOPMARGIN_Y;
extern const MOption MP_PRINT_BOTTOMMARGIN_X;
extern const MOption MP_PRINT_BOTTOMMARGIN_Y;
extern const MOption MP_PRINT_PREVIEWZOOM;
extern const MOption MP_BBDB_GENERATEUNIQUENAMES;
extern const MOption MP_BBDB_IGNOREANONYMOUS;
extern const MOption MP_BBDB_ANONYMOUS;
extern const MOption MP_BBDB_SAVEONEXIT;
extern const MOption MP_PERSONALNAME;
extern const MOption MP_ORGANIZATION;
extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_USERLEVEL;
extern const MOption MP_USERNAME;
extern const MOption MP_HOSTNAME;
extern const MOption MP_ADD_DEFAULT_HOSTNAME;
extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_REPLY_ADDRESS;
extern const MOption MP_POPHOST;
extern const MOption MP_POP_NO_AUTH;
extern const MOption MP_IMAPHOST;
extern const MOption MP_USE_SSL;
extern const MOption MP_USE_SSL_UNSIGNED;
extern const MOption MP_SMTPHOST;
extern const MOption MP_GUESS_SENDER;
extern const MOption MP_SENDER;
extern const MOption MP_SMTPHOST_LOGIN;
extern const MOption MP_SMTPHOST_PASSWORD;
extern const MOption MP_SMTPHOST_USE_SSL;
extern const MOption MP_SMTPHOST_USE_SSL_UNSIGNED;
extern const MOption MP_SMTP_USE_8BIT;
#ifdef USE_OWN_CCLIENT
extern const MOption MP_SMTP_DISABLED_AUTHS;
#endif // USE_OWN_CCLIENT

#ifdef OS_UNIX
extern const MOption MP_SENDMAILCMD;
extern const MOption MP_USE_SENDMAIL;
#endif // Unix

extern const MOption MP_NNTPHOST;
extern const MOption MP_NNTPHOST_LOGIN;
extern const MOption MP_NNTPHOST_PASSWORD;
extern const MOption MP_NNTPHOST_USE_SSL;
extern const MOption MP_NNTPHOST_USE_SSL_UNSIGNED;
extern const MOption MP_BEACONHOST;
extern const MOption MP_SET_REPLY_FROM_TO;
extern const MOption MP_SET_REPLY_STD_NAME;
extern const MOption MP_USEVCARD;
extern const MOption MP_VCARD;

extern const MOption MP_USE_FOLDER_CREATE_WIZARD;

#ifdef USE_DIALUP

extern const MOption MP_DIALUP_SUPPORT;
#ifdef OS_WIN
extern const MOption MP_NET_CONNECTION;
#else // !Windows
extern const MOption MP_NET_ON_COMMAND;
extern const MOption MP_NET_OFF_COMMAND;
#endif // Windows/!Windows

#endif // USE_DIALUP

extern const MOption MP_FOLDER_LOGIN;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_LOGLEVEL;
extern const MOption MP_SHOWBUSY_DURING_SORT;
extern const MOption MP_FOLDERPROGRESS_THRESHOLD;
extern const MOption MP_MESSAGEPROGRESS_THRESHOLD_SIZE;
extern const MOption MP_MESSAGEPROGRESS_THRESHOLD_TIME;
extern const MOption MP_DEFAULT_SAVE_PATH;
extern const MOption MP_DEFAULT_SAVE_FILENAME;
extern const MOption MP_DEFAULT_SAVE_EXTENSION;
extern const MOption MP_DEFAULT_LOAD_PATH;
extern const MOption MP_DEFAULT_LOAD_FILENAME;
extern const MOption MP_DEFAULT_LOAD_EXTENSION;
extern const MOption MP_COMPOSE_TO;
extern const MOption MP_COMPOSE_CC;
extern const MOption MP_COMPOSE_BCC;
extern const MOption MP_COMPOSE_SHOW_FROM;
extern const MOption MP_DEFAULT_REPLY_KIND;
extern const MOption MP_LIST_ADDRESSES;
extern const MOption MP_EQUIV_ADDRESSES;
extern const MOption MP_REPLY_PREFIX;
extern const MOption MP_FORWARD_PREFIX;
extern const MOption MP_REPLY_COLLAPSE_PREFIX;
extern const MOption MP_REPLY_QUOTE_ORIG;
extern const MOption MP_REPLY_QUOTE_SELECTION;
extern const MOption MP_REPLY_MSGPREFIX;
extern const MOption MP_REPLY_MSGPREFIX_FROM_XATTR;
extern const MOption MP_REPLY_MSGPREFIX_FROM_SENDER;
extern const MOption MP_REPLY_QUOTE_EMPTY;
extern const MOption MP_COMPOSE_USE_SIGNATURE;
extern const MOption MP_COMPOSE_SIGNATURE;
extern const MOption MP_COMPOSE_USE_SIGNATURE_SEPARATOR;
extern const MOption MP_COMPOSE_USE_PGP;
extern const MOption MP_COMPOSE_PGPSIGN;
extern const MOption MP_COMPOSE_PGPSIGN_AS;
extern const MOption MP_COMPOSE_USE_XFACE;
extern const MOption MP_COMPOSE_XFACE_FILE;
extern const MOption MP_FOLDER_TYPE;
extern const MOption MP_FOLDER_TRY_CREATE;
extern const MOption MP_FOLDER_ICON;
extern const MOption MP_FOLDER_TREEINDEX;
extern const MOption MP_MOVE_NEWMAIL;
extern const MOption MP_NEWMAIL_FOLDER;
extern const MOption MP_OUTBOX_NAME;
extern const MOption MP_USE_OUTBOX;
extern const MOption MP_TRASH_FOLDER;
extern const MOption MP_USE_TRASH_FOLDER;
extern const MOption MP_DRAFTS_FOLDER;
extern const MOption MP_DRAFTS_AUTODELETE;
extern const MOption MP_FOLDER_PATH;
extern const MOption MP_FOLDER_COMMENT;
extern const MOption MP_UPDATEINTERVAL;
extern const MOption MP_FOLDER_CLOSE_DELAY;
extern const MOption MP_CONN_CLOSE_DELAY;
extern const MOption MP_AUTOMATIC_WORDWRAP;
extern const MOption MP_WRAP_QUOTED;
extern const MOption MP_WRAPMARGIN;
extern const MOption MP_VIEW_AUTOMATIC_WORDWRAP;
extern const MOption MP_VIEW_WRAPMARGIN;
extern const MOption MP_PLAIN_IS_TEXT;
extern const MOption MP_RFC822_IS_TEXT;
extern const MOption MP_RFC822_DECORATE;
extern const MOption MP_RFC822_SHOW_HEADERS;
extern const MOption MP_SHOW_XFACES;
extern const MOption MP_INLINE_GFX;
extern const MOption MP_INLINE_GFX_EXTERNAL;
extern const MOption MP_INLINE_GFX_SIZE;
extern const MOption MP_MSGVIEW_SHOWBAR;
extern const MOption MP_MSGVIEW_VIEWER;
extern const MOption MP_MSGVIEW_AUTO_VIEWER;
extern const MOption MP_MSGVIEW_PREFER_HTML;
extern const MOption MP_MSGVIEW_ALLOW_HTML;
extern const MOption MP_MSGVIEW_ALLOW_IMAGES;
extern const MOption MP_MSGVIEW_HEADERS;
extern const MOption MP_MSGVIEW_ALL_HEADERS;
extern const MOption MP_MSGVIEW_DEFAULT_ENCODING;
extern const MOption MP_LAST_CREATED_FOLDER_TYPE;
extern const MOption MP_FILTER_RULE;
extern const MOption MP_FOLDER_FILTERS;
extern const MOption MP_FOLDER_FILE_DRIVER;

extern const MOption MP_MVIEW_TITLE_FMT;
extern const MOption MP_MVIEW_FONT;
extern const MOption MP_MVIEW_FONT_DESC;
extern const MOption MP_MVIEW_FONT_SIZE;
extern const MOption MP_MVIEW_FGCOLOUR;
extern const MOption MP_MVIEW_BGCOLOUR;
extern const MOption MP_MVIEW_SIGCOLOUR;
extern const MOption MP_MVIEW_URLCOLOUR;
extern const MOption MP_MVIEW_ATTCOLOUR;
extern const MOption MP_MVIEW_QUOTED_COLOURIZE;
extern const MOption MP_MVIEW_QUOTED_CYCLE_COLOURS;
extern const MOption MP_MVIEW_QUOTED_COLOUR1;
extern const MOption MP_MVIEW_QUOTED_COLOUR2;
extern const MOption MP_MVIEW_QUOTED_COLOUR3;
extern const MOption MP_MVIEW_QUOTED_MAXWHITESPACE;
extern const MOption MP_MVIEW_QUOTED_MAXALPHA;
extern const MOption MP_MVIEW_HEADER_NAMES_COLOUR;
extern const MOption MP_MVIEW_HEADER_VALUES_COLOUR;
extern const MOption MP_HIGHLIGHT_SIGNATURE;
extern const MOption MP_HIGHLIGHT_URLS;

// folder view
extern const MOption MP_FVIEW_FONT;
extern const MOption MP_FVIEW_FONT_DESC;
extern const MOption MP_FVIEW_FONT_SIZE;
extern const MOption MP_FVIEW_NAMES_ONLY;
extern const MOption MP_FVIEW_FGCOLOUR;
extern const MOption MP_FVIEW_BGCOLOUR;
extern const MOption MP_FVIEW_DELETEDCOLOUR;
extern const MOption MP_FVIEW_NEWCOLOUR;
extern const MOption MP_FVIEW_RECENTCOLOUR;
extern const MOption MP_FVIEW_UNREADCOLOUR;
extern const MOption MP_FVIEW_FLAGGEDCOLOUR;
extern const MOption MP_FVIEW_AUTONEXT_UNREAD_MSG;
extern const MOption MP_FVIEW_AUTONEXT_UNREAD_FOLDER;
extern const MOption MP_FVIEW_SIZE_FORMAT;
extern const MOption MP_FVIEW_STATUS_UPDATE;
extern const MOption MP_FVIEW_STATUS_FMT;
extern const MOption MP_FVIEW_PREVIEW_DELAY;
extern const MOption MP_FVIEW_VERTICAL_SPLIT;
extern const MOption MP_FVIEW_FVIEW_TOP;
extern const MOption MP_FVIEW_AUTONEXT_ON_COMMAND;

// folder tree
extern const MOption MP_FTREE_LEFT;
extern const MOption MP_FTREE_FGCOLOUR;
extern const MOption MP_FTREE_BGCOLOUR;
extern const MOption MP_FTREE_SHOWOPENED;
extern const MOption MP_FTREE_FORMAT;
extern const MOption MP_FTREE_PROPAGATE;
extern const MOption MP_FTREE_NEVER_UNREAD;
extern const MOption MP_FTREE_HOME;

// composer
extern const MOption MP_CVIEW_FONT;
extern const MOption MP_CVIEW_FONT_DESC;
extern const MOption MP_CVIEW_FONT_SIZE;
extern const MOption MP_CVIEW_FGCOLOUR;
extern const MOption MP_CVIEW_BGCOLOUR;
extern const MOption MP_CVIEW_COLOUR_HEADERS;
extern const MOption MP_CHECK_FORGOTTEN_ATTACHMENTS;
extern const MOption MP_CHECK_ATTACHMENTS_REGEX;

// sorting/threading
extern const MOption MP_MSGS_SERVER_SORT;
extern const MOption MP_MSGS_SORTBY;
extern const MOption MP_MSGS_RESORT_ON_CHANGE;

extern const MOption MP_MSGS_USE_THREADING;
extern const MOption MP_MSGS_SERVER_THREAD;
extern const MOption MP_MSGS_SERVER_THREAD_REF_ONLY;
extern const MOption MP_MSGS_GATHER_SUBJECTS;
extern const MOption MP_MSGS_BREAK_THREAD;
extern const MOption MP_MSGS_INDENT_IF_DUMMY;

#if wxUSE_REGEX
extern const MOption MP_MSGS_SIMPLIFYING_REGEX;
extern const MOption MP_MSGS_REPLACEMENT_STRING;
#else // !regex
extern const MOption MP_MSGS_REMOVE_LIST_PREFIX_GATHERING;
extern const MOption MP_MSGS_REMOVE_LIST_PREFIX_BREAKING;
#endif // regex/!regex

extern const MOption MP_MSGS_SEARCH_CRIT;
extern const MOption MP_MSGS_SEARCH_ARG;
extern const MOption MP_BROWSER;
extern const MOption MP_BROWSER_ISNS;
extern const MOption MP_BROWSER_INNW;
extern const MOption MP_EXTERNALEDITOR;
extern const MOption MP_ALWAYS_USE_EXTERNALEDITOR;
extern const MOption MP_PGP_COMMAND;
extern const MOption MP_PGP_KEYSERVER;
extern const MOption MP_USE_NEWMAILCOMMAND;
extern const MOption MP_NEWMAILCOMMAND;
extern const MOption MP_NEWMAIL_PLAY_SOUND;
extern const MOption MP_NEWMAIL_SOUND_FILE;
#if defined(OS_UNIX) || defined(__CYGWIN__)
extern const MOption MP_NEWMAIL_SOUND_PROGRAM;
#endif // OS_UNIX
extern const MOption MP_SHOW_NEWMAILMSG;
extern const MOption MP_SHOW_NEWMAILINFO;
extern const MOption MP_NEWMAIL_UNSEEN;
extern const MOption MP_COLLECT_INBOX;
extern const MOption MP_USEOUTGOINGFOLDER;
extern const MOption MP_OUTGOINGFOLDER;
extern const MOption MP_SHOWHEADERS;
extern const MOption MP_AUTOCOLLECT_INCOMING;
extern const MOption MP_AUTOCOLLECT_ADB;
extern const MOption MP_AUTOCOLLECT_NAMED;
extern const MOption MP_AUTOCOLLECT_SENDER;
extern const MOption MP_AUTOCOLLECT_OUTGOING;
extern const MOption MP_SSL_DLL_SSL;
extern const MOption MP_SSL_DLL_CRYPTO;
extern const MOption MP_INCFAX_SUPPORT;
extern const MOption MP_INCFAX_DOMAINS;
extern const MOption MP_ADB_SUBSTRINGEXPANSION;
extern const MOption MP_FVIEW_FROM_REPLACE;
extern const MOption MP_FROM_REPLACE_ADDRESSES;
extern const MOption MP_MAX_MESSAGE_SIZE;
extern const MOption MP_MAX_HEADERS_NUM;
extern const MOption MP_MAX_HEADERS_NUM_HARD;
extern const MOption MP_SAFE_FILTERS;
extern const MOption MP_IMAP_LOOKAHEAD;
extern const MOption MP_TCP_OPENTIMEOUT;
extern const MOption MP_TCP_READTIMEOUT;
extern const MOption MP_TCP_WRITETIMEOUT;
extern const MOption MP_TCP_CLOSETIMEOUT;
extern const MOption MP_TCP_RSHTIMEOUT;
extern const MOption MP_TCP_SSHTIMEOUT;
extern const MOption MP_RSH_PATH;
extern const MOption MP_SSH_PATH;
extern const MOption MP_FLC_STATUSWIDTH;
extern const MOption MP_FLC_SUBJECTWIDTH;
extern const MOption MP_FLC_FROMWIDTH;
extern const MOption MP_FLC_DATEWIDTH;
extern const MOption MP_FLC_SIZEWIDTH;
extern const MOption MP_FLC_STATUSCOL;
extern const MOption MP_FLC_SUBJECTCOL;
extern const MOption MP_FLC_FROMCOL;
extern const MOption MP_FLC_DATECOL;
extern const MOption MP_FLC_SIZECOL;
extern const MOption MP_FLC_MSGNOCOL;
extern const MOption MP_TESTENTRY;
extern const MOption MP_SYNC_REMOTE;
extern const MOption MP_SYNC_FOLDER;
extern const MOption MP_SYNC_DATE;
extern const MOption MP_SYNC_FILTERS;
extern const MOption MP_SYNC_IDS;
extern const MOption MP_SYNC_FOLDERS;
extern const MOption MP_SYNC_FOLDERGROUP;
extern const MOption MP_CONFIRM_SEND;
extern const MOption MP_PREVIEW_SEND;
extern const MOption MP_AWAY_AUTO_ENTER;
extern const MOption MP_AWAY_AUTO_EXIT;
extern const MOption MP_AWAY_REMEMBER;
extern const MOption MP_AWAY_STATUS;
extern const MOption MP_CREATE_INTERNAL_MESSAGE;
extern const MOption MP_WHITE_LIST;
extern const MOption MP_TREAT_AS_JUNK_MAIL_FOLDER;

// miscellaneous
// -------------

// highlight the options according to their origin in the options dialog
extern const MOption MP_OPTION_SHOW_ORIGIN;
#define MP_OPTION_SHOW_ORIGIN_NAME "ShowOptionOrigin"
#define MP_OPTION_SHOW_ORIGIN_DEFVAL 1l

// the colour for the options set at this folder level
extern const MOption MP_OPTION_ORIGIN_HERE;
#define MP_OPTION_ORIGIN_HERE_NAME "OptionSetHereCol"
#define MP_OPTION_ORIGIN_HERE_DEFVAL "blue"

// the colour for the options inherited from parent (but non default)
extern const MOption MP_OPTION_ORIGIN_INHERITED;
#define MP_OPTION_ORIGIN_INHERITED_NAME "OptionInheritedCol"
#define MP_OPTION_ORIGIN_INHERITED_DEFVAL "RGB(0, 0, 127)"

// ----------------------------------------------------------------------------
// the option names
// ----------------------------------------------------------------------------

/** @name names of configuration entries */
//@{
/// our version
#define   MP_VERSION_NAME          "Version"
/// are we running for the first time?
#define   MP_FIRSTRUN_NAME         "FirstRun"
/// shall we record default values in configuration files
#define   MP_RECORDDEFAULTS_NAME      "RecordDefaults"
/// expand env vars in entries read from config?
#define   MP_EXPAND_ENV_VARS_NAME  "ExpandEnvVars"
/// default position x
#define   MP_XPOS_NAME            "XPos"
/// default position y
#define   MP_YPOS_NAME            "YPos"
/// window width
#define   MP_WIDTH_NAME         "Width"
/// window height
#define   MP_HEIGHT_NAME         "Height"
/// window iconisation status
#define   MP_ICONISED_NAME         "Iconised"
/// window maximized?
#define   MP_MAXIMISED_NAME         "Maximised"
/// tool bars shown?
#define   MP_SHOW_TOOLBAR_NAME    "ShowToolBar"
/// status bars shown?
#define   MP_SHOW_STATUSBAR_NAME  "ShowStatusBar"
/// window shown full-screen?
#define   MP_SHOW_FULLSCREEN_NAME "ShowFullScreen"

/// show log window?
#define   MP_SHOWLOG_NAME          "ShowLog"
/// the file to save log messages to (if not empty)
#define   MP_LOGFILE_NAME          "LogFile"
/// debug protocols and folder access?
#define   MP_DEBUG_CCLIENT_NAME   "MailDebug"

/// open ADB editor on startup?
#define   MP_SHOWADBEDITOR_NAME    "ShowAdb"

/// show tips at startup?
#define MP_SHOWTIPS_NAME "ShowTips"
/// the index of the last tip which was shown
#define MP_LASTTIP_NAME "LastTip"

/// expand folder tree control?
#define   MP_EXPAND_TREECTRL_NAME   "ExpandTreeControl"
/// focus follows mouse?
#define MP_FOCUS_FOLLOWSMOUSE_NAME  "FocusFollowsMouse"
/// dockable menu bars?
#define   MP_DOCKABLE_MENUBARS_NAME "MenuBarsDockable"
/// dockable tool bars?
#define   MP_DOCKABLE_TOOLBARS_NAME   "ToolBarsDockable"
/// flat tool bars?
#define   MP_FLAT_TOOLBARS_NAME   "ToolBarsFlat"

/// help browser kind
#define   MP_HELPBROWSER_KIND_NAME   "HelpBrowserKind"
/// help browser name
#define   MP_HELPBROWSER_NAME   "HelpBrowser"
/// is help browser of netscape type?
#define   MP_HELPBROWSER_ISNS_NAME   "HelpBrowserIsNetscape"
/// width of help frame
#define MP_HELPFRAME_WIDTH_NAME   "HelpFrameWidth"
/// height of help frame
#define MP_HELPFRAME_HEIGHT_NAME  "HelpFrameHeight"
/// xpos of help frame
#define MP_HELPFRAME_XPOS_NAME   "HelpFrameXpos"
/// ypos of help frame
#define MP_HELPFRAME_YPOS_NAME   "HelpFrameYpos"
/// the directory for mbox folders
#define   MP_MBOXDIR_NAME         "FolderDir"
/// the news spool directory
#define MP_NEWS_SPOOL_DIR_NAME "NewsSpool"
/// command to convert tiff faxes to postscript
#define   MP_TIFF2PS_NAME         "FaxToPS"
/// preferred intermediary image format in conversions (0=xpm,1=png,2=bmp,3=jpg)
#define MP_TMPGFXFORMAT_NAME      "ConvertGfxFormat"
/// the user's M directory
#   define   MP_USERDIR_NAME         "UserDirectory"
/// the acceptance status of the license
#define MP_LICENSE_ACCEPTED_NAME   "LicenseAccepted"

/// the complete path to the global M directory
#define MP_GLOBALDIR_NAME      "GlobalDir"

/// run onl one copy of the program at once?
#define MP_RUNONEONLY_NAME "RunOneOnly"

/// show images in the toolbar
#define MP_TBARIMAGES_NAME "ShowTbarImages"

/// the directory containing the help files
#define MP_HELPDIR_NAME "HelpDir"

// Unix-only entries
#ifdef OS_UNIX
/// the path where to find .afm files
#   define   MP_AFMPATH_NAME         "AfmPath"
#endif //Unix

/// Which encryption algorithm to use : 0 = simple builtin, 1 = twofish
#define MP_CRYPTALGO_NAME    "CryptAlgo"
/// DoesTwoFish work? (-1 unknown, 0 no, 1 yes)
#define MP_CRYPT_TWOFISH_OK_NAME "TwoFishOk"
/// some test data
#define MP_CRYPT_TESTDATA_NAME "CryptData"
/// the locale for translation to national languages
#define   MP_LOCALE_NAME               "Locale"
/// the default character set
#define MP_CHARSET_NAME      "CharSet"
/// the default icon for frames
#define   MP_ICON_MFRAME_NAME         "MFrameIcon"
/// the icon for the main frame
#define   MP_ICON_MAINFRAME_NAME      "MainFrameIcon"
/// the icon directory
#define   MP_ICONPATH_NAME         "IconDirectory"
/// the path for finding profiles
#define   MP_PROFILE_PATH_NAME         "ProfilePath"
/// the extension to use for profile files
#define   MP_PROFILE_EXTENSION_NAME      "ProfileExtension"
/// the key for identity redirection
#define MP_PROFILE_IDENTITY_NAME "ProfileId"
/// the name of the mailcap file
#define   MP_MAILCAP_NAME         "MailCap"
/// the name of the mime types file
#define   MP_MIMETYPES_NAME         "MimeTypes"
/// the strftime() format for dates
#define   MP_DATE_FMT_NAME         "DateFormat"
/// display all dates as GMT?
#define   MP_DATE_GMT_NAME         "GMTDate"
/// show console window
#define   MP_SHOWCONSOLE_NAME      "ShowConsole"
/// name of address database
#define   MP_ADBFILE_NAME         "AddressBook"
/// open any folders at all on startup?
#define   MP_DONTOPENSTARTUP_NAME   "DontOpenAtStartup"
/// names of folders to open at startup (semicolon separated list)
#define   MP_OPENFOLDERS_NAME         "OpenFolders"
/// reopen the last opened folder in the main frame
#define   MP_REOPENLASTFOLDER_NAME "ReopenLastFolder"
/// name of folder to open in mainframe
#define   MP_MAINFOLDER_NAME          "MainFolder"
/// path for Python
#define   MP_PYTHONPATH_NAME        "PythonPath"
/// Python DLL
#define   MP_PYTHONDLL_NAME        "PythonDLL"
/// is Python enabled (this is a run-time option)?
#define   MP_USEPYTHON_NAME         "UsePython"
/// start-up script to run
#define   MP_PYTHONMODULE_TO_LOAD_NAME     "StartupScript"
/// show splash screen on startup?
#define   MP_SHOWSPLASH_NAME        "ShowSplash"
/// how long should splash screen stay (0 disables timeout)?
#define   MP_SPLASHDELAY_NAME       "SplashDelay"
/// how often should we autosave the profile settings (0 to disable)?
#define   MP_AUTOSAVEDELAY_NAME       "AutoSaveDelay"
/// how often should we check for incoming mail (secs, 0 to disable)?
#define   MP_POLLINCOMINGDELAY_NAME       "PollIncomingDelay"
/// poll folder only if it is opened
#define   MP_POLL_OPENED_ONLY_NAME "PollOpenedOnly"
/// collect all new mail at startup?
#define   MP_COLLECTATSTARTUP_NAME "CollectAtStartup"
/// ask user if he really wants to exit?
#define   MP_CONFIRMEXIT_NAME       "ConfirmExit"
/// open folders when they're clicked (otherwise - double clicked)
#define   MP_OPEN_ON_CLICK_NAME     "ClickToOpen"
/// show all folders (even hidden ones) in the folder tree?
#define   MP_SHOW_HIDDEN_FOLDERS_NAME "ShowHiddenFolders"
/// create .profile files?
#define   MP_CREATE_PROFILES_NAME   "CreateProfileFiles"
/// umask setting for normal files
#define   MP_UMASK_NAME               "Umask"
/// the last selected message in the folder view (or -1)
#define   MP_LASTSELECTED_MESSAGE_NAME "FViewLastSel"
/// automatically show th last selected message in folderview?
#define   MP_AUTOSHOW_LASTSELECTED_NAME "AutoShowLastSel"
/// automatically show first message in folderview?
#define   MP_AUTOSHOW_FIRSTMESSAGE_NAME "AutoShowFirstMessage"
/// automatically show first unread message in folderview?
#define   MP_AUTOSHOW_FIRSTUNREADMESSAGE_NAME "AutoShowFirstUnread"
/// open messages when they're clicked (otherwise - double clicked)
#define   MP_PREVIEW_ON_SELECT_NAME     "PreviewOnSelect"
/// select the initially focused message
#define   MP_AUTOSHOW_SELECT_NAME "AutoShowSelect"
/// program used to convert image files?
#define   MP_CONVERTPROGRAM_NAME      "ImageConverter"
/// list of modules to load at startup
#define MP_MODULES_NAME               "Modules"
/// list of modules to not load
#define MP_MODULES_DONT_LOAD_NAME               "ModulesIgnore"
/// the user path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_USER_NAME   "CompooseTemplatePathUser"
/// the global path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_GLOBAL_NAME   "CompooseTemplatePath"

/// the format string for the folder tree display
#define MP_FOLDERSTATUS_TREE_NAME "TreeViewFmt"
/// the format string for status bar folder status display
#define MP_FOLDERSTATUS_STATBAR_NAME "StatusBarFmt"
/// the format string for title bar folder status display
#define MP_FOLDERSTATUS_TITLEBAR_NAME "TitleBarFmt"

/**@name Printer settings */
//@{
/// Command
#define MP_PRINT_COMMAND_NAME   "PrintCommand"
/// Options
#define MP_PRINT_OPTIONS_NAME   "PrintOptions"
/// Orientation
#define MP_PRINT_ORIENTATION_NAME "PrintOrientation"
/// print mode
#define MP_PRINT_MODE_NAME      "PrintMode"
/// paper name
#define MP_PRINT_PAPER_NAME   "PrintPaperType"
/// paper name
#define MP_PRINT_FILE_NAME   "PrintFilenname"
/// print in colour?
#define MP_PRINT_COLOUR_NAME "PrintUseColour"
/// top margin
#define MP_PRINT_TOPMARGIN_X_NAME    "PrintMarginLeft"
/// left margin
#define MP_PRINT_TOPMARGIN_Y_NAME   "PrintMarginTop"
/// bottom margin
#define MP_PRINT_BOTTOMMARGIN_X_NAME    "PrintMarginRight"
/// right margin
#define MP_PRINT_BOTTOMMARGIN_Y_NAME   "PrintMarginBottom"
/// zoom level in print preview
#define MP_PRINT_PREVIEWZOOM_NAME   "PrintPreviewZoom"
//@}
/**@name for BBDB address book support */
//@{
/// generate unique names
#define   MP_BBDB_GENERATEUNIQUENAMES_NAME   "BbdbGenerateUniqueNames"
/// ignore entries without names
#define   MP_BBDB_IGNOREANONYMOUS_NAME      "BbdbIgnoreAnonymous"
/// name for anonymous entries, when neither first nor family name are set
#define   MP_BBDB_ANONYMOUS_NAME         "BbdbAnonymousName"
/// save on exit, 0=no, 1=ask, 2=always
#define   MP_BBDB_SAVEONEXIT_NAME  "BbdbSaveOnExit"
//@}
/**@name For Profiles: */
//@{
/// The Profile Type. [OBSOLETE]
#define   MP_PROFILE_TYPE_NAME      "ProfileType"
/// The type of the config source
#define   MP_CONFIG_SOURCE_TYPE_NAME "Type"
/// The priority of the config source
#define   MP_CONFIG_SOURCE_PRIO_NAME "Priority"
/// the current user identity
#define   MP_CURRENT_IDENTITY_NAME  "Identity"
/// the user's full name
#define   MP_PERSONALNAME_NAME     "PersonalName"
/// organization (the "Organization:" header name value)
#define   MP_ORGANIZATION_NAME     "Organization"
/// the user's qualification
#define   MP_USERLEVEL_NAME        "Userlevel"
/// the username/login
#define   MP_USERNAME_NAME         "UserName"
/// the user's hostname
#define   MP_HOSTNAME_NAME         "HostName"
/// Add this hostname for addresses without hostname?
#define   MP_ADD_DEFAULT_HOSTNAME_NAME   "AddDefaultHostName"
/// (the username for returned mail) E-mail address
#define   MP_FROM_ADDRESS_NAME      "ReturnAddress"
/// Reply address
#define   MP_REPLY_ADDRESS_NAME      "ReplyAddress"
/// the default POP3 host
#define   MP_POPHOST_NAME          "Pop3Host"
/// don't use AUTH with POP3
#define   MP_POP_NO_AUTH_NAME       "Pop3NoAuth"
/// the default IMAP4 host
#define   MP_IMAPHOST_NAME          "Imap4Host"
/// use SSL for POP/IMAP?
#define   MP_USE_SSL_NAME           "UseSSL"
/// accept unsigned SSL certificates?
#define   MP_USE_SSL_UNSIGNED_NAME  "SSLUnsigned"
/// the mail host
#define   MP_SMTPHOST_NAME         "MailHost"
/// use the specified sender value or guess it automatically?
#define   MP_GUESS_SENDER_NAME       "GuessSender"
/// the smtp sender value
#define   MP_SENDER_NAME           "Sender"
/// the smtp host user-id
#define   MP_SMTPHOST_LOGIN_NAME   "MailHostLogin"
/// the smtp host password
#define   MP_SMTPHOST_PASSWORD_NAME  "MailHostPw"
/// the news server
#define   MP_NNTPHOST_NAME         "NewsHost"
/// the news host user-id
#define   MP_NNTPHOST_LOGIN_NAME   "NewsHostLogin"
/// the news host password
#define   MP_NNTPHOST_PASSWORD_NAME  "NewsHostPw"
/// use SSL?
#define   MP_SMTPHOST_USE_SSL_NAME         "MailHostSSL"
/// check ssl-certs for SMTP connections?
#define   MP_SMTPHOST_USE_SSL_UNSIGNED_NAME   "MailHostSSLUnsigned"
/// use ESMTP 8BITMIME extension if available
#define   MP_SMTP_USE_8BIT_NAME         "Mail8Bit"
/// disabled SMTP authentificators
#define   MP_SMTP_DISABLED_AUTHS_NAME     "SmtpDisabledAuths"
/// sendmail command
#define MP_SENDMAILCMD_NAME "SendmailCmd"
/// use sendmail?
#define MP_USE_SENDMAIL_NAME "UseSendmail"
/// use SSL?
#define   MP_NNTPHOST_USE_SSL_NAME         "NewsHostSSL"
/// check ssl-certs for NNTP connections?
#define   MP_NNTPHOST_USE_SSL_UNSIGNED_NAME   "NewsHostSSLUnsigned"
/// the beacon host to test for net connection
#define   MP_BEACONHOST_NAME      "BeaconHost"
#ifdef USE_DIALUP
/// does Mahogany control dial-up networking?
#define MP_DIALUP_SUPPORT_NAME   "DialUpNetSupport"
#endif // USE_DIALUP

/// set reply string from To: field?
#define MP_SET_REPLY_FROM_TO_NAME   "ReplyEqualsTo"
/// set reply address only?
#define MP_SET_REPLY_STD_NAME_NAME  "ReplyUseStdName"
/// should we attach vCard to outgoing messages?
#define MP_USEVCARD_NAME "UseVCard"
/// the vCard to use
#define MP_VCARD_NAME "VCard"

/// use the folder create wizard (or the dialog directly)?
#define MP_USE_FOLDER_CREATE_WIZARD_NAME "FolderCreateWizard"

#if defined(OS_WIN)
/// the RAS connection to use
#define MP_NET_CONNECTION_NAME "RasConnection"
#elif defined(OS_UNIX)
/// the command to go online
#define MP_NET_ON_COMMAND_NAME   "NetOnCommand"
/// the command to go offline
#define MP_NET_OFF_COMMAND_NAME   "NetOffCommand"
#endif // platform

/// login for mailbox
#define   MP_FOLDER_LOGIN_NAME      "Login"
/// password for mailbox
#define   MP_FOLDER_PASSWORD_NAME      "Password"
/// log level
#define   MP_LOGLEVEL_NAME      "LogLevel"
/// show busy info while sorting/threading?
#define   MP_SHOWBUSY_DURING_SORT_NAME "BusyDuringSort"
/// threshold for displaying mailfolder progress dialog
#define   MP_FOLDERPROGRESS_THRESHOLD_NAME   "FolderProgressThreshold"
/// size threshold for displaying message retrieval progress dialog
#define   MP_MESSAGEPROGRESS_THRESHOLD_SIZE_NAME   "MsgProgressMinSize"
/// time threshold for displaying message retrieval progress dialog
#define   MP_MESSAGEPROGRESS_THRESHOLD_TIME_NAME   "MsgProgressDelay"
/// the default path for saving files
#define   MP_DEFAULT_SAVE_PATH_NAME      "SavePath"
/// the default filename for saving files
#define   MP_DEFAULT_SAVE_FILENAME_NAME   "SaveFileName"
/// the default extension for saving files
#define   MP_DEFAULT_SAVE_EXTENSION_NAME   "SaveExtension"
/// the wildcard for save dialog
#define   MP_DEFAULT_SAVE_WILDCARD_NAME   "SaveWildcard"
/// the default path for saving files
#define   MP_DEFAULT_LOAD_PATH_NAME      "LoadPath"
/// the default filename for saving files
#define   MP_DEFAULT_LOAD_FILENAME_NAME   "LoadFileName"
/// the default extension for saving files
#define   MP_DEFAULT_LOAD_EXTENSION_NAME   "LoadExtension"
/// the wildcard for save dialog
#define   MP_DEFAULT_LOAD_WILDCARD_NAME   "LoadWildcard"
/// default value for To: field in composition
#define   MP_COMPOSE_TO_NAME         "ComposeToDefault"
/// default value for Cc: field in composition
#define   MP_COMPOSE_CC_NAME         "ComposeCcDefault"
/// default value for Bcc: field in composition
#define   MP_COMPOSE_BCC_NAME         "ComposeBccDefault"
/// show "From:" field in composer?
#define   MP_COMPOSE_SHOW_FROM_NAME "ComposeShowFrom"

/// default reply kind
#define   MP_DEFAULT_REPLY_KIND_NAME "ReplyDefault"
/// the mailing list addresses
#define   MP_LIST_ADDRESSES_NAME "MLAddresses"
/// array of equivalent address pairs
#define   MP_EQUIV_ADDRESSES_NAME "EquivAddresses"
/// prefix for subject in replies
#define   MP_REPLY_PREFIX_NAME         "ReplyPrefix"
/// prefix for subject in forwards
#define   MP_FORWARD_PREFIX_NAME         "ForwardPrefix"
/// collapse reply prefixes? 0=no, 1=replace "Re"s with one, 2=use reply level
#define   MP_REPLY_COLLAPSE_PREFIX_NAME "CollapseReplyPrefix"
/// include the original message in the reply [no,ask,yes]
#define MP_REPLY_QUOTE_ORIG_NAME "ReplyQuoteInsert" 
/// include only the selected text (if any) in the reply?
#define MP_REPLY_QUOTE_SELECTION_NAME "ReplyQuoteSelection"
/// prefix for text in replies
#define   MP_REPLY_MSGPREFIX_NAME      "ReplyQuote"
/// use the value of X-Attribution header as the prefix
#define   MP_REPLY_MSGPREFIX_FROM_XATTR_NAME "ReplyQuoteXAttr"
/// prepend the initials of the sender to the reply prefix?
#define   MP_REPLY_MSGPREFIX_FROM_SENDER_NAME "ReplyQuoteUseSender"
/// quote the empty lines when replying?
#define   MP_REPLY_QUOTE_EMPTY_NAME      "ReplyQuoteEmpty"
/// use signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_NAME   "ComposeInsertSignature"
/// filename of signature file
#define   MP_COMPOSE_SIGNATURE_NAME      "SignatureFile"
/// use "-- " to separate signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_SEPARATOR_NAME   "ComposeSeparateSignature"
/// show PGP-related controls in the composer?
#define   MP_COMPOSE_USE_PGP_NAME   "ShowPGPControls"
/// sign messages with PGP?
#define   MP_COMPOSE_PGPSIGN_NAME   "SignWithPGP"
/// the user name to sign messages with PGP as
#define   MP_COMPOSE_PGPSIGN_AS_NAME   "SignWithPGPAs"

/// use XFace in composition?
#define   MP_COMPOSE_USE_XFACE_NAME   "UseXFaces"
/// Name from where to read XFace
#define   MP_COMPOSE_XFACE_FILE_NAME   "XFace"
/// the folder type for a mailbox (see enum in MFolder class)
#define   MP_FOLDER_TYPE_NAME         "Type"
/// should we try to create the folder before opening it?
#define   MP_FOLDER_TRY_CREATE_NAME "TryCreate"
/// the folder icon for a mailbox (see icon functions in FolderType.h)
#define   MP_FOLDER_ICON_NAME         "Icon"
/// the position of the folder in the tree
#define MP_FOLDER_TREEINDEX_NAME "Index"
/// Move new mail to the NewMail folder (if not, only copy)?
#define MP_MOVE_NEWMAIL_NAME      "MoveNewMail"
/// Where to store all new mail (obsolete)
#define MP_NEWMAIL_FOLDER_NAME      "NewMailFolder"
/// Where to store outgoing mail
#define MP_OUTBOX_NAME_NAME       "OutBoxName"
/// Use outbox?
#define MP_USE_OUTBOX_NAME          "UseOutBox"
/// Name of Trash folder?
#define MP_TRASH_FOLDER_NAME      "TrashFolder"
/// Use a trash folder?
#define MP_USE_TRASH_FOLDER_NAME   "UseTrash"
/// Name of the Drafts folder
#define MP_DRAFTS_FOLDER_NAME "DraftsFolder"
/// Delete the drafts automatically after the message was sent?
#define MP_DRAFTS_AUTODELETE_NAME "DraftsDelete"
/// the filename for a mailbox
#define   MP_FOLDER_PATH_NAME         "Path"
/// comment
#define   MP_FOLDER_COMMENT_NAME      "Comment"
/// update interval for folders in seconds
#define   MP_UPDATEINTERVAL_NAME      "UpdateInterval"
/// close of folders delayed by
#define MP_FOLDER_CLOSE_DELAY_NAME   "FolderCloseDelay"
/// close of network connection delayed by
#define MP_CONN_CLOSE_DELAY_NAME   "ConnCloseDelay"
/// do automatic word wrap?
#define MP_AUTOMATIC_WORDWRAP_NAME   "AutoWrap"
/// Wrap quoted lines?
#define MP_WRAP_QUOTED_NAME "WrapQuoted"
/// wrapmargin for composition (set to -1 to disable it)
#define   MP_WRAPMARGIN_NAME      "WrapMargin"
/// do automatic word wrap in message view?
#define MP_VIEW_AUTOMATIC_WORDWRAP_NAME   "ViewAutoWrap"
/// wrapmargin for message view (set to -1 to disable it)
#define   MP_VIEW_WRAPMARGIN_NAME      "ViewWrapMargin"
/// show TEXT/PLAIN as inlined text?
#define   MP_PLAIN_IS_TEXT_NAME      "PlainIsText"
/// show MESSAGE/RFC822 as inlined text?
#define   MP_RFC822_IS_TEXT_NAME      "Rfc822IsText"
/// show decorations around inlined MESSAGE/RFC822 text?
#define   MP_RFC822_DECORATE_NAME "Rfc822AsTextDecorate"
/// show headers of the inlined MESSAGE/RFC822 text?
#define   MP_RFC822_SHOW_HEADERS_NAME "Rfc822AsTextHeaders"
/// show XFaces?
#define   MP_SHOW_XFACES_NAME         "ShowXFaces"
/// show graphics inline
#define   MP_INLINE_GFX_NAME         "InlineGraphics"
/// show the external images (for HTML viewer only) too?
#define   MP_INLINE_GFX_EXTERNAL_NAME "InlineExtGraphics"
/// limit size for inline graphics
#define   MP_INLINE_GFX_SIZE_NAME     "InlineGraphicsSize"
/// show viewer bar in the message view?
#define MP_MSGVIEW_SHOWBAR_NAME "ShowViewerBar"
/// which viewer to use in the message view?
#define MP_MSGVIEW_VIEWER_NAME "MsgViewer"
/// select the "best" viewer automatically?
#define MP_MSGVIEW_AUTO_VIEWER_NAME "AutoViewer"
/// use HTML viewer whenever there is HTML content?
#define MP_MSGVIEW_PREFER_HTML_NAME "PreferHTML"
/// allow HTML viewer when there is only HTML content?
#define MP_MSGVIEW_ALLOW_HTML_NAME "AllowHTML"
/// allow image-capable viewer when there are inline images?
#define MP_MSGVIEW_ALLOW_IMAGES_NAME "AllowImages"
/// which headers to show in the message view?
#define   MP_MSGVIEW_HEADERS_NAME     "MsgViewHeaders"
/// all headers we know about
#define   MP_MSGVIEW_ALL_HEADERS_NAME     "MsgViewAllHeaders"
/// the default encoding for the viewer/composer
#define  MP_MSGVIEW_DEFAULT_ENCODING_NAME   "DefEncoding"
/// the type of the last created folder
#define   MP_LAST_CREATED_FOLDER_TYPE_NAME  "LastFolderType"
/// the filter program to apply (OBSOLETE)
#define MP_FILTER_RULE_NAME   "Filter"
/// the filters to use for this folder
#define MP_FOLDER_FILTERS_NAME   "Filters"
/// the default folder file format
#define MP_FOLDER_FILE_DRIVER_NAME   "MailboxFileFormat"
/**@name  Font settings for message view */
//@{
/// message view title
#define   MP_MVIEW_TITLE_FMT_NAME   "MViewTitleFmt"
/// which font to use
#define   MP_MVIEW_FONT_NAME         "MViewFont"
/// which font size
#define   MP_MVIEW_FONT_SIZE_NAME         "MViewFontSize"
/// the full font desc (replaces the 2 settings above)
#define   MP_MVIEW_FONT_DESC_NAME   "MViewFontDesc"
/// which foreground colour for the font
#define   MP_MVIEW_FGCOLOUR_NAME      "MViewFgColour"
/// which background colour for the font
#define   MP_MVIEW_BGCOLOUR_NAME      "MViewBgColour"
/// which colour for signature
#define   MP_MVIEW_SIGCOLOUR_NAME      "MViewSigColour"
/// which colour for URLS
#define   MP_MVIEW_URLCOLOUR_NAME      "MViewUrlColour"
/// colour for attachment labels
#define   MP_MVIEW_ATTCOLOUR_NAME      "MViewAttColour"
/// perform quoted text colourization?
#define   MP_MVIEW_QUOTED_COLOURIZE_NAME   "MViewQuotedColourized"
/// cycle colours?
#define   MP_MVIEW_QUOTED_CYCLE_COLOURS_NAME   "MViewQuotedCycleColours"
/// which colour for quoted text
#define   MP_MVIEW_QUOTED_COLOUR1_NAME      "MViewQuotedColour1"
/// which colour for quoted text, second level
#define   MP_MVIEW_QUOTED_COLOUR2_NAME      "MViewQuotedColour2"
/// which colour for quoted text, third level
#define   MP_MVIEW_QUOTED_COLOUR3_NAME      "MViewQuotedColour3"
/// the maximum number of whitespaces prepending >
#define   MP_MVIEW_QUOTED_MAXWHITESPACE_NAME    "MViewQuotedMaxWhitespace"
/// the maximum number of A-Z prepending >
#define   MP_MVIEW_QUOTED_MAXALPHA_NAME    "MViewQuotedMaxAlpha"
/// the colour for header names in the message view
#define   MP_MVIEW_HEADER_NAMES_COLOUR_NAME  "MViewHeaderNamesColour"
/// the colour for header values in the message view
#define   MP_MVIEW_HEADER_VALUES_COLOUR_NAME  "MViewHeaderValuesColour"
//@}
/**@name  Font settings for folder view */
//@{
/// which font to use
#define   MP_FVIEW_FONT_NAME         "FViewFont"
/// which font size
#define   MP_FVIEW_FONT_SIZE_NAME         "FViewFontSize"
/// the full font desc (replaces the 2 settings above)
#define   MP_FVIEW_FONT_DESC_NAME         "FViewFontDesc"
/// don't show full e-mail, only sender's name
#define   MP_FVIEW_NAMES_ONLY_NAME         "FViewNamesOnly"
/// which foreground colour for the font
#define   MP_FVIEW_FGCOLOUR_NAME      "FViewFgColour"
/// which background colour for the font
#define   MP_FVIEW_BGCOLOUR_NAME      "FViewBgColour"
/// colour for deleted messages
#define   MP_FVIEW_DELETEDCOLOUR_NAME      "FViewDeletedColour"
/// colour for new messages
#define   MP_FVIEW_NEWCOLOUR_NAME      "FViewNewColour"
/// colour for recent messages
#define   MP_FVIEW_RECENTCOLOUR_NAME      "FViewRecentColour"
/// colour for unread messages
#define   MP_FVIEW_UNREADCOLOUR_NAME      "FViewUnreadColour"
/// colour for flagged messages
#define   MP_FVIEW_FLAGGEDCOLOUR_NAME      "FViewFlaggedColour"
/// automatically select next unread message after finishing the current one
#define MP_FVIEW_AUTONEXT_UNREAD_MSG_NAME "FViewAutoNextMsg"
/// automatically select next unread folder after finishing the current one
#define MP_FVIEW_AUTONEXT_UNREAD_FOLDER_NAME "FViewAutoNextFolder"
/// how to show the size (MessageSizeShow enum value)
#define MP_FVIEW_SIZE_FORMAT_NAME   "SizeFormat"
/// update the folder view status bar to show the msg info?
#define   MP_FVIEW_STATUS_UPDATE_NAME "FViewStatUpdate"
/// folder view status bar string
#define   MP_FVIEW_STATUS_FMT_NAME  "FViewStatFmt"
/// delay before previewing the selected item in the folder view (0 to disable)
#define MP_FVIEW_PREVIEW_DELAY_NAME "FViewPreviewDelay"
/// split folder view vertically (or horizontally)?
#define MP_FVIEW_VERTICAL_SPLIT_NAME "FViewVertSplit"
/// put folder view on top and msg view on bottom or vice versa?
#define MP_FVIEW_FVIEW_TOP_NAME "FViewOnTop"
/// replace "From" address with "To" in messages from oneself?
#define MP_FVIEW_FROM_REPLACE_NAME "ReplaceFrom"
/// the ':' separated list of addresses which are "from oneself"
#define MP_FROM_REPLACE_ADDRESSES_NAME "ReplaceFromAdr"
/// Automatically focus next message after copy, mark (un)read, etc.
#define MP_FVIEW_AUTONEXT_ON_COMMAND_NAME "FViewAutoNextOnCommand"
//@}
/**@name  Font settings for folder tree */
//@{
/// is the folder tree on the left of folder view or on the right?
#define MP_FTREE_LEFT_NAME "FTreeLeft"
/// the foreground colour for the folder tree
#define MP_FTREE_FGCOLOUR_NAME "FTreeFgColour"
/// the background colour for the folder tree
#define MP_FTREE_BGCOLOUR_NAME "FTreeBgColour"
/// show the currently opened folder specially?
#define MP_FTREE_SHOWOPENED_NAME "FTreeShowOpened"
/// format for the folder tree entries
#define MP_FTREE_FORMAT_NAME "FTreeFormat"
/// reflect the folder status in its parent
#define MP_FTREE_PROPAGATE_NAME "FTreePropagate"
/// skip this folder when looking for next unread one in the tree
#define MP_FTREE_NEVER_UNREAD_NAME "FTreeNeverUnread"
/// go to this folder when Ctrl-Home is pressed
#define MP_FTREE_HOME_NAME "FTreeHome"
//@}
/**@name Font and colour settings for composer */
//@{
/// which font to use
#define   MP_CVIEW_FONT_NAME         "CViewFont"
/// which font size
#define   MP_CVIEW_FONT_SIZE_NAME         "CViewFontSize"
/// the full font desc (replaces the 2 settings above)
#define   MP_CVIEW_FONT_DESC_NAME         "CViewFontDesc"
/// which foreground colour for the font
#define   MP_CVIEW_FGCOLOUR_NAME      "CViewFGColour"
/// which background colour for the font
#define   MP_CVIEW_BGCOLOUR_NAME      "CViewBGColout" // typo but do *NOT* fix
/// use the colours and font for the headers as well?
#define   MP_CVIEW_COLOUR_HEADERS_NAME "CViewColourHeaders"
/// check for forgotten attachments?
#define   MP_CHECK_FORGOTTEN_ATTACHMENTS_NAME "CheckForgottenAttachments"
/// regex to use for the attachments check
#define   MP_CHECK_ATTACHMENTS_REGEX_NAME "AttachmentsCheckRegex"
//@}
/// highlight signature?
#define   MP_HIGHLIGHT_SIGNATURE_NAME      "HighlightSig"
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS_NAME      "HighlightURL"

/// do we want to use server side sort?
#define MP_MSGS_SERVER_SORT_NAME    "SortOnServer"
/// sort criterium for folder listing
#define MP_MSGS_SORTBY_NAME         "SortMessagesBy"
/// re-sort messages on status change?
#define MP_MSGS_RESORT_ON_CHANGE_NAME         "ReSortMessagesOnChange"
/// use threading
#define MP_MSGS_USE_THREADING_NAME  "ThreadMessages"
/// use server side threading?
#define MP_MSGS_SERVER_THREAD_NAME "ThreadOnServer"
/// only use server side threading by references (best threading method)?
#define MP_MSGS_SERVER_THREAD_REF_ONLY_NAME "ThreadByRefOnly"

/// Gather messages with same subject in one thread
#define MP_MSGS_GATHER_SUBJECTS_NAME "GatherSubjectsWhenThreading"
/// break thread when subject changes
#define MP_MSGS_BREAK_THREAD_NAME "BreakThreadIfSubjectChanges"
/// Indent messages when common ancestor is missing
#define MP_MSGS_INDENT_IF_DUMMY_NAME "IndentIfDummy"

#if wxUSE_REGEX
#   define MP_MSGS_SIMPLIFYING_REGEX_NAME "SimplifyingRegex"
#   define MP_MSGS_REPLACEMENT_STRING_NAME "ReplacementString"
#else // !wxUSE_REGEX
    /// Remove list prefix when comparing message's subject to gather them
#   define MP_MSGS_REMOVE_LIST_PREFIX_GATHERING_NAME "RemoveListPrefixWhenGathering"
    /// Remove list prefix when comparing message's subject to break threads
#   define MP_MSGS_REMOVE_LIST_PREFIX_BREAKING_NAME "RemoveListPrefixWhenBreaking"
#endif // wxUSE_REGEX

/// search criterium for searching in folders
#define MP_MSGS_SEARCH_CRIT_NAME   "SearchCriterium"
/// search argument
#define MP_MSGS_SEARCH_ARG_NAME    "SearchArgument"
/// open URLs with
#define   MP_BROWSER_NAME         "Browser"
/// Browser is netscape variant
#define   MP_BROWSER_ISNS_NAME    "BrowserIsNetscape"
/// Open netscape in new window
#define   MP_BROWSER_INNW_NAME    "BrowserInNewWindow"
/// external editor to use for message composition (use %s for filename)
#define MP_EXTERNALEDITOR_NAME    "ExternalEditor"
/// start external editor automatically?
#define MP_ALWAYS_USE_EXTERNALEDITOR_NAME    "AlwaysUseExtEditor"
/// PGP/GPG application
#define MP_PGP_COMMAND_NAME    "PGPCommand"
/// PGP/GPG key server for public keys
#define MP_PGP_KEYSERVER_NAME  "PGPKeyServer"
/// execute a command when new mail arrives?
#define   MP_USE_NEWMAILCOMMAND_NAME      "CommandOnNewMail"
/// command to execute when new mail arrives
#define   MP_NEWMAILCOMMAND_NAME      "OnNewMail"

/// play a sound on new mail?
#define MP_NEWMAIL_PLAY_SOUND_NAME "NewMailPlaySound"
/// which sound to play?
#define MP_NEWMAIL_SOUND_FILE_NAME "NewMailSound"
#if defined(OS_UNIX) || defined(__CYGWIN__)
/// the program to use to play this sound
#define MP_NEWMAIL_SOUND_PROGRAM_NAME "NewMailSoundProg"
#endif // OS_UNIX

/// show new mail messages?
#define   MP_SHOW_NEWMAILMSG_NAME      "ShowNewMail"
/// show detailed info about how many new mail messages?
#define   MP_SHOW_NEWMAILINFO_NAME      "ShowNewMailInfo"
/// consider only unseen messages as new?
#define   MP_NEWMAIL_UNSEEN_NAME "NewUnseenOnly"
/// collect mail from INBOX?
#define   MP_COLLECT_INBOX_NAME "CollectInbox"
/// keep copies of outgoing mail?
#define   MP_USEOUTGOINGFOLDER_NAME   "KeepCopies"
/// write outgoing mail to folder:
#define   MP_OUTGOINGFOLDER_NAME      "SentMailFolder"
/// Show all message headers?
#define   MP_SHOWHEADERS_NAME         "ShowHeaders"
/// Autocollect email addresses? 0=no 1=ask 2=always
#define   MP_AUTOCOLLECT_INCOMING_NAME         "AutoCollect"
/// Name of the address books for autocollected addresses
#define   MP_AUTOCOLLECT_ADB_NAME     "AutoCollectAdb"
/// Autocollect email addresses from sender only ?
#define   MP_AUTOCOLLECT_SENDER_NAME     "AutoCollectSender"
/// Autocollect addresses in mail sent by user?
#define   MP_AUTOCOLLECT_OUTGOING_NAME     "AutoCollectOutgoing"
/// Autocollect entries with names only?
#define   MP_AUTOCOLLECT_NAMED_NAME "AutoCollectNamed"
/// support efax style incoming faxes
#define MP_INCFAX_SUPPORT_NAME      "IncomingFaxSupport"
/// domains from which to support faxes, semicolon delimited
#define MP_INCFAX_DOMAINS_NAME      "IncomingFaxDomains"
/// Default name for the SSL library
#define MP_SSL_DLL_SSL_NAME      "SSLDll"
/// Default name for the SSL/crypto library
#define MP_SSL_DLL_CRYPTO_NAME "CryptoDll"

/// Use substrings in address expansion?
#define   MP_ADB_SUBSTRINGEXPANSION_NAME   "ExpandWithSubstring"

/** @name maximal amounts of data to retrieve from remote servers */
//@{
/// ask confirmation before retrieveing messages bigger than this (in Kb)
#define MP_MAX_MESSAGE_SIZE_NAME   "MaxMsgSize"
/// ask confirmation before retrieveing more headers than this
#define MP_MAX_HEADERS_NUM_NAME    "MaxHeadersNum"
/// never download more than that many messages
#define MP_MAX_HEADERS_NUM_HARD_NAME "HardHeadersLimit"
//@}

/// setting this prevents the filters from expuning the msgs automatically
#define MP_SAFE_FILTERS_NAME "SafeFilters"

/** @name timeout values for c-client mail library */
//@{
/// IMAP lookahead value
#define MP_IMAP_LOOKAHEAD_NAME "IMAPlookahead"
/// TCP/IP open timeout in seconds.
#define MP_TCP_OPENTIMEOUT_NAME "TCPOpenTimeout"
/// TCP/IP read timeout in seconds.
#define MP_TCP_READTIMEOUT_NAME "TCPReadTimeout"
/// TCP/IP write timeout in seconds.
#define  MP_TCP_WRITETIMEOUT_NAME "TCPWriteTimeout"
/// TCP/IP close timeout in seconds.
#define  MP_TCP_CLOSETIMEOUT_NAME "TCPCloseTimeout"
/// rsh connection timeout in seconds.
#define MP_TCP_RSHTIMEOUT_NAME "TCPRshTimeout"
/// ssh connection timeout in seconds.
#define MP_TCP_SSHTIMEOUT_NAME "TCPSshTimeout"
/// the path to rsh
#define MP_RSH_PATH_NAME "RshPath"
/// the path to ssh
#define MP_SSH_PATH_NAME "SshPath"
//@}
/** @name for folder list ctrls: ratios of the width to use for
    columns */
//@{
/// status
#define   MP_FLC_STATUSWIDTH_NAME   "ColumnWidthStatus"
/// subject
#define   MP_FLC_SUBJECTWIDTH_NAME   "ColumnWidthSubject"
/// from
#define   MP_FLC_FROMWIDTH_NAME   "ColumnWidthFrom"
/// date
#define   MP_FLC_DATEWIDTH_NAME   "ColumnWidthDate"
/// size
#define   MP_FLC_SIZEWIDTH_NAME   "ColumnWidthSize"
//@}
/** @name for folder list ctrls: column numbers */
//@{
/// status
#define   MP_FLC_STATUSCOL_NAME   "ColumnStatus"
/// subject
#define   MP_FLC_SUBJECTCOL_NAME   "ColumnSubject"
/// from
#define   MP_FLC_FROMCOL_NAME   "ColumnFrom"
/// date
#define   MP_FLC_DATECOL_NAME   "ColumnDate"
/// size
#define   MP_FLC_SIZECOL_NAME   "ColumnSize"
/// msgno
#define   MP_FLC_MSGNOCOL_NAME   "ColumnNum"
//@}
//@}
/// an entry used for testing
#define MP_TESTENTRY_NAME      "TestEntry"
/// Do we remote synchronise configuration settings?
#define MP_SYNC_REMOTE_NAME   "SyncRemote"
/// IMAP folder to sync to
#define MP_SYNC_FOLDER_NAME   "SyncFolder"
/// our last sync date
#define MP_SYNC_DATE_NAME      "SyncDate"
/// sync filters?
#define MP_SYNC_FILTERS_NAME   "SyncFilters"
/// sync filters?
#define MP_SYNC_IDS_NAME   "SyncIds"
/// sync folders?
#define MP_SYNC_FOLDERS_NAME   "SyncFolders"
/// sync folder tree
#define MP_SYNC_FOLDERGROUP_NAME   "SyncFolderGroup"

/** @name sending */
//@{
/// confirm sending the message?
#define MP_CONFIRM_SEND_NAME "ConfirmSend"
/// preview the message being sent?
#define MP_PREVIEW_SEND_NAME "PreviewSend"
//@}

/** @name away mode */
//@{
/// automatically enter the away mode after that many minutes (0 to disable)
#define MP_AWAY_AUTO_ENTER_NAME "AutoAway"
/// automaticlly exit from the away mode when the user chooses menu command
#define MP_AWAY_AUTO_EXIT_NAME "AutoExitAway"
/// keep the value of away mode for the next run (otherwise always reset it)
#define MP_AWAY_REMEMBER_NAME "RememberAway"
/// the saved value of away status (only written if MP_AWAY_REMEMBER_NAME is set)
#define MP_AWAY_STATUS_NAME "AwayStatus"
//@}

/** @name names of obsolete configuration entries, for upgrade routines */
//@{
#define MP_OLD_FOLDER_HOST_NAME "HostName"
//@}

/// stop "folder internal data" message
#define MP_CREATE_INTERNAL_MESSAGE_NAME   "CreateInternalMessage"

/// name of addressbook to use in whitelist spam filter
#define MP_WHITE_LIST_NAME "WhiteList"

/// treat mail in this folder as junk mail
#define MP_TREAT_AS_JUNK_MAIL_FOLDER_NAME "TreatAsJunkMailFolder"

//@}

// ----------------------------------------------------------------------------
// the option default values
// ----------------------------------------------------------------------------

/** @name default values of configuration entries */
//@{
/// The Profile Type. [OBSOLETE]
#define   MP_PROFILE_TYPE_DEFVAL      0l
/// The type of the config source
#define   MP_CONFIG_SOURCE_TYPE_DEFVAL ""
/// The priority of the config source
#define   MP_CONFIG_SOURCE_PRIO_DEFVAL 0l
/// our version
#define   MP_VERSION_DEFVAL          ""
/// are we running for the first time?
#define   MP_FIRSTRUN_DEFVAL         1
/// shall we record default values in configuration files
#define   MP_RECORDDEFAULTS_DEFVAL      0l
/// expand env vars in entries read from config?
#ifdef OS_WIN
#define   MP_EXPAND_ENV_VARS_DEFVAL 0l
#else
#define   MP_EXPAND_ENV_VARS_DEFVAL 1l
#endif
/// default window position x
#define   MP_XPOS_DEFVAL        20
/// default window position y
#define   MP_YPOS_DEFVAL        50
/// default window width
#define   MP_WIDTH_DEFVAL   600
/// default window height
#define   MP_HEIGHT_DEFVAL   400
/// window iconisation status
#define   MP_ICONISED_DEFVAL 0l
/// window maximized?
#define   MP_MAXIMISED_DEFVAL 0l
/// tool bars shown?
#define   MP_SHOW_TOOLBAR_DEFVAL    1L
/// status bars shown?
#define   MP_SHOW_STATUSBAR_DEFVAL  1L
/// window shown full-screen?
#define   MP_SHOW_FULLSCREEN_DEFVAL 0L
/// show log window?
#define   MP_SHOWLOG_DEFVAL  1
/// the file to save log messages to (if not empty)
#define   MP_LOGFILE_DEFVAL ""
/// debug protocols and folder access?
#define   MP_DEBUG_CCLIENT_DEFVAL   0l
/// open ADB editor on startup?
#define   MP_SHOWADBEDITOR_DEFVAL 0L
/// show tips at startup?
#define MP_SHOWTIPS_DEFVAL 1L
/// the index of the last tip which was shown
#define MP_LASTTIP_DEFVAL 0L
/// help browser kind
#define   MP_HELPBROWSER_KIND_DEFVAL 0l // internal
/// help browser name
#define   MP_HELPBROWSER_DEFVAL   "netscape"
/// is help browser of netscape type?
#define   MP_HELPBROWSER_ISNS_DEFVAL   1
/// width of help frame
#define MP_HELPFRAME_WIDTH_DEFVAL 560l
/// height of help frame
#define MP_HELPFRAME_HEIGHT_DEFVAL    500l
/// xpos of help frame
#define MP_HELPFRAME_XPOS_DEFVAL 40l
/// ypos of help frame
#define MP_HELPFRAME_YPOS_DEFVAL  40l
/// the directory for mbox folders
#define   MP_MBOXDIR_DEFVAL         ""
/// the news spool directory
#define MP_NEWS_SPOOL_DIR_DEFVAL ""
/// command to convert tiff faxes to postscript
#define   MP_TIFF2PS_DEFVAL         "convert %s %s"
/// preferred intermediary image format in conversions
#define MP_TMPGFXFORMAT_DEFVAL      2
/// expand folder tree control?
#define   MP_EXPAND_TREECTRL_DEFVAL   1
/// focus follows mouse?
#define MP_FOCUS_FOLLOWSMOUSE_DEFVAL    0l
/// dockable menu bars?
#define   MP_DOCKABLE_MENUBARS_DEFVAL   1l
/// dockable tool bars?
#define   MP_DOCKABLE_TOOLBARS_DEFVAL   1l
/// flat tool bars?
#define   MP_FLAT_TOOLBARS_DEFVAL      1l

/// the user's M directory
#define   MP_USERDIR_DEFVAL         ""

/// the acceptance status of the license
#define MP_LICENSE_ACCEPTED_DEFVAL   0l

/// run onl one copy of the program at once?
#define MP_RUNONEONLY_DEFVAL 1l

/// show images in the toolbar
#define MP_TBARIMAGES_DEFVAL 0l // == TbarShow_Icons - 1

// Unix-only entries
#ifdef OS_UNIX
/// the complete path to the global M directory
#  define   MP_GLOBALDIR_DEFVAL      M_PREFIX
/// the path where to find .afm files
#  define   MP_AFMPATH_DEFVAL M_BASEDIR "/afm:/usr/share:/usr/lib:/usr/local/share:/usr/local/lib:/opt/ghostscript:/opt/enscript"
#else // !Unix
/// the complete path to the global M directory
#  define   MP_GLOBALDIR_DEFVAL  ""
#endif // Unix/!Unix

/// the directory containing the help files
#define MP_HELPDIR_DEFVAL ""

/// Which encryption algorithm to use : 0 = simple builtin, 1 = twofish
#define MP_CRYPTALGO_DEFVAL   0L
/// DoesTwoFish work? (-1 unknown, 0 no, 1 yes)
#define MP_CRYPT_TWOFISH_OK_DEFVAL -1
/// some test data
#define MP_CRYPT_TESTDATA_DEFVAL ""
/// the locale for translation to national languages
#define   MP_LOCALE_DEFVAL               ""
/// the default character set
#define MP_CHARSET_DEFVAL               "ISO-8859-1"
/// the default icon for frames
#define   MP_ICON_MFRAME_DEFVAL      "mframe.xpm"
/// the icon for the main frame
#define   MP_ICON_MAINFRAME_DEFVAL      "mainframe.xpm"
/// the icon directoy
#define   MP_ICONPATH_DEFVAL         "icons"
/// the profile path
#define   MP_PROFILE_PATH_DEFVAL      "."
/// the extension to use for profile files
#define   MP_PROFILE_EXTENSION_DEFVAL      ".profile"
/// the key for identity redirection
#define MP_PROFILE_IDENTITY_DEFVAL  ""
/// the name of the mailcap file
#define   MP_MAILCAP_DEFVAL         "mailcap"
/// the name of the mime types file
#define   MP_MIMETYPES_DEFVAL         "mime.types"
/// the strftime() format for dates
#define   MP_DATE_FMT_DEFVAL         "%c"
/// display all dates as GMT?
#define   MP_DATE_GMT_DEFVAL         0l
/// show console window
#define   MP_SHOWCONSOLE_DEFVAL      1
/// open any folders at all on startup?
#define   MP_DONTOPENSTARTUP_DEFVAL   0l
/// names of folders to open at startup
#define   MP_OPENFOLDERS_DEFVAL      ""
/// reopen the last opened folder in the main frame
#define   MP_REOPENLASTFOLDER_DEFVAL 1
/// name of folder to open in mainframe
#define   MP_MAINFOLDER_DEFVAL        ""
/// path for Python
#define   MP_PYTHONPATH_DEFVAL         ""
/// Python DLL
#define   MP_PYTHONDLL_DEFVAL         ""
#ifdef OS_WIN
// under Windows, the setup program will create the appropriate registry key
#define   MP_USEPYTHON_DEFVAL         0l
#else
/// is Python enabled (this is a run-time option)?
#define   MP_USEPYTHON_DEFVAL         1
#endif
/// start-up script to run
#define     MP_PYTHONMODULE_TO_LOAD_DEFVAL   "Minit"
/// show splash screen on startup?
#define     MP_SHOWSPLASH_DEFVAL      1
/// how long should splash screen stay (0 disables timeout)?
#define MP_SPLASHDELAY_DEFVAL        5
/// how often should we autosave the profile settings (0 to disable)?
#define   MP_AUTOSAVEDELAY_DEFVAL       60
/// how often should we check for incoming mail (secs, 0 to disable)?
#define   MP_POLLINCOMINGDELAY_DEFVAL       300
/// poll folder only if it is opened
#define   MP_POLL_OPENED_ONLY_DEFVAL 0l
/// collect all new mail at startup?
#define   MP_COLLECTATSTARTUP_DEFVAL 0l
/// ask user if he really wants to exit?
#define   MP_CONFIRMEXIT_DEFVAL      1l
/// open folders when they're clicked (otherwise - double clicked)
#define   MP_OPEN_ON_CLICK_DEFVAL     0l
/// show all folders (even hidden ones) in the folder tree?
#define   MP_SHOW_HIDDEN_FOLDERS_DEFVAL 0l
/// create .profile files?
#define   MP_CREATE_PROFILES_DEFVAL   0l
/// umask setting for normal files
#define   MP_UMASK_DEFVAL               022
/// the last selected message in the folder view (or -1)
#define   MP_LASTSELECTED_MESSAGE_DEFVAL -1
/// automatically show th last selected message in folderview?
#define   MP_AUTOSHOW_LASTSELECTED_DEFVAL 1l
/// automatically show first message in folderview?
#define   MP_AUTOSHOW_FIRSTMESSAGE_DEFVAL 0l
/// automatically show first unread message in folderview?
#define   MP_AUTOSHOW_FIRSTUNREADMESSAGE_DEFVAL 1l
/// open messages when they're clicked (otherwise - double clicked)
#define   MP_PREVIEW_ON_SELECT_DEFVAL     1l
/// select the initially focused message
#define   MP_AUTOSHOW_SELECT_DEFVAL 0l
/// program used to convert image files?
#define   MP_CONVERTPROGRAM_DEFVAL      "convert %s -compress None %s"
/// list of modules to load at startup
#define MP_MODULES_DEFVAL   "Filters:Migrate"
/// list of modules to not load
#define MP_MODULES_DONT_LOAD_DEFVAL ""
/// the user path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_USER_DEFVAL   ""
/// the global path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_GLOBAL_DEFVAL   ""

/// the format string for the folder tree display
#define MP_FOLDERSTATUS_TREE_DEFVAL _("%f (%t, %u)")
/// the format string for status bar folder status display
#define MP_FOLDERSTATUS_STATBAR_DEFVAL _("%f (%t messages, %u unread, %n new)")
/// the format string for title bar folder status display
#define MP_FOLDERSTATUS_TITLEBAR_DEFVAL _("%f (%u unread, %n new)")

/**@name Printer settings */
//@{
/// Command
#define MP_PRINT_COMMAND_DEFVAL "lpr"
/// Options
#define MP_PRINT_OPTIONS_DEFVAL   ""
/// Orientation
#define MP_PRINT_ORIENTATION_DEFVAL 1  /* wxPORTRAIT */
/// print mode
#define MP_PRINT_MODE_DEFVAL 0L  /* PS_PRINTER */
/// paper name
#define MP_PRINT_PAPER_DEFVAL 0L
/// paper name
#define MP_PRINT_FILE_DEFVAL "mahogany.ps"
/// print in colour?
#define MP_PRINT_COLOUR_DEFVAL 1
/// top margin
#define MP_PRINT_TOPMARGIN_X_DEFVAL    0l
/// left margin
#define MP_PRINT_TOPMARGIN_Y_DEFVAL   0l
/// bottom margin
#define MP_PRINT_BOTTOMMARGIN_X_DEFVAL   0l
/// right margin
#define MP_PRINT_BOTTOMMARGIN_Y_DEFVAL 0l
/// zoom level in print preview
#define MP_PRINT_PREVIEWZOOM_DEFVAL   100l
//@}
/**@name for BBDB address book support */
//@{
/// generate unique names
#define   MP_BBDB_GENERATEUNIQUENAMES_DEFVAL   0L
/// ignore entries without names
#define   MP_BBDB_IGNOREANONYMOUS_DEFVAL      0L
/// name for anonymous entries, when neither first nor family name are set
#define   MP_BBDB_ANONYMOUS_DEFVAL   "anonymous"
/// save on exit, 0=no, 1=ask, 2=always
#define   MP_BBDB_SAVEONEXIT_DEFVAL  (long)M_ACTION_PROMPT
//@}
/**@name For Profiles: */
//@{
//@}
/// the user's full name
#define   MP_PERSONALNAME_DEFVAL      ""
/// organization (the "Organization:" header name value)
#define   MP_ORGANIZATION_DEFVAL     ""
/// the current user identity
#define   MP_CURRENT_IDENTITY_DEFVAL ""
/// the user's qualification (see M_USERLEVEL_XXX)
#define   MP_USERLEVEL_DEFVAL        (long)M_USERLEVEL_NOVICE
/// the username/login
#define   MP_USERNAME_DEFVAL         ""
/// the user's hostname
#define   MP_HOSTNAME_DEFVAL         ""
/// Add this hostname for addresses without hostname?
#define   MP_ADD_DEFAULT_HOSTNAME_DEFVAL 1L
/// (the username for returned mail) E-mail address
#define   MP_FROM_ADDRESS_DEFVAL      ""
/// Reply address
#define   MP_REPLY_ADDRESS_DEFVAL      ""
/// the default POP3 host
#define   MP_POPHOST_DEFVAL          "pop"
/// don't use AUTH with POP3
#define   MP_POP_NO_AUTH_DEFVAL     0l
/// the default IMAP4 host
#define   MP_IMAPHOST_DEFVAL          "imap"
/// use SSL for POP/IMAP?
#define   MP_USE_SSL_DEFVAL       1l // SSLSupport_TLSIfAvailable
/// accept unsigned SSL certificates?
#define   MP_USE_SSL_UNSIGNED_DEFVAL 0l
/// the mail host
#define   MP_SMTPHOST_DEFVAL         "mail"
/// use the specified sender value or guess it automatically?
#define   MP_GUESS_SENDER_DEFVAL  1l
/// the smtp sender value
#define   MP_SENDER_DEFVAL           ""
/// the mail host user-id
#define   MP_SMTPHOST_LOGIN_DEFVAL   ""
/// the mail host password
#define   MP_SMTPHOST_PASSWORD_DEFVAL   ""
/// use SSL?
#define   MP_SMTPHOST_USE_SSL_DEFVAL   1l // SSLSupport_TLSIfAvailable
/// check ssl-certs for SMTP connections?
#define   MP_SMTPHOST_USE_SSL_UNSIGNED_DEFVAL   0l
/// use ESMTP 8BITMIME extension if available
#define   MP_SMTP_USE_8BIT_DEFVAL 1l
/// disabled SMTP authentificators
#define   MP_SMTP_DISABLED_AUTHS_DEFVAL ""
/// sendmail command  FIXME - should be detected by configure
#ifdef OS_LINUX
#  define MP_SENDMAILCMD_DEFVAL "/usr/sbin/sendmail -t"
#else
#  define MP_SENDMAILCMD_DEFVAL "/usr/lib/sendmail -t"
#endif
/// use sendmail?
#ifdef OS_UNIX
#  define MP_USE_SENDMAIL_DEFVAL 1l
#else
#  define MP_USE_SENDMAIL_DEFVAL 0l
#endif
/// the news server
#define   MP_NNTPHOST_DEFVAL      "news"
/// the news server user-id
#define   MP_NNTPHOST_LOGIN_DEFVAL      ""
/// the news server password
#define   MP_NNTPHOST_PASSWORD_DEFVAL      ""
/// use SSL?
#define   MP_NNTPHOST_USE_SSL_DEFVAL   1l // SSLSupport_TLSIfAvailable
/// check ssl-certs for NNTP connections?
#define   MP_NNTPHOST_USE_SSL_UNSIGNED_DEFVAL   0l
/// the beacon host to test for net connection
#define   MP_BEACONHOST_DEFVAL      ""
#ifdef USE_DIALUP
/// does Mahogany control dial-up networking?
#define MP_DIALUP_SUPPORT_DEFVAL   0L
#endif // USE_DIALUP

/// set reply string from To: field?
#define MP_SET_REPLY_FROM_TO_DEFVAL   0l
/// set reply address only?
#define MP_SET_REPLY_STD_NAME_DEFVAL  1l
/// should we attach vCard to outgoing messages?
#define MP_USEVCARD_DEFVAL 0l
/// the vCard to use
#define MP_VCARD_DEFVAL "vcard.vcf"

/// use the folder create wizard (or the dialog directly)?
#define MP_USE_FOLDER_CREATE_WIZARD_DEFVAL 1l

#ifdef USE_DIALUP

#if defined(OS_WIN)
/// the RAS connection to use
#define MP_NET_CONNECTION_DEFVAL ""
#elif defined(OS_UNIX)
/// the command to go online
#define MP_NET_ON_COMMAND_DEFVAL   "wvdial"
/// the command to go offline
#define MP_NET_OFF_COMMAND_DEFVAL   "killall pppd"
#endif // platform

#endif // USE_DIALUP

/// login for mailbox
#define   MP_FOLDER_LOGIN_DEFVAL      ""
/// passwor for mailbox
#define   MP_FOLDER_PASSWORD_DEFVAL      ""
/// log level
#define   MP_LOGLEVEL_DEFVAL         0l
/// show busy info while sorting/threading?
#define   MP_SHOWBUSY_DURING_SORT_DEFVAL 1l
/// threshold for displaying mailfolder progress dialog
#define   MP_FOLDERPROGRESS_THRESHOLD_DEFVAL 20L
/// threshold for displaying message retrieval progress dialog (kbytes)
#define   MP_MESSAGEPROGRESS_THRESHOLD_SIZE_DEFVAL  40l
/// threshold for displaying message retrieval progress dialog (seconds)
#define   MP_MESSAGEPROGRESS_THRESHOLD_TIME_DEFVAL  1l
/// the default path for saving files
#define   MP_DEFAULT_SAVE_PATH_DEFVAL      ""
/// the default filename for saving files
#define   MP_DEFAULT_SAVE_FILENAME_DEFVAL   "*"
/// the default extension for saving files
#define   MP_DEFAULT_SAVE_EXTENSION_DEFVAL   ""
/// the default path for saving files
#define   MP_DEFAULT_LOAD_PATH_DEFVAL      ""
/// the default filename for saving files
#define   MP_DEFAULT_LOAD_FILENAME_DEFVAL   "*"
/// the default extension for saving files
#define   MP_DEFAULT_LOAD_EXTENSION_DEFVAL   ""
/// default value for To: field in composition
#define   MP_COMPOSE_TO_DEFVAL         ""
/// default value for Cc: field in composition
#define   MP_COMPOSE_CC_DEFVAL         ""
/// default value for Bcc: field in composition
#define   MP_COMPOSE_BCC_DEFVAL      ""
/// show "From:" field in composer?
#define   MP_COMPOSE_SHOW_FROM_DEFVAL 0l

/// default reply kind
#define   MP_DEFAULT_REPLY_KIND_DEFVAL 0l  // MailFolder::REPLY_SENDER
/// the mailing list addresses
#define   MP_LIST_ADDRESSES_DEFVAL ""
/// array of equivalent address pairs
#define   MP_EQUIV_ADDRESSES_DEFVAL ""
/// prefix for subject in replies
#define   MP_REPLY_PREFIX_DEFVAL      "Re: "
/// prefix for subject in forwards
#define   MP_FORWARD_PREFIX_DEFVAL      _("Forwarded message: ")
/// collapse reply prefixes? 0=no, 1=replace "Re"s with one, 2=use reply level
#define   MP_REPLY_COLLAPSE_PREFIX_DEFVAL 2l
/// include the original message in the reply [no,ask,yes]
#define MP_REPLY_QUOTE_ORIG_DEFVAL (long)M_ACTION_ALWAYS
/// include only the selected text (if any) in the reply?
#define MP_REPLY_QUOTE_SELECTION_DEFVAL true
/// prefix for text in replies
#define   MP_REPLY_MSGPREFIX_DEFVAL      "> "
/// use the value of X-Attribution header as the prefix
#define   MP_REPLY_MSGPREFIX_FROM_XATTR_DEFVAL 1l
/// prepend the initials of the sender to the reply prefix?
#define   MP_REPLY_MSGPREFIX_FROM_SENDER_DEFVAL 0l
/// quote the empty lines when replying?
#define   MP_REPLY_QUOTE_EMPTY_DEFVAL      1l
/// use signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_DEFVAL   1
/// filename of signature file
#ifdef OS_WIN
#  define   MP_COMPOSE_SIGNATURE_DEFVAL      ""
#else
#  define   MP_COMPOSE_SIGNATURE_DEFVAL      "$HOME/.signature"
#endif
/// use "-- " to separate signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_SEPARATOR_DEFVAL   1
/// show PGP-related controls in the composer?
#define   MP_COMPOSE_USE_PGP_DEFVAL   1l
/// sign messages with PGP?
#define   MP_COMPOSE_PGPSIGN_DEFVAL   1l
/// the user name to sign messages with PGP as
#define   MP_COMPOSE_PGPSIGN_AS_DEFVAL   ""

/// use XFace in composition?
#define   MP_COMPOSE_USE_XFACE_DEFVAL   1
/// Name from where to read XFace
#define   MP_COMPOSE_XFACE_FILE_DEFVAL   "$HOME/.xface"
/// the folder type for a mailbox
#define   MP_FOLDER_TYPE_DEFVAL         (long)(0x00ff)  // MF_ILLEGAL
/// should we try to create the folder before opening it?
#define   MP_FOLDER_TRY_CREATE_DEFVAL 0L
/// the folder icon for a mailbox (see icon functions in FolderType.h)
#define   MP_FOLDER_ICON_DEFVAL         (long)-1        // no special icon
/// the position of the folder in the tree
#define MP_FOLDER_TREEINDEX_DEFVAL      (long)-1        // see enum MFolderIndex
/// Move new mail to the NewMail folder (if not, only copy)?
#define MP_MOVE_NEWMAIL_DEFVAL      1l
/// Where to store all new mail
#define MP_NEWMAIL_FOLDER_DEFVAL      "" // obsolete
/// Which folder to use as Outbox
#define MP_OUTBOX_NAME_DEFVAL         ""
/// Use Outbox?
#define MP_USE_OUTBOX_DEFVAL            0l
/// Name of Trash folder?
#define MP_TRASH_FOLDER_DEFVAL      ""
/// Use a trash folder?
#define MP_USE_TRASH_FOLDER_DEFVAL   1l
/// Name of the Drafts folder
#define MP_DRAFTS_FOLDER_DEFVAL ""
/// Delete the drafts automatically after the message was sent?
#define MP_DRAFTS_AUTODELETE_DEFVAL  1l
/// the filename for a mailbox
#define   MP_FOLDER_PATH_DEFVAL      ((const char *)NULL) // don't change this!
/// comment
#define   MP_FOLDER_COMMENT_DEFVAL      ""
/// update interval for folders in seconds
#define   MP_UPDATEINTERVAL_DEFVAL      60
/// close of folders delayed by
#define MP_FOLDER_CLOSE_DELAY_DEFVAL    0l
/// close of network connection delayed by
#define MP_CONN_CLOSE_DELAY_DEFVAL    60
/// Wrap quoted lines?
#define MP_AUTOMATIC_WORDWRAP_DEFVAL   1l
/// do automatic word wrap?
#define MP_WRAP_QUOTED_DEFVAL 0l
/// wrapmargin for composition (set to -1 to disable it)
#define   MP_WRAPMARGIN_DEFVAL      75
/// do automatic word wrap in message view?
#define MP_VIEW_AUTOMATIC_WORDWRAP_DEFVAL   1l
/// wrapmargin for message view (set to -1 to disable it)
#define   MP_VIEW_WRAPMARGIN_DEFVAL      100
/// show TEXT/PLAIN as inlined text?
#define   MP_PLAIN_IS_TEXT_DEFVAL      1l
/// show MESSAGE/RFC822 as text?
#define   MP_RFC822_IS_TEXT_DEFVAL      0l
/// show decorations around inlined MESSAGE/RFC822 text?
#define   MP_RFC822_DECORATE_DEFVAL 1L
/// show headers of the inlined MESSAGE/RFC822 text?
#define   MP_RFC822_SHOW_HEADERS_DEFVAL 1L
/// show XFaces?
#define   MP_SHOW_XFACES_DEFVAL      1
/// show graphics inline
#define   MP_INLINE_GFX_DEFVAL       1
/// show the external images (for HTML viewer only) too?
#define   MP_INLINE_GFX_EXTERNAL_DEFVAL 0l
/// limit size for inline graphics in Kb (-1 for no limit)
#define   MP_INLINE_GFX_SIZE_DEFVAL  100
/// show viewer bar in the message view?
#define MP_MSGVIEW_SHOWBAR_DEFVAL 1
/// which viewer to use in the message view?
#define MP_MSGVIEW_VIEWER_DEFVAL "LayoutViewer"
/// select the "best" viewer automatically?
#define MP_MSGVIEW_AUTO_VIEWER_DEFVAL 1l
/// use HTML viewer whenever there is HTML content?
#define MP_MSGVIEW_PREFER_HTML_DEFVAL 0l
/// allow HTML viewer when there is only HTML content?
#define MP_MSGVIEW_ALLOW_HTML_DEFVAL 1l
/// allow image-capable viewer when there are inline images?
#define MP_MSGVIEW_ALLOW_IMAGES_DEFVAL 1l
/// which headers to show in the message view?
#define   MP_MSGVIEW_HEADERS_DEFVAL     \
          "Date:" \
          "From:" \
          "Subject:" \
          "Newsgroups:" \
          "To:" \
          "Cc:" \
          "Bcc:" \
          "Reply-To:"
/// all headers we know about
#define   MP_MSGVIEW_ALL_HEADERS_DEFVAL \
          MP_MSGVIEW_HEADERS_DEFVAL \
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
          "Followup-To:" \
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
/// the default encoding for the viewer/composer
#define  MP_MSGVIEW_DEFAULT_ENCODING_DEFVAL (long)wxFONTENCODING_DEFAULT
/// the type of the last created folder
#define   MP_LAST_CREATED_FOLDER_TYPE_DEFVAL  (long)MF_FILE
/// the filter program to apply (OBSOLETE)
#define MP_FILTER_RULE_DEFVAL   ""
/// the filters to use for this folder
#define MP_FOLDER_FILTERS_DEFVAL   ""
/// the default folder file format
#define MP_FOLDER_FILE_DRIVER_DEFVAL 0L
/* format: mbx:unix:mmdf:tenex defined in MailFolderCC.cpp */
/**@name  Font settings for message view */
//@{
/// message view title
#define   MP_MVIEW_TITLE_FMT_DEFVAL   _("from $from about \"$subject\"")
/// which font to use
#define   MP_MVIEW_FONT_DEFVAL         0L
/// which font size
#define   MP_MVIEW_FONT_SIZE_DEFVAL         -1
/// the full font desc (replaces the 2 settings above)
#define   MP_MVIEW_FONT_DESC_DEFVAL   ""
/// which foreground colour for the font
#define   MP_MVIEW_FGCOLOUR_DEFVAL      "black"
/// which background colour for the font
#define   MP_MVIEW_BGCOLOUR_DEFVAL      "white"
/// which colour for signature
#define   MP_MVIEW_SIGCOLOUR_DEFVAL     "thistle"
/// which colour for URLS
#define   MP_MVIEW_URLCOLOUR_DEFVAL     "blue"
/// colour for attachment labels
#define   MP_MVIEW_ATTCOLOUR_DEFVAL     "dark olive green"

/// colorize quoted text?
#define   MP_MVIEW_QUOTED_COLOURIZE_DEFVAL 1L
/// cycle colours?
#define   MP_MVIEW_QUOTED_CYCLE_COLOURS_DEFVAL  0L
/// which colour for quoted text
#define   MP_MVIEW_QUOTED_COLOUR1_DEFVAL     "gray"
/// which colour for quoted text, seconds level
#define   MP_MVIEW_QUOTED_COLOUR2_DEFVAL     "brown"
/// which colour for quoted text, seconds level
#define   MP_MVIEW_QUOTED_COLOUR3_DEFVAL     "purple"
/// the maximum number of whitespaces prepending >
#define   MP_MVIEW_QUOTED_MAXWHITESPACE_DEFVAL  2L
/// the maximum number of A-Z prepending >
#define   MP_MVIEW_QUOTED_MAXALPHA_DEFVAL    3L
/// the colour for header names in the message view
#define   MP_MVIEW_HEADER_NAMES_COLOUR_DEFVAL  "RGB(128,64,64)"
/// the colour for header values in the message view
#define   MP_MVIEW_HEADER_VALUES_COLOUR_DEFVAL "black"
//@}
/**@name  Font settings for message view */
//@{
/// which font to use
#define   MP_FVIEW_FONT_DEFVAL         0L
/// which font size
#define   MP_FVIEW_FONT_SIZE_DEFVAL         -1
/// the full font desc (replaces the 2 settings above)
#define   MP_FVIEW_FONT_DESC_DEFVAL   ""
/// don't show full e-mail, only sender's name
#define   MP_FVIEW_NAMES_ONLY_DEFVAL         0L
/// which foreground colour for the font
#define   MP_FVIEW_FGCOLOUR_DEFVAL      "black"
/// which background colour for the font
#define   MP_FVIEW_BGCOLOUR_DEFVAL      "white"
/// colour for deleted messages
#define   MP_FVIEW_DELETEDCOLOUR_DEFVAL      "grey"
/// colour for new messages
#define   MP_FVIEW_NEWCOLOUR_DEFVAL      "orange"
/// colour for recent messages
#define   MP_FVIEW_RECENTCOLOUR_DEFVAL      "gold"
/// colour for unread messages
#define   MP_FVIEW_UNREADCOLOUR_DEFVAL      "blue"
/// colour for flagged messages
#define   MP_FVIEW_FLAGGEDCOLOUR_DEFVAL      "purple"
/// automatically select next unread message after finishing the current one
#define MP_FVIEW_AUTONEXT_UNREAD_MSG_DEFVAL 1L
/// automatically select next unread folder after finishing the current one
#define MP_FVIEW_AUTONEXT_UNREAD_FOLDER_DEFVAL 0L
/// how to show the size (MessageSizeShow enum value)
#define MP_FVIEW_SIZE_FORMAT_DEFVAL  0l
/// update the folder view status bar to show the msg info?
#define   MP_FVIEW_STATUS_UPDATE_DEFVAL 0L
/// folder view status bar string
#define   MP_FVIEW_STATUS_FMT_DEFVAL _("Date: $date, Subject: $subject, From: $from")
/// delay before previewing the selected item in the folder view (0 to disable)
#define MP_FVIEW_PREVIEW_DELAY_DEFVAL 500L
/// split folder view vertically (or horizontally)?
#define MP_FVIEW_VERTICAL_SPLIT_DEFVAL 0L
/// put folder view on top and msg view on bottom or vice versa?
#define MP_FVIEW_FVIEW_TOP_DEFVAL 1L
/// Should many commands like copy,mark (un)read, etc., automatically go to the next message
#define MP_FVIEW_AUTONEXT_ON_COMMAND_DEFVAL 1L

//@}
/**@name Font settings for folder tree */
//@{
/// is the folder tree on the left of folder view or on the right?
#define MP_FTREE_LEFT_DEFVAL 1L
/// the foreground colour for the folder tree
#define MP_FTREE_FGCOLOUR_DEFVAL ""
/// the background colour for the folder tree
#define MP_FTREE_BGCOLOUR_DEFVAL ""
/// show the currently opened folder specially?
#define MP_FTREE_SHOWOPENED_DEFVAL 1L
/// format for the folder tree entries
#define MP_FTREE_FORMAT_DEFVAL " (%t, %u)"
/// reflect the folder status in its parent?
#define MP_FTREE_PROPAGATE_DEFVAL 1L
/// skip this folder when looking for next unread one in the tree
#define MP_FTREE_NEVER_UNREAD_DEFVAL 0L
/// go to this folder when Ctrl-Home is pressed
#define MP_FTREE_HOME_DEFVAL ""
//@}
/**@name Font settings for compose view */
//@{
/// which font to use
#define   MP_CVIEW_FONT_DEFVAL         0L
/// which font size
#define   MP_CVIEW_FONT_SIZE_DEFVAL    -1
/// the full font desc (replaces the 2 settings above)
#define   MP_CVIEW_FONT_DESC_DEFVAL   ""
/// which foreground colour for the font
#define   MP_CVIEW_FGCOLOUR_DEFVAL      "black"
/// which background colour for the font
#define   MP_CVIEW_BGCOLOUR_DEFVAL      "white"
/// use the colours and font for the headers as well?
#define   MP_CVIEW_COLOUR_HEADERS_DEFVAL 0L
/// check for forgotten attachments?
#define   MP_CHECK_FORGOTTEN_ATTACHMENTS_DEFVAL 1L
/// regex to use for the attachments check
#define   MP_CHECK_ATTACHMENTS_REGEX_DEFVAL "\\<(attach|enclose)"
//@}
/// highlight signature?
#define   MP_HIGHLIGHT_SIGNATURE_DEFVAL      1
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS_DEFVAL      1
/// do we want to use server side sort?
#define MP_MSGS_SERVER_SORT_DEFVAL    1l
/// sort criterium for folder listing (== MSO_NONE)
#define MP_MSGS_SORTBY_DEFVAL         0l
/// re-sort messages on status change?
#define MP_MSGS_RESORT_ON_CHANGE_DEFVAL 0l
/// use threading
#define MP_MSGS_USE_THREADING_DEFVAL  1l
/// use server side threading?
#define MP_MSGS_SERVER_THREAD_DEFVAL 1L
/// only use server side threading by references (best threading method)?
#define MP_MSGS_SERVER_THREAD_REF_ONLY_DEFVAL 0L

/// Gather messages with same subject in one thread
#define MP_MSGS_GATHER_SUBJECTS_DEFVAL 1l
/// break thread when subject changes
#define MP_MSGS_BREAK_THREAD_DEFVAL 1l
/// Indent messages when common ancestor is missing
#define MP_MSGS_INDENT_IF_DUMMY_DEFVAL 0l

#if wxUSE_REGEX
#  define MP_MSGS_SIMPLIFYING_REGEX_DEFVAL "^ *(R[Ee](\\[[0-9]+\\])?: +)*(\\[[^][]+\\]+)?(R[Ee](\\[[0-9]+\\])?: +)*"
#  define MP_MSGS_REPLACEMENT_STRING_DEFVAL "\\3"
#else // wxUSE_REGEX
   /// Remove list prefix when comparing message's subject to gather them
#  define MP_MSGS_REMOVE_LIST_PREFIX_GATHERING_DEFVAL 1l
   /// Remove list prefix when comparing message's subject to break threads
#  define MP_MSGS_REMOVE_LIST_PREFIX_BREAKING_DEFVAL 1l
#endif // wxUSE_REGEX

/// search criterium for searching in folders
#define MP_MSGS_SEARCH_CRIT_DEFVAL   0l
/// search argument
#define MP_MSGS_SEARCH_ARG_DEFVAL   ""
/// open URLs with
#ifdef  OS_UNIX
#  define   MP_BROWSER_DEFVAL         "netscape"
#  define   MP_BROWSER_ISNS_DEFVAL    1
#  define   MP_BROWSER_INNW_DEFVAL    1
#else  // under Windows, we know better...
#  define   MP_BROWSER_DEFVAL         ""
#  define   MP_BROWSER_ISNS_DEFVAL    0l
#  define   MP_BROWSER_INNW_DEFVAL    1l
#endif // Unix/Win

/// external editor to use for message composition (use %s for filename)
#ifdef OS_UNIX
#  define   MP_EXTERNALEDITOR_DEFVAL  "gvim %s"
#else
#  define   MP_EXTERNALEDITOR_DEFVAL  "notepad %s"
#endif // Unix/Win

/// start external editor automatically?
#define MP_ALWAYS_USE_EXTERNALEDITOR_DEFVAL    0l

/// PGP/GPG application
#ifdef OS_UNIX
#  define   MP_PGP_COMMAND_DEFVAL  "gpg"
#else
#  define   MP_PGP_COMMAND_DEFVAL  "gpg.exe"
#endif // Unix/Win

/// PGP/GPG key server for public keys
#define MP_PGP_KEYSERVER_DEFVAL "wwwkeys.eu.pgp.net"

/// command to execute when new mail arrives
#define   MP_USE_NEWMAILCOMMAND_DEFVAL   0l
#define   MP_NEWMAILCOMMAND_DEFVAL   ""

/// play a sound on new mail?
#ifdef __CYGWIN__
#define MP_NEWMAIL_PLAY_SOUND_DEFVAL 0l
#else
#define MP_NEWMAIL_PLAY_SOUND_DEFVAL 1l
#endif // cygwin

#if defined(OS_UNIX) || defined(__CYGWIN__)
/// which sound to play?
#define MP_NEWMAIL_SOUND_FILE_DEFVAL M_BASEDIR "/newmail.wav"
/// the program to use to play this sound
#define MP_NEWMAIL_SOUND_PROGRAM_DEFVAL "/usr/bin/play %s"
#else // !OS_UNIX
/// which sound to play?
#define MP_NEWMAIL_SOUND_FILE_DEFVAL ""
#endif // OS_UNIX/!OS_UNIX

/// show new mail messages?
#define   MP_SHOW_NEWMAILMSG_DEFVAL      1
/// show detailed info about how many new mail messages?
#define   MP_SHOW_NEWMAILINFO_DEFVAL      10
/// consider only unseen messages as new?
#define   MP_NEWMAIL_UNSEEN_DEFVAL  1
/// collect mail from INBOX?
#define   MP_COLLECT_INBOX_DEFVAL 0l
/// keep copies of outgoing mail?
#define   MP_USEOUTGOINGFOLDER_DEFVAL  1
/// write outgoing mail to folder:
#define   MP_OUTGOINGFOLDER_DEFVAL  "" // obsolete
/// Show all message headers?
#define   MP_SHOWHEADERS_DEFVAL         0l
/// Autocollect email addresses?
#define   MP_AUTOCOLLECT_INCOMING_DEFVAL     (long)M_ACTION_ALWAYS
/// Name of the address books for autocollected addresses
#define   MP_AUTOCOLLECT_ADB_DEFVAL    "autocollect.adb"
/// Autocollect email addresses from sender only ?
#define   MP_AUTOCOLLECT_SENDER_DEFVAL     1l
/// Autocollect addresses in mail sent by user?
#define   MP_AUTOCOLLECT_OUTGOING_DEFVAL     1l
/// Autocollect entries with names only?
#define   MP_AUTOCOLLECT_NAMED_DEFVAL 0l

/// Default names for the SSL and crypto libraries
#ifdef OS_UNIX
   #define MP_SSL_DLL_SSL_DEFVAL   "libssl.so"
   #define MP_SSL_DLL_CRYPTO_DEFVAL "libcrypto.so"
#elif defined(OS_WIN)
   #if defined(__CYGWIN__)
      #define MP_SSL_DLL_SSL_DEFVAL   "cygssl.dll"
      #define MP_SSL_DLL_CRYPTO_DEFVAL "cygcrypto.dll"
   #else // !cygwin
      #define MP_SSL_DLL_SSL_DEFVAL   "ssleay32.dll"
      #define MP_SSL_DLL_CRYPTO_DEFVAL "libeay32.dll"
   #endif
#else // !Unix, !Win
   #define MP_SSL_DLL_SSL_DEFVAL   ""
   #define MP_SSL_DLL_CRYPTO_DEFVAL ""
#endif

/**@name message view settings */
//@{
/// support efax style incoming faxes
#define MP_INCFAX_SUPPORT_DEFVAL      1L
/// domains from which to support faxes, semicolon delimited
#define MP_INCFAX_DOMAINS_DEFVAL      "efax.com"
//@}
/// Use substrings in address expansion?
#define   MP_ADB_SUBSTRINGEXPANSION_DEFVAL 0l

/// replace "From" address with "To" in messages from oneself?
#define MP_FVIEW_FROM_REPLACE_DEFVAL 0L
/// the ':' separated list of addresses which are "from oneself"
#define MP_FROM_REPLACE_ADDRESSES_DEFVAL ""

/** @name maximal amounts of data to retrieve from remote servers */
//@{
/// ask confirmation before retrieveing messages bigger than this (in Kb)
#define MP_MAX_MESSAGE_SIZE_DEFVAL 100l
/// ask confirmation before retrieveing more headers than this
#define MP_MAX_HEADERS_NUM_DEFVAL  100l
/// never download more than that many messages
#define MP_MAX_HEADERS_NUM_HARD_DEFVAL 0l
//@}
/// setting this prevents the filters from expuning the msgs automatically
#define MP_SAFE_FILTERS_DEFVAL 0l
/** @name timeout values for c-client mail library */
//@{
/// IMAP lookahead value
#define MP_IMAP_LOOKAHEAD_DEFVAL 0l
/// TCP/IP open timeout in seconds.
#define MP_TCP_OPENTIMEOUT_DEFVAL      30l
/// TCP/IP read timeout in seconds.
#define MP_TCP_READTIMEOUT_DEFVAL       90l
/// TCP/IP write timeout in seconds.
#define  MP_TCP_WRITETIMEOUT_DEFVAL    90l
/// TCP/IP close timeout in seconds.
#define  MP_TCP_CLOSETIMEOUT_DEFVAL 60l
/// rsh connection timeout in seconds.
#define MP_TCP_RSHTIMEOUT_DEFVAL 0l
/// ssh connection timeout in seconds.
#define MP_TCP_SSHTIMEOUT_DEFVAL 0l
/// the path to rsh
#define MP_RSH_PATH_DEFVAL "rsh"
/// the path to ssh
#define MP_SSH_PATH_DEFVAL "ssh"
//@}

/** @name for folder list ctrls: ratios of the width to use for
    columns */
//@{
/// status
#define   MP_FLC_STATUSWIDTH_DEFVAL 10
/// subject
#define   MP_FLC_SUBJECTWIDTH_DEFVAL  33
/// from
#define   MP_FLC_FROMWIDTH_DEFVAL   20
/// date
#define   MP_FLC_DATEWIDTH_DEFVAL   27
/// size
#define   MP_FLC_SIZEWIDTH_DEFVAL   10
//@}
/** @name for folder list ctrls: column numbers */
//@{
/// status
#define   MP_FLC_STATUSCOL_DEFVAL  0l
/// subject
#define   MP_FLC_SUBJECTCOL_DEFVAL 1
/// from
#define   MP_FLC_FROMCOL_DEFVAL   2
/// date
#define   MP_FLC_DATECOL_DEFVAL   3
/// size
#define   MP_FLC_SIZECOL_DEFVAL   4
/// size
#define   MP_FLC_MSGNOCOL_DEFVAL   -1l
//@}

/// the wildcard for save dialog
#ifdef OS_UNIX
#  define   MP_DEFAULT_SAVE_WILDCARD_DEFVAL   "*"
#  define   MP_DEFAULT_LOAD_WILDCARD_DEFVAL   "*"
#else
#  define   MP_DEFAULT_SAVE_WILDCARD_DEFVAL   "*.*"
#  define   MP_DEFAULT_LOAD_WILDCARD_DEFVAL   "*.*"
#endif
/// an entry used for testing
#define MP_TESTENTRY_DEFVAL      0L
/// Do we remote synchronise configuration settings?
#define MP_SYNC_REMOTE_DEFVAL   0L
/// IMAP folder to sync to
#define MP_SYNC_FOLDER_DEFVAL   "MahoganySharedConfig"
/// our last sync date
#define MP_SYNC_DATE_DEFVAL      0L
/// sync filters?
#define MP_SYNC_FILTERS_DEFVAL   0L
/// sync filters?
#define MP_SYNC_IDS_DEFVAL   0L
/// sync folders?
#define MP_SYNC_FOLDERS_DEFVAL   0L
/// sync folder tree
#define MP_SYNC_FOLDERGROUP_DEFVAL   ""

/** @name sending */
//@{
/// confirm sending the message?
#define MP_CONFIRM_SEND_DEFVAL 0L
/// preview the message being sent?
#define MP_PREVIEW_SEND_DEFVAL 0L
//@}

/** @name away mode */
//@{
/// automatically enter the away mode after that many minutes (0 to disable)
#define MP_AWAY_AUTO_ENTER_DEFVAL 0L
/// automaticlly exit from the away mode when the user chooses menu command
#define MP_AWAY_AUTO_EXIT_DEFVAL 1L
/// keep the value of away mode for the next run (otherwise always reset it)
#define MP_AWAY_REMEMBER_DEFVAL 0L
/// the saved value of away status (only written if MP_AWAY_REMEMBER is set)
#define MP_AWAY_STATUS_DEFVAL 0L
//@}

/// stop "folder internal data" message
#define MP_CREATE_INTERNAL_MESSAGE_DEFVAL   1

/// name of addressbook to use in whitelist spam filter
#define MP_WHITE_LIST_DEFVAL ""

/// treat mail in this folder as junk mail
#define MP_TREAT_AS_JUNK_MAIL_FOLDER_DEFVAL 0L

//@}

