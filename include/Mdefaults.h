/*-*- c++ -*-********************************************************
 * Mdefaults.h : all default defines for readEntry() config entries *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$              *
 *
 * $Log$
 * Revision 1.6  1998/05/02 18:29:57  KB
 * After many problems, Python integration is eventually taking off -
 * works.
 *
 *
 *******************************************************************/

#ifndef MDEFAULTS_H
#define   MDEFAULTS_H

/** @name The sections of the configuration file. */
//@{
/** The section in the global configuration file used for storing
    profiles (trailing '/' required).
*/
#ifndef M_PROFILE_CONFIG_SECTION
#   define   M_PROFILE_CONFIG_SECTION   "/M/Profiles"
#endif

/** The section in the global configuration file used for storing
    window positions (trailing '/' required).
*/
#ifndef M_FRAMES_CONFIG_SECTION
#   define   M_FRAMES_CONFIG_SECTION   "/M/Frames"
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
/// search paths for M's directory
#define   MC_PATHLIST         "PathList"
/// the name of M's root directory
#define   MC_ROOTDIRNAME         "RootDirectoryName"
/// the user's M directory
#define   MC_USERDIR         "UserDirectory"
/// the name of the M directory 
#define   MC_USER_MDIR         "MDirName"
/// the default icon for frames
#define   MC_ICON_MFRAME         "MFrameIcon"
/// the icon for the main frame
#define   MC_ICON_MAINFRAME      "MainFrameIcon"
/// the icon directory
#define   MC_ICONDIR         "IconDirectory"
/// the path for finding profiles
#define   MC_PROFILE_PATH         "ProfilePath"
/// the extension to use for profile files
#define   MC_PROFILE_EXTENSION      "ProfileExtension"
/// the path where to find .afm files
#define   MC_AFMPATH         "AfmPath"
/// the path to the /etc directories (configuration files)
#define   MC_ETCPATH         "ConfigPath"
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
/// the printf() format for dates
#define   MC_DATE_FMT         "DateFormat"
/// show console window
#define   MC_SHOWCONSOLE      "ShowConsole"  
/// name of address database
#define   MC_ADBFILE         "AddressBook"
/// names of folders to open at startup (semicolon separated list)
#define   MC_OPENFOLDERS         "OpenFolders"
/// path for Python
#define   MC_PYTHONPATH         "PythonPath"
/// start-up script to run
#define     MC_STARTUPSCRIPT               "StartupScript"
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
/// the folder type for a mailbox (0 = system inbox, 1 = file, 2 =pop3, 3 = imap, 4 = nntp
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
/// show XFaces?
#define   MP_SHOW_XFACES         "ShowXFaces"
/// which font to use
#define   MP_FTEXT_FONT         "FontFamily"
/// which font size
#define   MP_FTEXT_SIZE         "FontSize"
/// which font style
#define   MP_FTEXT_STYLE         "FontStyle"
/// which font weight
#define   MP_FTEXT_WEIGHT         "FontWeight"
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS      "HighlightURL"
/// open URLs with
#define   MP_BROWSER         "Browser"
/// the wildcard for save dialog
//@}
//@}

/** @name default values of configuration entries */
//@{
/// shall we record default values in configuration files
#define   MC_RECORDDEFAULTS_D      1
/// default window position x
#define   MC_XPOS_D        0L
/// default window position y
#define   MC_YPOS_D        0L
/// default window width
#define   MC_WIDTH_D   600L
/// default window height
#define   MC_HEIGHT_D   400L
/// path list for M's directory
#define   MC_PATHLIST_D   "/usr/local:/usr/:/opt:/opt/local:/usr/opt:/usr/local/opt:/home/karsten/src/Projects/M/test"
/// the name of M's root directory
#define   MC_ROOTDIRNAME_D   "M"
/// the user's M directory
#define   MC_USERDIR_D         ""
/// the name of the M directory 
#define   MC_USER_MDIR_D         ".M"
/// the default icon for frames
#define   MC_ICON_MFRAME_D      "mframe.xpm"
/// the icon for the main frame
#define   MC_ICON_MAINFRAME_D      "mainframe.xpm"
/// the icon directoy
#define   MC_ICONDIR_D         "icons"
/// the profile path
#define   MC_PROFILE_PATH_D      "/home/karsten/src/Projects/M/test/profiles:profiles:."
/// the extension to use for profile files
#define   MC_PROFILE_EXTENSION_D      ".profile"
/// the path where to find .afm files
#define   MC_AFMPATH_D "/usr/share:/usr/lib:/usr/local/share:/usr/local/lib:/opt/ghostscript:/opt/enscript"
/// the path to the /etc directories (configuration files)
#define   MC_ETCPATH_D   "/etc:/usr/etc:/usr/local/etc"
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
/// the printf() format for dates
#define   MC_DATE_FMT_D         "%1\\$2u.%2\\$2u.%3\\$4u"
/// show console window
#define   MC_SHOWCONSOLE_D      1
/// name of address database
#define   MC_ADBFILE_D         "M.adb"
/// names of folders to open at startup
#define   MC_OPENFOLDERS_D      "INBOX"
/// path for Python
#define   MC_PYTHONPATH_D         ""
/// start-up script to run
#define     MC_STARTUPSCRIPT_D            "Minit"
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
#define   MP_ADD_EXTRAHEADERS_D      1
/// list of extra headers, semicolon separated name=value
#define   MP_EXTRAHEADERS_D      "X-Mailer=M"
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
#define   MP_COMPOSE_SIGNATURE_D      "$HOME/.signature"
/// the folder type for a mailbox (0 = system inbox, 1 = file, 2 =pop3, 3 = imap, 4 = nntp
#define   MP_FOLDER_TYPE_D         1
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
/// show XFaces?
#define   MP_SHOW_XFACES_D      1
/// which font to use
#define   MP_FTEXT_FONT_D         0
/// which font size
#define   MP_FTEXT_SIZE_D         12
/// which font style
#define   MP_FTEXT_STYLE_D      1
/// which font weight
#define   MP_FTEXT_WEIGHT_D      0
/// highlight URLS?
#define   MP_HIGHLIGHT_URLS_D      1
/// open URLs with
#define   MP_BROWSER_D         "Mnetscape"
/// the wildcard for save dialog
#ifdef OS_UNIX
#define   MP_DEFAULT_SAVE_WILDCARD_D   "*"
#define   MP_DEFAULT_LOAD_WILDCARD_D   "*"
#else
#define   MP_DEFAULT_SAVE_WILDCARD_D   "*.*"
#define   MP_DEFAULT_LOAD_WILDCARD_D   "*.*"
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
