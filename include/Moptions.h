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

#ifdef OS_WIN
extern const MOption MP_USE_CONFIG_FILE;
#endif // OS_WIN

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
extern const MOption MP_USEPYTHON;
extern const MOption MP_STARTUPSCRIPT;
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
extern const MOption MP_SMTP_DISABLED_AUTHS;

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
extern const MOption MP_REPLY_PREFIX;
extern const MOption MP_FORWARD_PREFIX;
extern const MOption MP_REPLY_COLLAPSE_PREFIX;
extern const MOption MP_REPLY_QUOTE_ORIG;
extern const MOption MP_REPLY_QUOTE_SELECTION;
extern const MOption MP_REPLY_MSGPREFIX;
extern const MOption MP_REPLY_MSGPREFIX_FROM_XATTR;
extern const MOption MP_REPLY_MSGPREFIX_FROM_SENDER;
extern const MOption MP_REPLY_QUOTE_EMPTY;
extern const MOption MP_REPLY_DETECT_SIG;
extern const MOption MP_REPLY_SIG_SEPARATOR;
extern const MOption MP_COMPOSE_USE_SIGNATURE;
extern const MOption MP_COMPOSE_SIGNATURE;
extern const MOption MP_COMPOSE_USE_SIGNATURE_SEPARATOR;
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
extern const MOption MP_SHOW_XFACES;
extern const MOption MP_INLINE_GFX;
extern const MOption MP_INLINE_GFX_EXTERNAL;
extern const MOption MP_INLINE_GFX_SIZE;
extern const MOption MP_MSGVIEW_SHOWBAR;
extern const MOption MP_MSGVIEW_VIEWER;
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
extern const MOption MP_PGP_GET_PUBKEY;
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
extern const MOption MP_AUTOCOLLECT;
extern const MOption MP_AUTOCOLLECT_ADB;
extern const MOption MP_AUTOCOLLECT_NAMED;
extern const MOption MP_AUTOCOLLECT_SENDER;
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
#define MP_OPTION_SHOW_ORIGIN_NAME _T("ShowOptionOrigin")
#define MP_OPTION_SHOW_ORIGIN_DEFVAL 1l

// the colour for the options set at this folder level
extern const MOption MP_OPTION_ORIGIN_HERE;
#define MP_OPTION_ORIGIN_HERE_NAME _T("OptionSetHereCol")
#define MP_OPTION_ORIGIN_HERE_DEFVAL _T("blue")

// the colour for the options inherited from parent (but non default)
extern const MOption MP_OPTION_ORIGIN_INHERITED;
#define MP_OPTION_ORIGIN_INHERITED_NAME _T("OptionInheritedCol")
#define MP_OPTION_ORIGIN_INHERITED_DEFVAL _T("RGB(0, 0, 127)")

// ----------------------------------------------------------------------------
// the option names
// ----------------------------------------------------------------------------

/** @name names of configuration entries */
//@{
/// our version
#define   MP_VERSION_NAME          _T("Version")
/// are we running for the first time?
#define   MP_FIRSTRUN_NAME         _T("FirstRun")
/// shall we record default values in configuration files
#define   MP_RECORDDEFAULTS_NAME      _T("RecordDefaults")
/// expand env vars in entries read from config?
#define   MP_EXPAND_ENV_VARS_NAME  _T("ExpandEnvVars")
/// default position x
#define   MP_XPOS_NAME            _T("XPos")
/// default position y
#define   MP_YPOS_NAME            _T("YPos")
/// window width
#define   MP_WIDTH_NAME         _T("Width")
/// window height
#define   MP_HEIGHT_NAME         _T("Height")
/// window iconisation status
#define   MP_ICONISED_NAME         _T("Iconised")
/// window maximized?
#define   MP_MAXIMISED_NAME         _T("Maximised")

/// show log window?
#define   MP_SHOWLOG_NAME          _T("ShowLog")
/// the file to save log messages to (if not empty)
#define   MP_LOGFILE_NAME          _T("LogFile")
/// debug protocols and folder access?
#define   MP_DEBUG_CCLIENT_NAME   _T("MailDebug")

/// open ADB editor on startup?
#define   MP_SHOWADBEDITOR_NAME    _T("ShowAdb")

/// show tips at startup?
#define MP_SHOWTIPS_NAME _T("ShowTips")
/// the index of the last tip which was shown
#define MP_LASTTIP_NAME _T("LastTip")

/// expand folder tree control?
#define   MP_EXPAND_TREECTRL_NAME   _T("ExpandTreeControl")
/// focus follows mouse?
#define MP_FOCUS_FOLLOWSMOUSE_NAME  _T("FocusFollowsMouse")
/// dockable menu bars?
#define   MP_DOCKABLE_MENUBARS_NAME _T("MenuBarsDockable")
/// dockable tool bars?
#define   MP_DOCKABLE_TOOLBARS_NAME   _T("ToolBarsDockable")
/// flat tool bars?
#define   MP_FLAT_TOOLBARS_NAME   _T("ToolBarsFlat")

/// help browser kind
#define   MP_HELPBROWSER_KIND_NAME   _T("HelpBrowserKind")
/// help browser name
#define   MP_HELPBROWSER_NAME   _T("HelpBrowser")
/// is help browser of netscape type?
#define   MP_HELPBROWSER_ISNS_NAME   _T("HelpBrowserIsNetscape")
/// width of help frame
#define MP_HELPFRAME_WIDTH_NAME   _T("HelpFrameWidth")
/// height of help frame
#define MP_HELPFRAME_HEIGHT_NAME  _T("HelpFrameHeight")
/// xpos of help frame
#define MP_HELPFRAME_XPOS_NAME   _T("HelpFrameXpos")
/// ypos of help frame
#define MP_HELPFRAME_YPOS_NAME   _T("HelpFrameYpos")
/// the directory for mbox folders
#define   MP_MBOXDIR_NAME         _T("FolderDir")
/// the news spool directory
#define MP_NEWS_SPOOL_DIR_NAME _T("NewsSpool")
/// command to convert tiff faxes to postscript
#define   MP_TIFF2PS_NAME         _T("FaxToPS")
/// preferred intermediary image format in conversions (0=xpm,1=png,2=bmp,3=jpg)
#define MP_TMPGFXFORMAT_NAME      _T("ConvertGfxFormat")
/// the user's M directory
#   define   MP_USERDIR_NAME         _T("UserDirectory")
/// the acceptance status of the license
#define MP_LICENSE_ACCEPTED_NAME   _T("LicenseAccepted")

/// the complete path to the glocal M directory
#define MP_GLOBALDIR_NAME      _T("GlobalDir")

/// run onl one copy of the program at once?
#define MP_RUNONEONLY_NAME _T("RunOneOnly")

#ifdef OS_WIN
/// use config file even under Windows?
#define MP_USE_CONFIG_FILE_NAME _T("UseConfigFile")
#endif // OS_WIN

/// show images in the toolbar
#define MP_TBARIMAGES_NAME _T("ShowTbarImages")

/// the directory containing the help files
#define MP_HELPDIR_NAME _T("HelpDir")

// Unix-only entries
#ifdef OS_UNIX
/// the path where to find .afm files
#   define   MP_AFMPATH_NAME         _T("AfmPath")
#endif //Unix

/// Which encryption algorithm to use : 0 = simple builtin, 1 = twofish
#define MP_CRYPTALGO_NAME    _T("CryptAlgo")
/// DoesTwoFish work? (-1 unknown, 0 no, 1 yes)
#define MP_CRYPT_TWOFISH_OK_NAME _T("TwoFishOk")
/// some test data
#define MP_CRYPT_TESTDATA_NAME _T("CryptData")
/// the locale for translation to national languages
#define   MP_LOCALE_NAME               _T("Locale")
/// the default character set
#define MP_CHARSET_NAME      _T("CharSet")
/// the default icon for frames
#define   MP_ICON_MFRAME_NAME         _T("MFrameIcon")
/// the icon for the main frame
#define   MP_ICON_MAINFRAME_NAME      _T("MainFrameIcon")
/// the icon directory
#define   MP_ICONPATH_NAME         _T("IconDirectory")
/// the path for finding profiles
#define   MP_PROFILE_PATH_NAME         _T("ProfilePath")
/// the extension to use for profile files
#define   MP_PROFILE_EXTENSION_NAME      _T("ProfileExtension")
/// the key for identity redirection
#define MP_PROFILE_IDENTITY_NAME _T("ProfileId")
/// the name of the mailcap file
#define   MP_MAILCAP_NAME         _T("MailCap")
/// the name of the mime types file
#define   MP_MIMETYPES_NAME         _T("MimeTypes")
/// the strftime() format for dates
#define   MP_DATE_FMT_NAME         _T("DateFormat")
/// display all dates as GMT?
#define   MP_DATE_GMT_NAME         _T("GMTDate")
/// show console window
#define   MP_SHOWCONSOLE_NAME      _T("ShowConsole")
/// name of address database
#define   MP_ADBFILE_NAME         _T("AddressBook")
/// open any folders at all on startup?
#define   MP_DONTOPENSTARTUP_NAME   _T("DontOpenAtStartup")
/// names of folders to open at startup (semicolon separated list)
#define   MP_OPENFOLDERS_NAME         _T("OpenFolders")
/// reopen the last opened folder in the main frame
#define   MP_REOPENLASTFOLDER_NAME _T("ReopenLastFolder")
/// name of folder to open in mainframe
#define   MP_MAINFOLDER_NAME          _T("MainFolder")
/// path for Python
#define   MP_PYTHONPATH_NAME        _T("PythonPath")
/// is Python enabled (this is a run-time option)?
#define   MP_USEPYTHON_NAME         _T("UsePython")
/// start-up script to run
#define   MP_STARTUPSCRIPT_NAME     _T("StartupScript")
/// show splash screen on startup?
#define   MP_SHOWSPLASH_NAME        _T("ShowSplash")
/// how long should splash screen stay (0 disables timeout)?
#define   MP_SPLASHDELAY_NAME       _T("SplashDelay")
/// how often should we autosave the profile settings (0 to disable)?
#define   MP_AUTOSAVEDELAY_NAME       _T("AutoSaveDelay")
/// how often should we check for incoming mail (secs, 0 to disable)?
#define   MP_POLLINCOMINGDELAY_NAME       _T("PollIncomingDelay")
/// poll folder only if it is opened
#define   MP_POLL_OPENED_ONLY_NAME _T("PollOpenedOnly")
/// collect all new mail at startup?
#define   MP_COLLECTATSTARTUP_NAME _T("CollectAtStartup")
/// ask user if he really wants to exit?
#define   MP_CONFIRMEXIT_NAME       _T("ConfirmExit")
/// open folders when they're clicked (otherwise - double clicked)
#define   MP_OPEN_ON_CLICK_NAME     _T("ClickToOpen")
/// show all folders (even hidden ones) in the folder tree?
#define   MP_SHOW_HIDDEN_FOLDERS_NAME _T("ShowHiddenFolders")
/// create .profile files?
#define   MP_CREATE_PROFILES_NAME   _T("CreateProfileFiles")
/// umask setting for normal files
#define   MP_UMASK_NAME               _T("Umask")
/// the last selected message in the folder view (or -1)
#define   MP_LASTSELECTED_MESSAGE_NAME _T("FViewLastSel")
/// automatically show th last selected message in folderview?
#define   MP_AUTOSHOW_LASTSELECTED_NAME _T("AutoShowLastSel")
/// automatically show first message in folderview?
#define   MP_AUTOSHOW_FIRSTMESSAGE_NAME _T("AutoShowFirstMessage")
/// automatically show first unread message in folderview?
#define   MP_AUTOSHOW_FIRSTUNREADMESSAGE_NAME _T("AutoShowFirstUnread")
/// open messages when they're clicked (otherwise - double clicked)
#define   MP_PREVIEW_ON_SELECT_NAME     _T("PreviewOnSelect")
/// select the initially focused message
#define   MP_AUTOSHOW_SELECT_NAME _T("AutoShowSelect")
/// program used to convert image files?
#define   MP_CONVERTPROGRAM_NAME      _T("ImageConverter")
/// list of modules to load at startup
#define MP_MODULES_NAME               _T("Modules")
/// the user path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_USER_NAME   _T("CompooseTemplatePathUser")
/// the global path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_GLOBAL_NAME   _T("CompooseTemplatePath")

