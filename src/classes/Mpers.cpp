///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   src/classes/Mpers.cpp
// Purpose:     names of persistent message boxes used by M
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.03.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include <wx/intl.h>
#endif

#include "Mpers.h"

// ============================================================================
// data
// ============================================================================

// note that we need to declare the variable first as being extern and then
// define it (this can't be done all at once - at least not without a dummy
// ctor...) and we use an auxiliary header for this as we can't use just a
// macro in this file because a macro expansion can't contain #ifdef but some
// ids are only used in some configurations
#define DECL_OR_DEF extern const
#include "MpersIds.h"

#undef DECL_OR_DEF
#define DECL_OR_DEF const
#include "MpersIds.h"

// the order of the entries in this array must match the order of the
// declarations in MpersIds.h!
static const struct
{
   const char *name;
   const char *desc;
} gs_persMsgBoxData[] =
{
   { "AskSpecifyDir",            gettext_noop("prompt for global directory if not found") },
#ifdef OS_UNIX
   { "AskRunAsRoot",             gettext_noop("warn if Mahogany is run as root") },
#endif // OS_UNIX
   { "SendOutboxOnExit",         gettext_noop("ask whether to send unsent messages on exit") },
   { "AbandonCriticalFolders",   gettext_noop("prompt before abandoning critical folders ") },
   { "GoOnlineToSendOutbox",     gettext_noop("ask whether to go online to send Outbox") },
   { "FixTemplate",              gettext_noop("propose to fix template with errors") },
   { "AskForSig",                gettext_noop("ask for signature if none found") },
   { "UnsavedCloseAnyway",       gettext_noop("propose to save message before closing") },
   { "MessageSent",              gettext_noop("show notification after sending the message") },
   { "AskForExtEdit",            gettext_noop("propose to change external editor settings if unset") },
   { "MimeTypeCorrect",          gettext_noop("ask confirmation for guessed MIME types") },
   { "ConfigNetFromCompose",     gettext_noop("propose to configure network settings before sending the message if necessary") },
   { "SendemptySubject",         gettext_noop("ask confirmation before sending messages without subject") },
   { "ConfirmFolderDelete",      gettext_noop("ask confirmation before removing folder from the folder tree") },
   { "ConfirmFolderPhysDelete",  gettext_noop("ask confirmation before deleting folder with its contents") },
   { "MarkRead",                 gettext_noop("ask whether to mark all articles as read before closing folder") },
#ifdef USE_DIALUP
   { "DialUpConnectedMsg",       gettext_noop("show notification on dial-up network connection") },
   { "DialUpDisconnectedMsg",    gettext_noop("show notification on dial-up network disconnection") },
   { "DialUpOnOpenFolder",       gettext_noop("propose to start dial up networking before trying to open a remote folder") },
   { "NetDownOpenAnyway",        gettext_noop("warn before opening remote folder while not being online") },
   { "NoNetPingAnyway",          gettext_noop("ask whether to ping remote folders offline") },
#endif // USE_DIALUP
   // "ConfirmExit" is the same as MP_CONFIRMEXIT_NAME!
   { "ConfirmExit",              gettext_noop("ask confirmation before exiting the program") },
   { "AskLogin",                 gettext_noop("ask for the login name when opening the folder if required") },
   { "AskPwd",                   gettext_noop("ask for the password when opening the folder if required") },
   { "GoOfflineSendFirst",       gettext_noop("propose to send outgoing messages before hanging up") },
   { "OpenUnaccessibleFolder",   gettext_noop("warn before trying to reopen folder which couldn't be opened the last time") },
   { "ChangeUnaccessibleFolderSettings", gettext_noop("propose to change settings of a folder which couldn't be opened the last time before reopening it") },
   { "AskUrlBrowser",            gettext_noop("ask for the WWW browser if it was not configured") },
   { "OptTestAsk",               gettext_noop("propose to test new settings after changing any important ones") },
   { "WarnRestartOpt",           gettext_noop("warn if some options changes don't take effect until program restart") },
   { "SaveTemplate",             gettext_noop("propose to save changed template before closing it") },
   { "MailNoNetQueuedMessage",   gettext_noop("show notification if the message is queued in Outbox and not sent out immediately") },
   { "MailQueuedMessage",        gettext_noop("show notification for queued messages") },
   { "MailSentMessage",          gettext_noop("show notification for sent messages") },
   { "TestMailSent",             gettext_noop("show successful test message") },
   { "AdbDeleteEntry",           gettext_noop("ask for confirmation before deleting the address book entries") },
   { "ConfirmAdbImporter",       gettext_noop("ask for confirmation before importing unrecognized address book files") },
   { "ModulesWarning",           gettext_noop("warning that module changes take effect only after restart") },
   { "BbdbSaveDialog",           gettext_noop("ask for confirmation before saving address books in BBDB format") },
   { "FolderGroupHint",          gettext_noop("show explanation after creating a folder group") },
   { "SignatureTooLong",         gettext_noop("warn if signature is longer than netiquette recommends") },
   { "RememberPwd",              gettext_noop("propose to permanently remember passwords entered interactively") },
   { "KeepPwd",                  gettext_noop("propose to keep passwords entered interactively for the duration of this session") },
   { "ShowLogWinHint",           gettext_noop("show the hint about reopening the log window when it is being closed") },
   { "AutoExpunge",              gettext_noop("ask to expunge deleted messages before closing the folder") },
   { "SuspendAutoCollectFolder", gettext_noop("ask to suspend auto-collecting messages from failed incoming folder") },
#if 0 // unused any more
   { "RulesMismatchWarn1",       gettext_noop("Warning that filter rules do not match dialog")},
   { "RulesMismatchWarn2",       gettext_noop("Warning that filter rules have been edited") },
#endif // 0
   { "FilterReplace",            gettext_noop("ask whether to replace filter when adding a new filter") },
   { "AddAllSubfolders",         gettext_noop("create all subfolders automatically instead of browsing them") },

   { "StoreRemoteNow",           gettext_noop("question whether to store remote configuration from options dialog") },
   { "GetRemoteNow",             gettext_noop("question whether to retrieve remote configuration from options dialog") },
   { "OverwriteRemote",          gettext_noop("ask before overwriting remote configuration settings") },
   { "RetrieveRemote",           gettext_noop("question whether to retrieve remote settings at startup") },
   { "StoreRemote",              gettext_noop("question whether to store remote settings at shutdown") },
   { "StoredRemote",             gettext_noop("confirmation that remote config was saved") },

   { "ExplainGlobalPasswd",      gettext_noop("show explanation before asking for global password") },
   { "FilterNotUsedYet",         gettext_noop("warn that newly created filter is unused") },
   { "FilterOverwrite",          gettext_noop("ask confirmation before overwriting a filter with another one") },
   { "ImportUnderRoot",          gettext_noop("ask where do you want to import folders") },
   { "MoveExpungeConfirm",       gettext_noop("confirm expunging messages after moving") },
   { "ApplyQuickFilter",         gettext_noop("propose to apply quick filter after creation") },
   { "BrowseImapServers",        gettext_noop("propose to get all folders from IMAP server") },
   { "GfxNotInlined",            gettext_noop("ask if big images should be inlined") },
   { "EditOnOpenFail",           gettext_noop("propose to edit folder settings if opening it failed") },
   { "ExplainColClick",          gettext_noop("give explanation when clicking on a column in the folder view") },
   { "ViewerBarTip",             gettext_noop("give tip about reenabling the viewer bar when closing it") },
   { "EmptyTrashOnExit",         gettext_noop("ask whether to purge trash folder on exit") },
   { "SendOffline",              gettext_noop("send mail when the system is offline") },
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// generate a new unique msgbox id
static inline size_t NewMPersMsgBox()
{
   static size_t s_msgboxLast = 0;

   return s_msgboxLast++;
}

// ----------------------------------------------------------------------------
// MPersMsgBox implementation
// ----------------------------------------------------------------------------

MPersMsgBox::MPersMsgBox()
{
   m_id = NewMPersMsgBox();
}

// ----------------------------------------------------------------------------
// our public API
// ----------------------------------------------------------------------------

/// return the name to use for the given persistent msg box
extern String GetPersMsgBoxName(const MPersMsgBox& which)
{
   ASSERT_MSG( M_MSGBOX_MAX.GetId() == WXSIZEOF(gs_persMsgBoxData),
               "should be kept in sync!" );

   return gs_persMsgBoxData[which.GetId()].name;
}

/// return a user-readable description of the pers msg box with given name
extern String GetPersMsgBoxHelp(const String& name)
{
   String s;
   for ( size_t n = 0; n < WXSIZEOF(gs_persMsgBoxData); n++ )
   {
      if ( gs_persMsgBoxData[n].name == name )
      {
         s = _(gs_persMsgBoxData[n].desc);
         break;
      }
   }

   if ( !s )
   {
      s.Printf(_("unknown (%s)"), name.c_str());
   }

   return s;
}

