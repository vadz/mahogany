///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   upgrade.cpp - functions to upgrade from previous version of M
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// License:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef   USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "Mdefaults.h"
#  include "guidef.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "gui/wxIconManager.h"

#  include <wx/stattext.h>              // for wxStaticText
#  include <wx/sizer.h>                // for wxBoxSizer
#endif  //USE_PCH

#include "Mpers.h"
#include "SendMessage.h"
#include "MFolder.h"
#include "Message.h"
#include "strlist.h"
#include "ConfigSourcesAll.h"

#include <wx/file.h>                   // for wxFile
#include <wx/fileconf.h>

// use wizard and not the old (and probably broken) dialog-based code
#define USE_WIZARD

// define to enable DNS lookup of the server names - disabled for now as it
// causes too many problems (it may hang for a long time if DNS is not
// available)
//#define USE_DNS

// the DNS lookup code using wxDialUpManager
#ifndef USE_DIALUP
   #undef USE_DNS
#endif // USE_DIALUP

#include "HeaderInfo.h"
#include "MailFolderCC.h"

#include <wx/confbase.h>

#ifdef USE_DIALUP
   #include <wx/dialup.h>     // for IsAlwaysOnline()
#endif // USE_DIALUP

#ifdef USE_DNS
   #include <wx/socket.h>     // wxIPV4address
#endif // USE_DNS

#ifdef OS_UNIX
   // INBOX exists only under Unix - there is no such thing under Windows
   #define USE_INBOX
#endif

#ifdef USE_WIZARD
#   include <wx/panel.h>
#   include <wx/wizard.h>
#else
#   include "gui/wxOptionsDlg.h" // for ShowOptionsDialog()
#   include "gui/wxMDialogs.h"
#endif

#include "gui/wxDialogLayout.h"
#include "gui/wxFolderTree.h"

#include "Mpers.h"

#include "MImport.h"
#include "Mupgrade.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_COLLECT_INBOX;
#ifdef USE_DIALUP
extern const MOption MP_DIALUP_SUPPORT;
#endif // USE_DIALUP
extern const MOption MP_FIRSTRUN;
extern const MOption MP_FOLDER_COMMENT;
extern const MOption MP_FOLDER_FILE_DRIVER;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_FOLDER_PATH;
extern const MOption MP_FOLDER_TREEINDEX;
extern const MOption MP_FOLDER_TYPE;
extern const MOption MP_FROM_ADDRESS;
extern const MOption MP_FVIEW_FROM_REPLACE;
extern const MOption MP_HOSTNAME;
extern const MOption MP_IMAPHOST;
extern const MOption MP_LICENSE_ACCEPTED;
extern const MOption MP_MAINFOLDER;
extern const MOption MP_MBOXDIR;
extern const MOption MP_MODULES;
extern const MOption MP_MOVE_NEWMAIL;
extern const MOption MP_NET_CONNECTION;
extern const MOption MP_NET_OFF_COMMAND;
extern const MOption MP_NET_ON_COMMAND;
extern const MOption MP_NEWMAIL_FOLDER;
extern const MOption MP_NEWMAIL_PLAY_SOUND;
extern const MOption MP_NNTPHOST;
extern const MOption MP_NNTPHOST_USE_SSL;
extern const MOption MP_NNTPHOST_USE_SSL_UNSIGNED;
extern const MOption MP_ORGANIZATION;
extern const MOption MP_OUTBOX_NAME;
extern const MOption MP_OUTGOINGFOLDER;
extern const MOption MP_PERSONALNAME;
extern const MOption MP_POPHOST;
extern const MOption MP_PROFILE_TYPE;
extern const MOption MP_SHOWTIPS;
extern const MOption MP_SHOW_NEWMAILMSG;
extern const MOption MP_SMTPHOST;
extern const MOption MP_SMTPHOST_USE_SSL;
extern const MOption MP_SMTPHOST_USE_SSL_UNSIGNED;
extern const MOption MP_SYNC_FILTERS;
extern const MOption MP_SYNC_FOLDER;
extern const MOption MP_SYNC_FOLDERGROUP;
extern const MOption MP_SYNC_FOLDERS;
extern const MOption MP_SYNC_IDS;
extern const MOption MP_SYNC_REMOTE;
extern const MOption MP_TRASH_FOLDER;
extern const MOption MP_USEOUTGOINGFOLDER;
extern const MOption MP_USEPYTHON;
extern const MOption MP_USERDIR;
extern const MOption MP_USERNAME;
extern const MOption MP_USE_OUTBOX;
extern const MOption MP_USE_SSL;
extern const MOption MP_USE_SSL_UNSIGNED;
extern const MOption MP_USE_TRASH_FOLDER;
extern const MOption MP_VERSION;

#ifdef USE_INBOX
   // the INBOX name is not translated -- should it be?
   static const wxChar *INBOX_NAME = _T("INBOX");
#endif // USE_INBOX

// obsolete config names not used any more but needed here to be able to
// update the old versions of the config

#define MP_OLD_FOLDER_HOST _T("HostName")

#define M_TEMPLATES_SECTION _T("Templates")
#define M_TEMPLATE_SECTION _T("Template")

#define M_CUSTOM_HEADERS_CONFIG_SECTION _T("CustomHeaders")

// obsolete folder flags which are not used any more but are needed here for
// the same reason as above
enum
{
   MF_FLAGS_REOPENONPING  = 0x00008000,
   MF_FLAGS_SSLAUTH       = 0x00080000,
   MF_FLAGS_SSLUNSIGNED   = 0x00200000
};

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_RETRIEVE_REMOTE;
extern const MPersMsgBox *M_MSGBOX_STORE_REMOTE;
extern const MPersMsgBox *M_MSGBOX_OVERWRITE_REMOTE;
extern const MPersMsgBox *M_MSGBOX_CONFIG_SAVED_REMOTELY;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/*
   IMPORTANT when changing Mahogany version, you need to do the following:

   0. edit Mversion.h
   1. add a new Version_XXX constant to enum MVersion below
   2. add logic for detecting it to the beginning of Upgrade()
   3. write new UpgradeFromXXX() function
   4. call it from Upgrade() (add another case to the switch)
*/

// this enum contains only versions with incompatible changes between them
enum MVersion
{
   Version_None,     // this is the first installation of M on this machine
   Version_Alpha001, // first public version
   Version_Alpha010, // some config strucutre changes (due to wxPTextEntry)
   Version_Alpha020, // folder host name is now ServerName, not HostName
   Version_050,      // nothing really changed against 0.2x config-wise
   Version_060,      // templates are organised differently
   Version_061,      // system folders have non default positions in tree
   Version_062 = Version_061, // no changes in config since 0.61
   Version_063 = Version_062, // no changes in config since 0.62
   Version_064,      // folder profiles moved, MP_PROFILE_TYPE disappeared
   Version_064_1,    // MF_FLAGS_MONITOR added, half-replaces INCOMING
   Version_064_2 = Version_064_1, // no changes
   Version_065,      // SSL flag is not boolean any more and not a flag at all
   Version_066,      // "/M/Profiles" -> "/Profiles"
   Version_067 = Version_066,     // no changes
   Version_068,
   Version_Last = Version_068,    // last existing version
   Version_Unknown   // some unrecognized version
};

#ifdef USE_WIZARD

#if 0 // ndef OS_WIN
#   define USE_HELPERS_PAGE
#endif // OS_WIN

// ids for install wizard pages
enum InstallWizardPageId
{
   InstallWizard_WelcomePage,          // say hello
   InstallWizard_ImportPage,           // propose to import settings from
                                       // another MUA
   InstallWizard_FirstPage = InstallWizard_ImportPage,
   InstallWizard_IdentityPage,         // ask name, e-mail
   InstallWizard_ServersPage,          // ask POP, SMTP, NNTP servers
   InstallWizard_OperationsPage,       // how we want Mahogany to work
#ifdef USE_DIALUP
   InstallWizard_DialUpPage,           // set up dial-up networking
#endif // USE_DIALUP

//   InstallWizard_MiscPage,             // other common options
#ifdef USE_HELPERS_PAGE
   InstallWizard_HelpersPage,          // external programs set up
#endif // USE_HELPERS_PAGE

   InstallWizard_FinalPage,            // say that everything is ok
   InstallWizard_PagesMax,             // the number of pages
   InstallWizard_Done = -1             // invalid page index
};

// ----------------------------------------------------------------------------
// module global data
// ----------------------------------------------------------------------------

static wxWizardPage *gs_wizardPages[InstallWizard_PagesMax];

// the data which is collected by the wizard
//
// FIXME for now I just use a global - this is ugly, but I just can't find a
//       place to stick it into right now
struct InstallWizardData
{
   // identity page
   String name,         // personal name
          login,        // the user name
          organization, // optional organization
          email;

   // servers page
   String pop,
          imap,
          smtp,
          nntp;

   // POP3 server option
   bool leaveOnServer;

   // operations page:
#ifdef USE_DIALUP
   int    useDialUp; // initially -1
#endif // USE_DIALUP
   bool   useOutbox;
   bool   useTrash;
   int    folderType;
#ifdef USE_PISOCK
   bool   usePalmOs;
#endif
#ifdef USE_PYTHON
   bool   usePython;
#endif
#ifdef USE_INBOX
   // mail collection from INBOX makes sense only under Unix
   bool   collectAllMail;
#endif // USE_INBOX

   // dial up page
#ifdef USE_DIALUP
#if defined(OS_WIN)
   String connection;
#elif defined(OS_UNIX)
   String dialCommand,
          hangupCommand;

   // helpers page
   String browser;
#endif // OS_UNIX
#endif // USE_DIALUP

   // did we run the wizard at all?
   bool done;

   // do we want to send a test message?
   bool sendTestMsg;

   // do we have something to import?
   int showImportPage; // logically bool but initially -1
} gs_installWizardData;

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

#ifdef USE_DNS

// check the given address for validity
static bool CheckHostName(const wxString& hostname)
{
   // check if server names are valid by verifying the hostname part (i.e.
   // discard everything after ':' which is the port number) with DNS
   return hostname.empty() || wxIPV4address().Hostname(hostname.AfterLast(':'));
}

#endif // USE_DNS

static bool VerifyStdFolders(void);

// ----------------------------------------------------------------------------
// wizardry
// ----------------------------------------------------------------------------

// the base class for our wizards pages: it allows to return page ids (and not
// the pointers themselves) from GetPrev/Next and processes [Cancel] in a
// standard way an provides other useful functions for the derived classes
class InstallWizardPage : public wxWizardPage
{
public:
   InstallWizardPage(wxWizard *wizard, InstallWizardPageId id)
      : wxWizardPage(wizard) { m_id = id; }

   // override to return the id of the next/prev page: default versions go to
   // the next/prev in order of InstallWizardPageIds
   virtual InstallWizardPageId GetPrevPageId() const;
   virtual InstallWizardPageId GetNextPageId() const;

#ifdef USE_DIALUP
   // the dial up page should be shown only if this function returns true
   static bool ShouldShowDialUpPage();
#endif // USE_DIALUP

   // the import page should be shown only if this function returns true
   static bool ShouldShowImportPage();

   // implement the wxWizardPage pure virtuals in terms of our ones
   virtual wxWizardPage *GetPrev() const
      { return GetPageById(GetPrevPageId()); }
   virtual wxWizardPage *GetNext() const
      { return GetPageById(GetNextPageId()); }

   void OnWizardCancel(wxWizardEvent& event);

   wxWizardPage *GetPageById(InstallWizardPageId id) const;

protected:
   // creates an "enhanced panel" for placing controls into under the static
   // text (explanation)
   wxEnhancedPanel *CreateEnhancedPanel(wxStaticText *text);

private:
   // id of this page
   InstallWizardPageId m_id;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(InstallWizardPage)
};

// first page: welcome the user, explain what goes on
class InstallWizardWelcomePage : public InstallWizardPage
{
public:
   InstallWizardWelcomePage(wxWizard *wizard);

   // the next page depends on whether the user want or not to use the wizard
   virtual InstallWizardPageId GetNextPageId() const;

   // process check box click
   void OnUseWizardCheckBox(wxCommandEvent& event);

private:
   wxCheckBox *m_checkbox;
   bool        m_useWizard;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(InstallWizardWelcomePage)
};

// second page: ask the basic info about the user (name, e-mail)
class InstallWizardIdentityPage : public InstallWizardPage
{
public:
   InstallWizardIdentityPage(wxWizard *wizard);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   wxTextCtrl *m_name,
              *m_login,
              *m_organization,
              *m_email;

   DECLARE_NO_COPY_CLASS(InstallWizardIdentityPage)
};

class InstallWizardServersPage : public InstallWizardPage
{
public:
   InstallWizardServersPage(wxWizard *wizard);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   void AddDomain(wxString& server, const wxString& domain);

   void OnText(wxCommandEvent& event);

private:
   wxTextCtrl *m_pop,
              *m_imap,
              *m_smtp,
              *m_nntp;

   wxCheckBox *m_leaveOnServer;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(InstallWizardServersPage)
};

#ifdef USE_DIALUP

class InstallWizardDialUpPage : public InstallWizardPage
{
public:
   InstallWizardDialUpPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

private:
#if defined(OS_WIN)
   bool m_firstShow;
   wxComboBox *m_connections;
#elif defined(OS_UNIX)
   wxTextCtrl *m_connect,
              *m_disconnect;
#endif // platform

   DECLARE_NO_COPY_CLASS(InstallWizardDialUpPage)
};

#endif // USE_DIALUP

class InstallWizardOperationsPage : public InstallWizardPage
{
public:
   InstallWizardOperationsPage(wxWizard *wizard);

   virtual bool TransferDataToWindow()
      {
#ifdef USE_DIALUP
         // no setting yet?
         if ( gs_installWizardData.useDialUp == -1 )
         {
#ifdef OS_WIN
            // disabling this code for Windows because the program crashes in
            // release version after returning from IsAlwaysOnline(): I
            // strongly suspect an optimizer bug (it doesn't happen without
            // optimizations) but I can't fix it right now otherwise
            gs_installWizardData.useDialUp = false;
#else // !Win
            wxDialUpManager *dialupMan = wxDialUpManager::Create();

            // if we have a LAN connection, then we don't need to configure
            // dial-up networking, but if we don't, then we probably do
            gs_installWizardData.useDialUp = dialupMan &&
                                                !dialupMan->IsAlwaysOnline();

            delete dialupMan;
#endif // Win/!Win
         }
#endif // USE_DIALUP

         m_FolderTypeChoice->SetSelection(gs_installWizardData.folderType);
#ifdef USE_PYTHON
         m_UsePythonCheckbox->SetValue(gs_installWizardData.usePython != 0);
#endif // USE_PYTHON
#ifdef USE_DIALUP
         m_UseDialUpCheckbox->SetValue(gs_installWizardData.useDialUp != 0);
#endif // USE_DIALUP
#ifdef USE_PISOCK
         m_UsePalmOsCheckbox->SetValue(gs_installWizardData.usePalmOs != 0);
#endif // USE_PISOCK
         m_UseOutboxCheckbox->SetValue(gs_installWizardData.useOutbox != 0);
         m_TrashCheckbox->SetValue(gs_installWizardData.useTrash != 0);
#ifdef USE_INBOX
         m_CollectCheckbox->SetValue(gs_installWizardData.collectAllMail != 0);
#endif // USE_INBOX

         return true;
      }