/// the format string for the folder tree display
#define MP_FOLDERSTATUS_TREE_NAME _T("TreeViewFmt")
/// the format string for status bar folder status display
#define MP_FOLDERSTATUS_STATBAR_NAME _T("StatusBarFmt")
/// the format string for title bar folder status display
#define MP_FOLDERSTATUS_TITLEBAR_NAME _T("TitleBarFmt")

/**@name Printer settings */
//@{
/// Command
#define MP_PRINT_COMMAND_NAME   _T("PrintCommand")
/// Options
#define MP_PRINT_OPTIONS_NAME   _T("PrintOptions")
/// Orientation
#define MP_PRINT_ORIENTATION_NAME _T("PrintOrientation")
/// print mode
#define MP_PRINT_MODE_NAME      _T("PrintMode")
/// paper name
#define MP_PRINT_PAPER_NAME   _T("PrintPaperType")
/// paper name
#define MP_PRINT_FILE_NAME   _T("PrintFilenname")
/// print in colour?
#define MP_PRINT_COLOUR_NAME _T("PrintUseColour")
/// top margin
#define MP_PRINT_TOPMARGIN_X_NAME    _T("PrintMarginLeft")
/// left margin
#define MP_PRINT_TOPMARGIN_Y_NAME   _T("PrintMarginTop")
/// bottom margin
#define MP_PRINT_BOTTOMMARGIN_X_NAME    _T("PrintMarginRight")
/// right margin
#define MP_PRINT_BOTTOMMARGIN_Y_NAME   _T("PrintMarginBottom")
/// zoom level in print preview
#define MP_PRINT_PREVIEWZOOM_NAME   _T("PrintPreviewZoom")
//@}
/**@name for BBDB address book support */
//@{
/// generate unique names
#define   MP_BBDB_GENERATEUNIQUENAMES_NAME   _T("BbdbGenerateUniqueNames")
/// ignore entries without names
#define   MP_BBDB_IGNOREANONYMOUS_NAME      _T("BbdbIgnoreAnonymous")
/// name for anonymous entries, when neither first nor family name are set
#define   MP_BBDB_ANONYMOUS_NAME         _T("BbdbAnonymousName")
/// save on exit, 0=no, 1=ask, 2=always
#define   MP_BBDB_SAVEONEXIT_NAME  _T("BbdbSaveOnExit")
//@}
/**@name For Profiles: */
//@{
/// The Profile Type. [OBSOLETE]
#define   MP_PROFILE_TYPE_NAME      _T("ProfileType")
/// The type of the config source
#define   MP_CONFIG_SOURCE_TYPE_NAME _T("Type")
/// The priority of the config source
#define   MP_CONFIG_SOURCE_PRIO_NAME _T("Priority")
/// the current user identity
#define   MP_CURRENT_IDENTITY_NAME  _T("Identity")
/// the user's full name
#define   MP_PERSONALNAME_NAME     _T("PersonalName")
/// organization (the "Organization:" header name value)
#define   MP_ORGANIZATION_NAME     _T("Organization")
/// the user's qualification
#define   MP_USERLEVEL_NAME        _T("Userlevel")
/// the username/login
#define   MP_USERNAME_NAME         _T("UserName")
/// the user's hostname
#define   MP_HOSTNAME_NAME         _T("HostName")
/// Add this hostname for addresses without hostname?
#define   MP_ADD_DEFAULT_HOSTNAME_NAME   _T("AddDefaultHostName")
/// (the username for returned mail) E-mail address
#define   MP_FROM_ADDRESS_NAME      _T("ReturnAddress")
/// Reply address
#define   MP_REPLY_ADDRESS_NAME      _T("ReplyAddress")
/// the default POP3 host
#define   MP_POPHOST_NAME          _T("Pop3Host")
/// don't use AUTH with POP3
#define   MP_POP_NO_AUTH_NAME       _T("Pop3NoAuth")
/// the default IMAP4 host
#define   MP_IMAPHOST_NAME          _T("Imap4Host")
/// use SSL for POP/IMAP?
#define   MP_USE_SSL_NAME           _T("UseSSL")
/// accept unsigned SSL certificates?
#define   MP_USE_SSL_UNSIGNED_NAME  _T("SSLUnsigned")
/// the mail host
#define   MP_SMTPHOST_NAME         _T("MailHost")
/// use the specified sender value or guess it automatically?
#define   MP_GUESS_SENDER_NAME       _T("GuessSender")
/// the smtp sender value
#define   MP_SENDER_NAME           _T("Sender")
/// the smtp host user-id
#define   MP_SMTPHOST_LOGIN_NAME   _T("MailHostLogin")
/// the smtp host password
#define   MP_SMTPHOST_PASSWORD_NAME  _T("MailHostPw")
/// the news server
#define   MP_NNTPHOST_NAME         _T("NewsHost")
/// the news host user-id
#define   MP_NNTPHOST_LOGIN_NAME   _T("NewsHostLogin")
/// the news host password
#define   MP_NNTPHOST_PASSWORD_NAME  _T("NewsHostPw")
/// use SSL?
#define   MP_SMTPHOST_USE_SSL_NAME         _T("MailHostSSL")
/// check ssl-certs for SMTP connections?
#define   MP_SMTPHOST_USE_SSL_UNSIGNED_NAME   _T("MailHostSSLUnsigned")
/// use ESMTP 8BITMIME extension if available
#define   MP_SMTP_USE_8BIT_NAME         _T("Mail8Bit")
/// disabled SMTP authentificators
#define   MP_SMTP_DISABLED_AUTHS_NAME     _T("SmtpDisabledAuths")
/// sendmail command
#define MP_SENDMAILCMD_NAME _T("SendmailCmd")
/// use sendmail?
#define MP_USE_SENDMAIL_NAME _T("UseSendmail")
/// use SSL?
#define   MP_NNTPHOST_USE_SSL_NAME         _T("NewsHostSSL")
/// check ssl-certs for NNTP connections?
#define   MP_NNTPHOST_USE_SSL_UNSIGNED_NAME   _T("NewsHostSSLUnsigned")
/// the beacon host to test for net connection
#define   MP_BEACONHOST_NAME      _T("BeaconHost")
#ifdef USE_DIALUP
/// does Mahogany control dial-up networking?
#define MP_DIALUP_SUPPORT_NAME   _T("DialUpNetSupport")
#endif // USE_DIALUP

/// set reply string from To: field?
#define MP_SET_REPLY_FROM_TO_NAME   _T("ReplyEqualsTo")
/// should we attach vCard to outgoing messages?
#define MP_USEVCARD_NAME _T("UseVCard")
/// the vCard to use
#define MP_VCARD_NAME _T("VCard")

/// use the folder create wizard (or the dialog directly)?
#define MP_USE_FOLDER_CREATE_WIZARD_NAME _T("FolderCreateWizard")

#if defined(OS_WIN)
/// the RAS connection to use
#define MP_NET_CONNECTION_NAME _T("RasConnection")
#elif defined(OS_UNIX)
/// the command to go online
#define MP_NET_ON_COMMAND_NAME   _T("NetOnCommand")
/// the command to go offline
#define MP_NET_OFF_COMMAND_NAME   _T("NetOffCommand")
#endif // platform

