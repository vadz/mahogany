///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   modules/NetscapeImporter.cpp
// Purpose:     import all the mail, news and adb relevant properties from
//              Netscape to Mahogany
// Author:      Michele Ravani ( based on Vadim Zeitlin Pine importer)
// Modified by:
// Created:     23.03.01
// CVS-ID:
// Copyright:   (c) 2000 Michele ravani <michele.ravani@bigfoot.com>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

//
// This code borrows heavily, very heavily
// from the Pine Import module implemented by Vadim Zeitlin.
//

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// TODO
//  - check superfluos includes

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "Mdefaults.h"
   #include "MApplication.h"

   #include <wx/hash.h>
#endif // USE_PCH

#include <wx/dir.h>
#include <wx/tokenzr.h>
#include <wx/confbase.h>
#include <wx/textfile.h>

#include "MImport.h"
#include "MFolder.h"
#include "MEvent.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_ADD_DEFAULT_HOSTNAME;
extern const MOption MP_AUTOMATIC_WORDWRAP;
extern const MOption MP_COMPOSE_BCC;
extern const MOption MP_COMPOSE_SIGNATURE;
extern const MOption MP_COMPOSE_USE_SIGNATURE;
extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_HOSTNAME;
extern const MOption MP_IMAPHOST;
extern const MOption MP_MAILCAP;
extern const MOption MP_MAX_MESSAGE_SIZE;
extern const MOption MP_MIMETYPES;
extern const MOption MP_MSGS_USE_THREADING;
extern const MOption MP_MVIEW_QUOTED_COLOUR1;
extern const MOption MP_MVIEW_QUOTED_COLOURIZE;
extern const MOption MP_NNTPHOST;
extern const MOption MP_NNTPHOST_LOGIN;
extern const MOption MP_OUTBOX_NAME;
extern const MOption MP_OUTGOINGFOLDER;
extern const MOption MP_PERSONALNAME;
extern const MOption MP_POLLINCOMINGDELAY;
extern const MOption MP_POPHOST;
extern const MOption MP_PRINT_COLOUR;
extern const MOption MP_PRINT_COMMAND;
extern const MOption MP_PRINT_ORIENTATION;
extern const MOption MP_PRINT_PAPER;
extern const MOption MP_REPLY_ADDRESS;
#ifdef OS_UNIX
extern const MOption MP_USE_SENDMAIL;
extern const MOption MP_SENDMAILCMD;
#endif
extern const MOption MP_SMTPHOST;
extern const MOption MP_SMTPHOST_LOGIN;
extern const MOption MP_USEOUTGOINGFOLDER;
extern const MOption MP_USERNAME;
extern const MOption MP_USEVCARD;
extern const MOption MP_USE_OUTBOX;
extern const MOption MP_VIEW_AUTOMATIC_WORDWRAP;
extern const MOption MP_VIEW_WRAPMARGIN;
extern const MOption MP_WRAPMARGIN;

// User/Dev Comments TODO
// - create an outbox folder setting [DONE]
//
// - use flags in folder creation
// The important parameter is "flags" which is a combination of masks defined
// in MImport.h:
//    enum
//    {
//       ImportFolder_SystemImport    = 0x0001, // import system folders
//       ImportFolder_SystemUseParent = 0x0003, // put them under parent
//       ImportFolder_AllUseParent    = 0x0006  // put all folders under parent
//    };

// i.e. you should use the provided parent folder for the normal folders only if
// AllUseParent flag is given (be careful to test for equality, not just for !=
// 0) and the system folders (such as inbox, drafts, ...) may be:

// 1. not imported at all (if !SystemImport)
// 2. imported under root (if !SystemUseParent)
// 3. imported under parent

//  These flags are really confusing, I know, but this is the price to pay for
// the flexiblity Nerijus requested to have for XFMail importer - have a look
// at it, BTW, you will see how it handles system folders (Pine code doesn't do
// it although it probably should as well).
//


// GENERAL TODO
// - set up makefile stuff to create the module out of many
//   header and source files. I like to separate interfaces from
//   implementation and to put each class in a file by the same name.
//   It decreased the number of things one has to know and makes the
//   code browsing simpler.
//
// - find the default location of netscape files on other OS
//
// - import the addressbook. Netscape uses a LDIF(right?), the generic communication
//   formar for LDAP server and clients. The import of the adb could be a side effect
//   of an LDAP integration module ('import from file')
//
// - implement the choice between 'sharing' the mail folders or physically move
//   them to the .M directory.
//
// - using '/' explicitly in the code is not portable [DONE]
//
// - check that the config files is backed up or implement it!
//
// - option to move mail in the Netscape's Inbox to New Mail (others? e.g. drafts, templates)
//
// - import filters
//
// - import newsgroups and status
//
// - check error detection and treatment.
//   (If I hold my breath until I turn blue, can I have exceptions?)
//
// - Misc folders, i.e. foo in foo.sbd
//    + create only if they are not empty [No]
//    + if the group folder with the same name is empty, create only a mailbox folder [DONE]
//    + rename to 'AAA Misc' to make it appear in the first slot in the folder [NO]
//      use SetTreeIndex(10) to position after system folders. [DONE]
//
// - implement importing of IMAP server stuff [ASK]




#define NR_SYS_FLD 6
static const wxString sysFolderList[] = {_T("Drafts"),
                                             _T("Inbox"),
                                             _T("Sent"),
                                             _T("Templates"),
                                             _T("Trash"),
                                             _T("Unsent Messages")};

//------------------------------------------
// Map struct
//------------------------------------------


// this is quite a crude solution, but I guess it'll do for a start,
// as it simplifies the mapping a bit.
//
// TODO:
// - ask for help in mapping those settings I've missed
// - find the complete set of Netscape keys
// - investigate a config file based solution
//   + config.in file with M #defines, preprocessed to create
//     a proper config file, possibly in XML format.
//     maps should be built in the given Import<type>Settings method
// - set up the makefile stuff to do it


#define NM_NONE 0
#define NM_IS_BOOL 1
#define NM_IS_INT 2
#define NM_IS_STRING 3          // empty strings not allowed
#define NM_IS_NEGATE_BOOL 4
#define NM_IS_STRNIL 5          // empty strings allowed

