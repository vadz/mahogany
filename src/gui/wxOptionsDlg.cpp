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

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "guidef.h"
#  include "MHelp.h"

#  include <wx/dynarray.h>
#  include <wx/checkbox.h>
#  include <wx/listbox.h>
#  include <wx/radiobox.h>
#  include <wx/statbox.h>
#  include <wx/stattext.h>
#  include <wx/statbmp.h>
#  include <wx/textctrl.h>

#  include <wx/utils.h>    // for wxStripMenuCodes
#endif

#include <wx/log.h>
#include <wx/imaglist.h>
#include <wx/notebook.h>
#include <wx/resource.h>
#include <wx/confbase.h>

#include <wx/menuitem.h>
#include <wx/checklst.h>

#include <wx/layout.h>

#include "wx/persctrl.h"

#include "MDialogs.h"
#include "Mdefaults.h"
#include "Mupgrade.h"
#include "Mcallbacks.h"

#include "gui/wxIconManager.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxOptionsPage.h"

#include "HeadersDialogs.h"
#include "TemplateDialog.h"

// ----------------------------------------------------------------------------
// conditional compilation
// ----------------------------------------------------------------------------

// define this to have additional TCP parameters in the options dialog
#undef USE_TCP_TIMEOUTS

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
   ConfigField_UsernameHelp,
   ConfigField_Username,
   ConfigField_HostnameHelp,
   ConfigField_Hostname,
   ConfigField_AddDefaultHostname,
   ConfigField_ReturnAddress,
   ConfigField_SetReplyFromTo,
   ConfigField_PersonalName,
   ConfigField_VCardHelp,
   ConfigField_UseVCard,
   ConfigField_VCardFile,
   ConfigField_UserLevelHelp,
   ConfigField_UserLevel,
   ConfigField_IdentLast = ConfigField_UserLevel,

   // network
   ConfigField_NetworkFirst = ConfigField_IdentLast,
   ConfigField_ServersHelp,
   ConfigField_PopServer,
   ConfigField_ImapServer,
   ConfigField_MailServer,
   ConfigField_NewsServer,
   ConfigField_MailServerLoginHelp,
   ConfigField_MailServerLogin,
   ConfigField_MailServerPassword,
   ConfigField_NewsServerLogin,
   ConfigField_NewsServerPassword,
#ifdef USE_SSL
   ConfigField_SSLtext,
   ConfigField_PopServerSSL,
   ConfigField_NewsServerSSL,
#endif
   ConfigField_DialUpHelp,
   ConfigField_DialUpSupport,
   ConfigField_BeaconHost,
#ifdef OS_WIN
   ConfigField_NetConnection,
#elif defined(OS_UNIX)
   ConfigField_NetOnCommand,
   ConfigField_NetOffCommand,
#endif // platform
   ConfigField_TimeoutInfo,
   ConfigField_OpenTimeout,
#ifdef USE_TCP_TIMEOUTS
   ConfigField_ReadTimeout,
   ConfigField_WriteTimeout,
   ConfigField_CloseTimeout,
   ConfigField_RshTimeout,
   ConfigField_NetworkLast = ConfigField_RshTimeout,
#endif // USE_TCP_TIMEOUTS
   ConfigField_NetworkLast = ConfigField_OpenTimeout,

   // compose
   ConfigField_ComposeFirst = ConfigField_NetworkLast,
   ConfigField_UseOutgoingFolder,
   ConfigField_OutgoingFolder,
   ConfigField_WrapMargin,
   ConfigField_WrapAuto,
   ConfigField_ReplyString,
   ConfigField_ReplyCollapse,
   ConfigField_ReplyCharacter,

   ConfigField_Signature,
   ConfigField_SignatureFile,
   ConfigField_SignatureSeparator,
   ConfigField_XFaceFile,
   ConfigField_AdbSubstring,
   ConfigField_ComposeViewFontFamily,
   ConfigField_ComposeViewFontSize,
   ConfigField_ComposeViewFGColour,
   ConfigField_ComposeViewBGColour,

   ConfigField_ComposeHeaders,
   ConfigField_ComposeTemplates,

   ConfigField_ComposeLast = ConfigField_ComposeTemplates,

   // folders
   ConfigField_FoldersFirst = ConfigField_ComposeLast,
   ConfigField_OpenFolders,
   ConfigField_MainFolder,
// ConfigField_NewMailFolder,
   ConfigField_PollIncomingDelay,
   ConfigField_UpdateInterval,
   ConfigField_FolderProgressThreshold,
   ConfigField_AutoShowFirstMessage,
   ConfigField_UseOutbox,
   ConfigField_OutboxName,
   ConfigField_UseTrash,
   ConfigField_TrashName,
   ConfigField_FoldersFileFormat,
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
   ConfigField_MessageViewPreviewOnSelect,
   ConfigField_MessageViewFontFamily,
   ConfigField_MessageViewFontSize,
   ConfigField_MessageViewFGColour,
   ConfigField_MessageViewBGColour,
   ConfigField_MessageViewUrlColour,
   ConfigField_MessageViewHeaderNamesColour,
   ConfigField_MessageViewHeaderValuesColour,
   ConfigField_MessageViewInlineGraphics,
   ConfigField_MessageViewRfc822IsText,
#ifdef OS_UNIX
   ConfigField_MessageViewFaxSupport,
   ConfigField_MessageViewFaxDomains,
#endif // Unix
   ConfigField_FolderViewHelpText,
   ConfigField_FolderViewFontFamily,
   ConfigField_FolderViewOnlyNames,
   ConfigField_FolderViewFontSize,
   ConfigField_FolderViewFGColour,
   ConfigField_FolderViewBGColour,
   ConfigField_FolderViewNewColour,
   ConfigField_FolderViewRecentColour,
   ConfigField_FolderViewUnreadColour,
   ConfigField_FolderViewDeletedColour,
   ConfigField_MessageViewMaxHelpText,
   ConfigField_MessageViewMaxMsgSize,
   ConfigField_MessageViewMaxHeadersNum,
   ConfigField_MessageViewHeaders,
   ConfigField_MessageViewSortMessagesBy,
   ConfigField_MessageViewDateFormat,
   ConfigField_MessageViewLast = ConfigField_MessageViewDateFormat,

   // autocollecting addresses options
   ConfigField_AdbFirst = ConfigField_MessageViewLast,
   ConfigField_AutoCollect_HelpText,
   ConfigField_AutoCollect,
   ConfigField_AutoCollectAdb,
   ConfigField_AutoCollectNameless,
   ConfigField_Bbdb_HelpText,
   ConfigField_Bbdb_IgnoreAnonymous,
   ConfigField_Bbdb_GenerateUnique,
   ConfigField_Bbdb_AnonymousName,
   ConfigField_Bbdb_SaveOnExit,
   ConfigField_AdbLast = ConfigField_Bbdb_SaveOnExit,

   // helper programs
   ConfigField_HelpersFirst = ConfigField_AdbLast,
   ConfigField_HelpersHelp1,
   ConfigField_Browser,
   ConfigField_BrowserIsNetscape,
   ConfigField_HelpersHelp2,
   ConfigField_HelpBrowser,
   ConfigField_HelpBrowserIsNetscape,
   ConfigField_HelpExternalEditor,
   ConfigField_ExternalEditor,
   ConfigField_AutoLaunchExtEditor,
#ifdef OS_UNIX
   ConfigField_ImageConverter,
   ConfigField_ConvertGraphicsFormat,
#endif
   ConfigField_HelpNewMailCommand,
   ConfigField_NewMailCommand,
   ConfigField_HelpersLast = ConfigField_NewMailCommand,

   // other options
   ConfigField_OthersFirst = ConfigField_HelpersLast,
   ConfigField_ShowLog,
   ConfigField_ShowTips,
   ConfigField_Splash,
   ConfigField_SplashDelay,
   ConfigField_AutosaveHelp,
   ConfigField_AutosaveDelay,
   ConfigField_ConfirmExit,
   ConfigField_OpenOnClick,
   ConfigField_ShowHiddenFolders,
   ConfigField_ShowNewMail,
   ConfigField_DebugCClient,
#ifdef USE_SSL
   ConfigField_SslHelp,
   ConfigField_SslDllName,
   ConfigField_CryptoDllName,
#endif
#ifdef OS_UNIX
   ConfigField_AFMPath,
   ConfigField_FocusFollowsMouse,
#endif
   ConfigField_DockableMenubars,
   ConfigField_DockableToolbars,
   ConfigField_ToolbarsFlatButtons,
   ConfigField_ReenableDialog,
   ConfigField_OthersLast = ConfigField_ReenableDialog,

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
   static const char *ms_aszImages[];

   wxOptionsNotebook(wxWindow *parent);

   // the profile we use - just the global one here
   Profile *GetProfile() const { return Profile::CreateProfile(""); }
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
   const char **GetImagesArray(size_t nPages, const wxOptionsPageDesc *pageDesc);

   // the images names and NULL
   const char **m_aImages;
};