/// login for mailbox
#define   MP_FOLDER_LOGIN_NAME      _T("Login")
/// password for mailbox
#define   MP_FOLDER_PASSWORD_NAME      _T("Password")
/// log level
#define   MP_LOGLEVEL_NAME      _T("LogLevel")
/// show busy info while sorting/threading?
#define   MP_SHOWBUSY_DURING_SORT_NAME _T("BusyDuringSort")
/// threshold for displaying mailfolder progress dialog
#define   MP_FOLDERPROGRESS_THRESHOLD_NAME   _T("FolderProgressThreshold")
/// size threshold for displaying message retrieval progress dialog
#define   MP_MESSAGEPROGRESS_THRESHOLD_SIZE_NAME   _T("MsgProgressMinSize")
/// time threshold for displaying message retrieval progress dialog
#define   MP_MESSAGEPROGRESS_THRESHOLD_TIME_NAME   _T("MsgProgressDelay")
/// the default path for saving files
#define   MP_DEFAULT_SAVE_PATH_NAME      _T("SavePath")
/// the default filename for saving files
#define   MP_DEFAULT_SAVE_FILENAME_NAME   _T("SaveFileName")
/// the default extension for saving files
#define   MP_DEFAULT_SAVE_EXTENSION_NAME   _T("SaveExtension")
/// the wildcard for save dialog
#define   MP_DEFAULT_SAVE_WILDCARD_NAME   _T("SaveWildcard")
/// the default path for saving files
#define   MP_DEFAULT_LOAD_PATH_NAME      _T("LoadPath")
/// the default filename for saving files
#define   MP_DEFAULT_LOAD_FILENAME_NAME   _T("LoadFileName")
/// the default extension for saving files
#define   MP_DEFAULT_LOAD_EXTENSION_NAME   _T("LoadExtension")
/// the wildcard for save dialog
#define   MP_DEFAULT_LOAD_WILDCARD_NAME   _T("LoadWildcard")
/// default value for To: field in composition
#define   MP_COMPOSE_TO_NAME         _T("ComposeToDefault")
/// default value for Cc: field in composition
#define   MP_COMPOSE_CC_NAME         _T("ComposeCcDefault")
/// default value for Bcc: field in composition
#define   MP_COMPOSE_BCC_NAME         _T("ComposeBccDefault")
/// show "From:" field in composer?
#define   MP_COMPOSE_SHOW_FROM_NAME _T("ComposeShowFrom")

/// default reply kind
#define   MP_DEFAULT_REPLY_KIND_NAME _T("ReplyDefault")
/// the mailing list addresses
#define   MP_LIST_ADDRESSES_NAME _T("MLAddresses")
/// prefix for subject in replies
#define   MP_REPLY_PREFIX_NAME         _T("ReplyPrefix")
/// prefix for subject in forwards
#define   MP_FORWARD_PREFIX_NAME         _T("ForwardPrefix")
/// collapse reply prefixes? 0=no, 1=replace "Re"s with one, 2=use reply level
#define   MP_REPLY_COLLAPSE_PREFIX_NAME _T("CollapseReplyPrefix")
/// include the original message in the reply [no,ask,yes]
#define MP_REPLY_QUOTE_ORIG_NAME _T("ReplyQuoteInsert" )
/// include only the selected text (if any) in the reply?
#define MP_REPLY_QUOTE_SELECTION_NAME _T("ReplyQuoteSelection")
/// prefix for text in replies
#define   MP_REPLY_MSGPREFIX_NAME      _T("ReplyQuote")
/// use the value of X-Attribution header as the prefix
#define   MP_REPLY_MSGPREFIX_FROM_XATTR_NAME _T("ReplyQuoteXAttr")
/// prepend the initials of the sender to the reply prefix?
#define   MP_REPLY_MSGPREFIX_FROM_SENDER_NAME _T("ReplyQuoteUseSender")
/// quote the empty lines when replying?
#define   MP_REPLY_QUOTE_EMPTY_NAME      _T("ReplyQuoteEmpty")
/// detect and remove signature when replying?
#define MP_REPLY_DETECT_SIG_NAME _T("DetectSig")
#if wxUSE_REGEX
/// a regex to detect signature
#define MP_REPLY_SIG_SEPARATOR_NAME _T("SigSeparator")
#endif
/// use signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_NAME   _T("ComposeInsertSignature")
/// filename of signature file
#define   MP_COMPOSE_SIGNATURE_NAME      _T("SignatureFile")
/// use "-- " to separate signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_SEPARATOR_NAME   _T("ComposeSeparateSignature")

