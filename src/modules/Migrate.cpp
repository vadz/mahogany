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

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "gui/wxIconManager.h"

   #include <wx/app.h>        // for wxPostEvent()
   // it includes windows.h which defines SendMessage under Windows
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__WINE__)
   #undef SendMessage
#endif

   #include <wx/sizer.h>

   #include <wx/gauge.h>
   #include <wx/radiobut.h>
   #include <wx/stattext.h>     // for wxStaticText

   #include <wx/msgdlg.h>
#endif // USE_PCH

#include "MModule.h"
#include "MInterface.h"

#include "ListReceiver.h"

#include "UIdArray.h"
#include "HeaderInfo.h"

#include "ASMailFolder.h"               // for ATT_NOINFERIORS

#include "gui/wxDialogLayout.h"
#include "gui/wxMainFrame.h"
#include "gui/wxBrowseButton.h"

#include <wx/wizard.h>

// if we're still using old headers
#ifndef wxRB_SINGLE
   #define wxRB_SINGLE 0
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the suffix we add to the folder containing the messages if we can't have a
// folder which has both subfolders and messages
#define MESSAGES_SUFFIX ".messages"

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

   // the delimiter of the folder names (NUL initially meaning "unknown")
   char delimiter;

#ifdef USE_SSL
   // use SSL to access this server?
   bool useSSL;
#endif // USE_SSL

   MigrateImapServer()
   {
      port = -1;

      delimiter = '\0';

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

   MigrateLocal()
   {
      // use MBOX by default
      format = FileMbox_MBOX;
   }
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

   // the delimiter of the folder names (NUL initially meaning "unknown")
   char delimiterDst;

   // the destination server
   MigrateImapServer dstIMAP;

   // or the local file(s)
   MigrateLocal dstLocal;

   // the number of folders on the source server (-1 if unknown)
   int countFolders;

   // the (full) names of the folders to migrate (empty if countFolders == -1)
   wxArrayString folderNames;

   // the flags (ASMailFolder::ATT_XXX bit masks combination)
   wxArrayInt folderFlags;

   MigrateData()
   {
      toIMAP = true;

      delimiterDst = '\0';

      countFolders = -1;
   }

   // add a new folder to folderNames/folderFlags arrays
   void AddFolder(const String& path, char delim, long flags);

   // must be called at the end of folder enumeration to set the
   // ATT_NOINFERIORS flags for the folders correctly
   void FixFolderFlags();
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
   DECLARE_NO_COPY_CLASS(IMAPServerPanel)
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

   DECLARE_NO_COPY_CLASS(LocalPanel)
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

   // flags for EnableButtons()
   enum
   {
      Btn_Back   = 1 << 1,
      Btn_Next   = 1 << 2,
      Btn_Cancel = 1 << 3,
      Btn_All    = Btn_Back | Btn_Next | Btn_Cancel
   };

   MigrateWizard(wxWindow *parent);

   // do run the wizard
   bool Run();

   // access the migration data
   MigrateData& Data() { return m_migrateData; }
   const MigrateData& Data() const { return m_migrateData; }

   // enable/disable all or specified wizard buttons
   void EnableButtons(int buttons, bool enable);

   // return the next/previous page for the given one
   wxWizardPage *GetNextPage(Page page);
   wxWizardPage *GetPrevPage(Page page);

   // determine ourselves whether there is a prev/next page as we don't want to
   // create the pages just for this (this can usually be postponed until
   // later or even not done at all in some cases)
   virtual bool HasNextPage(wxWizardPage *page);
   virtual bool HasPrevPage(wxWizardPage *page);

   // implement ListEventReceiver methods
   virtual void OnListFolder(const String& path, wxChar delim, long flags);
   virtual void OnNoMoreFolders();

private:
   // return, creating if necessary, the given page
   wxWizardPage *GetPage(Page page);

   MigrateData m_migrateData;

   // the pages (created on demand by GetPage())
   wxWizardPage *m_pages[Page_Max];

   // false while we're waiting for the events from ListFolders()
   bool m_doneWithList;

   DECLARE_NO_COPY_CLASS(MigrateWizard)
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
   const MigrateData& Data() const { return GetWizard()->Data(); }

   MigrateWizard::Page GetId() const { return m_id; }

   void EnableButtons(int buttons, bool enable)
      { m_wizard->EnableButtons(buttons, enable); }

   // let the wizard decide the order in which the pages are shown
   virtual wxWizardPage *GetPrev() const
      { return GetWizard()->GetPrevPage(GetId()); }
   virtual wxWizardPage *GetNext() const
      { return GetWizard()->GetNextPage(GetId()); }

private:
   MigrateWizard *m_wizard;
   MigrateWizard::Page m_id;

   DECLARE_NO_COPY_CLASS(MigrateWizardPage)
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


   DECLARE_NO_COPY_CLASS(MigrateWizardMsgOnlyPage)
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

   DECLARE_NO_COPY_CLASS(MigrateWizardSourcePage)
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

private:
   DECLARE_NO_COPY_CLASS(MigrateWizardCantAccessPage)
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
              "You may want to return to the previous page\n"
              "and change the server parameters there."),
            parent->Data().source.server.c_str()
         )
        )
   {
   }

