///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxOptionsDlg.cpp - M options dialog
// Purpose:     allows to easily change from one dialog all program options
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifdef USE_PYTHON
#    include  "Mcallbacks.h"  // Python fix for MCB_* declares
#endif //USE_PYTHON

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "MHelp.h"
#  include "strutil.h"

#  include "Sorting.h"
#  include "Threading.h"
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "guidef.h"

#  include <wx/statbox.h>
#  include <wx/stattext.h>
#  include <wx/textdlg.h>  // for wxGetTextFromUser()
#  include <wx/checklst.h>
#  include <wx/layout.h>
#endif // USE_PCH

#include <wx/confbase.h>

#if defined(OS_WIN) && defined(USE_DIALUP)
#  include <wx/dialup.h>          // for wxDialUpManager
#endif

#include "Mpers.h"
#include "Moptions.h"            // we need all MP_XXX for our arrays
#include "ConfigSource.h"

// we have to include these 3 headers just for wxOptionsPageNewMail... move it
// to another file maybe?
#include "MFolder.h"
#include "gui/wxFiltersDialog.h"
#include "FolderMonitor.h"

#include "MessageView.h" // for list of available viewers

#include "gui/wxBrowseButton.h"  // for wxStaticCast(wxColorBrowseButton) only
#include "gui/wxOptionsDlg.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxMenuDefs.h"

#include "HeadersDialogs.h"
#include "FolderView.h"
#include "TemplateDialog.h"

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_CONFIRM_EXIT;
extern const MPersMsgBox *M_MSGBOX_OPT_STOREREMOTENOW;

// ----------------------------------------------------------------------------
// conditional compilation
// ----------------------------------------------------------------------------

#ifdef OS_UNIX
   // define this to allow using MTA (typically only for Unix)
   #define USE_SENDMAIL

   // BBDB support only makes sense for Unix
   #define USE_BBDB

   // do we need the OpenSSL libs?
   #ifdef USE_SSL
      #define USE_OPENSSL
   #endif
#endif

// define this to have additional TCP parameters in the options dialog
#define USE_TCP_TIMEOUTS

// define this to use the font description for specifying the fonts instead of
// font family/size combination which doesn't allow the user to set all
// available fonts he has
#define USE_FONT_DESC

// define this to show IMAP look ahead setting to the user -- IMHO this is
// normally unnecessary as the program should be smart enough to determine it
// automatically
//#define USE_IMAP_PREFETCH

// ----------------------------------------------------------------------------
// data
// ----------------------------------------------------------------------------

// first and last are shifted by -1, i.e. the range of fields for the page Foo
// is from ConfigField_FooFirst + 1 to ConfigField_FooLast inclusive - this is
// more convenient as the number of entries in a page is then just Last - First
// without any +1.
//
// only the wxOptionsPage ctor and ms_aIdentityPages depend on this, so if this
// is (for some reason) changed, it would be the only places to change.
//
// if you modify this enum, you must modify the data below too (search for
// DONT_FORGET_TO_MODIFY to find it)
enum ConfigFields
{
   // identity
   ConfigField_IdentFirst = -1,
   ConfigField_IdentHelp,
   ConfigField_PersonalName,
   ConfigField_Organization,
   ConfigField_UserName,
   ConfigField_AddressHelp,
   ConfigField_ReturnAddress,
   ConfigField_ReplyAddress,
   ConfigField_HostnameHelp,
   ConfigField_AddDefaultHostname,
   ConfigField_Hostname,
   ConfigField_SetReplyFromTo,
   ConfigField_VCardHelp,
   ConfigField_UseVCard,
   ConfigField_VCardFile,
   ConfigField_SetPassword,
   ConfigField_IdentLast = ConfigField_SetPassword,

   // network
   ConfigField_NetworkFirst = ConfigField_IdentLast,
   ConfigField_ServersHelp,
   ConfigField_PopServer,
   ConfigField_ImapServer,
#ifdef USE_SENDMAIL
   ConfigField_UseSendmail,
   ConfigField_SendmailCmd,
#endif
   ConfigField_MailServer,
   ConfigField_NewsServer,

   ConfigField_8BitHelp,
   ConfigField_8Bit,

   ConfigField_MailServerLoginHelp,
   ConfigField_MailServerLogin,
   ConfigField_MailServerPassword,
   ConfigField_NewsServerLogin,
   ConfigField_NewsServerPassword,
   ConfigField_SenderHelp,
   ConfigField_GuessSender,
   ConfigField_Sender,
   ConfigField_DisabledAuthsHelp,
   ConfigField_DisabledAuths,
#ifdef USE_SSL
   ConfigField_SSLHelp,
   ConfigField_SmtpServerSSL,
   ConfigField_SmtpServerSSLUnsigned,
   ConfigField_NntpServerSSL,
   ConfigField_NntpServerSSLUnsigned,
#endif // USE_SSL

#ifdef USE_DIALUP
   ConfigField_DialUpHelp,
   ConfigField_DialUpSupport,
   ConfigField_BeaconHost,
#ifdef OS_WIN
   ConfigField_NetConnection,
#elif defined(OS_UNIX)
   ConfigField_NetOnCommand,
   ConfigField_NetOffCommand,
#endif // platform
#endif // USE_DIALUP

   ConfigField_TimeoutInfo,
   ConfigField_OpenTimeout,
#ifdef USE_TCP_TIMEOUTS
   ConfigField_ReadTimeout,
   ConfigField_WriteTimeout,
   ConfigField_CloseTimeout,
#endif // USE_TCP_TIMEOUTS
#ifdef USE_IMAP_PREFETCH
   ConfigField_LookAheadHelp,
   Configfield_IMAPLookAhead,
#endif // USE_IMAP_PREFETCH
   ConfigField_RshHelp,
   ConfigField_RshTimeout,
   ConfigField_NetworkLast = ConfigField_RshTimeout,

   // new mail handling
   ConfigField_NewMailFirst = ConfigField_NetworkLast,
   ConfigField_NewMailHelpAppWide,
   ConfigField_NewMailHelpFolder,
   ConfigField_NewMailConfigFilters,
   ConfigField_NewMailTreatAsJunkFolder,
   ConfigField_NewMailCollect,
   ConfigField_NewMailCollectToFolder,
   ConfigField_NewMailLeaveOnServer,
   ConfigField_NewMailNotifyHelp,
   ConfigField_NewMailNotifyUseCommand,
   ConfigField_NewMailNotifyCommand,
   ConfigField_NewMailSoundHelp,
   ConfigField_NewMailPlaySound,
   ConfigField_NewMailSoundFile,
#if defined(OS_UNIX) || defined(__CYGWIN__)
   ConfigField_NewMailSoundProgram,
#endif // OS_UNIX
   ConfigField_NewMailNotify,
   ConfigField_NewMailNotifyThresholdHelp,
   ConfigField_NewMailNotifyDetailsThreshold,
   ConfigField_NewMailNewOnlyIfUnseenHelp,
   ConfigField_NewMailNewOnlyIfUnseen,
   ConfigField_NewMailUpdateHelp,
   ConfigField_NewMailUpdateInterval,
   ConfigField_NewMailMonitorIntervalDefault,
   ConfigField_NewMailMonitorHelp,
   ConfigField_NewMailMonitor,
   ConfigField_NewMailMonitorInterval,
   ConfigField_NewMailMonitorOpenedOnly,
   ConfigField_NewMailPingOnStartupHelp,
   ConfigField_NewMailPingOnStartup,
   ConfigField_NewMailLast = ConfigField_NewMailPingOnStartup,

   // compose
   ConfigField_ComposeFirst = ConfigField_NewMailLast,
   ConfigField_UseOutgoingFolder,
   ConfigField_OutgoingFolder,
   ConfigField_WrapMargin,
//   ConfigField_WrapAuto,
   ConfigField_ReplyDefKindHelp,
   ConfigField_ReplyDefKind,
   ConfigField_ReplyString,
   ConfigField_ForwardString,
   ConfigField_ReplyCollapse,
   ConfigField_ReplyQuoteOrig,
   ConfigField_ReplyQuoteSelection,
   ConfigField_ReplyCharacters,
   ConfigField_ReplyUseXAttr,
   ConfigField_ReplyUseSenderInitials,
   ConfigField_ReplyQuoteEmpty,
   ConfigField_ReplyDetectSig,
#if wxUSE_REGEX
   ConfigField_ReplySigSeparatorHelp,
   ConfigField_ReplySigSeparator,
#endif // wxUSE_REGEX

   ConfigField_Signature,
   ConfigField_SignatureFile,
   ConfigField_SignatureSeparator,
   ConfigField_XFaceFile,
   // not very useful ConfigField_AdbSubstring,

   ConfigField_ComposerAppearanceHelp,
#ifdef USE_FONT_DESC
   ConfigField_ComposeViewFont,
#else // !USE_FONT_DESC
   ConfigField_ComposeViewFontFamily,
   ConfigField_ComposeViewFontSize,
#endif // USE_FONT_DESC/!USE_FONT_DESC
   ConfigField_ComposeViewFGColour,
   ConfigField_ComposeViewBGColour,
   ConfigField_ComposeViewColourHeaders,

   ConfigField_ComposePreviewHelp,
   ConfigField_ComposePreview,
   ConfigField_ComposeConfirm,

   ConfigField_ComposeSpacer,
   ConfigField_ComposeShowFrom,

   ConfigField_ComposeHeaders,
   ConfigField_ComposeTemplates,

   ConfigField_ComposeLast = ConfigField_ComposeTemplates,

   // folders
   ConfigField_FoldersFirst = ConfigField_ComposeLast,
   ConfigField_ReopenLastFolder_HelpText,
   ConfigField_DontOpenAtStartup,
   ConfigField_ReopenLastFolder,
   ConfigField_OpenFolders,
   ConfigField_MainFolder,
   ConfigField_FolderProgressHelpText,
   ConfigField_FolderProgressThreshold,
   ConfigField_ShowBusyInfo,
   ConfigField_CloseDelay_HelpText,
   ConfigField_FolderCloseDelay,
   ConfigField_ConnCloseDelay,
   ConfigField_OutboxHelp,
   ConfigField_UseOutbox,
   ConfigField_OutboxName,
   ConfigField_TrashHelp,
   ConfigField_UseTrash,
   ConfigField_TrashName,
   ConfigField_DraftsName,
   ConfigField_DraftsAutoDelete,
   ConfigField_FoldersFileFormat,
   ConfigField_CreateInternalMessage,
   ConfigField_StatusFormatHelp,
   ConfigField_StatusFormat_StatusBar,
   ConfigField_StatusFormat_TitleBar,
   ConfigField_FoldersLast = ConfigField_StatusFormat_TitleBar,

#ifdef USE_PYTHON
   // python
   ConfigField_PythonFirst = ConfigField_FoldersLast,
   ConfigField_Python_HelpText,
   ConfigField_EnablePython,
   ConfigField_PythonPath,
   ConfigField_StartupScript,
   ConfigField_CallbackFolderOpen,
   ConfigField_CallbackFolderUpdate,
   ConfigField_CallbackFolderExpunge,
   ConfigField_CallbackSetFlag,
   ConfigField_CallbackClearFlag,
   ConfigField_PythonLast = ConfigField_CallbackClearFlag,
#else  // !USE_PYTHON
   ConfigField_PythonLast = ConfigField_FoldersLast,
#endif // USE_PYTHON

   // message view
   ConfigField_MessageViewFirst = ConfigField_PythonLast,
   ConfigField_MsgViewerHelp,
   ConfigField_MsgViewer,
#ifdef USE_VIEWER_BAR
   ConfigField_MsgViewerBar,
#endif // USE_VIEWER_BAR
#ifdef USE_FONT_DESC
   ConfigField_MessageViewFont,
#else // !USE_FONT_DESC
   ConfigField_MessageViewFontFamily,
   ConfigField_MessageViewFontSize,
#endif // USE_FONT_DESC/!USE_FONT_DESC
   ConfigField_MessageViewFGColour,
   ConfigField_MessageViewBGColour,
   ConfigField_MessageViewHeaderNamesColour,
   ConfigField_MessageViewHeaderValuesColour,
   ConfigField_MessageViewHighlightUrls,
   ConfigField_MessageViewUrlColour,
   ConfigField_MessageViewQuotedColourize,
   ConfigField_MessageViewQuotedColourizeHelp,
   ConfigField_MessageViewQuotedColour1,
   ConfigField_MessageViewQuotedColour2,
   ConfigField_MessageViewQuotedColour3,
   ConfigField_MessageViewQuotedCycleColours,
   ConfigField_MessageViewHighlightSig,
   ConfigField_MessageViewSigColour,
   ConfigField_MessageViewProgressHelp,
   ConfigField_MessageViewProgressThresholdSize,
   ConfigField_MessageViewProgressThresholdTime,
   ConfigField_MessageViewMaxHelp,
   ConfigField_MessageViewMaxMsgSize,
   ConfigField_MessageViewInlineGraphicsHelp,
   ConfigField_MessageViewInlineGraphics,
   ConfigField_MessageViewInlineGraphicsSize,
   ConfigField_MessageViewInlineGraphicsExternal,
   ConfigField_MessageViewPlainIsText,
   ConfigField_MessageViewRfc822IsText,
   ConfigField_ViewWrapMargin,
   ConfigField_ViewWrapAuto,
#ifdef OS_UNIX
   ConfigField_MessageViewFaxHelp,
   ConfigField_MessageViewFaxSupport,
   ConfigField_MessageViewFaxDomains,
   ConfigField_MessageViewFaxConverter,
#endif // Unix
   ConfigField_MessageViewHeaders,
   ConfigField_MessageViewDateFormat,
   ConfigField_MessageViewTitleBarFormat,
   ConfigField_MessageViewLast = ConfigField_MessageViewTitleBarFormat,

   // folder view options
   ConfigField_FolderViewFirst = ConfigField_MessageViewLast,
   ConfigField_FolderViewShowHelpText,
   ConfigField_FolderViewShowLastSel,
   ConfigField_FolderViewShowInitially,
   ConfigField_FolderViewPreviewHelp,
   ConfigField_FolderViewPreviewOnSelect,
   ConfigField_FolderViewSelectInitially,

   ConfigField_FolderViewAutoNextHelp,
   ConfigField_FolderViewAutoNextMsg,
   ConfigField_FolderViewAutoNextFolder,

   ConfigField_FolderViewHelpLayout,
   ConfigField_FolderViewSplitVertically,
   ConfigField_FolderViewFoldersOnTop,

   ConfigField_FolderViewHelpText2,
   ConfigField_FolderViewOnlyNames,
   ConfigField_FolderViewReplaceFrom,
#ifdef USE_FONT_DESC
   ConfigField_FolderViewFont,
#else // !USE_FONT_DESC
   ConfigField_FolderViewFontFamily,
   ConfigField_FolderViewFontSize,
#endif // USE_FONT_DESC/!USE_FONT_DESC
   ConfigField_FolderViewFGColour,
   ConfigField_FolderViewBGColour,
   ConfigField_FolderViewNewColour,
   ConfigField_FolderViewRecentColour,
   ConfigField_FolderViewUnreadColour,
   ConfigField_FolderViewDeletedColour,
   ConfigField_FolderViewFlaggedColour,

   ConfigField_FolderViewSortThreadHelp,
   ConfigField_FolderViewThread,
   ConfigField_FolderViewThreadMessages,
   ConfigField_FolderViewSortMessagesBy,

   ConfigField_FolderViewHeaders, // "Configure columns to show..."
   ConfigField_FolderViewSizeUnits,
   ConfigField_FolderViewPreviewDelayHelp,
   ConfigField_FolderViewPreviewDelay,
   ConfigField_FolderViewStatusHelp,
   ConfigField_FolderViewUpdateStatus,
   ConfigField_FolderViewStatusBarFormat,
   ConfigField_FolderViewLast = ConfigField_FolderViewStatusBarFormat,

   // folder tree options
   ConfigField_FolderTreeFirst = ConfigField_FolderViewLast,
   ConfigField_FolderTreeIsOnLeft,
   ConfigField_FolderTreeColourHelp,
   ConfigField_FolderTreeFgColour,
   ConfigField_FolderTreeBgColour,
   ConfigField_FolderTreeShowOpened,
   ConfigField_FolderTreeFormatHelp,
   ConfigField_FolderTreeFormat,
   ConfigField_FolderTreePropagateHelp,
   ConfigField_FolderTreePropagate,
   ConfigField_FolderTreeNeverUnreadHelp,
   ConfigField_FolderTreeNeverUnread,
   ConfigField_FolderTreeOpenOnClick,
   ConfigField_FolderTreeShowHiddenFolders,
   ConfigField_FolderTreeHomeHelp,
   ConfigField_FolderTreeHomeFolder,
   ConfigField_FolderTreeIsHome,
   ConfigField_FolderTreeLast = ConfigField_FolderTreeIsHome,

   // autocollecting and address books options
   ConfigField_AdbFirst = ConfigField_FolderTreeLast,
   ConfigField_OwnAddressesHelp,
   ConfigField_OwnAddresses,
   ConfigField_MLAddressesHelp,
   ConfigField_MLAddresses,
   ConfigField_AutoCollect_HelpText,
   ConfigField_AutoCollect,
   ConfigField_AutoCollectAdb,
   ConfigField_AutoCollectSenderOnly,
   ConfigField_AutoCollectNameless,
   ConfigField_WhiteList,
#ifdef USE_BBDB
   ConfigField_Bbdb_HelpText,
   ConfigField_Bbdb_IgnoreAnonymous,
   ConfigField_Bbdb_GenerateUnique,
   ConfigField_Bbdb_AnonymousName,
   ConfigField_Bbdb_SaveOnExit,
   ConfigField_AdbLast = ConfigField_Bbdb_SaveOnExit,
#else // !USE_BBDB
   ConfigField_AdbLast = ConfigField_WhiteList,
#endif // USE_BBDB/!USE_BBDB

   // helper programs
   ConfigField_HelpersFirst = ConfigField_AdbLast,
   ConfigField_HelpersHelp1,
   ConfigField_Browser,
#ifndef OS_WIN    // we don't care about browser kind under Windows
   ConfigField_BrowserIsNetscape,
#endif // !Win
   ConfigField_BrowserInNewWindow,

#ifdef OS_UNIX
   ConfigField_HelpersHelp2,
   ConfigField_HelpBrowserKind,
   ConfigField_HelpBrowser,
   ConfigField_HelpBrowserIsNetscape,
#endif // OS_UNIX

#ifdef OS_UNIX
   ConfigField_ImageConverter,
   ConfigField_ConvertGraphicsFormat,
#endif // OS_UNIX

   ConfigField_HelpExternalEditor,
   ConfigField_ExternalEditor,
   ConfigField_AutoLaunchExtEditor,
#ifdef USE_OPENSSL
   ConfigField_SslHelp,
   ConfigField_SslDllName,
   ConfigField_CryptoDllName,
#endif // USE_OPENSSL
   ConfigField_PGPHelp,
   ConfigField_PGPCommand,
   ConfigField_PGPKeyServer,
   ConfigField_PGPGetPubKey,

   ConfigField_HelpersLast = ConfigField_PGPGetPubKey,

#ifdef USE_TEST_PAGE
   ConfigField_TestFirst = ConfigField_HelpersLast,
   ConfigField_TestMessage,
   ConfigField_TestFolder,
   ConfigField_TestLast = ConfigField_TestFolder,

   ConfigField_OthersFirst = ConfigField_TestLast,
#else // !USE_TEST_PAGE
   ConfigField_OthersFirst = ConfigField_HelpersLast,
#endif // USE_TEST_PAGE/!USE_TEST_PAGE

   // other options
   ConfigField_LogHelp,
   ConfigField_ShowLog,
   ConfigField_LogToFile,
   ConfigField_MailLog,
   ConfigField_ShowTips,
   ConfigField_Splash,
   ConfigField_SplashDelay,
   ConfigField_ShowTbarImages,
   ConfigField_EnvVarsHelp,
   ConfigField_EnvVars,
   ConfigField_AutosaveHelp,
   ConfigField_AutosaveDelay,
   ConfigField_ConfirmExit,
   ConfigField_RunOneOnly,
   ConfigField_HelpDir,
#ifdef OS_UNIX
   ConfigField_AFMPath,
#endif // OS_UNIX
#if defined(OS_UNIX) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   ConfigField_FocusFollowsMouse,
#endif
#ifdef OS_UNIX
   ConfigField_DockableMenubars,
   ConfigField_DockableToolbars,
   ConfigField_ToolbarsFlatButtons,
#endif // OS_UNIX
   ConfigField_ReenableDialog,
   ConfigField_AwayHelp,
   ConfigField_AwayAutoEnter,
   ConfigField_AwayAutoExit,
   ConfigField_AwayRemember,
   ConfigField_OthersLast = ConfigField_AwayRemember,

   ConfigField_SyncFirst = ConfigField_OthersLast,
   ConfigField_RemoteSynchroniseMessage,
   ConfigField_RSynchronise,
   ConfigField_RSConfigFolder,
   ConfigField_RSFilters,
   ConfigField_RSIds,
   ConfigField_RSFolders,
   ConfigField_RSFolderGroup,
   ConfigField_SyncStore,
   ConfigField_SyncRetrieve,
#ifdef OS_WIN
   ConfigField_SyncSeparator,
   ConfigField_SyncConfigFileHelp,
   ConfigField_SyncConfigFile,
   ConfigField_SyncLast = ConfigField_SyncConfigFile,
#else // !OS_WIN
   ConfigField_SyncLast = ConfigField_SyncRetrieve,
#endif // OS_WIN/!OS_WIN