/// use XFace in composition?
#define   MP_COMPOSE_USE_XFACE_NAME   _T("UseXFaces")
/// Name from where to read XFace
#define   MP_COMPOSE_XFACE_FILE_NAME   _T("XFace")
/// the folder type for a mailbox (see enum in MFolder class)
#define   MP_FOLDER_TYPE_NAME         _T("Type")
/// should we try to create the folder before opening it?
#define   MP_FOLDER_TRY_CREATE_NAME _T("TryCreate")
/// the folder icon for a mailbox (see icon functions in FolderType.h)
#define   MP_FOLDER_ICON_NAME         _T("Icon")
/// the position of the folder in the tree
#define MP_FOLDER_TREEINDEX_NAME _T("Index")
/// Move new mail to the NewMail folder (if not, only copy)?
#define MP_MOVE_NEWMAIL_NAME      _T("MoveNewMail")
/// Where to store all new mail (obsolete)
#define MP_NEWMAIL_FOLDER_NAME      _T("NewMailFolder")
/// Where to store outgoing mail
#define MP_OUTBOX_NAME_NAME       _T("OutBoxName")
/// Use outbox?
#define MP_USE_OUTBOX_NAME          _T("UseOutBox")
/// Name of Trash folder?
#define MP_TRASH_FOLDER_NAME      _T("TrashFolder")
/// Use a trash folder?
#define MP_USE_TRASH_FOLDER_NAME   _T("UseTrash")
/// Name of the Drafts folder
#define MP_DRAFTS_FOLDER_NAME _T("DraftsFolder")
/// Delete the drafts automatically after the message was sent?
#define MP_DRAFTS_AUTODELETE_NAME _T("DraftsDelete")
/// the filename for a mailbox
#define   MP_FOLDER_PATH_NAME         _T("Path")
/// comment
#define   MP_FOLDER_COMMENT_NAME      _T("Comment")
/// update interval for folders in seconds
#define   MP_UPDATEINTERVAL_NAME      _T("UpdateInterval")
/// close of folders delayed by
#define MP_FOLDER_CLOSE_DELAY_NAME   _T("FolderCloseDelay")
/// close of network connection delayed by
#define MP_CONN_CLOSE_DELAY_NAME   _T("ConnCloseDelay")
/// do automatic word wrap?
#define MP_AUTOMATIC_WORDWRAP_NAME   _T("AutoWrap")
/// Wrap quoted lines?
#define MP_WRAP_QUOTED_NAME _T("WrapQuoted")
/// wrapmargin for composition (set to -1 to disable it)
#define   MP_WRAPMARGIN_NAME      _T("WrapMargin")
/// do automatic word wrap in message view?
#define MP_VIEW_AUTOMATIC_WORDWRAP_NAME   _T("ViewAutoWrap")
/// wrapmargin for message view (set to -1 to disable it)
#define   MP_VIEW_WRAPMARGIN_NAME      _T("ViewWrapMargin")
/// show TEXT/PLAIN as inlined text?
#define   MP_PLAIN_IS_TEXT_NAME      _T("PlainIsText")
/// show MESSAGE/RFC822 as inlined text?
#define   MP_RFC822_IS_TEXT_NAME      _T("Rfc822IsText")
/// show XFaces?
#define   MP_SHOW_XFACES_NAME         _T("ShowXFaces")
/// show graphics inline
#define   MP_INLINE_GFX_NAME         _T("InlineGraphics")
/// show the external images (for HTML viewer only) too?
#define   MP_INLINE_GFX_EXTERNAL_NAME _T("InlineExtGraphics")
/// limit size for inline graphics
#define   MP_INLINE_GFX_SIZE_NAME     _T("InlineGraphicsSize")
/// show viewer bar in the message view?
#define MP_MSGVIEW_SHOWBAR_NAME _T("ShowViewerBar")
/// which viewer to use in the message view?
#define MP_MSGVIEW_VIEWER_NAME _T("MsgViewer")
/// which headers to show in the message view?
#define   MP_MSGVIEW_HEADERS_NAME     _T("MsgViewHeaders")
/// all headers we know about
#define   MP_MSGVIEW_ALL_HEADERS_NAME     _T("MsgViewAllHeaders")
/// the default encoding for the viewer/composer
#define  MP_MSGVIEW_DEFAULT_ENCODING_NAME   _T("DefEncoding")
/// the type of the last created folder
#define   MP_LAST_CREATED_FOLDER_TYPE_NAME  _T("LastFolderType")
/// the filter program to apply (OBSOLETE)
#define MP_FILTER_RULE_NAME   _T("Filter")
/// the filters to use for this folder
#define MP_FOLDER_FILTERS_NAME   _T("Filters")
/// the default folder file format
#define MP_FOLDER_FILE_DRIVER_NAME   _T("MailboxFileFormat")
/**@name  Font settings for message view */
//@{
/// message view title
#define   MP_MVIEW_TITLE_FMT_NAME   _T("MViewTitleFmt")
/// which font to use
#define   MP_MVIEW_FONT_NAME         _T("MViewFont")
/// which font size
#define   MP_MVIEW_FONT_SIZE_NAME         _T("MViewFontSize")
/// the full font desc (replaces the 2 settings above)
#define   MP_MVIEW_FONT_DESC_NAME   _T("MViewFontDesc")
/// which foreground colour for the font
#define   MP_MVIEW_FGCOLOUR_NAME      _T("MViewFgColour")
/// which background colour for the font
#define   MP_MVIEW_BGCOLOUR_NAME      _T("MViewBgColour")
/// which colour for signature
#define   MP_MVIEW_SIGCOLOUR_NAME      _T("MViewSigColour")
/// which colour for URLS
#define   MP_MVIEW_URLCOLOUR_NAME      _T("MViewUrlColour")
/// colour for attachment labels
#define   MP_MVIEW_ATTCOLOUR_NAME      _T("MViewAttColour")
/// perform quoted text colourization?
#define   MP_MVIEW_QUOTED_COLOURIZE_NAME   _T("MViewQuotedColourized")
/// cycle colours?
#define   MP_MVIEW_QUOTED_CYCLE_COLOURS_NAME   _T("MViewQuotedCycleColours")
/// which colour for quoted text
#define   MP_MVIEW_QUOTED_COLOUR1_NAME      _T("MViewQuotedColour1")
/// which colour for quoted text, second level
#define   MP_MVIEW_QUOTED_COLOUR2_NAME      _T("MViewQuotedColour2")
/// which colour for quoted text, third level
#define   MP_MVIEW_QUOTED_COLOUR3_NAME      _T("MViewQuotedColour3")
/// the maximum number of whitespaces prepending >
#define   MP_MVIEW_QUOTED_MAXWHITESPACE_NAME    _T("MViewQuotedMaxWhitespace")
/// the maximum number of A-Z prepending >
#define   MP_MVIEW_QUOTED_MAXALPHA_NAME    _T("MViewQuotedMaxAlpha")
/// the colour for header names in the message view
#define   MP_MVIEW_HEADER_NAMES_COLOUR_NAME  _T("MViewHeaderNamesColour")
/// the colour for header values in the message view
#define   MP_MVIEW_HEADER_VALUES_COLOUR_NAME  _T("MViewHeaderValuesColour")
//@}
/**@name  Font settings for folder view */
//@{
/// which font to use
#define   MP_FVIEW_FONT_NAME         _T("FViewFont")
/// which font size
#define   MP_FVIEW_FONT_SIZE_NAME         _T("FViewFontSize")
/// the full font desc (replaces the 2 settings above)
#define   MP_FVIEW_FONT_DESC_NAME         _T("FViewFontDesc")
/// don't show full e-mail, only sender's name
#define   MP_FVIEW_NAMES_ONLY_NAME         _T("FViewNamesOnly")
/// which foreground colour for the font
#define   MP_FVIEW_FGCOLOUR_NAME      _T("FViewFgColour")
/// which background colour for the font
#define   MP_FVIEW_BGCOLOUR_NAME      _T("FViewBgColour")
/// colour for deleted messages
#define   MP_FVIEW_DELETEDCOLOUR_NAME      _T("FViewDeletedColour")
/// colour for new messages
#define   MP_FVIEW_NEWCOLOUR_NAME      _T("FViewNewColour")
/// colour for recent messages
#define   MP_FVIEW_RECENTCOLOUR_NAME      _T("FViewRecentColour")
/// colour for unread messages
#define   MP_FVIEW_UNREADCOLOUR_NAME      _T("FViewUnreadColour")
/// colour for flagged messages
#define   MP_FVIEW_FLAGGEDCOLOUR_NAME      _T("FViewFlaggedColour")
/// automatically select next unread message after finishing the current one
#define MP_FVIEW_AUTONEXT_UNREAD_MSG_NAME _T("FViewAutoNextMsg")
/// automatically select next unread folder after finishing the current one
#define MP_FVIEW_AUTONEXT_UNREAD_FOLDER_NAME _T("FViewAutoNextFolder")
/// how to show the size (MessageSizeShow enum value)
#define MP_FVIEW_SIZE_FORMAT_NAME   _T("SizeFormat")
/// update the folder view status bar to show the msg info?
#define   MP_FVIEW_STATUS_UPDATE_NAME _T("FViewStatUpdate")
/// folder view status bar string
#define   MP_FVIEW_STATUS_FMT_NAME  _T("FViewStatFmt")
/// delay before previewing the selected item in the folder view (0 to disable)
#define MP_FVIEW_PREVIEW_DELAY_NAME _T("FViewPreviewDelay")
/// split folder view vertically (or horizontally)?
#define MP_FVIEW_VERTICAL_SPLIT_NAME _T("FViewVertSplit")
/// put folder view on top and msg view on bottom or vice versa?
#define MP_FVIEW_FVIEW_TOP_NAME _T("FViewOnTop")
/// replace "From" address with "To" in messages from oneself?
#define MP_FVIEW_FROM_REPLACE_NAME _T("ReplaceFrom")
/// the ':' separated list of addresses which are "from oneself"
#define MP_FROM_REPLACE_ADDRESSES_NAME _T("ReplaceFromAdr")
/// Automatically focus next message after copy, mark (un)read, etc.
#define MP_FVIEW_AUTONEXT_ON_COMMAND_NAME _T("FViewAutoNextOnCommand")
//@}
/**@name  Font settings for folder tree */
//@{
/// is the folder tree on the left of folder view or on the right?
#define MP_FTREE_LEFT_NAME _T("FTreeLeft")
/// the foreground colour for the folder tree
#define MP_FTREE_FGCOLOUR_NAME _T("FTreeFgColour")
/// the background colour for the folder tree
#define MP_FTREE_BGCOLOUR_NAME _T("FTreeBgColour")
/// show the currently opened folder specially?
#define MP_FTREE_SHOWOPENED_NAME _T("FTreeShowOpened")
/// format for the folder tree entries
#define MP_FTREE_FORMAT_NAME _T("FTreeFormat")
/// reflect the folder status in its parent
#define MP_FTREE_PROPAGATE_NAME _T("FTreePropagate")
/// skip this folder when looking for next unread one in the tree
#define MP_FTREE_NEVER_UNREAD_NAME _T("FTreeNeverUnread")
/// go to this folder when Ctrl-Home is pressed
#define MP_FTREE_HOME_NAME _T("FTreeHome")
//@}
/**@name Font and colour settings for composer */
//@{
/// which font to use
#define   MP_CVIEW_FONT_NAME         _T("CViewFont")
/// which font size
#define   MP_CVIEW_FONT_SIZE_NAME         _T("CViewFontSize")
/// the full font desc (replaces the 2 settings above)
#define   MP_CVIEW_FONT_DESC_NAME         _T("CViewFontDesc")
/// which foreground colour for the font
#define   MP_CVIEW_FGCOLOUR_NAME      _T("CViewFGColour")
/// which background colour for the font
#define   MP_CVIEW_BGCOLOUR_NAME      _T("CViewBGColout") // typo but do *NOT* fix
/// use the colours and font for the headers as well?
#define   MP_CVIEW_COLOUR_HEADERS_NAME _T("CViewColourHeaders")
//@}
/// highlight signature?
#define   MP_HIGHLIGHT_SIGNATURE_NAME      _T("HighlightSig")
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS_NAME      _T("HighlightURL")

/// do we want to use server side sort?
#define MP_MSGS_SERVER_SORT_NAME    _T("SortOnServer")
/// sort criterium for folder listing
#define MP_MSGS_SORTBY_NAME         _T("SortMessagesBy")
/// re-sort messages on status change?
#define MP_MSGS_RESORT_ON_CHANGE_NAME         _T("ReSortMessagesOnChange")
/// use threading
#define MP_MSGS_USE_THREADING_NAME  _T("ThreadMessages")
/// use server side threading?
#define MP_MSGS_SERVER_THREAD_NAME _T("ThreadOnServer")
/// only use server side threading by references (best threading method)?
#define MP_MSGS_SERVER_THREAD_REF_ONLY_NAME _T("ThreadByRefOnly")

/// Gather messages with same subject in one thread
#define MP_MSGS_GATHER_SUBJECTS_NAME _T("GatherSubjectsWhenThreading")
/// break thread when subject changes
#define MP_MSGS_BREAK_THREAD_NAME _T("BreakThreadIfSubjectChanges")
/// Indent messages when common ancestor is missing
#define MP_MSGS_INDENT_IF_DUMMY_NAME _T("IndentIfDummy")

#if wxUSE_REGEX
#   define MP_MSGS_SIMPLIFYING_REGEX_NAME _T("SimplifyingRegex")
#   define MP_MSGS_REPLACEMENT_STRING_NAME _T("ReplacementString")
#else // !wxUSE_REGEX
    /// Remove list prefix when comparing message's subject to gather them
#   define MP_MSGS_REMOVE_LIST_PREFIX_GATHERING_NAME _T("RemoveListPrefixWhenGathering")
    /// Remove list prefix when comparing message's subject to break threads
#   define MP_MSGS_REMOVE_LIST_PREFIX_BREAKING_NAME _T("RemoveListPrefixWhenBreaking")
#endif // wxUSE_REGEX

/// search criterium for searching in folders
#define MP_MSGS_SEARCH_CRIT_NAME   _T("SearchCriterium")
/// search argument
#define MP_MSGS_SEARCH_ARG_NAME    _T("SearchArgument")
/// open URLs with
#define   MP_BROWSER_NAME         _T("Browser")
/// Browser is netscape variant
#define   MP_BROWSER_ISNS_NAME    _T("BrowserIsNetscape")
/// Open netscape in new window
#define   MP_BROWSER_INNW_NAME    _T("BrowserInNewWindow")
/// external editor to use for message composition (use %s for filename)
#define MP_EXTERNALEDITOR_NAME    _T("ExternalEditor")
/// start external editor automatically?
#define MP_ALWAYS_USE_EXTERNALEDITOR_NAME    _T("AlwaysUseExtEditor")
/// PGP/GPG application
#define MP_PGP_COMMAND_NAME    _T("PGPCommand")
/// PGP/GPG key server for public keys
#define MP_PGP_KEYSERVER_NAME  _T("PGPKeyServer")
/// get PGP key from server?
#define MP_PGP_GET_PUBKEY_NAME  _T("PGPGetPubKey")
/// execute a command when new mail arrives?
#define   MP_USE_NEWMAILCOMMAND_NAME      _T("CommandOnNewMail")
/// command to execute when new mail arrives
#define   MP_NEWMAILCOMMAND_NAME      _T("OnNewMail")