struct PrefMap {
  wxString npKey;     // the key in the Netscape pref file
  wxString mpKey;     // the key in the Mahogany congif file
  wxString desc;      // short description
  unsigned int type;  // type of the Mahogany pref
  bool procd;         // TRUE if entry has been processed.
};





// IDENTITY Preferences
static  PrefMap g_IdentityPrefMap[] = {
  {_T("mail.identity.username") , MP_PERSONALNAME , _T("user's full name"), NM_IS_STRING, FALSE },
  {_T("mail.identity.defaultdomain") , MP_HOSTNAME , _T("default domain"), NM_IS_STRING, FALSE },
  {_T("mail.identity.useremail") , MP_FROM_ADDRESS , _T("e-mail address"), NM_IS_STRING, FALSE },
  {_T("mail.identity.organization") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
  {_T("mail.identity.reply_to") , MP_REPLY_ADDRESS , _T("reply address"), NM_IS_STRING, FALSE },
  {_T("mail.attach_vcard") , MP_USEVCARD , _T("attach vCard to outgoing messages"), NM_IS_BOOL, FALSE },
  {_T("END"), _T("Ignored"), _T("No descrEond of list record"), NM_NONE }   // DO NOT REMOVE, hack to find the end
};

// IDENTITY Preferences
static  PrefMap g_NetworkPrefMap[] = {

 {_T("mail.smtp_name") , MP_SMTPHOST_LOGIN, _T("SMTP login name"), NM_IS_STRING, FALSE },
 // my guess that netscape uses the same name
 {_T("mail.smtp_name") , MP_NNTPHOST_LOGIN, _T("NNTP login name"), NM_IS_STRING, FALSE },
 {_T("mail.pop_name") , MP_USERNAME, _T("POP username"), NM_IS_STRING, FALSE },
 {_T("mail.pop_password") , _T("Ignored"), _T("Password for the POP server"), NM_IS_STRING, FALSE },
 {_T("network.hosts.smtp_server") , MP_SMTPHOST, _T("SMTP Server Name"), NM_IS_STRING, FALSE },
 {_T("network.hosts.nntp_server") , MP_NNTPHOST, _T("NNTP Server Name"), NM_IS_STRING, FALSE },
 {_T("network.hosts.pop_server") , MP_POPHOST, _T("POP Server Name"), NM_IS_STRING, FALSE },
   // imap stuff is not there yet. It is a bit more complex: a bunch of keys
   // like mail.imap.<servername>.<property>. I don't know enough to make sense
   // out of it at the moment. I may set the imap to nil.
 {_T("mail.imap.server_sub_directory") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.imap.root_dir") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.imap.local_copies") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.imap.server_ssl") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.imap.delete_is_move_to_trash") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 // something to use instead of send/fetchmail here?
#ifdef OS_UNIX
 {_T("mail.use_movemail") , MP_USE_SENDMAIL, _T("use mail moving program"), NM_IS_BOOL, FALSE },
 {_T("mail.use_builtin_movemail") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.movemail_program") , MP_SENDMAILCMD, _T("mail moving command"), NM_IS_STRING, FALSE },
#else
 {_T("mail.use_movemail") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.use_builtin_movemail") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.movemail_program") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
#endif
 {_T("mail.movemail_warn") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("END"), _T("Ignored"), _T("End of list record"), NM_NONE }   // DO NOT REMOVE, hack to find the end
};


// COMPOSE Preferences
static  PrefMap g_ComposePrefMap[] = {
 {_T("mail.wrap_long_lines") , MP_AUTOMATIC_WORDWRAP, _T("automatic line wrap"), NM_IS_BOOL, FALSE },
 {_T("mailnews.wraplength") , MP_WRAPMARGIN, _T("wrap lenght"), NM_IS_INT, FALSE },
   // additional bcc addresses: add to MP_COMPOSE_BCC if this true
 {_T("mail.default_cc") , _T("Special"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.use_default_cc") , _T("Special"), _T("No descr"), NM_NONE, FALSE },
   // directory where sent mail goes
 {_T("mail.default_fcc") , MP_OUTGOINGFOLDER, _T("sent mail folder"), NM_IS_STRNIL, FALSE }, //where copied
   // if true copy mail to def fcc
 {_T("mail.use_fcc") , MP_USEOUTGOINGFOLDER, _T("keep copies of sent mail"), NM_IS_BOOL, FALSE },
   // if set, put email addresse in BCC: MP_COMPOSE_BCC
 {_T("mail.cc_self") , _T("Special"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.auto_quote") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.html_compose") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
   // set MP_COMPOSE_USE_SIGNATURE if not empty
 {_T("mail.signature_file") , MP_COMPOSE_SIGNATURE , _T("filename of signature file"), NM_IS_STRNIL, FALSE },
 {_T("END"), _T("Ignored"), _T("End of list record"), NM_NONE }   // DO NOT REMOVE, hack to find the end
};



// FOLDER Preferences
static  PrefMap g_FolderPrefMap[] = {
  // no pref for the name of the outbox
  {_T("mail.deliver_immediately") , MP_USE_OUTBOX, _T("send messages later"), NM_IS_NEGATE_BOOL, FALSE },
   // map this to a very large number if "mail.check_new_mail" false
  {_T("mail.check_time") , MP_POLLINCOMINGDELAY, _T("interval between checks for incoming mail"), NM_IS_INT, FALSE },
  // AFAIK cannot switch off polling in M. Could set the interval to a veeery large number
  {_T("mail.check_new_mail") , _T("Special"), _T("check mail at intervals"), NM_IS_BOOL, FALSE },
   // not sure about this one. I mean ... even less sure than for the others
  {_T("mail.max_size") , MP_MAX_MESSAGE_SIZE, _T("max size for downloaded message"), NM_IS_INT, FALSE },
  {_T("END"), _T("Ignored"), _T("End of list record"), NM_NONE }   // DO NOT REMOVE, hack to find the end
};

// VIEWER Preferences
static  PrefMap g_ViewerPrefMap[] = {
 {_T("mail.quoted_style") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.quoted_size") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
   // color for quoted mails: MP_MVIEW_QUOTED_COLOUR1
   // if not "" set MP_MVIEW_QUOTED_COLOURIZE to true
 {_T("mail.citation_color") , MP_MVIEW_QUOTED_COLOUR1, _T("color for quoted mails"), NM_IS_STRING, FALSE },

 {_T("mail.wrap_long_lines") , MP_VIEW_AUTOMATIC_WORDWRAP, _T("automatic line wrap"), NM_IS_BOOL, FALSE },
 {_T("mailnews.wraplength") , MP_VIEW_WRAPMARGIN, _T("wrap lenght"), NM_IS_INT, FALSE },
 {_T("mail.thread_mail") , MP_MSGS_USE_THREADING, _T("display mail threads"), NM_IS_BOOL, FALSE },

  {_T("END"), _T("Ignored"), _T("End of list record"), NM_NONE }   // DO NOT REMOVE, hack to find the end
};


static  PrefMap g_RestPrefMap[] = {
  // very specially treated
 {_T("mail.directory") , _T("Special"), _T("mail directory"), NM_IS_STRING, FALSE },

 {_T("helpers.global_mime_types_file") , MP_MIMETYPES, _T("global mime types file"), NM_IS_STRING, FALSE },
 {_T("helpers.private_mime_types_file") , MP_MIMETYPES, _T("private mime types file"), NM_IS_STRING, FALSE },

 {_T("helpers.global_mailcap_file") , MP_MAILCAP, _T("global mailcap file"), NM_IS_STRING, FALSE },
 {_T("helpers.private_mailcap_file") , MP_MAILCAP, _T("private mailcap file"), NM_IS_STRING, FALSE },

 {_T("print.print_command") , MP_PRINT_COMMAND, _T("print command"), NM_IS_STRING, FALSE },
 {_T("print.print_reversed") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("print.print_color") , MP_PRINT_COLOUR, _T("print color"), NM_IS_BOOL, FALSE },
 {_T("print.print_landscape") , MP_PRINT_ORIENTATION, _T("print orientation"), NM_IS_NEGATE_BOOL, FALSE },
 // see what is used in Netscape
 {_T("print.print_paper_size") , MP_PRINT_PAPER, _T("paper size"), NM_IS_STRING, FALSE },

 {_T("intl.character_set") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("intl.font_charset") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("intl.font_spec_list") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("intl.accept_languages") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },

 {_T("mail.play_sound") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.strictly_mime") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.file_attach_binary") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.addr_book.lastnamefirst") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },

 {_T("mail.signature_date") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.leave_on_server") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.limit_message_size") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.prompt_purge_threshhold") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.purge_threshhold") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.use_mapi_server") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.server_type") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.fixed_width_messages") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.empty_trash") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.remember_password") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.support_skey") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.pane_config") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.sort_by") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mail.default_html_action") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },

 {_T("mailnews.reuse_message_window") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mailnews.reuse_thread_window") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mailnews.message_in_thread_window") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mailnews.nicknames_only") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mailnews.reply_on_top") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("mailnews.reply_with_extra_lines") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },

 {_T("network.ftp.passive") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.max_connections") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.tcpbufsize") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.hosts.socks_server") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.hosts.socks_serverport") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.ftp") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.ftp_port") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.http") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.http_port") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.gopher") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.gopher_port") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.wais") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.wais_port") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.ssl") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.ssl_port") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.no_proxies_on") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.type") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("network.proxy.autoconfig_url") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },

 {_T("news.default_cc") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.default_fcc") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.cc_self") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.use_fcc") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },

 {_T("news.directory") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.notify.on") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.max_articles") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.cache_xover") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.show_first_unread") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.sash_geometry") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.thread_news") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.pane_config") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.sort_by") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.keep.method") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.keep.days") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.keep.count") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.keep.only_unread") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.remove_bodies.by_age") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.remove_bodies.days") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.server_port") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("news.server_is_secure") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },

 {_T("offline.startup_mode") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("offline.news.download.unread_only") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("offline.news.download.by_date") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("offline.news.download.use_days") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("offline.news.download.days") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },
 {_T("offline.news.download.increments") , _T("Not mapped"), _T("No descr"), NM_NONE, FALSE },

 {_T("security.email_as_ftp_password") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.submit_email_forms") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.warn_entering_secure") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.warn_leaving_secure") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.warn_viewing_mixed") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.warn_submit_insecure") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.enable_java") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("javascript.enabled") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.enable_ssl2") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.enable_ssl3") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.ciphers") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.default_personal_cert") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.use_password") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.ask_for_password") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("security.password_lifetime") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("custtoolbar.has_toolbar_folder") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("custtoolbar.personal_toolbar_folder") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.author") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.html_editor") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.image_editor") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.template_location") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.auto_save_delay") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.use_custom_colors") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.background_color") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.text_color") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.link_color") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.active_link_color") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.followed_link_color") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.background_image") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.publish_keep_links") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.publish_keep_images") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.publish_location") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.publish_username") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.publish_password") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.publish_save_password") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.publish_browse_location") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("editor.show_copyright") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },

 {_T("fortezza.toggle") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("fortezza.timeout") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },

 {_T("general.startup.browser") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.startup.mail") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.startup.news") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.startup.editor") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.startup.conference") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.startup.netcaster") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.startup.calendar") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.always_load_images") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.help_source.site") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("general.help_source.url") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("images.dither") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },
 {_T("images.incremental_display") , _T("Ignored"), _T("No descr"), NM_NONE, FALSE },

 {_T("END"), _T("Ignored"), _T("No descr"), NM_NONE }   // DO NOT REMOVE, hack to find the end
 };