   // the end
   ConfigField_Max
};

// -----------------------------------------------------------------------------
// our notebook class
// -----------------------------------------------------------------------------

// notebook for the options
class wxOptionsNotebook : public wxNotebookWithImages
{
public:
   // icon names
   static const wxChar *ms_aszImages[];

   wxOptionsNotebook(wxWindow *parent);

   // the profile we use - just the global one here
   Profile *GetProfile() const { return Profile::CreateProfile(_T("")); }

private:
   DECLARE_NO_COPY_CLASS(wxOptionsNotebook)
};

// notebook for the given options page
class wxCustomOptionsNotebook : public wxNotebookWithImages
{
public:
   wxCustomOptionsNotebook(wxWindow *parent,
                           size_t nPages,
                           const wxOptionsPageDesc *pageDesc,
                           const wxString& configForNotebook,
                           Profile *profile);

   virtual ~wxCustomOptionsNotebook() { delete [] m_aImages; }

private:
   // this method creates and fills m_aImages and returns it
   const wxChar **GetImagesArray(size_t nPages, const wxOptionsPageDesc *pageDesc);

   // the images names and NULL
   const wxChar **m_aImages;

   DECLARE_NO_COPY_CLASS(wxCustomOptionsNotebook)
};

// -----------------------------------------------------------------------------
// dialog classes
// -----------------------------------------------------------------------------

class wxGlobalOptionsDialog : public wxOptionsEditDialog
{
public:
   wxGlobalOptionsDialog(wxFrame *parent, const wxString& configKey = _T("OptionsDlg"));

   virtual ~wxGlobalOptionsDialog();

   // override base class functions
   virtual void CreateNotebook(wxPanel *panel);
   virtual bool TransferDataToWindow();

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxGlobalOptionsDialog() { wxFAIL_MSG(_T("should be never used")); }

   // return TRUE if this dialog edits global options for the program, FALSE
   // if this is another kind of dialog
   virtual bool IsGlobalOptionsDialog() const { return TRUE; }

protected:
   // implement base class pure virtual
   virtual Profile *GetProfile() const
   {
      return ((wxOptionsNotebook *)m_notebook)->GetProfile();
   }

private:
   DECLARE_DYNAMIC_CLASS_NO_COPY(wxGlobalOptionsDialog)
};

// just like wxGlobalOptionsDialog but uses the given wxOptionsPage and not the
// standard ones
class wxCustomOptionsDialog : public wxGlobalOptionsDialog
{
public:
   // minimal ctor, use SetPagesDesc() and SetProfile() later
   wxCustomOptionsDialog(wxFrame *parent,
                         const wxString& configForDialog = _T("CustomOptions"),
                         const wxString& configForNotebook = _T("CustomNotebook"))
      : wxGlobalOptionsDialog(parent, configForDialog),
        m_configForNotebook(configForNotebook)
   {
      SetProfile(NULL);
      SetPagesDesc(0, NULL);
   }

   // full ctor specifying everything we need
   wxCustomOptionsDialog(size_t nPages,
                         const wxOptionsPageDesc *pageDesc,
                         Profile *profile,
                         wxFrame *parent,
                         const wxString& configForDialog = _T("CustomOptions"),
                         const wxString& configForNotebook = _T("CustomNotebook"))
      : wxGlobalOptionsDialog(parent, configForDialog),
        m_configForNotebook(configForNotebook)
   {
      m_profile = profile;
      SafeIncRef(m_profile);

      SetPagesDesc(nPages, pageDesc);
   }

   // delayed initializetion: use these methods for an object constructed with
   // the first ctor
   void SetPagesDesc(size_t nPages, const wxOptionsPageDesc *pageDesc)
   {
      m_nPages = nPages;
      m_pageDesc = pageDesc;
   }

   void SetProfile(Profile *profile)
   {
      m_profile = profile;
      SafeIncRef(m_profile);
   }

   // dtor
   virtual ~wxCustomOptionsDialog()
   {
      SafeDecRef(m_profile);
   }

   // overloaded base class virtual
   virtual void CreateNotebook(wxPanel *panel)
   {
      m_notebook = new wxCustomOptionsNotebook(panel,
                                               m_nPages,
                                               m_pageDesc,
                                               m_configForNotebook,
                                               m_profile);
   }

private:
   // the number and descriptions of the pages we show
   size_t m_nPages;
   const wxOptionsPageDesc *m_pageDesc;

   // the profile
   Profile *m_profile;

   // the config key where notebook will remember its last page
   wxString m_configForNotebook;

   DECLARE_NO_COPY_CLASS(wxCustomOptionsDialog)
};

// an identity edit dialog: works with settings in an identity profile, same as
// wxGlobalOptionsDialog otherwise
class wxIdentityOptionsDialog : public wxCustomOptionsDialog
{
public:
   wxIdentityOptionsDialog(const wxString& identity, wxFrame *parent)
      : wxCustomOptionsDialog(parent, _T("IdentDlg"), _T("IdentNotebook")),
        m_identity(identity)
   {
      // use the identity profile
      Profile *profile = Profile::CreateIdentity(identity);
      SetProfile(profile);
      profile->DecRef();   // SetProfile() will hold on it

      // set the pages descriptions: we use the standard pages of the options
      // dialog, but not all of them
      CreatePagesDesc();
      SetPagesDesc(m_nPages, m_aPages);

      SetTitle(wxString::Format(_("Settings for identity '%s'"), m_identity.c_str()));
   }

   virtual ~wxIdentityOptionsDialog()
   {
      delete [] m_aPages;
   }

   // editing identities shouldn't give warning about "important programs
   // settings were changed" as the only really important ones are the global
   // ones
   virtual void SetDoTest() { SetDirty(); } // TODO: might do something here
   virtual void SetGiveRestartWarning() { }

   // we're not the global options dialog
   virtual bool IsGlobalOptionsDialog() const { return FALSE; }

private:
   // create our pages desc: do it dynamically because they may depend on the
   // user level (which can change) in the future
   void CreatePagesDesc();

   // the identity which we edit
   wxString m_identity;

   // the array of descriptions of our pages
   size_t m_nPages;
   wxOptionsPageDesc *m_aPages;

   DECLARE_NO_COPY_CLASS(wxIdentityOptionsDialog)
};

// another dialog (not for options this one) which allows to restore the
// previously changed settings
class wxRestoreDefaultsDialog : public wxProfileSettingsEditDialog
{
public:
   wxRestoreDefaultsDialog(Profile *profile, wxFrame *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();

private:
   wxCheckListBox *m_checklistBox;

   DECLARE_NO_COPY_CLASS(wxRestoreDefaultsDialog)
};

// ----------------------------------------------------------------------------
// event tables and such
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGlobalOptionsDialog, wxOptionsEditDialog)

BEGIN_EVENT_TABLE(wxOptionsPage, wxNotebookPageBase)
   // any change should make us dirty
   EVT_CHECKBOX(-1, wxOptionsPage::OnControlChange)
   EVT_RADIOBOX(-1, wxOptionsPage::OnControlChange)
   EVT_CHOICE(-1, wxOptionsPage::OnControlChange)
   EVT_TEXT(-1, wxOptionsPage::OnTextChange)

   // listbox events handling
   EVT_BUTTON(-1, wxOptionsPage::OnListBoxButton)

   EVT_UPDATE_UI(wxOptionsPage_BtnModify, wxOptionsPage::OnUpdateUIListboxBtns)
   EVT_UPDATE_UI(wxOptionsPage_BtnDelete, wxOptionsPage::OnUpdateUIListboxBtns)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageIdent, wxOptionsPage)
   // buttons invoke subdialogs
   EVT_BUTTON(-1, wxOptionsPageIdent::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageNewMail, wxOptionsPage)
   EVT_BUTTON(-1, wxOptionsPageNewMail::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageCompose, wxOptionsPage)
   // buttons invoke subdialogs
   EVT_BUTTON(-1, wxOptionsPageCompose::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageMessageView, wxOptionsPage)
   // buttons invoke subdialogs
   EVT_BUTTON(-1, wxOptionsPageMessageView::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageFolderView, wxOptionsPage)
   // buttons invoke subdialogs
   EVT_BUTTON(-1, wxOptionsPageFolderView::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageFolders, wxOptionsPage)
   EVT_BUTTON(wxOptionsPage_BtnNew, wxOptionsPageFolders::OnAddFolder)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageSync, wxOptionsPage)
   EVT_BUTTON(-1, wxOptionsPageSync::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageOthers, wxOptionsPage)
   EVT_BUTTON(-1, wxOptionsPageOthers::OnButton)
END_EVENT_TABLE()

// ============================================================================
// data: both of these arrays *must* be in sync with ConfigFields enum!
// ============================================================================

