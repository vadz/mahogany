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
#endif  //USE_PCH

#  include "Message.h"
#  include "MailFolder.h"
#  include "MailFolderCC.h"
#  include "SendMessageCC.h"
#  include "MessageCC.h"

#ifdef USE_PYTHON
#  include <Python.h>
#  include "PythonHelp.h"
#  include "MScripts.h"
#endif

#include "Mdefaults.h"

#include <wx/log.h>
#include <wx/confbase.h>
#include <wx/utils.h>         // wxGetFullHostName()
#include <wx/dialup.h>        // for IsAlwaysOnline()

#define USE_WIZARD

#ifdef USE_WIZARD
#   include <wx/wizard.h>
#else
#   include "gui/wxOptionsDlg.h" // for ShowOptionsDialog()
#endif

#include "gui/wxDialogLayout.h"
#include "gui/wxFolderTree.h"

#include "Mupgrade.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// this enum contains only versions with incompatible changes between them
enum MVersion
{
   Version_None,     // this is the first installation of M on this machine
   Version_Alpha001, // first public version
   Version_Alpha010, // some config strucutre changes (due to wxPTextEntry)
   Version_Alpha020, // folder host name is now ServerName, not HostName
   Version_050,      // nothing really changed against 0.2x config-wise
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
   InstallWizard_IdentityPage,         // ask name, e-mail
   InstallWizard_ServersPage,          // ask POP, SMTP, NNTP servers
   InstallWizard_OperationsPage,       // how we want Mahogany to work
   InstallWizard_DialUpPage,           // set up dial-up networking
//   InstallWizard_MiscPage,             // other common options
#ifdef USE_HELPERS_PAGE
//   InstallWizard_HelpersPage,          // external programs set up
#endif // USE_HELPERS_PAGE
//   InstallWizard_ImportPage,           // propose to import ADBs (and folders)
   InstallWizard_FinalPage,            // say that everything is ok
   InstallWizard_PagesMax,             // the number of pages
   InstallWizard_Done = -1             // invalid page index
};

// ----------------------------------------------------------------------------
// wizardry
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
#ifdef USE_PISOCK
   bool   usePalmOs;
#endif
#ifdef USE_PYTHON
   bool   usePython;
#endif
   bool   collectAllMail;
   // dial up page

#if defined(OS_WIN)
   String connection;
#elif defined(OS_UNIX)
   String dialCommand,
          hangupCommand;

   // helpers page
   String browser;
#endif // OS_UNIX
} gs_installWizardData;

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

   virtual bool TransferDataFromWindow()
   {
      wxString email = m_email->GetValue();
      gs_installWizardData.name = m_name->GetValue();
      gs_installWizardData.email = email;

      // if the email is user@some.where, then suppose that the servers are
      // pop.some.where &c
      wxString domain = email.AfterFirst('@');
      if ( !!domain )
      {
         AddDomain(gs_installWizardData.pop, domain);
         AddDomain(gs_installWizardData.smtp, domain);
         AddDomain(gs_installWizardData.imap, domain);
         AddDomain(gs_installWizardData.nntp, domain);
      }
      //else: no domain specified

      return TRUE;
   }

   virtual bool TransferDataToWindow()
   {
      m_name->SetValue(gs_installWizardData.name);
      m_email->SetValue(gs_installWizardData.email);
      return TRUE;
   }

   void AddDomain(wxString& server, const wxString& domain)
   {
      if ( server.Find('.') == wxNOT_FOUND )
      {
         server << '.' << domain;
      }
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
            wxLogError(_("You need to specify at the SMTP server to be able "
                         "to send email, please do it!"));
            return FALSE;
         }
         else
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
         if(gs_installWizardData.useDialUp == -1) // no setting yet
         {
            wxDialUpManager *man = wxDialUpManager::Create();
            // if we have a LAN connection, then we don't need to configure dial-up
            // networking, but if we don't, then we probably do
            gs_installWizardData.useDialUp = !man->IsAlwaysOnline();
         }