// this should be ok on unix
static const wxString g_VarIdent      = _T("user_pref");   // identifies a key-value entry
static const wxString g_GlobalPrefDir = _T("/usr/lib/netscape");
static const wxString g_PrefFile      = _T("preferences.js");
static const wxString g_PrefFile2      = _T("liprefs.js");
static const wxString g_DefNetscapePrefDir = _T("$HOME/.netscape");
static const wxString g_DefNetscapeMailDir = _T("$HOME/nsmail");

static const wxChar   g_CommentChar   = '/';

//---------------------------------------------
// UTILITY CLASSES
//---------------------------------------------

// TODO
// - pile this stuff into header and impl classes in  a modules subdir
//   nicely one class per file.
// - makefile infrastructure to create .so and .a


// ----------------------------------------------------------------------------
// class MyFolderArray
//   simply a wxArray of MFolder pointers that calls DecRef for the items
//   in the array when destroyed. Simplifies cleanup
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(MFolder *, FolderArray);

class MyFolderArray: public FolderArray
{
public:

  ~MyFolderArray()
   {
     for (unsigned k = 0; k < GetCount(); k++ )
      Item(k)->DecRef();
   }
};


// ----------------------------------------------------------------------------
// class MyHashTable
//   utility class to get the primitives from the hashtable
// ----------------------------------------------------------------------------