private:
   DECLARE_NO_COPY_CLASS(MigrateWizardNothingToDoPage)
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
   void OnRadioButton(wxCommandEvent& event);

   // return the panel we're currently using (IMAP or local)
   wxEnhancedPanel *GetPanelToUse() const
   {
      return Data().toIMAP ? (wxEnhancedPanel *)m_panelIMAP
                           : (wxEnhancedPanel *)m_panelLocal;
   }

   // enable/disable the panels according to whether we use IMAP or local file
   void EnablePanelToBeUsed();

   // enable/disable the "Forward" button
   void UpdateForwardBtnUI(bool useIMAP);

private:
   wxRadioButton *m_radioIMAP,
                 *m_radioLocal;

   IMAPServerPanel *m_panelIMAP;
   LocalPanel *m_panelLocal;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MigrateWizardDstPage)
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

   DECLARE_NO_COPY_CLASS(MigrateWizardConfirmPage)
};

// ----------------------------------------------------------------------------
// MigrateWizardProgressPage: fourth and final page, show migration progress
// ----------------------------------------------------------------------------

class MigrateWizardProgressPage : public MigrateWizardPage
{
public:
   MigrateWizardProgressPage(MigrateWizard *parent);

protected:
   // enable/disable all wizard buttons
   void EnableWizardButtons(bool enable);

   // cancel button handler
   void OnButtonCancel(wxCommandEvent& event);

   // we don't have an "Ok" button but we generate a pseudo event from it to
   // start working immediately after being shown
   void OnButtonOk(wxCommandEvent& event);

   // this is where we post the "Ok" button event from
   void OnShow(wxShowEvent& event);

private:
   // update the messages progress meter
   bool UpdateMessageProgress();

   // update the folder progress meter (also updates the messages one)
   bool UpdateFolderProgress();

   // update the status shown in m_labelStatus
   bool UpdateStatus(const String& msg);


   // returns the folder to copy from
   MailFolder *OpenSource(const MigrateImapServer& imapData,
                          const String& name);

   // return the MFolder to copy to
   MFolder *GetDstFolder(const String& name, int flags);

   // set the access parameters for an IMAP folder (GetDstFolder() helper)
   void SetAccessParameters(MFolder *folderDst);

   // return the type of the folders we're creating
   MFolderType GetDstType() const;

   // get the dst folder name corresponding to the given source folder
   String GetDstNameForSource(const String& name);

   // do copy all messages
   bool CopyMessages(MailFolder *mfSrc, MFolder *folderDst);

   // process the source folder with this name
   bool ProcessOneFolder(const String& name, int flags);

   // create the target folder corresponding to the non-selectable source one
   bool CreateDstDirectory(const String& name);

   // process all folders
   bool ProcessAllFolders();

   // wrapper arounnd ProcessAllFolders()
   void DoMigration();


   // data
   int m_nFolder,                   // current folder or -1 if none yet
       m_nMessage,                  // current message in current folder
       m_countMessages;             // total number of messages or -1

