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
#  include "PathFinder.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "strutil.h"
#  include "Mpers.h"
#  include "gui/wxMApp.h"  // for wxMApp::GetDialUpManager()
#endif  //USE_PCH

#  include "Message.h"
#  include "MailFolder.h"
#  include "HeaderInfo.h"
#  include "MailFolderCC.h"
#  include "SendMessage.h"
#  include "MessageCC.h"

#ifdef USE_PYTHON
#  include <Python.h>
#  include "PythonHelp.h"
#  include "MScripts.h"
#endif

#include "Mdefaults.h"

#include <wx/log.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <wx/utils.h>         // wxGetFullHostName()
#include <wx/dialup.h>        // for IsAlwaysOnline()
#include <wx/socket.h>        // wxIPv4Address

#define USE_WIZARD

#ifdef OS_UNIX
   // mail collection from INBOX makes sense only under Unix
   #define USE_MAIL_COLLECT

   // as well as INBOX itself - there is no such thing under Windows
   #define USE_INBOX
#endif

#ifdef USE_WIZARD
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
   Version_NoChange, // any version from which we don't need to upgrade
   Version_Unknown   // some unrecognized version
};

#ifdef USE_WIZARD

#ifndef OS_WIN
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
   InstallWizard_DialUpPage,           // set up dial-up networking
//   InstallWizard_MiscPage,             // other common options
#ifdef USE_HELPERS_PAGE
//   InstallWizard_HelpersPage,          // external programs set up
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
   String name;         // personal name, not login one (we can find it out)
   String email;

   // servers page
   String pop,
          imap,
          smtp,
          nntp;

   // operations page:
   int    useDialUp; // initially -1
   bool   useOutbox;
   bool   useTrash;
   int    folderType;
#ifdef USE_PISOCK
   bool   usePalmOs;
#endif
#ifdef USE_PYTHON
   bool   usePython;
#endif
#ifdef USE_MAIL_COLLECT
   // mail collection from INBOX makes sense only under Unix
   bool   collectAllMail;
#endif // USE_MAIL_COLLECT

   // dial up page
#if defined(OS_WIN)
   String connection;
#elif defined(OS_UNIX)
   String dialCommand,
          hangupCommand;

   // helpers page
   String browser;
#endif // OS_UNIX

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

// check the given address for validity
static bool CheckHostName(const wxString& hostname)
{
   // check if server names are valid by verifying the hostname part (i.e.
   // discard everything after ':' which is the port number) with DNS
   return hostname.empty() || wxIPV4address().Hostname(hostname.AfterLast(':'));
}

// return the name of the main mail folder
static wxString GetMainMailFolderName()
{
   String mainFolderName;
#ifdef USE_MAIL_COLLECT
   if ( !gs_installWizardData.collectAllMail )
      mainFolderName = "INBOX";
   else
#endif // USE_MAIL_COLLECT
      mainFolderName = READ_APPCONFIG(MP_NEWMAIL_FOLDER);

   return mainFolderName;
}

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

   // the dial up page should be shown only if this function returns TRUE
   static bool ShouldShowDialUpPage();

   // the import page should be shown only if this function returns TRUE
   static bool ShouldShowImportPage();

   // implement the wxWizardPage pure virtuals in terms of our ones
   virtual wxWizardPage *GetPrev() const
      { return GetPageById(GetPrevPageId()); }
   virtual wxWizardPage *GetNext() const
      { return GetPageById(GetNextPageId()); }

   void OnWizardCancel(wxWizardEvent& event);

protected:
   wxWizardPage *GetPageById(InstallWizardPageId id) const;

   // creates an "enhanced panel" for placing controls into under the static
   // text (explanation)
   wxEnhancedPanel *CreateEnhancedPanel(wxStaticText *text);

private:
   // id of this page
   InstallWizardPageId m_id;

   DECLARE_EVENT_TABLE()
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
};

// second page: ask the basic info about the user (name, e-mail)
class InstallWizardIdentityPage : public InstallWizardPage
{
public:
   InstallWizardIdentityPage(wxWizard *wizard);

   virtual bool TransferDataToWindow()
   {
      // the first time the page is shown, construct the reasonable default
      // value
      if ( !gs_installWizardData.name )
         gs_installWizardData.name = READ_APPCONFIG(MP_PERSONALNAME);

      if ( !gs_installWizardData.email )
         gs_installWizardData.email = READ_APPCONFIG(MP_FROM_ADDRESS);

      if ( !gs_installWizardData.email )
      {
         gs_installWizardData.email = READ_APPCONFIG(MP_USERNAME);
         gs_installWizardData.email << '@' << READ_APPCONFIG(MP_HOSTNAME);
      }

      m_name->SetValue(gs_installWizardData.name);
      m_email->SetValue(gs_installWizardData.email);

      return TRUE;
   }

   virtual bool TransferDataFromWindow()
   {
      gs_installWizardData.name = m_name->GetValue();
      gs_installWizardData.email = m_email->GetValue();
      if ( !gs_installWizardData.email )
      {
         wxLogError(_("Please specify a valid email address."));
         m_email->SetFocus();

         return FALSE;
      }

      return TRUE;
   }

private:
   wxTextCtrl *m_name,
              *m_email;
};

class InstallWizardServersPage : public InstallWizardPage
{
public:
   InstallWizardServersPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
      {
         gs_installWizardData.pop  = m_pop->GetValue();
         gs_installWizardData.imap = m_imap->GetValue();
         gs_installWizardData.smtp = m_smtp->GetValue();
         gs_installWizardData.nntp = m_nntp->GetValue();
         if(gs_installWizardData.smtp.Length() == 0)
         {
            // FIXME: they could use an MTA under Unix instead...
            wxLogError(_("You need to specify the SMTP server to be able "
                         "to send email, please do it!"));
            return FALSE;
         }

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
                                       TRUE, NULL) )
                  return FALSE;
            }
         }

         return TRUE;
      }

   virtual bool TransferDataToWindow()
      {
         m_pop->SetValue(gs_installWizardData.pop);
         m_imap->SetValue(gs_installWizardData.imap);
         m_smtp->SetValue(gs_installWizardData.smtp);
         m_nntp->SetValue(gs_installWizardData.nntp);
         return TRUE;
      }

   void AddDomain(wxString& server, const wxString& domain)
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
         if(MDialog_YesNoDialog(msg,this, MDIALOG_YESNOTITLE, TRUE))
#endif // 0

         server << '.' << domain;
      }

private:
   wxTextCtrl *m_pop,
              *m_imap,
              *m_smtp,
              *m_nntp;
};

class InstallWizardDialUpPage : public InstallWizardPage
{
public:
   InstallWizardDialUpPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
#if defined(OS_WIN)
      gs_installWizardData.connection = m_connections->GetValue();
#elif defined(OS_UNIX)
      gs_installWizardData.dialCommand = m_connect->GetValue();
      gs_installWizardData.hangupCommand = m_disconnect->GetValue();
#endif // platform

      return TRUE;
   }

   virtual bool TransferDataToWindow()
   {
#if defined(OS_WIN)
      m_connections->SetValue(gs_installWizardData.connection);
#elif defined(OS_UNIX)
      m_connect->SetValue(gs_installWizardData.dialCommand);
      m_disconnect->SetValue(gs_installWizardData.hangupCommand);
#endif // platform

      return TRUE;
   }

private:
#if defined(OS_WIN)
   wxComboBox *m_connections;
#elif defined(OS_UNIX)
   wxTextCtrl *m_connect,
              *m_disconnect;
#endif // platform
};

class InstallWizardOperationsPage : public InstallWizardPage
{
public:
   InstallWizardOperationsPage(wxWizard *wizard);

   virtual bool TransferDataToWindow()
      {
         // no setting yet?
         if ( gs_installWizardData.useDialUp == -1 )
         {
#ifdef OS_WIN
            // disabling this code for Windows because the program crashes in
            // release version after returning from IsAlwaysOnline(): I
            // strongly suspect an optimizer bug (it doesn't happen without
            // optimizations) but I can't fix it right now otherwise
            gs_installWizardData.useDialUp = FALSE;
#else // !Win
            wxDialUpManager *dialupMan =
                  ((wxMApp *)mApplication)->GetDialUpManager();

            // if we have a LAN connection, then we don't need to configure
            // dial-up networking, but if we don't, then we probably do
            gs_installWizardData.useDialUp = dialupMan &&
                                                !dialupMan->IsAlwaysOnline();
#endif // Win/!Win
         }

         m_FolderTypeChoice->SetSelection(gs_installWizardData.folderType);
#ifdef USE_PYTHON
         m_UsePythonCheckbox->SetValue(gs_installWizardData.usePython != 0);
#endif
         m_UseDialUpCheckbox->SetValue(gs_installWizardData.useDialUp != 0);
#ifdef USE_PISOCK
         m_UsePalmOsCheckbox->SetValue(gs_installWizardData.usePalmOs != 0);
#endif
         m_UseOutboxCheckbox->SetValue(gs_installWizardData.useOutbox != 0);
         m_TrashCheckbox->SetValue(gs_installWizardData.useTrash != 0);
#ifdef USE_MAIL_COLLECT
         m_CollectCheckbox->SetValue(gs_installWizardData.collectAllMail != 0);
#endif // USE_MAIL_COLLECT

         return TRUE;
      }