#ifdef USE_PYTHON
         m_UsePythonCheckbox->SetValue(gs_installWizardData.usePython != 0);
#endif
         m_UseDialUpCheckbox->SetValue(gs_installWizardData.useDialUp != 0);
#ifdef USE_PISOCK
         m_UsePalmOsCheckbox->SetValue(gs_installWizardData.usePalmOs != 0);
#endif
         m_UseOutboxCheckbox->SetValue(gs_installWizardData.useOutbox != 0);
         m_TrashCheckbox->SetValue(gs_installWizardData.useTrash != 0);
         m_CollectCheckbox->SetValue(gs_installWizardData.collectAllMail != 0);
         return TRUE;
      }

   virtual bool TransferDataFromWindow()
      {
#ifdef USE_PYTHON
         gs_installWizardData.usePython  = m_UsePythonCheckbox->GetValue();
#endif
#ifdef USE_PISOCK
         gs_installWizardData.usePalmOs  = m_UsePalmOsCheckbox->GetValue();
#endif
         gs_installWizardData.useDialUp  = m_UseDialUpCheckbox->GetValue();
         gs_installWizardData.useOutbox  = m_UseOutboxCheckbox->GetValue();
         gs_installWizardData.useTrash   = m_TrashCheckbox->GetValue();
         gs_installWizardData.collectAllMail = m_CollectCheckbox->GetValue();
         return TRUE;
      }
private:
   wxCheckBox *m_CollectCheckbox, *m_TrashCheckbox,
         *m_UseOutboxCheckbox, *m_UseDialUpCheckbox
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

   virtual bool TransferDataFromWindow()
   {
      return TRUE;
   }
};

#endif // USE_HELPERS_PAGE

class InstallWizardMiscPage : public InstallWizardPage
{
public:
   InstallWizardMiscPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
      return TRUE;
   }
};

class InstallWizardImportPage : public InstallWizardPage
{
public:
   InstallWizardImportPage(wxWizard *wizard);

   virtual bool TransferDataFromWindow()
   {
      return TRUE;
   }
};

class InstallWizardFinalPage : public InstallWizardPage
{
public:
   InstallWizardFinalPage(wxWizard *wizard);
};

// ----------------------------------------------------------------------------
// prototypes
// ----------------------------------------------------------------------------

// the function which runs the install wizard
extern bool RunInstallWizard();

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

InstallWizardPageId InstallWizardPage::GetPrevPageId() const
{
   int id = m_id - 1;
   if ( id == InstallWizard_DialUpPage && !ShouldShowDialUpPage() )
   {
      // skip it
      id--;
   }

   return id < 0 ? InstallWizard_Done : (InstallWizardPageId)id;
}