   virtual bool TransferDataFromWindow()
      {
         gs_installWizardData.folderType  = m_FolderTypeChoice->GetSelection();
#ifdef USE_PYTHON
         gs_installWizardData.usePython  = m_UsePythonCheckbox->GetValue();
#endif // USE_PYTHON
#ifdef USE_PISOCK
         gs_installWizardData.usePalmOs  = m_UsePalmOsCheckbox->GetValue();
#endif // USE_PISOCK
#ifdef USE_DIALUP
         gs_installWizardData.useDialUp  = m_UseDialUpCheckbox->GetValue();
#endif // USE_DIALUP
         gs_installWizardData.useOutbox  = m_UseOutboxCheckbox->GetValue();
         gs_installWizardData.useTrash   = m_TrashCheckbox->GetValue();
#ifdef USE_INBOX
         gs_installWizardData.collectAllMail = m_CollectCheckbox->GetValue();
#endif // USE_INBOX

         return true;
      }
private:
   wxChoice *m_FolderTypeChoice;
   wxCheckBox *m_TrashCheckbox,
              *m_UseOutboxCheckbox
#ifdef USE_DIALUP
             , *m_UseDialUpCheckbox
#endif // USE_DIALUP
#ifdef USE_INBOX
             , *m_CollectCheckbox
#endif // USE_INBOX
#ifdef USE_PISOCK
             , *m_UsePalmOsCheckbox
#endif
#ifdef USE_PYTHON
             , *m_UsePythonCheckbox
#endif
             ;

   DECLARE_NO_COPY_CLASS(InstallWizardOperationsPage)
};

#ifdef USE_HELPERS_PAGE

class InstallWizardHelpersPage : public InstallWizardPage
{
public:
   InstallWizardHelpersPage(wxWizard *wizard);
};

#endif // USE_HELPERS_PAGE

class InstallWizardMiscPage : public InstallWizardPage
{
public:
   InstallWizardMiscPage(wxWizard *wizard);

private:
   DECLARE_NO_COPY_CLASS(InstallWizardMiscPage)
};

class InstallWizardImportPage : public InstallWizardPage
{
public:
   InstallWizardImportPage(wxWizard *wizard);

   void OnImportButton(wxCommandEvent& event);

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(InstallWizardImportPage)
};

class InstallWizardFinalPage : public InstallWizardPage
{
public:
   InstallWizardFinalPage(wxWizard *wizard);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   wxCheckBox *m_checkboxSendTestMsg;

   DECLARE_NO_COPY_CLASS(InstallWizardFinalPage)
};

// ----------------------------------------------------------------------------
// prototypes
// ----------------------------------------------------------------------------

// the function which runs the install wizard
extern bool RunInstallWizard(
#ifdef DEBUG
                             bool justTest = false
#endif // DEBUG
                            );

#endif // USE_WIZARD

// ============================================================================
// wizard pages implementation
// ============================================================================

#ifdef USE_WIZARD

// ----------------------------------------------------------------------------
// InstallWizardPage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(InstallWizardPage, wxWizardPage)
   EVT_WIZARD_CANCEL(-1, InstallWizardPage::OnWizardCancel)
END_EVENT_TABLE()

#ifdef USE_DIALUP

bool InstallWizardPage::ShouldShowDialUpPage()
{
   return gs_installWizardData.useDialUp != 0;
}

#endif // USE_DIALUP

bool InstallWizardPage::ShouldShowImportPage()
{
   if ( gs_installWizardData.showImportPage == -1 )
      gs_installWizardData.showImportPage = HasImporters();

   return gs_installWizardData.showImportPage != 0;
}

InstallWizardPageId InstallWizardPage::GetPrevPageId() const
{
   int id = m_id - 1;
   if (
#ifdef USE_DIALUP
        (id == InstallWizard_DialUpPage && !ShouldShowDialUpPage()) ||
#endif // USE_DIALUP
        (id == InstallWizard_ImportPage && !ShouldShowImportPage()) )
   {
      // skip it
      id--;
   }

   return id < 0 ? InstallWizard_Done : (InstallWizardPageId)id;
}

InstallWizardPageId InstallWizardPage::GetNextPageId() const
{
   int id = m_id + 1;
   if (
#ifdef USE_DIALUP
        (id == InstallWizard_DialUpPage && !ShouldShowDialUpPage()) ||
#endif // USE_DIALUP
        (id == InstallWizard_ImportPage && !ShouldShowImportPage()) )
   {
      // skip it
      id++;
   }

   return id < InstallWizard_PagesMax ? (InstallWizardPageId) id
                                      : InstallWizard_Done;
}

void InstallWizardPage::OnWizardCancel(wxWizardEvent& event)
{
   if ( !MDialog_YesNoDialog(_("Do you really want to abort the wizard?")) )
   {
      // not confirmed
      event.Veto();
   }
   else
   {
      // wizard will be cancelled, so don't try to test anything
      wxLogMessage(_("Please use the 'Options' dialog to configure\n"
                     "the program before using it!"));
   }
}

wxWizardPage *InstallWizardPage::GetPageById(InstallWizardPageId id) const
{
   if ( id == InstallWizard_Done )
      return (wxWizardPage *)NULL;

   if ( !gs_wizardPages[id] )
   {
#define CREATE_PAGE(id)                                              \
      case InstallWizard_##id##Page:                              \
         gs_wizardPages[InstallWizard_##id##Page] =               \
            new InstallWizard##id##Page((wxWizard *)GetParent()); \
         break

      switch ( id )
      {
         CREATE_PAGE(Import);
         CREATE_PAGE(Identity);
         CREATE_PAGE(Servers);
         CREATE_PAGE(Operations);
#ifdef USE_DIALUP
         CREATE_PAGE(DialUp);
#endif // USE_DIALUP

//         CREATE_PAGE(Misc);
#ifdef USE_HELPERS_PAGE
         CREATE_PAGE(Helpers);
#endif // USE_HELPERS_PAGE

         CREATE_PAGE(Final);

      case InstallWizard_WelcomePage:
      case InstallWizard_Done:
      case InstallWizard_PagesMax:
         ASSERT(0);
      }

#undef CREATE_PAGE
   }

   return gs_wizardPages[id];
}

wxEnhancedPanel *InstallWizardPage::CreateEnhancedPanel(wxStaticText *text)
{
   wxEnhancedPanel *panel = new wxEnhancedPanel(this, true /* scrolling */);

   wxBoxSizer *pageSizer = new wxBoxSizer(wxVERTICAL);
   SetSizer(pageSizer);

   pageSizer->Add(
      text,
      0, // No vertical stretching
      wxALL, // Border all around, no horizontal stretching
      5 // Border width
   );

   pageSizer->Add(
      panel,
      1, // Vertical stretching
      wxEXPAND // No border, horizontal stretching
   );

   panel->SetAutoLayout(true);

   return panel;
}

// InstallWizardWelcomePage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(InstallWizardWelcomePage, InstallWizardPage)
   EVT_CHECKBOX(-1, InstallWizardWelcomePage::OnUseWizardCheckBox)
END_EVENT_TABLE()

InstallWizardWelcomePage::InstallWizardWelcomePage(wxWizard *wizard)
                        : InstallWizardPage(wizard, InstallWizard_WelcomePage)
{
   m_useWizard = true;

   wxStaticText *introduction =
   new wxStaticText(this, -1, _(
      "Welcome to Mahogany!\n"
      "\n"
      "This wizard will help you to setup the most\n"
      "important settings needed to successfully use\n"
      "the program. You don't need to specify all of\n"
      "them here and now -- you can always change all\n"
      "the program options later from the \"Preferences\"\n"
      "dialog accessible via the \"Edit\" menu.\n"
      "\n"
      "However, the wizard may be helpful to setup a\n"
      "working default configuration and we advise you\n"
      "to complete it, especially if you are new to\n"
      "Mahogany, so please take time to complete it.\n"
      "\n"
      "If you still decide to not use it, just check\n"
      "the box below or press [Cancel] at any moment."
                                         ));

   m_checkbox = new wxCheckBox
                (
                  this, -1,
                  _("I'm an &expert and don't need the wizard")
                );

   wxBoxSizer *pageSizer = new wxBoxSizer(wxVERTICAL);
   pageSizer->Add(
      introduction,
      0, // No vertical stretching
      wxALL, // Border all around, no horizontal stretching
      5 // Border width
   );
   pageSizer->Add(
      m_checkbox,
      0, // No vertical stretching
      wxALL, // Border all around
      5 // Border width
   );

   SetSizer(pageSizer);
   pageSizer->Fit(this);
}

InstallWizardPageId InstallWizardWelcomePage::GetNextPageId() const
{
   // override the default logic if the user chose to skip the qizard entirely
   if ( !m_useWizard )
   {
      // remember that we didn't really run the wizard
      gs_installWizardData.done = false;

      return InstallWizard_Done;
   }
   else
   {
      return InstallWizardPage::GetNextPageId();
   }
}

void InstallWizardWelcomePage::OnUseWizardCheckBox(wxCommandEvent&)
{
   m_useWizard = !m_checkbox->GetValue();

   wxButton *btn = (wxButton *)GetParent()->FindWindow(wxID_FORWARD);
   if ( btn )
   {
      if ( m_useWizard )
         btn->SetLabel(_("&Next >"));
      else
         btn->SetLabel(_("&Finish"));
   }
}

// InstallWizardImportPage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(InstallWizardImportPage, InstallWizardPage)
   EVT_BUTTON(-1, InstallWizardImportPage::OnImportButton)
END_EVENT_TABLE()

InstallWizardImportPage::InstallWizardImportPage(wxWizard *wizard)
                       : InstallWizardPage(wizard, InstallWizard_ImportPage)
{
   wxStaticText *text = new wxStaticText(this, -1,
         _("Mahogany has detected that you have one or\n"
           "more other email programs installed on this\n"
           "computer.\n"
           "\n"
           "You may click the button below to invoke a\n"
           "dialog which will allow you to import some\n"
           "configuration settings (your personal name,\n"
           "email address, folders, address books &&c)\n"
           "from one or more of them.\n"
           "\n"
           "You may safely skip this step if you don't\n"
           "want to do it."));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);
   panel->CreateButton(_("&Import..."), NULL);

   GetSizer()->Fit(this);
}

void InstallWizardImportPage::OnImportButton(wxCommandEvent&)
{
   ShowImportDialog(this);
}

// ----------------------------------------------------------------------------
// InstallWizardIdentityPage
// ----------------------------------------------------------------------------