   virtual bool TransferDataFromWindow()
      {
         gs_installWizardData.folderType  = m_FolderTypeChoice->GetSelection();
#ifdef USE_PYTHON
         gs_installWizardData.usePython  = m_UsePythonCheckbox->GetValue();
#endif
#ifdef USE_PISOCK
         gs_installWizardData.usePalmOs  = m_UsePalmOsCheckbox->GetValue();
#endif
         gs_installWizardData.useDialUp  = m_UseDialUpCheckbox->GetValue();
         gs_installWizardData.useOutbox  = m_UseOutboxCheckbox->GetValue();
         gs_installWizardData.useTrash   = m_TrashCheckbox->GetValue();
#ifdef USE_MAIL_COLLECT
         gs_installWizardData.collectAllMail = m_CollectCheckbox->GetValue();
#endif // USE_MAIL_COLLECT

         return TRUE;
      }
private:
   wxChoice *m_FolderTypeChoice;
   wxCheckBox *m_TrashCheckbox,
              *m_UseOutboxCheckbox,
              *m_UseDialUpCheckbox
#ifdef USE_MAIL_COLLECT
             , *m_CollectCheckbox
#endif // USE_MAIL_COLLECT
#ifdef USE_PISOCK
             , *m_UsePalmOsCheckbox
#endif
#ifdef USE_PYTHON
             , *m_UsePythonCheckbox
#endif
             ;
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
};

class InstallWizardImportPage : public InstallWizardPage
{
public:
   InstallWizardImportPage(wxWizard *wizard);

   void OnImportButton(wxCommandEvent& event);

private:
   DECLARE_EVENT_TABLE()
};

class InstallWizardFinalPage : public InstallWizardPage
{
public:
   InstallWizardFinalPage(wxWizard *wizard);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   wxCheckBox *m_checkboxSendTestMsg;
};

// ----------------------------------------------------------------------------
// prototypes
// ----------------------------------------------------------------------------

// the function which runs the install wizard
extern bool RunInstallWizard();
extern bool VerifyMailConfig(void);

#endif // USE_WIZARD

// ============================================================================
// implementation
// ============================================================================

#ifdef USE_WIZARD

// ----------------------------------------------------------------------------
// wizard pages code
// ----------------------------------------------------------------------------

// InstallWizardPage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(InstallWizardPage, wxWizardPage)
   EVT_WIZARD_CANCEL(-1, InstallWizardPage::OnWizardCancel)
END_EVENT_TABLE()

bool InstallWizardPage::ShouldShowDialUpPage()
{
   return gs_installWizardData.useDialUp != 0;
}

bool InstallWizardPage::ShouldShowImportPage()
{
   if ( gs_installWizardData.showImportPage == -1 )
      gs_installWizardData.showImportPage = HasImporters();

   return gs_installWizardData.showImportPage != 0;
}

InstallWizardPageId InstallWizardPage::GetPrevPageId() const
{
   int id = m_id - 1;
   if ( (id == InstallWizard_DialUpPage && !ShouldShowDialUpPage()) ||
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
   if ( (id == InstallWizard_DialUpPage && !ShouldShowDialUpPage()) ||
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
         CREATE_PAGE(DialUp);
//         CREATE_PAGE(Misc);
#ifdef USE_HELPERS_PAGE
//         CREATE_PAGE(Helpers);
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
   wxSize sizeLabel = text->GetSize();
   wxSize sizePage = ((wxWizard *)GetParent())->GetPageSize();
//   wxSize sizePage = ((wxWizard *)GetParent())->GetSize();
   wxCoord y = sizeLabel.y + 2*LAYOUT_Y_MARGIN;

   wxEnhancedPanel *panel = new wxEnhancedPanel(this, TRUE /* scrolling */);
   panel->SetSize(0, y, sizePage.x, sizePage.y - y);

   panel->SetAutoLayout(TRUE);

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
   (void)new wxStaticText(this, -1, _(
      "Welcome to Mahogany!\n"
      "\n"
      "This wizard will help you to setup the most\n"
      "important settings needed to successfully use\n"
      "the program. You don't need to specify everything\n"
      "here - it can also be done later by opening the\n"
      "'Options' dialog - but if you do fill the, you\n"
      "should be able to start the program immediately.\n"
      "\n"
      "If you decide to not use the dialog, just check\n"
      "the box below or press [Cancel] at any moment."
                                         ));

   m_useWizard = true;

   m_checkbox = new wxCheckBox
                (
                  this, -1,
                  _("I'm an &expert and don't need the wizard")
                );

   wxSize sizeBox = m_checkbox->GetSize(),
          sizePage = wizard->GetPageSize();

   // adjust the vertical position
   m_checkbox->Move(5, sizePage.y - 2*sizeBox.y);
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

void InstallWizardWelcomePage::OnUseWizardCheckBox(wxCommandEvent& event)
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
         _("Mahogany has detected that you have one or more\n"
           "other email programs installed on this computer.\n"
           "\n"
           "You may click the button below to invoke a dialog\n"
           "which will allow you to import some configuration\n"
           "settings (your personal name, email address, folders,\n"
           "address books &c) from one or more of them."));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);
   panel->CreateButton(_("&Import..."), NULL);
   panel->ForceLayout();
}

void InstallWizardImportPage::OnImportButton(wxCommandEvent& event)
{
   ShowImportDialog(this);
}

// InstallWizardIdentityPage
// ----------------------------------------------------------------------------