InstallWizardPageId InstallWizardPage::GetNextPageId() const
{
   int id = m_id + 1;
   if ( id == InstallWizard_DialUpPage && !ShouldShowDialUpPage() )
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
      // wizard will be cancelled
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
         CREATE_PAGE(Identity);
         CREATE_PAGE(Servers);
         CREATE_PAGE(Operations);
         CREATE_PAGE(DialUp);
//         CREATE_PAGE(Misc);
#ifdef USE_HELPERS_PAGE
//         CREATE_PAGE(Helpers);
#endif // USE_HELPERS_PAGE
//         CREATE_PAGE(Import);
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
   return m_useWizard ? InstallWizard_IdentityPage : InstallWizard_Done;
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
      "servers, or through the local mail spool.\n"
      "\n"
      "Mail servers may use POP3 or IMAP4 access,\n"
      "but you usually need only one of them\n"
      "(IMAP4 is much better and faster, so use it if you can).\n"
      "\n"
      "To read Usenet discussion groups, you need to\n"
      "specify the NNTP (news) server and to be able\n"
      "to send e-mail, an SMTP server is required.\n"
      "\n"
      "All of these fields may be filled later as well\n"
      "(and you will be able to specify multiple servers too).\n"
      "Finally, leave a field empty if you are not going to use\n"
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
   wxDialUpManager *dial = wxDialUpManager::Create();
   wxArrayString connections;
   size_t nCount = dial->GetISPNames(connections);
   delete dial;

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
   wxStaticText *text = new wxStaticText(this, -1, _(
      "Mahogany can either leave all messages in\n"
      "your system mailbox or create its own\n"
      "mailbox for new mail and collect new\n"
      "messages in there. This is recommended,\n"
      "especially when collecting from remote\n"
      "servers."
      ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(text);

   wxArrayString labels;
   labels.Add(_("&Collect new mail:"));
   labels.Add(_("Use &Trash mailbox:"));
   labels.Add(_("Use &Outbox queue:"));
   labels.Add(_("&Use dial-up network:"));
   labels.Add(_("&Load PalmOS support:"));
   labels.Add(_("Enable &Python:"));

   long widthMax = GetMaxLabelWidth(labels, panel);

   m_CollectCheckbox = panel->CreateCheckBox(labels[0], widthMax,NULL);
   wxStaticText *text2 = panel->CreateMessage(
      _(
         "\n"
         "Mahogany has two options for deleting\n"
         "messages. You can either mark messages\n"
         "as deleted and leave them around to be\n"
         "expunged later, or you can use a Trash\n"
         "folder where to move them to."
         ), m_CollectCheckbox);
   m_TrashCheckbox = panel->CreateCheckBox(labels[1], widthMax, text2);

   wxStaticText *text3 = panel->CreateMessage(
      _(
         "\n"
         "Mahogany can either send messages\n"
         "immediately or queue them and only\n"
         "send them on demand. This is especially\n"
         "recommended for dial-up networking."
         ), m_TrashCheckbox);
   m_UseOutboxCheckbox = panel->CreateCheckBox(labels[2], widthMax, text3);

   wxStaticText *text4 = panel->CreateMessage(
      _(
         "\n"
         "If you are using dial-up networking,\n"
         "Mahogany detect your connection status\n"
         "and optionally dial and hang-up."
         ), m_UseOutboxCheckbox);
   m_UseDialUpCheckbox = panel->CreateCheckBox(labels[3], widthMax, text4);

#if defined(USE_PISOCK) || defined(USE_PYTHON)
   wxControl *last = m_UseDialUpCheckbox;
#endif // USE_PISOCK || USE_PYTHON

#ifdef USE_PISOCK
   wxStaticText *text5 = panel->CreateMessage(
      _(
         "\n"
         "Do you have a PalmOS based handheld computer?\n"
         "Mahogany has special support build in to connect\n"
         "to these."
         ), last);
   m_UsePalmOsCheckbox = panel->CreateCheckBox(labels[4], widthMax, text5);
   last = m_UsePalmOsCheckbox;
#endif // USE_PISOCK
#ifdef USE_PYTHON
   wxStaticText *text6 = panel->CreateMessage(
      _(
         "\n"
         "Mahogany contains a built-in interpreter for\n"
         "the scripting language `Python'.\n"
         "This can be used to further customise and\n"
         "expand Mahogany.\n"
         "Would you like to enable it?"), last);
   m_UsePythonCheckbox = panel->CreateCheckBox(labels[5], widthMax, text6);
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
// InstallWizardImportPage
// ----------------------------------------------------------------------------
/*
InstallWizardImportPage::InstallWizardImportPage(wxWizard *wizard)
                       : InstallWizardPage(wizard, InstallWizard_ImportPage)
{
   // TODO autodetect all mailers installed and propose to import as much as
   //      possible.
   //
   // NB: use an external function for this, this functionality should be
   //     separate from the update module
   new wxStaticText(this, -1, "This page is under construction");
}
*/
// InstallWizardFinalPage
// ----------------------------------------------------------------------------

InstallWizardFinalPage::InstallWizardFinalPage(wxWizard *wizard)
                      : InstallWizardPage(wizard, InstallWizard_FinalPage)
{
   (void)new wxStaticText(this, -1, _(
      "Congratulations!\n"
      "You have successfully configured\n"
      "Mahogany and may now start using it.\n"
      "\n"
      "Please remember to consult the online help\n"
      "and the documentaion files included.\n"
      "\n"
      "You may want to have a look at the\n"
      "full set of program options, too.\n"
      "\n"
      "In case of a problem, consult the help\n"
      "system and, if you cannot resolve it,\n"
      "please visit our web site at\n"
      "http://www.wxwindows.org/Mahogany/\n"
      "\n"
      "You might also want to join the Mahogany\n"
      "mailing lists, which you can access via\n"
      "the web page."
      "\n"
      "\n"
      "We hope you will enjoy using Mahogany!\n"
      "                    The M-Team"
                                     ));
}

#endif // USE_WIZARD

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

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
#if defined(OS_WIN)
   gs_installWizardData.connection = READ_APPCONFIG(MP_NET_CONNECTION);
#elif defined(OS_UNIX)
   gs_installWizardData.dialCommand = READ_APPCONFIG(MP_NET_ON_COMMAND);
   gs_installWizardData.hangupCommand = READ_APPCONFIG(MP_NET_OFF_COMMAND);
#endif // platform

   gs_installWizardData.useOutbox = TRUE;
   gs_installWizardData.useTrash = TRUE;
   gs_installWizardData.collectAllMail = TRUE;
#ifdef USE_PYTHON
   gs_installWizardData.usePython = FALSE;
#endif
#ifdef USE_PISOCK
   gs_installWizardData.usePalmOs = TRUE;
#endif

   gs_installWizardData.email = READ_APPCONFIG(MP_RETURN_ADDRESS);
   if(gs_installWizardData.email.Length() == 0)
   {
      gs_installWizardData.email
         << READ_APPCONFIG(MP_USERNAME)
         << '@' << READ_APPCONFIG(MP_HOSTNAME);
   }
   gs_installWizardData.name = READ_APPCONFIG(MP_PERSONALNAME);
   gs_installWizardData.pop  = READ_APPCONFIG(MP_POPHOST);
   gs_installWizardData.imap = READ_APPCONFIG(MP_IMAPHOST);
   gs_installWizardData.smtp = READ_APPCONFIG(MP_SMTPHOST);
   gs_installWizardData.nntp = READ_APPCONFIG(MP_NNTPHOST);

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
   if ( wizard->RunWizard(gs_wizardPages[InstallWizard_WelcomePage]) )
   {
      // transfer the wizard settings from InstallWizardData

      ProfileBase * profile = mApplication->GetProfile();
      profile->writeEntry(MP_RETURN_ADDRESS,
                          gs_installWizardData.email);
      profile->writeEntry(MP_PERSONALNAME, gs_installWizardData.name);

      // write the values even if they're empty as otherwise we'd try to
      // create folders with default names - instead of not creating them at
      // all
      profile->writeEntry(MP_POPHOST, gs_installWizardData.pop);
      profile->writeEntry(MP_IMAPHOST, gs_installWizardData.imap);
      profile->writeEntry(MP_NNTPHOST, gs_installWizardData.nntp);
      profile->writeEntry(MP_SMTPHOST, gs_installWizardData.smtp);

      // load all modules by default:
      wxString modules = "Filters";
#ifdef USE_PISOCK
      if(gs_installWizardData.usePalmOs)
         modules += ":PalmOS";
#endif // USE_PISOCK

      profile->writeEntry(MP_MODULES, modules);

      CompleteConfiguration(gs_installWizardData);
   }

   wizard->Destroy();

   gs_isWizardRunning = false;
   SetupServers();

   // create a welcome message:

   //MessageCC *msg = Create(
   String msgFmt =
      _("From: m-users-subscribe@egroups.com\015\012"
        "Subject: Welcome to Mahogany!\015\012"
        "Date: %s\015\012"
        "\015\012"
        "Thank you for trying Mahogany!\015\012"
        "\015\012"
        "This mail and news client is developed as an OpenSource project by a\015\012"
        "team of volunteers from around the world.\015\012"
        "If you would like to contribute to its development, you are\015\012"
        "always welcome to join in.\015\012"
        "\015\012"
        "We also rely on you to report any bugs or wishes for improvements\015\012"
        "that you may have.\015\012"
        "\015\012"
        "Please visit our web pages at http://www.wxwindows.org/Mahogany/\015\012"
        "\015\012"
        "Also, reply to this e-mail message and you will automatically be\015\012"
        "added to the mailing list of Mahogany users, where you will find\015\012"
        "other users happy to share their experiences with you and help you\015\012"
        "get started.\015\012"
        "\015\012"
        "Your Mahogany Developers Team\015\012"
        );
   String msgString; msgString.Printf(msgFmt, __DATE__);
   MailFolder *mf = NULL;
   if(gs_installWizardData.collectAllMail)
   {
      mApplication->GetProfile()->writeEntry(MP_MAINFOLDER, READ_APPCONFIG(MP_NEWMAIL_FOLDER));
      mf = MailFolder::OpenFolder(MF_FILE, READ_APPCONFIG(MP_NEWMAIL_FOLDER));
   }
   else
   {
      mApplication->GetProfile()->writeEntry(MP_MAINFOLDER, "INBOX");
      mf = MailFolder::OpenFolder(MF_INBOX, "INBOX");
   }
   ASSERT_MSG(mf,"Cannot get main folder?");
   if(mf)
   {
      mf->AppendMessage(msgString);
      mf->DecRef();
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
   ProfileBase * profile = mApplication->GetProfile();

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
         wxLogError(_("Could not create the outgoing mailbox '%'."),
                    name.c_str());
   }
   else
   {
      profile->writeEntry(MP_USE_OUTBOX, 0l);
   }

   // INBOX/NEW MAIL: in any case, create both INBOX and NEW MAIL folders, but
   // keep one of them hidden depending on "Collect all mail" setting
   int flagInbox, flagNewMail;
   if ( gs_installWizardData.collectAllMail )
   {
      // hide INBOX, show the other one
      flagInbox = MF_FLAGS_HIDDEN | MF_FLAGS_KEEPOPEN;
      flagNewMail = 0;
   }
   else
   {
      // hie NEW MAIL, show INBOX
      flagInbox = 0;
      flagNewMail = MF_FLAGS_HIDDEN | MF_FLAGS_KEEPOPEN;
   }

   // create hidden INBOX
   if(! MailFolder::CreateFolder("INBOX",
                                 MF_INBOX,
                                 MF_FLAGS_INCOMING |
                                 MF_FLAGS_DONTDELETE |
                                 flagInbox,
                                 "",
                                 _("Default system folder for incoming mail.")))
   {
      wxLogError(_("Could not create INBOX mailbox."));
   }

   // create New Mail folder:
   wxString foldername = READ_CONFIG(profile, MP_NEWMAIL_FOLDER);
   if(foldername.Length() == 0) // shouldn't happen unless run for the first time
   {
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

   // TRASH
   if(gs_installWizardData.useTrash)
   {
      profile->writeEntry(MP_USE_TRASH_FOLDER, 1l);
      profile->writeEntry(MP_TRASH_FOLDER, _("Trash"));
      if(! MailFolder::CreateFolder(_("Trash"),
                                    MF_FILE,
                                    MF_FLAGS_DONTDELETE|MF_FLAGS_KEEPOPEN,
                                    _("Trash"),
                                    _("Trash folder for deleted messages.") ) )
         wxLogError(_("Could not create Trash mailbox."));
      // the rest is done in Update()
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
      ProfileBase *pp = ProfileBase::CreateModuleProfile("PalmOS");
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
         pp = ProfileBase::CreateProfile("PalmBox");
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
#else // old code which didn't use the setup wizard
   wxLog *log = wxLog::GetActiveTarget();
   if ( log ) {
      wxLogMessage(_("As it seems that you are running Mahogany for the first\n"
                     "time, you should probably set up some of the options\n"
                     "needed by the program (especially network parameters)."));
      log->Flush();
   }
   ShowOptionsDialog();
#endif

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

   ProfileBase
      *p = ProfileBase::CreateProfile(""),
      *p2;
   kbStringList
      folders;
   String
      group, pw, tmp;
   long
      index = 0;
   // We need to rename the old mainfolder, to remove its leading
   // slash:
   String
      mainFolder = p->readEntry(MP_MAINFOLDER,MP_MAINFOLDER_D);
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
      if(p->readEntry(tmp, MP_PROFILE_TYPE_D) == ProfileBase::PT_FolderProfile)
         folders.push_back(new String(group));
   }
   for(kbStringList::iterator i = folders.begin(); i != folders.end();i++)
   {
      group = **i;
      p->SetPath(group);
      if(p->readEntry(MP_FOLDER_TYPE, MP_FOLDER_TYPE_D) != MP_FOLDER_TYPE_D)
      {
         p2 = ProfileBase::CreateProfile(group);
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
   else if ( fromVersion == "0.01a" )
      oldVersion = Version_Alpha001;
   else if ( fromVersion == "0.02a" || fromVersion == "0.10a")
      oldVersion = Version_Alpha010;
   else if ( fromVersion == "0.20a" )
      oldVersion = Version_Alpha020;
   else if ( fromVersion == "0.21a" || fromVersion == "0.22a" ||
             fromVersion == "0.23a" || fromVersion == "0.50")
      oldVersion = Version_NoChange;
   else
      oldVersion = Version_Unknown;

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
      if ( success && UpgradeFrom020() )
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
   case Version_050:
   case Version_NoChange:
      break;
   default:
      FAIL_MSG("invalid version value");
      // fall through

   case Version_Unknown:
      wxLogError(_("The previously installed version of Mahogany was "
                   "probably newer than this one. Cannot upgrade."));
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
                             f->GetName().c_str(),
                             m_NewMailFolder.c_str()));
               f->SetFlags(f->GetFlags() & !MF_FLAGS_NEWMAILFOLDER);
            }
            else
            {
               m_NewMailFolder = f->GetName();
               if(f->GetFlags() & MF_FLAGS_INCOMING)
               {
                  ERRORMESSAGE((_("Cannot auto-collect mail from the new mail folder\n"
                                  "'%s'\n"
                                  "Corrected configuration data."),
                                f->GetName().c_str()));
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
   ProfileBase *parent = mApplication->GetProfile();
   // INBOX has no meaning under Windows
#ifndef OS_WIN
   // Do we need to create the INBOX (special folder for incoming mail)?
   if ( parent->HasEntry("INBOX") )
      rc = TRUE;
   else
   {
      rc = FALSE;
      ProfileBase *ibp = ProfileBase::CreateProfile("INBOX");
      ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
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
#endif

   /*
    * Is the newmail folder properly configured?
    */
   MFolder_obj folderRoot("");
   NewMailFolderTraversal traverse(folderRoot);
   traverse.Traverse(true); // ignore result
   String foldername = traverse.GetNewMailFolder();
   if(foldername.Length() == 0) // shouldn't happen unless run for the first time
      foldername = READ_APPCONFIG(MP_NEWMAIL_FOLDER);
   /* We need to keep the old "New Mail" name for now, to keep it
      backwards compatible. */
   if(foldername.IsEmpty()) // this must not be
      foldername = "New Mail";
   mApplication->GetProfile()->writeEntry(MP_NEWMAIL_FOLDER, foldername);
   strutil_delwhitespace(foldername);

   static const long flagsNewMail = MF_FLAGS_NEWMAILFOLDER |
                                    MF_FLAGS_DONTDELETE |
                                    MF_FLAGS_KEEPOPEN;

   // Do we need to create the NewMailFolder?
   ProfileBase *ibp = ProfileBase::CreateProfile(foldername);
   if (!  parent->HasEntry(foldername) )
   {
      ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
      ibp->writeEntry(MP_FOLDER_TYPE,
                      CombineFolderTypeAndFlags(MF_FILE, flagsNewMail));
      ibp->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
      ibp->writeEntry(MP_FOLDER_COMMENT,
                      _("Folder where Mahogany will collect all new mail."));
      rc = FALSE;
   }

   // Make sure the flags and type are valid
   int typeAndFlags = READ_CONFIG(ibp,MP_FOLDER_TYPE);
   int oldflags = GetFolderFlags(typeAndFlags);
   int flags = oldflags;
   if( flags & MF_FLAGS_INCOMING )
      flags ^= MF_FLAGS_INCOMING;
   flags |= flagsNewMail;
   if( (flags != oldflags) || (GetFolderType(typeAndFlags) != MF_FILE) )
   {
      ibp->writeEntry(MP_FOLDER_TYPE,
                      CombineFolderTypeAndFlags(MF_FILE, flags));
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
      ProfileBase *ibp = ProfileBase::CreateProfile(foldername);
      if (!  parent->HasEntry(foldername) )
      {
         ibp->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
         ibp->writeEntry(MP_FOLDER_TYPE, MF_FILE|MF_FLAGS_KEEPOPEN);
         ibp->writeEntry(MP_FOLDER_PATH, strutil_expandfoldername(foldername));
         ibp->writeEntry(MP_FOLDER_COMMENT,
                         _("Folder where Mahogany will store copies of outgoing messages."));
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
      ProfileBase *p = ProfileBase::CreateProfile(foldername);
      // Don't overwrite settings if entry already exists:
      if (!  parent->HasEntry(foldername) )
      {
         p->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
         p->writeEntry(MP_FOLDER_TYPE, MF_FILE|MF_FLAGS_KEEPOPEN);
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
      ProfileBase *p = ProfileBase::CreateProfile(foldername);
      // Don't overwrite settings if entry already exists:
      if (!  parent->HasEntry(foldername) )
      {
         p->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
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

   String me;
   me = READ_APPCONFIG(MP_RETURN_ADDRESS);
   if(me.Length() == 0)
      me << READ_APPCONFIG(MP_USERNAME) << '@' << READ_APPCONFIG(MP_HOSTNAME);

// FIXME we should make sure that hostname can be resolved - otherwise this
// message will stay in sendmail queue _forever_ (at least it does happen
// here with sendmail 8.8.8 running under Solaris 2.6)
   String nil;
   SendMessageCC  sm(mApplication->GetProfile());
   sm.SetSubject(_("Mahogany Test Message"));
   sm.SetAddresses(me);
   String msg =
      _("If you have received this mail, your Mahogany configuration works.\n"
        "You should also try to reply to this mail and check that your reply arrives.");
   sm.AddPart(Message::MSG_TYPETEXT, msg.c_str(), msg.length());
   sm.SendOrQueue();
   mApplication->SendOutbox();
   msg.Empty();
   msg << _("Sent email message to:\n")
       << me
       << _("\n\nPlease check whether it arrives.");
   MDialog_Message(msg, NULL, _("Testing your configuration"), "TestMailSent");

   return true; // till we know something better
}

static
void VerifyUserDir(void)
{
   ProfileBase *profile = mApplication->GetProfile();
#if defined(OS_UNIX)
   if( strutil_isempty(READ_APPCONFIG(MP_USERDIR)) )
   {
      wxString strHome;
      strHome = getenv("HOME");
      strHome << DIR_SEPARATOR << READ_APPCONFIG(MP_USER_MDIR);
      profile->writeEntry(MP_USERDIR, strHome);
   }
#elif defined(OS_WIN)
   if ( strutil_isempty(READ_APPCONFIG(MP_USERDIR)) )
   {
      // take the directory of the program
      char szFileName[MAX_PATH];
      if ( !GetModuleFileName(NULL, szFileName, WXSIZEOF(szFileName)) )
      {
         wxLogError(_("Cannot find your Mahogany directory, please specify it "
                      "in the options dialog."));
      }
      else
      {
         wxString localDir;
         wxSplitPath(szFileName, &localDir, NULL, NULL);
         profile->writeEntry(MP_USERDIR, localDir);
      }
   }
#else
#   error "Don't know how to find local dir under this OS"
#endif // OS

}

/*
  This function sets up folder entries for the servers, which can then
  be used for browsing them. Called only once when the application is
  initialised for the first time. */
static
void
SetupServers(void)
{
   String serverName;
   MFolder *mfolder;
   ProfileBase *p;

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
      p = ProfileBase::CreateProfile(mfolder->GetName());
      p->writeEntry(MP_NNTPHOST, serverName);
      p->DecRef();
      SafeDecRef(mfolder);
   }

   /* The IMAP server: */
   serverName = READ_APPCONFIG(MP_IMAPHOST);
   if ( !!serverName )
   {
      mfolder = CreateFolderTreeEntry(NULL,
                                      _("IMAP Server"),
                                      MF_IMAP                   ,
                                      MF_FLAGS_GROUP,
                                      "",
                                      FALSE);
      p = ProfileBase::CreateProfile(mfolder->GetName());
      p->writeEntry(MP_IMAPHOST, serverName);
      p->DecRef();
      SafeDecRef(mfolder);
   }

   /* The POP3 server: */
   serverName = READ_APPCONFIG(MP_POPHOST);
   if ( !!serverName )
   {
      mfolder = CreateFolderTreeEntry(NULL,
                                      _("POP3 Server"),
                                      MF_POP,
                                      0,
                                      "",
                                      FALSE);
      p = ProfileBase::CreateProfile(mfolder->GetName());
      p->writeEntry(MP_POPHOST, serverName);
      p->writeEntry(MP_USERNAME, READ_APPCONFIG(MP_USERNAME));
      p->DecRef();
      SafeDecRef(mfolder);
   }
}

/* Make sure we have the minimal set of things set up:

   MP_USERNAME, MP_PERSONALNAME,
   MP_USERDIR
   MP_MBOXDIR
   MP_HOSTNAME,
   MP_SMTPHOST, MP_NNTPHOST, MP_IMAPHOST, MP_POPHOST
*/
static
void
SetupMinimalConfig(void)
{
   String str;
   const size_t bufsize = 200;
   char  buffer[bufsize];

   ProfileBase *profile = mApplication->GetProfile();

   if( strutil_isempty(READ_APPCONFIG(MP_USERNAME)) )
   {
      wxGetUserId(buffer,bufsize); // contains , delimited fields of info
      str = strutil_before(str,',');
      profile->writeEntry(MP_USERNAME, buffer);
   }
   if( strutil_isempty(READ_APPCONFIG(MP_PERSONALNAME)) )
   {
      wxGetUserName(buffer,bufsize);
      profile->writeEntry(MP_PERSONALNAME, buffer);
   }

   VerifyUserDir();

   // now that we have the local dir, we can set up a default mail
   // folder dir
   str = READ_APPCONFIG(MP_MBOXDIR);
   if( strutil_isempty(str) )
   {
      str = READ_APPCONFIG(MP_USERDIR);
      profile->writeEntry(MP_MBOXDIR, str);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_HOSTNAME)) )
   {
      wxGetFullHostName(buffer,bufsize);
      profile->writeEntry(MP_HOSTNAME, buffer);
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

#if 0

   mApplication->GetProfile()->writeEntry(MP_OUTGOINGFOLDER, _("Sent Mail"));
   mApplication->GetProfile()->writeEntry(MP_NEWMAIL_FOLDER, _("New Mail"));

   (void)VerifyInbox();

#if 0
#if defined ( USE_PYTHON )
// run a python script to set up things
   PyH_RunMScript(MSCRIPT_USER_SETUP);
#else
#   pragma warning "Missing functionality without Python!"
#endif
#endif
#endif

}

extern bool
CheckConfiguration(void)
{
   ProfileBase *profile = mApplication->GetProfile();

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

         return FALSE;
      }
   }

   if ( READ_APPCONFIG(MP_FIRSTRUN) != 0 )
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
      if ( ! Upgrade(version) )
         return false;
      // write the new version
      profile->writeEntry(MP_VERSION, M_VERSION);
   }

   if ( !VerifyInbox() )
      wxLogDebug("Evil user corrupted his profile settings - restored.");

   return true;
}