// The labels of all fields, their types and also the field they "depend on"
// (being dependent on another field only means that if that field is disabled
//  or unchecked, we're disabled too)
//
// Special case: if the index of the field we depend on is negative, the
// update logic is inverted: this field will be enabled only if the other one
// is disabled/unchecked. Note that it is impossible to have the field to
// inversely depend on the field with index 1 - but this is probably ok.
//
// If you modify this array, search for DONT_FORGET_TO_MODIFY and modify data
// there too
const wxOptionsPage::FieldInfo wxOptionsPageStandard::ms_aFields[] =
{
   // general config and identity
   { gettext_noop("Your personal name and organization are used only for\n"
                  "informational purposes and so you may give them any\n"
                  "values at all or even leave them empty, while the\n"
                  "user name is used as the default login for the servers\n"
                  "which require authentification and should be set to\n"
                  "the value they expect."),       Field_Message, -1 },
   { gettext_noop("&Personal name"),               Field_Text,    -1,                        },
   { gettext_noop("&Organization"),                Field_Text,    -1,                        },
   { gettext_noop("&User name or login"),          Field_Text,    -1,                        },
   { gettext_noop("The e-mail address is the value of the \"From:\" header\n"
                  "while the reply address is used for the \"Reply-To:\" one.\n"
                  "If you don't know whether you should set it, leave the\n"
                  "reply address empty."),         Field_Message | Field_Advanced, -1 },
   { gettext_noop("&E-mail address"),              Field_Text | Field_Vital,   -1, },
   { gettext_noop("&Reply address"),               Field_Text | Field_Advanced,   -1, },
   { gettext_noop("The following domain name can be used as a default "
                  "suffix for local mail addresses."),
                                                   Field_Message, -1 },
   { gettext_noop("&Add this suffix if none specified"), Field_Bool, -1 },
   { gettext_noop("&Domain"),                    Field_Text | Field_Vital,   ConfigField_AddDefaultHostname, },
   { gettext_noop("Reply return address from &To: field"), Field_Bool | Field_Advanced, -1, },
   { gettext_noop(
      "You may want to attach your personal information card (vCard)\n"
      "to all outoing messages. In this case you will need to specify\n"
      "a file containing it.\n"
      "Notice that such file can be created by exporting an address\n"
      "book entry in the vCard format."), Field_Message, -1 },
   { gettext_noop("Attach a v&Card to outgoing messages"), Field_Bool,    -1,                        },
   { gettext_noop("&vCard file"),                  Field_File, ConfigField_UseVCard,                        },
   { gettext_noop("Set &global password"),  Field_SubDlg | Field_Global, -1,},

   // network
   { gettext_noop("The following fields are used as default values for the\n"
                  "corresponding server names. You may set them independently\n"
                  "for each folder as well, overriding the values specified "
                  "here"),                         Field_Message | Field_AppWide, -1,                        },
   { gettext_noop("&POP server"),                  Field_Text | Field_AppWide,    -1,                        },
   { gettext_noop("&IMAP server"),                 Field_Text | Field_AppWide,    -1,                        },
#ifdef USE_SENDMAIL
   { gettext_noop("Use local mail transfer a&gent"), Field_Bool, -1,           },
   { gettext_noop("Local MTA &command"), Field_Text, ConfigField_UseSendmail },
#endif // USE_SENDMAIL
   { gettext_noop("SMTP (&mail) server"),          Field_Text | Field_Vital,
#ifdef USE_SENDMAIL
                                                   -ConfigField_UseSendmail,
#else
                                                   -1
#endif
   },
   { gettext_noop("NNTP (&news) server"),          Field_Text,    -1, },

   { gettext_noop("\n"
                  "If your SMTP server supports 8BITMIME ESMTP extension\n"
                  "Mahogany may send 8 bit data without encoding it. If\n"
                  "the server doesn't support it, the data will be encoded\n"
                  "properly, so there is normally no risk in setting this "
                  "option."),                      Field_Message |
                                                   Field_Advanced,
#ifdef USE_SENDMAIL
                                                  -ConfigField_UseSendmail
#else
                                                  -1
#endif
   },
   { gettext_noop("Send &8 bit data"),             Field_Bool |
                                                   Field_Advanced,
#ifdef USE_SENDMAIL
                                                  -ConfigField_UseSendmail
#else
                                                  -1
#endif
   },

   { gettext_noop("\n"
                  "Some SMTP or NNTP servers require a user name/login.\n"
                  "You might need to enter the e-mail address provided by\n"
                  "your ISP either with or without the part after '@':\n"
                  "please follow the ISP instructions about this."),
                                                   Field_Message, -1,                        },
   { gettext_noop("SMTP server &user ID"),         Field_Text,
#ifdef USE_SENDMAIL
                                                   -ConfigField_UseSendmail,
#else
                                                   -1
#endif
   },
   { gettext_noop("SMTP server pa&ssword"),        Field_Passwd, ConfigField_MailServerLogin,           },
   { gettext_noop("NNTP server user &ID"),         Field_Text,   -1,           },
   { gettext_noop("NNTP server pass&word"),        Field_Passwd, ConfigField_NewsServerLogin,           },

   { gettext_noop("\n"
                  "If you use SMTP authentification, you may also\n"
                  "have to specify a \"Sender:\" header.\n"
                  "Mahogany tries to guess it, but if it doesn't work\n"
                  "you can try to override it."),  Field_Message, ConfigField_MailServerLogin },
   { gettext_noop("Try to guess SMTP sender header"), Field_Bool | Field_Advanced, ConfigField_MailServerLogin,           },
   { gettext_noop("SMTP sender header"), Field_Text | Field_Advanced, -ConfigField_GuessSender,           },

   { gettext_noop("\n"
                  "Final complication with SMTP authentification is that\n"
                  "some servers (notably qmail and Exim 3) advertise the\n"
                  "authentification mechanisms which they don't support.\n"
                  "To use such broken servers you need to explicitly disable\n"
                  "the incorrectly implemented methods by entering them below\n"
                  "(you may enter \"PLAIN\" or \"PLAIN;CRAM-MD5\", for example)"),
                                                   Field_Message |
                                                   Field_Advanced, ConfigField_MailServerLogin },
   { gettext_noop("Methods NOT to use"),           Field_Text |
                                                   Field_Advanced, ConfigField_MailServerLogin },
#ifdef USE_SSL
   { "\n\n"
     gettext_noop("\n"
                  "Mahogany can attempt to use either SSL or TLS to send\n"
                  "mail or news. TLS is used by default if available but\n"
                  "you may disable it if you experience problems because\n"
                  "of this.\n"
                  "You may also have to tell Mahogany to accept unsigned (or\n"
                  "self-signed) certificates if your organization uses them."),
                                                   Field_Message, -1 },
   { gettext_noop("SS&L/TLS for SMTP server"
                  ":Don't use SSL at all"
                  ":Use TLS if available"
                  ":Use TLS only"
                  ":Use SSL only"),                Field_Combo,
#ifdef USE_SENDMAIL
                                                  -ConfigField_UseSendmail,
#else
                                                  -1,
#endif
   },
   { gettext_noop("&Accept unsigned certificates for SMTP"), Field_Bool, ConfigField_SmtpServerSSL,     },
   { gettext_noop("SSL/TLS for NNTP s&erver"
                  ":Don't use SSL at all"
                  ":Use TLS if available"
                  ":Use TLS only"
                  ":Use SSL only"),                Field_Combo, -1 },
   { gettext_noop("A&ccept unsigned certificates for NNTP"), Field_Bool, ConfigField_NntpServerSSL,     },
#endif // USE_SSL

#ifdef USE_DIALUP
   { gettext_noop("\n"
                  "Mahogany has support for dial-up networks and can detect if the\n"
                  "network is up or not. It can also be used to connect and disconnect the\n"
                  "network. To aid in detecting the network status, you can specify a beacon\n"
                  "host which should only be reachable if the network is up, e.g. the WWW\n"
                  "server of your ISP. Leave it empty to use the SMTP server for this.")
     , Field_Message | Field_AppWide | Field_Global, -1 },
   { gettext_noop("&Dial-up network support"),    Field_Bool | Field_AppWide | Field_Global,    -1,                        },
   { gettext_noop("&Beacon host (e.g. www.yahoo.com)"), Field_Text | Field_AppWide | Field_Global,   ConfigField_DialUpSupport},
#ifdef OS_WIN
   { gettext_noop("&RAS connection to use"),   Field_Combo | Field_AppWide | Field_Global, ConfigField_DialUpSupport},
#elif defined(OS_UNIX)
   { gettext_noop("Command to &activate network"),   Field_Text | Field_AppWide | Field_Global, ConfigField_DialUpSupport},
   { gettext_noop("Command to &deactivate network"), Field_Text | Field_AppWide | Field_Global, ConfigField_DialUpSupport},
#endif // platform
#endif // USE_DIALUP

   { gettext_noop("\n"
                  "The following timeout values are used for TCP connections to\n"
                  "remote mail or news servers.\n"
                  "Try increasing them if your connection is slow."),
                                                   Field_Message | Field_Global | Field_Advanced, -1 },
   { gettext_noop("&Open timeout (in seconds)"),   Field_Number | Field_Global | Field_Advanced,    -1,                        },
#ifdef USE_TCP_TIMEOUTS
   { gettext_noop("&Read timeout"),                Field_Number | Field_Global | Field_Advanced,    -1,                        },
   { gettext_noop("&Write timeout"),               Field_Number | Field_Global | Field_Advanced,    -1,                        },
   { gettext_noop("&Close timeout"),               Field_Number | Field_Global | Field_Advanced,    -1,                        },
#endif // USE_TCP_TIMEOUTS
#ifdef USE_IMAP_PREFETCH
   { gettext_noop("When Mahogany needs to fetch a header from IMAP server\n"
                  "it may also get a couple of adjacent messages which\n"
                  "will speed up the access to them if they are needed later\n"
                  "but can slow down retrieving the individual messages a bit\n"
                  "\n"
                  "Set this option to 0 to let Mahogany decide."), Field_Message | Field_Global | Field_Advanced, -1 },
   { gettext_noop("&Pre-fetch this many msgs"),    Field_Number | Field_Global | Field_Advanced,    -1,                        },
#endif // USE_IMAP_PREFETCH
   { gettext_noop("If the RSH timeout below is greater than 0, Mahogany will\n"
                  "first try to connect to IMAP servers using rsh instead of\n"
                  "sending passwords in clear text. However, if the server\n"
                  "does not support rsh connections, enabling this option can\n"
                  "lead to unneeded delays."),     Field_Message | Field_Global | Field_Advanced,    -1,                        },
   { gettext_noop("&Rsh timeout"),                 Field_Number | Field_Global | Field_Advanced,    -1,                        },

   // new mail handling
   { gettext_noop("Please note that the details of new mail handling must be\n"
                  "set separately for each folder, you should select a folder\n"
                  "in the tree and choose the \"Properties\" command from the\n"
                  "\"Folder\" menu to do this."),  Field_Message |
                                                   Field_AppWide, -1 },

   { gettext_noop("Mahogany processes the new mail in several steps:\n"
                  "first it applies the filters, if any, to it. Then,\n"
                  "if any new messages are left, it may collect (or download\n"
                  "in case of remote server) them to the central new mail folder\n"
                  "Finally, if any new messages are still left in this folder,\n"
                  "Mahogany may notify you about them either by running an\n"
                  "external program or by performing an internal action.\n"
                  "\n"
                  "Please notice that if you select any of these options,\n"
                  "you probably want to check the \"Permanently monitor this folder\"\n"
                  "checkbox to detect new mail in it automatically - otherwise\n"
                  "you'll have to open it manually to do it."
                  "\n"),
                                                   Field_Message |
                                                   Field_NotApp, -1 },

   { gettext_noop("Configure &filters..."),        Field_SubDlg |
                                                   Field_NotApp, -1 },
   { gettext_noop("Treat as junk mail folder"),    Field_Bool, -1 },
   { gettext_noop("&Collect new mail from this folder"),
                                                   Field_Bool | Field_NotApp,
                                                   -1 },
   { gettext_noop("&To this folder"),              Field_Folder |
                                                   Field_Advanced |
                                                   Field_NotApp,
                                                   ConfigField_NewMailCollect },
   { gettext_noop("&Leave mail in this folder"),   Field_Bool |
                                                   Field_Inverse |
                                                   Field_NotApp,
                                                   ConfigField_NewMailCollect },

   { gettext_noop("When new mail message appears in this folder Mahogany\n"
                  "may execute an external command and/or show a message "
                  "about it."),                    Field_Message,    -1 },
   { gettext_noop("E&xecute new mail command"),    Field_Bool,    -1 },
   { gettext_noop("New mail &command"),            Field_File,
                                                   ConfigField_NewMailNotifyUseCommand },

   { gettext_noop("In addition to running an external program, Mahogany may\n"
                  "also play a sound when a new message arrives. Leave the\n"
                  "sound file empty to play the default sound."),
                                                   Field_Message,    -1 },
   { gettext_noop("Play a &sound on new mail"),    Field_Bool,    -1 },
   { gettext_noop("Sound &file"),                  Field_File,    ConfigField_NewMailPlaySound },
#if defined(OS_UNIX) || defined(__CYGWIN__)
   { gettext_noop("&Program to play the sound"),   Field_File,    ConfigField_NewMailPlaySound },
#endif // OS_UNIX

   { gettext_noop("Show new mail &notification"),  Field_Bool,    -1 },
   { gettext_noop("If there are not too many new messages, Mahogany will\n"
                  "show a detailed notification message with the subjects\n"
                  "and senders of all messages. Otherwise it will just\n"
                  "show the number of the new messages in the folder.\n"
                  "You may set the threshold to -1 to always see the details\n"
                  "or to 0 to always show just the number of messages."),
                                                   Field_Message,
                                                   ConfigField_NewMailNotify },
   { gettext_noop("&Threshold for detailed notification"), 
                                                   Field_Number,
                                                   ConfigField_NewMailNotify },

   { gettext_noop("Normally only unread messages are considered as \"new\" mail.\n"
                  "You can uncheck this checkbox to apply new mail processing\n"
                  "to all messages appearing in the folder, even already read ones."), Field_Message, -1 },
   { gettext_noop("New messages must be &unread"), Field_Bool, -1 },

   { gettext_noop("The first setting below specifies how often should Mahogany\n"
                  "update the currently opened folders while the second one\n"
                  "specifies the default polling interval for the folders which\n"
                  "are monitored permanently (even when they are not opened).\n"
                  "You can configure a folder to be monitored in its properties dialog."),
                                                   Field_Message |
                                                   Field_AppWide, -1 },
   { gettext_noop("&Ping folder interval in seconds"), Field_Number |
                                                       Field_AppWide, -1 },
   { gettext_noop("Pol&ling interval in seconds"), Field_Number |
                                                   Field_AppWide, -1 },

   { gettext_noop("If you check the checkbox below, Mahogany will always monitor\n"
                  "this folder in the background even when it is not opened.\n"
                  "This allows to detect new mail in the folder without opening it.\n"
                  "\n"
                  "Please note that monitoring many folders may slow down the program!"),
                                                   Field_Message |
                                                   Field_NotApp, -1 },
   { gettext_noop("Permanently &monitor this folder"), Field_Bool | Field_NotApp, -1},
   { gettext_noop("Polling &interval in seconds"), Field_Number | Field_NotApp,
                                                   ConfigField_NewMailMonitor },
   { gettext_noop("Poll only if &opened"),         Field_Bool | Field_NotApp,
                                                   ConfigField_NewMailMonitor },
   { gettext_noop("Checking the checkbox below will result in checking the folder\n"
                  "for the new mail immediately after starting up instead of\n"
                  "waiting until the interval above expires."),
                                                   Field_Message | Field_NotApp,
                                                   ConfigField_NewMailMonitor },
   { gettext_noop("Poll folder at s&tartup"),      Field_Bool | Field_NotApp,
                                                   ConfigField_NewMailMonitor },

   // compose
   { gettext_noop("Sa&ve sent messages"),          Field_Bool,    -1,                        },
   { gettext_noop("&Folder for sent messages"),
                                                   Field_Folder,    ConfigField_UseOutgoingFolder },
   { gettext_noop("&Wrap margin"),                 Field_Number | Field_Global,  -1,                        },
//   { gettext_noop("Wra&p lines automatically"),    Field_Bool | Field_Global,  -1,                        },
   { gettext_noop("There are several different reply commands in Mahogany:\n"
                  "reply to sender only replies to the person who sent the\n"
                  "message, reply to all - to all message recipients and\n"
                  "reply to list replies to the mailing list address only\n"
                  "(for this to work you need to configure the mailing list\n"
                  "addresses in the \"Addresses\" page).\n"
                  "What do you want the default reply command to do?"),
                                                   Field_Message |
                                                   Field_Advanced, -1},
   { gettext_noop("Default &reply kind:sender:all:list:newsgroup"),
                                                   Field_Combo |
                                                   Field_Advanced, -1},
   { gettext_noop("&Reply string in subject"),     Field_Text,    -1,                        },
   { gettext_noop("&Forward string in subject"),   Field_Text,    -1,                        },
   { gettext_noop("Co&llapse reply markers"
                  ":no:collapse:collapse & count"),Field_Combo,   -1,                        },
   { gettext_noop("Quote &original message in reply"), Field_Radio,   -1,                        },
   { gettext_noop("Quote &selected part only"),    Field_Bool, ConfigField_ReplyQuoteOrig,                        },
   { gettext_noop("Reply prefi&x"),                Field_Text, ConfigField_ReplyQuoteOrig,                        },
   { gettext_noop("Use &sender attribution"),      Field_Bool, ConfigField_ReplyCharacters },
   { gettext_noop("Prepend &sender initials"),     Field_Bool, ConfigField_ReplyCharacters,                        },
   { gettext_noop("&Quote empty lines too"),       Field_Bool |
                                                   Field_Advanced,    ConfigField_ReplyCharacters,                        },
   { gettext_noop("Detect and remove &signature when replying"),
                                                   Field_Bool | Field_Advanced, ConfigField_ReplyQuoteOrig, },
#if wxUSE_REGEX
   { gettext_noop("This regular expression is used to detect the beginning\n"
                  "of the signature of the message replied to."),
                                                   Field_Message | Field_Advanced, ConfigField_ReplyDetectSig  },
   { gettext_noop("Signature separator"),          Field_Text | Field_Advanced,    ConfigField_ReplyDetectSig, },
#endif // wxUSE_REGEX

   { gettext_noop("&Use signature"),               Field_Bool,    -1,                        },
   { gettext_noop("&Signature file"),              Field_File,    ConfigField_Signature      },
   { gettext_noop("Use signature se&parator"),     Field_Bool,    ConfigField_Signature      },

   { gettext_noop("Configure &XFace..."),          Field_XFace,   -1          },
   // not very useful { gettext_noop("Mail alias substring ex&pansion"), Field_Bool,    -1,                        },

   { gettext_noop("\n"
                  "The following settings control the appearance\n"
                  "of the composer window:"),      Field_Message, -1 },
#ifdef USE_FONT_DESC
   { gettext_noop("&Font to use"),                 Field_Font | Field_Global, -1 },
#else // !USE_FONT_DESC
   { gettext_noop("Font famil&y"
                  ":default:decorative:roman:script:swiss:modern:teletype"),
                                                   Field_Combo | Field_Global,   -1},
   { gettext_noop("Font si&ze"),                   Field_Number | Field_Global,  -1},
#endif // USE_FONT_DESC/!USE_FONT_DESC

   { gettext_noop("Foreground c&olour"),           Field_Color | Field_Global,   -1},
   { gettext_noop("Back&ground colour"),           Field_Color | Field_Global,   -1},
   { gettext_noop("Use these settings for &headers too"),
                                                   Field_Bool | Field_Global,   -1},

   { gettext_noop("\n"
                  "The settings below allow you to have a last look at the\n"
                  "message before sending it and/or change your mind about\n"
                  "sending it even after pressing the \"Send\" button."),
                                                   Field_Message |
                                                   Field_Advanced, -1 },
   { gettext_noop("P&review message before sending"), Field_Bool |
                                                   Field_Advanced, -1 },
   { gettext_noop("&Confirm sending mail"),        Field_Bool |
                                                   Field_Advanced,
                                                  -ConfigField_ComposePreview },

   { "",                                           Field_Message, -1},
   { gettext_noop("Show \"&From\" field"),         Field_Bool |
                                                   Field_Advanced,  -1},
   { gettext_noop("Configure &headers..."),        Field_SubDlg,  -1},
   { gettext_noop("Configure &templates..."),      Field_SubDlg,  -1},

   // folders
   { gettext_noop("You may choose to not open any folders at all on startup,\n"
                  "reopen all folders which were open when the program was closed\n"
                  "for the last time or explicitly specify the folders to reopen:"),
                  Field_Message | Field_AppWide, -1 },
   { gettext_noop("Don't open any folders at startup"), Field_Bool | Field_AppWide, -1, },
   { gettext_noop("Reopen last open folders"), Field_Bool | Field_AppWide,
                                               -ConfigField_DontOpenAtStartup, },
   { gettext_noop("Folders to open on &startup"),  Field_List |
                                                   Field_Restart |
                                                   Field_AppWide,
                                                   -ConfigField_ReopenLastFolder,           },
   { gettext_noop("Folder opened in &main frame"), Field_Folder |
                                                   Field_Restart |
                                                   Field_AppWide,
                                                   -ConfigField_ReopenLastFolder,                        },
   { gettext_noop("A progress dialog will be shown while retrieving more\n"
                  "than specified number of messages. Set it to 0 to never\n"
                  "show the progress dialog at all."), Field_Message, -1},
   { gettext_noop("&Threshold for displaying progress dialog"), Field_Number, -1},
   { gettext_noop("Show bus&y dialog while sorting/threading"), Field_Bool, -1},
   { gettext_noop("\n"
                  "Mahogany may keep the folder open after closing it\n"
                  "for some time to make reopening the same folder faster.\n"
                  "It also can close the folder but keep the connection to\n"
                  "the server alive for some time to make it faster to open\n"
                  "other folders on the same server.\n"
                  "\n"
                  "Note that the first option is ignored for POP3 folders and\n"
                  "the second one is only used for IMAP and NNTP servers."), Field_Message, -1 },
   { gettext_noop("&Keep folder open for (seconds)"), Field_Number, -1},
   { gettext_noop("Keep &connection alive for (seconds)"), Field_Number, -1},

   { gettext_noop("\nThe outgoing messages may be sent out immediately\n"
                  "or just stored in an \"Outbox\" and sent later. Choose\n"
                  "the sending mode here:"),       Field_Message |
                                                   Field_AppWide, -1 },
   { gettext_noop("Send outgoing messages later"), Field_Bool |
                                                   Field_Restart |
                                                   Field_AppWide, -1 },
   { gettext_noop("Folder for &outgoing messages"), Field_Folder |
                                                    Field_Restart |
                                                    Field_AppWide, ConfigField_UseOutbox },

   { gettext_noop("\n"
                  "When you delete a message in the folder it can be\n"
                  "either just marked as deleted (and you will need to\n"
                  "use the \"Expunge\" command later to physically\n"
                  "delete it) or moved to another folder, customarily\n"
                  "called \"Trash\".\n"
                  "Which operation mode would you like to use?"), Field_Message, -1 },
   { gettext_noop("Use &Trash folder"), Field_Bool, -1},
   { gettext_noop("&Trash folder name"), Field_Folder, ConfigField_UseTrash},
   { gettext_noop("&Drafts folder name"), Field_Folder, -1 },
   { gettext_noop("Delete drafts &automatically"), Field_Bool, -1 },
   { gettext_noop("Default format for mailbox files"
                  ":Unix mbx mailbox:Unix mailbox:MMDF (SCO Unix):Tenex (Unix MM format)"),
     Field_Combo | Field_AppWide | Field_Advanced, -1},
   { gettext_noop("Create \"folder internal data\" message"), Field_Bool, -1 },
   { gettext_noop("You can specify the format for the strings shown in the\n"
                  "status and title bars. Use %f for the folder name and\n"
                  "%t, %r and %n for the number of all, recent and new\n"
                  "messages respectively."), Field_Message | Field_Advanced, -1 },
   { gettext_noop("Status &bar format"), Field_Text | Field_Advanced, -1 },
   { gettext_noop("T&itle bar format"), Field_Text | Field_Advanced, -1 },

#ifdef USE_PYTHON
   // python
   { gettext_noop("Python is the built-in scripting language which can be\n")
     gettext_noop("used to extend Mahogany's functionality. It is not essential\n")
     gettext_noop("for the program's normal operation."), Field_Message, -1},
   { gettext_noop("&Enable Python"),               Field_Bool |
                                                   Field_AppWide, -1,                        },
   { gettext_noop("Python &path"),                 Field_Text |
                                                   Field_AppWide, ConfigField_EnablePython   },
   { gettext_noop("&Startup script"),              Field_File |
                                                   Field_AppWide, ConfigField_EnablePython   },
   { gettext_noop("&Folder open callback"),        Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Folder &update callback"),      Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Folder e&xpunge callback"),     Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Flag &set callback"),           Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Flag &clear callback"),         Field_Text,    ConfigField_EnablePython   },
#endif // USE_PYTHON

   // message view
   // FIXME: this text is stupid but we should have _some_ explanation, right?
   { gettext_noop("Mahogany has several standard message viewers and more\n"
                  "can be plugged in. The default one is the safest choice\n"
                  "but you may try the others for an alternative look."),
                                                   Field_Message |
                                                   Field_Advanced,    -1 },
   { gettext_noop("&Message viewer"),              Field_Combo |
                                                   Field_Advanced,    -1 },
#ifdef USE_VIEWER_BAR
   { gettext_noop("Show viewer &bar"),             Field_Bool,    -1 },
#endif // USE_VIEWER_BAR

#ifdef USE_FONT_DESC
   { gettext_noop("&Font to use"),                 Field_Font,    -1 },
#else // !USE_FONT_DESC
   { gettext_noop("&Font family"
                  ":default:decorative:roman:script:swiss:modern:teletype"),
                                                   Field_Combo,   -1 },
   { gettext_noop("Font si&ze"),                   Field_Number,  -1 },
#endif // USE_FONT_DESC/!USE_FONT_DESC
   { gettext_noop("Foreground c&olour"),           Field_Color,   -1 },
   { gettext_noop("Back&ground colour"),           Field_Color,   -1 },
   { gettext_noop("Colour for header &names"),     Field_Color,   -1 },
   { gettext_noop("Colour for header &values"),    Field_Color,   -1 },
   { gettext_noop("&Highlight the URLs"),          Field_Bool,    -1 },
   { gettext_noop("Colour for &URLs"),             Field_Color,   ConfigField_MessageViewHighlightUrls },
   { gettext_noop("Colourize &quoted text"),       Field_Bool,    -1 },
   { gettext_noop("You may specify the colours for the first 3 levels of\n"
                  "the quoted text individually. If quoting level is greater\n"
                  "than 3, Mahogany may either reuse the last colour for\n"
                  "the deeper levels or start over with the first colour\n"
                  "again if you check the option below."), Field_Message, ConfigField_MessageViewQuotedColourize },
   { gettext_noop("Colour for the &1st level"),Field_Color,   ConfigField_MessageViewQuotedColourize },
   { gettext_noop("Colour for the &2nd level"),Field_Color,   ConfigField_MessageViewQuotedColourize },
   { gettext_noop("Colour for the &3rd level"),Field_Color,   ConfigField_MessageViewQuotedColourize },
   { gettext_noop("Loop over these colours"),      Field_Bool,    ConfigField_MessageViewQuotedColourize },
   { gettext_noop("Highlight the si&gnature"),     Field_Bool,    -1 },
   { gettext_noop("Colour for s&ignatures"),       Field_Color, ConfigField_MessageViewHighlightSig },
   { gettext_noop("A progress dialog can be shown during the message download\n"
                  "if it is bigger than the given size or takes longer than the\n"
                  "specified time (use -1 to disable progress dialog entirely)"), Field_Message, -1 },
   { gettext_noop("Dialog minimal size t&hreshold (kb)"),             Field_Number,    -1 },
   { gettext_noop("Progress dialog &delay (seconds)"),             Field_Number,    -1 },
   { gettext_noop("The following setting allows to limit the amount of data\n"
                  "retrieved from remote server: if the message size\n"
                  "is greater than the value specified here, you\n"
                  "will be asked for confirmation before transfering data"
                  "(set to 0 to disable)"),
                                                   Field_Message,  -1 },
   { gettext_noop("Ask if size of &message (in Kb) >"), Field_Number,   -1 },
   { gettext_noop("Some of the Mahogany text viewers may show the images\n"
                  "included with the message inline, i.e. directly in the\n"
                  "viewer window.\n"
                  "If you enable this feature, you may specify below the\n"
                  "maximal size of an image to display inline (setting it\n"
                  "to 0 disables image inlining, setting it to -1 means to\n"
                  "always inline them) and also enable showing the images\n"
                  "embedded in HTML messages even if they are are not included\n"
                  "into the message itself (for the HTML viewer only).\n"
                  "Be warned that this could be a potential security risk!"), Field_Message,    -1 },
   { gettext_noop("Show images &inline"),             Field_Bool,    -1 },
   { gettext_noop("But only if size is less than (kb)"), Field_Number, ConfigField_MessageViewInlineGraphics },
   { gettext_noop("&Enable showing external images"), Field_Bool,    ConfigField_MessageViewInlineGraphics },
   { gettext_noop("Display &text attachments inline"),Field_Bool,    -1 },
   { gettext_noop("Display &mail messages as text"),Field_Bool,    -1 },
   { gettext_noop("&Wrap margin"),                 Field_Number,  -1,                        },
   { gettext_noop("Wra&p lines automatically"),    Field_Bool,  -1,                        },
#ifdef OS_UNIX
   { gettext_noop("Mahogany has built-in support for the faxes sent via eFax\n"
                  "or a similar service, use the options below to configure it."), Field_Message,    -1 },
   { gettext_noop("Support special &fax mailers"), Field_Bool,    -1 },
   { gettext_noop("&Domains sending faxes"),       Field_Text,    ConfigField_MessageViewFaxSupport},
   { gettext_noop("Conversion program for fa&xes"), Field_File,    ConfigField_MessageViewFaxSupport},
#endif // unix
   { gettext_noop("Configure &headers to show..."),Field_SubDlg,   -1 },
   { gettext_noop("Configure &format for displaying dates"),         Field_SubDlg,    -1                     },
   { gettext_noop("&Title of message view frame"),         Field_Text,    -1                     },

   // folder view
   { gettext_noop("What happens when the folder is opened? Mahogany may "
                  "either return\n"
                  "to the message which had been previously selected or "
                  "go to the specified one."),
                                                   Field_Message,  -1 },
   { gettext_noop("Return to the &last selected"), Field_Bool,  -1 },
   { gettext_noop("&Go to this message:First unread:First:Last"),
                                                   Field_Radio, -1 },

   { gettext_noop("\nWhen the message is selected Mahogany may preview "
                  "it immediately\n"
                  "or wait until you double click it or press <Return>.\n"
                  "In this mode it may be convenient to avoid automatically "
                  "previewing the\n"
                  "message initially selected when you open the folder "
                  "as well."),
                                                   Field_Message,    -1 },
   { gettext_noop("Preview message when &selected"), Field_Bool,  -1 },
   { gettext_noop("&Select the initial message"),    Field_Bool,  ConfigField_FolderViewPreviewOnSelect },

   { gettext_noop("\nWhat happens when you scroll down beyond "
                  "the end of the current message?\n"
                  "Mahogany may automatically select..."),
                                                   Field_Message, -1},
   { gettext_noop("    next unread &message"),     Field_Bool,    -1},
   { gettext_noop("    next unread &folder"),      Field_Bool,
                                                   ConfigField_FolderViewAutoNextMsg},
   { gettext_noop("\nThe settings below specify how should the two windows\n"
                  "(folder and message view) arranged inside the folder view."),
                                                   Field_Message, -1 },
   { gettext_noop("Split the view vertically"),    Field_Bool, -1 },
   { gettext_noop("Put folder view on top/to the left"), Field_Bool |
                                                         Field_Restart, -1 },
   { gettext_noop("\nThe following settings control appearance of the messages list:"), Field_Message,  -1 },
   { gettext_noop("Show only sender's name, not &e-mail"), Field_Bool,    -1 },
   { gettext_noop("Show \"&To\" for messages from oneself"), Field_Bool,    -1 },
#ifdef USE_FONT_DESC
   { gettext_noop("&Font to use"),                 Field_Font,    -1 },
#else // !USE_FONT_DESC
   { gettext_noop("Font famil&y"
                  ":default:decorative:roman:script:swiss:modern:teletype"),
                                                   Field_Combo,   -1},
   { gettext_noop("Font si&ze"),                   Field_Number,  -1},