// -----------------------------------------------------------------------------
// dialog classes
// -----------------------------------------------------------------------------

class wxOptionsDialog : public wxNotebookDialog
{
public:
   wxOptionsDialog(wxFrame *parent, const wxString& configKey = "OptionsDlg");

   ~wxOptionsDialog();

   // notifications from the notebook pages
      // something important change
   void SetDoTest() { SetDirty(); m_bTest = TRUE; }
      // some setting changed, but won't take effect until restart
   void SetGiveRestartWarning() { m_bRestartWarning = TRUE; }

   // override base class functions
   virtual void CreateNotebook(wxPanel *panel);
   virtual bool TransferDataToWindow();

   virtual bool OnSettingsChange();

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxOptionsDialog() { wxFAIL_MSG("should be never used"); }

protected:
   // unset the dirty flag
   virtual void ResetDirty();

   // implement base class pure virtual
   virtual Profile *GetProfile() const
   {
      return ((wxOptionsNotebook *)m_notebook)->GetProfile();
   }

private:
   bool  m_bTest,            // test new settings?
         m_bRestartWarning;  // changes will take effect after restart

   DECLARE_DYNAMIC_CLASS(wxOptionsDialog)
};

// just like wxOptionsDialog but uses the given wxOptionsPage and not the
// standard ones
class wxCustomOptionsDialog : public wxOptionsDialog
{
public:
   // minimal ctor, use SetPagesDesc() and SetProfile() later
   wxCustomOptionsDialog(wxFrame *parent,
                         const wxString& configForDialog = "CustomOptions",
                         const wxString& configForNotebook = "CustomNotebook")
      : wxOptionsDialog(parent, configForDialog),
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
                         const wxString& configForDialog = "CustomOptions",
                         const wxString& configForNotebook = "CustomNotebook")
      : wxOptionsDialog(parent, configForDialog),
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
      m_notebook = new wxCustomOptionsNotebook(this,
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
};

// an identity edit dialog: works with settings in an identity profile, same as
// wxOptionsDialog otherwise
class wxIdentityOptionsDialog : public wxCustomOptionsDialog
{
public:
   wxIdentityOptionsDialog(const wxString& identity, wxFrame *parent)
      : wxCustomOptionsDialog(parent, "IdentDlg", "IdentNotebook"),
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
   }

   virtual wxControl *CreateControlsAbove(wxPanel *panel);

   virtual ~wxIdentityOptionsDialog()
   {
      delete [] m_aPages;
   }

private:
   // create our pages desc: do it dynamically because they may depend on the
   // user level (which can change) in the future
   void CreatePagesDesc();

   // the identity which we edit
   wxString m_identity;

   // the array of descriptions of our pages
   size_t m_nPages;
   wxOptionsPageDesc *m_aPages;
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
};

// ----------------------------------------------------------------------------
// event tables and such
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxOptionsDialog, wxNotebookDialog)

BEGIN_EVENT_TABLE(wxOptionsPage, wxNotebookPageBase)
   // any change should make us dirty
   EVT_CHECKBOX(-1, wxOptionsPage::OnControlChange)
   EVT_RADIOBOX(-1, wxOptionsPage::OnControlChange)
   EVT_COMBOBOX(-1, wxOptionsPage::OnControlChange)
   EVT_TEXT(-1, wxOptionsPage::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageCompose, wxOptionsPage)
   // buttons invoke subdialogs
   EVT_BUTTON(-1, wxOptionsPageCompose::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageMessageView, wxOptionsPage)
   // buttons invoke subdialogs
   EVT_BUTTON(-1, wxOptionsPageMessageView::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageFolders, wxOptionsPage)
   EVT_BUTTON(-1, wxOptionsPageFolders::OnButton)

   EVT_IDLE(wxOptionsPageFolders::OnIdle)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageOthers, wxOptionsPage)
   EVT_BUTTON(-1, wxOptionsPageOthers::OnButton)
END_EVENT_TABLE()

// ============================================================================
// data: both of these arrays *must* be in sync with ConfigFields enum!
// ============================================================================