InstallWizardIdentityPage::InstallWizardIdentityPage(wxWizard *wizard)
                         : InstallWizardPage(wizard, InstallWizard_IdentityPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
         "Please specify your e-mail address which will\n"
         "be used for the outgoing messages.\n"
         "\n"
         "The personal name and organization are used\n"
         "for informational purposes only and may be\n"
         "left empty.\n"
         "\n"
         "Finally, the user name will be used as default\n"
         "login to your POP, IMAP and/or SMTP servers."
         ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;
   labels.Add(_("&Personal name:"));
   labels.Add(_("&Organization:"));
   labels.Add(_("&User name/login:"));
   labels.Add(_("&E-mail:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_name = panel->CreateTextWithLabel(labels[0], widthMax, NULL);
   m_organization = panel->CreateTextWithLabel(labels[1], widthMax, m_name);
   m_login = panel->CreateTextWithLabel(labels[2], widthMax, m_organization);
   m_email = panel->CreateTextWithLabel(labels[3], widthMax, m_login);

   GetSizer()->Fit(this);
}

bool InstallWizardIdentityPage::TransferDataToWindow()
{
   // the first time the page is shown, construct the reasonable default
   // value
   if ( gs_installWizardData.name.empty() )
      gs_installWizardData.name = READ_APPCONFIG_TEXT(MP_PERSONALNAME);

   if ( gs_installWizardData.organization.empty() )
      gs_installWizardData.organization = READ_APPCONFIG_TEXT(MP_ORGANIZATION);

   if ( gs_installWizardData.login.empty() )
      gs_installWizardData.login = READ_APPCONFIG_TEXT(MP_USERNAME);

   if ( gs_installWizardData.email.empty() )
   {
      gs_installWizardData.email = READ_APPCONFIG_TEXT(MP_FROM_ADDRESS);

      if ( gs_installWizardData.email.empty() )
      {
         gs_installWizardData.email << gs_installWizardData.login
                                    << '@'
                                    << READ_APPCONFIG_TEXT(MP_HOSTNAME);
      }
   }

   m_name->SetValue(gs_installWizardData.name);
   m_organization->SetValue(gs_installWizardData.organization);
   m_login->SetValue(gs_installWizardData.login);
   m_email->SetValue(gs_installWizardData.email);

   return true;
}

bool InstallWizardIdentityPage::TransferDataFromWindow()
{
   gs_installWizardData.email = m_email->GetValue();

   if ( gs_installWizardData.email.empty() )
   {
      wxLogError(_("Please specify a valid email address."));
      m_email->SetFocus();

      return false;
   }

   gs_installWizardData.name = m_name->GetValue();
   gs_installWizardData.login = m_login->GetValue();
   gs_installWizardData.organization = m_organization->GetValue();

   return true;
}

// ----------------------------------------------------------------------------
// InstallWizardServersPage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(InstallWizardServersPage, InstallWizardPage)
   EVT_TEXT(-1, InstallWizardServersPage::OnText)
END_EVENT_TABLE()

InstallWizardServersPage::InstallWizardServersPage(wxWizard *wizard)
                        : InstallWizardPage(wizard, InstallWizard_ServersPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
      "You need an IMAP4 (preferred) or a POP3 server\n"
      "to be able to receive email and an SMTP server\n"
      "to be able to send it.\n"
      "\n"
      "All of these fields may be filled later as well\n"
      "and you may add additional servers later too.\n"
      "You also may leave a field empty if you don't\n"
      "want to use the corresponding server."
      ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;
   labels.Add(_("&IMAP server:"));
   labels.Add(_("&POP server:"));
   labels.Add(_("&Leave mail on it:"));
   labels.Add(_("&SMTP server:"));
   labels.Add(_("&NNTP server:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_imap = panel->CreateTextWithLabel(labels[0], widthMax, NULL);
   m_pop = panel->CreateTextWithLabel(labels[1], widthMax, m_imap);
   m_leaveOnServer = panel->CreateCheckBox(labels[2], widthMax, m_pop);
   m_smtp = panel->CreateTextWithLabel(labels[3], widthMax, m_leaveOnServer);
   m_nntp = panel->CreateTextWithLabel(labels[4], widthMax, m_smtp);

   GetSizer()->Fit(this);
}

bool InstallWizardServersPage::TransferDataToWindow()
{
   // use the user email address as the default domain for the servers -
   // this i not ideal but has the best chance to work
   wxString domain = gs_installWizardData.email.AfterFirst('@');
   if ( !domain.empty() )
   {
      AddDomain(gs_installWizardData.pop, domain);
      AddDomain(gs_installWizardData.imap, domain);
      AddDomain(gs_installWizardData.smtp, domain);
      AddDomain(gs_installWizardData.nntp, domain);
   }

   m_pop->SetValue(gs_installWizardData.pop);
   m_imap->SetValue(gs_installWizardData.imap);
   m_smtp->SetValue(gs_installWizardData.smtp);
   m_nntp->SetValue(gs_installWizardData.nntp);

   return true;
}

bool InstallWizardServersPage::TransferDataFromWindow()
{
   gs_installWizardData.imap = m_imap->GetValue();
   gs_installWizardData.pop  = m_pop->GetValue();
   gs_installWizardData.leaveOnServer = m_leaveOnServer->GetValue();
   gs_installWizardData.smtp = m_smtp->GetValue();
   gs_installWizardData.nntp = m_nntp->GetValue();

   if ( gs_installWizardData.smtp.empty() )
   {
#ifdef OS_UNIX
      // we could use an MTA under Unix instead...
      wxLogWarning(_("You will need to specify the MTA to use for "
                     "mail delivery later to be able to send email "
                     "as you didn't choose the SMTP server."));
#else // !Unix
      wxLogError(_("You need to specify the SMTP server to be able "
                   "to send email, please do it!"));
      return false;
#endif // Unix/!Unix
   }

#ifdef USE_DNS
   // check all the hostnames unless we use dial up - we can't call
   // MApp::IsOnline() here yet because this stuff is not yet
   // configured, so use wxDialUpManager directly
   wxDialUpManager *dialupMan =
            ((wxMApp *)mApplication)->GetDialUpManager();
   if ( dialupMan->IsOnline() )
   {
      String check, tmp;
      int failed = 0;
      if( !CheckHostName(gs_installWizardData.pop) )
      {
         failed++;
         tmp.Printf(_("POP3 server '%s'.\n"),
                    gs_installWizardData.pop.c_str());
         check += tmp;
      }
      if( !CheckHostName(gs_installWizardData.smtp) )
      {
         failed++;
         tmp.Printf(_("SMTP server '%s'.\n"),
                    gs_installWizardData.smtp.c_str());
         check += tmp;
      }
      if( !CheckHostName(gs_installWizardData.imap) )
      {
         failed++;
         tmp.Printf(_("IMAP server '%s'.\n"),
                    gs_installWizardData.imap.c_str());
         check += tmp;
      }
      if( !CheckHostName(gs_installWizardData.nntp) )
      {
         failed++;
         tmp.Printf(_("NNTP server '%s'.\n"),
                    gs_installWizardData.nntp.c_str());
         check += tmp;
      }
      if(failed)
      {
         tmp.Printf(_("%d of the server names specified could not\n"
                      "be resolved. This could be due to a temporary\n"
                      "network problem, or because the server name really\n"
                      "does not exist. If you use dialup-networking and\n"
                      "are not currently connected, this is perfectly normal.\n"
                      "The failed server name(s) were:\n"),
                    failed);
         check = tmp + check;
         check += _("\nDo you want to change these settings?");
         if( MDialog_YesNoDialog(check,this,
                                 _("Potentially wrong server names"),
                                 M_DLG_YES_DEFAULT) )
            return false;
      }
   }
#endif // USE_DNS

   return true;
}

void
InstallWizardServersPage::AddDomain(wxString& server, const wxString& domain)
{
   // don't add the domain to the host names which already contain it
   // and for the empty host names
   if ( !server || server.Find('.') != wxNOT_FOUND )
      return;

#if 0 // VZ: this is annoying! either don't do it all or do without asking
   wxString msg;
   msg.Printf(_("You have no domain specified for the server '%s'.\n"
                "Do you want to add the domain '%s'?"),
              server.c_str(), domain.c_str());
   if(MDialog_YesNoDialog(msg,this, MDIALOG_YESNOTITLE, true))
#endif // 0

   server << '.' << domain;
}

void InstallWizardServersPage::OnText(wxCommandEvent& event)
{
   if ( event.GetEventObject() == m_pop )
   {
      m_leaveOnServer->Enable(!m_pop->GetValue().empty());
   }

   event.Skip();
}

#ifdef USE_DIALUP

// InstallWizardDialUpPage
// ----------------------------------------------------------------------------

InstallWizardDialUpPage::InstallWizardDialUpPage(wxWizard *wizard)
                       : InstallWizardPage(wizard, InstallWizard_DialUpPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
      "Mahogany can automatically detect if your\n"
      "network connection is online or offline.\n"
      "It can also connect and disconnect you to the\n"
      "network, but for this you need to specify\n"
      "the informations below:"));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;

#if defined(OS_WIN)
   // do create controls now
   long widthMax = GetMaxLabelWidth(labels, panel);

   m_connections = panel->CreateComboBox(_("&Dial up connection to use"),
      widthMax, NULL);

   m_firstShow = true;
#elif defined(OS_UNIX)
   labels.Add(_("Command to &connect:"));
   labels.Add(_("Command to &disconnect:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_connect = panel->CreateTextWithLabel(labels[0], widthMax, NULL);
   m_disconnect = panel->CreateTextWithLabel(labels[1], widthMax, m_connect);
#endif // platform

   GetSizer()->Fit(this);
}

bool InstallWizardDialUpPage::TransferDataFromWindow()
{
#if defined(OS_WIN)
   gs_installWizardData.connection = m_connections->GetValue();
#elif defined(OS_UNIX)
   gs_installWizardData.dialCommand = m_connect->GetValue();
   gs_installWizardData.hangupCommand = m_disconnect->GetValue();
#endif // platform

   return true;
}

bool InstallWizardDialUpPage::TransferDataToWindow()
{
#if defined(OS_WIN)
   if(m_firstShow)
   {
      m_firstShow = false;

      // get all existing RAS connections
      wxDialUpManager *dial = wxDialUpManager::Create();

      wxArrayString connections;
      size_t nCount;
      if ( !dial )
      {
         FAIL_MSG( _T("GetDialUpManager() returned NULL?") );
         nCount = 0;
      }
      else
      {
         nCount = dial->GetISPNames(connections);

         delete dial;
      }

      for ( size_t n = 0; n < nCount; n++ )
      {
         m_connections->Append(connections[n]);
      }
   }

   if ( !gs_installWizardData.connection.empty() )
      m_connections->SetValue(gs_installWizardData.connection);
#elif defined(OS_UNIX)
   m_connect->SetValue(gs_installWizardData.dialCommand);
   m_disconnect->SetValue(gs_installWizardData.hangupCommand);
#endif // platform

   return true;
}

#endif // USE_DIALUP

// InstallWizardOperationsPage
// ----------------------------------------------------------------------------

InstallWizardOperationsPage::InstallWizardOperationsPage(wxWizard *wizard)
                        : InstallWizardPage(wizard, InstallWizard_OperationsPage)
{

   wxStaticText *itext = new wxStaticText(this, -1, _(
      "This page contains some of the basic options\n"
      "controlling Mahogany's operation. Please take\n"
      "a moment to check that these settings are as\n"
      "you prefer them."));

   wxEnhancedPanel *panel = CreateEnhancedPanel(itext);

   // the enum should be synced with the labels below!
   enum
   {
#ifdef USE_INBOX
      Label_Collect,
#endif // USE_INBOX
      Label_UseTrash,
      Label_UseOutbox,
#ifdef USE_DIALUP
      Label_UseDialUp,
#endif // USE_DIALUP
#ifdef USE_PISOCK
      Label_UsePalmOS,
#endif // USE_PISOCK
      Label_MbxFormat,
#ifdef USE_PYTHON
      Label_UsePython,
#endif // USE_PYTHON
      Label_Max
   };

   wxArrayString labels;
#ifdef USE_INBOX
   labels.Add(_("&Collect new mail:"));
#endif // USE_INBOX
   labels.Add(_("Use &Trash mailbox:"));
   labels.Add(_("Use &Outbox queue:"));
#ifdef USE_DIALUP
   labels.Add(_("&Use dial-up network:"));
#endif // USE_DIALUP
#ifdef USE_PISOCK
   labels.Add(_("&Load PalmOS support:"));
#endif // USE_PISOCK
   labels.Add(_("Default &mailbox format"));
#ifdef USE_PYTHON
   labels.Add(_("Enable &Python:"));
#endif // USE_PYTHON

   long widthMax = GetMaxLabelWidth(labels, panel);

   wxControl *last = NULL;

#ifdef USE_INBOX
   wxStaticText *text = panel->CreateMessage(_(
      "Mahogany can either leave all messages in\n"
      "your system mailbox or create its own\n"
      "mailbox for new mail and move all new\n"
      "messages there. This is recommended,\n"
      "especially in a multi-user environment."), last);

   m_CollectCheckbox = panel->CreateCheckBox(labels[Label_Collect],
                                             widthMax, text);
   last = m_CollectCheckbox;
#endif // USE_INBOX

   wxStaticText *text2 = panel->CreateMessage(
      _(
         "\n"
         "Mahogany has two options for deleting\n"
         "messages. You can either mark messages\n"
         "as deleted and leave them around to be\n"
         "expunged later, or you can use a Trash\n"
         "folder where to move them to."
         ), last);

   m_TrashCheckbox = panel->CreateCheckBox(labels[Label_UseTrash],
                                           widthMax, text2);

   wxStaticText *text3 = panel->CreateMessage(
      _(
         "\n"
         "Mahogany can either send messages\n"
         "immediately or queue them and only\n"
         "send them on demand. This is mostly\n"
         "useful for dial-up networking."
         ), m_TrashCheckbox);
   m_UseOutboxCheckbox = panel->CreateCheckBox(labels[Label_UseOutbox],
                                               widthMax, text3);

   last = m_UseOutboxCheckbox;

#ifdef USE_DIALUP
   wxStaticText *text4 = panel->CreateMessage(
      _(
         "\n"
         "If you are using dial-up networking,\n"
         "Mahogany may detect your connection status\n"
         "and optionally dial and hang-up."
         ), last);
   m_UseDialUpCheckbox = panel->CreateCheckBox(labels[Label_UseDialUp],
                                               widthMax, text4);
   last = m_UseDialUpCheckbox;
#endif // USE_DIALUP

#ifdef USE_PISOCK
   wxStaticText *text5 = panel->CreateMessage(
      _(
         "\n"
         "Do you have a PalmOS based handheld computer?\n"
         "Mahogany has special support build in to connect\n"
         "to these."
         ), last);
   m_UsePalmOsCheckbox = panel->CreateCheckBox(labels[Label_UsePalmOS],
                                               widthMax, text5);
   last = m_UsePalmOsCheckbox;
#endif // USE_PISOCK

   wxString tmp;
   tmp << labels[Label_MbxFormat]
       << _T(":Unix mbx mailbox:Unix mailbox:MMDF (SCO Unix):Tenex (Unix MM format)");
   m_FolderTypeChoice = panel->CreateChoice(tmp, widthMax, last);
   last = m_FolderTypeChoice;

#ifdef USE_PYTHON
   wxStaticText *text6 = panel->CreateMessage(
      _(
         "\n"
         "Mahogany contains a built-in interpreter for\n"
         "the scripting language `Python'.\n"
         "This can be used to further customise and\n"
         "expand Mahogany.\n"
         "Would you like to enable it?"), last);

   m_UsePythonCheckbox = panel->CreateCheckBox(labels[Label_UsePython],
                                               widthMax, text6);
#endif // USE_PYTHON

   GetSizer()->Fit(this);
}

#ifdef USE_HELPERS_PAGE

// InstallWizardHelpersPage
// ----------------------------------------------------------------------------
/*
InstallWizardHelpersPage::InstallWizardHelpersPage(wxWizard *wizard)
                        : InstallWizardPage(wizard, InstallWizard_HelpersPage)
{
   // TODO ask for the web browser
   new wxStaticText(this, -1, "This page is under construction");
}
*/
#endif // USE_HELPERS_PAGE

// InstallWizardMiscPage
// ----------------------------------------------------------------------------
/*
InstallWizardMiscPage::InstallWizardMiscPage(wxWizard *wizard)
                     : InstallWizardPage(wizard, InstallWizard_MiscPage)
{
   new wxStaticText(this, -1, "This page is under construction");
}
*/

// InstallWizardFinalPage
// ----------------------------------------------------------------------------

InstallWizardFinalPage::InstallWizardFinalPage(wxWizard *wizard)
                      : InstallWizardPage(wizard, InstallWizard_FinalPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
      "Congratulations!\n"
      "You have successfully configured\n"
      "Mahogany and may now start using it.\n"
      "\n"
      "In case of a problem, consult the help\n"
      "system and, if you cannot resolve it,\n"
      "subscribe to m-users mailing list at\n"
      "   http://mahogany.sourceforge.net/\n"
      "\n"
      "We hope you will enjoy using Mahogany!\n"
      "                    The M-Team"));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   // we can only send test message if we have the SMTP server configured
   if ( !gs_installWizardData.smtp.empty() )
   {
      wxArrayString labels;
      labels.Add(_("&Send test message:"));

      long widthMax = GetMaxLabelWidth(labels, panel);

      text = panel->CreateMessage(_(
         "\n"
         "Finally, it is advised that you test your\n"
         "configuration by sending a test message to\n"
         "yourself. Please uncheck the checkbox below\n"
         "if you don't want to do it."), NULL);
      m_checkboxSendTestMsg = panel->CreateCheckBox(labels[0], widthMax, text);
   }
   else
   {
      String msg = _("\nPlease don't forget to configure SMTP server\n");
#ifdef OS_UNIX
      msg += _("(or a MTA for local mail delivery)\n");
#endif // OS_UNIX
      msg += _("to be able to send the outgoing messages!");

      text = panel->CreateMessage(msg, NULL);

      m_checkboxSendTestMsg = NULL;
   }

   GetSizer()->Fit(this);
}

bool InstallWizardFinalPage::TransferDataToWindow()
{
   if ( m_checkboxSendTestMsg )
   {
      m_checkboxSendTestMsg->SetValue(gs_installWizardData.sendTestMsg);
   }

   return true;
}

bool InstallWizardFinalPage::TransferDataFromWindow()
{
   gs_installWizardData.sendTestMsg = m_checkboxSendTestMsg
                                       ? m_checkboxSendTestMsg->GetValue()
                                       : false;

   return true;
}

#endif // USE_WIZARD

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static wxString GetRFC822Time(void)
{
   // we don't use strftime() here as we need the untranslated month
   // names
   time_t tt;
   time(&tt);
   struct tm *ourtime = localtime(&tt);
   String timeStr;
   switch(ourtime->tm_mon)
   {
   case  0: timeStr = _T("Jan"); break;
   case  1: timeStr = _T("Feb"); break;
   case  2: timeStr = _T("Mar"); break;
   case  3: timeStr = _T("Apr"); break;
   case  4: timeStr = _T("May"); break;
   case  5: timeStr = _T("Jun"); break;
   case  6: timeStr = _T("Jul"); break;
   case  7: timeStr = _T("Aug"); break;
   case  8: timeStr = _T("Sep"); break;
   case  9: timeStr = _T("Oct"); break;
   case 10: timeStr = _T("Nov"); break;
   case 11: timeStr = _T("Dec"); break;
   default:
      ; // suppress warning
   }
   timeStr.Printf(_T("%02d %s %d %02d:%02d:%02d"),
                  ourtime->tm_mday,
                  timeStr.c_str(),
                  ourtime->tm_year+1900,
                  ourtime->tm_hour,
                  ourtime->tm_min,
                  ourtime->tm_sec);
   return timeStr;
}


#ifdef USE_WIZARD
static
void CompleteConfiguration();

static void SetupServers(void);

bool RunInstallWizard(
#ifdef DEBUG
                      bool justTest
#endif // DEBUG
                     )
{
   // as we use a static array, make sure that only one install wizard is
   // running at any time (a sensible requirment anyhow)
   static bool gs_isWizardRunning = false;

   if ( gs_isWizardRunning )
   {
      FAIL_MSG( _T("the installation wizard is already running!") );

      return false;
   }

   gs_isWizardRunning = true;

   // first, set up the default values for the wizard:

   gs_installWizardData.showImportPage = -1;

#ifdef USE_DIALUP
   gs_installWizardData.useDialUp = -1;
#if defined(OS_WIN)
   gs_installWizardData.connection = READ_APPCONFIG_TEXT(MP_NET_CONNECTION);
#elif defined(OS_UNIX)
   gs_installWizardData.dialCommand = READ_APPCONFIG_TEXT(MP_NET_ON_COMMAND);
   gs_installWizardData.hangupCommand = READ_APPCONFIG_TEXT(MP_NET_OFF_COMMAND);
#endif // platform
#endif // USE_DIALUP

   gs_installWizardData.useOutbox = GetNumericDefault(MP_USE_OUTBOX) != 0;
   gs_installWizardData.useTrash = GetNumericDefault(MP_USE_TRASH_FOLDER) != 0;
#ifdef USE_INBOX
   gs_installWizardData.collectAllMail = true;
#endif // USE_INBOX
   gs_installWizardData.folderType = 0; /* mbx */
#ifdef USE_PYTHON
   gs_installWizardData.usePython = READ_APPCONFIG_BOOL(MP_USEPYTHON);
#endif
#ifdef USE_PISOCK
   gs_installWizardData.usePalmOs = true;
#endif
   gs_installWizardData.sendTestMsg = true;

   gs_installWizardData.pop  = READ_APPCONFIG_TEXT(MP_POPHOST);
   gs_installWizardData.imap = READ_APPCONFIG_TEXT(MP_IMAPHOST);
   gs_installWizardData.smtp = READ_APPCONFIG_TEXT(MP_SMTPHOST);
   gs_installWizardData.nntp = READ_APPCONFIG_TEXT(MP_NNTPHOST);

   gs_installWizardData.leaveOnServer = false;

   // assume we don't skip the wizard by default
   gs_installWizardData.done = true;

   wxIconManager *iconManager = mApplication->GetIconManager();
   wxWizard *wizard = new wxWizard
                      (
                        NULL,                         // parent
                        -1,                           // id
                        _("Mahogany Installation"),   // title
                        iconManager->GetBitmap(_T("install_welcome")), // def image
                        wxDefaultPosition,
                        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER
                      );

   // NULL the pages array
   memset(gs_wizardPages, 0, sizeof(gs_wizardPages));

   InstallWizardWelcomePage *welcomePage
      = new InstallWizardWelcomePage(wizard);

   gs_wizardPages[InstallWizard_WelcomePage] = welcomePage;

   // we need to add to the sizer all pages not reachable from the initial one
   // by calling GetNext()
   wizard->GetPageAreaSizer()->Add(welcomePage);
   #ifdef USE_DIALUP
      wizard->GetPageAreaSizer()->Add(
            welcomePage->GetPageById(InstallWizard_DialUpPage)
      );
   #endif // USE_DIALUP

   // the wizard may be either cancelled or a checkbox may be used to skip it
   // entirely (besides, this is confusing - the checkbox is probably useless,
   // except that it allows to cancel it without having to answer the
   // confirmation...)
   bool wizardDone = false;
   if ( wizard->RunWizard(gs_wizardPages[InstallWizard_WelcomePage]) )
   {
      wizardDone = gs_installWizardData.done;
   }

   wizard->Destroy();

   gs_isWizardRunning = false;

#ifdef DEBUG
   if ( justTest )
      return true;
#endif // DEBUG

   // make sure we have some _basic_ things set up whether the wizard ran or
   // not (basic here meaning that the program will not operate properly
   // without any of them)
   Profile *profile = mApplication->GetProfile();
   if ( wizardDone )
   {
      // load all modules by default:
      wxString modules = _T("Filters");
#ifdef USE_PISOCK
      if(gs_installWizardData.usePalmOs)
         modules += _T(":PalmOS");
#endif // USE_PISOCK

      profile->writeEntry(MP_MODULES, modules);
   }
   else
   {
      // assume the simplest possible values for everything
      gs_installWizardData.useOutbox = false;
      gs_installWizardData.useTrash = false;
#ifdef USE_DIALUP
      gs_installWizardData.useDialUp = false;
#endif // USE_DIALUP
#if USE_PYTHON
      gs_installWizardData.usePython = false;
#endif // USE_PYTHON
#if USE_PISOCK
      gs_installWizardData.usePalmOs = false;
#endif // USE_PISOCK
#ifdef USE_INBOX
      gs_installWizardData.collectAllMail = false;
#endif // USE_INBOX

      // don't reset the SMTP server: it won't lead to creation of any
      // (unwanted) folders, so it doesn't hurt to have the default value
      gs_installWizardData.pop =
      gs_installWizardData.imap =
      gs_installWizardData.nntp = wxEmptyString;

      // don't try to send the test message if the SMTP server wasn't
      // configured
      gs_installWizardData.sendTestMsg = false;

      // also, don't show the tips for the users who skip the wizard
      profile->writeEntry(MP_SHOWTIPS, 0l);
   }

   // Don't reset defaults when running wizard second time and stopping it
   // No effect on users, but it's awfully annoying when testing
   if(gs_wizardPages[InstallWizard_IdentityPage])
   {
      // transfer the wizard settings from InstallWizardData
      profile->writeEntry(MP_FROM_ADDRESS, gs_installWizardData.email);
      profile->writeEntry(MP_PERSONALNAME, gs_installWizardData.name);
      profile->writeEntry(MP_ORGANIZATION, gs_installWizardData.organization);
      profile->writeEntry(MP_USERNAME, gs_installWizardData.login);
   }

   // write the values even if they're empty as otherwise we'd try to create
   // folders with default names - instead of not creating them at all
   profile->writeEntry(MP_POPHOST, gs_installWizardData.pop);
   profile->writeEntry(MP_IMAPHOST, gs_installWizardData.imap);
   profile->writeEntry(MP_NNTPHOST, gs_installWizardData.nntp);
   profile->writeEntry(MP_SMTPHOST, gs_installWizardData.smtp);

   if ( gs_installWizardData.leaveOnServer )
   {
      profile->writeEntry(MP_MOVE_NEWMAIL, false);
   }

   CompleteConfiguration();

   String mainFolderName;
#ifdef USE_INBOX
   if ( !gs_installWizardData.collectAllMail )
      mainFolderName = INBOX_NAME;
   else
#endif // USE_INBOX
      mainFolderName = READ_APPCONFIG_TEXT(MP_NEWMAIL_FOLDER);

   mApplication->GetProfile()->writeEntry(MP_MAINFOLDER, mainFolderName);

   // create a welcome message unless the user didn't use the wizard (in which
   // case we assume he is so advanced that he doesn't need this stuff)
   if ( wizardDone )
   {
      // send a test message unless disabled
      //
      // NB: we can't send email if we don't have SMTP server configured
      if ( gs_installWizardData.sendTestMsg &&
            !gs_installWizardData.smtp.empty() )
      {
         VerifyEMailSendingWorks(new MProgressInfo
                                     (
                                        NULL,
                                        _("Sending the test message...")
                                     ));
      }

      // and create the welcome message in the new mail folder
      MFolder_obj folderMain(mainFolderName);

      MailFolder *mf = MailFolder::OpenFolder(folderMain);

      if ( mf )
      {
         // this might take a long time if the new mail folder already exists
         // and has a lot of messages
         MProgressInfo proginfo(NULL, _("Creating the welcome message..."));

         // make the lines short enough to ensure they're not wrapped with the
         // default line wrap setting (60 columns)
         String msgFmt =
            _("From: mahogany-users-request@lists.sourceforge.net\n"
              "Subject: Welcome to Mahogany!\n"
              "Date: %s\n"
              "\n"
              "Thank you for using Mahogany!\n"
              "\n"
              "This mail and news client is developed as an OpenSource\n"
              "project by a team of volunteers from around the world.\n"
              "If you would like to contribute to its development, you\n"
              "are always welcome to join in!\n"
              "\n"
              "We also rely on you to report any bugs or wishes for\n"
              "improvements that you may have.\n"
              "\n"
              "Please look at http://mahogany.sourceforge.net/ for\n"
              "additional information, news about the latest releases\n"
              "and frequently asked questions."
              "\n"
              "Also, if you reply to this e-mail message with the word\n"
              "'subscribe' in the body or subject of the message, you will\n"
              "be automatically added to the mailing list of Mahogany\n"
              "users, where you will find other users happy to share\n"
              "their experiences with you and help you get started.\n"
              "\n"
              "Your Mahogany Developers Team\n"
              );

         String timeStr = GetRFC822Time();
         String msgString = wxString::Format(msgFmt, timeStr.c_str());

         msgString = strutil_enforceCRLF(msgString);
         mf->AppendMessage(msgString);
         mf->DecRef();
      }
      else
      {
         FAIL_MSG( _T("Cannot get main folder?") );
      }
   }

   return true;
}


/**
  This function uses the wizard data to complete the configuration
  as needed.
*/
static
void CompleteConfiguration()
{
   Profile *profile = mApplication->GetProfile();

   // options telling us which standard folders we need among
   // Inbox/NewMail/SentMail/Trash/Outbox
#ifdef USE_INBOX
   profile->writeEntry(MP_COLLECT_INBOX, gs_installWizardData.collectAllMail);
#endif // USE_INBOX
   profile->writeEntry(MP_USEOUTGOINGFOLDER, true);
   profile->writeEntry(MP_USE_TRASH_FOLDER, gs_installWizardData.useTrash);
   profile->writeEntry(MP_USE_OUTBOX, gs_installWizardData.useOutbox);

   // do setup the std folders now
   VerifyStdFolders();

#ifdef USE_DIALUP
   // Dial-Up network:
   profile->writeEntry(MP_DIALUP_SUPPORT, gs_installWizardData.useDialUp);
   if(gs_installWizardData.useDialUp)
   {
#if defined(OS_WIN)
      profile->writeEntry(MP_NET_CONNECTION, gs_installWizardData.connection);
#elif defined(OS_UNIX)
      profile->writeEntry(MP_NET_ON_COMMAND,gs_installWizardData.dialCommand);
      profile->writeEntry(MP_NET_OFF_COMMAND,gs_installWizardData.hangupCommand);
#endif // platform
   }
#endif // USE_DIALUP

   if(gs_installWizardData.folderType !=
         GetNumericDefault(MP_FOLDER_FILE_DRIVER) )
   {
      profile->writeEntry(MP_FOLDER_FILE_DRIVER,
                          gs_installWizardData.folderType);
   }

#ifdef USE_PYTHON
   profile->writeEntry(MP_USEPYTHON, gs_installWizardData.usePython);
#endif

#ifdef USE_PISOCK
   // PalmOS-box
   if(gs_installWizardData.usePalmOs)
   {
      Profile_obj pp(Profile::CreateModuleProfile("PalmOS"));
      pp->writeEntry("PalmBox", "PalmBox");
      pp->DecRef();

      MFolder_obj folderPalm(MFolder::Create("PalmBox", MF_FILE));

      if( !folderPalm )
      {
         wxLogError(_("Could not create PalmOS mailbox."));
      }
      else
      {
         folderPalm->SetPath("PalmBox");
         folderPalm->SetComment
                     (
                        _("This folder and its configuration settings\n"
                          "are used to synchronise with your PalmOS\n"
                          "based handheld computer."
                          )
                     );

         folderPalm->SetIcon(wxFolderTree::iconPalmPilot);

         MDialog_Message(_(
            "Set up the `PalmBox' mailbox used to synchronise\n"
            "e-mail with your handheld computer.\n"
            "Please use the Plugin-menu to configure\n"
            "the PalmOS support."));
      }

      // the rest is done in Update()
   }
#endif // USE_PISOCK

   // setup the standard servers too
   SetupServers();
}

#endif // USE_WIZARD

static bool
UpgradeFromNone()
{
#ifdef USE_WIZARD // new code which uses the configuration wizard
   (void)RunInstallWizard();

#else // old code which didn't use the setup wizard
   wxLog *log = wxLog::GetActiveTarget();
   if ( log ) {
      wxLogMessage(_("As it seems that you are running Mahogany for the first\n"
                     "time, you should probably set up some of the options\n"
                     "needed by the program (especially network parameters)."));
      log->Flush();
   }
   ShowOptionsDialog();
#endif // USE_WIZARD/!USE_WIZARD

   return true;
}

// ----------------------------------------------------------------------------
// 0.01 -> 0.10
// ----------------------------------------------------------------------------

static bool
UpgradeFrom001()
{
   // Config structure incompatible changes from 0.01a to 0.02a:
   // 1) last prompts stored as 0, 1, 2, ... subkeys of /Prompts/foo instead
   //    of a single value /Prompts/foo.
   // 2) last values of the adb editor search control stored as subkeys of
   //    /AdbEditor/LastSearch instead of a single value with this name.
   // 3) /AdbEditor/LastPage is /AdbEditor/Notebook/Page
   // 4) /AdbEditor/LastAdb{Dir|File} => AdbFilePrompt/AdbFilePromptPath

   // TODO
   return true;
}

#define COPYENTRY(type) \
   { \
      type val; \
      if ( !src->Read(entry, &val) || !dest->Write(newentry,val) ) \
         return -1; \
   }

/** Copies all entries and optionally subgroups from path from to path
    to in the wxConfig.
    NOTE: Currently both from and to should be absolute paths!
    @param from  absolute group from where to copy entries
    @param to    absolute group where to copy to
    @param recursive if true, copy all subgroups, too
    @param dest      if NULL, the global config will be used
    @return number of groups copied (may be 0) or -1 on error
*/
int
CopyEntries(wxConfigBase *src,
            const wxString &from,
            const wxString &to,
            bool recursive,
            wxConfigBase *dest)
{
   // we shouldn't expand the env variables when copying entries
   bool expandEnvVarsOld = src->IsExpandingEnvVars();
   src->SetExpandEnvVars(false);

   wxString oldPath = src->GetPath();

   // we count the groups copied, not entries (the former is more interesting
   // as it corresponds to the number of filters/folders/identities, for
   // example)
   int numCopied = 1;

   if ( !dest )
   {
      // use the global config by default
      dest = mApplication->GetProfile()->GetConfig();
   }

   // Build a list of all entries to copy:
   src->SetPath(from);

   long
      index = 0;
   wxString
      entry, newentry;
   bool ok;
   for ( ok = src->GetFirstEntry(entry, index);
         ok ;
         ok = src->GetNextEntry(entry, index))
   {
      newentry = to;
      newentry << '/' << entry;

      switch ( src->GetEntryType(entry) )
      {
         case wxConfigBase::Type_Unknown:
            wxFAIL_MSG(_T("unexpected entry type"));
            // fall through

         case wxConfigBase::Type_String:
            // GetEntryType() returns string for all wxFileConfig entries, so
            // try to correct it here
            {
               wxString val;
               if ( src->Read(entry, &val) )
               {
                  bool copiedOk;
                  long l;
                  if ( val.ToLong(&l) )
                     copiedOk = dest->Write(newentry, l);
                  else
                     copiedOk = dest->Write(newentry, val);

                  if ( copiedOk )
                     numCopied++;
               }
            }
            break;

         case wxConfigBase::Type_Integer:
            COPYENTRY(long);
            break;

         case wxConfigBase::Type_Float:
            COPYENTRY(double);
            break;

         case wxConfigBase::Type_Boolean:
            COPYENTRY(bool);
            break;
      }
   }

   if ( recursive )
   {
      size_t
         idx = 0,
         n = src->GetNumberOfGroups(false);
      if(n > 0)
      {
         wxString *groups = new wxString[n];
         index = 0;
         for ( ok = src->GetFirstGroup(entry, index);
               ok ;
               ok = src->GetNextGroup(entry, index))
         {
            wxASSERT(idx < n);
            groups[idx++] = entry;
         }

         wxString
            fromgroup, togroup;
         for(idx = 0; idx < n; idx++)
         {
            fromgroup = from;
            fromgroup << '/' << groups[idx];
            togroup = to;
            togroup << '/' << groups[idx];

            int nSub = CopyEntries(src, fromgroup, togroup, recursive, dest);
            if ( nSub == -1 )
            {
               // fatal error, bail out
               numCopied = -1;

               break;
            }

            numCopied += nSub;
         }

         delete [] groups;
      }
   }

   src->SetPath(oldPath);
   src->SetExpandEnvVars(expandEnvVarsOld);

   return numCopied;
}

// ----------------------------------------------------------------------------
// 0.10 -> 0.20
// ----------------------------------------------------------------------------

static bool
UpgradeFrom010()
{
   /* Config structure incompatible changes from 0.10a to 0.20a:
      New Profile system. Visible change is that the
      M/Profile/Folders/ hierarchy no longer exists, entries from
      M/Profile/Folders/xxx must be merged into M/Profile/xxx.

      Passwords are now encrypted using an extremly simple encryption
      algorithms. Totally unsafe but better than cleartext.
    */

   bool rc = true;

   //FIXME paths need adjustment for windows?
   if ( CopyEntries(mApplication->GetProfile()->GetConfig(),
                    _T("/M/Profiles/Folders"), _T("/M/Profiles")) == -1 )
      rc = false;

   if ( CopyEntries(mApplication->GetProfile()->GetConfig(),
                    _T("/AdbEditor"), _T("/M/Profiles/AdbEditor")) )
      rc = false;

   Profile_obj p(wxEmptyString);
   StringList
      folders;
   String
      group, pw, tmp;

   // We need to rename the old mainfolder, to remove its leading slash:
   String mainFolder = READ_CONFIG(p, MP_MAINFOLDER);
   if(mainFolder.Length())
   {
      if(mainFolder[0u] == '/')
      {
         mainFolder = mainFolder.Mid(1);
         p->writeEntry(MP_MAINFOLDER, mainFolder);
      }
   }

   //FIXME paths need adjustment for windows?
   wxConfigBase *c = mApplication->GetProfile()->GetConfig();
   // Delete obsolete groups:
   c->DeleteGroup(_T("/M/Profiles/Folders"));
   c->DeleteGroup(_T("/AdbEditor"));

   /* Encrypt passwords in new location and make sure we have no
      illegal old profiles around. */
   Profile::EnumData cookie;
   for ( bool ok = p->GetFirstGroup(group, cookie);
         ok ;
         ok = p->GetNextGroup(group, cookie))
   {
      tmp = group;
      tmp << '/' << GetOptionName(MP_PROFILE_TYPE);
      if(p->readEntry(tmp, GetNumericDefault(MP_PROFILE_TYPE)) ==
            1 /* Profile::PT_FolderProfile */ )
      {
         folders.push_back(group.ToStdString());
      }
   }
   for(StringList::iterator i = folders.begin(); i != folders.end();i++)
   {
      group = *i;

      Profile_obj profile(group);
      if( READ_CONFIG(profile, MP_FOLDER_TYPE) !=
               GetNumericDefault(MP_FOLDER_TYPE) )
      {
         pw = READ_CONFIG_TEXT(profile, MP_FOLDER_PASSWORD);
         if ( !pw.empty() )
            profile->writeEntry(MP_FOLDER_PASSWORD, strutil_encrypt(pw));
      }
      else
      {
         p->DeleteGroup(group);
         String msg = _("Deleted illegal folder profile: '");
         msg << p->GetName() << '/' << group << '\'';
         wxLogMessage(msg);
      }
   }

   //FIXME check returncodes!

   return rc;
}

// ----------------------------------------------------------------------------
// 0.20 -> 0.21
// ----------------------------------------------------------------------------

class UpgradeFolderFrom020Traversal : public MFolderTraversal
{
public:
   UpgradeFolderFrom020Traversal(MFolder* folder) : MFolderTraversal(*folder)
      { }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         Profile_obj profile(folderName);

         Profile::ReadResult found;
         String hostname = profile->readEntry(MP_OLD_FOLDER_HOST, "", &found);
         if ( found == Profile::Read_FromHere )
         {
            // delete the old entry, create the new one
            wxConfigBase *config = profile->GetConfig();
            if ( config )
            {
               config->DeleteEntry(MP_OLD_FOLDER_HOST);

               wxLogTrace(_T("Successfully converted folder '%s'"),
                          folderName.c_str());
            }
            else
            {
               // what can we do? nothing...
               FAIL_MSG( _T("profile without config - can't delete entry") );
            }

            profile->writeEntry(MP_IMAPHOST, hostname);
            profile->writeEntry(MP_POPHOST, hostname);
         }

         return true;
      }
};

static bool
UpgradeFrom020()
{
   /* Config structure incompatible changes from 0.20a to 0.21a:
      in the folder settings, HostName has become ServerName
    */

   // enumerate all folders recursively
   MFolder_obj folderRoot(wxEmptyString);
   UpgradeFolderFrom020Traversal traverse(folderRoot);
   traverse.Traverse();

   // TODO it would be very nice to purge the redundant settings from config
   //      because the older versions wrote everything to it and it has only
   //      been fixed in 0.21a -- but this seems a bit too complicated

   return true;
}

// ----------------------------------------------------------------------------
// 0.21 -> 0.50
// ----------------------------------------------------------------------------

class TemplateFixFolderTraversal : public MFolderTraversal
{
public:
   TemplateFixFolderTraversal(MFolder* folder) : MFolderTraversal(*folder)
   {
      m_ok = true;
   }

   bool IsOk() const { return m_ok; }

   virtual bool OnVisitFolder(const wxString& folderName)
   {
      Profile_obj profile(folderName);
      String group = M_TEMPLATE_SECTION;
      if ( profile->HasGroup(group) )
      {
         static const wxChar *templateKinds[] =
         {
            MP_TEMPLATE_NEWMESSAGE,
            MP_TEMPLATE_NEWARTICLE,
            MP_TEMPLATE_REPLY,
            MP_TEMPLATE_FOLLOWUP,
            MP_TEMPLATE_FORWARD,
         };

         wxLogTrace(_T("Updating templates for the folder '%s'..."),
                    folderName.c_str());

         for ( size_t n = 0; n < WXSIZEOF(templateKinds); n++ )
         {
            String entry = group + templateKinds[n];
            if ( profile->HasEntry(entry) )
            {
               String templateValue = profile->readEntry(entry, "");

               String entryNew;
               entryNew << M_TEMPLATES_SECTION << '/'
                        << templateKinds[n] << '/'
                        << folderName;
               Profile *profileApp = mApplication->GetProfile();
               if ( profileApp->HasEntry(entryNew) )
               {
                  wxLogWarning(_("A profile entry '%s' already exists, "
                                 "impossible to upgrade the existing template "
                                 "in '%s/%s/%s'"),
                               entryNew.c_str(),
                               folderName.c_str(),
                               group.c_str(),
                               entry.c_str());

                  m_ok = false;
               }
               else
               {
                  wxLogTrace(_T("\t%s/%s/%s upgraded to %s"),
                             folderName.c_str(),
                             group.c_str(),
                             entry.c_str(),
                             entryNew.c_str());

                  profileApp->writeEntry(entryNew, templateValue);
                  profile->writeEntry(entry, entryNew);
               }
            }
         }
      }
      //else: no custom templates for this folder

      // continue with enumeration
      return true;
   }

private:
   bool m_ok;
};

// ----------------------------------------------------------------------------
// 0.50 -> 0.60
// ----------------------------------------------------------------------------

static bool
UpgradeFrom050()
{
   // See the comments in MessageTemplate.h about the changes in the templates
   // location. Briefly, the templates now live in /Templates and not in each
   // folder and the <folder>/Template/<kind> profile entry now contains the
   // name of the template to use and not the contents of it

   // enumerate all folders recursively
   MFolder_obj folderRoot(wxEmptyString);
   TemplateFixFolderTraversal traverse(folderRoot);
   traverse.Traverse();

   if ( !traverse.IsOk() )
      return false;

   // TODO: move filter settings to new profile locations and warn
   // user about changed settings:
   /*
     1.
     Combine "1" + "Criterium" + "Action" into one string and write
     it as the new "Settings" value   ( "1" is the number of Criteria,
     for more than 1 it was broken in 0.50 and cannot be repaired)
     2.
     We could either create new filter name entries for the individual
     folders, or simply tell the user to set them up anew.

     Important: MFilter will remove the "Settings" entries
     automatically when edited and convert everything to proper filter
     code instead, this needn't be done here.
   */

   MDialog_Message(
      _("Since version 0.5 the use of the server settings has\n"
        "changed slightly. If you experience any problems in\n"
        "accessing remote servers, please check the correct\n"
        "settings for those mailboxes.\n"
        "You might have to re-set some of these server hosts.\n"
        "\n"
        "The filter handling has also changed significantly and the\n"
        "old filter rules won't work anymore, sorry.\n"
        "You will have to set up new filter rules and assign them\n"
        "to the folders."));

   return true;
}

// ----------------------------------------------------------------------------
// 0.60 -> 0.63
// ----------------------------------------------------------------------------

static bool
UpgradeFrom060()
{
   // FIXME: this won't work if the user has changed the names of the system
   //        folders or even if they are just translated (but then I think
   //        only trash is translated anyhow)

   static const struct
   {
      const wxChar *name;
      int pos;
   } SystemFolders[] =
   {
      { _T("INBOX"),     MFolderIndex_Inbox   },
      { _T("New Mail"),  MFolderIndex_NewMail },
      { _T("SentMail"),  MFolderIndex_SentMail},
      { _T("Trash"),     MFolderIndex_Trash   },
      { _T("Outbox"),    MFolderIndex_Outbox  },
      { _T("Draft"),     MFolderIndex_Draft   },
   };

   // position the system folders in the tree correctly (we can't reposition
   // the IMAP/POP/NNTP servers as we don't know what they are)
   MFolder *folder;
   for ( size_t n = 0; n < WXSIZEOF(SystemFolders); n++ )
   {
      folder = MFolder::Get(SystemFolders[n].name);
      if ( folder )
      {
         folder->SetTreeIndex(SystemFolders[n].pos);
         folder->DecRef();
      }
   }

   return true;
}

// ----------------------------------------------------------------------------
// 0.63 -> 0.64
// ----------------------------------------------------------------------------

// UpdateNonFolderProfiles() helper: copies all entries from CustomHeaders
// group to the current group itself flattening their names (see
// wxHeadersDialogs.cpp for the details of the config format used)
static void
UpdateCustomHeadersTo064(wxConfigBase *config)
{
   // from wxHeadersDialogs.cpp
   static const wxChar *customHeaderSubgroups[] =
   {
      _T("News"),
      _T("Mail"),
      _T("Both")
   };

   String pathBase = M_CUSTOM_HEADERS_CONFIG_SECTION;
   for ( size_t type = 0; type < WXSIZEOF(customHeaderSubgroups); type++ )
   {
      String path = pathBase + _T('/') + customHeaderSubgroups[type];
      if ( config->HasGroup(path) )
      {
         wxArrayString headerNames,
                       headerValues;

         config->SetPath(path);

         String name;
         long cookie;
         for ( bool cont = config->GetFirstEntry(name, cookie);
               cont;
               cont = config->GetNextEntry(name, cookie) )
         {
            headerNames.Add(name);
            headerValues.Add(config->Read(name, wxEmptyString));
         }

         config->SetPath(_T("../.."));

         // write them as ::GetCustomHeaders() expects them to be
         config->Write(pathBase + customHeaderSubgroups[type],
                       strutil_flatten_array(headerNames));

         size_t count = headerValues.GetCount();
         for ( size_t n = 0; n < count; n++ )
         {
            path.clear();
            path << pathBase
                 << ':' << headerNames[n]
                 << ':' << customHeaderSubgroups[type];

            config->Write(path, headerValues[n]);
         }
      }
   }
}

static void
UpdateTemplatesTo064(wxConfigBase *config)
{
   config->SetPath(M_TEMPLATE_SECTION);

   wxArrayString names,
                 values;

   String name;
   long cookie;
   for ( bool cont = config->GetFirstEntry(name, cookie);
         cont;
         cont = config->GetNextEntry(name, cookie) )
   {
      names.Add(name);
      values.Add(config->Read(name, wxEmptyString));
   }

   config->SetPath(_T(".."));

   size_t count = names.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      String path;
      path << M_TEMPLATE_SECTION << '_' << names[n];
      config->Write(path, values[n]);
   }
}

