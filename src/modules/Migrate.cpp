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

#include "ASMailFolder.h"
#include "ListReceiver.h"

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

   // the number of folders on the source server (-1 if unknown)
   int countFolders;

   MigrateData()
   {
      toIMAP = true;

      countFolders = -1;
   }
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

   // has anything changed?
   bool IsDirty() const { return m_isDirty; }

   // update the state of the wizards "Forward" button
   void UpdateForwardBtnUI();

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

   // the dirty flag, true if anything has changed
   bool m_isDirty;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// LocalPanel: a panel containing the controls for MigrateLocal data
// ----------------------------------------------------------------------------

class LocalPanel : public wxEnhancedPanel
{
public:
   LocalPanel(wxWindow *panel, MigrateLocal *localData);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // returns the name of the mailbox format
   static const wxChar *GetFormatName(FileMailboxFormat format);

private:
   wxTextCtrl *m_textDir;
   wxChoice *m_choiceFormat;

   MigrateLocal *m_localData;
};

// ----------------------------------------------------------------------------
// MigrateWizard: the wizard used to interact with the user
// ----------------------------------------------------------------------------

class MigrateWizard : public wxWizard,
                      public ListEventReceiver
{
public:
   // all our pages
   enum Page
   {
      Page_Source,
      Page_CantAccessSource,
      Page_WarnEmptySource,
      Page_Dst,
      Page_Confirm,
      Page_Progress,
      Page_Max
   };

   MigrateWizard(wxWindow *parent);

   // do run the wizard
   bool Run();

   // access the migration data
   MigrateData& Data() { return m_migrateData; }

   // return the next/previous page for the given one
   wxWizardPage *GetNextPage(Page page);
   wxWizardPage *GetPrevPage(Page page);

   // determine ourselves whether there is a prev/next page as we don't want to
   // create the pages just for this (this can usually be postponed until
   // later or even not done at all in some cases)
   virtual bool HasNextPage(wxWizardPage *page);
   virtual bool HasPrevPage(wxWizardPage *page);

   // implement ListEventReceiver methods
   virtual void OnListFolder(const String& path, char delim, long flags);
   virtual void OnNoMoreFolders();

private:
   // return, creating if necessary, the given page
   wxWizardPage *GetPage(Page page);

   MigrateData m_migrateData;

   // the pages (created on demand by GetPage())
   wxWizardPage *m_pages[Page_Max];

   // false while we're waiting for the events from ListFolders()
   bool m_doneWithList;
};

// ----------------------------------------------------------------------------
// MigrateWizardPage: base class for all MigrateWizard pages
// ----------------------------------------------------------------------------

class MigrateWizardPage : public wxWizardPage
{
public:
   MigrateWizardPage(MigrateWizard *wizard, MigrateWizard::Page id)
      : wxWizardPage(wizard)
   {
      m_wizard = wizard;
      m_id = id;
   }

   // common accessors
   MigrateWizard *GetWizard() const { return m_wizard; }
   MigrateData& Data() { return GetWizard()->Data(); }

   MigrateWizard::Page GetId() const { return m_id; }

   // let the wizard decide the order in which the pages are shown
   virtual wxWizardPage *GetPrev() const
      { return GetWizard()->GetPrevPage(GetId()); }
   virtual wxWizardPage *GetNext() const
      { return GetWizard()->GetNextPage(GetId()); }

private:
   MigrateWizard *m_wizard;
   MigrateWizard::Page m_id;
};

// ----------------------------------------------------------------------------
// MigrateWizardMsgOnlyPage: base class for pages without any controls
// ----------------------------------------------------------------------------