#endif // USE_FONT_DESC/!USE_FONT_DESC
   { gettext_noop("Foreground c&olour"),           Field_Color,   -1},
   { gettext_noop("&Backgroud colour"),            Field_Color,   -1},
   { gettext_noop("Colour for &new message"),      Field_Color,   -1},
   { gettext_noop("Colour for &recent messages"),  Field_Color,   -1},
   { gettext_noop("Colour for u&nread messages"),  Field_Color,   -1},
   { gettext_noop("Colour for &deleted messages" ),Field_Color,   -1},
   { gettext_noop("Colour for &flagged messages" ),Field_Color,   -1},

   { gettext_noop("You may configure how (and if) Mahogany sorts and threads\n"
                  "messages in the folder using the buttons below. To access\n"
                  "these configuration dialogs faster, just right click on\n"
                  "any folder view header column."), Field_Message, -1 },
   { gettext_noop("&Thread messages"),             Field_Bool, -1 },
   { gettext_noop("Threading &options..."),        Field_SubDlg, ConfigField_FolderViewThread },
   { gettext_noop("&Sort messages by..."),         Field_SubDlg,  -1},

   { gettext_noop("Configure &columns to show..."),Field_SubDlg,   -1 },
   // combo choices must be in sync with MessageSizeShow enum values
   { gettext_noop("Show size in &units of:automatic:autobytes:bytes:kbytes:mbytes"),
                                                   Field_Combo,   -1 },
   { gettext_noop("Mahogany may delay previewing the selected message until\n"
                  "the timeout specified below expires. This allows to\n"
                  "quickly browse through the list of messages without\n"
                  "waiting for them to be previewed. Set it to 0 to disable\n"
                  "this feature."),                Field_Message | Field_Advanced, -1 },
   { gettext_noop("Delay in miliseconds"),        Field_Number | Field_Advanced, -1 },
   { gettext_noop("You can choose to show the information about\n"
                  "the currently selected message in the status bar.\n"
                  "You can use the same macros as in the template\n"
                  "dialog (i.e. $subject, $from, ...) in this string."),
                                                   Field_Message, -1 },
   { gettext_noop("&Use status bar"),              Field_Bool,    -1 },
   { gettext_noop("&Status bar line format"),      Field_Text,    ConfigField_FolderViewUpdateStatus },

   // folder tree
   { gettext_noop("Put folder tree on the left side"), Field_Bool | Field_AppWide, -1 },
   { gettext_noop("The two options below are common for all folders,\n"
                  "but note that the first of them may be overridden by setting\n"
                  "the normal folder colour in the \"Folder View\" page."), Field_Message | Field_AppWide, -1 },
   { gettext_noop("Tree &foreground colour"), Field_Color | Field_AppWide, -1 },
   { gettext_noop("Tree &background colour"), Field_Color | Field_AppWide, -1 },
   { gettext_noop("Show &opened folder specially"), Field_Bool | Field_AppWide, -1 },
   { gettext_noop("Mahogany can show the number of messages in the folder\n"
                  "directly in the folder tree. You may wish to disable\n"
                  "this feature to speed it up slightly by leaving the text\n"
                  "below empty or enter a string containing %t and/or %u\n"
                  "to be replaced with the total number of messages and the\n"
                  "number of unseen messages respectively."),
                  Field_Message, -1 },
   { gettext_noop("Folder tree &format"), Field_Text | Field_Restart, -1 },
   { gettext_noop("By default, if the folder has new/recent/unread messages\n"
                  "its parent is shown in the same state as well. Disable\n"
                  "it below if you don't like it (this makes sense mostly\n"
                  "for folders such as \"Trash\" or \"Sent Mail\")."),
                                                   Field_Message |
                                                   Field_NotApp, -1 },
   { gettext_noop("&Parent shows status"),         Field_Bool |
                                                   Field_NotApp, -1 },
   { gettext_noop("You may check the option below to skip this folder when\n"
                  "navigating in the folder tree using Ctrl-Up/Down arrows\n"
                  "which normally selects the next folder with unread messages.\n"
                  "But if you select it, this one will be skipped even it has\n"
                  "unread mail (again, this is mainly useful for \"Trash\"\n"
                  "folder, for example)."), Field_Message | Field_NotApp, -1 },
   { gettext_noop("&Skip this folder"), Field_Bool | Field_NotApp, -1 },
   { gettext_noop("Open folders on single &click"), Field_Bool | Field_AppWide | Field_Global, -1 },
   { gettext_noop("Show &hidden folders in the folder tree"), Field_Bool | Field_AppWide | Field_Global,-1 },
   { gettext_noop("When you press Ctrl-Home in the tree Mahogany will go to\n"
                  "the home folder (usually Inbox or NewMail) if any."), Field_Message, -1 },
   { gettext_noop("&Home folder"), Field_Folder | Field_AppWide, -1 },
   { gettext_noop("Is &home folder"), Field_Bool | Field_NotApp, -1 },

   // adb: autocollect and bbdb options
   { gettext_noop("The addresses listed below are the ones which are\n"
                  "recognized as being your own when showing the message\n"
                  "headers and also answering to the messages (they are\n"
                  "excluded from the list of your reply recipients)"),
                                                   Field_Message, -1 },
   { gettext_noop("&Addresses to replace with \"To\""),  Field_List, -1,           },
   { gettext_noop("Mahogany will usually extract the mailing list addresses\n"
                  "from messages sent to a list automatically but if this\n"
                  "doesn't work, you may manually specify the addresses of\n"
                  "the lists you are subscribed to below."),
                                                   Field_Message, -1 },
   { gettext_noop("&Mailing list addresses"),  Field_List, -1,           },
   { gettext_noop("Mahogany may automatically remember all e-mail addresses in the messages you\n"
                  "receive in a special address book. This is called 'address\n"
                  "autocollection' and may be turned on or off from this page."),
                                                   Field_Message, -1                     },
   { gettext_noop("&Autocollect addresses"),       Field_Radio,  -1,                    },
   { gettext_noop("Address &book to use"),         Field_Text, ConfigField_AutoCollect   },
   { gettext_noop("Autocollect only &senders' addresses"), Field_Bool, ConfigField_AutoCollect},
   { gettext_noop("Ignore addresses without &names"), Field_Bool, ConfigField_AutoCollect},
   { gettext_noop("&Whitelist"),                   Field_Text | Field_AppWide, -1 },
#ifdef USE_BBDB
   { gettext_noop("The following settings configure the support of the Big Brother\n"
                  "addressbook (BBDB) format. This is supported only for compatibility\n"
                  "with other software (emacs). The normal addressbook is unaffected by\n"
                  "these settings."), Field_Message, -1},
   { gettext_noop("&Ignore entries without names"), Field_Bool, -1 },
   { gettext_noop("&Generate unique aliases"),      Field_Bool, -1 },
   { gettext_noop("&Name for nameless entries"),    Field_Text, ConfigField_Bbdb_GenerateUnique },
   { gettext_noop("&Save on exit"),                 Field_Radio, -1 },
#endif // USE_BBDB

   // helper programs
   { gettext_noop("The following program will be used to open URLs embedded in messages:"),       Field_Message, -1                      },
   { gettext_noop("Open &URLs with"),             Field_File,    -1                      },
      // we don't care if it is Netscape or not under Windows
#ifndef OS_WIN
   { gettext_noop("URL &browser is Netscape"),    Field_Bool,    -1                      },
#endif // OS_UNIX
   { gettext_noop("Open browser in new &window"), Field_Bool,
      // under Unix we can only implement this with Netscape, under Windows it
      // also works with IE (and presumably others too)
#ifdef OS_WIN
                  -1
#else // Unix
                  ConfigField_BrowserIsNetscape
#endif // Win/Unix
   },
#ifdef OS_UNIX
   { gettext_noop("The following program will be used to view the online help system:"),     Field_Message, -1                      },
   { gettext_noop("&Help viewer"
                  ":internal"
                  ":external"),                Field_Combo,    -1                      },
   { gettext_noop("&External viewer"),                Field_File,    -1                      },
   { gettext_noop("Help &viewer is Netscape"),    Field_Bool,    -1                      },
#endif // OS_UNIX

#ifdef OS_UNIX
   { gettext_noop("&Image format converter"),     Field_File,    -1                      },
   { gettext_noop("Conversion &graphics format"
                  ":XPM:PNG:BMP:JPG:GIF:PCX:PNM"),    Field_Combo,   -1 },
#endif // OS_UNIX

   { gettext_noop("You may configure the external editor to be used when composing the messages\n"
                  "and optionally choose to launch it automatically."),
                                                  Field_Message, -1                      },
   { gettext_noop("&External editor"),            Field_File,    -1                      },
   { gettext_noop("Always &use it"),              Field_Bool, ConfigField_ExternalEditor },
#ifdef USE_OPENSSL
   { gettext_noop("\n"
                  "Mahogany can use SSL (Secure Sockets Layer) for secure,\n"
                  "encrypted communications, if you have the libssl and libcrypto\n"
                  "shared libraries (DLLs) on your system."),
     Field_Message, -1                     },
   { gettext_noop("Location of lib&ssl"),         Field_File,    -1                     },
   { gettext_noop("Location of libcr&ypto"),      Field_File,    -1                     },
#endif // USE_OPENSSL
   { gettext_noop("\n"
                  "GNU Privacy Guard or a compatible program may be used\n"
                  "to verify the cryptographic signatures of the messages\n"
                  "you receive and decrypt them."), Field_Message, -1 },
   { gettext_noop("&GPG command"),                Field_File,    -1                      },
   { gettext_noop("GPG &key server"),             Field_Text,    -1                      },
   { gettext_noop("Get GPG key &from server"),    Field_Bool,    -1                      },

   // test page
#ifdef USE_TEST_PAGE
   { _T("This is a test page used only for debugging.\n")
     _T("Normally you shouldn't see it at all."), Field_Message, -1 },
   { _T("Test folder entry"), Field_Folder, -1 },
#endif // USE_TEST_PAGE

   // other options
   { gettext_noop("Mahogany may log everything into the log window, a file\n"
                  "or both simultaneously. Additionally, you may enable mail\n"
                  "debugging option to get much more detailed messages about\n"
                  "mail folder accesses: although this slows down the program\n"
                  "a lot, it is very useful to diagnose the problems.\n\n"
                  "Please turn it on and join the log output to any bug\n"
                  "reports you send us (of course, everybody knows that there\n"
                  "are no bugs in Mahogany, but just in case :-)"),             Field_Message,    -1,                    },
   { gettext_noop("Show &log window"),             Field_Bool,    -1,                    },
   { gettext_noop("Log to &file"),                 Field_File | Field_FileSave,    -1,                    },
   { gettext_noop("Debug server and mailbox access"), Field_Bool, -1                     },
   { gettext_noop("Show &tips at startup"),        Field_Bool,    -1,                    },
   { gettext_noop("&Splash screen at startup"),    Field_Bool | Field_Restart, -1,                    },
   { gettext_noop("Splash screen &delay"),         Field_Number,  ConfigField_Splash     },
   { gettext_noop("Mahogany may expand environment variables in the\n"
                  "configuration entries which is useful but takes some\n"
                  "extra time, you may disable it here if you don't need it."),
                                                   Field_Message, -1, },
   { gettext_noop("E&xpand variables"),            Field_Bool, -1 },
   { gettext_noop("Show toolbar buttons and notebook tabs &as"
                  ":Images:Text:Both"),            Field_Radio | Field_Restart, -1     },
   { gettext_noop("If autosave delay is not 0, the program will periodically\n"
                  "save all unsaved information (settings, contents of the\n"
                  "opened compose windows, ...) which reduces the risk of loss\n"
                  "of it in a case of crash."),     Field_Message, -1                     },
   { gettext_noop("&Autosave delay"),              Field_Number, -1                      },
   { gettext_noop("Confirm &exit"),                Field_Bool | Field_Restart, -1                     },
   { gettext_noop("Always run only &one instance"),Field_Bool | Field_Restart, -1                     },
   { gettext_noop("Directory with the help files"), Field_Dir, -1 },

#ifdef OS_UNIX
   { gettext_noop("&Path where to find AFM files"), Field_Dir,    -1                     },
#endif // OS_UNIX
#if defined(OS_UNIX) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   { gettext_noop("&Focus follows mouse"), Field_Bool,    -1                     },
#endif
#ifdef OS_UNIX
   { gettext_noop("Use floating &menu-bars"), Field_Bool,    -1                     },
   { gettext_noop("Use floating &tool-bars"), Field_Bool,    -1                     },
   { gettext_noop("Tool-bars with f&lat buttons"), Field_Bool,    -1                     },
#endif // OS_UNIX
   { gettext_noop("&Reenable disabled message boxes..."), Field_SubDlg, -1 },
   { gettext_noop("\n"
                  "\"Away\", or unattended, state is a special mode in\n"
                  "which Mahogany tries to avoid any interaction with the user,\n"
                  "e.g. new mail notification is disabled, no progress dialogs\n"
                  "are shown &&c.\n"
                  "\n"
                  "This is useful if you want to not be distracted by new\n"
                  "mail arrival temporarily without having to shut down."), Field_Message, -1 },
   { gettext_noop("Enter awa&y mode when idle during (min):"), Field_Number, -1 },
   { gettext_noop("E&xit away mode automatically"), Field_Bool, -1 },
   { gettext_noop("Rememeber a&way status"), Field_Bool, -1 },

   // sync page
   { gettext_noop("Mahogany can synchronise part of its configuration\n"
                  "with settings stored in a special folder. This can\n"
                  "be used to share settings between machines by storing\n"
                  "them in a special IMAP folder on the server.\n"
                  "Please read the documentation first! It is accessible\n"
                  "via the Help button below."),
                  Field_Message, -1 },
   { gettext_noop("Sync options with remote server"), Field_Bool|Field_Global, -1 },
   { gettext_noop("Remote (IMAP) folder for synchronisation"), Field_Folder|Field_Global, ConfigField_RSynchronise },
   { gettext_noop("Sync filter rules"), Field_Bool|Field_Global, ConfigField_RSynchronise },
   { gettext_noop("Sync identities"), Field_Bool|Field_Global, ConfigField_RSynchronise },
   { gettext_noop("Sync part of the folder tree"), Field_Bool|Field_Global, ConfigField_RSynchronise },
   { gettext_noop("Folder group to synchronise"), Field_Folder|Field_Global, ConfigField_RSFolders },
   { gettext_noop("&Store settings..."), Field_SubDlg | Field_Global, ConfigField_RSynchronise },
   { gettext_noop("&Retrieve settings..."), Field_SubDlg | Field_Global, ConfigField_RSynchronise },
#ifdef OS_WIN
   { "\n\n",      Field_Message, -1 },
   { gettext_noop("If a non empty filename is specified here, Mahogany uses\n"
                  "this file instead of the registry for storing all of its\n"
                  "options. This may be useful if you want to share the same\n"
                  "file among several Windows machines (warning: don't try\n"
                  "to share it with Unix machines, this wouldn't work well)."),
                  Field_Message, -1 },
   { gettext_noop("&Config file"),           Field_File |
                                             Field_FileSave |
                                             Field_Global |
                                             Field_AppWide |
                                             Field_Restart, -1 },
#endif // OS_WIN
};

// FIXME ugly, ugly, ugly... config settings should be living in an array from
//       the beginning which would avoid us all these contorsions
#define CONFIG_ENTRY(name)  ConfigValueDefault(name##_NAME, name##_DEFVAL)
// worse: dummy entries for message fields
#define CONFIG_NONE()  ConfigValueNone()

// if you modify this array, search for DONT_FORGET_TO_MODIFY and modify data
// there too
const ConfigValueDefault wxOptionsPageStandard::ms_aConfigDefaults[] =
{
   // identity
   CONFIG_NONE(), // ident help
   CONFIG_ENTRY(MP_PERSONALNAME),
   CONFIG_ENTRY(MP_ORGANIZATION),
   CONFIG_ENTRY(MP_USERNAME),
   CONFIG_NONE(), // return address help
   CONFIG_ENTRY(MP_FROM_ADDRESS),
   CONFIG_ENTRY(MP_REPLY_ADDRESS),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_ADD_DEFAULT_HOSTNAME),
   CONFIG_ENTRY(MP_HOSTNAME),
   CONFIG_ENTRY(MP_SET_REPLY_FROM_TO),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_USEVCARD),
   CONFIG_ENTRY(MP_VCARD),
   CONFIG_NONE(),

   // network
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_POPHOST),
   CONFIG_ENTRY(MP_IMAPHOST),
#ifdef USE_SENDMAIL
   CONFIG_ENTRY(MP_USE_SENDMAIL),
   CONFIG_ENTRY(MP_SENDMAILCMD),
#endif // USE_SENDMAIL
   CONFIG_ENTRY(MP_SMTPHOST),
   CONFIG_ENTRY(MP_NNTPHOST),

   CONFIG_NONE(), // 8 bit help
   CONFIG_ENTRY(MP_SMTP_USE_8BIT),

   CONFIG_NONE(),
   CONFIG_ENTRY(MP_SMTPHOST_LOGIN),
   CONFIG_ENTRY(MP_SMTPHOST_PASSWORD),
   CONFIG_ENTRY(MP_NNTPHOST_LOGIN),
   CONFIG_ENTRY(MP_NNTPHOST_PASSWORD),
   CONFIG_NONE(),    // "Sender" help
   CONFIG_ENTRY(MP_GUESS_SENDER),
   CONFIG_ENTRY(MP_SENDER),
   CONFIG_NONE(),    // disabled auths help
   CONFIG_NONE(),    // almost CONFIG_ENTRY(MP_SMTP_DISABLED_AUTHS)
#ifdef USE_SSL
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_SMTPHOST_USE_SSL),
   CONFIG_ENTRY(MP_SMTPHOST_USE_SSL_UNSIGNED),
   CONFIG_ENTRY(MP_NNTPHOST_USE_SSL),
   CONFIG_ENTRY(MP_NNTPHOST_USE_SSL_UNSIGNED),
#endif // USE_SSL

#ifdef USE_DIALUP
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_DIALUP_SUPPORT),
   CONFIG_ENTRY(MP_BEACONHOST),
#ifdef OS_WIN
   CONFIG_ENTRY(MP_NET_CONNECTION),
#elif defined(OS_UNIX)
   CONFIG_ENTRY(MP_NET_ON_COMMAND),
   CONFIG_ENTRY(MP_NET_OFF_COMMAND),
#endif // platform
#endif // USE_DIALUP

   CONFIG_NONE(),
   CONFIG_ENTRY(MP_TCP_OPENTIMEOUT),
#ifdef USE_TCP_TIMEOUTS
   CONFIG_ENTRY(MP_TCP_READTIMEOUT),
   CONFIG_ENTRY(MP_TCP_WRITETIMEOUT),
   CONFIG_ENTRY(MP_TCP_CLOSETIMEOUT),
#endif // USE_TCP_TIMEOUTS
#ifdef USE_IMAP_PREFETCH
   CONFIG_NONE(), // IMAP look ahead help
   CONFIG_ENTRY(MP_IMAP_LOOKAHEAD),
#endif // USE_IMAP_PREFETCH
   CONFIG_NONE(), // rsh help
   CONFIG_ENTRY(MP_TCP_RSHTIMEOUT),

   // new mail handling
   CONFIG_NONE(), // new mail handling help (for the app)
   CONFIG_NONE(), // new mail handling help (for the folder)
   CONFIG_NONE(), // configure filters
   CONFIG_ENTRY(MP_TREAT_AS_JUNK_MAIL_FOLDER),

   CONFIG_NONE(), // collect mail (MF_FLAGS_INCOMING)
   CONFIG_ENTRY(MP_NEWMAIL_FOLDER),
   CONFIG_ENTRY(MP_MOVE_NEWMAIL),

   CONFIG_NONE(), // notify help
   CONFIG_ENTRY(MP_USE_NEWMAILCOMMAND),
   CONFIG_ENTRY(MP_NEWMAILCOMMAND),

   CONFIG_NONE(), // play sound help
   CONFIG_ENTRY(MP_NEWMAIL_PLAY_SOUND),
   CONFIG_ENTRY(MP_NEWMAIL_SOUND_FILE),
#if defined(OS_UNIX) || defined(__CYGWIN__)
   CONFIG_ENTRY(MP_NEWMAIL_SOUND_PROGRAM),
#endif // OS_UNIX

   CONFIG_ENTRY(MP_SHOW_NEWMAILMSG),
   CONFIG_NONE(), // details threshold help
   CONFIG_ENTRY(MP_SHOW_NEWMAILINFO),

   CONFIG_NONE(), // "new mail must be unread" help
   CONFIG_ENTRY(MP_NEWMAIL_UNSEEN),

   CONFIG_NONE(), // update help
   CONFIG_ENTRY(MP_UPDATEINTERVAL),
   CONFIG_ENTRY(MP_POLLINCOMINGDELAY),
   CONFIG_NONE(), // monitor help
   CONFIG_NONE(), // monitor folder (MF_FLAGS_MONITOR)
   CONFIG_ENTRY(MP_POLLINCOMINGDELAY),
   CONFIG_ENTRY(MP_POLL_OPENED_ONLY),
   CONFIG_NONE(), // poll at startup help
   CONFIG_ENTRY(MP_COLLECTATSTARTUP),

   // compose
   CONFIG_ENTRY(MP_USEOUTGOINGFOLDER), // where to keep copies of messages sent
   CONFIG_ENTRY(MP_OUTGOINGFOLDER),
   CONFIG_ENTRY(MP_WRAPMARGIN),