static void
UpdateNonFolderProfiles(wxConfigBase *config)
{
   wxArrayString groupsToDelete;

   String name;
   long cookie;
   bool cont = config->GetFirstGroup(name, cookie);
   while ( cont )
   {
      bool deleteGroup;
      if ( name == M_CUSTOM_HEADERS_CONFIG_SECTION )
      {
         UpdateCustomHeadersTo064(config);

         deleteGroup = true;
      }
      else if ( name == _T("Template") )
      {
         UpdateTemplatesTo064(config);

         deleteGroup = true;
      }
      else
      {
         // delete all groups under M_PROFILE_CONFIG_SECTION without ProfileType=1
         // in them: normally, there shouldn't be any, but somehow in my own
         // ~/.M/config there is some junk (maybe left from some very old
         // version?) and if we leave them in config we'd have all kinds of
         // problems with them because they don't represent the real folders
         deleteGroup = config->Read(name + _T("/ProfileType"), 0l) != 1;
         if ( deleteGroup )
         {
            wxLogWarning(_("Removing invalid config settings group '%s'."),
                         name.c_str());
         }
      }

      if ( deleteGroup )
      {
         // don't delete it now as it confuses enumerating the subgroups, do it
         // when we finish with them
         groupsToDelete.Add(name);
      }
      else // valid folder group, descend into it
      {
         // remove obsolete "ProfileType", it's not used nor needed any more
         config->DeleteEntry(name + _T("/ProfileType"));

         config->SetPath(name);
         UpdateNonFolderProfiles(config);
         config->SetPath(_T(".."));
      }

      cont = config->GetNextGroup(name, cookie);
   }

   size_t count = groupsToDelete.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      config->DeleteGroup(groupsToDelete[n]);
   }
}