// the labels of all fields, their types and also the field they "depend on"
// (being dependent on another field only means that if that field is disabled
//  or unchecked, we're disabled too)
//
// if you modify this array, search for DONT_FORGET_TO_MODIFY and modify data
// there too
const wxOptionsPage::FieldInfo wxOptionsPageStandard::ms_aFields[] =
{
   // network config and identity
   { gettext_noop("The user and host names are used to compose "
                  "the return address, unless\n"
                  "a different return address is explicitly specified."),
                                                   Field_Message, -1 },
   { gettext_noop("&Username"),                    Field_Text,    -1 },
   { gettext_noop("The host name is also used as a default host "
                  "name for local mail addresses."),
                                                   Field_Message, -1 },
   { gettext_noop("&Hostname"),                    Field_Text | Field_Vital,   -1, },
   { gettext_noop("&Add this hostname if none specified"), Field_Bool, -1 },
   { gettext_noop("&Return/Reply address"),        Field_Text | Field_Vital,   -1, },
   { gettext_noop("Reply return address from &To: field"), Field_Bool, -1, },
   { gettext_noop("&Personal name"),               Field_Text,    -1,                        },
   { gettext_noop(
      "You may want to attach your personal information card (vCard)\n"
      "to all outoing messages. In this case you will need to specify\n"
      "a file containing it.\n"
      "Notice that such file can be created by exporting an address\n"
      "book entry in the vCard format."), Field_Message, -1 },
   { gettext_noop("Attach a v&Card to outgoing messages"), Field_Bool,    -1,                        },
   { gettext_noop("&vCard file"),                  Field_File, ConfigField_UseVCard,                        },
   { gettext_noop("Some more rarely used and less obvious program\n"
                  "features are only accessible in the so-called\n"
                  "`advanced' user mode which may be set here."),
                                                   Field_Message,   -1,                        },
   { gettext_noop("User &level:novice:advanced"),  Field_Combo,   -1,                        },

   // network
   { gettext_noop("The following fields are used as default values for the\n"
                  "corresponding server names. You may set them independently\n"
                  "for each folder as well, overriding the values specified "
                  "here"),                         Field_Message, -1,                        },
   { gettext_noop("&POP server"),                  Field_Text,    -1,                        },
   { gettext_noop("&IMAP server"),                 Field_Text,    -1,                        },
   { gettext_noop("SMTP (&mail) server"),          Field_Text | Field_Vital,   -1,           },
   { gettext_noop("NNTP (&news) server"),          Field_Text,    -1,
   },
   { gettext_noop("Some SMTP or NNTP servers require a user Id or login.\n"
                  "Leave these fields empty unless told to set it up by your ISP."),
     Field_Message, -1,                        },
   { gettext_noop("SMTP server &user ID"),         Field_Text,   -1,           },
   { gettext_noop("SMTP server pa&ssword"),        Field_Passwd, ConfigField_MailServerLogin,           },
   { gettext_noop("NNTP server user &ID"),         Field_Text,   -1,           },
   { gettext_noop("NNTP server pass&word"),        Field_Passwd, ConfigField_NewsServerLogin,           },
#ifdef USE_SSL
   { gettext_noop("Mahogany can attempt to use SSL (secure sockets layer) to send\n"
                  "mail or news. Tick the following boxes to activate this.")
     , Field_Message, -1 },
   { gettext_noop("POP server uses SS&L"), Field_Bool,    -1,                        },
   { gettext_noop("IMAP s&erver uses SSL"),Field_Bool,    -1,                        },
#endif
   { gettext_noop("Mahogany contains support for dial-up networks and can detect if the\n"
                  "network is up or not. It can also be used to connect and disconnect the\n"
                  "network. To aid in detecting the network status, you can specify a beacon\n"
                  "host which should only be reachable if the network is up, e.g. the WWW\n"
                  "server of your ISP. Leave it empty to use the SMTP server for this.")
     , Field_Message, -1 },
   { gettext_noop("&Dial-up network support"),    Field_Bool,    -1,                        },
   { gettext_noop("&Beacon host (e.g. www.yahoo.com)"),Field_Text,   ConfigField_DialUpSupport},
#ifdef OS_WIN
   { gettext_noop("&RAS connection to use"),   Field_Combo, ConfigField_DialUpSupport},
#elif defined(OS_UNIX)
   { gettext_noop("Command to &activate network"),   Field_Text, ConfigField_DialUpSupport},
   { gettext_noop("Command to &deactivate network"), Field_Text, ConfigField_DialUpSupport},
#endif // platform
   { gettext_noop("The following timeout value is used for TCP connections to\n"
                  "remote mail or news servers."), Field_Message | Field_Advanced, -1 },
   { gettext_noop("&Open timeout (in seconds)"),  Field_Number | Field_Advanced,    -1,                        },
#ifdef USE_TCP_TIMEOUTS
   { gettext_noop("&Read timeout"),                Field_Number | Field_Advanced,    -1,                        },
   { gettext_noop("&Write timeout"),               Field_Number | Field_Advanced,    -1,                        },
   { gettext_noop("&Close timeout"),               Field_Number | Field_Advanced,    -1,                        },
   { gettext_noop("&rsh timeout"),                 Field_Number | Field_Advanced,    -1,                        },
#endif // USE_TCP_TIMEOUTS

   // compose
   { gettext_noop("Sa&ve sent messages"),          Field_Bool,    -1,                        },
   { gettext_noop("&Folder file for sent messages"),
                                                   Field_File,    ConfigField_UseOutgoingFolder },
   { gettext_noop("&Wrap margin"),                 Field_Number,  -1,                        },
   { gettext_noop("Wra&p lines automatically"),    Field_Bool,  -1,                        },
   { gettext_noop("&Reply string in subject"),     Field_Text,    -1,                        },
   { gettext_noop("Co&llapse reply markers"
                  ":no:collapse:collapse & count"),            Field_Combo,   -1,                        },
   { gettext_noop("Quote &character"),             Field_Text,    -1,                        },

   { gettext_noop("&Use signature"),               Field_Bool,    -1,                        },
   { gettext_noop("&Signature file"),              Field_File,    ConfigField_Signature      },
   { gettext_noop("Use signature se&parator"),     Field_Bool,    ConfigField_Signature      },

   { gettext_noop("Configure &XFace..."),                  Field_XFace,  -1          },
   { gettext_noop("Mail alias substring ex&pansion"),
                                                   Field_Bool,    -1,                        },
   { gettext_noop("Font famil&y"
                  ":default:decorative:roman:script:swiss:modern:teletype"),
                                                   Field_Combo,   -1},
   { gettext_noop("Font si&ze"),                   Field_Number,  -1},
   { gettext_noop("Foreground c&olour"),           Field_Color,   -1},
   { gettext_noop("Back&ground colour"),           Field_Color,   -1},

   { gettext_noop("Configure &headers..."),        Field_SubDlg,  -1},
   { gettext_noop("Configure &templates..."),      Field_SubDlg,  -1},

   // folders
   { gettext_noop("Folders to open on &startup"),  Field_List |
                                                   Field_Restart, -1,           },
   { gettext_noop("Folder opened in &main frame"), Field_Text,    -1,                        },
// { gettext_noop("Folder where to collect &new mail"), Field_Text, -1},
   { gettext_noop("Poll for &new mail interval in seconds"), Field_Number, -1},
   { gettext_noop("&Ping/check folder interval in seconds"), Field_Number, -1},
   { gettext_noop("&Automatically select first message in viewer"), Field_Bool, -1},
   { gettext_noop("&Threshold for displaying progress dialog"), Field_Number, -1},
   { gettext_noop("Send outgoing messages later"), Field_Bool, -1 },
   { gettext_noop("Folder for &outgoing messages"), Field_Text, ConfigField_UseOutbox },
   { gettext_noop("Move &deleted messages to Trash folder"), Field_Bool, -1},
   { gettext_noop("&Trash folder name"), Field_Text, ConfigField_UseTrash},
   { gettext_noop("Default format for mailbox files"
      ":Unix mbx mailbox:Unix mailbox:MMDF (SCO Unix):Tenex (Unix MM format)"),
     Field_Combo, -1},
   { gettext_noop("You can specify the format for the strings shown in the\n"
                  "status and title bars. Use %f for the folder name and\n"
                  "%t, %r and %n for the number of all, recent and new\n"
                  "messages respectively."), Field_Message, -1 },
   { gettext_noop("Status &bar format"), Field_Text, -1 },
   { gettext_noop("T&itle bar format"), Field_Text, -1 },

#ifdef USE_PYTHON
   // python
   { gettext_noop("Python is the built-in scripting language which can be\n")
     gettext_noop("used to extend Mahogany's functionality. It is not essential\n")
     gettext_noop("for the program's normal operation."), Field_Message, -1},
   { gettext_noop("&Enable Python"),               Field_Bool,    -1,                        },
   { gettext_noop("Python &Path"),                 Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("&Startup script"),              Field_File,    ConfigField_EnablePython   },
   { gettext_noop("&Folder open callback"),        Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Folder &update callback"),      Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Folder e&xpunge callback"),     Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Flag &set callback"),           Field_Text,    ConfigField_EnablePython   },
   { gettext_noop("Flag &clear callback"),         Field_Text,    ConfigField_EnablePython   },
#endif // USE_PYTHON

   // message views:
   { gettext_noop("Preview message when &selected"), Field_Bool,    -1 },
   { gettext_noop("&Font family"
                  ":default:decorative:roman:script:swiss:modern:teletype"),
                                                   Field_Combo,   -1 },
   { gettext_noop("Font si&ze"),                   Field_Number,  -1 },
   { gettext_noop("Foreground c&olour"),           Field_Color,   -1 },
   { gettext_noop("Back&ground colour"),           Field_Color,   -1 },
   { gettext_noop("Colour for &URLs"),             Field_Color,   -1 },
   { gettext_noop("Colour for header &names"),     Field_Color,   -1 },
   { gettext_noop("Colour for header &values"),    Field_Color,   -1 },
   { gettext_noop("&Inline graphics"),             Field_Bool,    -1 },
   { gettext_noop("Display &mail messages as text"),Field_Bool,    -1 },
#ifdef OS_UNIX
   { gettext_noop("Support special &fax mailers"), Field_Bool,    -1 },
   { gettext_noop("&Domains sending faxes"),       Field_Text,    ConfigField_MessageViewFaxSupport},
#endif // unix
   { gettext_noop("The following settings are for the list of messages."),
                                                   Field_Message,  -1 },
   { gettext_noop("Font famil&y"
                  ":default:decorative:roman:script:swiss:modern:teletype"),
                                                   Field_Combo,   -1},
   { gettext_noop("Font si&ze"),                   Field_Number,  -1},
   { gettext_noop("Show only sender's name, not &e-mail"),
                                                   Field_Bool,    -1 },
   { gettext_noop("Foreground c&olour"),           Field_Color,   -1},
   { gettext_noop("&Backgroud colour"),            Field_Color,   -1},
   { gettext_noop("Colour for &new message"),      Field_Color,   -1},
   { gettext_noop("Colour for &recent messages"),  Field_Color,   -1},
   { gettext_noop("Colour for u&nread messages"),  Field_Color,   -1},
   { gettext_noop("Colour for &deleted messages" ),Field_Color,   -1},
   { gettext_noop("The following settings allow to limit the amount of data\n"
                  "retrieved from remote server: if the message size or\n"
                  "number is greater than the value specified here, you\n"
                  "will be asked for confirmation before transfering data."),
                                                   Field_Message,  -1 },
   { gettext_noop("Maximum size of &message (in Kb)"),
                                                   Field_Number,   -1 },
   { gettext_noop("Maximum &number of messages"),  Field_Number,   -1 },
   { gettext_noop("Configure &headers to show..."),Field_SubDlg,   -1 },
   { gettext_noop("&Sort messages by..."),         Field_SubDlg,  -1},
   { gettext_noop("Configure &format for displaying dates"),         Field_SubDlg,    -1                     },

   // adb: autocollect and bbdb options
   { gettext_noop("Mahogany may automatically remember all e-mail addresses in the messages you\n"
                  "receive in a special addresss book. This is called 'address\n"
                  "autocollection' and may be turned on or off from this page."),
                                                   Field_Message, -1                     },
   { gettext_noop("&Autocollect addresses"),       Field_Action,  -1,                    },
   { gettext_noop("Address &book to use"),         Field_Text, ConfigField_AutoCollect   },
   { gettext_noop("Ignore addresses without &names"), Field_Bool, ConfigField_AutoCollect},
   { gettext_noop("The following settings configure the support of the Big Brother\n"
                  "addressbook (BBDB) format. This is supported only for compatibility\n"
                  "with other software (emacs). The normal addressbook is unaffected by\n"
                  "these settings."), Field_Message, -1},
   { gettext_noop("&Ignore entries without names"), Field_Bool, -1 },
   { gettext_noop("&Generate unique aliases"),      Field_Bool, -1 },
   { gettext_noop("&Name for nameless entries"),    Field_Text, ConfigField_Bbdb_GenerateUnique },
   { gettext_noop("&Save on exit"),                 Field_Action, -1 },

   // helper programs
   { gettext_noop("The following program will be used to open URLs embedded in messages:"),       Field_Message, -1                      },
   { gettext_noop("Open &URLs with"),             Field_File,    -1                      },
   { gettext_noop("URL &browser is Netscape"),    Field_Bool,    -1                      },
   { gettext_noop("The following program will be used to view the online help system:"),     Field_Message, -1                      },
   { gettext_noop("&Help viewer"),                Field_File,    -1                      },
   { gettext_noop("Help &viewer is Netscape"),    Field_Bool,    -1                      },
   { gettext_noop("You may configure the external editor to be used "
                  "when composing the messages and optionally choose "
                  "to launch it automatically."),       Field_Message, -1                      },
   { gettext_noop("&External editor"),            Field_Text,    -1                      },
   { gettext_noop("Always &use it"),              Field_Bool, ConfigField_ExternalEditor },
#ifdef OS_UNIX
   { gettext_noop("&Image format converter"),     Field_File,    -1                      },
   { gettext_noop("Conversion &graphics format"
                  ":XPM:PNG:BMP:JPG:GIF:PCX:PNM"),    Field_Combo,   -1 },
#endif
   { gettext_noop("The following line will be executed each time new mail is received:"),       Field_Message, -1                      },
   { gettext_noop("&New Mail Command"),           Field_File,    -1                      },

   // other options
   { gettext_noop("Show &log window"),             Field_Bool,    -1,                    },
   { gettext_noop("Show &tips at startup"),        Field_Bool,    -1,                    },
   { gettext_noop("&Splash screen at startup"),    Field_Bool | Field_Restart, -1,                    },
   { gettext_noop("Splash screen &delay"),         Field_Number,  ConfigField_Splash     },
   { gettext_noop("If autosave delay is not 0, the program will periodically\n"
                  "save all unsaved information which reduces the risk of loss\n"
                  "of information"),               Field_Message, -1                     },
   { gettext_noop("&Autosave delay"),              Field_Number, -1                      },
   { gettext_noop("Confirm &exit"),                Field_Bool | Field_Restart, -1                     },
   { gettext_noop("Open folder on single &click"), Field_Bool,    -1                     },
   { gettext_noop("Show &hidden folders in the folder tree"), Field_Bool,    -1                     },
   { gettext_noop("Show new mail &notifications"), Field_Bool,    -1                     },
   { gettext_noop("Debug server and mailbox access"), Field_Bool, -1
   },
#ifdef USE_SSL
   /* The two settings are not really Field_Restart, but if one has
      tried to use SSL before setting them correctly, then
      MailFolderCC will not attempt to load the libs again. So we just
      pretent that you always have to restart it to prevent users from
      complaining to us if it doesn't work. I'm lazy. KB*/
   { gettext_noop("Mahogany can use SSL (Secure Sockets Layer) for secure,\n"
                  "encrypted communications, if you have the libssl and libcrypto\n"
                  "shared libraries (DLLs) on your system."),
     Field_Message, -1                     },
   { gettext_noop("Path where to find s&hared libssl"), Field_File|Field_Restart,    -1                     },
   { gettext_noop("Path where to find sha&red libcrypto"), Field_File|Field_Restart,    -1                     },
#endif
#ifdef OS_UNIX
   { gettext_noop("&Path where to find AFM files"), Field_Dir,    -1                     },
   { gettext_noop("&Focus follows mouse"), Field_Bool,    -1                     },
#endif
   { gettext_noop("Use floating &menu-bars"), Field_Bool,    -1                     },
   { gettext_noop("Use floating &tool-bars"), Field_Bool,    -1                     },
   { gettext_noop("Tool-bars with f&lat buttons"), Field_Bool,    -1                     },
   { gettext_noop("&Reenable disabled message boxes..."), Field_SubDlg, -1 }
};