//   CONFIG_ENTRY(MP_AUTOMATIC_WORDWRAP), // Meaningless with minimal editor
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_DEFAULT_REPLY_KIND),
   CONFIG_ENTRY(MP_REPLY_PREFIX),
   CONFIG_ENTRY(MP_FORWARD_PREFIX),
   CONFIG_ENTRY(MP_REPLY_COLLAPSE_PREFIX),
   CONFIG_ENTRY(MP_REPLY_QUOTE_ORIG),
   CONFIG_ENTRY(MP_REPLY_QUOTE_SELECTION),
   CONFIG_ENTRY(MP_REPLY_MSGPREFIX),
   CONFIG_ENTRY(MP_REPLY_MSGPREFIX_FROM_XATTR),
   CONFIG_ENTRY(MP_REPLY_MSGPREFIX_FROM_SENDER),
   CONFIG_ENTRY(MP_REPLY_QUOTE_EMPTY),
   CONFIG_ENTRY(MP_REPLY_DETECT_SIG),
#if wxUSE_REGEX
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_REPLY_SIG_SEPARATOR),
#endif // wxUSE_REGEX

   CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE),
   CONFIG_ENTRY(MP_COMPOSE_SIGNATURE),
   CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE_SEPARATOR),
   CONFIG_ENTRY(MP_COMPOSE_XFACE_FILE),
   // not very useful: CONFIG_ENTRY(MP_ADB_SUBSTRINGEXPANSION),

   CONFIG_NONE(),
#ifdef USE_FONT_DESC
   CONFIG_ENTRY(MP_CVIEW_FONT_DESC),
#else // !USE_FONT_DESC
   CONFIG_ENTRY(MP_CVIEW_FONT),
   CONFIG_ENTRY(MP_CVIEW_FONT_SIZE),
#endif // USE_FONT_DESC/!USE_FONT_DESC
   CONFIG_ENTRY(MP_CVIEW_FGCOLOUR),
   CONFIG_ENTRY(MP_CVIEW_BGCOLOUR),
   CONFIG_ENTRY(MP_CVIEW_COLOUR_HEADERS),

   CONFIG_NONE(), // preview help
   CONFIG_ENTRY(MP_PREVIEW_SEND),
   CONFIG_ENTRY(MP_CONFIRM_SEND),

   CONFIG_NONE(), // spacer
   CONFIG_ENTRY(MP_COMPOSE_SHOW_FROM),

   CONFIG_NONE(), // headers button
   CONFIG_NONE(), // templates button

   // folders
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_DONTOPENSTARTUP),
   CONFIG_ENTRY(MP_REOPENLASTFOLDER),
   CONFIG_ENTRY(MP_OPENFOLDERS),
   CONFIG_ENTRY(MP_MAINFOLDER),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FOLDERPROGRESS_THRESHOLD),
   CONFIG_ENTRY(MP_SHOWBUSY_DURING_SORT),
   CONFIG_NONE(), // keep alive help
   CONFIG_ENTRY(MP_FOLDER_CLOSE_DELAY),
   CONFIG_ENTRY(MP_CONN_CLOSE_DELAY),
   CONFIG_NONE(), // outbox help
   CONFIG_ENTRY(MP_USE_OUTBOX),
   CONFIG_ENTRY(MP_OUTBOX_NAME),
   CONFIG_NONE(), // trash help
   CONFIG_ENTRY(MP_USE_TRASH_FOLDER),
   CONFIG_ENTRY(MP_TRASH_FOLDER),
   CONFIG_ENTRY(MP_DRAFTS_FOLDER),
   CONFIG_ENTRY(MP_DRAFTS_AUTODELETE),
   CONFIG_ENTRY(MP_FOLDER_FILE_DRIVER),
   CONFIG_ENTRY(MP_CREATE_INTERNAL_MESSAGE),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FOLDERSTATUS_STATBAR),
   CONFIG_ENTRY(MP_FOLDERSTATUS_TITLEBAR),

   // python
#ifdef USE_PYTHON
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_USEPYTHON),
   CONFIG_ENTRY(MP_PYTHONPATH),
   CONFIG_ENTRY(MP_STARTUPSCRIPT),
   ConfigValueDefault(MCB_FOLDEROPEN, ""),
   ConfigValueDefault(MCB_FOLDERUPDATE, ""),
   ConfigValueDefault(MCB_FOLDEREXPUNGE, ""),
   ConfigValueDefault(MCB_FOLDERSETMSGFLAG, ""),
   ConfigValueDefault(MCB_FOLDERCLEARMSGFLAG, ""),
#endif // USE_PYTHON

   // message view
   CONFIG_NONE(),
   CONFIG_NONE(), // and not MP_MSGVIEW_VIEWER: we handle it specially
#ifdef USE_VIEWER_BAR
   CONFIG_ENTRY(MP_MSGVIEW_SHOWBAR),
#endif // USE_VIEWER_BAR
#ifdef USE_FONT_DESC
   CONFIG_ENTRY(MP_MVIEW_FONT_DESC),
#else // !USE_FONT_DESC
   CONFIG_ENTRY(MP_MVIEW_FONT),
   CONFIG_ENTRY(MP_MVIEW_FONT_SIZE),
#endif // USE_FONT_DESC/!USE_FONT_DESC
   CONFIG_ENTRY(MP_MVIEW_FGCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_BGCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_HEADER_NAMES_COLOUR),
   CONFIG_ENTRY(MP_MVIEW_HEADER_VALUES_COLOUR),
   CONFIG_ENTRY(MP_HIGHLIGHT_URLS),
   CONFIG_ENTRY(MP_MVIEW_URLCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_QUOTED_COLOURIZE),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_MVIEW_QUOTED_COLOUR1),
   CONFIG_ENTRY(MP_MVIEW_QUOTED_COLOUR2),
   CONFIG_ENTRY(MP_MVIEW_QUOTED_COLOUR3),
   CONFIG_ENTRY(MP_MVIEW_QUOTED_CYCLE_COLOURS),
   CONFIG_ENTRY(MP_HIGHLIGHT_SIGNATURE),
   CONFIG_ENTRY(MP_MVIEW_SIGCOLOUR),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_MESSAGEPROGRESS_THRESHOLD_SIZE),
   CONFIG_ENTRY(MP_MESSAGEPROGRESS_THRESHOLD_TIME),
   CONFIG_NONE(), // max msg size help
   CONFIG_ENTRY(MP_MAX_MESSAGE_SIZE),
   CONFIG_NONE(), // inline images help
   CONFIG_ENTRY(MP_INLINE_GFX),
   CONFIG_ENTRY(MP_INLINE_GFX_SIZE),
   CONFIG_ENTRY(MP_INLINE_GFX_EXTERNAL),
   CONFIG_ENTRY(MP_PLAIN_IS_TEXT),
   CONFIG_ENTRY(MP_RFC822_IS_TEXT),
   CONFIG_ENTRY(MP_VIEW_WRAPMARGIN),
   CONFIG_ENTRY(MP_VIEW_AUTOMATIC_WORDWRAP),
#ifdef OS_UNIX
   CONFIG_NONE(), // fax help
   CONFIG_ENTRY(MP_INCFAX_SUPPORT),
   CONFIG_ENTRY(MP_INCFAX_DOMAINS),
   CONFIG_ENTRY(MP_TIFF2PS),
#endif
   CONFIG_ENTRY(MP_MSGVIEW_HEADERS),
   CONFIG_ENTRY(MP_DATE_FMT),
   CONFIG_ENTRY(MP_MVIEW_TITLE_FMT),

   // folder view
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_AUTOSHOW_LASTSELECTED),
   CONFIG_NONE(), // MP_AUTOSHOW_XXX radio
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_PREVIEW_ON_SELECT),
   CONFIG_ENTRY(MP_AUTOSHOW_SELECT),

   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FVIEW_AUTONEXT_UNREAD_MSG),
   CONFIG_ENTRY(MP_FVIEW_AUTONEXT_UNREAD_FOLDER),

   CONFIG_NONE(),    // layout help
   CONFIG_ENTRY(MP_FVIEW_VERTICAL_SPLIT),
   CONFIG_ENTRY(MP_FVIEW_FVIEW_TOP),

   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FVIEW_NAMES_ONLY),
   CONFIG_ENTRY(MP_FVIEW_FROM_REPLACE),
#ifdef USE_FONT_DESC
   CONFIG_ENTRY(MP_FVIEW_FONT_DESC),
#else // !USE_FONT_DESC
   CONFIG_ENTRY(MP_FVIEW_FONT),
   CONFIG_ENTRY(MP_FVIEW_FONT_SIZE),
#endif // USE_FONT_DESC/!USE_FONT_DESC
   CONFIG_ENTRY(MP_FVIEW_FGCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_BGCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_NEWCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_RECENTCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_UNREADCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_DELETEDCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_FLAGGEDCOLOUR),

   CONFIG_NONE(), // sorting/threading help
   CONFIG_ENTRY(MP_MSGS_USE_THREADING),
   CONFIG_NONE(), // threading subdialog
   CONFIG_NONE(), // sorting subdialog
   CONFIG_NONE(), // columns subdialog
   CONFIG_ENTRY(MP_FVIEW_SIZE_FORMAT),
   CONFIG_NONE(), // preview delay help
   CONFIG_ENTRY(MP_FVIEW_PREVIEW_DELAY),
   CONFIG_NONE(), // status/title format help
   CONFIG_ENTRY(MP_FVIEW_STATUS_UPDATE),
   CONFIG_ENTRY(MP_FVIEW_STATUS_FMT),

   // folder tree
   CONFIG_ENTRY(MP_FTREE_LEFT),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FTREE_FGCOLOUR),
   CONFIG_ENTRY(MP_FTREE_BGCOLOUR),
   CONFIG_ENTRY(MP_FTREE_SHOWOPENED),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FTREE_FORMAT),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FTREE_PROPAGATE),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FTREE_NEVER_UNREAD),
   CONFIG_ENTRY(MP_OPEN_ON_CLICK),
   CONFIG_ENTRY(MP_SHOW_HIDDEN_FOLDERS),
   CONFIG_NONE(), // home help
   CONFIG_ENTRY(MP_FTREE_HOME),
   CONFIG_NONE(), // is home checkbox

   // addresses
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FROM_REPLACE_ADDRESSES),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_LIST_ADDRESSES),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_AUTOCOLLECT),
   CONFIG_ENTRY(MP_AUTOCOLLECT_ADB),
   CONFIG_ENTRY(MP_AUTOCOLLECT_SENDER),
   CONFIG_ENTRY(MP_AUTOCOLLECT_NAMED),
   CONFIG_ENTRY(MP_WHITE_LIST),
#ifdef USE_BBDB
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_BBDB_IGNOREANONYMOUS),
   CONFIG_ENTRY(MP_BBDB_GENERATEUNIQUENAMES),
   CONFIG_ENTRY(MP_BBDB_ANONYMOUS),
   CONFIG_ENTRY(MP_BBDB_SAVEONEXIT),
#endif // USE_BBDB

   // helper programs
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_BROWSER),
#ifndef OS_WIN
   CONFIG_ENTRY(MP_BROWSER_ISNS),
#endif // OS_WIN
   CONFIG_ENTRY(MP_BROWSER_INNW),

#ifdef OS_UNIX
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_HELPBROWSER_KIND),
   CONFIG_ENTRY(MP_HELPBROWSER),
   CONFIG_ENTRY(MP_HELPBROWSER_ISNS),
#endif // OS_UNIX

#ifdef OS_UNIX
   CONFIG_ENTRY(MP_CONVERTPROGRAM),
   CONFIG_ENTRY(MP_TMPGFXFORMAT),
#endif

   CONFIG_NONE(),
   CONFIG_ENTRY(MP_EXTERNALEDITOR),
   CONFIG_ENTRY(MP_ALWAYS_USE_EXTERNALEDITOR),
#ifdef USE_OPENSSL
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_SSL_DLL_SSL),
   CONFIG_ENTRY(MP_SSL_DLL_CRYPTO),
#endif // USE_OPENSSL
   CONFIG_NONE(), // PGP help
   CONFIG_ENTRY(MP_PGP_COMMAND),
   CONFIG_ENTRY(MP_PGP_KEYSERVER),
   CONFIG_ENTRY(MP_PGP_GET_PUBKEY),

#ifdef USE_TEST_PAGE
   CONFIG_NONE(),
   CONFIG_NONE(),
#endif // USE_TEST_PAGE

   // other
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_SHOWLOG),
   CONFIG_ENTRY(MP_LOGFILE),
   CONFIG_ENTRY(MP_DEBUG_CCLIENT),
   CONFIG_ENTRY(MP_SHOWTIPS),
   CONFIG_ENTRY(MP_SHOWSPLASH),
   CONFIG_ENTRY(MP_SPLASHDELAY),
   CONFIG_NONE(),                   // env vars help
   CONFIG_ENTRY(MP_EXPAND_ENV_VARS),
   CONFIG_ENTRY(MP_TBARIMAGES),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_AUTOSAVEDELAY),
   CONFIG_ENTRY(MP_CONFIRMEXIT),
   CONFIG_ENTRY(MP_RUNONEONLY),
   CONFIG_ENTRY(MP_HELPDIR),
#ifdef OS_UNIX
   CONFIG_ENTRY(MP_AFMPATH),
#endif // OS_UNIX
#if defined(OS_UNIX) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   CONFIG_ENTRY(MP_FOCUS_FOLLOWSMOUSE),
#endif
#ifdef OS_UNIX
   CONFIG_ENTRY(MP_DOCKABLE_MENUBARS),
   CONFIG_ENTRY(MP_DOCKABLE_TOOLBARS),
   CONFIG_ENTRY(MP_FLAT_TOOLBARS),
#endif // OS_UNIX
   CONFIG_NONE(), // reenable disabled msg boxes
   CONFIG_NONE(), // away help
   CONFIG_ENTRY(MP_AWAY_AUTO_ENTER),
   CONFIG_ENTRY(MP_AWAY_AUTO_EXIT),
   CONFIG_ENTRY(MP_AWAY_REMEMBER),

   // sync
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_SYNC_REMOTE),
   CONFIG_ENTRY(MP_SYNC_FOLDER),
   CONFIG_ENTRY(MP_SYNC_FILTERS),
   CONFIG_ENTRY(MP_SYNC_IDS),
   CONFIG_ENTRY(MP_SYNC_FOLDERS),
   CONFIG_ENTRY(MP_SYNC_FOLDERGROUP),
   CONFIG_NONE(),
   CONFIG_NONE(),
#ifdef OS_WIN
   CONFIG_NONE(), // separator
   CONFIG_NONE(), // help for config file setting
   CONFIG_ENTRY(MP_USE_CONFIG_FILE),
#endif // OS_WIN
};

#undef CONFIG_ENTRY
#undef CONFIG_NONE

// check that we didn't forget to update one of the arrays...
wxCOMPILE_TIME_ASSERT(
      WXSIZEOF(wxOptionsPageStandard::ms_aConfigDefaults) == ConfigField_Max,
      MismatchFieldDefaults
   );

wxCOMPILE_TIME_ASSERT(
      WXSIZEOF(wxOptionsPageStandard::ms_aFields) == ConfigField_Max,
      MismatchFieldDescs
   );

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxOptionsPage
// ----------------------------------------------------------------------------

wxOptionsPage::wxOptionsPage(FieldInfoArray aFields,
                             ConfigValuesArray aDefaults,
                             size_t nFirst,
                             size_t nLast,
                             wxNotebook *notebook,
                             const wxChar *title,
                             Profile *profile,
                             int helpId,
                             int image)
             : wxNotebookPageBase(notebook)
{
   // no listbox by default
   m_lboxData = NULL;

   m_aFields = aFields;
   m_aDefaults = aDefaults;

   // assume that if the user doesn't want to see the images in the toolbar, he
   // doesn't want to see them elsewhere, in particular in the notebook tabs
   // neither
   //
   // NB: the config values are shifted related to the enum values, hence "+1"
   if ( (int)READ_APPCONFIG(MP_TBARIMAGES) + 1 == TbarShow_Text )
   {
      image = -1;
   }

   notebook->AddPage(this, title, FALSE /* don't select */, image);

   m_Profile = profile;
   m_Profile->IncRef();

   m_HelpId = helpId;

   m_nFirst = nFirst;
   m_nLast = nLast;

   CreateControls();
}

wxOptionsPage::~wxOptionsPage()
{
   while ( m_lboxData )
   {
      LboxData *data = m_lboxData;
      m_lboxData = data->m_next;

      delete data;
   }

   SafeDecRef(m_Profile);
}

String wxOptionsPage::GetFolderName() const
{
   return m_Profile->GetFolderName();
}

void wxOptionsPage::CreateControls()
{
   size_t n;

   // some fields are only shown in 'advanced' mode, so check if we're in it
   //
   // update: now we don't have 2 modes any more but I still keep
   // Field_Advanced for now, maybe we're going to emphasize them somehow or
   // warn the user that he needs to understand what he is doing when changing
   // them or something like this in the future
   bool isAdvanced = true;

   // some others are not shown when we're inside an identity or folder dialog
   // but only in the global one
   wxGlobalOptionsDialog *dialog = GET_PARENT_OF_CLASS(this, wxGlobalOptionsDialog);
   bool isIdentDialog = dialog && !dialog->IsGlobalOptionsDialog();
   bool isFolderDialog = !dialog;

   // first determine the longest label
   wxArrayString aLabels;
   for ( n = m_nFirst; n < m_nLast; n++ ) {
      wxString label;

      int flags = GetFieldFlags(n);

      // don't show the global settings when editing the identity dialog and
      // don't show the advanced ones in the novice mode
      //
      // also don't show app-wide settings when editing the folder proprieties
      // nor the folder settings when editing the global options
      //
      // NB: when modifying this code, don't forget to update the code below!
      if ( (!isAdvanced && (flags & Field_Advanced)) ||
           (isIdentDialog && (flags & Field_Global)) ||
           (isFolderDialog && (flags & Field_AppWide)) ||
           (!isFolderDialog && (flags & Field_NotApp)) )
      {
         // skip this one
         continue;
      }

      // do it only for text control labels
      switch ( GetFieldType(n) ) {
         case Field_Passwd:
         case Field_Number:
         case Field_Dir:
         case Field_File:
         case Field_Color:
         case Field_Folder:
         case Field_Bool:
            // fall through: for this purpose (finding the longest label)
            // they're the same as text
         case Field_Text:
            label = wxGetTranslation(m_aFields[n].label);
            break;

         case Field_Radio:
            // take only the label part in consideration
            label = wxGetTranslation(m_aFields[n].label);
            label = label.BeforeFirst(':');
            break;

         default:
            // don't take into account the other types
            continue;
      }

      aLabels.Add(label);
   }

   long widthMax = GetMaxLabelWidth(aLabels, this);

   // now create the controls
   int styleText = wxTE_LEFT;
   wxControl *last = NULL; // last control created
   for ( n = m_nFirst; n < m_nLast; n++ ) {
      int flags = GetFieldFlags(n);
      if ( (!isAdvanced && (flags & Field_Advanced)) ||
           (isIdentDialog && (flags & Field_Global)) ||
           (isFolderDialog && (flags & Field_AppWide)) ||
           (!isFolderDialog && (flags & Field_NotApp)) )
      {
         // skip this one
         m_aControls.Add(NULL);
         m_aDirtyFlags.Add(false);

         continue;
      }

      switch ( GetFieldType(n) ) {
         case Field_Dir:
            last = CreateDirEntry(wxGetTranslation(m_aFields[n].label), widthMax, last);
            break;

         case Field_File:
            last = CreateFileEntry(wxGetTranslation(m_aFields[n].label), widthMax, last,
                                   NULL, !(flags & Field_FileSave));
            break;

         case Field_Folder:
            last = CreateFolderEntry(wxGetTranslation(m_aFields[n].label), widthMax, last);
            break;

         case Field_Color:
            last = CreateColorEntry(wxGetTranslation(m_aFields[n].label), widthMax, last);
            break;

         case Field_Font:
            last = CreateFontEntry(wxGetTranslation(m_aFields[n].label), widthMax, last);
            break;

         case Field_Radio:
            if ( wxStrchr(m_aFields[n].label, _T(':')) )
               last = CreateRadioBox(wxGetTranslation(m_aFields[n].label), widthMax, last);
            else
               last = CreateActionChoice(wxGetTranslation(m_aFields[n].label), widthMax, last);
            break;

         case Field_Combo:
            last = CreateChoice(wxGetTranslation(m_aFields[n].label), widthMax, last);
            break;

         case Field_Passwd:
            styleText |= wxTE_PASSWORD;
            // fall through

         case Field_Number:
            // fall through -- for now they're the same as text
         case Field_Text:
            last = CreateTextWithLabel(wxGetTranslation(m_aFields[n].label), widthMax, last,
                                       0, styleText);

            // reset
            styleText = wxTE_LEFT;
            break;

         case Field_List:
            last = CreateListbox(wxGetTranslation(m_aFields[n].label), last);
            break;

         case Field_Bool:
            last = CreateCheckBox(wxGetTranslation(m_aFields[n].label), widthMax, last);
            break;

         case Field_Message:
            last = CreateMessage(wxGetTranslation(m_aFields[n].label), last);
            break;

         case Field_SubDlg:
            last = CreateButton(wxGetTranslation(m_aFields[n].label), last);
            break;

         case Field_XFace:
            last = CreateXFaceButton(wxGetTranslation(m_aFields[n].label), widthMax, last);
            break;

         default:
            wxFAIL_MSG(_T("unknown field type in CreateControls"));
         }

      wxCHECK_RET( last, _T("control creation failed") );

      // only global settings may be vital, we shouldn't bother the user if he
      // changed some of these settings for one folder only
      if ( !isFolderDialog )
      {
         if ( flags & Field_Vital )
            m_aVitalControls.Add(last);

         if ( flags & Field_Restart )
            m_aRestartControls.Add(last);
      }

      m_aControls.Add(last);
      m_aDirtyFlags.Add(false);
   }
}