static bool
UpgradeFrom061()
{
   // we have to move all non folder profiles out of M_PROFILE_CONFIG_SECTION
   // as starting with 0.64 only folders live there

   Profile *profile = mApplication->GetProfile();
   wxConfigBase *config = profile->GetConfig();

   // first deal with ADB settings: they now live under /Settings
   String pathOld, pathNew;
   pathOld << '/' << M_PROFILE_CONFIG_SECTION << '/' << ADB_CONFIG_PATH;
   pathNew << '/' << M_SETTINGS_CONFIG_SECTION << '/' << ADB_CONFIG_PATH;
   if ( CopyEntries(config, pathOld, pathNew) == -1 )
   {
      wxLogWarning(_("Address book editor settings couldn't be updated."));

      return false;
   }
   else // copied successfully
   {
      // delete them from old location
      config->DeleteGroup(pathOld);
   }

   // next copy the global templates data to its own section

   // these settings contain plenty of "$"s which shouldn't be expanded
   ProfileEnvVarSave noEnvVarsExp(profile);

   pathOld.clear();
   pathNew.clear();
   pathOld << _T('/') << Profile::GetProfilePath() << _T("/Templates");
   pathNew << Profile::GetTemplatesPath();
   if ( CopyEntries(config, pathOld, pathNew) == -1 )
   {
      wxLogWarning(_("Template settings couldn't be updated."));

      return false;
   }
   else // copied successfully
   {
      // delete them from old location
      config->DeleteGroup(pathOld);
   }

   // now examine all folders recursively upgrading the old settings to the new
   // format and removing all obsolete ProfileType settings in progress
   UpdateNonFolderProfiles(config);

   return true;
}