/// play a sound on new mail?
#define MP_NEWMAIL_PLAY_SOUND_NAME _T("NewMailPlaySound")
/// which sound to play?
#define MP_NEWMAIL_SOUND_FILE_NAME _T("NewMailSound")
#if defined(OS_UNIX) || defined(__CYGWIN__)
/// the program to use to play this sound
#define MP_NEWMAIL_SOUND_PROGRAM_NAME _T("NewMailSoundProg")
#endif // OS_UNIX

/// show new mail messages?
#define   MP_SHOW_NEWMAILMSG_NAME      _T("ShowNewMail")
/// show detailed info about how many new mail messages?
#define   MP_SHOW_NEWMAILINFO_NAME      _T("ShowNewMailInfo")
/// consider only unseen messages as new?
#define   MP_NEWMAIL_UNSEEN_NAME _T("NewUnseenOnly")
/// collect mail from INBOX?
#define   MP_COLLECT_INBOX_NAME _T("CollectInbox")
/// keep copies of outgoing mail?
#define   MP_USEOUTGOINGFOLDER_NAME   _T("KeepCopies")
/// write outgoing mail to folder:
#define   MP_OUTGOINGFOLDER_NAME      _T("SentMailFolder")
/// Show all message headers?
#define   MP_SHOWHEADERS_NAME         _T("ShowHeaders")
/// Autocollect email addresses? 0=no 1=ask 2=always
#define   MP_AUTOCOLLECT_NAME         _T("AutoCollect")
/// Name of the address books for autocollected addresses
#define   MP_AUTOCOLLECT_ADB_NAME     _T("AutoCollectAdb")
/// Autocollect email addresses from sender only ?
#define   MP_AUTOCOLLECT_SENDER_NAME     _T("AutoCollectSender")
/// Autocollect entries with names only?
#define   MP_AUTOCOLLECT_NAMED_NAME _T("AutoCollectNamed")
/// support efax style incoming faxes
#define MP_INCFAX_SUPPORT_NAME      _T("IncomingFaxSupport")
/// domains from which to support faxes, semicolon delimited
#define MP_INCFAX_DOMAINS_NAME      _T("IncomingFaxDomains")
/// Default name for the SSL library
#define MP_SSL_DLL_SSL_NAME      _T("SSLDll")
/// Default name for the SSL/crypto library
#define MP_SSL_DLL_CRYPTO_NAME _T("CryptoDll")

/// Use substrings in address expansion?
#define   MP_ADB_SUBSTRINGEXPANSION_NAME   _T("ExpandWithSubstring")

/** @name maximal amounts of data to retrieve from remote servers */
//@{
/// ask confirmation before retrieveing messages bigger than this (in Kb)
#define MP_MAX_MESSAGE_SIZE_NAME   _T("MaxMsgSize")
/// ask confirmation before retrieveing more headers than this
#define MP_MAX_HEADERS_NUM_NAME    _T("MaxHeadersNum")
/// never download more than that many messages
#define MP_MAX_HEADERS_NUM_HARD_NAME _T("HardHeadersLimit")
//@}

/// setting this prevents the filters from expuning the msgs automatically
#define MP_SAFE_FILTERS_NAME _T("SafeFilters")

/** @name timeout values for c-client mail library */
//@{
/// IMAP lookahead value
#define MP_IMAP_LOOKAHEAD_NAME _T("IMAPlookahead")
/// TCP/IP open timeout in seconds.
#define MP_TCP_OPENTIMEOUT_NAME _T("TCPOpenTimeout")
/// TCP/IP read timeout in seconds.
#define MP_TCP_READTIMEOUT_NAME _T("TCPReadTimeout")
/// TCP/IP write timeout in seconds.
#define  MP_TCP_WRITETIMEOUT_NAME _T("TCPWriteTimeout")
/// TCP/IP close timeout in seconds.
#define  MP_TCP_CLOSETIMEOUT_NAME _T("TCPCloseTimeout")
/// rsh connection timeout in seconds.
#define MP_TCP_RSHTIMEOUT_NAME _T("TCPRshTimeout")
/// ssh connection timeout in seconds.
#define MP_TCP_SSHTIMEOUT_NAME _T("TCPSshTimeout")
/// the path to rsh
#define MP_RSH_PATH_NAME _T("RshPath")
/// the path to ssh
#define MP_SSH_PATH_NAME _T("SshPath")
//@}
/** @name for folder list ctrls: ratios of the width to use for
    columns */
//@{
/// status
#define   MP_FLC_STATUSWIDTH_NAME   _T("ColumnWidthStatus")
/// subject
#define   MP_FLC_SUBJECTWIDTH_NAME   _T("ColumnWidthSubject")
/// from
#define   MP_FLC_FROMWIDTH_NAME   _T("ColumnWidthFrom")
/// date
#define   MP_FLC_DATEWIDTH_NAME   _T("ColumnWidthDate")
/// size
#define   MP_FLC_SIZEWIDTH_NAME   _T("ColumnWidthSize")
//@}
/** @name for folder list ctrls: column numbers */
//@{
/// status
#define   MP_FLC_STATUSCOL_NAME   _T("ColumnStatus")
/// subject
#define   MP_FLC_SUBJECTCOL_NAME   _T("ColumnSubject")
/// from
#define   MP_FLC_FROMCOL_NAME   _T("ColumnFrom")
/// date
#define   MP_FLC_DATECOL_NAME   _T("ColumnDate")
/// size
#define   MP_FLC_SIZECOL_NAME   _T("ColumnSize")
/// msgno
#define   MP_FLC_MSGNOCOL_NAME   _T("ColumnNum")
//@}
//@}
/// an entry used for testing
#define MP_TESTENTRY_NAME      _T("TestEntry")
/// Do we remote synchronise configuration settings?
#define MP_SYNC_REMOTE_NAME   _T("SyncRemote")
/// IMAP folder to sync to
#define MP_SYNC_FOLDER_NAME   _T("SyncFolder")
/// our last sync date
#define MP_SYNC_DATE_NAME      _T("SyncDate")
/// sync filters?
#define MP_SYNC_FILTERS_NAME   _T("SyncFilters")
/// sync filters?
#define MP_SYNC_IDS_NAME   _T("SyncIds")
/// sync folders?
#define MP_SYNC_FOLDERS_NAME   _T("SyncFolders")
/// sync folder tree
#define MP_SYNC_FOLDERGROUP_NAME   _T("SyncFolderGroup")

/** @name sending */
//@{
/// confirm sending the message?
#define MP_CONFIRM_SEND_NAME _T("ConfirmSend")
/// preview the message being sent?
#define MP_PREVIEW_SEND_NAME _T("PreviewSend")
//@}

/** @name away mode */
//@{
/// automatically enter the away mode after that many minutes (0 to disable)
#define MP_AWAY_AUTO_ENTER_NAME _T("AutoAway")
/// automaticlly exit from the away mode when the user chooses menu command
#define MP_AWAY_AUTO_EXIT_NAME _T("AutoExitAway")
/// keep the value of away mode for the next run (otherwise always reset it)
#define MP_AWAY_REMEMBER_NAME _T("RememberAway")
/// the saved value of away status (only written if MP_AWAY_REMEMBER_NAME is set)
#define MP_AWAY_STATUS_NAME _T("AwayStatus")
//@}

/** @name names of obsolete configuration entries, for upgrade routines */
//@{
#define MP_OLD_FOLDER_HOST_NAME _T("HostName")
//@}

/// stop "folder internal data" message
#define MP_CREATE_INTERNAL_MESSAGE_NAME   _T("CreateInternalMessage")

/// name of addressbook to use in whitelist spam filter
#define MP_WHITE_LIST_NAME _T("WhiteList")

/// treat mail in this folder as junk mail
#define MP_TREAT_AS_JUNK_MAIL_FOLDER_NAME _T("TreatAsJunkMailFolder")

//@}

// ----------------------------------------------------------------------------
// the option default values
// ----------------------------------------------------------------------------

/** @name default values of configuration entries */
//@{
/// The Profile Type. [OBSOLETE]
#define   MP_PROFILE_TYPE_DEFVAL      0l
/// The type of the config source
#define   MP_CONFIG_SOURCE_TYPE_DEFVAL wxEmptyString
/// The priority of the config source
#define   MP_CONFIG_SOURCE_PRIO_DEFVAL 0l
/// our version
#define   MP_VERSION_DEFVAL          M_EMPTYSTRING
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
/// show log window?
#define   MP_SHOWLOG_DEFVAL  1
/// the file to save log messages to (if not empty)
#define   MP_LOGFILE_DEFVAL wxEmptyString
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
#define   MP_HELPBROWSER_DEFVAL   _T("netscape")
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
#define   MP_MBOXDIR_DEFVAL         M_EMPTYSTRING
/// the news spool directory
#define MP_NEWS_SPOOL_DIR_DEFVAL M_EMPTYSTRING
/// command to convert tiff faxes to postscript
#define   MP_TIFF2PS_DEFVAL         _T("convert %s %s")
/// preferred intermediary image format in conversions
#define MP_TMPGFXFORMAT_DEFVAL      2
/// expand folder tree control?
#define   MP_EXPAND_TREECTRL_DEFVAL   1
/// focus follows mouse?
#ifdef OS_WIN
#define MP_FOCUS_FOLLOWSMOUSE_DEFVAL    0l
#else
#define MP_FOCUS_FOLLOWSMOUSE_DEFVAL    1l
#endif
/// dockable menu bars?
#define   MP_DOCKABLE_MENUBARS_DEFVAL   1l
/// dockable tool bars?
#define   MP_DOCKABLE_TOOLBARS_DEFVAL   1l
/// flat tool bars?
#define   MP_FLAT_TOOLBARS_DEFVAL      1l