   size_t m_nErrors;                // number of folders we couldn't copy

   bool m_continue;                 // set to false if we're cancelled

   // the GUI controls
   wxStaticText *m_labelFolder,     // folder progress label
                *m_labelMsg,        // message in the folder progress one
                *m_labelStatus;     // textual desscription of what we're doing

   wxGauge      *m_gaugeFolder,     // the progress meters themselves
                *m_gaugeMsg;

   wxButton     *m_btnAbort;        // the cancel button

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MigrateWizardProgressPage)
};

// ============================================================================
// MigrateModule implementation
// ============================================================================

MMODULE_BEGIN_IMPLEMENT(MigrateModule,
                        "Migrate",
                        STARTUP_INTERFACE,
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
   MFolder_obj folder(m_btnFolder->GetFolder());
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
            m_textServer->SetValue(wxEmptyString);

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
         m_chkSSL->SetValue(m_folder->GetSSL() == SSLSupport_SSL);
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

   int sel = m_choiceFormat->GetSelection();
   if ( sel == -1 )
   {
      wxLogError(_("Please select the local mailbox format."));

      return false;
   }

   m_localData->format = (FileMailboxFormat)sel;

   return true;
}

/* static */
const wxChar *LocalPanel::GetFormatName(FileMailboxFormat format)
{
   // this array must be in sync with FileMailboxFormat
   static const char *formatNames[] =
   {
      gettext_noop("MBX (UW-IMAPd)"),
      gettext_noop("MBOX (traditional Unix)"),
      gettext_noop("MMDF (SCO Unix)"),
      gettext_noop("Tenex (historical)"),
      gettext_noop("MH (one file per message)"),
   };

   ASSERT_MSG( (size_t)format < WXSIZEOF(formatNames),
               _T("mailbox format out of range") );

   return wxGetTranslation(formatNames[format]);
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
      Data().folderNames.Empty();
   }

   return true;
}

// ----------------------------------------------------------------------------
// MigrateWizardDstPage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MigrateWizardDstPage, MigrateWizardPage)
   EVT_RADIOBUTTON(-1, MigrateWizardDstPage::OnRadioButton)
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

   m_radioIMAP = new wxRadioButton(this, -1, _("to another &IMAP server:"),
                                   wxDefaultPosition, wxDefaultSize,
                                   wxRB_SINGLE);
   sizer->Add(m_radioIMAP, 0, wxALL, LAYOUT_X_MARGIN);

   m_panelIMAP = new IMAPServerPanel(this, &Data().dstIMAP);
   sizer->Add(m_panelIMAP, 1, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   m_radioLocal = new wxRadioButton(this, -1, _("or to a local &file:"),
                                    wxDefaultPosition, wxDefaultSize,
                                    wxRB_SINGLE);
   sizer->Add(m_radioLocal);

   m_panelLocal = new LocalPanel(this, &Data().dstLocal);
   sizer->Add(m_panelLocal, 1, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   SetSizer(sizer);
}

void MigrateWizardDstPage::EnablePanelToBeUsed()
{
   const bool useIMAP = Data().toIMAP;

   m_panelIMAP->Enable(useIMAP);
   m_panelLocal->Enable(!useIMAP);
}

void MigrateWizardDstPage::UpdateForwardBtnUI(bool useIMAP)
{
   if ( useIMAP )
   {
      // button should be disabled if the server is not entered
      m_panelIMAP->UpdateForwardBtnUI();
   }
   else // local dst
   {
      // button is always enabled
      EnableButtons(MigrateWizard::Btn_Next, true);
   }
}

void MigrateWizardDstPage::OnRadioButton(wxCommandEvent& event)
{
   const bool useIMAP = event.GetEventObject() == m_radioIMAP;
   Data().toIMAP = useIMAP;

   EnablePanelToBeUsed();

   UpdateForwardBtnUI(useIMAP);

   // manually disable the other radiobox as we use wxRB_SINGLE
   (useIMAP ? m_radioLocal : m_radioIMAP)->SetValue(false);
}

bool MigrateWizardDstPage::TransferDataToWindow()
{
   const bool useIMAP = Data().toIMAP;
   m_radioIMAP->SetValue(useIMAP);
   m_radioLocal->SetValue(!useIMAP);

   EnablePanelToBeUsed();

   // call TransferDataToWindow() for both panels even if only one is used at
   // any given moment
   if ( !m_panelIMAP->TransferDataToWindow() ||
            !m_panelLocal->TransferDataToWindow() )
   {
      return false;
   }

   UpdateForwardBtnUI(useIMAP);

   return true;
}

bool MigrateWizardDstPage::TransferDataFromWindow()
{
   // but only call TransferDataFromWindow() for the one the user selected
   return GetPanelToUse()->TransferDataFromWindow();
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

   msg.Printf(_("About to start copying %d folders from the\n"
                "server %s"),
              data.countFolders, data.source.server.c_str());

   const String& root = data.source.root;
   if ( !root.empty() )
      msg += String::Format(_(" (under %s only)"), root.c_str());

   msg += _T('\n');

   if ( data.toIMAP )
   {
      msg += String::Format
             (
               _("to the IMAP server\n%s"),
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
               _("to the files in %s format under the\n"
                 "directory \"%s\""),
               LocalPanel::GetFormatName(data.dstLocal.format),
               data.dstLocal.root.c_str()
             );
   }

   msg += _("\n\nPlease press \"Next\" to continue, \"Back\" to\n"
            "modify the migration parameters\n"
            "or \"Cancel\" to abort the operation.");

   return msg;
}

// ----------------------------------------------------------------------------
// MigrateWizardProgressPage
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MigrateWizardProgressPage, MigrateWizardPage)
   EVT_SHOW(MigrateWizardProgressPage::OnShow)

   EVT_BUTTON(wxID_OK, MigrateWizardProgressPage::OnButtonOk)
   EVT_BUTTON(wxID_CANCEL, MigrateWizardProgressPage::OnButtonCancel)