// ----------------------------------------------------------------------------
// 0.64 -> 0.64.1
// ----------------------------------------------------------------------------

class UpgradeFolderFrom064Traversal : public MFolderTraversal
{
public:
   UpgradeFolderFrom064Traversal(MFolder* folder) : MFolderTraversal(*folder)
      { }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         MFolder_obj folder(folderName);
         CHECK( folder, false, _T("traversed folder which doesn't exist?") );

         if ( folder->GetFlags() & MF_FLAGS_INCOMING )
         {
            folder->AddFlags(MF_FLAGS_MONITOR);
         }

         return true;
      }
};

static bool
UpgradeFrom064()
{
   // add MF_FLAGS_MONITOR to all folders with MF_FLAGS_INCOMING flag
   MFolder_obj folderRoot(wxEmptyString);
   UpgradeFolderFrom064Traversal traverse(folderRoot);

   return traverse.Traverse();
}

// ----------------------------------------------------------------------------
// 0.64.[12] -> 0.65
// ----------------------------------------------------------------------------

class UpgradeFolderFrom0641Traversal : public MFolderTraversal
{
public:
   UpgradeFolderFrom0641Traversal(MFolder* folder) : MFolderTraversal(*folder)
      { }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         MFolder_obj folder(folderName);
         CHECK( folder, false, _T("traversed folder which doesn't exist?") );

         Profile_obj profile(folder->GetProfile());

         const int flags = folder->GetFlags();

         int flagsNew = flags;

         // replace the SSL flags with SSL profile entries
         if ( flags & MF_FLAGS_SSLAUTH )
         {
            if ( !profile->writeEntryIfNeeded
                           (
                              GetOptionName(MP_USE_SSL),
                              SSLSupport_SSL,
                              GetNumericDefault(MP_USE_SSL)
                           ) )
            {
               return false;
            }

            if ( flags & MF_FLAGS_SSLUNSIGNED )
            {
               if ( !profile->writeEntryIfNeeded
                              (
                                 GetOptionName(MP_USE_SSL_UNSIGNED),
                                 SSLSupport_SSL,
                                 GetNumericDefault(MP_USE_SSL_UNSIGNED)
                              ) )
               {
                  return false;
               }

            }

            // reset the old SSL flags
            flagsNew &= ~(MF_FLAGS_SSLAUTH | MF_FLAGS_SSLUNSIGNED);
         }

         // simply reset the MF_FLAGS_REOPENONPING flag, it's not used any
         // longer
         flagsNew &= ~MF_FLAGS_REOPENONPING;

         // anything to update?
         if ( flagsNew != flags )
         {
            folder->SetFlags(flagsNew);
         }


         // also check SMTP/NNTP server flags: need to replace bools with
         // SSLSupport_XXX values

         const char *key = GetOptionName(MP_SMTPHOST_USE_SSL);
         Profile::ReadResult readFrom;
         bool wasUsingSSL = profile->readEntry(key, false, &readFrom);

         // only replace the value if it is present in this folder as it is
         // going to be changed in the parent when we visit it anyhow
         if ( readFrom == Profile::Read_FromHere )
         {
            if ( !profile->writeEntryIfNeeded
                           (
                              key,
                              wasUsingSSL ? SSLSupport_SSL
                                          : SSLSupport_TLSIfAvailable,
                              GetNumericDefault(MP_SMTPHOST_USE_SSL)
                           ) )
            {
               return false;
            }
         }

         key = GetOptionName(MP_NNTPHOST_USE_SSL);
         wasUsingSSL = profile->readEntry(key, false, &readFrom);
         if ( readFrom == Profile::Read_FromHere )
         {
            if ( !profile->writeEntryIfNeeded
                           (
                              key,
                              wasUsingSSL ? SSLSupport_SSL
                                          : SSLSupport_TLSIfAvailable,
                              GetNumericDefault(MP_NNTPHOST_USE_SSL)
                           ) )
            {
               return false;
            }
         }

         return true;
      }
};

static bool
UpgradeFrom064_1()
{
   // replace SSL flags with SSL profile settings
   MFolder_obj folderRoot(wxEmptyString);
   UpgradeFolderFrom0641Traversal traverse(folderRoot);

   // it is important to process the parent before its children because by
   // using writeEntryIfNeeded() they rely on its settings being updated before
   // their own ones
   return traverse.Traverse(MFolderTraversal::Recurse_ParentFirst);
}

// ----------------------------------------------------------------------------
// 0.65 -> 0.66
// ----------------------------------------------------------------------------

static bool
UpgradeFrom065()
{
#ifdef OS_UNIX
   // we don't use "/M" prefix any more, copy everything from under it to the
   // root
   wxConfigBase *config = mApplication->GetProfile()->GetConfig();

   if ( CopyEntries(config, _T("/M"), _T("/")) < 0 )
      return false;

   if ( !config->DeleteGroup(_T("/M")) )
   {
      wxLogDebug(_T("Old data was left in [M] config section."));
   }


   // the folder-specific message box settings must be changed as well as
   // profile paths have changed
   //
   // we use hardcoded path here but this is ok as we don't intend to ever
   // change this code, even if the path used for storing the msg boxes
   // settings changes in the future
   String pathOld = config->GetPath();

   String path;
   path << _T('/') << M_SETTINGS_CONFIG_SECTION << _T("MessageBox");
   config->SetPath(path);

   long cookie;
   String groupOld,
          groupNew;
   for ( bool cont = config->GetFirstGroup(groupOld, cookie);
         cont;
         cont = config->GetNextGroup(groupOld, cookie) )
   {
      if ( groupOld.StartsWith(_T("M_Profiles_"), &groupNew) )
      {
         config->RenameGroup(groupOld, groupNew);
      }
   }

   config->SetPath(pathOld);
#endif // OS_UNIX

   return true;
}

// ----------------------------------------------------------------------------
// 0.66 -> 0.68
// ----------------------------------------------------------------------------

static bool
UpgradeFrom066()
{
   // purge the unnecessary MP_PGP_GET_PUBKEY and M_MSGBOX_GET_PGP_PUBKEY
   mApplication->GetProfile()->DeleteEntry("PGPGetPubKey");
   wxPMessageBoxEnable("GetPGPPubKey");

   // also remove the listbook settings section as we use wx persistent
   // controls support now and so these settings are not used any more
   AllConfigSources::Get().DeleteGroup("/Settings/ListbookPages");

   return true;
}

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// upgrade from the specified version to the current one
extern bool
Upgrade(const String& fromVersion)
{
   // what is the old version?
   MVersion oldVersion;
   if ( fromVersion.empty() )
      oldVersion = Version_None;
   else
   {
      wxString version = fromVersion;

      // terminating 'a' means alpha, just remove it for the purposes of
      // version comparison
      if ( version.Last() == 'a' )
         version.Truncate(version.Len() - 1);

      // trailing ".0" is not significant neither
      if ( version.Right(2) == ".0" )
         version.Truncate(version.Len() - 2);

      if ( version == "0.01" )
         oldVersion = Version_Alpha001;
      else if ( version == "0.02" || version == "0.10")
         oldVersion = Version_Alpha010;
      else if ( version == "0.20" )
         oldVersion = Version_Alpha020;
      else if ( version == "0.21" || version == "0.22" ||
                version == "0.23" || version == "0.50" )
         oldVersion = Version_050;
      else if ( version == "0.60" )
         oldVersion = Version_060;
      else if ( version == "0.61" || version == "0.62" || version == "0.63" )
         oldVersion = Version_061;
      else if ( version == "0.64" )
         oldVersion = Version_064;
      else if ( version == "0.64.1" || version == "0.64.2" )
         oldVersion = Version_064_1;
      else if ( version == "0.65" )
         oldVersion = Version_065;
      else if ( version == "0.66" ||
                version == "0.67" ||
                version == "0.67.1" ||
                version == "0.67.2" )
         oldVersion = Version_066;
      else if ( version == "0.68" )
         oldVersion = Version_068;
      else
         oldVersion = Version_Unknown;
   }

   bool success = true;
   switch ( oldVersion )
   {
      case Version_None:
         UpgradeFromNone();
         break;

      case Version_Alpha001:
         if ( success )
            success = UpgradeFrom001();
         // fall through

      case Version_Alpha010:
         if ( success )
            success = UpgradeFrom010();
         // fall through

      case Version_Alpha020:
         if ( success )
            success = UpgradeFrom020();
         // fall through

      case Version_050:
         if ( success )
            success = UpgradeFrom050();
         // fall through

      case Version_060:
         if ( success )
            success = UpgradeFrom060();
         // fall through

      case Version_061:
         if ( success )
            success = UpgradeFrom061();
         // fall through

      case Version_064:
         if ( success )
            success = UpgradeFrom064();
         // fall through

      case Version_064_1:
         if ( success )
            success = UpgradeFrom064_1();
         // fall through

      case Version_065:
         if ( success )
            success = UpgradeFrom065();
         // fall through

      case Version_066:
         if ( success && UpgradeFrom066() )
            wxLogMessage(_("Configuration information and program files were "
                           "successfully upgraded from the version '%s'."),
                         fromVersion.c_str());
         else
            wxLogError(_("Configuration information and program files "
                         "could not be upgraded from version '%s', some "
                         "settings might be lost.\n"
                         "\n"
                         "It is recommended that you uninstall and reinstall "
                         "the program before using it."),
                       fromVersion.c_str());
         // fall through

      case Version_Last:
         // nothing to do, it's the latest one
         break;

      default:
         FAIL_MSG(_T("invalid version value"));
         // fall through

      case Version_Unknown:
         wxLogError(_("The previously installed version of Mahogany (%s) was "
                      "probably newer than this one. Cannot upgrade."),
                    fromVersion.c_str());
         return false;
   }

   return true;
}


/// only used to find new mail folder
class NewMailFolderTraversal : public MFolderTraversal
{
public:
   NewMailFolderTraversal(MFolder* folder) : MFolderTraversal(*folder)
      { }