/// the user's M directory
#define   MP_USERDIR_DEFVAL         M_EMPTYSTRING

/// the acceptance status of the license
#define MP_LICENSE_ACCEPTED_DEFVAL   0l

/// run onl one copy of the program at once?
#define MP_RUNONEONLY_DEFVAL 1l

#ifdef OS_WIN
/// use config file even under Windows?
#define MP_USE_CONFIG_FILE_DEFVAL wxEmptyString
#endif // OS_WIN

/// show images in the toolbar
#define MP_TBARIMAGES_DEFVAL 0l // == TbarShow_Icons - 1

// Unix-only entries
#ifdef OS_UNIX
/// the complete path to the glocal M directory
#  define   MP_GLOBALDIR_DEFVAL      M_BASEDIR
/// the path where to find .afm files
#  define   MP_AFMPATH_DEFVAL M_BASEDIR _T("/afm:/usr/share:/usr/lib:/usr/local/share:/usr/local/lib:/opt/ghostscript:/opt/enscript")
#else // !Unix
/// the complete path to the glocal M directory
#  define   MP_GLOBALDIR_DEFVAL  wxEmptyString
#endif // Unix/!Unix

/// the directory containing the help files
#define MP_HELPDIR_DEFVAL wxEmptyString

/// Which encryption algorithm to use : 0 = simple builtin, 1 = twofish
#define MP_CRYPTALGO_DEFVAL   0L
/// DoesTwoFish work? (-1 unknown, 0 no, 1 yes)
#define MP_CRYPT_TWOFISH_OK_DEFVAL -1
/// some test data
#define MP_CRYPT_TESTDATA_DEFVAL wxEmptyString
/// the locale for translation to national languages
#define   MP_LOCALE_DEFVAL               M_EMPTYSTRING
/// the default character set
#define MP_CHARSET_DEFVAL               _T("ISO-8859-1")
/// the default icon for frames
#define   MP_ICON_MFRAME_DEFVAL      _T("mframe.xpm")
/// the icon for the main frame
#define   MP_ICON_MAINFRAME_DEFVAL      _T("mainframe.xpm")
/// the icon directoy
#define   MP_ICONPATH_DEFVAL         _T("icons")
/// the profile path
#define   MP_PROFILE_PATH_DEFVAL      _T(".")
/// the extension to use for profile files
#define   MP_PROFILE_EXTENSION_DEFVAL      _T(".profile")
/// the key for identity redirection
#define MP_PROFILE_IDENTITY_DEFVAL  wxEmptyString
/// the name of the mailcap file
#define   MP_MAILCAP_DEFVAL         _T("mailcap")
/// the name of the mime types file
#define   MP_MIMETYPES_DEFVAL         _T("mime.types")
/// the strftime() format for dates
#define   MP_DATE_FMT_DEFVAL         _T("%c")
/// display all dates as GMT?
#define   MP_DATE_GMT_DEFVAL         0l
/// show console window
#define   MP_SHOWCONSOLE_DEFVAL      1
/// open any folders at all on startup?
#define   MP_DONTOPENSTARTUP_DEFVAL   0l
/// names of folders to open at startup
#define   MP_OPENFOLDERS_DEFVAL      M_EMPTYSTRING
/// reopen the last opened folder in the main frame
#define   MP_REOPENLASTFOLDER_DEFVAL 1
/// name of folder to open in mainframe
#define   MP_MAINFOLDER_DEFVAL        wxEmptyString
/// path for Python
#define   MP_PYTHONPATH_DEFVAL         M_EMPTYSTRING
#ifdef OS_WIN
// under Windows, the setup program will create the appropriate registry key
#define   MP_USEPYTHON_DEFVAL         0l
#else
/// is Python enabled (this is a run-time option)?
#define   MP_USEPYTHON_DEFVAL         1
#endif
/// start-up script to run
#define     MP_STARTUPSCRIPT_DEFVAL   wxEmptyString
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
#define   MP_CONVERTPROGRAM_DEFVAL      _T("convert %s -compress None %s")
/// list of modules to load at startup
#define MP_MODULES_DEFVAL   _T("Filters:Migrate")
/// the user path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_USER_DEFVAL   wxEmptyString
/// the global path for template files used for message composition
#define MP_COMPOSETEMPLATEPATH_GLOBAL_DEFVAL   wxEmptyString

/// the format string for the folder tree display
#define MP_FOLDERSTATUS_TREE_DEFVAL _("%f (%t, %u)")
/// the format string for status bar folder status display
#define MP_FOLDERSTATUS_STATBAR_DEFVAL _("%f (%t messages, %u unread, %n new)")
/// the format string for title bar folder status display
#define MP_FOLDERSTATUS_TITLEBAR_DEFVAL _("%f (%u unread, %n new)")

/**@name Printer settings */
//@{
/// Command
#define MP_PRINT_COMMAND_DEFVAL _T("lpr")
/// Options
#define MP_PRINT_OPTIONS_DEFVAL   wxEmptyString
/// Orientation
#define MP_PRINT_ORIENTATION_DEFVAL 1  /* wxPORTRAIT */
/// print mode
#define MP_PRINT_MODE_DEFVAL 0L  /* PS_PRINTER */
/// paper name
#define MP_PRINT_PAPER_DEFVAL 0L
/// paper name
#define MP_PRINT_FILE_DEFVAL _T("mahogany.ps")
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
#define   MP_BBDB_ANONYMOUS_DEFVAL   _T("anonymous")
/// save on exit, 0=no, 1=ask, 2=always
#define   MP_BBDB_SAVEONEXIT_DEFVAL  (long)M_ACTION_PROMPT
//@}
/**@name For Profiles: */
//@{
//@}
/// the user's full name
#define   MP_PERSONALNAME_DEFVAL      M_EMPTYSTRING
/// organization (the "Organization:" header name value)
#define   MP_ORGANIZATION_DEFVAL     M_EMPTYSTRING
/// the current user identity
#define   MP_CURRENT_IDENTITY_DEFVAL M_EMPTYSTRING
/// the user's qualification (see M_USERLEVEL_XXX)
#define   MP_USERLEVEL_DEFVAL        (long)M_USERLEVEL_NOVICE
/// the username/login
#define   MP_USERNAME_DEFVAL         M_EMPTYSTRING
/// the user's hostname
#define   MP_HOSTNAME_DEFVAL         M_EMPTYSTRING
/// Add this hostname for addresses without hostname?
#define   MP_ADD_DEFAULT_HOSTNAME_DEFVAL 1L
/// (the username for returned mail) E-mail address
#define   MP_FROM_ADDRESS_DEFVAL      M_EMPTYSTRING
/// Reply address
#define   MP_REPLY_ADDRESS_DEFVAL      M_EMPTYSTRING
/// the default POP3 host
#define   MP_POPHOST_DEFVAL          _T("pop")
/// don't use AUTH with POP3
#define   MP_POP_NO_AUTH_DEFVAL     0l
/// the default IMAP4 host
#define   MP_IMAPHOST_DEFVAL          _T("imap")
/// use SSL for POP/IMAP?
#define   MP_USE_SSL_DEFVAL       1l // SSLSupport_TLSIfAvailable
/// accept unsigned SSL certificates?
#define   MP_USE_SSL_UNSIGNED_DEFVAL 0l
/// the mail host
#define   MP_SMTPHOST_DEFVAL         _T("mail")
/// use the specified sender value or guess it automatically?
#define   MP_GUESS_SENDER_DEFVAL  1l
/// the smtp sender value
#define   MP_SENDER_DEFVAL           wxEmptyString
/// the mail host user-id
#define   MP_SMTPHOST_LOGIN_DEFVAL   wxEmptyString
/// the mail host password
#define   MP_SMTPHOST_PASSWORD_DEFVAL   wxEmptyString
/// use SSL?
#define   MP_SMTPHOST_USE_SSL_DEFVAL   1l // SSLSupport_TLSIfAvailable
/// check ssl-certs for SMTP connections?
#define   MP_SMTPHOST_USE_SSL_UNSIGNED_DEFVAL   0l
/// use ESMTP 8BITMIME extension if available
#define   MP_SMTP_USE_8BIT_DEFVAL 1l
/// disabled SMTP authentificators
#define   MP_SMTP_DISABLED_AUTHS_DEFVAL wxEmptyString
/// sendmail command  FIXME - should be detected by configure
#ifdef OS_LINUX
#  define MP_SENDMAILCMD_DEFVAL _T("/usr/sbin/sendmail -t")
#else
#  define MP_SENDMAILCMD_DEFVAL _T("/usr/lib/sendmail -t")
#endif
/// use sendmail?
#ifdef OS_UNIX
#  define MP_USE_SENDMAIL_DEFVAL 1l
#else
#  define MP_USE_SENDMAIL_DEFVAL 0l
#endif
/// the news server
#define   MP_NNTPHOST_DEFVAL      _T("news")
/// the news server user-id
#define   MP_NNTPHOST_LOGIN_DEFVAL      wxEmptyString
/// the news server password
#define   MP_NNTPHOST_PASSWORD_DEFVAL      wxEmptyString
/// use SSL?
#define   MP_NNTPHOST_USE_SSL_DEFVAL   1l // SSLSupport_TLSIfAvailable
/// check ssl-certs for NNTP connections?
#define   MP_NNTPHOST_USE_SSL_UNSIGNED_DEFVAL   0l
/// the beacon host to test for net connection
#define   MP_BEACONHOST_DEFVAL      wxEmptyString
#ifdef USE_DIALUP
/// does Mahogany control dial-up networking?
#define MP_DIALUP_SUPPORT_DEFVAL   0L
#endif // USE_DIALUP