// FIXME ugly, ugly, ugly... config settings should be living in an array from
//       the beginning which would avoid us all these contorsions
#define CONFIG_ENTRY(name)  ConfigValueDefault(name, name##_D)
// even worse: dummy entries for message fields
#define CONFIG_NONE()  ConfigValueNone()

// if you modify this array, search for DONT_FORGET_TO_MODIFY and modify data
// there too
const ConfigValueDefault wxOptionsPageStandard::ms_aConfigDefaults[] =
{
   // identity
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_USERNAME),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_HOSTNAME),
   CONFIG_ENTRY(MP_ADD_DEFAULT_HOSTNAME),
   CONFIG_ENTRY(MP_RETURN_ADDRESS),
   CONFIG_ENTRY(MP_SET_REPLY_FROM_TO),
   CONFIG_ENTRY(MP_PERSONALNAME),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_USEVCARD),
   CONFIG_ENTRY(MP_VCARD),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_USERLEVEL),

   // network
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_POPHOST),
   CONFIG_ENTRY(MP_IMAPHOST),
   CONFIG_ENTRY(MP_SMTPHOST),
   CONFIG_ENTRY(MP_NNTPHOST),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_SMTPHOST_LOGIN),
   CONFIG_ENTRY(MP_SMTPHOST_PASSWORD),
   CONFIG_ENTRY(MP_NNTPHOST_LOGIN),
   CONFIG_ENTRY(MP_NNTPHOST_PASSWORD),
#ifdef USE_SSL
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_SMTPHOST_USE_SSL),
   CONFIG_ENTRY(MP_NNTPHOST_USE_SSL),
#endif
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_DIALUP_SUPPORT),
   CONFIG_ENTRY(MP_BEACONHOST),
#ifdef OS_WIN
   CONFIG_ENTRY(MP_NET_CONNECTION),
#elif defined(OS_UNIX)
   CONFIG_ENTRY(MP_NET_ON_COMMAND),
   CONFIG_ENTRY(MP_NET_OFF_COMMAND),
#endif // platform
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_TCP_OPENTIMEOUT),
#ifdef USE_TCP_TIMEOUTS
   CONFIG_ENTRY(MP_TCP_READTIMEOUT),
   CONFIG_ENTRY(MP_TCP_WRITETIMEOUT),
   CONFIG_ENTRY(MP_TCP_RSHTIMEOUT),
   CONFIG_ENTRY(MP_TCP_CLOSETIMEOUT),
#endif // USE_TCP_TIMEOUTS

   // compose
   CONFIG_ENTRY(MP_USEOUTGOINGFOLDER), // where to keep copies of
                                       // messages send
   CONFIG_ENTRY(MP_OUTGOINGFOLDER),
   CONFIG_ENTRY(MP_WRAPMARGIN),
   CONFIG_ENTRY(MP_AUTOMATIC_WORDWRAP),
   CONFIG_ENTRY(MP_REPLY_PREFIX),
   CONFIG_ENTRY(MP_REPLY_COLLAPSE_PREFIX),
   CONFIG_ENTRY(MP_REPLY_MSGPREFIX),

   CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE),
   CONFIG_ENTRY(MP_COMPOSE_SIGNATURE),
   CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE_SEPARATOR),
   CONFIG_ENTRY(MP_COMPOSE_XFACE_FILE),
   CONFIG_ENTRY(MP_ADB_SUBSTRINGEXPANSION),
   CONFIG_ENTRY(MP_CVIEW_FONT),
   CONFIG_ENTRY(MP_CVIEW_FONT_SIZE),
   CONFIG_ENTRY(MP_CVIEW_FGCOLOUR),
   CONFIG_ENTRY(MP_CVIEW_BGCOLOUR),
   CONFIG_NONE(),
   CONFIG_NONE(),

   // folders
   CONFIG_ENTRY(MP_OPENFOLDERS),
   CONFIG_ENTRY(MP_MAINFOLDER),
// CONFIG_ENTRY(MP_NEWMAIL_FOLDER),
   CONFIG_ENTRY(MP_POLLINCOMINGDELAY),
   CONFIG_ENTRY(MP_UPDATEINTERVAL),
   CONFIG_ENTRY(MP_AUTOSHOW_FIRSTMESSAGE),
   CONFIG_ENTRY(MP_FOLDERPROGRESS_THRESHOLD),
   CONFIG_ENTRY(MP_USE_OUTBOX), // where to store message before
                                // sending them
   CONFIG_ENTRY(MP_OUTBOX_NAME),
   CONFIG_ENTRY(MP_USE_TRASH_FOLDER),
   CONFIG_ENTRY(MP_TRASH_FOLDER),
   CONFIG_ENTRY(MP_FOLDER_FILE_DRIVER),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FOLDERSTATUS_STATBAR),
   CONFIG_ENTRY(MP_FOLDERSTATUS_TITLEBAR),
   // python
#ifdef USE_PYTHON
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_USEPYTHON),
   CONFIG_ENTRY(MP_PYTHONPATH),
   CONFIG_ENTRY(MP_STARTUPSCRIPT),
   CONFIG_ENTRY(MCB_FOLDEROPEN),
   CONFIG_ENTRY(MCB_FOLDERUPDATE),
   CONFIG_ENTRY(MCB_FOLDEREXPUNGE),
   CONFIG_ENTRY(MCB_FOLDERSETMSGFLAG),
   CONFIG_ENTRY(MCB_FOLDERCLEARMSGFLAG),