class MigrateWizardMsgOnlyPage : public MigrateWizardPage
{
public:
   MigrateWizardMsgOnlyPage(MigrateWizard *wizard,
                            MigrateWizard::Page id,
                            const String& msg)
      : MigrateWizardPage(wizard, id)
   {
      new wxStaticText(this, -1, msg);
   }
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

// ----------------------------------------------------------------------------
// MigrateWizardCantAccessPage: page shown if we failed to connect to IMAP src
// ----------------------------------------------------------------------------

class MigrateWizardCantAccessPage : public MigrateWizardMsgOnlyPage
{
public:
   MigrateWizardCantAccessPage(MigrateWizard *parent)
      : MigrateWizardMsgOnlyPage
        (
         parent,
         MigrateWizard::Page_CantAccessSource,
         String::Format
         (
            _("Failed to access the IMAP server %s,\n"
              "please return to the previous page and\n"
              "check its parameters."),
            parent->Data().source.server.c_str()
         )
        )
   {
   }
};

// ----------------------------------------------------------------------------
// MigrateWizardNothingToDoPage: shown if there are no folders to copy
// ----------------------------------------------------------------------------

class MigrateWizardNothingToDoPage : public MigrateWizardMsgOnlyPage
{
public:
   MigrateWizardNothingToDoPage(MigrateWizard *parent)
      : MigrateWizardMsgOnlyPage
        (
         parent,
         MigrateWizard::Page_WarnEmptySource,
         String::Format
         (
            _("There doesn't seem to be any folders on\n"
              "the IMAP server %s!\n"
              "\n"
              "You may want to change its parameters\n"
              "by return to the previous pages."),
            parent->Data().source.server.c_str()
         )
        )
   {
   }
};

// ----------------------------------------------------------------------------
// MigrateWizardDstPage: second page, choose the destination
// ----------------------------------------------------------------------------

class MigrateWizardDstPage : public MigrateWizardPage
{
public:
   MigrateWizardDstPage(MigrateWizard *parent);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   void OnCheckBox(wxCommandEvent& event);

   // enable/disable the panels according to whether we use IMAP or local file
   void UseIMAP(bool useIMAP);

private:
   wxCheckBox *m_chkUseIMAP;

   IMAPServerPanel *m_panelIMAP;
   LocalPanel *m_panelLocal;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// MigrateWizardConfirmPage: third page, confirm the migration parameters
// ----------------------------------------------------------------------------

class MigrateWizardConfirmPage : public MigrateWizardMsgOnlyPage
{
public:
   MigrateWizardConfirmPage(MigrateWizard *parent);

private:
   String BuildMsg(MigrateWizard *parent) const;
};

// ----------------------------------------------------------------------------
// MigrateWizardProgressPage: fourth and final page, show migration progress
// ----------------------------------------------------------------------------

class MigrateWizardProgressPage : public MigrateWizardPage
{
public:
   MigrateWizardProgressPage(MigrateWizard *parent);
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
   m_isDirty = false;

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

   UpdateForwardBtnUI();

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

      if ( (unsigned long)m_imapData->port != l )
      {
         // cast is safe because of the check above
         m_imapData->port = (int)l;

         m_isDirty = true;
      }

      // remove the port pat from the host string
      server.erase(posColon);
   }

   if ( m_imapData->server != server )
   {
      m_isDirty = true;
      m_imapData->server = server;
   }

   String value = m_textRoot->GetValue();
   if ( m_imapData->root != value )
   {
      m_isDirty = true;
      m_imapData->root = value;
   }

   value = m_textLogin->GetValue();
   if ( m_imapData->username != value )
   {
      m_isDirty = true;
      m_imapData->username = value;
   }

   value = m_textPass->GetValue();
   if ( m_imapData->password != value )
   {
      m_isDirty = true;
      m_imapData->password = value;
   }

#ifdef USE_SSL
   bool useSSL = m_chkSSL->GetValue();
   if ( m_imapData->useSSL != useSSL )
   {
      m_isDirty = true;
      m_imapData->useSSL = useSSL;
   }
#endif // USE_SSL