class MyHashTable
{
public:

  MyHashTable();
  ~MyHashTable();

  bool Exist(const wxString& key) const;
  bool GetValue(const wxString& key, bool& value) const;
  bool GetValue(const wxString& key, wxString& value ) const;
  bool GetValue(const wxString& key, unsigned long& value ) const;

  void Put(const wxString& key, const wxString& val);
  void Delete(const wxString& key);

private:
  wxHashTable m_tbl;

};

MyHashTable::MyHashTable()
  : m_tbl(wxKEY_STRING)
{}

// I'm not too sure about this stuff ...
// It is getting down to details I can't remember
// why isn't a wxString a wxObject? What about hashes of strings?
MyHashTable::~MyHashTable()
{
  // should delete the strings hier;
  m_tbl.BeginFind();
  wxHashTable::Node* node = NULL;
  while ( (node = m_tbl.Next()) != NULL )
   delete (wxString*)node->GetData();

  //  m_tbl.DeleteContents(FALSE);  // just ot make sure, they are deleted

}

void MyHashTable::Put(const wxString& key, const wxString& val)
{
  wxString* tmp = new wxString(val);
  m_tbl.Put(key,(wxObject*)tmp);
}

void MyHashTable::Delete(const wxString& key)
{
  wxString* tmp = (wxString*) m_tbl.Delete(key);
  delete tmp;
}

bool MyHashTable::Exist(const wxString& key) const
{
  wxString* tmp = (wxString *)m_tbl.Get(key);
  return ( tmp != NULL );
}

bool MyHashTable::GetValue(const wxString& key, bool& value) const
{
  value = FALSE;
  wxString* tmp = (wxString *)m_tbl.Get(key);
  if ( tmp )
   {
     value = (( *tmp == _T("true") ) || ( *tmp == _T("TRUE") ) || ( *tmp == _T("1")));
     return TRUE;
   }
  else
   return FALSE;
}

bool MyHashTable::GetValue(const wxString& key, wxString& value ) const
{
  value.Empty();
  wxString* tmp = (wxString *)m_tbl.Get(key);
  if ( tmp )
   {
     value = *tmp;
     return TRUE;
   }
  else
   return FALSE;
}

bool MyHashTable::GetValue(const wxString& key, unsigned long& value ) const
{
  wxString* tmp = (wxString *)m_tbl.Get(key);

  if ( tmp && tmp->ToULong(&value) )
   return TRUE;

  value = (unsigned long)-1; // FIXME: is this really needed?

  return FALSE;
}


//---------------------------------------------
// CLASS MNetscapeImporter
//---------------------------------------------

class MNetscapeImporter : public MImporter
{
public:

  MNetscapeImporter();

  virtual bool Applies() const;
  virtual int  GetFeatures() const;

  virtual bool ImportADB();
  virtual bool ImportFolders(MFolder *folderParent, int flags);
  virtual bool ImportSettings();
  virtual bool ImportFilters();

  DECLARE_M_IMPORTER();

private:

  // call ImportSettingsFromFile() if the file exists, do nothing (and return
  // TRUE) otherwise
  bool ImportSettingsFromFileIfExists( const wxString& prefs );

  // parses the preferences file to find key-value pairs
  bool ImportSettingsFromFile( const wxString& prefs );
  bool ImportIdentitySettings ( MyHashTable& tbl );
  bool ImportNetworkSettings ( MyHashTable& tbl );
  bool ImportComposeSettings ( MyHashTable& tbl );
  bool ImportFolderSettings ( MyHashTable& tbl );
  bool ImportViewerSettings ( MyHashTable& tbl );
  bool ImportRestSettings ( MyHashTable& tbl );

  bool ImportSettingList( PrefMap* map, const MyHashTable& tbl);

  bool WriteProfileEntry(const wxString& key, const wxString& val, const wxString& desc );
  bool WriteProfileEntry(const wxString& key, const int val, const wxString& desc );
  bool WriteProfileEntry(const wxString& key, const bool val, const wxString& desc );

  bool CreateFolders(MFolder *parent, const wxString& dir, int flags);

  wxString m_MailDir;    // it is a free user choice, stored in preferences
  wxString m_PrefDir;    // it is a free user choice


};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor & dtor
// ----------------------------------------------------------------------------

MNetscapeImporter::MNetscapeImporter()
  : m_MailDir(wxExpandEnvVars(g_DefNetscapeMailDir)),
    m_PrefDir(wxExpandEnvVars(g_DefNetscapePrefDir))
{}


// ----------------------------------------------------------------------------
// generic
// ----------------------------------------------------------------------------

// TODO: mention with which Netscape versions it works (i.e. should it be
//       "Import settings from Netscape 4.x"? "6.x"?)
IMPLEMENT_M_IMPORTER(MNetscapeImporter, _T("Netscape"),
                     gettext_noop("Import settings from Netscape"));

int MNetscapeImporter::GetFeatures() const
{
  // return Import_Folders | Import_Settings | Import_ADB;
  return Import_Folders | Import_Settings;
}

// ----------------------------------------------------------------------------
// determine if Netscape is installed
// ----------------------------------------------------------------------------

bool MNetscapeImporter::Applies() const
{
  // just check for ~/.netscape directory
  bool b = wxDir::Exists(m_PrefDir.c_str());
  return b;
}

// ----------------------------------------------------------------------------
// import the Netscape ADB
// ----------------------------------------------------------------------------