bool wxOptionsPage::OnChangeCommon(wxControl *control)
{
   int index = m_aControls.Index(control);

   if ( index == wxNOT_FOUND )
   {
      // we can get events from the text controls from "file open" dialog here
      // too - just skip them silently
      return FALSE;
   }

   // mark this control as being dirty
   m_aDirtyFlags[(size_t)index] = true;

   // update this page controls state
   UpdateUI();

   // mark the dialog as being dirty
   wxOptionsEditDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsEditDialog);
   CHECK( dialog, FALSE, _T("option page without option dialog?") );

   if ( m_aVitalControls.Index(control) != -1 )
      dialog->SetDoTest();
   else
      dialog->SetDirty();

   if ( m_aRestartControls.Index(control) != -1 )
      dialog->SetGiveRestartWarning();

   return TRUE;
}

void wxOptionsPage::OnTextChange(wxEvent& event)
{
   // special case of text controls associated with the colour browsing
   // buttons: we have to translate this event into one from the button itself
   // because otherwise OnChangeCommon() would never find it in m_aControls
   // array (we don't keep text control itself there)
   wxControl *control = wxStaticCast(event.GetEventObject(), wxControl);
   CHECK_RET( control, _T("text event from nowhere?") );

   wxWindow *win = FindWindow(NextControlId(control->GetId()));
   wxColorBrowseButton *btn = wxDynamicCast(win, wxColorBrowseButton);
   if ( btn )
   {
      OnChangeCommon(btn);
   }
   else // normal text event
   {
      OnChange(event);
   }
}

void wxOptionsPage::OnChange(wxEvent& event)
{
   if ( !OnChangeCommon((wxControl *)event.GetEventObject()) )
   {
      // not our event
      event.Skip();
   }
}

void wxOptionsPage::OnControlChange(wxEvent& event)
{
   OnChange(event);
}

void wxOptionsPage::UpdateUI()
{
   for ( size_t n = m_nFirst; n < m_nLast; n++ ) {
      int nCheckField = m_aFields[n].enable;
      if ( nCheckField != -1 ) {
         wxControl *control = GetControl(n);

         // the control might be NULL if it is an advanced control and the
         // user level is novice
         if ( !control )
            continue;

         // HACK: if the index of the field we depend on is negative, inverse
         //       the usual logic, i.e. only enable this field if the checkbox
         //       is cleared and disabled it otherwise
         bool inverseMeaning = nCheckField < 0;
         if ( inverseMeaning ) {
            nCheckField = -nCheckField;
         }

         wxASSERT( nCheckField >= 0 && nCheckField < ConfigField_Max );

         // avoid signed/unsigned mismatch in expressions
         size_t nCheck = (size_t)nCheckField;
         wxCHECK_RET( nCheck >= m_nFirst && nCheck < m_nLast,
                      _T("control index out of range") );

         bool bEnable = true;
         wxControl *controlDep = GetControl(nCheck);
         if ( !controlDep )
         {
            // control we depend on wasn't created: this is possible if it is
            // advanced/global control and we're in novice/identity mode, so
            // just ignore it
            continue;
         }

         // first of all, check if the control we depend on is not disabled
         // itself because some other control overrides it too - if it is
         // disabled, we are disabled as well
         if ( !controlDep->IsEnabled() )
         {
            bEnable = false;
         }
         else // control we depend on is enabled
         {
            if ( GetFieldType(nCheck) == Field_Bool )
            {
               // enable only if the checkbox is checked
               wxCheckBox *checkbox = wxStaticCast(controlDep, wxCheckBox);

               bEnable = checkbox->GetValue();
            }
            else if ( GetFieldType(nCheck) == Field_Radio )
            {
               // disable if the radiobox selection is 0 (meaning "yes")
               wxRadioBox *radiobox = wxStaticCast(controlDep, wxRadioBox);

               if ( radiobox && radiobox->GetSelection() == 0 )
                  bEnable = false;
            }
            else if ( GetFieldType(nCheck) == Field_Combo )
            {
               // disable if the combobox selection is 0
               wxChoice *choice = wxStaticCast(controlDep, wxChoice);

               if ( choice && choice->GetSelection() == 0 )
                  bEnable = false;
            }
            else
            {
               // assume that this is one of the text controls
               wxTextCtrl *text = wxStaticCast(controlDep, wxTextCtrl);
               wxCHECK_RET( text, _T("can't depend on this control type") );

               // only enable if the text control has something
               bEnable = !text->GetValue().IsEmpty();
            }

            if ( inverseMeaning )
               bEnable = !bEnable;
         }

         control->Enable(bEnable);

         switch ( GetFieldType(n) )
         {
               // for file entries, also disable the browse button
            case Field_File:
            case Field_Dir:
            case Field_Folder:
            case Field_Font:
               wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

               EnableTextWithButton((wxTextCtrl *)control, bEnable);
               break;

            case Field_Color:
               EnableColourBrowseButton
               (
                  wxStaticCast(control, wxColorBrowseButton),
                  bEnable
               );
               break;

            case Field_Passwd:
            case Field_Number:
            case Field_Text:
               wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

               EnableTextWithLabel((wxTextCtrl *)control, bEnable);
               break;

            case Field_List:
               EnableListBox(wxDynamicCast(control, wxListBox), bEnable);
               break;

            case Field_Combo:
               EnableControlWithLabel(control, bEnable);

            default:
                ;
         }
      }
      // this field is always enabled
   }
}

// read the data from config
bool wxOptionsPage::TransferDataToWindow()
{
   // disable environment variable expansion here because we want the user to
   // edit the real value stored in the config
   ProfileEnvVarSave suspend(m_Profile, false);

   Profile::ReadResult readResult;
   String strValue;
   long lValue = 0;
   for ( size_t n = m_nFirst; n < m_nLast; n++ )
   {
      if ( !*m_aDefaults[n].name )
      {
         // it doesn't have any associated value
         continue;
      }

      if ( m_aDefaults[n].IsNumeric() )
      {
         lValue = m_Profile->readEntry(m_aDefaults[n].name,
                                       (int)m_aDefaults[n].lValue,
                                       &readResult);
         strValue.Printf(_T("%ld"), lValue);
      }
      else {
         // it's a string
         strValue = m_Profile->readEntry(m_aDefaults[n].name,
                                         m_aDefaults[n].szValue,
                                         &readResult);
      }

      wxControl *control = GetControl(n);
      if ( !control )
         continue;

      wxControl *label = NULL;
      switch ( GetFieldType(n) )
      {
         case Field_Text:
         case Field_Number:
            if ( GetFieldType(n) == Field_Number ) {
               wxASSERT( m_aDefaults[n].IsNumeric() );

               strValue.Printf(_T("%ld"), lValue);
            }
            else {
               wxASSERT( !m_aDefaults[n].IsNumeric() );
            }

            // can only have text value
         case Field_Passwd:
            if( GetFieldType(n) == Field_Passwd )
               strValue = strutil_decrypt(strValue);

         case Field_Font:
            strValue = wxFontBrowseButton::FontDescToUser(strValue);
            // fall through

         case Field_Dir:
         case Field_File:
         case Field_Folder:
            wxStaticCast(control, wxTextCtrl)->SetValue(strValue);
            break;

         case Field_Color:
            wxStaticCast(control, wxColorBrowseButton)->SetValue(strValue);
            break;

         case Field_Bool:
            wxASSERT( m_aDefaults[n].IsNumeric() );

            if ( GetFieldFlags(n) & Field_Inverse )
            {
               lValue = !lValue;
            }

            wxStaticCast(control, wxCheckBox)->SetValue(lValue != 0);

            label = control;
            break;

         case Field_Radio:
            wxStaticCast(control, wxRadioBox)->SetSelection(lValue);
            break;

         case Field_Combo:
            wxStaticCast(control, wxChoice)->SetSelection(lValue);
            break;

         case Field_List:
            wxASSERT( !m_aDefaults[n].IsNumeric() );

            {
               wxListBox *lbox = wxStaticCast(control, wxListBox);

               // split it on the separator char: this is ':' for everything
               // except ConfigField_OpenFolders where it is ';' for config
               // backwards compatibility
               wxChar ch = n == ConfigField_OpenFolders ? ';' : ':';
               wxArrayString entries = strutil_restore_array(strValue, ch);

               size_t count = entries.GetCount();
               for ( size_t m = 0; m < count; m++ )
               {
                  lbox->Append(entries[m]);
               }
            }
            break;

         case Field_XFace:
         {
            wxXFaceButton *btnXFace = (wxXFaceButton *)control;
            if ( READ_CONFIG(m_Profile, MP_COMPOSE_USE_XFACE) )
               btnXFace->SetFile(READ_CONFIG(m_Profile, MP_COMPOSE_XFACE_FILE));
            else
               btnXFace->SetFile(_T(""));
         }
         break;

         case Field_SubDlg:      // these settings will be read later
            break;

         case Field_Message:
         default:
            wxFAIL_MSG(_T("unexpected field type"));
      }

#ifdef EXPERIMENTAL_zeitlin
      if ( readResult )
      {
         if ( !label )
            label = GetLabelForControl(control);

         if ( label )
            label->SetForegroundColour(readResult == Profile::Read_FromHere
                                          ? *wxRED
                                          : *wxBLUE);
      }
#endif // EXPERIMENTAL_zeitlin

      // the dirty flag was set from the OnChange() callback, reset it!
      ClearDirty(n);
   }

   return TRUE;
}

// write the data to config
bool wxOptionsPage::TransferDataFromWindow()
{
   String strValue;
   long lValue = 0;
   for ( size_t n = m_nFirst; n < m_nLast; n++ )
   {
      // only write the controls which were really changed
      if ( !IsDirty(n) )
         continue;

      wxControl *control = GetControl(n);
      if ( !control )
         continue;

      // some controls are not transfered to config in the normal way, this is
      // indicated by using CONFIG_NONE() in the ms_aConfigDefaults array for
      // which the name is empty
      if ( !*m_aDefaults[n].name )
         continue;

      FieldType fieldType = GetFieldType(n);
      switch ( fieldType )
      {
         case Field_Passwd:
         case Field_Text:
         case Field_Dir:
         case Field_File:
         case Field_Color:
         case Field_Font:
         case Field_Folder:
         case Field_Number:
            if ( fieldType == Field_Color )
               strValue = wxStaticCast(control, wxColorBrowseButton)->GetValue();
            else
               strValue = wxStaticCast(control, wxTextCtrl)->GetValue();

            // post processing is needed for some fields
            if ( fieldType == Field_Passwd )
               strValue = strutil_encrypt(strValue);
            else if ( fieldType == Field_Font )
               strValue = wxFontBrowseButton::FontDescFromUser(strValue);
            else if ( fieldType == Field_Number ) {
               wxASSERT( m_aDefaults[n].IsNumeric() );

               lValue = wxAtol(strValue);
            }
            else {
               wxASSERT( !m_aDefaults[n].IsNumeric() );
            }
            break;

         case Field_Bool:
            wxASSERT( m_aDefaults[n].IsNumeric() );

            lValue = wxStaticCast(control, wxCheckBox)->GetValue();

            if ( GetFieldFlags(n) & Field_Inverse )
            {
               lValue = !lValue;
            }
            break;

         case Field_Radio:
            wxASSERT( m_aDefaults[n].IsNumeric() );

            lValue = wxStaticCast(control, wxRadioBox)->GetSelection();
            break;

         case Field_Combo:
            wxASSERT( m_aDefaults[n].IsNumeric() );

            lValue = wxStaticCast(control, wxChoice)->GetSelection();
            break;

         case Field_List:
            wxASSERT( !m_aDefaults[n].IsNumeric() );

            // join it using a separator char: this is ':' for everything except
            // ConfigField_OpenFolders where it is ';' for config backwards
            // compatibility
            {
               wxChar ch = n == ConfigField_OpenFolders ? ';' : ':';
               wxListBox *listbox = wxStaticCast(control, wxListBox);
               size_t count = listbox->GetCount();
               for ( size_t m = 0; m < count; m++ ) {
                  if ( !strValue.empty() ) {
                     strValue << ch;
                  }

                  strValue << listbox->GetString(m);
               }
            }
            break;

         case Field_Message:
         case Field_SubDlg:      // already done
         case Field_XFace:       // already done
            break;

         default:
            wxFAIL_MSG(_T("unexpected field type"));
      }

      if ( m_aDefaults[n].IsNumeric() )
      {
         m_Profile->writeEntry(m_aDefaults[n].name, (int)lValue);
      }
      else
      {
         // it's a string
         m_Profile->writeEntry(m_aDefaults[n].name, strValue);
      }
   }

   // TODO life is easy as we don't check for errors...
   return TRUE;
}

// ----------------------------------------------------------------------------
// wxOptionsPage listbox handling
// ----------------------------------------------------------------------------

bool wxOptionsPage::GetListboxFromButtonEvent(const wxEvent& event,
                                              wxListBox **pLbox,
                                              LboxData **pData) const
{
   // find the listbox associated with the button which generated this event
   wxButton *btn = wxStaticCast(event.GetEventObject(), wxButton);
   CHECK( btn, false, _T("button event from non-button?") );

   wxListBox *lbox = wxStaticCast((wxObject *)btn->GetClientData(), wxListBox);
   CHECK( lbox, false, _T("lbox button event without lbox?") );

   // find which one of our listboxes this one is
   LboxData *data;
   for ( data = m_lboxData; data; data = data->m_next )
   {
      if ( lbox == GetControl(data->m_idListbox) )
         break;
   }

   CHECK( data, false, _T("button even from foreign lbox?") );

   if ( pLbox )
      *pLbox = lbox;

   if ( pData )
      *pData = data;

   return true;
}

void wxOptionsPage::OnListBoxButton(wxCommandEvent& event)
{
   if ( !m_lboxData )
   {
      // see comment in OnUpdateUIListboxBtns()
      event.Skip();

      return;
   }

   int id = event.GetId();
   if ( id < wxOptionsPage_BtnNew || id > wxOptionsPage_BtnDelete )
   {
      event.Skip();

      return;
   }

   wxListBox *lbox;
   LboxData *data;
   if ( !GetListboxFromButtonEvent(event, &lbox, &data) )
   {
      return;
   }

   switch ( id )
   {
      case wxOptionsPage_BtnNew:
         OnListBoxAdd(lbox, *data);
         break;

      case wxOptionsPage_BtnModify:
         OnListBoxModify(lbox, *data);
         break;

      case wxOptionsPage_BtnDelete:
         OnListBoxDelete(lbox, *data);
         break;

      default:
         FAIL_MSG( _T("can't get here") );
   }
}

bool wxOptionsPage::OnListBoxAdd(wxListBox *lbox, const LboxData& lboxData)
{
   // get the string from user
   wxString str;
   if ( !MInputBox(&str,
                   lboxData.m_lboxDlgTitle,
                   lboxData.m_lboxDlgPrompt,
                   GET_PARENT_OF_CLASS(this, wxDialog),
                   lboxData.m_lboxDlgPers) ) {
      return FALSE;
   }

   // check that it's not already there
   if ( lbox->FindString(str) != -1 ) {
      // it is, don't add it twice
      wxLogError(_("String '%s' is already present in the list, not added."),
                 str.c_str());

      return FALSE;
   }

   // ok, do add it
   lbox->Append(str);

   wxOptionsPage::OnChangeCommon(lbox);

   return TRUE;
}

bool wxOptionsPage::OnListBoxModify(wxListBox *lbox, const LboxData& lboxData)
{
   int nSel = lbox->GetSelection();

   wxCHECK_MSG( nSel != -1, FALSE, _T("should be disabled") );

   wxString val = wxGetTextFromUser
                  (
                     lboxData.m_lboxDlgPrompt,
                     lboxData.m_lboxDlgTitle,
                     lbox->GetString(nSel),
                     GET_PARENT_OF_CLASS(this, wxDialog)
                  );

   if ( !val || val == lbox->GetString(nSel) )
   {
      // cancelled or unchanged
      return FALSE;
   }

   lbox->SetString(nSel, val);

   wxOptionsPage::OnChangeCommon(lbox);

   return TRUE;
}

bool
wxOptionsPage::OnListBoxDelete(wxListBox *lbox, const LboxData& /* lboxData */)
{
   int nSel = lbox->GetSelection();

   wxCHECK_MSG( nSel != -1, FALSE, _T("should be disabled") );

   lbox->Delete(nSel);

   wxOptionsPage::OnChangeCommon(lbox);

   return TRUE;
}

void wxOptionsPage::OnUpdateUIListboxBtns(wxUpdateUIEvent& event)
{
   if ( !m_lboxData )
   {
      // unfortunately this does happen sometimes: I discovered it when the
      // program crashed trying to process UpdateUI event from a popup menu in
      // the folder tree window shown from an options page because some item had
      // the same id as wxOptionsPage_BtnModify - I've changed the id now to
      // make it unique, but it's not impossible that we'll have another one in
      // the future, so be careful here
      event.Skip();
   }
   else
   {
      wxListBox *lbox;
      if ( GetListboxFromButtonEvent(event, &lbox) )
      {
         event.Enable(lbox->GetSelection() != -1);
      }
   }
}

void wxOptionsPage::SetProfile(Profile *profile)
{
   SafeDecRef(m_Profile);
   m_Profile = profile;
   SafeIncRef(m_Profile);
}

// ----------------------------------------------------------------------------
// wxOptionsPageDynamic
// ----------------------------------------------------------------------------

wxOptionsPageDynamic::wxOptionsPageDynamic(wxNotebook *parent,
                                           const wxChar *title,
                                           Profile *profile,
                                           FieldInfoArray aFields,
                                           ConfigValuesArray aDefaults,
                                           size_t nFields,
                                           size_t nOffset,
                                           int helpId,
                                           int image)
                     : wxOptionsPage(aFields - nOffset,
                                     aDefaults - nOffset,
                                     nOffset, nOffset + nFields,
                                     parent, title, profile, helpId, image)
{
}

// ----------------------------------------------------------------------------
// wxOptionsPageStandard
// ----------------------------------------------------------------------------

wxOptionsPageStandard::wxOptionsPageStandard(wxNotebook *notebook,
                                             const wxChar *title,
                                             Profile *profile,
                                             int nFirst,
                                             size_t nLast,
                                             int helpId)
                     : wxOptionsPage(ms_aFields, ms_aConfigDefaults,
                                     // see enum ConfigFields for the
                                     // explanation of "+1"
                                     (size_t)(nFirst + 1), nLast + 1,
                                     notebook, title, profile, helpId,
                                     notebook->GetPageCount())
{
   // nFirst + 1 really must be unsigned
   ASSERT_MSG( nFirst >= -1, _T("bad parameret in wxOptionsPageStandard ctor") );
}

// ----------------------------------------------------------------------------
// wxOptionsPageCompose
// ----------------------------------------------------------------------------

wxOptionsPageCompose::wxOptionsPageCompose(wxNotebook *parent,
                                           Profile *profile)
                    : wxOptionsPageStandard(parent,
                                    _("Compose"),
                                    profile,
                                    ConfigField_ComposeFirst,
                                    ConfigField_ComposeLast,
                                    MH_OPAGE_COMPOSE)
{
}

void wxOptionsPageCompose::OnButton(wxCommandEvent& event)
{
   bool dirty;

   wxObject *obj = event.GetEventObject();
   if ( obj == GetControl(ConfigField_ComposeHeaders) )
   {
      // create and show the "outgoing headers" config dialog
      dirty = ConfigureComposeHeaders(m_Profile, this);
   }
   else if ( obj == GetControl(ConfigField_ComposeTemplates) )
   {
      dirty = ConfigureTemplates(m_Profile, this);
   }
   else if ( obj == GetControl(ConfigField_XFaceFile) )
   {
      dirty = PickXFaceDialog(m_Profile, this);
      if(dirty)
      {
         wxXFaceButton *btn = (wxXFaceButton*)obj;
         // Why doesn't UpdateUI() have the same effect here?
         if(READ_CONFIG(m_Profile, MP_COMPOSE_USE_XFACE))
            btn->SetFile(READ_CONFIG(m_Profile,MP_COMPOSE_XFACE_FILE));
         else
            btn->SetFile(_T(""));
      }
   }
   else
   {
      FAIL_MSG(_T("click from alien button in compose view page"));

      dirty = FALSE;

      event.Skip();
   }

   if ( dirty )
   {
      // something changed - make us dirty
      wxOptionsEditDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsEditDialog);

      wxCHECK_RET( dialog, _T("options page without a parent dialog?") );

      dialog->SetDirty();
   }
}

// ----------------------------------------------------------------------------
// wxOptionsPageMessageView
// ----------------------------------------------------------------------------

wxOptionsPageMessageView::wxOptionsPageMessageView(wxNotebook *parent,
                                                   Profile *profile)
   : wxOptionsPageStandard(parent,
                   _("Message View"),
                   profile,
                   ConfigField_MessageViewFirst,
                   ConfigField_MessageViewLast,
                   MH_OPAGE_MESSAGEVIEW)
{
   m_currentViewer = -1;
}

