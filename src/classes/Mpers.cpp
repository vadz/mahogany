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
#endif // USE_PCH

#include "Mpers.h"

// ----------------------------------------------------------------------------
// MPersMsgBox class
// ----------------------------------------------------------------------------

class MPersMsgBox
{
public:
   MPersMsgBox();

private:
   // for GetPersMsgBoxName() only
   size_t GetId() const { return m_id; }

   size_t m_id;

   friend String GetPersMsgBoxName(const MPersMsgBox *which);
};

// ============================================================================
// data
// ============================================================================

// note that we need to first define the variable itself and then define a
// pointer to it and we use an auxiliary header for this as we can't use just a
// macro in this file because a macro expansion can't contain #ifdef but some
// ids are only used in some configurations
#define DECL_OR_DEF(name) static const MPersMsgBox M_MSGBOX_OBJ_##name
#include "MpersIds.h"

#undef DECL_OR_DEF
#define DECL_OR_DEF(name) \
   extern const MPersMsgBox *M_MSGBOX_##name = &M_MSGBOX_OBJ_##name
#include "MpersIds.h"

// the order of the entries in this array must match the order of the
// declarations in MpersIds.h!
static const struct
{
   const wxChar *name;
   const wxChar *desc;
} gs_persMsgBoxData[] =
{
   { _T("AskSpecifyDir"),            gettext_noop("prompt for global directory if not found") },
#ifdef OS_UNIX
   { _T("AskRunAsRoot"),             gettext_noop("warn if Mahogany is run as root") },
#endif // OS_UNIX
   { _T("SendOutboxOnExit"),         gettext_noop("send unsent messages on exit") },
   { _T("AbandonCriticalFolders"),   gettext_noop("prompt before abandoning critical folders ") },
   { _T("GoOnlineToSendOutbox"),     gettext_noop("go online to send Outbox") },
   { _T("FixTemplate"),              gettext_noop("propose to fix template with errors") },
   { _T("AskForSig"),                gettext_noop("ask for signature if none found") },
   { _T("AskSaveHeaders"),           gettext_noop("ask for confirmation before closing if the headers were modified in the composer") },
   { _T("MessageSent"),              gettext_noop("show notification after sending the message") },
   { _T("AskForExtEdit"),            gettext_noop("propose to change external editor settings if unset") },
   { _T("MimeTypeCorrect"),          gettext_noop("ask confirmation for guessed MIME types") },
   { _T("ConfigNetFromCompose"),     gettext_noop("propose to configure network settings before sending the message if necessary") },
   { _T("SendemptySubject"),         gettext_noop("confirm sending a message without subject") },
   { _T("ConfirmFolderDelete"),      gettext_noop("confirm removing folder from the folder tree") },
   { _T("ConfirmFolderPhysDelete"),  gettext_noop("confirm before deleting folder with its contents") },
   { _T("MarkRead"),                 gettext_noop("mark all articles as read before closing the folder") },
#ifdef USE_DIALUP
   { _T("DialUpConnectedMsg"),       gettext_noop("show notification on dial-up network connection") },
   { _T("DialUpDisconnectedMsg"),    gettext_noop("show notification on dial-up network disconnection") },
   { _T("DialUpOnOpenFolder"),       gettext_noop("propose to start dial up networking before trying to open a remote folder") },
   { _T("NetDownOpenAnyway"),        gettext_noop("warn before opening remote folder while not being online") },
   { _T("NoNetPingAnyway"),          gettext_noop("ping remote folders even while offline") },
#endif // USE_DIALUP
   // "ConfirmExit" mist be same as MP_CONFIRMEXIT_NAME!
   { _T("ConfirmExit"),              gettext_noop("don't ask confirmation before exiting the program") },
   { _T("AskLogin"),                 gettext_noop("ask for the login name when creating the folder if required") },
   { _T("AskPwd"),                   gettext_noop("ask for the password when creating the folder if required") },
   { _T("SavePwd"),                  gettext_noop("ask confirmation before saving the password in the file") },
   { _T("GoOfflineSendFirst"),       gettext_noop("propose to send outgoing messages before hanging up") },
   { _T("OpenUnaccessibleFolder"),   gettext_noop("warn before trying to reopen folder which couldn't be opened the last time") },
   { _T("ChangeUnaccessibleFolderSettings"), gettext_noop("propose to change settings of a folder which couldn't be opened the last time before reopening it") },
   { _T("AskUrlBrowser"),            gettext_noop("ask for the WWW browser if it was not configured") },
   { _T("OptTestAsk"),               gettext_noop("propose to test new settings after changing any important ones") },
   { _T("WarnRestartOpt"),           gettext_noop("warn if some options changes don't take effect until program restart") },
   { _T("SaveTemplate"),             gettext_noop("propose to save changed template before closing it") },
   { _T("DeleteTemplate"),           gettext_noop("ask for confirmation before deleting a template") },
   { _T("MailNoNetQueuedMessage"),   gettext_noop("show notification if the message is queued in Outbox and not sent out immediately") },
   { _T("MailQueuedMessage"),        gettext_noop("show notification for queued messages") },
   { _T("MailSentMessage"),          gettext_noop("show notification for sent messages") },
   { _T("TestMailSent"),             gettext_noop("show successful test message") },
   { _T("AdbDeleteEntry"),           gettext_noop("ask for confirmation before deleting the address book entries") },
   { _T("ConfirmAdbImporter"),       gettext_noop("ask for confirmation before importing unrecognized address book files") },
   { _T("ModulesWarning"),           gettext_noop("warning that module changes take effect only after restart") },
   { _T("BbdbSaveDialog"),           gettext_noop("ask for confirmation before saving address books in BBDB format") },
   { _T("FolderGroupHint"),          gettext_noop("show explanation after creating a folder group") },
   { _T("SignatureTooLong"),         gettext_noop("warn if signature is longer than netiquette recommends") },
   { _T("RememberPwd"),              gettext_noop("propose to permanently remember passwords entered interactively") },
   { _T("KeepPwd"),                  gettext_noop("propose to keep passwords entered interactively for the duration of this session") },
   { _T("ShowLogWinHint"),           gettext_noop("show the hint about reopening the log window when it is being closed") },
   { _T("AutoExpunge"),              gettext_noop("expunge deleted messages before closing the folder") },
   { _T("SuspendAutoCollectFolder"), gettext_noop("suspend auto-collecting messages from failed incoming folder") },
#if 0 // unused any more
   { _T("RulesMismatchWarn1"),       gettext_noop("Warning that filter rules do not match dialog")},
   { _T("RulesMismatchWarn2"),       gettext_noop("Warning that filter rules have been edited") },
#endif // 0
   { _T("FilterReplace"),            gettext_noop("ask whether to replace filter when adding a new filter") },
   { _T("AddAllSubfolders"),         gettext_noop("create all subfolders automatically instead of browsing them") },

   { _T("StoreRemoteNow"),           gettext_noop("store remote configuration from options dialog") },
   { _T("GetRemoteNow"),             gettext_noop("retrieve remote configuration from options dialog") },
   { _T("OverwriteRemote"),          gettext_noop("ask before overwriting remote configuration settings") },
   { _T("RetrieveRemote"),           gettext_noop("retrieve remote settings at startup") },
   { _T("StoreRemote"),              gettext_noop("store remote settings at shutdown") },
   { _T("StoredRemote"),             gettext_noop("show confirmation after remote config was saved") },

   { _T("ExplainGlobalPasswd"),      gettext_noop("show explanation before asking for global password") },
   { _T("FilterNotUsedYet"),         gettext_noop("warn that newly created filter is unused") },
   { _T("FilterOverwrite"),          gettext_noop("ask confirmation before overwriting a filter with another one") },
   { _T("ImportUnderRoot"),          gettext_noop("ask where do you want to import folders") },
   { _T("MoveExpungeConfirm"),       gettext_noop("confirm expunging messages after moving") },
   { _T("ApplyQuickFilter"),         gettext_noop("propose to apply quick filter after creation") },
   { _T("BrowseImapServers"),        gettext_noop("propose to get all folders from IMAP server") },
   { _T("GfxNotInlined"),            gettext_noop("inline the big images") },
   { _T("EditOnOpenFail"),           gettext_noop("propose to edit folder settings if opening it failed") },
   { _T("ExplainColClick"),          gettext_noop("give explanation when clicking on a column in the folder view") },
#ifdef USE_VIEWER_BAR
   { _T("ViewerBarTip"),             gettext_noop("give tip about reenabling the viewer bar when closing it") },
#endif // USE_VIEWER_BAR
   { _T("EmptyTrashOnExit"),         gettext_noop("purge trash folder on exit") },
   { _T("SendOffline"),              gettext_noop("send mail when the system is offline") },
   { _T("AskForVCard"),              gettext_noop("propose to attach a vCard if none specified") },
   { _T("ReenableHint"),             gettext_noop("hint about how to reenable disabled message boxes") },
   { _T("DraftSaved"),               gettext_noop("confirm saving the message as a draft") },
   { _T("DraftAutoDel"),             gettext_noop("show a warning when \"automatically delete drafts\" option is on") },
   { _T("ConfirmResend"),            gettext_noop("confirm resending messages") },
   { _T("ConfirmZap"),               gettext_noop("confirm permanently deleting messages") },
   { _T("ZapSpam"),                  gettext_noop("permanently delete message after marking them as spam") },
   { _T("RememberPGPPassphrase"),    gettext_noop("remember PGP/GPG passphrase in memory") },
   { _T("GetPGPPubKey"),             gettext_noop("retrieve PGP/GPG public keys from key server") },
   { _T("SearchAgainIfNoMatch"),     gettext_noop("propose to search again if no matches were found") },
   { _T("UseCreateFolderWizard"),    gettext_noop("use wizard when creating a new folder") },
   { _T("FormatParagraphBeforeExit"),gettext_noop("format all paragraphs in composer before exit") },
   { _T("LangChangedWarn"),          gettext_noop("warn if 8bit characters are used in composer and language is not set") },
   { _T("ContUpdateAfterError"),     gettext_noop("ask if we should continue updating folders after an error") },
   { _T("RemoveAllAttach"),          gettext_noop("ask for confirmation before removing all attachments from composer") },
   //{ _T(""), gettext_noop("") },
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
extern String GetPersMsgBoxName(const MPersMsgBox *which)
{
   ASSERT_MSG( M_MSGBOX_MAX->GetId() == WXSIZEOF(gs_persMsgBoxData),
               _T("should be kept in sync!") );

   CHECK( which, _T(""), _T("NULL pointer in GetPersMsgBoxName") );

   return gs_persMsgBoxData[which->GetId()].name;
}

/// return a user-readable description of the pers msg box with given name
extern String GetPersMsgBoxHelp(const String& name)
{
   String s;
   for ( size_t n = 0; n < WXSIZEOF(gs_persMsgBoxData); n++ )
   {
      if ( gs_persMsgBoxData[n].name == name )
      {
         s = wxGetTranslation(gs_persMsgBoxData[n].desc);
         break;
      }
   }

   if ( !s )
   {
      s.Printf(_("unknown (%s)"), name.c_str());
   }

   return s;
}

/* vi: set tw=0: */