bool MNetscapeImporter::ImportADB()
{
#if 0
   // import the Netscape address book from ~/.addressbook
   AdbImporter *importer = AdbImporter::GetImporterByName("AdbNetscapeImporter");
   if ( !importer )
   {
      wxLogError(_("%s address book import module not found."), "Netscape");

      return FALSE;
   }

   wxString filename = importer->GetDefaultFilename();
   wxLogMessage(_("Starting importing %s address book '%s'..."),
                "Netscape", filename.c_str());
   bool ok = AdbImport(filename, "Netscape.adb", "Netscape Address Book", importer);

   importer->DecRef();

   if ( ok )
      wxLogMessage(_("Successfully imported %s address book."), "Netscape");
   else
      wxLogError(_("Failed to import %s address book."), "Netscape");

   return ok;
#endif // 0

  return FALSE;
}

// ----------------------------------------------------------------------------
// import the Netscape folders
// ----------------------------------------------------------------------------

bool MNetscapeImporter::ImportFolders(MFolder *folderParent, int flags)
{
  // create a folder for each mbox file

  // if the mail dir was found in the preferences, it will be used
  // otherwise the default ($HOME/nsmail) will have to do.

  if (! wxDir::Exists(m_MailDir.c_str()) )
   {
     // TODO
     // - ask the user for his Netscape mail dir
     //   On the other hand it should ahve been read in prefs
     wxLogMessage(_("Cannot import folders, directory '%s' doesn't exist"),
                  m_MailDir.c_str());
     return FALSE;
   }

  wxDir dir(m_MailDir);
  if ( ! dir.IsOpened() )    // if it can't be opened bail out
   return FALSE;          // looks like the isOpened already logs a message

  // the parent for all folders: use the given one, don't create an extra level
  // of indirection if the user doesn't want it
  if ( CreateFolders(folderParent, m_MailDir, flags) )
   {
     MEventManager::Send
      (
       new MEventFolderTreeChangeData
       (
        folderParent ? folderParent->GetFullName() : String(),
        MEventFolderTreeChangeData::CreateUnder
        )
       );
   }
  // then is ok
  else
   // TODO
   // - remove the created folders, something went wrong
   return FALSE;

  return TRUE;
}


// Recursively called method which creates the FoFs and mail folders
// as found in Netscape.
// Netscape allows to store messages in the FoFs. It implements this
// functionality by creating a file with the name of the folder and
// at the same time a directory (same name + '.sbd') where the subfolders
// will be stored.
// M lacks this functionality, therefore messages in the Netscape FoF will
// be stored in a subfolder called "Misc"

bool MNetscapeImporter::CreateFolders(MFolder *parent,
                                      const wxString& dir,
                                      int flags )
{
  // based on Pine and XFMail Importer by Vadim and Nerijus
  static int level = 0;

  wxDir currDir(dir);

  if ( ! currDir.IsOpened() )    // if it can't be opened bail out
   return FALSE;          // looks like the isOpened already logs a message

  // find the folders
  wxString filename;
  wxArrayString fileList;
  bool cont = currDir.GetFirst(&filename, _T("*"), wxDIR_FILES);
  while ( cont )
   {
     fileList.Add(filename);
     cont = currDir.GetNext(&filename);
   }

  // find the Folders of Folders (FoF)
  wxArrayString dirList;
  cont = currDir.GetFirst(&filename, _T("*.sbd"), wxDIR_DIRS); // FoF in Netscape have .sbd extension
  while ( cont )
   {
     dirList.Add(filename);
     cont = currDir.GetNext(&filename);
   }

  // not really necessary
  fileList.Sort();
  dirList.Sort();

  if ( !fileList.GetCount() && !dirList.GetCount() )
   {
    wxLogMessage(_("No folders found in '%s'."), dir.c_str());

    // we can consider the operation successful
    return TRUE;
   }

  // as far as I know there isn't a flag in Netscape to mark
  // system folders, but I don't know much. So, if you know
  // please let me know

  // Usually, system folders are at root level and have given names
  // Eliminate them from the file list, if user doesn't want to import them

  if (level == 0) {                             // only at root level
    if (!(flags & ImportFolder_SystemImport) )      // don't want to import them
      {
        for (int i=0; i<NR_SYS_FLD; i++)
          fileList.Remove(sysFolderList[i]); 
        wxLogMessage(_T("NOTE: >>>>>>> Netscape system folders not imported"));
      }
  }    

  MFolder *folder = NULL;
  MFolder *subFolder = NULL;
  MyFolderArray folderList;
  wxString dirFldName;

  folderList.Alloc(25);

  // loop through the found directories
  // for each one,
  // - create a folder
  // - check if a file with the 'same' name exists
  // - if yes, create a subfolder named "Misc"
  // - call this method for the directory
  // - create all the 'real' mail folders

  // TODO
  // - in the case of failure, remove the created folders
  //   (essentially before each 'return FALSE;'
  // - [DONE] find out the type (MF_?) of FoFs [DONE]

  // in the next for loop no system folders will be treated anyway
  MFolder *tmpParent = NULL;
  if (level == 0) {
    if ( (flags & ImportFolder_AllUseParent)
         == ImportFolder_AllUseParent )
      tmpParent = parent;
  }
  else 
    tmpParent = parent;   // at lower levels, the dad is known
  
  for ( size_t n = 0; n < dirList.GetCount(); n++ )
   {
     const wxString& name = dirList[n];
     wxString path;
     path << dir << DIR_SEPARATOR << name;
     dirFldName = name.BeforeLast('.');

     // if directory empty,skip it to avoid creating empty folders
     // for instance a folder containing a empty Misc could be created
     // only group folders which contain 'real' folders should be created
     wxDir tmpDir(path);
     wxString tmpStr;
     if ( (! tmpDir.IsOpened()) ||
          (! tmpDir.GetFirst(&tmpStr, _T("*"), wxDIR_FILES | wxDIR_DIRS)) ||
          (tmpStr.empty()) )
       continue;

     folder = CreateFolderTreeEntry (tmpParent,    // parent may be NULL
                                     dirFldName,   // the folder name
                                     MF_GROUP,     //            type
                                     0,            //            flags
                                     path,         //            path
                                     FALSE         // don't notify
                                     );

     if ( folder )
      {
        folderList.Add(folder);
        wxLogMessage(_("Imported group folder: %s."),dirFldName.c_str());
      }
     else
      return FALSE;

     // check if there is a file matching (without .sbd)
     int i = fileList.Index( dirFldName.c_str() );
     if ( i != wxNOT_FOUND)
      {
        // TODO
        // - ask the user what name does he want to give to the folder

        wxString tmpPath;
        tmpPath << dir << DIR_SEPARATOR << fileList[i];
        subFolder = CreateFolderTreeEntry ( folder,    // parent may be NULL
                                   _T("Misc"),      // the folder name
                                   MF_FILE,   //            type
                                   0,         //            flags
                                   tmpPath,      //            path
                                   FALSE      // don't notify
                                   );


        if ( subFolder )
         {
           subFolder->SetTreeIndex(10);   // popsition folder after system folders
           folderList.Add(subFolder);
           fileList.RemoveAt(i);      // this one has been created, remove from filelist
           wxLogMessage(_("NOTE: >>>>>> Created 'AAA Misc' folder to contain the msgs currently in group folder %s."),
                        dirFldName.c_str());
         }
        else
         return FALSE;
      }

     // crude way to know if we are at the root mail dir level
     // another would be to check if the current dir is m_MailDir
     level++; 
     // recursive call
     if ( ! CreateFolders( folder, path, flags ) )
      return FALSE;
     level--;
   }

  // we have created all the directories, etc
  // now create the normal folders

  for ( size_t n = 0; n < fileList.GetCount(); n++ )
  {
    const wxString& name = fileList[n];
    wxString path;
    path << dir << DIR_SEPARATOR << name;

     // find out the folder type (system or not) by walking the list
     // to know how to set the parent folder (accordig to flags)
    tmpParent = NULL;
     
    if (level == 0) {
      // look mum, I'm making fire with two stones!
      bool found = FALSE;
      for (int i=0; i<NR_SYS_FLD; i++)
        if (name == sysFolderList[i]) {
          found = TRUE;
          break;
        }
      
      if ( ! found )  {    // it isn't a system folder
        if ( (flags & ImportFolder_AllUseParent)
             == ImportFolder_AllUseParent )
          tmpParent = parent;
      }
      else                        // it is, it is
        if ( (flags & ImportFolder_SystemUseParent) 
             == ImportFolder_SystemUseParent )
          tmpParent = parent;
    }
    else 
      tmpParent = parent;   // at lower levels, the dad is known
    
    folder = CreateFolderTreeEntry ( tmpParent,    // parent may be NULL
                                     name,      // the folder name
                                     MF_FILE,   //            type
                                     0,         //            flags
                                     path,      //            path
                                     FALSE      // don't notify
                                     );
    if ( folder )
      {
        folderList.Add(folder);
        wxLogMessage(_("Imported mail folder: %s "), name.c_str());
      }
    else
      return FALSE;
  }
  
  return TRUE;
  }



