//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/Migrate.cpp: the "Migrate tool" Mahogany plugin
// Purpose:     implements functionality of the "Migrate" plugin
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.10.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "MFPool.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "gui/wxMainFrame.h"
   #include "gui/wxDialogLayout.h"
   #include "gui/wxBrowseButton.h"

   #include <wx/sizer.h>

   #include <wx/checkbox.h>
   #include <wx/stattext.h>
   #include <wx/textctrl.h>
#endif // USE_PCH

#include "MModule.h"

#include <wx/wizard.h>

// ----------------------------------------------------------------------------
// MigrateImapServer: IMAP server parameters
// ----------------------------------------------------------------------------

struct MigrateImapServer
{
   // the server name
   String server;

   // the port or -1 for default
   int port;

   // the starting folder or empty if root
   String root;

   // the username and password (anonymous access if empty)
   String username,
          password;

#ifdef USE_SSL
   // use SSL to access this server?
   bool useSSL;
#endif // USE_SSL

   MigrateImapServer()
   {
      port = -1;

#ifdef USE_SSL
      useSSL = false;
#endif // USE_SSL
   }
};

// ----------------------------------------------------------------------------
// MigrateLocal: local storage parameters
// ----------------------------------------------------------------------------

struct MigrateLocal
{
   // the directory in which to create the files
   String root;

   // the format of the folders to create
   FileMailboxFormat format;
};

// ----------------------------------------------------------------------------
// MigrateData: the parameters of migration procedure
// ----------------------------------------------------------------------------

struct MigrateData
{
   // the server we're copying the mail from
   MigrateImapServer source;

   // if true, use dstIMAP below, otherwise use dstLocal
   bool toIMAP;

   // the destination server
   MigrateImapServer dstIMAP;

   // or the local file(s)
   MigrateLocal dstLocal;
};

// ----------------------------------------------------------------------------
// MigrateModule: implementation of MModule by this plugin
// ----------------------------------------------------------------------------

class MigrateModule : public MModule
{
public:
   MigrateModule(MInterface *minterface);

   virtual int Entry(int arg, ...);

private:
   // add a menu entry for us to the main frame menu, return true if ok
   bool RegisterWithMainFrame();

   // the main worker function
   bool DoMigrate();

   MMODULE_DEFINE();
};

// ============================================================================
// GUI classes
// ============================================================================

// ----------------------------------------------------------------------------
// IMAPServerPanel: a panel containing the controls for MigrateImapServer data
// ----------------------------------------------------------------------------

class IMAPServerPanel : public wxEnhancedPanel
{
public:
   IMAPServerPanel(wxWindow *parent, MigrateImapServer *imapData);
   virtual ~IMAPServerPanel();

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   void OnText(wxCommandEvent& event);

private:
   // the GUI controls
   wxTextCtrl *m_textServer,
              *m_textRoot,
              *m_textLogin,
              *m_textPass;

   wxFolderBrowseButton *m_btnFolder;

   wxCheckBox *m_chkSSL;

   // the data we edit
   MigrateImapServer *m_imapData;

   // the current folder selected by the user using the browse button or NULL
   MFolder *m_folder;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// MigrateWizard: the wizard used to interact with the user
// ----------------------------------------------------------------------------

class MigrateWizard : public wxWizard
{
public:
   MigrateWizard(wxWindow *parent);

   bool Run();

   MigrateData& Data() { return m_migrateData; }

private:
   MigrateData m_migrateData;
};

// ----------------------------------------------------------------------------
// MigrateWizardPage: base class for all MigrateWizard pages
// ----------------------------------------------------------------------------

class MigrateWizardPage : public wxWizardPageSimple
{
public:
   MigrateWizardPage(MigrateWizard *wizard)
      : wxWizardPageSimple(wizard)
   {
      m_wizard = wizard;
   }