#endif // USE_PYTHON

   // message views
   CONFIG_ENTRY(MP_PREVIEW_ON_SELECT),
   CONFIG_ENTRY(MP_MVIEW_FONT),
   CONFIG_ENTRY(MP_MVIEW_FONT_SIZE),
   CONFIG_ENTRY(MP_MVIEW_FGCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_BGCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_URLCOLOUR),
   CONFIG_ENTRY(MP_MVIEW_HEADER_NAMES_COLOUR),
   CONFIG_ENTRY(MP_MVIEW_HEADER_VALUES_COLOUR),
   CONFIG_ENTRY(MP_INLINE_GFX),
   CONFIG_ENTRY(MP_RFC822_IS_TEXT),
#ifdef OS_UNIX
   CONFIG_ENTRY(MP_INCFAX_SUPPORT),
   CONFIG_ENTRY(MP_INCFAX_DOMAINS),
#endif
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_FVIEW_FONT),
   CONFIG_ENTRY(MP_FVIEW_FONT_SIZE),
   CONFIG_ENTRY(MP_FVIEW_NAMES_ONLY),
   CONFIG_ENTRY(MP_FVIEW_FGCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_BGCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_NEWCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_RECENTCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_UNREADCOLOUR),
   CONFIG_ENTRY(MP_FVIEW_DELETEDCOLOUR),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_MAX_MESSAGE_SIZE),
   CONFIG_ENTRY(MP_MAX_HEADERS_NUM),
   CONFIG_ENTRY(MP_MSGVIEW_HEADERS),
   CONFIG_ENTRY(MP_MSGS_SORTBY),
   CONFIG_ENTRY(MP_DATE_FMT),

   // autocollect
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_AUTOCOLLECT),
   CONFIG_ENTRY(MP_AUTOCOLLECT_ADB),
   CONFIG_ENTRY(MP_AUTOCOLLECT_NAMED),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_BBDB_IGNOREANONYMOUS),
   CONFIG_ENTRY(MP_BBDB_GENERATEUNIQUENAMES),
   CONFIG_ENTRY(MP_BBDB_ANONYMOUS),
   CONFIG_ENTRY(MP_BBDB_SAVEONEXIT),

   // helper programs
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_BROWSER),
   CONFIG_ENTRY(MP_BROWSER_ISNS),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_HELPBROWSER),
   CONFIG_ENTRY(MP_HELPBROWSER_ISNS),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_EXTERNALEDITOR),
   CONFIG_ENTRY(MP_ALWAYS_USE_EXTERNALEDITOR),
#ifdef OS_UNIX
   CONFIG_ENTRY(MP_CONVERTPROGRAM),
   CONFIG_ENTRY(MP_TMPGFXFORMAT),
#endif
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_NEWMAILCOMMAND),

   // other
   CONFIG_ENTRY(MP_SHOWLOG),
   CONFIG_ENTRY(MP_SHOWTIPS),
   CONFIG_ENTRY(MP_SHOWSPLASH),
   CONFIG_ENTRY(MP_SPLASHDELAY),
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_AUTOSAVEDELAY),
   CONFIG_ENTRY(MP_CONFIRMEXIT),
   CONFIG_ENTRY(MP_OPEN_ON_CLICK),
   CONFIG_ENTRY(MP_SHOW_HIDDEN_FOLDERS),
   CONFIG_ENTRY(MP_SHOW_NEWMAILMSG),
   CONFIG_ENTRY(MP_DEBUG_CCLIENT),
#ifdef USE_SSL
   CONFIG_NONE(),
   CONFIG_ENTRY(MP_SSL_DLL_SSL),
   CONFIG_ENTRY(MP_SSL_DLL_CRYPTO),
#endif
#ifdef OS_UNIX
   CONFIG_ENTRY(MP_AFMPATH),
   CONFIG_ENTRY(MP_FOCUS_FOLLOWSMOUSE),
#endif
   CONFIG_ENTRY(MP_DOCKABLE_MENUBARS),
   CONFIG_ENTRY(MP_DOCKABLE_TOOLBARS),
   CONFIG_ENTRY(MP_FLAT_TOOLBARS),
   CONFIG_NONE()
};

#undef CONFIG_ENTRY
#undef CONFIG_NONE

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
                             const char *title,
                             Profile *profile,
                             int helpId,
                             int image)
             : wxNotebookPageBase(notebook)
{
   m_aFields = aFields;
   m_aDefaults = aDefaults;

   notebook->AddPage(this, title, FALSE /* don't select */, image);

   m_Profile = profile;
   m_Profile->IncRef();

   m_HelpId = helpId;

   m_nFirst = nFirst;
   m_nLast = nLast;

   CreateControls();
}

void wxOptionsPage::CreateControls()
{
   size_t n;

   // soem fields are only shown in 'advanced' mode, so check if we're in it
   bool isAdvanced = READ_APPCONFIG(MP_USERLEVEL) >= M_USERLEVEL_ADVANCED;

   // first determine the longest label
   wxArrayString aLabels;
   for ( n = m_nFirst; n < m_nLast; n++ ) {
      if ( !isAdvanced && (GetFieldFlags(n) & Field_Advanced) )
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
      case Field_Action:
      case Field_Text:
         break;

      default:
         // don't take into account the other types
         continue;
      }

      aLabels.Add(_(m_aFields[n].label));
   }

   long widthMax = GetMaxLabelWidth(aLabels, this);

   // now create the controls
   int styleText = wxALIGN_RIGHT;
   wxControl *last = NULL; // last control created
   for ( n = m_nFirst; n < m_nLast; n++ ) {
      FieldFlags flags = GetFieldFlags(n);
      if ( !isAdvanced && (flags & Field_Advanced) )
      {
         // skip this one
         m_aControls.Add(NULL);
         m_aDirtyFlags.Add(false);

         continue;
      }

      switch ( GetFieldType(n) ) {
         case Field_Dir:
            last = CreateDirEntry(_(m_aFields[n].label), widthMax, last);
            break;
            
         case Field_File:
            last = CreateFileEntry(_(m_aFields[n].label), widthMax, last);
            break;

         case Field_Folder:
            last = CreateFolderEntry(_(m_aFields[n].label), widthMax, last);
            break;

         case Field_Color:
            last = CreateColorEntry(_(m_aFields[n].label), widthMax, last);
            break;

         case Field_Action:
            last = CreateActionChoice(_(m_aFields[n].label), widthMax, last);
            break;

         case Field_Combo:
            // a hack to dynamicially fill the RAS connections combo box under
            // Windows - I didn't find anything better to do right now, may be
            // later (feel free to tell me if you have any ideas)
#ifdef OS_WIN
            if ( n == ConfigField_NetConnection )
            {
               wxString connections = _(m_aFields[n].label);

               wxDialUpManager *dial = wxDialUpManager::Create();
               wxArrayString aConnections;
               size_t nCount = dial->GetISPNames(aConnections);
               delete dial;

               for ( size_t n = 0; n < nCount; n++ )
               {
                  connections << ':' << aConnections[n];
               }

               last = CreateComboBox(connections, widthMax, last);
            }
            else
#endif // OS_WIN
                last = CreateComboBox(_(m_aFields[n].label), widthMax, last);
            break;

         case Field_Passwd:
            styleText |= wxTE_PASSWORD;
            // fall through

         case Field_Number:
            // fall through -- for now they're the same as text
         case Field_Text:
            last = CreateTextWithLabel(_(m_aFields[n].label), widthMax, last,
                                       0, styleText);

            // reset
            styleText = wxALIGN_RIGHT;
            break;

         case Field_List:
            last = CreateListbox(_(m_aFields[n].label), last);
            break;

         case Field_Bool:
            last = CreateCheckBox(_(m_aFields[n].label), widthMax, last);
            break;

         case Field_Message:
            last = CreateMessage(_(m_aFields[n].label), last);
            break;

         case Field_SubDlg:
            last = CreateButton(_(m_aFields[n].label), last);
            break;

         case Field_XFace:
            last = CreateXFaceButton(_(m_aFields[n].label), widthMax, last);
            break;

         default:
            wxFAIL_MSG("unknown field type in CreateControls");
         }

      wxCHECK_RET( last, "control creation failed" );

      if ( flags & Field_Vital )
         m_aVitalControls.Add(last);
      if ( flags & Field_Restart )
         m_aRestartControls.Add(last);

      m_aControls.Add(last);
      m_aDirtyFlags.Add(false);
   }
}