END_EVENT_TABLE()

MigrateWizardProgressPage::MigrateWizardProgressPage(MigrateWizard *parent)
                         : MigrateWizardPage
                           (
                              parent,
                              MigrateWizard::Page_Progress
                           )
{
   m_continue = true;

   // create the GUI controls
   wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
   sizer->Add
          (
            new wxStaticText
                (
                  this,
                  -1,
                  _("You may press \"Abort\" at any moment to\n"
                    "abort the migration but please be warned that\n"
                    "it will be impossible to resume it later.")
                ),
            0,
            wxALL,
            LAYOUT_X_MARGIN
          );

   sizer->Add(0, 2*LAYOUT_Y_MARGIN);

   m_labelFolder = new wxStaticText(this, -1, wxEmptyString);
   sizer->Add(m_labelFolder, 0, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   m_gaugeFolder = new wxGauge(this, -1, Data().countFolders,
                               wxDefaultPosition, wxDefaultSize,
                               wxGA_HORIZONTAL | wxGA_SMOOTH);
   sizer->Add(m_gaugeFolder, 0, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   m_labelMsg = new wxStaticText(this, -1, wxEmptyString);
   sizer->Add(m_labelMsg, 0, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   m_gaugeMsg = new wxGauge(this, -1, 0, // range will be set later
                            wxDefaultPosition, wxDefaultSize,
                            wxGA_HORIZONTAL | wxGA_SMOOTH);
   sizer->Add(m_gaugeMsg, 0, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   sizer->Add(0, 4*LAYOUT_Y_MARGIN);

   m_btnAbort = new wxButton(this, wxID_CANCEL, _("&Abort"));
   sizer->Add(m_btnAbort,
              0, wxALL | wxALIGN_CENTER_HORIZONTAL, LAYOUT_X_MARGIN);

   sizer->Add(0, 4*LAYOUT_Y_MARGIN);

   m_labelStatus = new wxStaticText(this, -1, _("Working..."),
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTRE);
   sizer->Add(m_labelStatus, 0, wxALL | wxEXPAND, LAYOUT_X_MARGIN);

   SetSizer(sizer);
}

bool MigrateWizardProgressPage::UpdateMessageProgress()
{
   m_labelMsg->SetLabel
               (
                  wxString::Format
                  (
                     _("Message: %d/%d"),
                     m_nMessage + 1,
                     m_countMessages
                  )
               );

   m_gaugeMsg->SetValue(m_nMessage);

   wxYield();

   return m_continue;
}

bool MigrateWizardProgressPage::UpdateFolderProgress()
{
   String fullname = Data().source.root,
          name = Data().folderNames[m_nFolder];

   if ( !fullname.empty() && !name.empty() )
      fullname += Data().source.delimiter;

   fullname += name;

   m_labelFolder->SetLabel
                  (
                     wxString::Format
                     (
                        _("Folder: %d/%d (%s)"),
                        m_nFolder + 1,
                        Data().countFolders,
                        fullname.c_str()
                     )
                  );

   m_gaugeFolder->SetValue(m_nFolder);

   wxYield();

   return m_continue;
}

bool MigrateWizardProgressPage::UpdateStatus(const String& msg)
{
   // we need to relayout because the size of the control changed and it must
   // be recentred
   m_labelStatus->SetLabel(msg);

   Layout();

   wxYield();

   return m_continue;
}

void MigrateWizardProgressPage::EnableWizardButtons(bool enable)
{
   // when we (re)enable the buttons, only "Finish" still makes sense, but when
   // we disable them, we want to disable all of them
   EnableButtons(enable ? MigrateWizard::Btn_Next
                        : MigrateWizard::Btn_All, enable);
}

MailFolder *
MigrateWizardProgressPage::OpenSource(const MigrateImapServer& imapData,
                                      const String& name)
{
   MFolder_obj folderSrc(MFolder::CreateTemp(wxEmptyString, MF_IMAP));
   CHECK( folderSrc, NULL, _T("MFolder::CreateTemp() failed?") );

   folderSrc->SetServer(imapData.server);

   String path = imapData.root;
   if ( !name.empty() )
   {
      path << Data().source.delimiter << name;
   }
   folderSrc->SetPath(path);

   folderSrc->SetAuthInfo(imapData.username, imapData.password);

#ifdef USE_SSL
   if ( imapData.useSSL )
   {
      folderSrc->SetSSL(SSLSupport_SSL, SSLCert_AcceptUnsigned);
   }
#endif // USE_SSL

   return MailFolder::OpenFolder(folderSrc, MailFolder::ReadOnly);
}

MFolderType
MigrateWizardProgressPage::GetDstType() const
{
   MFolderType folderType;
   if ( Data().toIMAP )
   {
      folderType = MF_IMAP;
   }
   else // file, but which?
   {
      folderType = Data().dstLocal.format == FileMbox_Max ? MF_MH : MF_FILE;
   }

   return folderType;
}

String
MigrateWizardProgressPage::GetDstNameForSource(const String& name)
{
   // when we start from source folder foo we want the target folder for foo to
   // be named foo (and not have empty name) so always append source.root to
   // the path
   String path = Data().dstLocal.root,
          rootSrc = Data().source.root;

   const char delimiterSrc = Data().source.delimiter;

   if ( !rootSrc.empty() )
   {
      if ( !path.empty() )
         path += delimiterSrc;

      path += rootSrc;
   }

   if ( !name.empty() )
   {
      if ( !path.empty() )
         path += delimiterSrc;

      path += name;
   }

   // make it into the dst path: it can have different delimiter
   for ( size_t n = 0; n < path.length(); n++ )
   {
      if ( path[n] == delimiterSrc )
      {
         if ( !Data().delimiterDst )
         {
            MFolder_obj folderDst(MFolder::CreateTemp(wxEmptyString, GetDstType()));
            SetAccessParameters(folderDst);

            Data().delimiterDst = MailFolder::GetFolderDelimiter(folderDst);

            if ( Data().delimiterDst == delimiterSrc )
            {
               // nothing to do, finally!
               break;
            }
         }

         path[n] = Data().delimiterDst;
      }
   }

   return path;
}

MFolder *
MigrateWizardProgressPage::GetDstFolder(const String& name, int flags)
{
   // which kind of folder are we going to create?
   MFolderType folderType = GetDstType();
   MFolder *folderDst = MFolder::CreateTemp(wxEmptyString, folderType);
   CHECK( folderDst, NULL, _T("MFolder::CreateTemp() failed?") );

   if ( folderType == MF_FILE )
   {
      folderDst->SetFileMboxFormat(Data().dstLocal.format);
   }

   String path = GetDstNameForSource(name);

   // there is a complication here: although IMAP folders may (depending on
   // server) contain both messages and subfolders, this is impossible with
   // MBOX and MBX folders (although ok with MH) and the way we deal with this
   // is to put the messages in a file named "folder.messages"
   if ( folderType == MF_FILE )
   {
      if ( !(flags & ASMailFolder::ATT_NOINFERIORS) )
      {
         // create a subdirectory for the subfolders if it doesn't exist yet
         if ( !wxDirExists(path) && !wxMkdir(path) )
         {
            wxLogWarning(_("Failed to create directory \"%s\" for folder \"%s\""),
                         path.c_str(), name.c_str());
         }

         // and modify the name for the file itself
         path += MESSAGES_SUFFIX;
      }
   }
   else // MF_IMAP
   {
      // TODO: some IMAP servers do support folders which have both messages
      //       and subfolders, we should test for this -- but for now we
      //       suppose the worst case
      if ( !(flags & ASMailFolder::ATT_NOINFERIORS) )
      {
         path += MESSAGES_SUFFIX;
      }

      SetAccessParameters(folderDst);
   }

   folderDst->SetPath(path);

   return folderDst;
}

void
MigrateWizardProgressPage::SetAccessParameters(MFolder *folderDst)
{
   CHECK_RET( folderDst, _T("NULL folder in SetAccessParameters") );

   if ( folderDst->GetType() == MF_IMAP )
   {
      const MigrateImapServer& dstIMAP = Data().dstIMAP;
      folderDst->SetServer(dstIMAP.server);
      folderDst->SetAuthInfo(dstIMAP.username, dstIMAP.password);

#ifdef USE_SSL
      folderDst->SetSSL(dstIMAP.useSSL ? SSLSupport_SSL : SSLSupport_None,
                        SSLCert_AcceptUnsigned);
#endif // USE_SSL
   }
}

bool
MigrateWizardProgressPage::CopyMessages(MailFolder *mfSrc, MFolder *folderDst)
{
   UIdArray uids;
   uids.Add(UID_ILLEGAL);

   HeaderInfoList_obj headers(mfSrc->GetHeaders());

   m_countMessages = headers->Count();
   m_gaugeMsg->SetRange(m_countMessages);

   for ( m_nMessage = 0; m_nMessage < m_countMessages; m_nMessage++ )
   {
      if ( !UpdateMessageProgress() )
      {
         // cancelled
         return true;
      }

      HeaderInfo *hi = headers->GetItemByIndex(m_nMessage);
      if ( !hi )
      {
         wxLogError(_("Failed to retrieve header for message %d"),
                    m_nMessage);
         continue;
      }

      uids[0] = hi->GetUId();
      if ( !mfSrc->SaveMessages(&uids, folderDst) )
      {
         wxLogError(_("Failed to copy the message %d from folder \"%s\""),
                    m_nMessage,
                    Data().folderNames[m_nFolder].c_str());

         return false;
      }
   }

   return true;
}

bool MigrateWizardProgressPage::CreateDstDirectory(const String& name)
{
   if ( Data().toIMAP )
   {
      // we don't have to do anything, it will be created when we access the
      // first subfolder of this folder (which will also be created
      // automatically)
      return true;
   }
   else // local file destination
   {
      // create a directory if it doesn't exist yet
      const String dir = GetDstNameForSource(name);
      return wxDirExists(dir) || wxMkdir(dir);
   }
}

bool MigrateWizardProgressPage::ProcessOneFolder(const String& name, int flags)
{
   // open the source folder
   MailFolder_obj mf(OpenSource(Data().source, name));
   if ( !mf )
   {
      wxLogError(_("Failed to open source folder \"%s\""), name.c_str());

      return false;
   }

   // don't create the target folder if the destination is empty and has
   // subfolders: i.e. we do create empty folders if they contain messages only
   // but we want to avoid creating empty "folder.messages" files if we have a
   // "folder" directory
   if ( !(flags & ASMailFolder::ATT_NOINFERIORS) && !mf->GetMessageCount() )
   {
      // nothing to do
      return true;
   }

   // create the folder to save the messages to
   MFolder_obj folderDst(GetDstFolder(name, flags));
   MailFolder_obj mfDst(MailFolder::OpenFolder(folderDst));
   if ( !mfDst )
   {
      wxLogError(_("Failed to create the target folder \"%s\""), name.c_str());

      return false;
   }

   // now copy all the messages from src to dst
   return CopyMessages(mf, folderDst);
}

bool MigrateWizardProgressPage::ProcessAllFolders()
{
   // ensure that the directory where we're going to create our files exists
   if ( !Data().toIMAP )
   {
      const String& dir = Data().dstLocal.root;
      if ( !dir.empty() && !wxDirExists(dir) )
      {
         if ( !wxMkdir(dir) )
         {
            wxLogError(_("Can't create the directory for the mailbox files.\n"
                         "\n"
                         "Migration aborted"));

            return false;
         }
      }
   }

   for ( m_nFolder = 0, m_nErrors = 0;
         m_nFolder < Data().countFolders;
         m_nFolder++ )
   {
      if ( !UpdateFolderProgress() )
      {
         // cancelled
         break;
      }

      const String& name = Data().folderNames[m_nFolder];

      // is this a "file" or a "directory"?
      if ( Data().folderFlags[m_nFolder] & ASMailFolder::ATT_NOSELECT )
      {
         // just create the corresponding directory or folder
         if ( !CreateDstDirectory(name) )
         {
            // it's not a fatal error (no messages lost...) but still worth
            // noting
            wxLogWarning(_("Failed to copy the folder \"%s\""), name.c_str());
         }
      }
      else // a "file"-like folder, copy the messages from it
      {
         if ( !ProcessOneFolder(name, Data().folderFlags[m_nFolder]) )
         {
            wxLogError(_("Failed to copy messages from folder \"%s\""),
                       name.c_str());

            m_nErrors++;
         }
      }
   }

   return true;
}

void MigrateWizardProgressPage::DoMigration()
{
   EnableWizardButtons(false);

   bool ok = ProcessAllFolders();

   // update the UI to show that we're done now
   m_btnAbort->Disable();

   m_labelFolder->Disable();
   m_gaugeFolder->Disable();
   m_labelMsg->Disable();
   m_gaugeMsg->Disable();

   String msg;
   if ( !ok )
   {
      msg = _("Migration couldn't be done.");
   }
   else if ( m_continue )
   {
      m_gaugeMsg->SetValue(m_countMessages);
      m_gaugeFolder->SetValue(Data().countFolders);

      String msg;
      if ( m_nErrors )
      {
         wxLogError(_("There were errors during the migration."));

         msg.Printf(_("Done with %u error(s)"), m_nErrors);
      }
      else
      {
         msg = _("Completed successfully.");
      }
   }
   else // cancelled
   {
      msg = _("Migration aborted.");
   }

   UpdateStatus(msg);

   // let the user dismiss the wizard now
   EnableWizardButtons(true);

   wxWindow *btnFinish = GetParent()->FindWindow(wxID_FORWARD);
   CHECK_RET( btnFinish, _T("no \"Finish\" button?") );

   btnFinish->SetFocus();
}

void MigrateWizardProgressPage::OnShow(wxShowEvent& event)
{
#if wxCHECK_VERSION(2, 9, 0)
   if ( event.IsShown() )
#else
   if ( event.GetShow() )
#endif
   {
      wxCommandEvent eventOk(wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK);
      wxPostEvent(this, eventOk);
   }

   event.Skip();
}

void MigrateWizardProgressPage::OnButtonOk(wxCommandEvent& /* event */)
{
   DoMigration();
}

void MigrateWizardProgressPage::OnButtonCancel(wxCommandEvent& /* event */)
{
   if ( wxMessageBox(_("Warning: you won't be able to resume later!"
                       "\n"
                       "Are you still sure you want to abort?"),
                     _("Mahogany: Please confirm"),
                     wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT) == wxYES )
   {
      m_continue = false;
   }
}

// ----------------------------------------------------------------------------
// MigrateWizard
// ----------------------------------------------------------------------------

MigrateWizard::MigrateWizard(wxWindow *parent)
             : wxWizard(parent,
                        -1,
                        _("Mahogany Migration Tool"),
                        mApplication->GetIconManager()->GetBitmap(_T("migrate")))
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

void MigrateWizard::EnableButtons(int buttons, bool enable)
{
   wxWindow *btn;

   if ( buttons & Btn_Back )
   {
      btn = FindWindow(wxID_BACKWARD);
      if ( btn )
         btn->Enable(enable);
   }

   if ( buttons & Btn_Next )
   {
      btn = FindWindow(wxID_FORWARD);
      if ( btn )
         btn->Enable(enable);
   }

   if ( buttons & Btn_Cancel )
   {
      btn = FindWindow(wxID_CANCEL);
      if ( btn )
         btn->Enable(enable);
   }
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
      /* Page_WarnEmptySource */    Page_Max,
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

         MFolder_obj folderSrc(MFolder::CreateTemp(wxEmptyString, MF_IMAP));

         const MigrateImapServer& imapData = Data().source;
         folderSrc->SetServer(imapData.server);
         folderSrc->SetPath(imapData.root);
         folderSrc->SetAuthInfo(imapData.username, imapData.password);

#ifdef USE_SSL
         if ( imapData.useSSL )
         {
            // we don't have a separate checkbox for accepting unsigned SSL
            // certs because of lack of space in the dialog but we do accept
            // them here because always accepting them is better than never
            // doing it
            folderSrc->SetSSL(SSLSupport_SSL, SSLCert_AcceptUnsigned);
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

            if ( ListAll(asmf) )
            {
               // process the events from ListFolders
               do
               {
                  MEventManager::ForceDispatchPending();
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
      case Page_Dst:
      case Page_Confirm:
         return true;

      default:
         FAIL_MSG( _T("unknown page in MigrateWizard") );

      case Page_WarnEmptySource:
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
MigrateWizard::OnListFolder(const String& path, wxChar delim, long flags)
{
   Data().AddFolder(path, delim, flags);
}

void
MigrateWizard::OnNoMoreFolders()
{
   Data().FixFolderFlags();

   m_doneWithList = true;
}

// ----------------------------------------------------------------------------
// MigrateData implementation
// ----------------------------------------------------------------------------

void MigrateData::AddFolder(const String& path, char delim, long flags)
{
   source.delimiter = delim;

   // we abuse ATT_NOINFERIORS flag here to mean not only that the folder
   // doesn't have children but also to imply that a folder without this
   // flag does have children (which is not true for IMAP) -- initially it's
   // set as we don't have any children yet and we correct it in
   // FixFolderFlags() later
   folderNames.Add(path);
   folderFlags.Add(flags | ASMailFolder::ATT_NOINFERIORS);
   countFolders++;
}

void MigrateData::FixFolderFlags()
{
   for ( int n = 0; n < countFolders; n++ )
   {
      const String& name = folderNames[n];

      // the index of the folder which has children (none so far)
      int idx = wxNOT_FOUND;

      // all folders are children of the the root one so if we have any more
      // folders we must not have ATT_NOINFERIORS flag
      if ( name.empty() )
      {
         if ( countFolders > 1 )
         {
            idx = n;
         }
      }
      else // check if this folder is a child of some of the previous folders
      {
         String parent = name.BeforeLast(source.delimiter);
         if ( !parent.empty() )
         {
            idx = folderNames.Index(parent);
         }
      }

      if ( idx != wxNOT_FOUND )
      {
         // this one does have children
         folderFlags[(size_t)idx] &= ~ASMailFolder::ATT_NOINFERIORS;
      }
   }
}