// ----------------------------------------------------------------------------
// import the Netscape settings
// ----------------------------------------------------------------------------

bool MNetscapeImporter::ImportSettings()
{

   // parse different netscape config files and pick the settings of interest to
   // us from it. Try the global preference file first

  wxString filename = g_GlobalPrefDir + DIR_SEPARATOR + g_PrefFile;

  if ( ! ImportSettingsFromFileIfExists(filename) )
   wxLogMessage(_("Couldn't import the global preferences file: %s."),
             filename.c_str());

  // user own preference files
  // entries here will override the global settings
  // Two files 'liprefs.js' and 'preferences.js' exist
  // mostly the latter is the most recent, but I don't know
  // the strategy beyond them. Moreover the contents overlap

  // usually 'liprefs' is older, process it first.
  // It doesn't matter if it fails
  filename = m_PrefDir + DIR_SEPARATOR + g_PrefFile2;
  ImportSettingsFromFileIfExists(filename);

  // 'preferences.js' is the main one
  // if it doesn't exist, bail out
  filename = m_PrefDir + DIR_SEPARATOR + g_PrefFile;
  if (! wxFile::Exists(filename.c_str()) )
   {
     // TODO
     // - ask user if he knows where the prefs file is
     return FALSE;
   }

  bool status = ImportSettingsFromFileIfExists(filename);
  if ( !status )
  {
     wxLogMessage(_("Couldn't import the user preferences file: %s."),
                  filename.c_str());
  }

  return status;
}

bool
MNetscapeImporter::ImportSettingsFromFileIfExists(const wxString& filename)
{
   if ( !wxFile::Exists(filename) )
   {
      // pretend everything is ok
      return TRUE;
   }

   return ImportSettingsFromFile(filename);
}


bool MNetscapeImporter::ImportSettingsFromFile(const wxString& filename)
{
  wxTextFile file(filename);
  if ( !file.Open() )
      return FALSE;

  wxString token;
  wxStringTokenizer tkz;
  wxString varName;
  wxString value;
  wxString msg;
  size_t nLines = file.GetLineCount();



  MyHashTable keyval;   // to keep the key-value pairs

  for ( size_t nLine = 0; nLine < nLines; nLine++ )
   {
      const wxString& line = file[nLine];

      // skip empty lines and the comments
      if ( !line || line[0] == g_CommentChar)
      continue;

      // extract variable name and its value
      int nEq = line.Find(g_VarIdent);

     // lines which do not contain a key-value pair will log a message
      if ( nEq == wxNOT_FOUND )
      {
         wxLogDebug(_T("%s(%lu): missing variable identifier ('%s')."),
                    filename.c_str(),
                    (unsigned long)nLine + 1,
                    g_VarIdent.c_str());

         // skip line
         continue;
      }

     // Very, Very primitive parsing of the prefs line
     // TODO
     //  - check out alternatives
     //    + reuse mozilla code
     //    + write a small parser
     //    + use regex

     tkz.SetString(line, _T("(,)"));
     token = tkz.GetNextToken();  // get rid of the first part
     varName = tkz.GetNextToken();  // the variable name
     value = tkz.GetNextToken();

     // get rid of white space
     value.Trim(); value.Trim(FALSE);
     varName.Trim(); varName.Trim(FALSE);

     // clean away eventual quotes
     if (varName[0u] == '"' && varName[varName.Len()-1] == '"')
      varName = varName(1,varName.Len()-2);

     if (value[0u] == '"' && value[value.Len()-1] == '"')
      value = value(1,value.Len()-2);

     // and now the white space again, e.g. " \" the value \""
     value.Trim(); value.Trim(FALSE);
     varName.Trim(); varName.Trim(FALSE);

     // key-value found (hopefully), add to hashtable
     keyval.Put(varName,value);
   }

  // a special case
  if ( keyval.Exist(_T("mail.directory")) )
   keyval.GetValue(_T("mail.directory"),m_MailDir);

  ImportIdentitySettings( keyval );
  ImportNetworkSettings ( keyval );
  ImportComposeSettings ( keyval );
  ImportFolderSettings ( keyval );
  ImportViewerSettings ( keyval );
  ImportRestSettings ( keyval );


  return TRUE;
}