   MigrateWizard *GetWizard() const { return m_wizard; }
   MigrateData& Data() { return GetWizard()->Data(); }

private:
   MigrateWizard *m_wizard;
};

// ----------------------------------------------------------------------------
// MigrateWizardSourcePage: the first page of the wizard, choose the source
// ----------------------------------------------------------------------------

class MigrateWizardSourcePage : public MigrateWizardPage
{
public:
   MigrateWizardSourcePage(MigrateWizard *parent);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   IMAPServerPanel *m_panel;
};

// ============================================================================
// MigrateModule implementation
// ============================================================================

MMODULE_BEGIN_IMPLEMENT(MigrateModule,
                        "Migrate",
                        "Migrate",
                        "Migration tool",
                        "1.00")
   MMODULE_PROP("description",
                _("Allows to migrate entire IMAP server mail hierarchy to "
                  "another IMAP server or local storage."))
   MMODULE_PROP("author", "Vadim Zeitlin <vadim@wxwindows.org>")
MMODULE_END_IMPLEMENT(MigrateModule)

/* static */
MModule *
MigrateModule::Init(int verMajor, int verMinor, int verRelease,
                    MInterface *minterface, int *errorCode)
{
   if ( !MMODULE_SAME_VERSION(verMajor, verMinor, verRelease))
   {
      if( errorCode )
         *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }

   return new MigrateModule(minterface);
}

MigrateModule::MigrateModule(MInterface *minterface)
{
   SetMInterface(minterface);
}

int
MigrateModule::Entry(int arg, ...)
{
   switch( arg )
   {
      case MMOD_FUNC_INIT:
         return RegisterWithMainFrame() ? 0 : -1;

      // Menu event
      case MMOD_FUNC_MENUEVENT:
      {
         va_list ap;
         va_start(ap, arg);
         int id = va_arg(ap, int);
         va_end(ap);

         if ( id == WXMENU_MODULES_MIGRATE_DO )
            return DoMigrate() ? 0 : -1;

         FAIL_MSG( _T("unexpected menu event in migrate module") );
         // fall through
      }

      default:
         return 0;
   }
}

bool
MigrateModule::RegisterWithMainFrame()
{
   MAppBase *mapp = m_MInterface->GetMApplication();
   CHECK( mapp, false, _T("can't init migration module -- no app object") );

   wxMFrame *mframe = mapp->TopLevelFrame();
   CHECK( mframe, false, _T("can't init migration module -- no main frame") );

   ((wxMainFrame *)mframe)->AddModulesMenu
                            (
                             _("&Migrate..."),
                             _("Migrate IMAP server contents"),
                             WXMENU_MODULES_MIGRATE_DO
                            );

   return true;
}

bool
MigrateModule::DoMigrate()
{
   MAppBase *mapp = m_MInterface->GetMApplication();
   wxMFrame *mframe = mapp ? mapp->TopLevelFrame() : NULL;

   MigrateWizard *wizard = new MigrateWizard(mframe);

   bool done = wizard->Run();

   wizard->Destroy();

   return done ? 0 : 1;
}

// ============================================================================
// GUI classes implementation
// ============================================================================

// ----------------------------------------------------------------------------
// IMAPServerPanel
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(IMAPServerPanel, wxEnhancedPanel)
   EVT_TEXT(-1, IMAPServerPanel::OnText)
END_EVENT_TABLE()

IMAPServerPanel::IMAPServerPanel(wxWindow *parent, MigrateImapServer *imapData)
               : wxEnhancedPanel(parent)
{
   m_imapData = imapData;
   m_folder = NULL;

   // the controls data
   enum Fields
   {
      Label_Server,
      Label_Root,
      Label_Login,
      Label_Password,
#ifdef USE_SSL
      Label_SSL,
#endif // USE_SSL
      Label_Max
   };

   wxArrayString labels;
   labels.Add(_("&Server:"));
   labels.Add(_("&Root folder:"));
   labels.Add(_("&User name:"));
   labels.Add(_("&Password:"));
#ifdef USE_SSL
   labels.Add(_("Use SS&L"));
#endif // USE_SSL

   // check that we didn't forget to update something
   ASSERT_MSG( labels.GetCount() == Label_Max, _T("label count mismatch") );

   const long widthMax = GetMaxLabelWidth(labels, this);

   // create the controls: server and the root folder to use on it
   m_textServer = CreateFolderEntry(labels[Label_Server], widthMax, NULL,
                                    &m_btnFolder);
   m_textRoot = CreateTextWithLabel(labels[Label_Root], widthMax, m_textServer);

   // the authentication parameters
   m_textLogin = CreateTextWithLabel(labels[Label_Login], widthMax, m_textRoot);
   m_textPass = CreateTextWithLabel(labels[Label_Password], widthMax,
                                    m_textLogin, 0, wxTE_PASSWORD);

#ifdef USE_SSL
   m_chkSSL = CreateCheckBox(labels[Label_SSL], widthMax, m_textPass);
#endif // USE_SSL
}

IMAPServerPanel::~IMAPServerPanel()
{
   if ( m_folder )
      m_folder->DecRef();
}

bool IMAPServerPanel::TransferDataToWindow()
{
   CHECK( m_imapData, false, _T("no data in IMAPServerPanel") );

   String server = m_imapData->server;
   if ( m_imapData->port != -1 )
   {
      server += String::Format(_T(":%d"), m_imapData->port);
   }

   m_textServer->SetValue(server);

   m_textRoot->SetValue(m_imapData->root);
   m_textLogin->SetValue(m_imapData->username);
   m_textPass->SetValue(m_imapData->password);

#ifdef USE_SSL
   m_chkSSL->SetValue(m_imapData->useSSL);
#endif // USE_SSL

   return true;
}

bool IMAPServerPanel::TransferDataFromWindow()
{
   // extract the host name and the port from the provided string
   String server = m_textServer->GetValue();
   const size_t posColon = server.find(_T(':'));
   if ( posColon != String::npos )
   {
      const String port = server.substr(posColon);

      unsigned long l;
      if ( !port.ToULong(&l) || l > INT_MAX )
      {
         wxLogError(_("Invalid port specification: %s"), port.c_str());

         return false;
      }

      // cast is safe because of the check above
      m_imapData->port = (int)l;

      // remove the port pat from the host string
      server.erase(posColon);
   }

   m_imapData->server = server;
   m_imapData->root = m_textRoot->GetValue();
   m_imapData->username = m_textLogin->GetValue();
   m_imapData->password = m_textPass->GetValue();

#ifdef USE_SSL
   m_imapData->useSSL = m_chkSSL->GetValue();
#endif // USE_SSL

   return true;
}

void IMAPServerPanel::OnText(wxCommandEvent& event)
{
   MFolder_obj folder = m_btnFolder->GetFolder();
   if ( folder != m_folder )
   {
      // check that a valid folder was chosen
      if ( folder && folder->GetType() != MF_IMAP )
      {
         wxLogError(_("You can only migrate from IMAP folder."));

         // it was already changed by the browse button
         m_textServer->SetValue(_T(""));

         return;
      }

      if ( m_folder )
      {
         m_folder->DecRef();
      }

      m_folder = folder;

      if ( m_folder )
      {
         // to compensate for implicit DecRef() by MFolder_obj dtor
         m_folder->IncRef();

         // update the controls
         m_textServer->SetValue(m_folder->GetServer());
         m_textRoot->SetValue(m_folder->GetPath());
         m_textLogin->SetValue(m_folder->GetLogin());
         m_textPass->SetValue(m_folder->GetPassword());

#ifdef USE_SSL
         m_chkSSL->SetValue(m_folder->GetFlags() & MF_FLAGS_SSLAUTH);        
#endif // USE_SSL
      }
   }

   event.Skip();
}

// ----------------------------------------------------------------------------
// MigrateWizardSourcePage
// ----------------------------------------------------------------------------

MigrateWizardSourcePage::MigrateWizardSourcePage(MigrateWizard *parent)
                       : MigrateWizardPage(parent)
{
   wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
   sizer->Add
          (
            new wxStaticText
                (
                  this,
                  -1,
                  _("Welcome to the migration wizard!"
                    "\n"
                    "It allows you to copy the entire contents of\n"
                    "an IMAP server either to another server or\n"
                    "to local files.\n")
                ),
            0,
            wxALL | wxEXPAND,
            LAYOUT_X_MARGIN
          );

   m_panel = new IMAPServerPanel(this, &Data().source);
   sizer->Add(m_panel, 1, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   SetSizer(sizer);
}

bool MigrateWizardSourcePage::TransferDataToWindow()
{
   return m_panel->TransferDataToWindow();
}

bool MigrateWizardSourcePage::TransferDataFromWindow()
{
   return m_panel->TransferDataFromWindow();
}

// ----------------------------------------------------------------------------
// MigrateWizard
// ----------------------------------------------------------------------------

MigrateWizard::MigrateWizard(wxWindow *parent)
             : wxWizard(parent, -1, _("Mahogany Migration Tool")) // TODO: bmp
{
}

bool MigrateWizard::Run()
{
   return RunWizard(new MigrateWizardSourcePage(this));
}