   String GetNewMailFolder(void) const { return m_NewMailFolder; }
   bool OnVisitFolder(const wxString& folderName)
      {
         MFolder *f = MFolder::Get(folderName);
         if(f && (f->GetFlags() & MF_FLAGS_NEWMAILFOLDER))
         {
            if(m_NewMailFolder.Length())
            {
               ERRORMESSAGE((_(
                  "Found additional folder '%s'\n"
                  "marked as central new mail folder. Ignoring it.\n"
                  "New Mail folder used is '%s'."),
                             f->GetFullName().c_str(),
                             m_NewMailFolder.c_str()));
//               f->SetFlags(f->GetFlags() & !MF_FLAGS_NEWMAILFOLDER);
               Profile *p = Profile::CreateProfile(f->GetFullName());
               if(p)
               {
                 int typeAndFlags = READ_CONFIG(p,MP_FOLDER_TYPE);
                 MFolderType type = GetFolderType(typeAndFlags);
                 int flags = GetFolderFlags(typeAndFlags);
                 flags ^= MF_FLAGS_NEWMAILFOLDER;
                 p->writeEntry(MP_FOLDER_TYPE,
                      CombineFolderTypeAndFlags(type, flags));
                 p->DecRef();
               }
            }
            else
            {
               m_NewMailFolder = f->GetFullName();
               if(f->GetFlags() & MF_FLAGS_INCOMING)
               {
                  ERRORMESSAGE((_("Cannot auto-collect mail from the new mail folder\n"
                                  "'%s'\n"
                                  "Corrected configuration data."),
                                f->GetFullName().c_str()));
                  f->SetFlags(f->GetFlags() & !MF_FLAGS_INCOMING);
               }
            }
         }
         if(f) f->DecRef();
         return true;
      }
private:
   String m_NewMailFolder;
};

/**
  Check that the given folder exists and has correct flags, corrects them if
  not.

  Returns value > 0 if the folder already existed, < 0 if it didn't exist and
  was successfulyl created and 0 if we failed to create it
 */
extern int
VerifyStdFolder(const MOption& optName,
                const String& nameDefault,
                int flags,
                const String& comment,
                MFolderIndex idxInTree = MFolderIndex_Max,
                int icon = -1)
{
   wxString name = READ_APPCONFIG(optName);

   if ( name.empty() )
   {
      // write the translated folder name to the profile the first time we run
      name = nameDefault;
      mApplication->GetProfile()->writeEntry(optName, nameDefault);
   }

   // all system folders have this flag
   flags |= MF_FLAGS_DONTDELETE;

   int rc;

   MFolder *folder = MFolder::Get(name);
   if ( !folder )
   {
      folder = MFolder::Create(name, MF_FILE);

      if ( !folder )
      {
         wxLogError(_("Failed to create system folder '%s'"), name.c_str());

         return 0;
      }

      folder->SetPath(name);
      folder->SetFlags(flags);
      folder->SetComment(comment);

      if ( idxInTree != MFolderIndex_Max )
         folder->SetTreeIndex(idxInTree);

      if ( icon != -1 )
         folder->SetIcon(icon);

      // we have created it
      rc = -1;
   }
   else // already exists
   {
      // check that it has right flags: we allow the user to change all the
      // flags except the ones below
      static const int flagsToCheck = MF_FLAGS_INCOMING |
                                      MF_FLAGS_NEWMAILFOLDER;

      int flagsCur = folder->GetFlags();

      if ( (flagsCur & flagsToCheck) != (flags & flagsToCheck) )
      {
         flagsCur &= ~flagsToCheck;
         flagsCur |= flags & flagsToCheck;

         folder->SetFlags(flagsCur);
      }

      // it already was there
      rc = 1;
   }

   folder->DecRef();

   return rc;
}

/// set MP_FVIEW_FROM_REPLACE option for the given folder
static void
SetReplaceFromOption(const String& folderName)
{
   MFolder_obj folder(folderName);
   if ( !folder )
   {
      FAIL_MSG( _T("folder must exist") );
   }
   else
   {
      Profile_obj profile(folder->GetProfile());
      if ( !profile )
      {
         FAIL_MSG( _T("profile must exist") );
      }
      else
      {
         profile->writeEntry(MP_FVIEW_FROM_REPLACE, 1);
      }
   }
}

/**
  Checks that the standard folders (INBOX, NeMail, SentMail, Trash, ...) exist
  and creates them if it doesn't. Also checks that they have the correct flags
  and fixes them if they don't.

  Returns true if ok, false if we failed
 */
static bool
VerifyStdFolders(void)
{
   // INBOX: the mail spool folder
   // ----------------------------

#ifdef USE_INBOX
   bool collectFromInbox = READ_APPCONFIG_BOOL(MP_COLLECT_INBOX);

   MFolder *folderInbox = MFolder::Get(INBOX_NAME);
   if ( !folderInbox )
   {
      folderInbox = MFolder::Create(INBOX_NAME, MF_INBOX);

      if ( !folderInbox )
      {
         wxLogError(_("Failed to create the INBOX folder."));

         return false;
      }

      int flags = MF_FLAGS_DONTDELETE | MF_FLAGS_MONITOR;

      if ( collectFromInbox || MDialog_YesNoDialog
           (
             _("Normally Mahogany will automatically collect all mail\n"
               "from your system's default mail folder (INBOX,\n"
               "representing your mail spool entry),\n"
               "and move it to a special 'New Mail' folder.\n"
               "This is generally considered good practice as you\n"
               "should not leave your new mail in INBOX.\n"
               "\n"
               "Please confirm this. If you select No, your mail\n"
               "will remain in INBOX and you need to check that\n"
               "folder manually."
              ),
             NULL,
             _("Collect mail from INBOX?"),
             M_DLG_YES_DEFAULT
           ) )
      {
         collectFromInbox = true;
         flags |= MF_FLAGS_INCOMING;
      }
      else
      {
         MDialog_Message
         (
          _("Mahogany will not automatically collect mail from your\n"
            "system's default INBOX.\n"
            "You can change this in the INBOX folder's preferences\n"
            "dialog at any time."
           ),
          NULL,
          MDIALOG_MSGTITLE,
          "WarnInbox"
         );
      }

      folderInbox->SetFlags(flags);
      folderInbox->SetComment(_("Default system folder for incoming mail."));
      folderInbox->SetTreeIndex(MFolderIndex_Inbox);
      folderInbox->SetIcon(wxFolderTree::iconInbox);
   }

   folderInbox->DecRef();
#else // !USE_INBOX
   // if there is no INBOX at all, we don't want to hide NewMail folder in the
   // code below
   bool collectFromInbox = true;
#endif // USE_INBOX/!USE_INBOX

   // NewMail: the folder to which all new mail is collected
   // ------------------------------------------------------

   int flagsNewMail = MF_FLAGS_NEWMAILFOLDER;
   if ( !collectFromInbox )
   {
      // still create it even if we don't collect mail from INBOX as the user
      // might create other folder from which he does want to collect mail
      // later but hide it in the folder tree by default then
      flagsNewMail |= MF_FLAGS_HIDDEN;
   }

   if ( !VerifyStdFolder
         (
            MP_NEWMAIL_FOLDER,
            _("New Mail"),
            flagsNewMail,
            _("Folder where Mahogany will collect all new mail."),
            MFolderIndex_NewMail,
            wxFolderTree::iconNewMail
         ) )
   {
      return false;
   }

   // SentMail: the folder where the copies of sent messages are stored
   // -----------------------------------------------------------------

   if ( READ_APPCONFIG(MP_USEOUTGOINGFOLDER) )
   {
      switch ( VerifyStdFolder
               (
                  MP_OUTGOINGFOLDER,
                  _("SentMail"),
                  MF_FLAGS_KEEPOPEN,
                  _("Folder where Mahogany will store copies of outgoing messages."),
                  MFolderIndex_SentMail,
                  wxFolderTree::iconSentMail
               ) )
      {
         case 0:
            return false;

         case -1:
            // set some more default options for the SentMail: by default, show
            // "To:" addresses in it and not "From:"
            SetReplaceFromOption(READ_APPCONFIG(MP_OUTGOINGFOLDER));
            break;

         default:
            FAIL_MSG( _T("unexpected VerifyStdFolder return value") );
            // fall through

         case 1:
            // nothing to do
            ;
      }
   }

   // Trash: the folder where deleted messages are stored
   // ---------------------------------------------------

   if ( READ_APPCONFIG(MP_USE_TRASH_FOLDER) )
   {
      switch ( VerifyStdFolder
               (
                  MP_TRASH_FOLDER,
                  _("Trash"),
                  MF_FLAGS_KEEPOPEN,
                  _("Folder where Mahogany will store deleted messages."),
                  MFolderIndex_Trash,
                  wxFolderTree::iconTrash
               ) )
      {
         case 0:
            // failed to create
            return false;

         case -1:
            // it doesn't make sense to notify the user about new mail in the
            // trash folder (this would be triggered by deleting any message)
            {
               MFolder_obj folder(READ_APPCONFIG(MP_TRASH_FOLDER));
               if ( !folder )
               {
                  FAIL_MSG( _T("trash folder must exist") );
               }
               else
               {
                  Profile_obj profile(folder->GetProfile());
                  if ( !profile )
                  {
                     FAIL_MSG( _T("Trash folder profile must exist") );
                  }
                  else
                  {
                     profile->writeEntry(MP_NEWMAIL_PLAY_SOUND, 0l);
                     profile->writeEntry(MP_SHOW_NEWMAILMSG, 0l);
                  }
               }
            }
            break;

         default:
            FAIL_MSG( _T("unexpected VerifyStdFolder return value") );
            // fall through

         case 1:
            // already existed, don't change the options -- don't do anything
            // in fact
            ;
      }
   }

   // Outbox: the folder where the messages queue for being sent
   // ----------------------------------------------------------

   if ( READ_APPCONFIG(MP_USE_OUTBOX) )
   {
      switch ( VerifyStdFolder
               (
                  MP_OUTBOX_NAME,
                  _("Outbox"),
                  0,
                  _("Folder where Mahogany will queue messages to be sent"),
                  MFolderIndex_Outbox,
                  wxFolderTree::iconOutbox
               ) )
      {
         case 0:
            return false;

         case -1:
            // set some more default options for the Outbox: by default, show
            // "To:" addresses in it and not "From:"
            SetReplaceFromOption(READ_APPCONFIG(MP_OUTBOX_NAME));
            break;

         default:
            FAIL_MSG( _T("unexpected VerifyStdFolder return value") );
            // fall through

         case 1:
            // nothing to do
            ;
      }
   }

   return true;
}

extern bool
VerifyEMailSendingWorks(MProgressInfo *proginfo)
{
   // we send a mail to ourself
   Profile *p = mApplication->GetProfile();
   String me = READ_CONFIG(p, MP_FROM_ADDRESS);

   SendMessage *sm = SendMessage::Create(p);
   sm->SetSubject(_("Mahogany Test Message"));
   sm->SetAddresses(me);
   String msg =
      _("If you have received this mail, your Mahogany configuration works.\r\n"
        "You should also try to reply to this mail and check that your reply arrives.");
   sm->AddPart(MimeType::TEXT, msg.c_str(), msg.length());
   bool ok = sm->SendOrQueue(SendMessage::Silent);
   delete sm;

   // delete the progress frame, we don't need it any more and it would hide
   // the message dialogs we pop up below
   delete proginfo;

   if ( !ok )
   {
      MDialog_ErrorMessage(_("Sending the test message failed, please recheck "
                             "your configuration settings"));

      return false;
   }

   mApplication->SendOutbox();
   msg.Empty();
   msg << _("Sent email message to:\n")
       << me
       << _("\n\nPlease check whether it arrives.");
   MDialog_Message(msg, NULL, _("Testing your configuration"), "TestMailSent");

   return true; // till we know something better
}

/**
  Create a folder in the folder tree - just a convenient wrapper around
  CreateFolderTreeEntry()
 */
static inline MFolder *CreateServerEntry(const String& name,
                                         MFolderType type,
                                         int flags)
{
   return CreateFolderTreeEntry(NULL, name, type, flags, wxEmptyString, false);
}

/**
  This function sets up folder entries for the servers, which can then
  be used for browsing them. Called only once when the application is
  initialised for the first time.
 */
static void
SetupServers(void)
{
   String serverName;

   // The NNTP server
   serverName = READ_APPCONFIG_TEXT(MP_NNTPHOST);
   if ( !serverName.empty() )
   {
      MFolder_obj mfolder(CreateServerEntry
                          (
                              _("NNTP Server"),
                              MF_NNTP,
                              MF_FLAGS_ANON | MF_FLAGS_GROUP
                          ));

      if ( !mfolder )
      {
         wxLogError(_("Could not create an entry for the NNTP server."));
      }
      else
      {
         mfolder->SetIcon(wxFolderTree::iconNNTP);
         mfolder->SetTreeIndex(MFolderIndex_NNTP);
      }
   }

   // The IMAP server
   serverName = READ_APPCONFIG_TEXT(MP_IMAPHOST);
   if ( !serverName.empty() )
   {
      MFolder_obj mfolder(CreateServerEntry
                          (
                              _("IMAP Server"),
                              MF_IMAP,
                              MF_FLAGS_GROUP
                          ));

      if ( !mfolder )
      {
         wxLogError(_("Could not create an entry for the IMAP server."));
      }
      else
      {
         mfolder->SetIcon(wxFolderTree::iconIMAP);
         mfolder->SetTreeIndex(MFolderIndex_IMAP);

         // don't create the INBOX automatically, let the user do it if he wants
         // later - like this he can use IMAP server entry created above directly
         // which would be impossible if we created INBOX automatically because
         // doing this disables the parent folder
#if 0
         MFolder *imapInbox = CreateFolderTreeEntry(mfolder,
                                         _("IMAP INBOX"),
                                         MF_IMAP,
                                         MF_FLAGS_DEFAULT,
                                         "INBOX",
                                         false);

         SafeDecRef(imapInbox);
#endif // 0
      }
   }

   // The POP3 server:
   serverName = READ_APPCONFIG_TEXT(MP_POPHOST);
   if ( !serverName.empty() )
   {
      // the POP3 folder is created as incoming as otherwise it doesn't work
      // really well
      MFolder_obj mfolder(CreateServerEntry
                          (
                              _("POP3 Server"),
                              MF_POP,
                              MF_FLAGS_INCOMING
                          ));

      if ( !mfolder )
      {
         wxLogError(_("Could not create an entry for the POP3 server."));
      }
      else
      {
         mfolder->SetIcon(wxFolderTree::iconPOP);
         mfolder->SetTreeIndex(MFolderIndex_POP);
      }
   }

   // local newsspool ?
   String newsspool = MailFolderCC::GetNewsSpool();
   if ( wxDirExists(newsspool) )
   {
      MFolder_obj mfolder(CreateServerEntry
                         (
                             _("News spool"),
                             MF_NEWS,
                             MF_FLAGS_GROUP
                         ));
      if ( !mfolder )
      {
         wxLogError(_("Could not create an entry for the local news spool."));
      }
      else
      {
         mfolder->SetComment(_("Local news-spool found on your system during installation."));
         mfolder->SetIcon(wxFolderTree::iconNews);
      }
   }
}