bool MNetscapeImporter::ImportIdentitySettings ( MyHashTable& tbl )
{
  wxLogMessage(_T(">>>>>>>>>> Import identity settings"));

  PrefMap* map = g_IdentityPrefMap;

  if ( ! ImportSettingList(map, tbl) )
   return FALSE;


  // do the special stuff (derived settings etc)

  for (int i=0; map[i].npKey != _T("END"); i++)
   {
     // set default domain flag if name has been imported
     if ( map[i].npKey == _T("mail.identity.defaultdomain"))
      WriteProfileEntry(MP_ADD_DEFAULT_HOSTNAME, map[i].procd, _T("use default domain"));
   }

  return TRUE;
}


bool MNetscapeImporter::ImportNetworkSettings ( MyHashTable& tbl )
{
  wxLogMessage(_T(">>>>>>>>>> Import network settings"));

  PrefMap* map = g_NetworkPrefMap;

  if ( ! ImportSettingList(map, tbl) )
   return FALSE;


  // do the special stuff (derived settings etc)

  // set the imap host to nil (at least until implemented!)
  // Silly, but to insure that the right WriteProfileEntry
  // is called, not the bool one
  wxString tmp = wxEmptyString;
  WriteProfileEntry(MP_IMAPHOST, tmp, _T("imap server name"));

  // for (int i=0; map[i].npKey != "END"; i++)
  //    {
  //    }

  return TRUE;
}

bool MNetscapeImporter::ImportComposeSettings ( MyHashTable& tbl )
{
  wxLogMessage(_T(">>>>>>>>>> Import compose settings"));

  wxString lstr;

  // Netscape uses the complete path to the folder instead of the name
  if ( tbl.GetValue(_T("mail.default_fcc"), lstr) && !lstr.empty())
   {
     // if I'm correct, Netscape names the files and the folders the same
     // therefore taking the last of the path should be sufficient
     lstr = lstr.AfterLast(DIR_SEPARATOR);
     tbl.Delete(_T("mail.default_fcc"));     // it isn't a dictionary, multiple entries ok
     tbl.Put(_T("mail.default_fcc"), lstr);   // change the value in the hashtable
   }                                     // insertion will be done by ImportSettingList

  PrefMap* map = g_ComposePrefMap;

  if ( ! ImportSettingList(map, tbl) )
   return FALSE;

  // add additional bcc addresses

  bool tmpBool = FALSE;

  if ( tbl.GetValue(_T("mail.use_default_cc"), tmpBool) && tmpBool) // BCC to others
   tbl.GetValue(_T("mail.default_cc"), lstr);

  wxString lstr2;
  if ( tbl.GetValue(_T("mail.cc_self"), tmpBool) && tmpBool )    // BCC to self
   tbl.GetValue(_T("mail.identity.useremail"), lstr2);

  lstr = lstr2 + lstr;

  if (! lstr.empty() )
   WriteProfileEntry(MP_COMPOSE_BCC, lstr, _T("blind copy addresses"));

  // use the fact that these variables are set to infer that they are also
  // used is weak, but it is all I have at the moment
  if ( tbl.GetValue(_T("mail.signature_file"), lstr) && !lstr.empty())
   WriteProfileEntry(MP_COMPOSE_USE_SIGNATURE, TRUE, _T("use signature file"));

  return TRUE;
}


bool MNetscapeImporter::ImportFolderSettings ( MyHashTable& tbl )
{
  wxLogMessage(_T(">>>>>>>>>> Import folder settings"));

  PrefMap* map = g_FolderPrefMap;

  if ( ! ImportSettingList(map, tbl) )
   return FALSE;

  bool tmpBool = FALSE;
  wxString lstr;

  // pref says not to check for new mail, then set to a very large number
  // otherwise leave it as set.
  if ( tbl.GetValue(_T("mail.check_new_mail"), tmpBool) && ! tmpBool )
   WriteProfileEntry(MP_POLLINCOMINGDELAY, 30000, _T("new mail polling delay"));

  // if Not deliver immediately, then create an Outbox to be used
  if ( tbl.GetValue(_T("mail.deliver_immediately"), tmpBool) && ! tmpBool )
	WriteProfileEntry(MP_OUTBOX_NAME, String(_T("Outbox")), _T("Outgoing mail folder"));

  return TRUE;
}

bool MNetscapeImporter::ImportViewerSettings ( MyHashTable& tbl )
{

  wxLogMessage(_T(">>>>>>>>>> Import viewer settings"));

  PrefMap* map = g_ViewerPrefMap;

  if ( ! ImportSettingList(map, tbl) )
   return FALSE;

  wxString lstr;
  // use the fact that these variables are set to infer that they are also
  // used is weak, but it is all I have at the moment
  if ( tbl.GetValue(_T("mail.citation_color"), lstr) && !lstr.empty())
   WriteProfileEntry(MP_MVIEW_QUOTED_COLOURIZE, TRUE, _T("use color for quoted messages"));

  return TRUE;
}

bool MNetscapeImporter::ImportRestSettings ( MyHashTable& tbl )
{
  // just to run through the rest of the prefs

  wxLogMessage(_T(">>>>>>>>>> Import remaining settings"));

  PrefMap* map = g_RestPrefMap;

  if ( ! ImportSettingList(map, tbl) )
   return FALSE;

  return TRUE;
}