InstallWizardIdentityPage::InstallWizardIdentityPage(wxWizard *wizard)
                         : InstallWizardPage(wizard, InstallWizard_IdentityPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
         "Please specify your name and e-mail address:\n"
         "they will be used for sending the messages.\n"
         ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;
   labels.Add(_("&Personal name:"));
   labels.Add(_("&E-mail:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_name = panel->CreateTextWithLabel(labels[0], widthMax, NULL);
   m_email = panel->CreateTextWithLabel(labels[1], widthMax, m_name);

   panel->ForceLayout();
}

// InstallWizardServersPage
// ----------------------------------------------------------------------------

InstallWizardServersPage::InstallWizardServersPage(wxWizard *wizard)
                        : InstallWizardPage(wizard, InstallWizard_ServersPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
      "You can receive e-mail from remote mail\n"
      "servers using either POP3 or IMAP4 protocols\n"
      "but you usually need only one of them\n"
      "(IMAP is more secure and efficient, so use it if you can).\n"
      "\n"
      "All of these fields may be filled later as well\n"
      "(and you will be able to specify multiple servers too).\n"
      "You also may leave a field empty if you are not going to use\n"
      "the corresponding server."
      ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;
   labels.Add(_("&POP server:"));
   labels.Add(_("&IMAP server:"));
   labels.Add(_("&SMTP server:"));
   labels.Add(_("&NNTP server:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_pop = panel->CreateTextWithLabel(labels[0], widthMax, NULL);
   m_imap = panel->CreateTextWithLabel(labels[1], widthMax, m_pop);
   m_smtp = panel->CreateTextWithLabel(labels[2], widthMax, m_imap);
   m_nntp = panel->CreateTextWithLabel(labels[3], widthMax, m_smtp);

   panel->ForceLayout();
}

// InstallWizardDialUpPage
// ----------------------------------------------------------------------------

InstallWizardDialUpPage::InstallWizardDialUpPage(wxWizard *wizard)
                       : InstallWizardPage(wizard, InstallWizard_DialUpPage)
{
   wxStaticText *text = new wxStaticText(this, -1, _(
      "Mahogany can automatically detect if your\n"
      "network connection is online or offline.\n"
      "It can also connect and disconnect you to the\n"
      "network, but for this it needs to know which\n"
      "commands to execute to go online or offline."));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;

#if defined(OS_WIN)
   // get all existing RAS connections
   wxDialUpManager *dial =
      ((wxMApp *)mApplication)->GetDialUpManager();;
   wxArrayString connections;
   size_t nCount = dial->GetISPNames(connections);

   // concatenate all connection names into one ':' separated string
   wxString comboChoices = _("&Dial up connection to use");
   labels.Add(comboChoices);
   for ( size_t n = 0; n < nCount; n++ )
   {
      comboChoices << ':' << connections[n];
   }

   // do create controls now
   long widthMax = GetMaxLabelWidth(labels, panel);

   m_connections = panel->CreateComboBox(comboChoices, widthMax, NULL);
#elif defined(OS_UNIX)
   labels.Add(_("Command to &connect:"));
   labels.Add(_("Command to &disconnect:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_connect = panel->CreateTextWithLabel(labels[0], widthMax, NULL);
   m_disconnect = panel->CreateTextWithLabel(labels[1], widthMax, m_connect);
#endif // platform

   panel->ForceLayout();
}

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
#ifdef USE_MAIL_COLLECT
      Label_Collect,
#endif // USE_MAIL_COLLECT
      Label_UseTrash,
      Label_UseOutbox,
      Label_UseDialUp,
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
#ifdef USE_MAIL_COLLECT
   labels.Add(_("&Collect new mail:"));
#endif // USE_MAIL_COLLECT
   labels.Add(_("Use &Trash mailbox:"));
   labels.Add(_("Use &Outbox queue:"));
   labels.Add(_("&Use dial-up network:"));
#ifdef USE_PISOCK
   labels.Add(_("&Load PalmOS support:"));
#endif // USE_PISOCK
   labels.Add(_("Default &mailbox format"));
#ifdef USE_PYTHON
   labels.Add(_("Enable &Python:"));
#endif // USE_PYTHON

   long widthMax = GetMaxLabelWidth(labels, panel);

   wxControl *last = NULL;

#ifdef USE_MAIL_COLLECT
   wxStaticText *text = panel->CreateMessage(_(
      "Mahogany can either leave all messages in\n"
      "your system mailbox or create its own\n"
      "mailbox for new mail and move all new\n"
      "messages there. This is recommended,\n"
      "especially in a multi-user environment."), last);

   m_CollectCheckbox = panel->CreateCheckBox(labels[Label_Collect],
                                             widthMax, text);
   last = m_CollectCheckbox;
#endif // USE_MAIL_COLLECT

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

   wxStaticText *text4 = panel->CreateMessage(
      _(
         "\n"
         "If you are using dial-up networking,\n"
         "Mahogany may detect your connection status\n"
         "and optionally dial and hang-up."
         ), m_UseOutboxCheckbox);
   m_UseDialUpCheckbox = panel->CreateCheckBox(labels[Label_UseDialUp],
                                               widthMax, text4);
   last = m_UseDialUpCheckbox;

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
       << ":Unix mbx mailbox:Unix mailbox:MMDF (SCO Unix):Tenex (Unix MM format)";
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

   panel->ForceLayout();
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
      "http://mahogany.sourceforge.net/\n"
      "\n"
      "We hope you will enjoy using Mahogany!\n"
      "                    The M-Team"
                                     ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);
   wxArrayString labels;
   labels.Add(_("&Send test message:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   text = panel->CreateMessage(_(
      "\n"
      "Finally, you may want to test your configuration\n"
      "by sending a test message to yourself."
                                 ), NULL);
   m_checkboxSendTestMsg = panel->CreateCheckBox(labels[0], widthMax, text);

   panel->ForceLayout();
}

bool InstallWizardFinalPage::TransferDataToWindow()
{
   m_checkboxSendTestMsg->SetValue(gs_installWizardData.sendTestMsg);

   return TRUE;
}

bool InstallWizardFinalPage::TransferDataFromWindow()
{
   gs_installWizardData.sendTestMsg = m_checkboxSendTestMsg->GetValue();

   return TRUE;
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
   case  0: timeStr = "Jan"; break;
   case  1: timeStr = "Feb"; break;
   case  2: timeStr = "Mar"; break;
   case  3: timeStr = "Apr"; break;
   case  4: timeStr = "May"; break;
   case  5: timeStr = "Jun"; break;
   case  6: timeStr = "Jul"; break;
   case  7: timeStr = "Aug"; break;
   case  8: timeStr = "Sep"; break;
   case  9: timeStr = "Oct"; break;
   case 10: timeStr = "Nov"; break;
   case 11: timeStr = "Dec"; break;
   default:
      ; // suppress warning
   }
   timeStr.Printf("%02d %s %d %02d:%02d:%02d",
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
void CompleteConfiguration(const struct InstallWizardData &gs_installWizardData);

static void SetupServers(void);

bool RunInstallWizard()
{
   // as we use a static array, make sure that only one install wizard is
   // running at any time (a sensible requirment anyhow)
   static bool gs_isWizardRunning = false;

   if ( gs_isWizardRunning )
   {
      FAIL_MSG( "the installation wizard is already running!" );

      return false;
   }

   gs_isWizardRunning = true;

   // first, set up the default values for the wizard:

   gs_installWizardData.useDialUp = -1;
   gs_installWizardData.showImportPage = -1;
#if defined(OS_WIN)
   gs_installWizardData.connection = READ_APPCONFIG(MP_NET_CONNECTION);
#elif defined(OS_UNIX)
   gs_installWizardData.dialCommand = READ_APPCONFIG(MP_NET_ON_COMMAND);
   gs_installWizardData.hangupCommand = READ_APPCONFIG(MP_NET_OFF_COMMAND);
#endif // platform

   gs_installWizardData.useOutbox = MP_USE_OUTBOX_D;
   gs_installWizardData.useTrash = MP_USE_TRASH_FOLDER_D;
#ifdef USE_MAIL_COLLECT
   gs_installWizardData.collectAllMail = TRUE;
#endif // USE_MAIL_COLLECT
   gs_installWizardData.folderType = 0; /* mbx */
#ifdef USE_PYTHON
   gs_installWizardData.usePython = READ_APPCONFIG(MP_USEPYTHON) != 0;
#endif
#ifdef USE_PISOCK
   gs_installWizardData.usePalmOs = TRUE;
#endif
   gs_installWizardData.sendTestMsg = TRUE;

   gs_installWizardData.pop  = READ_APPCONFIG(MP_POPHOST);
   gs_installWizardData.imap = READ_APPCONFIG(MP_IMAPHOST);
   gs_installWizardData.smtp = READ_APPCONFIG(MP_SMTPHOST);
   gs_installWizardData.nntp = READ_APPCONFIG(MP_NNTPHOST);

   // assume we don't skip the wizard by default
   gs_installWizardData.done = true;

   wxIconManager *iconManager = mApplication->GetIconManager();
   wxWizard *wizard = wxWizard::Create
                      (
                        NULL,                         // parent
                        -1,                           // id
                        _("Mahogany Installation"),   // title
                        iconManager->GetBitmap("install_welcome") // def image
                      );

   // NULL the pages array
   memset(gs_wizardPages, 0, sizeof(gs_wizardPages));

   gs_wizardPages[InstallWizard_WelcomePage] = new InstallWizardWelcomePage(wizard);

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

   // make sure we have some _basic_ things set up whether the wizard ran or
   // not (basic here meaning that the program will not operate properly
   // without any of them)
   Profile *profile = mApplication->GetProfile();
   if ( wizardDone )
   {
      // load all modules by default:
      wxString modules = "Filters";
#ifdef USE_PISOCK
      if(gs_installWizardData.usePalmOs)
         modules += ":PalmOS";
#endif // USE_PISOCK

      profile->writeEntry(MP_MODULES, modules);
   }
   else
   {
      // assume the simplest possible values for everything
      gs_installWizardData.useOutbox = false;
      gs_installWizardData.useTrash = false;
      gs_installWizardData.useDialUp = false;
#if USE_PYTHON
      gs_installWizardData.usePython = false;
#endif // USE_PYTHON
#if USE_PISOCK
      gs_installWizardData.usePalmOs = false;
#endif // USE_PISOCK
#ifdef USE_MAIL_COLLECT
      gs_installWizardData.collectAllMail = false;
#endif // USE_MAIL_COLLECT

      // don't reset the SMTP server: it won't lead to creation of any
      // (unwanted) folders, so it doesn't hurt to have the default value
      gs_installWizardData.pop =
      gs_installWizardData.imap =
      gs_installWizardData.nntp = "";

      // don't try to send the test message if the SMTP server wasn't
      // configured
      gs_installWizardData.sendTestMsg = false;

      // also, don't show the tips for the users who skip the wizard
      profile->writeEntry(MP_SHOWTIPS, 0l);
   }

   // transfer the wizard settings from InstallWizardData
   profile->writeEntry(MP_FROM_ADDRESS, gs_installWizardData.email);
   profile->writeEntry(MP_PERSONALNAME, gs_installWizardData.name);

   // write the values even if they're empty as otherwise we'd try to create
   // folders with default names - instead of not creating them at all
   profile->writeEntry(MP_POPHOST, gs_installWizardData.pop);
   profile->writeEntry(MP_IMAPHOST, gs_installWizardData.imap);
   profile->writeEntry(MP_NNTPHOST, gs_installWizardData.nntp);
   profile->writeEntry(MP_SMTPHOST, gs_installWizardData.smtp);

   CompleteConfiguration(gs_installWizardData);

   SetupServers();

   String mainFolderName = GetMainMailFolderName();
   mApplication->GetProfile()->writeEntry(MP_MAINFOLDER, mainFolderName);

   // create a welcome message unless the user didn't use the wizard (in which
   // case we assume he is so advanced that he doesn't need this stuff)
   if ( wizardDone )
   {
      MailFolder *mf = MailFolder::OpenFolder(MF_FILE, mainFolderName);

      if ( mf )
      {
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
         FAIL_MSG( "Cannot get main folder?" );
      }
   }

   return true;
}


/** This function uses the wizard data to complete the configuration
    as needed. It is responsible for setting up INBOX "New Mail" etc.


    TODO: the folder flags settings etc here and in verifyinbox could
    be combined into one function
*/
static
void CompleteConfiguration(const struct InstallWizardData &gs_installWizardData)
{
   Profile * profile = mApplication->GetProfile();

   // OUTBOX
   if(gs_installWizardData.useOutbox)
   {
      profile->writeEntry(MP_USE_OUTBOX, 1l);
      wxString name = READ_CONFIG(profile, MP_OUTBOX_NAME);
      if(name.Length() == 0)
      {
         name = _("Outbox");
         profile->writeEntry(MP_OUTBOX_NAME, name);
      }
      if(! MailFolder::CreateFolder(name, MF_FILE,
                                    MF_FLAGS_KEEPOPEN,
                                    name,
                                    _("Queue of messages to be sent.")))
      {
         wxLogError(_("Could not create the outgoing mailbox '%'."),
                    name.c_str());
      }
      else
      {
         MFolder_obj folder(name);
         folder->SetTreeIndex(MFolderIndex_Outbox);
      }
   }
   else
   {
      profile->writeEntry(MP_USE_OUTBOX, 0l);
   }

   // INBOX/NEW MAIL: under Unix, always create both INBOX and NEW MAIL
   // folders, but keep one of them hidden depending on "Collect all mail"
   // setting; under Windows we never use INBOX at all
   int flagInbox, flagNewMail;
#ifdef USE_MAIL_COLLECT
   if ( !gs_installWizardData.collectAllMail )
   {
      // hide NEW MAIL, show INBOX but don't collect mail from it
      flagInbox = 0;
      flagNewMail = MF_FLAGS_HIDDEN;
   }
   else // collect all mail
#endif // USE_MAIL_COLLECT
   {
      // hide INBOX and collect mail from it, show the New Mail folder
      flagInbox = MF_FLAGS_HIDDEN | MF_FLAGS_INCOMING;
      flagNewMail = 0;
   }

#ifdef USE_INBOX
   // create INBOX
   static const char *INBOX_NAME = "INBOX";
   if(! MailFolder::CreateFolder(INBOX_NAME,
                                 MF_INBOX,
                                 MF_FLAGS_DONTDELETE |
                                 flagInbox,
                                 "",
                                 _("Default system folder for incoming mail.")))
   {
      wxLogError(_("Could not create INBOX mailbox."));
   }
   else
   {
      MFolder_obj folder(INBOX_NAME);
      folder->SetTreeIndex(MFolderIndex_Inbox);
   }
#endif // USE_INBOX

   // create New Mail folder:
   wxString foldername = READ_CONFIG(profile, MP_NEWMAIL_FOLDER);
   if(foldername.Length() == 0)
   {
      // shouldn't happen unless run for the first time
      foldername = _("New Mail");
      profile->writeEntry(MP_NEWMAIL_FOLDER, foldername);
   }

   if(! MailFolder::CreateFolder(foldername,
                                 MF_FILE,
                                 MF_FLAGS_NEWMAILFOLDER |
                                 flagNewMail,
                                 foldername,
                                 _("Mailbox in which Mahogany collects all new messages.")))
   {
      wxLogError(_("Could not create central incoming mailbox '%s'."), foldername.c_str());
   }
   else
   {
      MFolder_obj folder(foldername);
      folder->SetTreeIndex(MFolderIndex_NewMail);
   }

   // VZ: MP_SHOW_NEWMAILMSG is now on by default for all folders, so this code
   //     is not needed any more
#if 0
   // by default, activate new mail notification for the folder which
   // is used and user visible, default for all other folders is "off"
   Profile_obj prof(GetMainMailFolderName());
   prof->writeEntry(MP_SHOW_NEWMAILMSG, 1);
#endif // 0

   // TRASH
   if(gs_installWizardData.useTrash)
   {
      // VZ: why is this one translated and all others are not?
      String nameTrash = _("Trash");
      profile->writeEntry(MP_USE_TRASH_FOLDER, 1l);
      profile->writeEntry(MP_TRASH_FOLDER, nameTrash);
      if(! MailFolder::CreateFolder(nameTrash,
                                    MF_FILE,
                                    MF_FLAGS_DONTDELETE,
                                    nameTrash,
                                    _("Trash folder for deleted messages.") ) )
      {
         wxLogError(_("Could not create Trash mailbox."));
      }
      else
      {
         MFolder_obj folder(nameTrash);
         folder->SetTreeIndex(MFolderIndex_Trash);
      }

      // the rest is done in Update()
   }
   else
   {
      profile->writeEntry(MP_USE_TRASH_FOLDER, 0l);
   }


   // local newsspool ?
   {
      // calling c-client lib:
      String newsspool = MailFolderCC::GetNewsSpool();
      if(wxDirExists(newsspool))
      {
         String foldername = _("News-spool");
         if(! MailFolder::CreateFolder(foldername,
                                       MF_NEWS,
                                       MF_FLAGS_GROUP,
                                       "",
                                       _("Local news-spool found on your system during installation.")))
            wxLogError(_("Could not create an entry for the local news spool."));
      }
   }

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

   if(gs_installWizardData.folderType != MP_FOLDER_FILE_DRIVER_D)
      profile->writeEntry(MP_FOLDER_FILE_DRIVER, gs_installWizardData.folderType);

#ifdef USE_PYTHON
   if(gs_installWizardData.usePython)
      profile->writeEntry(MP_USEPYTHON, 1l);
   else
      profile->writeEntry(MP_USEPYTHON, 0l);
#endif

#ifdef USE_PISOCK
   // PalmOS-box
   if(gs_installWizardData.usePalmOs)
   {
      Profile *pp = Profile::CreateModuleProfile("PalmOS");
      pp->writeEntry("PalmBox", "PalmBox");
      pp->DecRef();
      if(! MailFolder::CreateFolder("PalmBox",
                                    MF_FILE,
                                    0,
                                    "PalmBox",
                                    _("This folder and its configuration settings\n"
                                      "are used to synchronise with your PalmOS\n"
                                      "based handheld computer.") ) )
         wxLogError(_("Could not create PalmOS mailbox."));
      else
      {
         pp = Profile::CreateProfile("PalmBox");
         pp->writeEntry("Icon", wxFolderTree::iconPalmPilot);
         pp->DecRef();
         MDialog_Message(_(
            "Set up the `PalmBox' mailbox used to synchronise\n"
            "e-mail with your handheld computer.\n"
            "Please use the Plugin-menu to configure\n"
            "the PalmOS support."));

      }

      // the rest is done in Update()
   }
#endif
}

#endif // USE_WIZARD

static bool
UpgradeFromNone()
{
#ifdef USE_WIZARD // new code which uses the configuration wizard
   (void)RunInstallWizard();
   if ( gs_installWizardData.sendTestMsg )
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
   {
      VerifyMailConfig();
   }

   return true;
}

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

#define COPYENTRY(type)  { type val; rc &= _this->Read(entry, &val); rc &= dest->Write(newentry,val); }

/** Copies all entries and optionally subgroups from path from to path
    to in the wxConfig.
    NOTE: Currently both from and to should be absolute paths!
    @param from  absolute group from where to copy entries
    @param to    absolute group where to copy to
    @param recursive if true, copy all subgroups, too
    @param dest      if non-NULL, use that config as the destination otherwise use this
    @return false on error
*/
static bool
CopyEntries(wxConfigBase *_this,
            const wxString &from, const wxString &to,
            bool recursive = TRUE,
            wxConfigBase *dest = NULL)
{
   wxString oldPath = _this->GetPath();
   bool rc = TRUE;

   if(dest == NULL) dest = _this;

   // Build a list of all entries to copy:
   _this->SetPath(from);

   long
      index = 0;
   wxString
      entry, newentry;
   bool ok;
   for ( ok = _this->GetFirstEntry(entry, index);
         ok ;
         ok = _this->GetNextEntry(entry, index))
   {
      newentry = to;
      newentry << '/' << entry;
      switch(_this->GetEntryType(entry))
      {
      case wxConfigBase::Type_String:
         COPYENTRY(wxString); break;
      case wxConfigBase::Type_Integer:
         COPYENTRY(long); break;
      case wxConfigBase::Type_Float:
         COPYENTRY(double); break;
      case wxConfigBase::Type_Boolean:
         COPYENTRY(bool); break;
      case wxConfigBase::Type_Unknown:
         wxFAIL_MSG("unexpected entry type");
      }
   }
   if(recursive)
   {
      wxString
         fromgroup, togroup;
      wxString
         *groups;

      size_t
         idx = 0,
         n = _this->GetNumberOfGroups(FALSE);
      if(n > 0)
      {
         groups = new wxString[n];
         index = 0;
         for ( ok = _this->GetFirstGroup(entry, index);
               ok ;
               ok = _this->GetNextGroup(entry, index))
         {
            wxASSERT(idx < n);
            groups[idx++] = entry;
         }
         for(idx = 0; idx < n; idx++)
         {
            fromgroup = from;
            fromgroup << '/' << groups[idx];
            togroup = to;
            togroup << '/' << groups[idx];
            rc &= CopyEntries(_this, fromgroup, togroup, recursive, dest);
         }
         delete [] groups;
      }
   }

   _this->SetPath(oldPath);
   return rc;
}

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
   rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                     "/M/Profiles/Folders","/M/Profiles", true);

   rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                     "/AdbEditor","/M/Profiles/AdbEditor", true);

   Profile
      *p = Profile::CreateProfile(""),
      *p2;
   kbStringList
      folders;
   String
      group, pw, tmp;
   long
      index = 0;

   // We need to rename the old mainfolder, to remove its leading slash:
   String mainFolder = READ_CONFIG(p, MP_MAINFOLDER);
   if(mainFolder.Length())
   {
      if(mainFolder[0u] == '/')
      {
         wxString tmp = mainFolder.Mid(1);
         mainFolder = tmp;
         p->writeEntry(MP_MAINFOLDER, mainFolder);
      }
   }

   //FIXME paths need adjustment for windows?
   wxConfigBase *c = mApplication->GetProfile()->GetConfig();
   // Delete obsolete groups:
   c->DeleteGroup("/M/Profiles/Folders");
   c->DeleteGroup("/AdbEditor");

   /* Encrypt passwords in new location and make sure we have no
      illegal old profiles around. */
   p->ResetPath(); // to be safe
   for ( bool ok = p->GetFirstGroup(group, index);
         ok ;
         ok = p->GetNextGroup(group, index))
   {
      tmp = group;
      tmp << '/' << MP_PROFILE_TYPE;
      if(p->readEntry(tmp, MP_PROFILE_TYPE_D) == Profile::PT_FolderProfile)
         folders.push_back(new String(group));
   }
   for(kbStringList::iterator i = folders.begin(); i != folders.end();i++)
   {
      group = **i;
      p->SetPath(group);
      if(p->readEntry(MP_FOLDER_TYPE, MP_FOLDER_TYPE_D) != MP_FOLDER_TYPE_D)
      {
         p2 = Profile::CreateProfile(group);
         pw = p2->readEntry(MP_FOLDER_PASSWORD, MP_FOLDER_PASSWORD_D);
         if(pw.Length()) // only if we have a password
            p2->writeEntry(MP_FOLDER_PASSWORD, strutil_encrypt(pw));
         p2->DecRef();
         p->ResetPath();
      }
      else
      {
         p->ResetPath();
         p->DeleteGroup(group);
         String msg = _("Deleted illegal folder profile: '");
         msg << p->GetName() << '/' << group << '\'';
         wxLogMessage(msg);
      }
   }
   p->DecRef();
   //FIXME check returncodes!


   return rc;

}



class UpgradeFolderTraversal : public MFolderTraversal
{
public:
   UpgradeFolderTraversal(MFolder* folder) : MFolderTraversal(*folder)
      { }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         Profile_obj profile(folderName);
         bool found;
         String hostname = profile->readEntry(MP_OLD_FOLDER_HOST, "", &found);
         if ( found )
         {
            // delete the old entry, create the new one
            wxConfigBase *config = profile->GetConfig();
            if ( config )
            {
               config->DeleteEntry(MP_OLD_FOLDER_HOST);

               wxLogTrace("Successfully converted folder '%s'",
                          folderName.c_str());
            }
            else
            {
               // what can we do? nothing...
               FAIL_MSG( "profile without config - can't delete entry" );
            }

            profile->writeEntry(MP_IMAPHOST, hostname);
            profile->writeEntry(MP_POPHOST, hostname);
         }

         return TRUE;
      }
};

static bool
UpgradeFrom020()
{
   /* Config structure incompatible changes from 0.20a to 0.21a:
      in the folder settings, HostName has become ServerName
    */

   // enumerate all folders recursively
   MFolder_obj folderRoot("");
   UpgradeFolderTraversal traverse(folderRoot);
   traverse.Traverse();

   // TODO it would be very nice to purge the redundant settings from config
   //      because the older versions wrote everything to it and it has only
   //      been fixed in 0.21a -- but this seems a bit too complicated

   return TRUE;
}

class TemplateFixFolderTraversal : public MFolderTraversal
{
public:
   TemplateFixFolderTraversal(MFolder* folder) : MFolderTraversal(*folder)
   {
      m_ok = TRUE;
   }

   bool IsOk() const { return m_ok; }

   virtual bool OnVisitFolder(const wxString& folderName)
   {
      Profile_obj profile(folderName);
      String group = M_TEMPLATE_SECTION;
      if ( profile->HasGroup(group) )
      {
         static const char *templateKinds[] =
         {
            MP_TEMPLATE_NEWMESSAGE,
            MP_TEMPLATE_NEWARTICLE,
            MP_TEMPLATE_REPLY,
            MP_TEMPLATE_FOLLOWUP,
            MP_TEMPLATE_FORWARD,
         };

         wxLogTrace("Updating templates for the folder '%s'...",
                    folderName.c_str());

         for ( size_t n = 0; n < WXSIZEOF(templateKinds); n++ )
         {
            String entry = group + templateKinds[n];
            if ( profile->HasEntry(entry) )
            {
               String templateValue = profile->readEntry(entry, "");

               String entryNew;
               entryNew << M_TEMPLATES_SECTION
                        << templateKinds[n] << '/'
                        << folderName;
               Profile *profileApp = mApplication->GetProfile();
               if ( profileApp->HasEntry(entryNew) )
               {
                  wxLogWarning("A profile entry '%s' already exists, "
                               "impossible to upgrade the existing template "
                               "in '%s/%s/%s'",
                               entryNew.c_str(),
                               folderName.c_str(),
                               group.c_str(),
                               entry.c_str());

                  m_ok = false;
               }
               else
               {
                  wxLogTrace("\t%s/%s/%s upgraded to %s",
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
      return TRUE;
   }

private:
   bool m_ok;
};

static bool
UpgradeFrom050()
{
   // See the comments in MessageTemplate.h about the changes in the templates
   // location. Briefly, the templates now live in /Templates and not in each
   // folder and the <folder>/Template/<kind> profile entry now contains the
   // name of the template to use and not the contents of it

   // enumerate all folders recursively
   MFolder_obj folderRoot("");
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

static bool
UpgradeFrom060()
{
   // FIXME: this won't work if the user has changed the names of the system
   //        folders or even if they are just translated (but then I think
   //        only trash is translated anyhow)

   static const struct
   {
      const char *name;
      int pos;
   } SystemFolders[] =
   {
      { "INBOX",     MFolderIndex_Inbox   },
      { "New Mail",  MFolderIndex_NewMail },
      { "SentMail",  MFolderIndex_SentMail},
      { "Trash",     MFolderIndex_Trash   },
      { "Outbox",    MFolderIndex_Outbox  },
      { "Draft",     MFolderIndex_Draft   },
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
// global functions
// ----------------------------------------------------------------------------

// upgrade from the specified version to the current one
extern bool
Upgrade(const String& fromVersion)
{
   // what is the old version?
   MVersion oldVersion;
   if ( fromVersion.IsEmpty() )
      oldVersion = Version_None;
   else
   {
      wxString version = fromVersion;

      // terminating 'a' means alpha, just remove it for the purposes of
      // version comparison
      if ( version.Last() == 'a' )
         version.Truncate(version.Len() - 1);

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
      else if ( version == "0.61" )
         oldVersion = Version_NoChange;
      else
         oldVersion = Version_Unknown;
   }

   bool success = TRUE;
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
      if ( success && UpgradeFrom060() )
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
      break;

   case Version_061:
   case Version_NoChange:
      break;

   default:
      FAIL_MSG("invalid version value");
      // fall through

   case Version_Unknown:
      wxLogError(_("The previously installed version of Mahogany (%s) was "
                   "probably newer than this one. Cannot upgrade."),
                 fromVersion.c_str());
      return FALSE;
   }

   return TRUE;
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
                 FolderType type = GetFolderType(typeAndFlags);
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

/** Make sure we have /Profiles/INBOX set up and the global
    NewMailFolder folder.
    Returns TRUE if the profile already existed, FALSE if it was just created
 */
extern bool
VerifyInbox(void)
{
   bool rc = TRUE;
   Profile *parent = mApplication->GetProfile();
   // INBOX has no meaning under Windows
#ifndef OS_WIN
   // Do we need to create the INBOX (special folder for incoming mail)?
   if ( parent->HasEntry("INBOX") )
      rc = TRUE;
   else
   {
      rc = FALSE;
      Profile *ibp = Profile::CreateProfile("INBOX");
      ibp->writeEntry(MP_PROFILE_TYPE, Profile::PT_FolderProfile);
      if(READ_APPCONFIG(MP_NEWMAIL_FOLDER) != "INBOX"
         && MDialog_YesNoDialog(
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
            NULL, _("Collect mail from INBOX?"), true))
         ibp->writeEntry(MP_FOLDER_TYPE, MF_INBOX|MF_FLAGS_INCOMING|MF_FLAGS_DONTDELETE);
      else
      {
         ibp->writeEntry(MP_FOLDER_TYPE, MF_INBOX|MF_FLAGS_DONTDELETE);
         MDialog_Message(_(
            "Mahogany will not automatically collect mail from your\n"
            "system's default INBOX.\n"
            "You can change this in the INBOX folder's preferences\n"
            "dialog at any time."));

      }
      ibp->writeEntry(MP_FOLDER_COMMENT, _("Default system folder for incoming mail."));
      ibp->DecRef();
   }
#endif // OS_WIN

   /*
    * Is the newmail folder properly configured?
    */
   MFolder_obj folderRoot("");
   String foldername = READ_APPCONFIG(MP_NEWMAIL_FOLDER);
   if(foldername.IsEmpty()) // this must not be
   {
      foldername = _("New Mail");
      mApplication->GetProfile()->writeEntry(MP_NEWMAIL_FOLDER, foldername);
   }
   static const long flagsNewMail = MF_FLAGS_DEFAULT|MF_FLAGS_NEWMAILFOLDER;
   // Do we need to create the NewMailFolder?
   Profile *ibp = Profile::CreateProfile(foldername);
   if (!  parent->HasEntry(foldername) )
   {
      ibp->writeEntry(MP_PROFILE_TYPE, Profile::PT_FolderProfile);
      ibp->writeEntry(MP_FOLDER_TYPE,
                      CombineFolderTypeAndFlags(MF_FILE, flagsNewMail));
      ibp->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
      ibp->writeEntry(MP_FOLDER_COMMENT,
                      _("Folder where Mahogany will collect all new mail."));
      rc = FALSE;
   }

   // Make sure the flags and type are valid
   int typeAndFlags = READ_CONFIG(ibp,MP_FOLDER_TYPE);
   FolderType type = GetFolderType(typeAndFlags);
   int oldflags = GetFolderFlags(typeAndFlags);
   int flags = oldflags;
   if( flags & MF_FLAGS_INCOMING )
      flags ^= MF_FLAGS_INCOMING;
   flags |= flagsNewMail;
   if( (flags != oldflags) || (GetFolderType(typeAndFlags) != MF_FILE) )
   {
      ibp->writeEntry(MP_FOLDER_TYPE,
                      CombineFolderTypeAndFlags(type, flags));
   }
   ibp->DecRef();

   /*
    * Set up the SentMail folder:
    */
   if( READ_APPCONFIG(MP_USEOUTGOINGFOLDER) )
   {
      // this line is for backwards compatibility only
      foldername = READ_APPCONFIG(MP_OUTGOINGFOLDER);
      strutil_delwhitespace(foldername);
      // We have to stick with the following for now, until we add
      // some upgrade functionality which can set this to a different
      // (translated) value.  "SentMail" is the old default name.
      if(foldername.Length() == 0)
         foldername = "SentMail";
      mApplication->GetProfile()->writeEntry(MP_OUTGOINGFOLDER, foldername);
      bool exists = parent->HasEntry(foldername);
      Profile *ibp = Profile::CreateProfile(foldername);
      if (! exists )
      {
         ibp->writeEntry(MP_PROFILE_TYPE, Profile::PT_FolderProfile);
         ibp->writeEntry(MP_FOLDER_TYPE, MF_FILE);
         ibp->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
         ibp->writeEntry(MP_FOLDER_COMMENT,
                         _("Folder where Mahogany will store copies of outgoing messages."));
         ibp->writeEntry(MP_FOLDER_TREEINDEX, MFolderIndex_SentMail);
         rc = FALSE;
      }
      // Make sure the flags are valid:
      flags = READ_CONFIG(ibp,MP_FOLDER_TYPE);
      oldflags = flags;
      if(flags & MF_FLAGS_INCOMING) flags ^= MF_FLAGS_INCOMING;
      if(flags & MF_FLAGS_DONTDELETE) flags ^= MF_FLAGS_DONTDELETE;
      if(flags != oldflags)
         ibp->writeEntry(MP_FOLDER_TYPE, flags);
      if(ibp->readEntry("Icon", -1) == -1)
         ibp->writeEntry("Icon", wxFolderTree::iconSentMail);
      ibp->DecRef();
   }

   /*
    * Set up the Trash folder:
    */
   if( READ_APPCONFIG(MP_USE_TRASH_FOLDER) )
   {
      foldername = READ_APPCONFIG(MP_TRASH_FOLDER);
      strutil_delwhitespace(foldername);
      if(foldername.Length() == 0)
      {
         foldername = _("Trash");
         wxLogError(_("The name for the trash folder was empty, using default '%s'."),
                    foldername.c_str());
      }
      mApplication->GetProfile()->writeEntry(MP_TRASH_FOLDER, foldername);
      Profile *p = Profile::CreateProfile(foldername);
      // Don't overwrite settings if entry already exists:
      if (!  parent->HasEntry(foldername) )
      {
         p->writeEntry(MP_PROFILE_TYPE, Profile::PT_FolderProfile);
         p->writeEntry(MP_FOLDER_TYPE, MF_FILE);
         p->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
         p->writeEntry(MP_FOLDER_COMMENT,
                         _("Folder where Mahogany will store deleted messages."));
         rc = FALSE;
      }
      // Make sure the flags are valid:
      flags = READ_CONFIG(p,MP_FOLDER_TYPE);
      oldflags = flags;
      if(flags & MF_FLAGS_INCOMING) flags ^= MF_FLAGS_INCOMING;
      if(flags & MF_FLAGS_DONTDELETE) flags ^= MF_FLAGS_DONTDELETE;
      if(flags != oldflags)
         p->writeEntry(MP_FOLDER_TYPE, flags);
      if(p->readEntry("Icon", -1) == -1)
         p->writeEntry("Icon", wxFolderTree::iconTrash);
      p->DecRef();
   }

   /*
    * Set up the Outbox folder:
    */
   if( READ_APPCONFIG(MP_USE_OUTBOX) )
   {
      foldername = READ_APPCONFIG(MP_OUTBOX_NAME);
      strutil_delwhitespace(foldername);
      if(foldername.Length() == 0)
      {
         foldername = _("Outbox");
         wxLogError(_("The name for the outgoing mailbox was empty, using default '%s'."),
                    foldername.c_str());
      }
      mApplication->GetProfile()->writeEntry(MP_OUTBOX_NAME, foldername);
      Profile *p = Profile::CreateProfile(foldername);
      // Don't overwrite settings if entry already exists:
      if (!  parent->HasEntry(foldername) )
      {
         p->writeEntry(MP_PROFILE_TYPE, Profile::PT_FolderProfile);
         p->writeEntry(MP_FOLDER_TYPE, MF_FILE|MF_FLAGS_KEEPOPEN);
         p->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
         p->writeEntry(MP_FOLDER_COMMENT,
                       _("Folder where Mahogany will store copies of outgoing messages."));
         rc = FALSE;
      }
      // Make sure the flags are valid:
      flags = READ_CONFIG(p,MP_FOLDER_TYPE);
      oldflags = flags;
      if(flags & MF_FLAGS_INCOMING) flags ^= MF_FLAGS_INCOMING;
      if(flags & MF_FLAGS_DONTDELETE) flags ^= MF_FLAGS_DONTDELETE;
      if(flags != oldflags)
         p->writeEntry(MP_FOLDER_TYPE, flags);
      if(p->readEntry("Icon", -1) == -1)
         p->writeEntry("Icon", wxFolderTree::iconOutbox);
      p->DecRef();
   }

   // Set up the treectrl to be expanded at startup:
   wxConfigBase *conf = mApplication->GetProfile()->GetConfig();
   conf->Write("Settings/TreeCtrlExp/FolderTree", 1L);
   return rc;
}


extern bool
VerifyMailConfig(void)
{
   // we send a mail to ourself
   Profile *p = mApplication->GetProfile();
   String me = miscutil_GetFromAddress(p);

   SendMessage *sm = SendMessage::Create(p);
   sm->SetSubject(_("Mahogany Test Message"));
   sm->SetAddresses(me);
   String msg =
      _("If you have received this mail, your Mahogany configuration works.\n"
        "You should also try to reply to this mail and check that your reply arrives.");
   sm->AddPart(Message::MSG_TYPETEXT, msg.c_str(), msg.length());
   bool ok = sm->SendOrQueue();
   delete sm;

   if ( ok )
   {
      mApplication->SendOutbox();
      msg.Empty();
      msg << _("Sent email message to:\n")
          << me
          << _("\n\nPlease check whether it arrives.");
      MDialog_Message(msg, NULL, _("Testing your configuration"), "TestMailSent");

      return true; // till we know something better
   }
   else
   {
      MDialog_ErrorMessage(_("Send the test message failed, please recheck "
                             "your configuration settings"));

      return false;
   }
}

static
void VerifyUserDir(void)
{
   String userdir = READ_APPCONFIG(MP_USERDIR);
   if ( !userdir )
   {
      Profile *profile = mApplication->GetProfile();
#if defined(OS_UNIX)
      userdir = getenv("HOME");
      userdir << DIR_SEPARATOR << READ_APPCONFIG(MP_USER_MDIR);
#elif defined(OS_WIN)
      // take the directory of the program
      char szFileName[MAX_PATH];
      if ( !GetModuleFileName(NULL, szFileName, WXSIZEOF(szFileName)) )
      {
         wxLogError(_("Cannot find your Mahogany directory, please specify it "
                      "in the options dialog."));
      }
      else
      {
         wxSplitPath(szFileName, &userdir, NULL, NULL);
      }
#else
#     error "Don't know how to find local dir under this OS"
#endif // OS

      // save it for the next runs
      profile->writeEntry(MP_USERDIR, userdir);
   }

   mApplication->SetLocalDir(userdir);
}

/**
  This function sets up folder entries for the servers, which can then
  be used for browsing them. Called only once when the application is
  initialised for the first time.
 */
static
void
SetupServers(void)
{
   String serverName;
   MFolder *mfolder;
   Profile *p;

   /* The NNTP server: */
   serverName = READ_APPCONFIG(MP_NNTPHOST);
   if ( !!serverName )
   {
      mfolder = CreateFolderTreeEntry(NULL,
                                      _("NNTP Server"),
                                      MF_NNTP,
                                      MF_FLAGS_ANON|MF_FLAGS_GROUP,
                                      "",
                                      FALSE);
      mfolder->SetTreeIndex(MFolderIndex_NNTP);

      p = Profile::CreateProfile(mfolder->GetName());
      //inherit default instead p->writeEntry(MP_NNTPHOST, serverName);
      p->DecRef();
      SafeDecRef(mfolder);
   }

   /* The IMAP server: */
   serverName = READ_APPCONFIG(MP_IMAPHOST);
   if ( !!serverName )
   {
      mfolder = CreateFolderTreeEntry(NULL,
                                      _("IMAP Server"),
                                      MF_IMAP,
                                      MF_FLAGS_GROUP,
                                      "",
                                      FALSE);
      mfolder->SetTreeIndex(MFolderIndex_IMAP);

      MFolder *imapInbox = CreateFolderTreeEntry(mfolder,
                                      _("IMAP INBOX"),
                                      MF_IMAP,
                                      MF_FLAGS_DEFAULT,
                                      "INBOX",
                                      FALSE);
      SafeDecRef(imapInbox);

      p = Profile::CreateProfile(mfolder->GetName());

      // inherit default instead
      //p->writeEntry(MP_IMAPHOST, serverName);

      p->DecRef();
      SafeDecRef(mfolder);
   }

   /* The POP3 server: */
   serverName = READ_APPCONFIG(MP_POPHOST);
   if ( !!serverName )
   {
      // the POP3 folder is created as incoming as otherwise it doesn't work
      // really well
      mfolder = CreateFolderTreeEntry(NULL,
                                      _("POP3 Server"),
                                      MF_POP,
                                      MF_FLAGS_INCOMING,
                                      "",
                                      FALSE);
      mfolder->SetTreeIndex(MFolderIndex_POP);
      p = Profile::CreateProfile(mfolder->GetName());
      // inherit default instead:
      //p->writeEntry(MP_POPHOST, serverName);
      //p->writeEntry(MP_USERNAME, READ_APPCONFIG(MP_USERNAME));
      p->DecRef();
      SafeDecRef(mfolder);
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

   // this is normally already done, but better be safe
   VerifyUserDir();

   // now that we have the local dir, we can set up a default mail folder dir
   String str = READ_APPCONFIG(MP_MBOXDIR);
   if( strutil_isempty(str) )
   {
      str = READ_APPCONFIG(MP_USERDIR);
      profile->writeEntry(MP_MBOXDIR, str);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_HOSTNAME)) )
   {
      profile->writeEntry(MP_HOSTNAME, wxGetFullHostName());
   }

   if( strutil_isempty(READ_APPCONFIG(MP_NNTPHOST)) )
   {
      char *cptr = getenv("NNTPSERVER");
      if(!cptr || !*cptr)
        cptr = MP_NNTPHOST_FB;
      profile->writeEntry(MP_NNTPHOST, cptr);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_SMTPHOST)) )
   {
      char *cptr = getenv("SMTPSERVER");
      if(!cptr || !*cptr)
        cptr = MP_SMTPHOST_FB;
      profile->writeEntry(MP_SMTPHOST, cptr);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_POPHOST)) )
   {
      char *cptr = getenv("POPSERVER");
      if(cptr && *cptr)
         profile->writeEntry(MP_POPHOST, cptr);
   }
   if( strutil_isempty(READ_APPCONFIG(MP_IMAPHOST)) )
   {
      char *cptr = getenv("IMAPSERVER");
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

#define M_SYNCMAIL_SUBJECT   "DO NOT DELETE! Mahogany remote configuration info"
#define M_SYNCMAIL_CONFIGSTART "Mahogany configuration info:"

static time_t  gs_RemoteSyncDate = 0;

extern
bool RetrieveRemoteConfigSettings(bool confirm)
{
   if ( confirm )
   {
      if(READ_APPCONFIG(MP_SYNC_REMOTE) == 0)
         return TRUE; // nothing to do

      if (! MDialog_YesNoDialog(
         _("Retrieve remote configuration settings now?"), NULL,
         _("Retrieve remote settings?"), true,
               GetPersMsgBoxName(M_MSGBOX_RETRIEVE_REMOTE) ) )
           return TRUE;
   }

   MailFolder *mf =
      MailFolder::OpenFolder(READ_APPCONFIG(MP_SYNC_FOLDER));

   if(! mf)
      return FALSE;

   if(mf->CountMessages() != 1)
   {
      if(mf->CountMessages() == 0)
         wxLogError(_("Configuration mailbox '%s' does not contain any "
                      "information."), mf->GetName().c_str());
      else
         wxLogError(
            _("Configuration mailbox '%s' contains more than\n"
              "one message. Possibly wrong mailbox specified?\n"
              "If this mailbox is the correct one, please remove\n"
              "the extra messages and try again."), mf->GetName().c_str());
      mf->DecRef();
      return FALSE;
   }
   HeaderInfoList *hil = mf->GetHeaders();
   Message * msg = mf->GetMessage( (*hil)[0]->GetUId() );
   if( msg == NULL)
      return FALSE; // what happened?
   if(msg->Subject() != M_SYNCMAIL_SUBJECT)
   {
      wxLogError(
         _("The message in the configuration mailbox '%s' does not\n"
           "contain configuration settings. Please remove it."),
         mf->GetName().c_str());
      mf->DecRef();
      msg->DecRef();
      hil->DecRef();
      return FALSE;
   }

   wxString msgText = msg->FetchText();
   gs_RemoteSyncDate = (*hil)[0]->GetDate();
   hil->DecRef();
   msg->DecRef();
   mf->DecRef();

   msgText = msgText.Mid( msgText.Find(M_SYNCMAIL_CONFIGSTART) +
                          strlen( M_SYNCMAIL_CONFIGSTART) );
   wxString filename = wxGetTempFileName("MTemp");
   wxFile tmpfile(filename, wxFile::write);
   tmpfile.Write(msgText, msgText.Length());
   tmpfile.Close();

   wxFileConfig *fc = new
      wxFileConfig("","",filename,"",wxCONFIG_USE_LOCAL_FILE);

   bool rc = TRUE;
   if(fc)
   {
      if(READ_APPCONFIG(MP_SYNC_FILTERS) != 0)
         rc &= CopyEntries(fc,
                           M_FILTERS_CONFIG_SECTION,
                           M_FILTERS_CONFIG_SECTION_UNIX, TRUE,
                           mApplication->GetProfile()->GetConfig());

      if(READ_APPCONFIG(MP_SYNC_IDS) != 0)
         rc &= CopyEntries(fc,
                           M_IDENTITY_CONFIG_SECTION,
                           M_IDENTITY_CONFIG_SECTION_UNIX, TRUE,
                           mApplication->GetProfile()->GetConfig());

      if(READ_APPCONFIG(MP_SYNC_FOLDERS) != 0)
      {
         String group = READ_APPCONFIG(MP_SYNC_FOLDERGROUP);
         String src, dest;
         src << M_PROFILE_CONFIG_SECTION << '/' << group;
         dest << M_PROFILE_CONFIG_SECTION_UNIX << '/' << group;
         rc &= CopyEntries(fc,
                           src, dest, TRUE,
                           mApplication->GetProfile()->GetConfig());
      }
   }
   else
      rc = FALSE;
   delete  fc;
   wxRemoveFile(filename);
   return rc;
}

extern
bool SaveRemoteConfigSettings(bool confirm)
{
   if ( confirm )
   {
      if(READ_APPCONFIG(MP_SYNC_REMOTE) == 0)
         return TRUE; // nothing to do

      if (! MDialog_YesNoDialog(
         _("Store remote configuration settings now?"), NULL,
         _("Store remote settings?"), true,
               GetPersMsgBoxName(M_MSGBOX_STORE_REMOTE) ) )
           return TRUE;
   }

   MailFolder *mf =
      MailFolder::OpenFolder(READ_APPCONFIG(MP_SYNC_FOLDER));

   if(! mf)
      return FALSE;

   if(mf->CountMessages() > 1)
   {
      wxLogError(
         _("Configuration mailbox '%s' contains more than\n"
           "one message. Possibly wrong mailbox specified?\n"
           "If this mailbox is the correct one, please remove\n"
           "the extra messages and try again."), mf->GetName().c_str());
      mf->DecRef();
      return FALSE;
   }
   // If we have information stored there, delete it:
   if(mf->CountMessages() != 0)
   {
      HeaderInfoList *hil = mf->GetHeaders();
      Message * msg = mf->GetMessage( (*hil)[0]->GetUId() );
      time_t storedDate = (*hil)[0]->GetDate();
      hil->DecRef();
      if( msg == NULL)
         return FALSE; // what happened?
      if(msg->Subject() != M_SYNCMAIL_SUBJECT)
      {
         wxLogError(
            _("The message in the configuration mailbox '%s' does not\n"
              "contain configuration settings. Please remove it."),
            mf->GetName().c_str());
         mf->DecRef();
         msg->DecRef();
         return FALSE;
      }
      if(gs_RemoteSyncDate != 0 &&
         storedDate > gs_RemoteSyncDate)
      {
         if (! MDialog_YesNoDialog(
            _("The remotely stored configuration information seems to have changed\n"
              "since it was retrieved.\n"
              "Are you sure you want to overwrite it with the current settings?"),
            NULL,
            _("Overwrite remote settings?"),
            true,
            GetPersMsgBoxName(M_MSGBOX_OVERWRITE_REMOTE) ) )
         {
            mf->DecRef();
            msg->DecRef();
            return FALSE;
         }
      }
      if(! mf->DeleteMessage(msg->GetUId()))
      {
         wxLogError(
            _("Cannot remove old configuration information from\n"
              "mailbox '%s'."), mf->GetName().c_str());
         mf->DecRef();
         msg->DecRef();
         return FALSE;
      }
      msg->DecRef();
      mf->ExpungeMessages();
   }

   wxString filename = wxGetTempFileName("MTemp");
   wxFileConfig *fc = new
      wxFileConfig("","",filename,"",wxCONFIG_USE_LOCAL_FILE);
   if(!fc)
   {
      mf->DecRef();
      return FALSE;
   }

   bool rc = TRUE;
   if(READ_APPCONFIG(MP_SYNC_FILTERS) != 0)
      rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                        M_FILTERS_CONFIG_SECTION_UNIX,
                        M_FILTERS_CONFIG_SECTION, TRUE,
                        fc);

   if(READ_APPCONFIG(MP_SYNC_IDS) != 0)
      rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                        M_IDENTITY_CONFIG_SECTION_UNIX,
                        M_IDENTITY_CONFIG_SECTION, TRUE,
                        fc);

   if(READ_APPCONFIG(MP_SYNC_FOLDERS) != 0)
   {
      String group = READ_APPCONFIG(MP_SYNC_FOLDERGROUP);
      String src, dest;
      src << M_PROFILE_CONFIG_SECTION_UNIX << '/' << group;
      dest << M_PROFILE_CONFIG_SECTION << '/' << group;
      rc &= CopyEntries(mApplication->GetProfile()->GetConfig(),
                        src, dest, TRUE,
                        fc);
   }
   delete  fc;
   if(rc == FALSE)
   {
      wxLogError(_("Could not export configuration information."));
      mf->DecRef();
      return FALSE;
   }
   wxFile tmpfile(filename, wxFile::read);
   char * buffer = new char [ tmpfile.Length() + 1];
   if(tmpfile.Read(buffer, tmpfile.Length()) != tmpfile.Length())
   {
      wxLogError(_("Cannot read configuration info from temporary file\n"
                   "'%s'."), filename.c_str());
      tmpfile.Close();
      delete [] buffer;
      mf->DecRef();
      return FALSE;
   }
   buffer[tmpfile.Length()] = '\0';
   tmpfile.Close();

   wxString msgText;
   msgText << "From: mahogany-developers@lists.sourceforge.net\n"
           << "Subject: " << M_SYNCMAIL_SUBJECT << "\n"
           << "Date: " << GetRFC822Time() << "\n"
           << "\n"
           << M_SYNCMAIL_CONFIGSTART << "\n"
           << buffer << "\n";
   delete [] buffer;
   if( ! mf->AppendMessage(msgText) )
   {
      wxLogError(_("Storing configuration information in mailbox\n"
                   "'%s' failed."), mf->GetName().c_str());
      rc = FALSE;
   }
   mf->DecRef();

   if(rc)
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

   if ( READ_APPCONFIG(MP_LICENSE_ACCEPTED) == 0 ) // not accepted
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

         return FALSE;
      }
   }

   // this is vital: we must have the local directory to read address books
   // and other M files from it
   VerifyUserDir();

   bool firstRun = READ_APPCONFIG(MP_FIRSTRUN) != 0;
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

      if ( ! Upgrade(version) )
         return false;
      // write the new version
      profile->writeEntry(MP_VERSION, M_VERSION);
   }

   // VerifyInbox() will always return FALSE during the first run, don't
   // insult the user (just yet ;-)
   if ( !VerifyInbox() && !firstRun )
      wxLogDebug("Evil user corrupted his profile settings - restored.");

   if ( !RetrieveRemoteConfigSettings() )
      wxLogError(_("Remote configuration information could not be retrieved."));

   return true;
}