void wxOptionsPageMessageView::OnButton(wxCommandEvent& event)
{
   bool dirty;

   wxObject *obj = event.GetEventObject();
   if ( obj == GetControl(ConfigField_MessageViewDateFormat) )
      dirty = ConfigureDateFormat(m_Profile, this);
   else if ( obj == GetControl(ConfigField_MessageViewHeaders) )
      dirty = ConfigureMsgViewHeaders(m_Profile, this);
   else
   {
      wxFAIL_MSG( _T("alien button") );

      dirty = false;
   }

   if ( dirty )
   {
      // something changed - make us dirty
      wxOptionsEditDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsEditDialog);
      wxCHECK_RET( dialog, _T("options page without a parent dialog?") );
      dialog->SetDirty();
   }
}

bool wxOptionsPageMessageView::TransferDataToWindow()
{
   bool bRc = wxOptionsPage::TransferDataToWindow();
   if ( bRc )
   {
      wxArrayString descViewers;
      size_t count = MessageView::GetAllAvailableViewers(&m_nameViewers,
                                                         &descViewers);

      wxWindow *win = GetControl(ConfigField_MsgViewer);
      if ( win )
      {
         wxChoice *choice = wxStaticCast(win, wxChoice);
         if ( choice )
         {
            for ( size_t n = 0; n < count; n++ )
            {
               choice->Append(descViewers[n]);
            }

            wxString name = READ_CONFIG(m_Profile, MP_MSGVIEW_VIEWER);
            m_currentViewer = m_nameViewers.Index(name);
            if ( m_currentViewer != -1 )
            {
               choice->SetSelection(m_currentViewer);
            }
         }
         else
         {
            FAIL_MSG( _T("ConfigField_MsgViewer is not a wxChoice?") );
         }
      }
      //else: we're in "novice" mode and this control doesn't exist at all
   }

   return bRc;
}

bool wxOptionsPageMessageView::TransferDataFromWindow()
{
   bool bRc = wxOptionsPage::TransferDataFromWindow();
   if ( bRc )
   {
      wxWindow *win = GetControl(ConfigField_MsgViewer);
      if ( win )
      {
         wxChoice *choice = wxStaticCast(win, wxChoice);

         if ( choice )
         {
            int sel = choice->GetSelection();
            if ( sel != -1 && sel != m_currentViewer )
            {
               m_Profile->writeEntry(MP_MSGVIEW_VIEWER,
                                     m_nameViewers[(size_t)sel]);
            }
         }
         else
         {
            FAIL_MSG( _T("ConfigField_MsgViewer is not a wxChoice?") );
         }
      }
   }

   return bRc;
}

// ----------------------------------------------------------------------------
// wxOptionsPageFolderView
// ----------------------------------------------------------------------------

// the possible values of the "Show initially" radiobox
enum
{
   FolderViewPage_Show_Unread,
   FolderViewPage_Show_First,
   FolderViewPage_Show_Last
};

wxOptionsPageFolderView::wxOptionsPageFolderView(wxNotebook *parent,
                                                 Profile *profile)
                       : wxOptionsPageStandard(parent,
                                               _("Folder View"),
                                               profile,
                                               ConfigField_FolderViewFirst,
                                               ConfigField_FolderViewLast,
                                               MH_OPAGE_MESSAGEVIEW)
{
}

bool wxOptionsPageFolderView::TransferDataToWindow()
{
   if ( !wxOptionsPageStandard::TransferDataToWindow() )
      return false;

   wxRadioBox *radio =
      wxStaticCast(GetControl(ConfigField_FolderViewShowInitially), wxRadioBox);

   CHECK( radio, false, _T("where is the initial selection radio box?") );

   int sel;
   if ( READ_CONFIG(m_Profile, MP_AUTOSHOW_FIRSTUNREADMESSAGE) )
   {
      sel = FolderViewPage_Show_Unread;
   }
   else
   {
      sel = READ_CONFIG_BOOL(m_Profile, MP_AUTOSHOW_FIRSTMESSAGE)
               ? FolderViewPage_Show_First
               : FolderViewPage_Show_Last;
   }

   radio->SetSelection(sel);

   return true;
}

bool wxOptionsPageFolderView::TransferDataFromWindow()
{
   if ( !wxOptionsPageStandard::TransferDataFromWindow() )
      return false;

   wxRadioBox *radio =
      wxStaticCast(GetControl(ConfigField_FolderViewShowInitially), wxRadioBox);

   CHECK( radio, false, _T("where is the initial selection radio box?") );

   int sel = radio->GetSelection();
   switch ( sel )
   {
      case FolderViewPage_Show_Last:
      case FolderViewPage_Show_First:
         m_Profile->writeEntry(MP_AUTOSHOW_FIRSTMESSAGE,
                               sel == FolderViewPage_Show_First);
         m_Profile->writeEntry(MP_AUTOSHOW_FIRSTUNREADMESSAGE, false);
         break;

      default:
         FAIL_MSG( _T("unexpected initial selection radiobox value") );
         // fall through

      case FolderViewPage_Show_Unread:
         m_Profile->writeEntry(MP_AUTOSHOW_FIRSTUNREADMESSAGE, true);
   }

   return true;
}

void wxOptionsPageFolderView::OnButton(wxCommandEvent& event)
{
   bool dirty;

   wxObject *obj = event.GetEventObject();
   if ( obj == GetControl(ConfigField_FolderViewSortMessagesBy) )
      dirty = ConfigureSorting(m_Profile, this);
   if ( obj == GetControl(ConfigField_FolderViewThreadMessages) )
      dirty = ConfigureThreading(m_Profile, this);
   else if ( obj == GetControl(ConfigField_FolderViewHeaders) )
      dirty = ConfigureFolderViewHeaders(m_Profile, this);
   else
   {
      event.Skip();

      dirty = false;
   }

   if ( dirty )
   {
      // something changed - make us dirty
      wxOptionsEditDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsEditDialog);
      wxCHECK_RET( dialog, _T("options page without a parent dialog?") );
      dialog->SetDirty();
   }
}

// ----------------------------------------------------------------------------
// wxOptionsPageFolderTree
// ----------------------------------------------------------------------------

wxOptionsPageFolderTree::wxOptionsPageFolderTree(wxNotebook *parent,
                                                 Profile *profile)
   : wxOptionsPageStandard(parent,
                           _("Folder Tree"),
                           profile,
                           ConfigField_FolderTreeFirst,
                           ConfigField_FolderTreeLast)
{
}

bool wxOptionsPageFolderTree::TransferDataToWindow()
{
   if ( !wxOptionsPageStandard::TransferDataToWindow() )
      return false;

   wxControl *control = GetControl(ConfigField_FolderTreeIsHome);
   if ( control )
   {
      wxCheckBox *check = wxStaticCast(control, wxCheckBox);

      // don't use CHECK here as in release builds check == control anyhow and
      // checking that it's not NULL simply results in a "unreachable code"
      // warnings from the compiler
      ASSERT_MSG( check, _T("folder tree is home control is not a checkbox?") );

      String folderHome = READ_APPCONFIG_TEXT(MP_FTREE_HOME);
      m_isHomeOrig = !folderHome.empty() && folderHome == GetFolderName();

      check->SetValue(m_isHomeOrig);
   }
   //else: ok, we could be editing the global options

   return true;
}

bool wxOptionsPageFolderTree::TransferDataFromWindow()
{
   if ( !wxOptionsPageStandard::TransferDataFromWindow() )
      return false;

   wxControl *control = GetControl(ConfigField_FolderTreeIsHome);
   if ( control )
   {
      wxCheckBox *check = wxStaticCast(control, wxCheckBox);

      // don't use CHECK here -- see above for the reason
      ASSERT_MSG( check, _T("folder tree is home control is not a checkbox?") );

      // only do something if the value really changed
      if ( check->GetValue() != m_isHomeOrig )
      {
         Profile *profile = mApplication->GetProfile();
         if ( m_isHomeOrig )
         {
            // the checkbox was unchecked, we're not the home folder any more
            profile->DeleteEntry(MP_FTREE_HOME);
         }
         else // we're the new home folder
         {
            profile->writeEntry(MP_FTREE_HOME, GetFolderName());
         }
      }
   }
   //else: ok, we could be editing the global options

   return true;
}

// ----------------------------------------------------------------------------
// wxOptionsPageIdent
// ----------------------------------------------------------------------------

wxOptionsPageIdent::wxOptionsPageIdent(wxNotebook *parent,
                                       Profile *profile)
                  : wxOptionsPageStandard(parent,
                                  _("Identity"),
                                  profile,
                                  ConfigField_IdentFirst,
                                  ConfigField_IdentLast,
                                  MH_OPAGE_IDENT)
{
}

void wxOptionsPageIdent::OnButton(wxCommandEvent& event)
{
   wxObject *obj = event.GetEventObject();
   if ( obj == GetControl(ConfigField_SetPassword) )
   {
      (void) PickGlobalPasswdDialog(m_Profile, this);
   }
   else
   {
      FAIL_MSG(_T("click from alien button in compose view page"));
      event.Skip();
   }
}

// ----------------------------------------------------------------------------
// wxOptionsPageNetwork
// ----------------------------------------------------------------------------

wxOptionsPageNetwork::wxOptionsPageNetwork(wxNotebook *parent,
                                           Profile *profile)
                    : wxOptionsPageStandard(parent,
                                    _("Network"),
                                    profile,
                                    ConfigField_NetworkFirst,
                                    ConfigField_NetworkLast,
                                    MH_OPAGE_NETWORK)
{
}

// dynamicially fill the RAS connections combo box under Windows
bool wxOptionsPageNetwork::TransferDataToWindow()
{
   m_oldAuthsDisabled = READ_CONFIG_TEXT(m_Profile, MP_SMTP_DISABLED_AUTHS);
   wxTextCtrl *text = wxDynamicCast(GetControl(ConfigField_DisabledAuths),
                                    wxTextCtrl);
   if ( text )
   {
      // remove the trailing semicolon, it looks odd
      if ( !m_oldAuthsDisabled.empty() && m_oldAuthsDisabled.Last() == _T(';') )
      {
         m_oldAuthsDisabled.Truncate(m_oldAuthsDisabled.length() - 1);
      }

      text->SetValue(m_oldAuthsDisabled);
   }

   bool bRc = wxOptionsPage::TransferDataToWindow();

#if defined(OS_WIN) && defined(USE_DIALUP)
   if ( bRc )
   {
      wxControl *control = GetControl(ConfigField_NetConnection);
      wxChoice *choice = control ? wxStaticCast(control, wxChoice) : NULL;

      if ( choice )
      {
         // may be NULL if we don't use dial up manager at all
         wxDialUpManager *dial = wxDialUpManager::Create();

         if ( dial )
         {
            wxArrayString aConnections;
            dial->GetISPNames(aConnections);

            size_t count = aConnections.GetCount();
            for ( size_t n = 0; n < count; n++ )
            {
               choice->Append(aConnections[n]);
            }

            delete dial;
         }
      }
   }
#endif // USE_DIALUP

   return bRc;
}

bool wxOptionsPageNetwork::TransferDataFromWindow()
{
   // we need to massage the disabled authentificators string a bit to fit it
   wxTextCtrl *text = wxDynamicCast(GetControl(ConfigField_DisabledAuths),
                                    wxTextCtrl);
   if ( text )
   {
      wxString authsDisabled = text->GetValue();
      if ( authsDisabled.CmpNoCase(m_oldAuthsDisabled) != 0 )
      {
         // add back the semicolon, unless there are no disabled auths at all
         if ( !authsDisabled.empty() && authsDisabled.Last() != _T(';') )
         {
            authsDisabled += _T(';');
         }

         // auth names are case-insensitive
         authsDisabled.MakeUpper();

         m_Profile->writeEntry(MP_SMTP_DISABLED_AUTHS, authsDisabled);
      }
   }

   return wxOptionsPage::TransferDataFromWindow();
}

// ----------------------------------------------------------------------------
// wxOptionsPageNewMail
// ----------------------------------------------------------------------------

wxOptionsPageNewMail::wxOptionsPageNewMail(wxNotebook *parent,
                                           Profile *profile)
                    : wxOptionsPageStandard(parent,
                                    _("New Mail"),
                                    profile,
                                    ConfigField_NewMailFirst,
                                    ConfigField_NewMailLast,
                                    MH_OPAGE_NEWMAIL)
{
   m_nIncomingDelay =
   m_nPingDelay = -1;

   m_folder = NULL;
}

wxOptionsPageNewMail::~wxOptionsPageNewMail()
{
   if ( m_folder )
      m_folder->DecRef();
}

bool wxOptionsPageNewMail::GetFolderFromProfile()
{
   CHECK( !m_folder, true, _T("creating the folder twice") );

   m_folder = MFolder::Get(GetFolderName());

   return m_folder != NULL;
}

bool wxOptionsPageNewMail::TransferDataToWindow()
{
   // the "collect new mail" checkbox corresponds to the value of
   // MF_FLAGS_INCOMING flag
   wxWindow *win = GetControl(ConfigField_NewMailCollect);
   if ( win )
   {
      if ( GetFolderFromProfile() )
      {
         m_collectOld = (m_folder->GetFlags() & MF_FLAGS_INCOMING) != 0;
      }
      else
      {
         // may happen if we're creating the folder (but didn't yet)
         m_collectOld = false;
      }

      wxStaticCast(win, wxCheckBox)->SetValue(m_collectOld);

      // we should also have the "monitor this folder" checkbox
      win = GetControl(ConfigField_NewMailMonitor);
      if ( win )
      {
         m_monitorOld = m_folder
                           ? (m_folder->GetFlags() & MF_FLAGS_MONITOR) != 0
                           : false;

         wxStaticCast(win, wxCheckBox)->SetValue(m_monitorOld);
      }
      else
      {
         FAIL_MSG( _T("where is the monitor checkbox?") );
      }
   }
   //else: happens when editing the global settings

   m_nIncomingDelay = READ_CONFIG(m_Profile, MP_POLLINCOMINGDELAY);
   m_nPingDelay = READ_CONFIG(m_Profile, MP_UPDATEINTERVAL);

   return wxOptionsPage::TransferDataToWindow();
}

bool wxOptionsPageNewMail::TransferDataFromWindow()
{
   bool rc = wxOptionsPage::TransferDataFromWindow();

   if ( rc )
   {
      // collect mail checkbox (MF_FLAGS_INCOMING)
      wxWindow *win = GetControl(ConfigField_NewMailCollect);

      if ( win )
      {
         wxCheckBox *checkbox = wxStaticCast(win, wxCheckBox);

         if ( checkbox->GetValue() != m_collectOld )
         {
            // by now we must have a valid folder
            if ( !m_folder )
            {
               if ( !GetFolderFromProfile() )
               {
                  FAIL_MSG( _T("failed to create the folder in new mail page") );
               }
            }

            if ( m_folder )
            {
               if ( m_collectOld )
                  m_folder->ResetFlags(MF_FLAGS_INCOMING);
               else
                  m_folder->AddFlags(MF_FLAGS_INCOMING);
            }
         }
      }

      // monitor folder checkbox (MF_FLAGS_MONITOR)
      win = GetControl(ConfigField_NewMailMonitor);
      if ( win )
      {
         wxCheckBox *checkbox = wxStaticCast(win, wxCheckBox);

         if ( checkbox->GetValue() != m_monitorOld )
         {
            if ( m_folder )
            {
               // NB: remember that it was toggled, hence "!m_monitorOld"
               FolderMonitor *folderMonitor = mApplication->GetFolderMonitor();
               if ( folderMonitor )
                  folderMonitor->AddOrRemoveFolder(m_folder, !m_monitorOld);
            }
         }
      }
   }

   if ( rc )
   {
      long nIncomingDelay = READ_CONFIG(m_Profile, MP_POLLINCOMINGDELAY),
           nPingDelay = READ_CONFIG(m_Profile, MP_UPDATEINTERVAL);

      if ( nIncomingDelay != m_nIncomingDelay )
      {
         wxLogTrace(_T("timer"), _T("Restarting timer for polling incoming folders"));

         rc = mApplication->RestartTimer(MAppBase::Timer_PollIncoming);
      }

      if ( rc && (nPingDelay != m_nPingDelay) )
      {
         wxLogTrace(_T("timer"), _T("Restarting timer for pinging folders"));

         rc = mApplication->RestartTimer(MAppBase::Timer_PingFolder);
      }

      if ( !rc )
      {
         wxLogError(_("Failed to restart the timers, please change the "
                      "delay to a valid value."));
      }
   }

   return rc;
}

void wxOptionsPageNewMail::OnButton(wxCommandEvent& event)
{
   wxObject *obj = event.GetEventObject();
   if ( obj != GetControl(ConfigField_NewMailConfigFilters) )
   {
      event.Skip();

      return;
   }

   ConfigureFiltersForFolder(m_folder, this);
}

// ----------------------------------------------------------------------------
// wxOptionsPagePython
// ----------------------------------------------------------------------------

#ifdef USE_PYTHON

wxOptionsPagePython::wxOptionsPagePython(wxNotebook *parent,
                                         Profile *profile)
                   : wxOptionsPageStandard(parent,
                                   _("Python"),
                                   profile,
                                   ConfigField_PythonFirst,
                                   ConfigField_PythonLast,
                                   MH_OPAGE_PYTHON)
{
}

#endif // USE_PYTHON

// ----------------------------------------------------------------------------
// wxOptionsPageAdb
// ----------------------------------------------------------------------------

wxOptionsPageAdb::wxOptionsPageAdb(wxNotebook *parent,
                                    Profile *profile)
                : wxOptionsPageStandard(parent,
                                _("Addresses"),
                                profile,
                                ConfigField_AdbFirst,
                                ConfigField_AdbLast,
                                MH_OPAGE_ADB)
{
   m_lboxData = new LboxData;
   m_lboxData->m_idListbox = ConfigField_OwnAddresses;
   m_lboxData->m_lboxDlgTitle = _("My own addresses");
   m_lboxData->m_lboxDlgPrompt = _("Address");
   m_lboxData->m_lboxDlgPers = _T("LastMyAddress");

   LboxData *lboxData2 = new LboxData;
   lboxData2->m_idListbox = ConfigField_MLAddresses;
   lboxData2->m_lboxDlgTitle = _("Mailing list addresses");
   lboxData2->m_lboxDlgPrompt = _("Address");
   lboxData2->m_lboxDlgPers = _T("LastMLAddress");

   m_lboxData->m_next = lboxData2;
}

bool wxOptionsPageAdb::TransferDataToWindow()
{
   bool bRc = wxOptionsPage::TransferDataToWindow();

   if ( bRc )
   {
      // if the listbox is empty, add the reply-to address to it
      wxListBox *listbox = wxStaticCast(GetControl(ConfigField_OwnAddresses),
                                        wxListBox);
      if ( !listbox->GetCount() )
      {
         listbox->Append(READ_CONFIG(m_Profile, MP_FROM_ADDRESS));
      }
   }

   return bRc;
}

bool wxOptionsPageAdb::TransferDataFromWindow()
{
   // if the listbox contains just the return address, empty it: it is the
   // default anyhow and this avoids remembering it in config
   wxListBox *listbox = wxStaticCast(GetControl(ConfigField_OwnAddresses),
                                     wxListBox);
   if ( listbox->GetCount() == 1 &&
        listbox->GetString(0) == READ_CONFIG(m_Profile, MP_FROM_ADDRESS) )
   {
      listbox->Clear();
   }

   return wxOptionsPage::TransferDataFromWindow();
}

// ----------------------------------------------------------------------------
// wxOptionsPageSync
// ----------------------------------------------------------------------------

wxOptionsPageSync::wxOptionsPageSync(wxNotebook *parent,
                                     Profile *profile)
                 : wxOptionsPageStandard(parent,
                                         _("Synchronize"),
                                         profile,
                                         ConfigField_SyncFirst,
                                         ConfigField_SyncLast,
                                         MH_OPAGE_SYNC)
{
#ifdef OS_WIN
   m_usingConfigFile =
#endif // OS_WIN
   m_activateSync = -1;
}

bool wxOptionsPageSync::TransferDataToWindow()
{
   bool rc = wxOptionsPage::TransferDataToWindow();
   if ( rc )
   {
      m_activateSync = READ_CONFIG(m_Profile, MP_SYNC_REMOTE);

#ifdef OS_WIN
      m_usingConfigFile =
         !READ_CONFIG_TEXT(m_Profile, MP_USE_CONFIG_FILE).empty();
#endif // OS_WIN
   }

   return rc;
}