bool MNetscapeImporter::ImportSettingList( PrefMap* map, const MyHashTable& tbl)
{
  wxString value;
  bool tmp = FALSE;
  unsigned long lval = (unsigned long)-1;

  for (int i=0; map[i].npKey != _T("END"); i++)
   {
     if ( ! tbl.Exist(map[i].npKey) )  // no key
      {
        continue; // next
      }

     else if (map[i].mpKey == _T("Not mapped"))
      {
        wxLogMessage(_("Key '%s' hasn't been mapped yet"), map[i].npKey.c_str());
        map[i].procd = TRUE; // mark to find out which ones in the file are also in the maps
        continue;
      }

     else if (( map[i].mpKey == _T("Ignored")) || ( map[i].mpKey == _T("Special") ))
      {
        map[i].procd = TRUE;
        continue;
      }

     switch (map[i].type)
      {
      case NM_IS_BOOL:
      case NM_IS_NEGATE_BOOL:
        {
         if ( tbl.GetValue(map[i].npKey,tmp) )
           {
            if (map[i].type == NM_IS_NEGATE_BOOL)
              tmp = !tmp;
            map[i].procd = WriteProfileEntry(map[i].mpKey, tmp, map[i].desc);
           }
         else
           wxLogMessage(_("Parsing error for key '%s'"),
                     map[i].npKey.c_str());
         break;
        }
      case NM_IS_STRING:
      case NM_IS_STRNIL:
         {
          if ( tbl.GetValue(map[i].npKey,value) )
            {
             if ((map[i].type == NM_IS_STRING) && value.empty() )
               {
                wxLogMessage(_("Bad value for key '%s': cannot be empty"),
                          map[i].npKey.c_str());
                break;
               }
             map[i].procd = WriteProfileEntry(map[i].mpKey, value, map[i].desc);
            }
         else
           wxLogMessage(_("Parsing error for key '%s'"),
                     map[i].npKey.c_str());
         break;
        }
      case NM_IS_INT:
        {
         if ( tbl.GetValue(map[i].npKey,lval) )
           {
            map[i].procd = WriteProfileEntry(map[i].mpKey, (int)lval, map[i].desc);
           }
         else
           wxLogMessage(_("Type mismatch for key '%s' ulong expected.'"),
                     map[i].npKey.c_str());
         break;
        }
      default:
        wxLogMessage(_("Bad type key '%s'"), map[i].npKey.c_str());
      }
     if ( ! map[i].procd )
      return FALSE;
   }
  return TRUE;
}




bool MNetscapeImporter::WriteProfileEntry(const wxString& key, const wxString& val, const wxString& desc )
{
  bool status = FALSE;

  // let's make sure that if there are environment variable
  // they are expanded
  wxString tmpVal = wxExpandEnvVars(val);

  Profile* l_Profile = mApplication->GetProfile();

  if ( (status = l_Profile->writeEntry( key, tmpVal)) == true )
   wxLogMessage(_("Imported '%s' setting from %s: %s."),
             desc.c_str(),"Netscape",tmpVal.c_str());
  else
   wxLogWarning(_("Failed to write '%s' entry to profile"), desc.c_str());

  return status;
}

bool MNetscapeImporter::WriteProfileEntry(const wxString& key, const int val, const wxString& desc )
{
  bool status = FALSE;

  Profile* l_Profile = mApplication->GetProfile();

  if ( (status = l_Profile->writeEntry(key, val)) == true )
   wxLogMessage(_("Imported '%s' setting from %s: %u."),
             desc.c_str(),"Netscape",val);
  else
   wxLogWarning(_("Failed to write '%s' entry to profile"), desc.c_str());

  return status;
}

bool MNetscapeImporter::WriteProfileEntry(const wxString& key, const bool val, const wxString& desc )
{
  bool status = FALSE;

  Profile* l_Profile = mApplication->GetProfile();

  if ( val )
   status = l_Profile->writeEntry( key.c_str(), 1L);
  else
   status = l_Profile->writeEntry( key.c_str(), 0L);

  if ( status )
   wxLogMessage(_("Imported '%s' setting from %s: %u."), desc.c_str(),"Netscape",val);
  else
   wxLogWarning(_("Failed to write '%s' entry to profile"), desc.c_str());

  return status;
}




// ----------------------------------------------------------------------------
// import the Netscape filters
// ----------------------------------------------------------------------------

bool MNetscapeImporter::ImportFilters()
{
   return FALSE;
}


// ----------------------------------------------------------------------------
// Netscape Keys, not yet used
// ----------------------------------------------------------------------------



 // this is windows positioning stuff, I guess we really need to import it
/*  {"taskbar.floating" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"taskbar.horizontal" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"taskbar.ontop" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"taskbar.x" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"taskbar.y" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Browser.Navigation_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Browser.Navigation_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Browser.Navigation_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Browser.Location_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Browser.Location_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Browser.Location_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Browser.Personal_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Browser.Personal_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Browser.Personal_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Messenger.Navigation_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Messenger.Navigation_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Messenger.Navigation_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Messenger.Location_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Messenger.Location_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Messenger.Location_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Messages.Navigation_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Messages.Navigation_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Messages.Navigation_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Messages.Location_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Messages.Location_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Messages.Location_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Folders.Navigation_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Folders.Navigation_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Folders.Navigation_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Folders.Location_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Folders.Location_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Folders.Location_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Address_Book.Address_Book_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Address_Book.Address_Book_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Address_Book.Address_Book_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Compose_Message.Message_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Compose_Message.Message_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Compose_Message.Message_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Composer.Composition_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Composer.Composition_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Composer.Composition_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"custtoolbar.Composer.Formatting_Toolbar.position" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Composer.Formatting_Toolbar.showing" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"custtoolbar.Composer.Formatting_Toolbar.open" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"browser.win_width" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"browser.win_height" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"mail.compose.win_width" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"mail.compose.win_height" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */


/*  {"editor.win_width" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"editor.win_height" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"mail.folder.win_width" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"mail.folder.win_height" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"mail.msg.win_width" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"mail.msg.win_height" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

/*  {"mail.thread.win_width" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */
/*  {"mail.thread.win_height" , "Ignored", _T("No descr"), NM_NONE, FALSE }, */