/**
   Make sure we have the minimal set of things set up:

   MP_USERNAME, MP_PERSONALNAME,
   MP_USERDIR
   MP_MBOXDIR
   MP_HOSTNAME

   and maybe take the values for

   MP_SMTPHOST, MP_NNTPHOST, MP_IMAPHOST, MP_POPHOST

   from the environment
*/
static
void SetupMinimalConfig(void)
{
   // DNS lookup in wxGetFullHostName() may take quite some time (especially
   // if the DNS doesn't work...), show something to the user while we're
   // blocking in it
   MProgressInfo proginfo(NULL, _("One time only environment setup..."));

   wxYield(); // to show proginfo

   Profile *profile = mApplication->GetProfile();

   if( strutil_isempty(READ_APPCONFIG(MP_USERNAME)) )
   {
      profile->writeEntry(MP_USERNAME, wxGetUserId());
   }

   if( strutil_isempty(READ_APPCONFIG(MP_PERSONALNAME)) )
   {
      profile->writeEntry(MP_PERSONALNAME, wxGetUserName());
   }

   // now that we have the local dir, we can set up a default mail folder dir
   String str = READ_APPCONFIG(MP_MBOXDIR);
   if( strutil_isempty(str) )
   {
      str = READ_APPCONFIG_TEXT(MP_USERDIR);
      profile->writeEntry(MP_MBOXDIR, str);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_HOSTNAME)) )
   {
      profile->writeEntry(MP_HOSTNAME, wxGetFullHostName());
   }

   if( strutil_isempty(READ_APPCONFIG(MP_NNTPHOST)) )
   {
      const wxChar *cptr = wxGetenv(_T("NNTPSERVER"));
      if(!cptr || !*cptr)
        cptr = _T("news");
      profile->writeEntry(MP_NNTPHOST, cptr);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_SMTPHOST)) )
   {
      const wxChar *cptr = wxGetenv(_T("SMTPSERVER"));
      if(!cptr || !*cptr)
        cptr = _T("localhost");
      profile->writeEntry(MP_SMTPHOST, cptr);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_POPHOST)) )
   {
      const wxChar *cptr = wxGetenv(_T("POPSERVER"));
      if(cptr && *cptr)
         profile->writeEntry(MP_POPHOST, cptr);
   }
   if( strutil_isempty(READ_APPCONFIG(MP_IMAPHOST)) )
   {
      const wxChar *cptr = wxGetenv(_T("IMAPSERVER"));
      if(cptr && *cptr)
         profile->writeEntry(MP_IMAPHOST, cptr);
   }
}

/*
 * This doesn't strictly belong here(?) but for now it's the easiest
 * place to put it.
 * This function exports a configurable subset of the configuration
 * settings to a file.
 */

#define M_SYNCMAIL_SUBJECT   _T("DO NOT DELETE! Mahogany remote configuration info")
#define M_SYNCMAIL_CONFIGSTART _T("Mahogany configuration info:")

static time_t  gs_RemoteSyncDate = 0;

extern
bool RetrieveRemoteConfigSettings(bool confirm)
{
   if ( confirm )
   {
      if ( !READ_APPCONFIG_BOOL(MP_SYNC_REMOTE) )
         return true; // nothing to do

      if ( !MDialog_YesNoDialog
            (
             _("Retrieve remote configuration settings now?"), NULL,
             _("Retrieve remote settings?"),
             M_DLG_YES_DEFAULT,
            M_MSGBOX_RETRIEVE_REMOTE ) )
      {
           return true;
      }
   }

   String foldername = READ_APPCONFIG(MP_SYNC_FOLDER);
   MFolder_obj folder(foldername);
   if ( !folder )
   {
      wxLogError(_("Folder '%s' for storing remote configuration "
                   "doesn't exist."), foldername.c_str());
      return false;
   }

   MailFolder *mf = MailFolder::OpenFolder(folder);

   if(! mf)
   {
      wxLogError(_("Please check that the folder '%s' where the remote "
                   "configuration is stored exists."), foldername.c_str());

      return false;
   }

   unsigned long nMessages = mf->GetMessageCount();
   if ( nMessages != 1 )
   {
      if ( nMessages == 0 )
         wxLogError(_("Configuration mailbox '%s' does not contain any "
                      "information."), mf->GetName().c_str());
      else
         wxLogError(
            _("Configuration mailbox '%s' contains more than\n"
              "one message. Possibly wrong mailbox specified?\n"
              "If this mailbox is the correct one, please remove\n"
              "the extra messages and try again."), mf->GetName().c_str());
      mf->DecRef();
      return false;
   }
   HeaderInfoList *hil = mf->GetHeaders();
   Message * msg = mf->GetMessage( (*hil)[0]->GetUId() );
   if( msg == NULL)
      return false; // what happened?
   if(msg->Subject() != M_SYNCMAIL_SUBJECT)
   {
      wxLogError(
         _("The message in the configuration mailbox '%s' does not\n"
           "contain configuration settings. Please remove it."),
         mf->GetName().c_str());
      mf->DecRef();
      msg->DecRef();
      hil->DecRef();
      return false;
   }

   wxString msgText = msg->FetchText();
   gs_RemoteSyncDate = (*hil)[0]->GetDate();
   hil->DecRef();
   msg->DecRef();
   mf->DecRef();

   msgText = msgText.Mid( msgText.Find(M_SYNCMAIL_CONFIGSTART) +
                          wxStrlen( M_SYNCMAIL_CONFIGSTART) );
   wxFile tmpfile;
   wxString filename = wxFileName::CreateTempFileName("MTemp", &tmpfile);
   tmpfile.Write(msgText, msgText.Length());
   tmpfile.Close();

   wxFileConfig fc(wxEmptyString, wxEmptyString, filename, wxEmptyString,
                   wxCONFIG_USE_LOCAL_FILE);

   bool rc = true;

   // note that the first path given to CopyEntries() is always in Unix format,
   // while the second path should be the real path we use in our config now
   // (may be different under Windows)
   if ( READ_APPCONFIG_BOOL(MP_SYNC_FILTERS) )
   {
      int nFilters = CopyEntries(&fc,
                                 M_FILTERS_CONFIG_SECTION,
                                 Profile::GetFiltersPath());
      if ( nFilters == -1 )
      {
         rc = false;

         wxLogError(_("Failed to retrieve remote filters information."));
      }
      else
      {
         wxLogMessage(_("Retrieved %d remote filters."), nFilters);
      }
   }

   if ( READ_APPCONFIG_BOOL(MP_SYNC_IDS) )
   {
      int nIds = CopyEntries(&fc,
                             M_IDENTITY_CONFIG_SECTION,
                             Profile::GetIdentityPath());
      if ( nIds == -1 )
      {
         rc = false;

         wxLogError(_("Failed to retrieve remote identities information."));
      }
      else
      {
         wxLogMessage(_("Retrieved %d remote identities."), nIds);
      }
   }

   if ( READ_APPCONFIG_BOOL(MP_SYNC_FOLDERS) )
   {
      String group = READ_APPCONFIG(MP_SYNC_FOLDERGROUP);
      String src, dest;
      src << M_PROFILE_CONFIG_SECTION << '/' << group;
      dest << Profile::GetProfilePath() << '/' << group;

      int nFolders = CopyEntries(&fc, src, dest);
      if ( nFolders == -1 )
      {
         rc = false;

         wxLogError(_("Failed to retrieve remote folders information."));
      }
      else
      {
         wxLogMessage(_("Retrieved %d remote folders."), nFolders);

         // refresh all existing folder trees
         MEventData *data = new MEventFolderTreeChangeData
                                (
                                 wxEmptyString,
                                 MEventFolderTreeChangeData::CreateUnder
                                );
         MEventManager::Send(data);
      }
   }

   wxRemoveFile(filename);

   return rc;
}

extern
bool SaveRemoteConfigSettings(bool confirm)
{
   if ( confirm )
   {
      if ( !READ_APPCONFIG_BOOL(MP_SYNC_REMOTE) )
         return true; // nothing to do

      if ( !MDialog_YesNoDialog
            (
             _("Store remote configuration settings now?"), NULL,
             _("Store remote settings?"),
             M_DLG_YES_DEFAULT,
             M_MSGBOX_STORE_REMOTE
            ) )
      {
           return true;
      }
   }

   MFolder_obj folderSync(READ_APPCONFIG(MP_SYNC_FOLDER));
   if ( !folderSync )
      return false;

   MailFolder *mf = MailFolder::OpenFolder(folderSync);

   if(! mf)
      return false;

   unsigned long nMessages = mf->GetMessageCount();
   if( nMessages > 1 )
   {
      wxLogError(
         _("Configuration mailbox '%s' contains more than\n"
           "one message. Possibly wrong mailbox specified?\n"
           "If this mailbox is the correct one, please remove\n"
           "the extra messages and try again."), mf->GetName().c_str());
      mf->DecRef();
      return false;
   }
   // If we have information stored there, delete it:
   if( nMessages != 0 )
   {
      HeaderInfoList *hil = mf->GetHeaders();
      Message * msg = mf->GetMessage( (*hil)[0]->GetUId() );
      time_t storedDate = (*hil)[0]->GetDate();
      hil->DecRef();
      if( msg == NULL)
         return false; // what happened?
      if(msg->Subject() != M_SYNCMAIL_SUBJECT)
      {
         wxLogError(
            _("The message in the configuration mailbox '%s' does not\n"
              "contain configuration settings. Please remove it."),
            mf->GetName().c_str());
         mf->DecRef();
         msg->DecRef();
         return false;
      }
      if(gs_RemoteSyncDate != 0 &&
         storedDate > gs_RemoteSyncDate)
      {
         if ( !MDialog_YesNoDialog
               (
                _("The remotely stored configuration information seems to have changed\n"
                  "since it was retrieved.\n"
                  "Are you sure you want to overwrite it with the current settings?"),
                NULL,
                _("Overwrite remote settings?"),
                M_DLG_YES_DEFAULT,
                M_MSGBOX_OVERWRITE_REMOTE
               ) )
         {
            mf->DecRef();
            msg->DecRef();
            return false;
         }
      }
      if(! mf->DeleteMessage(msg->GetUId()))
      {
         wxLogError(
            _("Cannot remove old configuration information from\n"
              "mailbox '%s'."), mf->GetName().c_str());
         mf->DecRef();
         msg->DecRef();
         return false;
      }
      msg->DecRef();
      mf->ExpungeMessages();
   }

   wxString filename = wxFileName::CreateTempFileName(_T("MTemp"));
   wxFileConfig fc(wxEmptyString, wxEmptyString, filename, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);

   bool rc = true;

   // always create the config file in Unix format
   if ( READ_APPCONFIG_BOOL(MP_SYNC_FILTERS) )
   {
      rc &= (CopyEntries(mApplication->GetProfile()->GetConfig(),
                         Profile::GetFiltersPath(),
                         M_FILTERS_CONFIG_SECTION,
                         true,
                         &fc) != -1);
   }

   if ( READ_APPCONFIG_BOOL(MP_SYNC_IDS) )
   {
      rc &= (CopyEntries(mApplication->GetProfile()->GetConfig(),
                         Profile::GetIdentityPath(),
                         M_IDENTITY_CONFIG_SECTION,
                         true,
                         &fc) != -1);
   }

   if ( READ_APPCONFIG_BOOL(MP_SYNC_FOLDERS) )
   {
      String group = READ_APPCONFIG(MP_SYNC_FOLDERGROUP);
      String src, dest;
      src << Profile::GetProfilePath() << '/' << group;
      dest << M_PROFILE_CONFIG_SECTION << '/' << group;

      rc &= (CopyEntries(mApplication->GetProfile()->GetConfig(),
                         src, dest, true,
                         &fc) != -1);
   }

   if(rc == false)
   {
      wxLogError(_("Could not export configuration information."));
      mf->DecRef();
      return false;
   }

   // flush the config into file before reading it
   fc.Flush();

   wxFile tmpfile(filename, wxFile::read);
   wxChar * buffer = new wxChar [ tmpfile.Length() + 1];
   const wxFileOffset lenTmp = tmpfile.Length();
   if (lenTmp == wxInvalidOffset ||
       tmpfile.Read(buffer, lenTmp) != lenTmp)
   {
      wxLogError(_("Cannot read configuration info from temporary file\n"
                   "'%s'."), filename.c_str());
      tmpfile.Close();
      delete [] buffer;
      mf->DecRef();
      return false;
   }
   buffer[tmpfile.Length()] = '\0';
   tmpfile.Close();

   wxString msgText;
   msgText << _T("From: mahogany-developers@lists.sourceforge.net\n")
           << _T("Subject: ") << M_SYNCMAIL_SUBJECT << _T("\n")
           << _T("Date: ") << GetRFC822Time() << _T("\n")
           << _T("\n")
           << M_SYNCMAIL_CONFIGSTART << _T("\n")
           << buffer << _T("\n");
   delete [] buffer;
   if( ! mf->AppendMessage(msgText) )
   {
      wxLogError(_("Storing configuration information in mailbox\n"
                   "'%s' failed."), mf->GetName().c_str());
      rc = false;
   }
   mf->DecRef();

   // if !confirm, we're in not interactive mode, so don't show anything
   if(rc && confirm)
   {
      wxString msg;
      msg.Printf(
         _("Successfully stored shared configuration info in folder '%s'."),
         mf->GetName().c_str());
      MDialog_Message(msg, NULL, _("Saved settings"),
                      GetPersMsgBoxName(M_MSGBOX_CONFIG_SAVED_REMOTELY));
   }
   return rc;
}

extern bool
CheckConfiguration(void)
{
   Profile *profile = mApplication->GetProfile();

   if ( !READ_APPCONFIG_BOOL(MP_LICENSE_ACCEPTED) ) // not accepted
   {
      bool accepted = ShowLicenseDialog();
      if(accepted)
      {
         profile->writeEntry(MP_LICENSE_ACCEPTED, 1l);
      }
      else
      {
         ERRORMESSAGE((_("You must accept the license to run Mahogany.")));

         mApplication->SetLastError(M_ERROR_CANCEL);

         return false;
      }
   }

   bool firstRun = READ_APPCONFIG_BOOL(MP_FIRSTRUN);
   if ( firstRun != 0 )
   {
      // make sure the essential things have proper values
      SetupMinimalConfig();

      // next time won't be the first one any more
      profile->writeEntry(MP_FIRSTRUN, 0);
   }

   // do we need to upgrade something?
   String version = profile->readEntry(MP_VERSION, "");
   if ( version != M_VERSION )
   {
      CloseSplash();

      if ( !Upgrade(version) )
         return false;

      // write the new version
      profile->writeEntry(MP_VERSION, M_VERSION);
   }

   if ( !VerifyStdFolders() )
   {
      wxLogError(_("Some of the standard folders are missing, the program "
                   "won't function normally."));

      return false;
   }

   if ( !RetrieveRemoteConfigSettings() )
   {
      wxLogError(_("Remote configuration information could not be retrieved."));
   }

   return true;
}