bool wxOptionsPageSync::TransferDataFromWindow()
{
   bool rc = wxOptionsPage::TransferDataFromWindow();
   if ( rc )
   {
      bool syncRemote = READ_CONFIG_BOOL(m_Profile, MP_SYNC_REMOTE);
      if ( syncRemote && !m_activateSync )
      {
         if ( MDialog_YesNoDialog
              (
               _("You have activated remote configuration synchronisation.\n"
                 "\n"
                 "Do you want to store the current setup now?"),
               this,
               _("Store settings now?"),
               M_DLG_YES_DEFAULT,
               M_MSGBOX_OPT_STOREREMOTENOW
              )
            )
         {
            SaveRemoteConfigSettings();
         }
      }

#ifdef OS_WIN
      String filenameConfig = READ_CONFIG_TEXT(m_Profile, MP_USE_CONFIG_FILE);
      const int usingConfigFile = !filenameConfig.empty();

      if ( usingConfigFile != m_usingConfigFile )
      {
         String title(usingConfigFile ? _("Export settings?")
                                      : _("Import settings?"));

         String msg(_("You have changed the config file option.\n\n"));
         if ( usingConfigFile )
            msg += _("Do you want to export the registry settings "
                     "to config file?");
         else
            msg += _("Do you want to import the existing contents of config "
                     "file to the reigstry?");

         msg << _T('\n')
             << _("Note that this may overwrite the existing settings.");

         if ( MDialog_YesNoDialog
              (
                  msg,
                  this,
                  title,
                  M_DLG_YES_DEFAULT
              ) )
         {
            ConfigSource_obj
               configSrc(ConfigSourceLocal::CreateRegistry()),
               configDst(ConfigSourceLocal::CreateFile(filenameConfig));
            if ( !usingConfigFile )
            {
               // importing to registry, not exporting from it
               configSrc.Swap(configDst);
            }

            if ( !ConfigSource::Copy(*configDst.Get(), *configSrc.Get()) )
            {
               if ( usingConfigFile )
                  wxLogError(_("Failed to export settings to the file \"%s\"."),
                             filenameConfig.c_str());
               else
                  wxLogError(_("Failed to import settings from the file \"%s\"."),
                             filenameConfig.c_str());
            }
         }
      }
#endif // OS_WIN
   }

   return rc;
}

void wxOptionsPageSync::OnButton(wxCommandEvent& event)
{
   wxObject *obj = event.GetEventObject();
   bool save = obj == GetControl(ConfigField_SyncStore);
   if ( !save && (obj != GetControl(ConfigField_SyncRetrieve)) )
   {
      // nor save nor restore, not interesting
      event.Skip();

      return;
   }

   // maybe the user changed the folder setting but didn't press [Apply] ye
   // - still use the current setting, not the one from config
   String foldernameOld = READ_APPCONFIG(MP_SYNC_FOLDER);

   wxTextCtrl *text = wxStaticCast(GetControl(ConfigField_RSConfigFolder),
                                   wxTextCtrl);
   String foldername = text->GetValue();

   if ( foldername != foldernameOld )
   {
      mApplication->GetProfile()->writeEntry(MP_SYNC_FOLDER, foldername);
   }

   // false means don't ask confirmation
   bool ok = save ? SaveRemoteConfigSettings(false)
                  : RetrieveRemoteConfigSettings(false);
   if ( !ok )
   {
      if ( save )
      {
         wxLogError(_("Failed to save remote configuration to '%s'"),
                    foldername.c_str());
      }
      else // restoring
      {
         wxLogError(_("Failed to retrieve remote configuration from '%s'"),
                    foldername.c_str());
      }
   }
   else // ok
   {
      if ( save )
      {
         wxLogMessage(_("Successfully saved remote configuration to '%s'."),
                      foldername.c_str());
      }
      else // restoring
      {
         wxLogMessage(_("Successfully restored remote configuration from '%s'."),
                      foldername.c_str());
      }
   }

   // restore old value regardless
   if ( foldername != foldernameOld )
   {
      mApplication->GetProfile()->writeEntry(MP_SYNC_FOLDER, foldernameOld);
   }
}

// ----------------------------------------------------------------------------
// wxOptionsPageTest
// ----------------------------------------------------------------------------

#ifdef USE_TEST_PAGE

wxOptionsPageTest::wxOptionsPageTest(wxNotebook *parent,
                                     Profile *profile)
                 : wxOptionsPageStandard(parent,
                                         _T("Test page"),
                                         profile,
                                         ConfigField_TestFirst,
                                         ConfigField_TestLast)
{
}

#endif // USE_TEST_PAGE

// ----------------------------------------------------------------------------
// wxOptionsPageOthers
// ----------------------------------------------------------------------------

wxOptionsPageOthers::wxOptionsPageOthers(wxNotebook *parent,
                                         Profile *profile)
                   : wxOptionsPageStandard(parent,
                                   _("Miscellaneous"),
                                   profile,
                                   ConfigField_OthersFirst,
                                   ConfigField_OthersLast,
                                   MH_OPAGE_OTHERS)
{
   m_nAutosaveDelay =
   m_nAutoAwayDelay = -1;
}

void wxOptionsPageOthers::OnButton(wxCommandEvent& event)
{
   wxObject *obj = event.GetEventObject();
   if ( obj == GetControl(ConfigField_ReenableDialog) )
   {
      if ( ReenablePersistentMessageBoxes(this) )
      {
         wxOptionsEditDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsEditDialog);
         wxCHECK_RET( dialog, _T("options page without a parent dialog?") );
         dialog->SetDirty();
      }
   }
   else
   {
      event.Skip();
   }
}

bool wxOptionsPageOthers::TransferDataToWindow()
{
   // if the user checked "don't ask me again" checkbox in the message box
   // these setting might be out of date - synchronize

   // TODO this should be table based too probably...
   m_Profile->writeEntry(GetPersMsgBoxName(M_MSGBOX_CONFIRM_EXIT),
                           !wxPMessageBoxIsDisabled(MP_CONFIRMEXIT));

   bool rc = wxOptionsPage::TransferDataToWindow();
   if ( rc )
   {
      m_nAutosaveDelay = READ_CONFIG(m_Profile, MP_AUTOSAVEDELAY);
      m_nAutoAwayDelay = READ_CONFIG(m_Profile, MP_AWAY_AUTO_ENTER);
   }

   return rc;
}

bool wxOptionsPageOthers::TransferDataFromWindow()
{
   // read the old value before calling the base class TransferDataFromWindow()
   // whieh may change it
   wxString logfileOld = READ_CONFIG(m_Profile, MP_LOGFILE);

   bool rc = wxOptionsPage::TransferDataFromWindow();
   if ( rc )
   {
      // now if the user checked "confirm exit" checkbox we must reenable
      // the message box by erasing the stored answer to it
      if ( READ_CONFIG_BOOL(m_Profile, MP_CONFIRMEXIT) )
         wxPMessageBoxEnable(GetPersMsgBoxName(M_MSGBOX_CONFIRM_EXIT));

      // restart the timer if the timeout changed
      long delayNew = READ_CONFIG(m_Profile, MP_AUTOSAVEDELAY);
      if ( delayNew != m_nAutosaveDelay )
      {
         if ( !mApplication->RestartTimer(MAppBase::Timer_Autosave) )
         {
            wxLogError(_("Invalid delay value for autosave timer."));

            rc = false;
         }
      }

      // and the other timer too
      delayNew = READ_CONFIG(m_Profile, MP_AWAY_AUTO_ENTER);
      if ( delayNew != m_nAutoAwayDelay )
      {
         if ( !mApplication->RestartTimer(MAppBase::Timer_Away) )
         {
            wxLogError(_("Invalid delay value for auto away timer."));

            rc = false;
         }
      }

      // show/hide the log window depending on the new setting value
      bool showLog = READ_CONFIG_BOOL(m_Profile, MP_SHOWLOG);
      if ( showLog != mApplication->IsLogShown() )
      {
         mApplication->ShowLog(showLog);
      }

      // has the logfile value changed?
      wxString logfileNew = READ_CONFIG(m_Profile, MP_LOGFILE);
      if ( logfileNew != logfileOld )
      {
         // yes, redirect the logging to the new logfile
         mApplication->SetLogFile(logfileNew);
      }
   }

   return rc;
}

// ----------------------------------------------------------------------------
// wxOptionsPageHelpers
// ----------------------------------------------------------------------------

wxOptionsPageHelpers::wxOptionsPageHelpers(wxNotebook *parent,
                                         Profile *profile)
   : wxOptionsPageStandard(parent,
                   _("Helpers"),
                   profile,
                   ConfigField_HelpersFirst,
                   ConfigField_HelpersLast,
                   MH_OPAGE_HELPERS)
{
}

// ----------------------------------------------------------------------------
// wxOptionsPageFolders
// ----------------------------------------------------------------------------

wxOptionsPageFolders::wxOptionsPageFolders(wxNotebook *parent,
                                           Profile *profile)
   : wxOptionsPageStandard(parent,
                   _("Folders"),
                   profile,
                   ConfigField_FoldersFirst,
                   ConfigField_FoldersLast,
                   MH_OPAGE_FOLDERS)
{
   m_lboxData = new LboxData;
   m_lboxData->m_idListbox = ConfigField_OpenFolders;
   m_lboxData->m_lboxDlgTitle = _("Folders to open on startup");
   m_lboxData->m_lboxDlgPrompt = _("Folder name");
   m_lboxData->m_lboxDlgPers = _T("LastStartupFolder");
}

bool wxOptionsPageFolders::TransferDataToWindow()
{
   bool bRc = wxOptionsPage::TransferDataToWindow();

   if ( bRc )
   {
      // we add the folder opened in the main frame to the list of folders
      // opened on startup if it's not yet among them
      wxControl *control = GetControl(ConfigField_OpenFolders);
      if ( control )
      {
         wxListBox *listbox = wxStaticCast(control, wxListBox);
         wxString strMain = GetControlText(ConfigField_MainFolder);
         int n = listbox->FindString(strMain);
         if ( n == -1 )
         {
            listbox->Append(strMain);
         }
      }
      //else: the listbox is not created in this dialog at all
   }

   return bRc;
}

bool wxOptionsPageFolders::TransferDataFromWindow()
{
   // undo what we did in TransferDataToWindow: remove the main folder from
   // the list of folders to be opened on startup
   wxControl *control = GetControl(ConfigField_OpenFolders);
   if ( control )
   {
      wxListBox *listbox = wxStaticCast(control, wxListBox);
      wxString strMain = GetControlText(ConfigField_MainFolder);
      int n = listbox->FindString(strMain);
      if ( n != -1 )
      {
         listbox->Delete(n);
      }
   }

   bool rc = wxOptionsPage::TransferDataFromWindow();

   if ( rc )
   {
      // update the frame title/status bar if needed
      if ( IsDirty(ConfigField_StatusFormat_StatusBar) ||
           IsDirty(ConfigField_StatusFormat_TitleBar) )
      {
         // TODO: send the folder status change event
      }
   }

   return rc;
}

void wxOptionsPageFolders::OnAddFolder(wxCommandEvent& event)
{
   wxListBox *lbox;
   LboxData *data;
   if ( !GetListboxFromButtonEvent(event, &lbox, &data) )
   {
      return;
   }

   // show the folder selection dialog instead of using the default text entry
   // dialog shown by the base wxOptionsPage class
   MFolder *folder = MDialog_FolderChoose(this);
   if ( !folder || folder->GetType() == MF_GROUP || folder->GetType() == MF_ROOT )
   {
      // cancelled by user or group or root ("All folders") selected
      return;
   }

   lbox->Append(folder->GetFullName());

   folder->DecRef();

   wxOptionsPage::OnChangeCommon(lbox);
}

// ----------------------------------------------------------------------------
// wxGlobalOptionsDialog
// ----------------------------------------------------------------------------

wxGlobalOptionsDialog::wxGlobalOptionsDialog(wxFrame *parent, const wxString& configKey)
               : wxOptionsEditDialog(parent, _("Program options"), configKey)
{
}

bool
wxGlobalOptionsDialog::TransferDataToWindow()
{
   if ( !wxOptionsEditDialog::TransferDataToWindow() )
      return FALSE;

   int nPageCount = m_notebook->GetPageCount();
   for ( int nPage = 0; nPage < nPageCount; nPage++ ) {
      ((wxOptionsPage *)m_notebook->GetPage(nPage))->UpdateUI();
   }

   return TRUE;
}

void wxGlobalOptionsDialog::CreateNotebook(wxPanel *panel)
{
   m_notebook = new wxOptionsNotebook(panel);
}

wxGlobalOptionsDialog::~wxGlobalOptionsDialog()
{
   // save settings
   Profile::FlushAll();
}

// ----------------------------------------------------------------------------
// wxCustomOptionsNotebook is a notebook which contains the given page
// ----------------------------------------------------------------------------

wxCustomOptionsNotebook::wxCustomOptionsNotebook
                         (
                          wxWindow *parent,
                          size_t nPages,
                          const wxOptionsPageDesc *pageDesc,
                          const wxString& configForNotebook,
                          Profile *profile
                         )
                       : wxNotebookWithImages(
                                              configForNotebook,
                                              parent,
                                              GetImagesArray(nPages, pageDesc)
                                             )
{
   // use the global profile by default
   if ( profile )
      profile->IncRef();
   else
      profile = Profile::CreateProfile(_T(""));


   for ( size_t n = 0; n < nPages; n++ )
   {
      // the page ctor will add it to the notebook
      const wxOptionsPageDesc& desc = pageDesc[n];
      wxOptionsPageDynamic *page = new wxOptionsPageDynamic(
                                                            this,
                                                            wxGetTranslation(desc.title),
                                                            profile,
                                                            desc.aFields,
                                                            desc.aDefaults,
                                                            desc.nFields,
                                                            desc.nOffset,
                                                            desc.helpId,
                                                            n  // image index
                                                           );
      page->Layout();
   }

   profile->DecRef();
}

// return the array which should be passed to wxNotebookWithImages ctor
const wxChar **
wxCustomOptionsNotebook::GetImagesArray(size_t nPages,
                                        const wxOptionsPageDesc *pageDesc)
{
   m_aImages = new const wxChar *[nPages + 1];

   for ( size_t n = 0; n < nPages; n++ )
   {
      m_aImages[n] = pageDesc[n].image;
   }

   m_aImages[nPages] = NULL;

   return m_aImages;
}

// ----------------------------------------------------------------------------
// wxOptionsNotebook manages its own image list
// ----------------------------------------------------------------------------

// should be in sync with the enum OptionsPage in wxOptionsDlg.h!
const wxChar *wxOptionsNotebook::ms_aszImages[] =
{
   _T("ident"),
   _T("network"),
   _T("newmail"),
   _T("compose"),
   _T("folders"),
#ifdef USE_PYTHON
   _T("python"),
#endif
   _T("msgview"),
   _T("folderview"),
   _T("foldertree"),
   _T("adrbook"),
   _T("helpers"),
   _T("sync"),
#ifdef USE_TEST_PAGE
   _T("unknown"),
#endif // USE_TEST_PAGE
   _T("miscopt"),
   NULL
};

// don't forget to update both the array above and the enum when modifying
// either of them!
wxCOMPILE_TIME_ASSERT(
      WXSIZEOF(wxOptionsNotebook::ms_aszImages) == OptionsPage_Max + 1,
      MismatchImagesAndPages
   );

// create the control and add pages too
wxOptionsNotebook::wxOptionsNotebook(wxWindow *parent)
                 : wxNotebookWithImages(_T("OptionsNotebook"), parent, ms_aszImages)
{
   Profile *profile = GetProfile();

   // create and add the pages
   new wxOptionsPageIdent(this, profile);
   new wxOptionsPageNetwork(this, profile);
   new wxOptionsPageNewMail(this, profile);
   new wxOptionsPageCompose(this, profile);
   new wxOptionsPageFolders(this, profile);
#ifdef USE_PYTHON
   new wxOptionsPagePython(this, profile);
#endif
   new wxOptionsPageMessageView(this, profile);
   new wxOptionsPageFolderView(this, profile);
   new wxOptionsPageFolderTree(this, profile);
   new wxOptionsPageAdb(this, profile);
   new wxOptionsPageHelpers(this, profile);
   new wxOptionsPageSync(this, profile);
#ifdef USE_TEST_PAGE
   new wxOptionsPageTest(this, profile);
#endif // USE_TEST_PAGE
   new wxOptionsPageOthers(this, profile);

   profile->DecRef();
}

// ----------------------------------------------------------------------------
// wxIdentityOptionsDialog
// ----------------------------------------------------------------------------

void wxIdentityOptionsDialog::CreatePagesDesc()
{
   // TODO: it would be better to have custom pages here as not all settings
   //       make sense for the identities, but it's easier to use the standard
   //       ones

   size_t nOffset;
   m_nPages = 3;
   m_aPages = new wxOptionsPageDesc[3];

   // identity page
   nOffset = ConfigField_IdentFirst + 1;
   m_aPages[0] = wxOptionsPageDesc
   (
      _("Identity"),
      wxOptionsNotebook::ms_aszImages[OptionsPage_Ident],
      MH_OPAGE_IDENT,
      wxOptionsPageStandard::ms_aFields + nOffset,
      wxOptionsPageStandard::ms_aConfigDefaults + nOffset,
      ConfigField_IdentLast - ConfigField_IdentFirst,
      nOffset
   );

   // network page
   nOffset = ConfigField_NetworkFirst + 1;
   m_aPages[1] = wxOptionsPageDesc
   (
      _("Network"),
      wxOptionsNotebook::ms_aszImages[OptionsPage_Network],
      MH_OPAGE_NETWORK,
      wxOptionsPageStandard::ms_aFields + nOffset,
      wxOptionsPageStandard::ms_aConfigDefaults + nOffset,
      ConfigField_NetworkLast - ConfigField_NetworkFirst,
      nOffset
   );

   // compose page
   nOffset = ConfigField_ComposeFirst + 1;
   m_aPages[2] = wxOptionsPageDesc
   (
      _("Compose"),
      wxOptionsNotebook::ms_aszImages[OptionsPage_Compose],
      MH_OPAGE_COMPOSE,
      wxOptionsPageStandard::ms_aFields + nOffset,
      wxOptionsPageStandard::ms_aConfigDefaults + nOffset,
      ConfigField_ComposeLast - ConfigField_ComposeFirst,
      nOffset
   );
};

// ----------------------------------------------------------------------------
// wxRestoreDefaultsDialog implementation
// ----------------------------------------------------------------------------

wxRestoreDefaultsDialog::wxRestoreDefaultsDialog(Profile *profile,
                                                 wxFrame *parent)
                       : wxProfileSettingsEditDialog
                         (
                           profile,
                           _T("RestoreOptionsDlg"),
                           parent,
                           _("Restore default options")
                         )
{
   wxLayoutConstraints *c;

   // create the Ok and Cancel buttons in the bottom right corner
   wxStaticBox *box = CreateStdButtonsAndBox(_("&All settings"));

   // create a short help message above

   wxStaticText *msg = new wxStaticText
                           (
                              this, -1,
                              _("Please check the settings whose values\n"
                                "should be reset to the defaults.")
                           );
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   // create the checklistbox in the area which is left
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);

   m_checklistBox = new wxCheckListBox(this, -1);
   m_checklistBox->SetConstraints(c);

   // add the items to the checklistbox
   for ( size_t n = 0; n < ConfigField_Max; n++ )
   {
      switch ( wxOptionsPageStandard::GetStandardFieldType(n) )
      {
         case wxOptionsPage::Field_Message:
            // this is not a setting, just some help text
            continue;

         case wxOptionsPage::Field_SubDlg:
            // TODO inject in the checklistbox the settings of subdlg
            continue;

         case wxOptionsPage::Field_XFace:
            // TODO inject the settings of subdlg
            continue;

      default:
            break;
      }

      m_checklistBox->Append(
            wxStripMenuCodes(wxGetTranslation(wxOptionsPageStandard::ms_aFields[n].label)));
   }

   // set the initial and minimal size
   SetDefaultSize(4*wBtn, 10*hBtn, FALSE /* not minimal size */);
}

bool wxRestoreDefaultsDialog::TransferDataFromWindow()
{
   // delete the values of all selected settings in this profile - this will
   // effectively restore their default values
#if wxCHECK_VERSION(2, 3, 2)
   size_t count = (size_t)m_checklistBox->GetCount();
#else
   size_t count = (size_t)m_checklistBox->Number();
#endif
   for ( size_t n = 0; n < count; n++ )
   {
      if ( m_checklistBox->IsChecked(n) )
      {
         MarkDirty();

         GetProfile()->GetConfig()->DeleteEntry(
               wxOptionsPageStandard::ms_aConfigDefaults[n].name);
      }
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

void ShowOptionsDialog(wxFrame *parent, OptionsPage page)
{
   wxGlobalOptionsDialog dlg(parent);
   dlg.CreateAllControls();
   dlg.SetNotebookPage(page);
   dlg.Layout();
   (void)dlg.ShowModal();
}

bool ShowRestoreDefaultsDialog(Profile *profile, wxFrame *parent)
{
   wxRestoreDefaultsDialog dlg(profile, parent);
   (void)dlg.ShowModal();

   return dlg.HasChanges();
}

bool ShowCustomOptionsDialog(size_t nPages,
                             const wxOptionsPageDesc *pageDesc,
                             Profile *profile,
                             wxFrame *parent)
{
   wxCustomOptionsDialog dlg(nPages, pageDesc, profile, parent);
   dlg.CreateAllControls();
   dlg.Layout();

   return dlg.ShowModal() == wxID_OK;
}

void ShowIdentityDialog(const wxString& identity, wxFrame *parent)
{
   wxIdentityOptionsDialog dlg(identity, parent);
   dlg.CreateAllControls();
   dlg.Layout();

   (void)dlg.ShowModal();
}