/// set reply string from To: field?
#define MP_SET_REPLY_FROM_TO_DEFVAL   0l
/// should we attach vCard to outgoing messages?
#define MP_USEVCARD_DEFVAL 0l
/// the vCard to use
#define MP_VCARD_DEFVAL _T("vcard.vcf")

/// use the folder create wizard (or the dialog directly)?
#define MP_USE_FOLDER_CREATE_WIZARD_DEFVAL 1l

#ifdef USE_DIALUP

#if defined(OS_WIN)
/// the RAS connection to use
#define MP_NET_CONNECTION_DEFVAL wxEmptyString
#elif defined(OS_UNIX)
/// the command to go online
#define MP_NET_ON_COMMAND_DEFVAL   _T("wvdial")
/// the command to go offline
#define MP_NET_OFF_COMMAND_DEFVAL   _T("killall pppd")
#endif // platform

#endif // USE_DIALUP

/// login for mailbox
#define   MP_FOLDER_LOGIN_DEFVAL      M_EMPTYSTRING
/// passwor for mailbox
#define   MP_FOLDER_PASSWORD_DEFVAL      M_EMPTYSTRING
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
#define   MP_DEFAULT_SAVE_PATH_DEFVAL      M_EMPTYSTRING
/// the default filename for saving files
#define   MP_DEFAULT_SAVE_FILENAME_DEFVAL   _T("*")
/// the default extension for saving files
#define   MP_DEFAULT_SAVE_EXTENSION_DEFVAL   M_EMPTYSTRING
/// the default path for saving files
#define   MP_DEFAULT_LOAD_PATH_DEFVAL      M_EMPTYSTRING
/// the default filename for saving files
#define   MP_DEFAULT_LOAD_FILENAME_DEFVAL   _T("*")
/// the default extension for saving files
#define   MP_DEFAULT_LOAD_EXTENSION_DEFVAL   M_EMPTYSTRING
/// default value for To: field in composition
#define   MP_COMPOSE_TO_DEFVAL         M_EMPTYSTRING
/// default value for Cc: field in composition
#define   MP_COMPOSE_CC_DEFVAL         M_EMPTYSTRING
/// default value for Bcc: field in composition
#define   MP_COMPOSE_BCC_DEFVAL      M_EMPTYSTRING
/// show "From:" field in composer?
#define   MP_COMPOSE_SHOW_FROM_DEFVAL 0l

/// default reply kind
#define   MP_DEFAULT_REPLY_KIND_DEFVAL 0l  // MailFolder::REPLY_SENDER
/// the mailing list addresses
#define   MP_LIST_ADDRESSES_DEFVAL wxEmptyString
/// prefix for subject in replies
#define   MP_REPLY_PREFIX_DEFVAL      _T("Re: ")
/// prefix for subject in forwards
#define   MP_FORWARD_PREFIX_DEFVAL      _("Forwarded message: ")
/// collapse reply prefixes? 0=no, 1=replace "Re"s with one, 2=use reply level
#define   MP_REPLY_COLLAPSE_PREFIX_DEFVAL 2l
/// include the original message in the reply [no,ask,yes]
#define MP_REPLY_QUOTE_ORIG_DEFVAL (long)M_ACTION_ALWAYS
/// include only the selected text (if any) in the reply?
#define MP_REPLY_QUOTE_SELECTION_DEFVAL true
/// prefix for text in replies
#define   MP_REPLY_MSGPREFIX_DEFVAL      _T("> ")
/// use the value of X-Attribution header as the prefix
#define   MP_REPLY_MSGPREFIX_FROM_XATTR_DEFVAL 1l
/// prepend the initials of the sender to the reply prefix?
#define   MP_REPLY_MSGPREFIX_FROM_SENDER_DEFVAL 0l
/// quote the empty lines when replying?
#define   MP_REPLY_QUOTE_EMPTY_DEFVAL      1l
/// detect and remove signature when replying?
#define MP_REPLY_DETECT_SIG_DEFVAL   1
#if wxUSE_REGEX
/// a regex to detect signature
#define MP_REPLY_SIG_SEPARATOR_DEFVAL _T("((_____*)|(-- ?))")
#endif
/// use signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_DEFVAL   1
/// filename of signature file
#ifdef OS_WIN
#  define   MP_COMPOSE_SIGNATURE_DEFVAL      M_EMPTYSTRING
#else
#  define   MP_COMPOSE_SIGNATURE_DEFVAL      _T("$HOME/.signature")
#endif
/// use "-- " to separate signature in composition?
#define   MP_COMPOSE_USE_SIGNATURE_SEPARATOR_DEFVAL   1

/// use XFace in composition?
#define   MP_COMPOSE_USE_XFACE_DEFVAL   1
/// Name from where to read XFace
#define   MP_COMPOSE_XFACE_FILE_DEFVAL   _T("$HOME/.xface")
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
#define MP_NEWMAIL_FOLDER_DEFVAL      wxEmptyString // obsolete
/// Which folder to use as Outbox
#define MP_OUTBOX_NAME_DEFVAL         wxEmptyString
/// Use Outbox?
#define MP_USE_OUTBOX_DEFVAL            0l
/// Name of Trash folder?
#define MP_TRASH_FOLDER_DEFVAL      wxEmptyString
/// Use a trash folder?
#define MP_USE_TRASH_FOLDER_DEFVAL   1l
/// Name of the Drafts folder
#define MP_DRAFTS_FOLDER_DEFVAL wxEmptyString
/// Delete the drafts automatically after the message was sent?
#define MP_DRAFTS_AUTODELETE_DEFVAL  1l
/// the filename for a mailbox
#define   MP_FOLDER_PATH_DEFVAL      ((const wxChar *)NULL) // don't change this!
/// comment
#define   MP_FOLDER_COMMENT_DEFVAL      M_EMPTYSTRING
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
#define MP_MSGVIEW_VIEWER_DEFVAL _T("LayoutViewer")
/// which headers to show in the message view?
#define   MP_MSGVIEW_HEADERS_DEFVAL     \
          _T("Date:" \
          "From:" \
          "Subject:" \
          "Newsgroups:" \
          "To:" \
          "Cc:" \
          "Bcc:" \
          "Reply-To:")
/// all headers we know about
#define   MP_MSGVIEW_ALL_HEADERS_DEFVAL \
          MP_MSGVIEW_HEADERS_DEFVAL \
          _T("Approved:" \
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
          "X-WM-Posted-At:")
/// the default encoding for the viewer/composer
#define  MP_MSGVIEW_DEFAULT_ENCODING_DEFVAL (long)wxFONTENCODING_DEFAULT
/// the type of the last created folder
#define   MP_LAST_CREATED_FOLDER_TYPE_DEFVAL  (long)MF_FILE
/// the filter program to apply (OBSOLETE)
#define MP_FILTER_RULE_DEFVAL   wxEmptyString
/// the filters to use for this folder
#define MP_FOLDER_FILTERS_DEFVAL   wxEmptyString
/// the default folder file format
#define MP_FOLDER_FILE_DRIVER_DEFVAL 0L
/* format: mbx:unix:mmdf:tenex defined in MailFolderCC.cpp */
/**@name  Font settings for message view */
//@{
/// message view title
#define   MP_MVIEW_TITLE_FMT_DEFVAL   _("from $from about \"$subject\"")
/// which font to use
#define   MP_MVIEW_FONT_DEFVAL         6L
/// which font size
#define   MP_MVIEW_FONT_SIZE_DEFVAL         DEFAULT_FONT_SIZE
/// the full font desc (replaces the 2 settings above)
#define   MP_MVIEW_FONT_DESC_DEFVAL   wxEmptyString
/// which foreground colour for the font
#define   MP_MVIEW_FGCOLOUR_DEFVAL      _T("black")
/// which background colour for the font
#define   MP_MVIEW_BGCOLOUR_DEFVAL      _T("white")
/// which colour for signature
#define   MP_MVIEW_SIGCOLOUR_DEFVAL     _T("thistle")
/// which colour for URLS
#define   MP_MVIEW_URLCOLOUR_DEFVAL     _T("blue")
/// colour for attachment labels
#define   MP_MVIEW_ATTCOLOUR_DEFVAL     _T("dark olive green")