   return true;
}

void IMAPServerPanel::OnText(wxCommandEvent& event)
{
   MFolder_obj folder = m_btnFolder->GetFolder();
   if ( folder != m_folder )
   {
      if ( m_folder )
      {
         m_folder->DecRef();
      }

      m_folder = folder;

      if ( m_folder )
      {
         // check that a valid folder was chosen
         if ( m_folder->GetType() != MF_IMAP )
         {
            wxLogError(_("You can only migrate from or to an IMAP server."));

            // it was already changed by the browse button
            m_textServer->SetValue(_T(""));

            m_folder = NULL;

            return;
         }

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

   // check if the server name became empty
   UpdateForwardBtnUI();

   event.Skip();
}

void IMAPServerPanel::UpdateForwardBtnUI()
{
   wxWindow *wizard = GetParent()->GetParent();
   CHECK_RET( wizard, _T("IMAPServerPanel outside a wizard?") );

   wxWindow *btnForward = wizard->FindWindow(wxID_FORWARD);
   CHECK_RET( btnForward, _T("no \"Forward\" button?") );

   btnForward->Enable( !m_textServer->GetValue().empty() );
}

// ----------------------------------------------------------------------------
// LocalPanel
// ----------------------------------------------------------------------------

LocalPanel::LocalPanel(wxWindow *panel, MigrateLocal *localData)
          : wxEnhancedPanel(panel)
{
   m_localData = localData;

   // create the controls
   enum Fields
   {
      Label_Dir,
      Label_Format,
      Label_Max
   };

   wxArrayString labels;
   labels.Add(_("&Directory for files:"));
   labels.Add(_("Mailbox &format"));

   // check that we didn't forget to update something
   ASSERT_MSG( labels.GetCount() == Label_Max, _T("label count mismatch") );

   const long widthMax = GetMaxLabelWidth(labels, this);

   m_textDir = CreateDirEntry(labels[Label_Dir], widthMax, NULL);

   String formats = labels[Label_Format];
   for ( int fmt = 0; fmt < FileMbox_Max + 1 /* for MH */; fmt++ )
   {
      formats << _T(':') << GetFormatName((FileMailboxFormat)fmt);
   }

   m_choiceFormat = CreateChoice(formats, widthMax, m_textDir);
}

bool LocalPanel::TransferDataToWindow()
{
   m_textDir->SetValue(m_localData->root);
   m_choiceFormat->SetSelection(m_localData->format);

   return true;
}

bool LocalPanel::TransferDataFromWindow()
{
   m_localData->root = m_textDir->GetValue();
   m_localData->format = (FileMailboxFormat)m_choiceFormat->GetSelection();

   return true;
}

/* static */
const wxChar *LocalPanel::GetFormatName(FileMailboxFormat format)
{
   // this array must be in sync with FileMailboxFormat
   static const wxChar *formatNames[] =
   {
      gettext_noop("MBX (UW-IMAPd)"),
      gettext_noop("MBOX (traditional Unix)"),
      gettext_noop("MMDF (SCO Unix)"),
      gettext_noop("Tenex (historical)"),
      gettext_noop("MH (one file per message)"),
   };

   ASSERT_MSG( (size_t)format < WXSIZEOF(formatNames),
               _T("mailbox format out of range") );

   return _(formatNames[format]);
}

// ----------------------------------------------------------------------------
// MigrateWizardSourcePage
// ----------------------------------------------------------------------------

MigrateWizardSourcePage::MigrateWizardSourcePage(MigrateWizard *parent)
                       : MigrateWizardPage(parent, MigrateWizard::Page_Source)
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
   if ( !m_panel->TransferDataFromWindow() )
      return false;

   if ( m_panel->IsDirty() )
   {
      // reset the number of folders as the server has changed
      Data().countFolders = -1;
   }

   return true;
}

// ----------------------------------------------------------------------------
// MigrateWizardDstPage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MigrateWizardDstPage, MigrateWizardPage)
   EVT_CHECKBOX(-1, MigrateWizardDstPage::OnCheckBox)
END_EVENT_TABLE()

MigrateWizardDstPage::MigrateWizardDstPage(MigrateWizard *parent)
                    : MigrateWizardPage(parent, MigrateWizard::Page_Dst)
{
   wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
   sizer->Add
          (
            new wxStaticText
                (
                  this,
                  -1,
                  _("Please choose where to copy mail now:")
                ),
            0,
            wxALL | wxEXPAND,
            LAYOUT_X_MARGIN
          );

   m_chkUseIMAP = new wxCheckBox(this, -1, _("to another &IMAP server:"));
   sizer->Add(m_chkUseIMAP, 0, wxALL, LAYOUT_X_MARGIN);

   m_panelIMAP = new IMAPServerPanel(this, &Data().dstIMAP);
   sizer->Add(m_panelIMAP, 1, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   sizer->Add(new wxStaticText(this, -1, _("or to a &local file:")));

   m_panelLocal = new LocalPanel(this, &Data().dstLocal);
   sizer->Add(m_panelLocal, 1, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   SetSizer(sizer);
}

void MigrateWizardDstPage::UseIMAP(bool useIMAP)
{
   m_panelIMAP->Enable(useIMAP);
   m_panelLocal->Enable(!useIMAP);
}

void MigrateWizardDstPage::OnCheckBox(wxCommandEvent& event)
{
   if ( event.GetEventObject() == m_chkUseIMAP )
   {
      bool useIMAP = event.IsChecked();
      Data().toIMAP = useIMAP;

      UseIMAP(useIMAP);

      if ( useIMAP )
      {
         // button should be disabled if the server is not entered
         m_panelIMAP->UpdateForwardBtnUI();
      }
      else // local dst
      {
         // button is always enabled
         wxWindow *btnForward = GetParent()->FindWindow(wxID_FORWARD);
         CHECK_RET( btnForward, _T("no \"Forward\" button?") );

         btnForward->Enable();
      }
   }

   event.Skip();
}

bool MigrateWizardDstPage::TransferDataToWindow()
{
   bool useIMAP = Data().toIMAP;

   m_chkUseIMAP->SetValue(useIMAP);

   UseIMAP(useIMAP);

   return (useIMAP ? m_panelIMAP : m_panelLocal)->TransferDataToWindow();
}

bool MigrateWizardDstPage::TransferDataFromWindow()
{
   return (Data().toIMAP ? m_panelIMAP : m_panelLocal)->TransferDataFromWindow();
}

// ----------------------------------------------------------------------------
// MigrateWizardConfirmPage
// ----------------------------------------------------------------------------

MigrateWizardConfirmPage::MigrateWizardConfirmPage(MigrateWizard *parent)
                        : MigrateWizardMsgOnlyPage
                          (
                           parent,
                           MigrateWizard::Page_Confirm,
                           BuildMsg(parent)
                          )
{
}

String
MigrateWizardConfirmPage::BuildMsg(MigrateWizard *parent) const
{
   const MigrateData& data = parent->Data();

   String msg;

   msg.Printf(_("About to start copying %d folders from %s\n"),
              data.countFolders, data.source.server.c_str());
   if ( data.toIMAP )
   {
      msg += String::Format
             (
               _("to the IMAP server %s"),
               data.dstIMAP.server.c_str()
             );

      const String& root = data.dstIMAP.root;
      if ( !root.empty() )
         msg += String::Format(_(" (under %s)"), root.c_str());

      msg += _T('\n');
   }
   else
   {
      msg += String::Format
             (
               _("to the files in %s format under\n"
                 "the directory \"%s\""),
               LocalPanel::GetFormatName(data.dstLocal.format),
               data.dstLocal.root.c_str()
             );
   }

   msg += _("\n\nPlease press \"Next\" to conitnue, \"Back\" to\n"
            "modify the migration parameters\n"
            "or \"Cancel\" to abort the operation.");

   return msg;
}

// ----------------------------------------------------------------------------
// MigrateWizardProgressPage
// ----------------------------------------------------------------------------

MigrateWizardProgressPage::MigrateWizardProgressPage(MigrateWizard *parent)
                         : MigrateWizardPage
                           (
                              parent,
                              MigrateWizard::Page_Progress
                           )
{
}

// ----------------------------------------------------------------------------
// MigrateWizard
// ----------------------------------------------------------------------------

MigrateWizard::MigrateWizard(wxWindow *parent)
             : wxWizard(parent, -1, _("Mahogany Migration Tool")) // TODO: bmp
{
   for ( size_t n = 0; n < WXSIZEOF(m_pages); n++ )
   {
      m_pages[n] = NULL;
   }
}

bool MigrateWizard::Run()
{
   return RunWizard(GetPage(Page_Source));
}

wxWizardPage *MigrateWizard::GetPage(Page page)
{
   wxWizardPage *p;
   switch ( page )
   {
      case Page_Source:
         p = new MigrateWizardSourcePage(this);
         break;

      case Page_CantAccessSource:
         p = new MigrateWizardCantAccessPage(this);
         break;

      case Page_WarnEmptySource:
         p = new MigrateWizardNothingToDoPage(this);
         break;

      case Page_Dst:
         p = new MigrateWizardDstPage(this);
         break;

      case Page_Confirm:
         p = new MigrateWizardConfirmPage(this);
         break;

      case Page_Progress:
         p = new MigrateWizardProgressPage(this);
         break;

      default:
         FAIL_MSG( _T("unknown page in MigrateWizard") );
         return NULL;
   }

   m_pages[page] = p;
   return p;
}

wxWizardPage *MigrateWizard::GetNextPage(Page page)
{
   static const Page nextPages[Page_Max] =
   {
      /* Page_Source */             Page_Max, // not used!
      /* Page_CantAccessSource */   Page_Max,
      /* Page_WarnEmptySource */    Page_Dst,
      /* Page_Dst */                Page_Confirm,
      /* Page_Confirm */            Page_Progress,
      /* Page_Progress */           Page_Max,
   };

   // first deal with dynamic page order cases
   Page pageNext;
   if ( page == Page_Source )
   {
      // try to connect to the source server to get the number of folders to
      // copy
      if ( Data().countFolders == -1 )
      {
         MProgressInfo progress(this, _("Accessing IMAP server..."));

         MFolder_obj folderSrc(MFolder::CreateTemp(_T(""), MF_IMAP));

         const MigrateImapServer& imapData = Data().source;
         folderSrc->SetServer(imapData.server);
         folderSrc->SetPath(imapData.root);
         folderSrc->SetAuthInfo(imapData.username, imapData.password);

#ifdef USE_SSL
         if ( imapData.useSSL )
         {
            // we don't have a separate checkbox for accepting unsinged SSL
            // certs because of lack of space in the dialog but we do accept
            // them here because always accepting them is better than never
            // doing it
            folderSrc->AddFlags(MF_FLAGS_SSLAUTH | MF_FLAGS_SSLUNSIGNED);
         }
#endif // USE_SSL

         ASMailFolder *asmf = ASMailFolder::OpenFolder
                              (
                               folderSrc,
                               MailFolder::HalfOpen
                              );
         if ( asmf )
         {
            Data().countFolders = 0;
            m_doneWithList = false;

            // the cast below is needed as ListEventReceiver compares the user
            // data in the results it gets with "this" and its this pointer is
            // different from our "this" as we use multiple inheritance
            Ticket t = asmf->ListFolders(_T(""), false, _T(""),
                                         (ListEventReceiver *)this);
            if ( t != ILLEGAL_TICKET )
            {
               // process the events from ListFolders
               do
               {
                  MEventManager::DispatchPending();
               }
               while ( !m_doneWithList );
            }

            asmf->DecRef();
         }
      }

      switch ( Data().countFolders )
      {
         case -1:
            pageNext = Page_CantAccessSource;
            break;

         case 0:
            pageNext = Page_WarnEmptySource;
            break;

         default:
            pageNext = Page_Dst;
      }
   }
   else // static case
   {
      pageNext = nextPages[page];
   }

   return pageNext == Page_Max ? NULL : GetPage(pageNext);
}

wxWizardPage *MigrateWizard::GetPrevPage(Page page)
{
   static const Page prevPages[Page_Max] =
   {
      /* Page_Source */             Page_Max,
      /* Page_CantAccessSource */   Page_Source,
      /* Page_WarnEmptySource */    Page_Source,
      /* Page_Dst */                Page_Source,
      /* Page_Confirm */            Page_Dst,
      /* Page_Progress */           Page_Confirm,
   };

   Page pagePrev = prevPages[page];
   return pagePrev == Page_Max ? NULL : GetPage(pagePrev);
}

bool MigrateWizard::HasNextPage(wxWizardPage *page)
{
   switch ( ((MigrateWizardPage *)page)->GetId() )
   {
      case Page_Source:
      case Page_WarnEmptySource:
      case Page_Dst:
      case Page_Confirm:
         return true;

      default:
         FAIL_MSG( _T("unknown page in MigrateWizard") );

      case Page_CantAccessSource:
      case Page_Progress:
         return false;
   }
}

bool MigrateWizard::HasPrevPage(wxWizardPage *page)
{
   // all pages have a previous one except the initial one
   return ((MigrateWizardPage *)page)->GetId() != Page_Source;
}

void
MigrateWizard::OnListFolder(const String& path, char delim, long flags)
{
   Data().countFolders++;
}

void
MigrateWizard::OnNoMoreFolders()
{
   m_doneWithList = true;
}