void wxOptionsPage::OnChange(wxEvent& event)
{
   wxControl *control = (wxControl *)event.GetEventObject();
   int index = m_aControls.Index(control);

   if ( index == wxNOT_FOUND )
   {
      // we can get events from the text controls from "file open" dialog here
      // too - just skip them silently
      event.Skip();

      return;
   }

   m_aDirtyFlags[(size_t)index] = true;

   wxOptionsDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsDialog);

   UpdateUI();

   if ( !dialog )
   {
      // we don't put an assert here because this does happen when we're a
      // page in the folder properties dialog, but we must have event.Skip()
      // here to allow the base class version mark the dialog as being dirty
      event.Skip();

      return;
   }

   if ( m_aVitalControls.Index(control) != -1 )
      dialog->SetDoTest();
   else
      dialog->SetDirty();

   if ( m_aRestartControls.Index(control) != -1 )
      dialog->SetGiveRestartWarning();
}

void wxOptionsPage::OnControlChange(wxEvent& event)
{
   OnChange(event);

   Refresh();
}

void wxOptionsPage::UpdateUI()
{
   for ( size_t n = m_nFirst; n < m_nLast; n++ ) {
      int nCheckField = m_aFields[n].enable;
      if ( nCheckField != -1 ) {
         wxASSERT( nCheckField >= 0 && nCheckField < ConfigField_Max );

         // avoid signed/unsigned mismatch in expressions
         size_t nCheck = (size_t)nCheckField;
         wxCHECK_RET( nCheck >= m_nFirst && nCheck < m_nLast,
                      "control index out of range" );

         bool bEnable = true;
         if ( GetFieldType(nCheck) == Field_Bool )
         {
            // enable only if the checkbox is checked
            wxCheckBox *checkbox = wxStaticCast(GetControl(nCheck), wxCheckBox);

            bEnable = checkbox->GetValue();
         }
         else if ( GetFieldType(nCheck) == Field_Action )
         {
            // only enable if the radiobox selection is 0 (meaning "yes")
            wxRadioBox *radiobox = wxStaticCast(GetControl(nCheck), wxRadioBox);

            if ( radiobox->GetSelection() == 0 ) // FIXME hardcoded!
               bEnable = false;
         }
         else
         {
            // assume that this is one of the text controls
            wxTextCtrl *text = wxStaticCast(GetControl(nCheck), wxTextCtrl);
            wxCHECK_RET( text, "can't depend on this control type" );

            // only enable if the text control has something
            bEnable = !text->GetValue().IsEmpty();
         }

         wxControl *control = GetControl(n);
         control->Enable(bEnable);

         switch ( GetFieldType(n) )
         {
               // for file entries, also disable the browse button
            case Field_File:
            case Field_Dir:
            case Field_Color:
            case Field_Folder:
               wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

               EnableTextWithButton((wxTextCtrl *)control, bEnable);
               break;

            case Field_Passwd:
            case Field_Number:
            case Field_Text:
               wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

               EnableTextWithLabel((wxTextCtrl *)control, bEnable);
               break;

            case Field_List:
               // also disable the buttons
            {
               long i;
               for ( i = wxOptionsPage_BtnNew; i <= wxOptionsPage_BtnNew; i++ ) {
                  wxWindow *win = FindWindow(i);
                  if ( win ) {
                     win->Enable(bEnable);
                  }
                  else {
                     wxFAIL_MSG("can't find listbox buttons by id");
                  }
               }
            }
            break;

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

   String strValue;
   long lValue = 0;
   for ( size_t n = m_nFirst; n < m_nLast; n++ )
   {
      if ( m_aDefaults[n].IsNumeric() )
      {
         lValue = m_Profile->readEntry(m_aDefaults[n].name,
                                       (int)m_aDefaults[n].lValue);
         strValue.Printf("%ld", lValue);
      }
      else {
         // it's a string
         strValue = m_Profile->readEntry(m_aDefaults[n].name,
                                         m_aDefaults[n].szValue);
      }

      wxControl *control = GetControl(n);
      if ( !control )
         continue;

      switch ( GetFieldType(n) ) {
      case Field_Text:
      case Field_Number:
         if ( GetFieldType(n) == Field_Number ) {
            wxASSERT( m_aDefaults[n].IsNumeric() );

            strValue.Printf("%ld", lValue);
         }
         else {
            wxASSERT( !m_aDefaults[n].IsNumeric() );
         }

         // can only have text value
      case Field_Passwd:
      case Field_Dir:
      case Field_File:
      case Field_Color:
      case Field_Folder:
         wxStaticCast(control, wxTextCtrl)->SetValue(strValue);
         break;

      case Field_Bool:
         wxASSERT( m_aDefaults[n].IsNumeric() );
         wxStaticCast(control, wxCheckBox)->SetValue(lValue != 0);
         break;

      case Field_Action:
         wxStaticCast(control, wxRadioBox)->SetSelection(lValue);
         break;

      case Field_Combo:
         wxStaticCast(control, wxComboBox)->SetSelection(lValue);
         break;

      case Field_List:
         wxASSERT( !m_aDefaults[n].IsNumeric() );

         // split it (FIXME what if it contains ';'?)
         {
            wxListBox *lbox = wxStaticCast(control, wxListBox);
            String str;
            for ( size_t m = 0; m < strValue.Len(); m++ ) {
               if ( strValue[m] == ';' ) {
                  if ( !str.IsEmpty() ) {
                     lbox->Append(str);
                     str.Empty();
                  }
                  //else: nothing to do, two ';' one after another
               }
               else {
                  str << strValue[m];
               }
            }

            if ( !str.IsEmpty() ) {
               lbox->Append(str);
            }
         }
         break;

      case Field_XFace:
      {
         wxXFaceButton *btnXFace = (wxXFaceButton *)control;
         if ( READ_CONFIG(m_Profile, MP_COMPOSE_USE_XFACE) )
            btnXFace->SetFile(READ_CONFIG(m_Profile, MP_COMPOSE_XFACE_FILE));
         else
            btnXFace->SetFile("");
      }
      break;

      case Field_Message:
      case Field_SubDlg:      // these settings will be read later
         break;

      default:
         wxFAIL_MSG("unexpected field type");
      }

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

      switch ( GetFieldType(n) )
      {
      case Field_Passwd:
      case Field_Text:
      case Field_Dir:
      case Field_File:
      case Field_Color:
      case Field_Folder:
      case Field_Number:
         wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

         strValue = ((wxTextCtrl *)control)->GetValue();

         if ( GetFieldType(n) == Field_Number ) {
            wxASSERT( m_aDefaults[n].IsNumeric() );

            lValue = atol(strValue);
         }
         else {
            wxASSERT( !m_aDefaults[n].IsNumeric() );
         }
         break;

      case Field_Bool:
         wxASSERT( m_aDefaults[n].IsNumeric() );
         wxASSERT( control->IsKindOf(CLASSINFO(wxCheckBox)) );

         lValue = ((wxCheckBox *)control)->GetValue();
         break;

      case Field_Action:
         wxASSERT( m_aDefaults[n].IsNumeric() );
         wxASSERT( control->IsKindOf(CLASSINFO(wxRadioBox)) );

         lValue = ((wxRadioBox *)control)->GetSelection();
         break;

      case Field_Combo:
         wxASSERT( m_aDefaults[n].IsNumeric() );
         wxASSERT( control->IsKindOf(CLASSINFO(wxComboBox)) );

         lValue = ((wxComboBox *)control)->GetSelection();
         break;

      case Field_List:
         wxASSERT( !m_aDefaults[n].IsNumeric() );
         wxASSERT( control->IsKindOf(CLASSINFO(wxListBox)) );

            // join it (FIXME what if it contains ';'?)
         {
            wxListBox *listbox = (wxListBox *)control;
            for ( size_t m = 0; m < (size_t)listbox->Number(); m++ ) {
               if ( !strValue.IsEmpty() ) {
                  strValue << ';';
               }

               strValue << listbox->GetString(m);
            }
         }
         break;

      case Field_Message:
      case Field_SubDlg:      // already done
      case Field_XFace:      // already done
         break;

      default:
         wxFAIL_MSG("unexpected field type");
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
// wxOptionsPageDynamic
// ----------------------------------------------------------------------------

wxOptionsPageDynamic::wxOptionsPageDynamic(wxNotebook *parent,
                                           const char *title,
                                           Profile *profile,
                                           FieldInfoArray aFields,
                                           ConfigValuesArray aDefaults,
                                           size_t nFields,
                                           int helpId,
                                           int image)
                     : wxOptionsPage(aFields, aDefaults, 0, nFields,
                                     parent, title, profile, helpId, image)
{
}

// ----------------------------------------------------------------------------
// wxOptionsPageStandard
// ----------------------------------------------------------------------------

wxOptionsPageStandard::wxOptionsPageStandard(wxNotebook *notebook,
                                             const char *title,
                                             Profile *profile,
                                             size_t nFirst,
                                             size_t nLast,
                                             int helpId)
                     : wxOptionsPage(ms_aFields, ms_aConfigDefaults,
                                     // see enum ConfigFields for the
                                     // explanation of "+1"
                                     nFirst + 1, nLast + 1,
                                     notebook, title, profile, helpId,
                                     notebook->GetPageCount())
{
   // check that we didn't forget to update one of the arrays...
   wxASSERT( WXSIZEOF(ms_aConfigDefaults) == ConfigField_Max );
   wxASSERT( WXSIZEOF(ms_aFields) == ConfigField_Max );
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
         // Why doesn´t UpdateUI() have the same effect here?
         if(READ_CONFIG(m_Profile, MP_COMPOSE_USE_XFACE))
            btn->SetFile(READ_CONFIG(m_Profile,MP_COMPOSE_XFACE_FILE));
         else
            btn->SetFile("");
      }
   }
   else
   {
      FAIL_MSG("click from alien button in compose view page");

      dirty = FALSE;

      event.Skip();
   }

   if ( dirty )
   {
      // something changed - make us dirty
      wxNotebookDialog *dialog = GET_PARENT_OF_CLASS(this, wxNotebookDialog);

      wxCHECK_RET( dialog, "options page without a parent dialog?" );

      dialog->SetDirty();
   }
}

bool wxOptionsPageCompose::TransferDataFromWindow()
{
   bool rc = wxOptionsPage::TransferDataFromWindow();
   if ( rc && READ_CONFIG(m_Profile, MP_USE_OUTBOX) )
   {
      /* Make sure the Outbox setting is consistent across all
         folders! */
      wxString outbox = READ_CONFIG(m_Profile, MP_OUTBOX_NAME);
      wxString globalOutbox = READ_APPCONFIG(MP_OUTBOX_NAME);
      if(outbox != globalOutbox)
      {
         /* Erasing the local value should be good enough, but let´s
            play it safe. */
         m_Profile->writeEntry(MP_OUTBOX_NAME, globalOutbox);
         wxString msg;
         msg.Printf(_("You set the name of the outbox for temporarily storing messages\n"
                      "before sending them to be ´%s´. This is different from the\n"
                      "setting in the global options, which is ´%s´. A you can have\n"
                      "only a single outbox, the value has been restored to be ´%s´."),
                    outbox.c_str(), globalOutbox.c_str(), globalOutbox.c_str());
         wxLogError(msg);
      }
   }
   return rc;
}

// ----------------------------------------------------------------------------
// wxOptionsPageMessageView
// ----------------------------------------------------------------------------

wxOptionsPageMessageView::wxOptionsPageMessageView(wxNotebook *parent,
                                                   Profile *profile)
   : wxOptionsPageStandard(parent,
                   _("Message Viewer"),
                   profile,
                   ConfigField_MessageViewFirst,
                   ConfigField_MessageViewLast,
                   MH_OPAGE_MESSAGEVIEW)
{
}

void wxOptionsPageMessageView::OnButton(wxCommandEvent& event)
{
   bool dirty = false;

   wxObject *obj = event.GetEventObject();
   if ( obj == GetControl(ConfigField_MessageViewSortMessagesBy) )
      dirty = ConfigureSorting(m_Profile, this);
   else if ( obj == GetControl(ConfigField_MessageViewDateFormat) )
      dirty = ConfigureDateFormat(m_Profile, this);
   else if(obj == GetControl(ConfigField_MessageViewHeaders))
      dirty = ConfigureMsgViewHeaders(m_Profile, this);
   else
   {
      wxASSERT_MSG( 0, "alien button" );
   }
   if(dirty)
   {
      // something changed - make us dirty
      wxNotebookDialog *dialog = GET_PARENT_OF_CLASS(this, wxNotebookDialog);
      wxCHECK_RET( dialog, "options page without a parent dialog?" );
      dialog->SetDirty();
   }
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
                                _("Addressbook"),
                                profile,
                                ConfigField_AdbFirst,
                                ConfigField_AdbLast,
                                MH_OPAGE_ADB)
{
}


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
   m_nAutosaveDelay = -1;
}

void wxOptionsPageOthers::OnButton(wxCommandEvent& event)
{
   wxObject *obj = event.GetEventObject();
   if ( obj == GetControl(ConfigField_ReenableDialog) )
   {
      if ( ReenablePersistentMessageBoxes(this) )
      {
         wxNotebookDialog *dialog = GET_PARENT_OF_CLASS(this, wxNotebookDialog);
         wxCHECK_RET( dialog, "options page without a parent dialog?" );
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
   m_Profile->writeEntry(MP_CONFIRMEXIT, wxPMessageBoxEnabled(MP_CONFIRMEXIT));

   bool rc = wxOptionsPage::TransferDataToWindow();
   if ( rc )
   {
      m_nAutosaveDelay = READ_CONFIG(m_Profile, MP_AUTOSAVEDELAY);
   }

   return rc;
}

bool wxOptionsPageOthers::TransferDataFromWindow()
{
   bool rc = wxOptionsPage::TransferDataFromWindow();
   if ( rc )
   {
      // now if the user checked "confirm exit" checkbox we must reenable
      // the message box by erasing the stored answer to it
      wxPMessageBoxEnable(MP_CONFIRMEXIT,
                          READ_CONFIG(m_Profile, MP_CONFIRMEXIT) != 0);

      // restart the timer if the timeout changed
      long nAutosaveDelay = READ_CONFIG(m_Profile, MP_AUTOSAVEDELAY);
      if ( nAutosaveDelay != m_nAutosaveDelay )
      {
         if ( !mApplication->RestartTimer(MAppBase::Timer_Autosave) )
         {
            wxLogError(_("Invalid delay value for autosave timer."));

            rc = false;
         }
      }

      // show/hide the log window depending on the new setting value
      bool showLog = READ_CONFIG(m_Profile, MP_SHOWLOG) != 0;
      if ( showLog != mApplication->IsLogShown() )
      {
         mApplication->ShowLog(showLog);
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
   m_nIncomingDelay =
   m_nPingDelay = -1;
}

bool wxOptionsPageFolders::TransferDataToWindow()
{
   bool bRc = wxOptionsPage::TransferDataToWindow();

   if ( bRc ) {
      // we add the folder opened in the main frame to the list of folders
      // opened on startup if it's not yet among them
      wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
      wxString strMain = GetControlText(ConfigField_MainFolder);
      int n = listbox->FindString(strMain);
      if ( n == -1 ) {
         listbox->Append(strMain);
      }

      m_nIncomingDelay = READ_CONFIG(m_Profile, MP_POLLINCOMINGDELAY);
      m_nPingDelay = READ_CONFIG(m_Profile, MP_UPDATEINTERVAL);
   }

   return bRc;
}

bool wxOptionsPageFolders::TransferDataFromWindow()
{
   // undo what we did in TransferDataToWindow: remove the main folder from
   // the list of folders to be opened on startup
   wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
   wxString strMain = GetControlText(ConfigField_MainFolder);
   int n = listbox->FindString(strMain);
   if ( n != -1 ) {
      listbox->Delete(n);
   }

   bool rc = wxOptionsPage::TransferDataFromWindow();
   if ( rc )
   {
      long nIncomingDelay = READ_CONFIG(m_Profile, MP_POLLINCOMINGDELAY),
           nPingDelay = READ_CONFIG(m_Profile, MP_UPDATEINTERVAL);

      if ( nIncomingDelay != m_nIncomingDelay )
      {
         wxLogDebug("Restarting timer for polling incoming folders");

         rc = mApplication->RestartTimer(MAppBase::Timer_PollIncoming);
      }

      if ( rc && (nPingDelay != m_nPingDelay) )
      {
         wxLogDebug("Restarting timer for pinging folders");

         rc = mApplication->RestartTimer(MAppBase::Timer_PingFolder);
      }

      if ( !rc )
      {
         wxLogError(_("Failed to restart the timers, please change the "
                      "delay to a valid value."));
      }
   }

   if ( rc )
   {
      // update the frame title/status bar if needed
      if ( IsDirty(ConfigField_StatusFormat_StatusBar) ||
           IsDirty(ConfigField_StatusFormat_TitleBar) )
      {
         // TODO: send the folder statyus change event
      }
   }

   return rc;
}

void wxOptionsPageFolders::OnButton(wxCommandEvent& event)
{
   bool dirty = FALSE;

   switch(event.GetId())
   {
   case wxOptionsPage_BtnNew: OnNewFolder(event); break;
   case wxOptionsPage_BtnModify: OnModifyFolder(event); break;
   case wxOptionsPage_BtnDelete: OnDeleteFolder(event); break;
   default:
      ;
   }

   wxOptionsDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsDialog);
   if ( dirty && dialog )
      dialog->SetDirty();
}


void wxOptionsPageFolders::OnNewFolder(wxCommandEvent& event)
{
   wxString str;
   if ( !MInputBox(&str, _("Folders to open on startup"), _("Folder name"),
                   GET_PARENT_OF_CLASS(this, wxDialog), "LastStartupFolder") ) {
      return;
   }

   // check that it's not already there
   wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
   if ( listbox->FindString(str) != -1 ) {
      wxLogError(_("Folder '%s' is already present in the list, not added."),
                 str.c_str());
   }
   else {
      // ok, do add it
      listbox->Append(str);

      wxOptionsPage::OnChange(event);
   }
}

void wxOptionsPageFolders::OnModifyFolder(wxCommandEvent&)
{
   wxListBox *l = (wxListBox *)GetControl(ConfigField_OpenFolders);
   int nSel = l->GetSelection();

   wxCHECK_RET( nSel != -1, "should be disabled" );

   MDialog_FolderProfile(GET_PARENT_OF_CLASS(this, wxDialog),
                         l->GetString(nSel));
}

void wxOptionsPageFolders::OnDeleteFolder(wxCommandEvent& event)
{
   wxListBox *l = (wxListBox *)GetControl(ConfigField_OpenFolders);
   int nSel = l->GetSelection();

   wxCHECK_RET( nSel != -1, "should be disabled" );

   l->Delete(nSel);
   wxOptionsPage::OnChange(event);
}

void wxOptionsPageFolders::OnIdle(wxIdleEvent&)
{
   bool bEnable = ((wxListBox *)GetControl(ConfigField_OpenFolders))
      ->GetSelection() != -1;

   long id;
   for ( id = wxOptionsPage_BtnModify; id <= wxOptionsPage_BtnDelete; id++ ) {
      wxWindow *win = FindWindow(id);
      if ( win ) {
         win->Enable(bEnable);
      }
      else {
         wxFAIL_MSG("can't find listbox buttons by id");
      }
   }
}

// ----------------------------------------------------------------------------
// wxOptionsDialog
// ----------------------------------------------------------------------------

wxOptionsDialog::wxOptionsDialog(wxFrame *parent, const wxString& configKey)
               : wxNotebookDialog(parent, _("Program options"), configKey)
{
}

bool
wxOptionsDialog::TransferDataToWindow()
{
   if ( !wxNotebookDialog::TransferDataToWindow() )
      return FALSE;

   int nPageCount = m_notebook->GetPageCount();
   for ( int nPage = 0; nPage < nPageCount; nPage++ ) {
      ((wxOptionsPage *)m_notebook->GetPage(nPage))->UpdateUI();
   }

   return TRUE;
}

bool
wxOptionsDialog::OnSettingsChange()
{
   if ( m_bTest )
   {
      if ( MDialog_YesNoDialog(_("Some important program settings were changed.\n"
                                 "\nWould you like to test the new setup "
                                 "(recommended)?"),
                               this,
                               _("Test setup?"),
                               true,
                               "OptTestAsk") )
      {
         if ( !VerifyMailConfig() )
         {
            return FALSE;
         }
      }
      else
      {
         // no test was done, assume it's ok...
         m_bTest = FALSE;
      }
   }

   if ( m_bRestartWarning )
   {
      MDialog_Message(_("Some of the changes to the program options will\n"
                        "only take effect when the progam will be run the\n"
                        "next time and not during this session."),
                      this, MDIALOG_MSGTITLE, "WarnRestartOpt");
      m_bRestartWarning = FALSE;
   }

   return TRUE;
}

void wxOptionsDialog::CreateNotebook(wxPanel *panel)
{
   m_notebook = new wxOptionsNotebook(panel);
}

// reset the dirty flag
void wxOptionsDialog::ResetDirty()
{
   wxNotebookDialog::ResetDirty();

   m_bTest =
   m_bRestartWarning = FALSE;
}

wxOptionsDialog::~wxOptionsDialog()
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
      profile = Profile::CreateProfile("");


   for ( size_t n = 0; n < nPages; n++ )
   {
      // the page ctor will add it to the notebook
      const wxOptionsPageDesc& desc = pageDesc[n];
      wxOptionsPageDynamic *page = new wxOptionsPageDynamic(
                                                            this,
                                                            desc.title,
                                                            profile,
                                                            desc.aFields,
                                                            desc.aDefaults,
                                                            desc.nFields,
                                                            desc.helpId,
                                                            n  // image index
                                                           );
      page->Layout();
   }

   profile->DecRef();
}

// return the array which should be passed to wxNotebookWithImages ctor
const char **
wxCustomOptionsNotebook::GetImagesArray(size_t nPages,
                                        const wxOptionsPageDesc *pageDesc)
{
   m_aImages = new const char *[nPages + 1];

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
const char *wxOptionsNotebook::ms_aszImages[] =
{
   "ident",
   "network",
   "compose",
   "folders",
#ifdef USE_PYTHON
   "python",
#endif
   "msgview",
   "adrbook",
   "helpers",
   "miscopt",
   NULL
};

// create the control and add pages too
wxOptionsNotebook::wxOptionsNotebook(wxWindow *parent)
                 : wxNotebookWithImages("OptionsNotebook", parent, ms_aszImages)
{
   // don't forget to update both the array above and the enum!
   wxASSERT( WXSIZEOF(ms_aszImages) == OptionsPage_Max + 1);

   Profile *profile = GetProfile();

   // create and add the pages
   new wxOptionsPageIdent(this, profile);
   new wxOptionsPageNetwork(this, profile);
   new wxOptionsPageCompose(this, profile);
   new wxOptionsPageFolders(this, profile);
#ifdef USE_PYTHON
   new wxOptionsPagePython(this, profile);
#endif
   new wxOptionsPageMessageView(this, profile);
   new wxOptionsPageAdb(this, profile);
   new wxOptionsPageHelpers(this, profile);
   new wxOptionsPageOthers(this, profile);

   profile->DecRef();
}

// ----------------------------------------------------------------------------
// wxIdentityOptionsDialog
// ----------------------------------------------------------------------------

void wxIdentityOptionsDialog::CreatePagesDesc()
{
   m_nPages = 2;
   m_aPages = new wxOptionsPageDesc[2];

   // identity page
   m_aPages[0] = wxOptionsPageDesc
   (
      _("Identity"),
      wxOptionsNotebook::ms_aszImages[OptionsPage_Ident],
      MH_OPAGE_IDENT,
      wxOptionsPageStandard::ms_aFields + ConfigField_IdentFirst + 1,
      wxOptionsPageStandard::ms_aConfigDefaults + ConfigField_IdentFirst + 1,
      ConfigField_IdentLast - ConfigField_IdentFirst
   );

   // network page
   m_aPages[1] = wxOptionsPageDesc
   (
      _("Network"),
      wxOptionsNotebook::ms_aszImages[OptionsPage_Network],
      MH_OPAGE_NETWORK,
      wxOptionsPageStandard::ms_aFields + ConfigField_NetworkFirst + 1,
      wxOptionsPageStandard::ms_aConfigDefaults + ConfigField_NetworkFirst + 1,
      ConfigField_NetworkLast - ConfigField_NetworkFirst
   );
};

wxControl *wxIdentityOptionsDialog::CreateControlsAbove(wxPanel *panel)
{
   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(panel, -1, _("Folder Name: "),
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   wxTextCtrl *pText = new wxTextCtrl(panel, -1,
                                      m_identity,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxTE_READONLY);

   c = new wxLayoutConstraints;
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
   c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
   c->height.AsIs();
   pText->SetConstraints(c);

   return pText;
}

// ----------------------------------------------------------------------------
// wxRestoreDefaultsDialog implementation
// ----------------------------------------------------------------------------

wxRestoreDefaultsDialog::wxRestoreDefaultsDialog(Profile *profile,
                                                 wxFrame *parent)
                       : wxProfileSettingsEditDialog
                         (
                           profile,
                           "RestoreOptionsDlg",
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
            wxStripMenuCodes(_(wxOptionsPageStandard::ms_aFields[n].label)));
   }

   // set the initial and minimal size
   SetDefaultSize(4*wBtn, 10*hBtn, FALSE /* not minimal size */);
}

bool wxRestoreDefaultsDialog::TransferDataFromWindow()
{
   // delete the values of all selected settings in this profile - this will
   // effectively restore their default values
   size_t count = (size_t)m_checklistBox->Number();
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
   wxOptionsDialog dlg(parent);
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

void ShowCustomOptionsDialog(size_t nPages,
                             const wxOptionsPageDesc *pageDesc,
                             Profile *profile,
                             wxFrame *parent)
{
   wxCustomOptionsDialog dlg(nPages, pageDesc, profile, parent);
   dlg.CreateAllControls();
   dlg.Layout();

   (void)dlg.ShowModal();
}

void ShowIdentityDialog(const wxString& identity, wxFrame *parent)
{
   wxIdentityOptionsDialog dlg(identity, parent);
   dlg.CreateAllControls();
   dlg.Layout();

   (void)dlg.ShowModal();
}