/// colorize quoted text?
#define   MP_MVIEW_QUOTED_COLOURIZE_DEFVAL 1L
/// cycle colours?
#define   MP_MVIEW_QUOTED_CYCLE_COLOURS_DEFVAL  0L
/// which colour for quoted text
#define   MP_MVIEW_QUOTED_COLOUR1_DEFVAL     _T("gray")
/// which colour for quoted text, seconds level
#define   MP_MVIEW_QUOTED_COLOUR2_DEFVAL     _T("brown")
/// which colour for quoted text, seconds level
#define   MP_MVIEW_QUOTED_COLOUR3_DEFVAL     _T("purple")
/// the maximum number of whitespaces prepending >
#define   MP_MVIEW_QUOTED_MAXWHITESPACE_DEFVAL  2L
/// the maximum number of A-Z prepending >
#define   MP_MVIEW_QUOTED_MAXALPHA_DEFVAL    3L
/// the colour for header names in the message view
#define   MP_MVIEW_HEADER_NAMES_COLOUR_DEFVAL  _T("RGB(128,64,64)")
/// the colour for header values in the message view
#define   MP_MVIEW_HEADER_VALUES_COLOUR_DEFVAL _T("black")
//@}
/**@name  Font settings for message view */
//@{
/// which font to use
#define   MP_FVIEW_FONT_DEFVAL         4L
/// which font size
#define   MP_FVIEW_FONT_SIZE_DEFVAL         DEFAULT_FONT_SIZE
/// the full font desc (replaces the 2 settings above)
#define   MP_FVIEW_FONT_DESC_DEFVAL   wxEmptyString
/// don't show full e-mail, only sender's name
#define   MP_FVIEW_NAMES_ONLY_DEFVAL         0L
/// which foreground colour for the font
#define   MP_FVIEW_FGCOLOUR_DEFVAL      _T("black")
/// which background colour for the font
#define   MP_FVIEW_BGCOLOUR_DEFVAL      _T("white")
/// colour for deleted messages
#define   MP_FVIEW_DELETEDCOLOUR_DEFVAL      _T("grey")
/// colour for new messages
#define   MP_FVIEW_NEWCOLOUR_DEFVAL      _T("orange")
/// colour for recent messages
#define   MP_FVIEW_RECENTCOLOUR_DEFVAL      _T("gold")
/// colour for unread messages
#define   MP_FVIEW_UNREADCOLOUR_DEFVAL      _T("blue")
/// colour for flagged messages
#define   MP_FVIEW_FLAGGEDCOLOUR_DEFVAL      _T("purple")
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
#define MP_FTREE_FGCOLOUR_DEFVAL wxEmptyString
/// the background colour for the folder tree
#define MP_FTREE_BGCOLOUR_DEFVAL wxEmptyString
/// show the currently opened folder specially?
#define MP_FTREE_SHOWOPENED_DEFVAL 1L
/// format for the folder tree entries
#define MP_FTREE_FORMAT_DEFVAL _T(" (%t, %u)")
/// reflect the folder status in its parent?
#define MP_FTREE_PROPAGATE_DEFVAL 1L
/// skip this folder when looking for next unread one in the tree
#define MP_FTREE_NEVER_UNREAD_DEFVAL 0L
/// go to this folder when Ctrl-Home is pressed
#define MP_FTREE_HOME_DEFVAL wxEmptyString
//@}
/**@name Font settings for compose view */
//@{
/// which font to use
#define   MP_CVIEW_FONT_DEFVAL         6L
/// which font size
#define   MP_CVIEW_FONT_SIZE_DEFVAL    DEFAULT_FONT_SIZE
/// the full font desc (replaces the 2 settings above)
#define   MP_CVIEW_FONT_DESC_DEFVAL   wxEmptyString
/// which foreground colour for the font
#define   MP_CVIEW_FGCOLOUR_DEFVAL      _T("black")
/// which background colour for the font
#define   MP_CVIEW_BGCOLOUR_DEFVAL      _T("white")
/// use the colours and font for the headers as well?
#define   MP_CVIEW_COLOUR_HEADERS_DEFVAL 0L
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
#  define MP_MSGS_SIMPLIFYING_REGEX_DEFVAL _T("^ *(R[Ee](\\[[0-9]+\\])?: +)*(\\[[^][]+\\]+)?(R[Ee](\\[[0-9]+\\])?: +)*")
#  define MP_MSGS_REPLACEMENT_STRING_DEFVAL _T("\\3")
#else // wxUSE_REGEX
   /// Remove list prefix when comparing message's subject to gather them
#  define MP_MSGS_REMOVE_LIST_PREFIX_GATHERING_DEFVAL 1l
   /// Remove list prefix when comparing message's subject to break threads
#  define MP_MSGS_REMOVE_LIST_PREFIX_BREAKING_DEFVAL 1l
#endif // wxUSE_REGEX

/// search criterium for searching in folders
#define MP_MSGS_SEARCH_CRIT_DEFVAL   0l
/// search argument
#define MP_MSGS_SEARCH_ARG_DEFVAL   wxEmptyString
/// open URLs with
#ifdef  OS_UNIX
#  define   MP_BROWSER_DEFVAL         _T("netscape")
#  define   MP_BROWSER_ISNS_DEFVAL    1
#  define   MP_BROWSER_INNW_DEFVAL    1
#else  // under Windows, we know better...
#  define   MP_BROWSER_DEFVAL         M_EMPTYSTRING
#  define   MP_BROWSER_ISNS_DEFVAL    0l
#  define   MP_BROWSER_INNW_DEFVAL    1l
#endif // Unix/Win

/// external editor to use for message composition (use %s for filename)
#ifdef OS_UNIX
#  define   MP_EXTERNALEDITOR_DEFVAL  _T("gvim %s")
#else
#  define   MP_EXTERNALEDITOR_DEFVAL  _T("notepad %s")
#endif // Unix/Win

/// start external editor automatically?
#define MP_ALWAYS_USE_EXTERNALEDITOR_DEFVAL    0l

/// PGP/GPG application
#ifdef OS_UNIX
#  define   MP_PGP_COMMAND_DEFVAL  _T("gpg")
#else
#  define   MP_PGP_COMMAND_DEFVAL  _T("gpg.exe")
#endif // Unix/Win

/// PGP/GPG key server for public keys
#define MP_PGP_KEYSERVER_DEFVAL _T("wwwkeys.eu.pgp.net")
/// get PGP key from server?
#define MP_PGP_GET_PUBKEY_DEFVAL 1l

/// command to execute when new mail arrives
#define   MP_USE_NEWMAILCOMMAND_DEFVAL   0l
#define   MP_NEWMAILCOMMAND_DEFVAL   M_EMPTYSTRING

/// play a sound on new mail?
#ifdef __CYGWIN__
#define MP_NEWMAIL_PLAY_SOUND_DEFVAL 0l
#else
#define MP_NEWMAIL_PLAY_SOUND_DEFVAL 1l
#endif // cygwin

#if defined(OS_UNIX) || defined(__CYGWIN__)
/// which sound to play?
#define MP_NEWMAIL_SOUND_FILE_DEFVAL M_BASEDIR _T("/newmail.wav")
/// the program to use to play this sound
#define MP_NEWMAIL_SOUND_PROGRAM_DEFVAL _T("/usr/bin/play %s")
#else // !OS_UNIX
/// which sound to play?
#define MP_NEWMAIL_SOUND_FILE_DEFVAL wxEmptyString
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
#define   MP_OUTGOINGFOLDER_DEFVAL  wxEmptyString // obsolete
/// Show all message headers?
#define   MP_SHOWHEADERS_DEFVAL         0l
/// Autocollect email addresses?
#define   MP_AUTOCOLLECT_DEFVAL     (long)M_ACTION_ALWAYS
/// Name of the address books for autocollected addresses
#define   MP_AUTOCOLLECT_ADB_DEFVAL    _T("autocollect.adb")
/// Autocollect email addresses from sender only ?
#define   MP_AUTOCOLLECT_SENDER_DEFVAL     1l
/// Autocollect entries with names only?
#define   MP_AUTOCOLLECT_NAMED_DEFVAL 0l

/// Default names for the SSL and crypto libraries
#ifdef OS_UNIX
   #define MP_SSL_DLL_SSL_DEFVAL   _T("libssl.so")
   #define MP_SSL_DLL_CRYPTO_DEFVAL _T("libcrypto.so")
#elif defined(OS_WIN)
   #if defined(__CYGWIN__)
      #define MP_SSL_DLL_SSL_DEFVAL   _T("cygssl.dll")
      #define MP_SSL_DLL_CRYPTO_DEFVAL _T("cygcrypto.dll")
   #else // !cygwin
      #define MP_SSL_DLL_SSL_DEFVAL   _T("ssleay32.dll")
      #define MP_SSL_DLL_CRYPTO_DEFVAL _T("libeay32.dll")
   #endif
#else // !Unix, !Win
   #define MP_SSL_DLL_SSL_DEFVAL   wxEmptyString
   #define MP_SSL_DLL_CRYPTO_DEFVAL wxEmptyString
#endif

/**@name message view settings */
//@{
/// support efax style incoming faxes
#define MP_INCFAX_SUPPORT_DEFVAL      1L
/// domains from which to support faxes, semicolon delimited
#define MP_INCFAX_DOMAINS_DEFVAL      _T("efax.com")
//@}
/// Use substrings in address expansion?
#define   MP_ADB_SUBSTRINGEXPANSION_DEFVAL 0l

/// replace "From" address with "To" in messages from oneself?
#define MP_FVIEW_FROM_REPLACE_DEFVAL 0L
/// the ':' separated list of addresses which are "from oneself"
#define MP_FROM_REPLACE_ADDRESSES_DEFVAL wxEmptyString

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
#define MP_RSH_PATH_DEFVAL _T("rsh")
/// the path to ssh
#define MP_SSH_PATH_DEFVAL _T("ssh")
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
#  define   MP_DEFAULT_SAVE_WILDCARD_DEFVAL   _T("*")
#  define   MP_DEFAULT_LOAD_WILDCARD_DEFVAL   _T("*")
#else
#  define   MP_DEFAULT_SAVE_WILDCARD_DEFVAL   _T("*.*")
#  define   MP_DEFAULT_LOAD_WILDCARD_DEFVAL   _T("*.*")
#endif
/// an entry used for testing
#define MP_TESTENTRY_DEFVAL      0L
/// Do we remote synchronise configuration settings?
#define MP_SYNC_REMOTE_DEFVAL   0L
/// IMAP folder to sync to
#define MP_SYNC_FOLDER_DEFVAL   _T("MahoganySharedConfig")
/// our last sync date
#define MP_SYNC_DATE_DEFVAL      0L
/// sync filters?
#define MP_SYNC_FILTERS_DEFVAL   0L
/// sync filters?
#define MP_SYNC_IDS_DEFVAL   0L
/// sync folders?
#define MP_SYNC_FOLDERS_DEFVAL   0L
/// sync folder tree
#define MP_SYNC_FOLDERGROUP_DEFVAL   wxEmptyString

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
#define MP_WHITE_LIST_DEFVAL _T("whitelist.adb")

/// treat mail in this folder as junk mail
#define MP_TREAT_AS_JUNK_MAIL_FOLDER_DEFVAL 0L

//@}

