///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   wizards.cpp - various GUI wizards
// Purpose:
// Author:      Karsten Ballüder (ballueder@gmx.net)
// Modified by:
// Created:     2000
// CVS-ID:      $Id$
// Copyright:   (c) 2000-2002 M-Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "strutil.h"
#   include "Mdefaults.h"
#   include "MApplication.h"
#   include "guidef.h"
#   include "gui/wxIconManager.h"

#   include <wx/stattext.h>        // for wxStaticText
#endif // USE_PCH

#include "Mpers.h"

#include "MailFolderCC.h"
#include "MFolder.h"

#include <wx/wizard.h>

#include "gui/wxBrowseButton.h"
#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_COLLECTATSTARTUP;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_FOLDER_PATH;
extern const MOption MP_IMAPHOST;
extern const MOption MP_MOVE_NEWMAIL;
extern const MOption MP_NNTPHOST;
extern const MOption MP_POPHOST;
extern const MOption MP_USERNAME;
extern const MOption MP_USE_FOLDER_CREATE_WIZARD;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_SAVE_PWD;
extern const MPersMsgBox *M_MSGBOX_USE_FOLDER_CREATE_WIZARD;

// ----------------------------------------------------------------------------
// global vars and functions
// ----------------------------------------------------------------------------

/**
  The MWizard is a very simple to use Wizard class. Each wizard uses a
  set of pages (can be shared across wizards) and knows on which page
  to start and finish. The decision which page is next/previous is
  handled by the page which can query the Wizard for a numeric id to
  know what kind of wizard is running it.
 */

// ids for install wizard pages
enum MWizardPageId
{
   MWizard_CreateFolder_First,
   MWizard_CreateFolder_Welcome = MWizard_CreateFolder_First,     // say hello
   MWizard_CreateFolder_Type,

   MWizard_CreateFolder_File,
   MWizard_CreateFolder_MH,
   MWizard_CreateFolder_Imap,
   MWizard_CreateFolder_Pop3,

   // the new mail page is displayed only for the folder types <= than this one
   MWizard_CreateFolder_LastNewMail = MWizard_CreateFolder_Pop3,

   MWizard_CreateFolder_Nntp,
   MWizard_CreateFolder_News,
   MWizard_CreateFolder_Group,

   MWizard_CreateFolder_NewMail,    // should we monitor this folder?
   MWizard_CreateFolder_Final,      // say that everything is ok
   MWizard_CreateFolder_FirstType  = MWizard_CreateFolder_File,
   MWizard_CreateFolder_LastType = MWizard_CreateFolder_Group,
   MWizard_CreateFolder_Last = MWizard_CreateFolder_Final,

   MWizard_ImportFolders_First,
   MWizard_ImportFolders_Choice = MWizard_ImportFolders_First,
   MWizard_ImportFolders_MH,
   MWizard_ImportFolders_Last = MWizard_ImportFolders_MH,

   MWizard_PagesMax,                // the number of pages

   MWizard_PageNone = -1            // illegal value
};


enum MWizardType
{
   MWizardType_CreateFolder,
   MWizardType_ImportFolders
};

class MWizard : public wxWizard
{
public:
   MWizard(MWizardType type,
           MWizardPageId first,
           MWizardPageId last,
           const wxString &title,
           const wxBitmap * bitmap = NULL,
           wxWindow *parent = NULL)
      : wxWizard( parent, -1, title,
                  // using bitmap before '?' results in a compile error with
                  // Borland C++ - go figure
                  !bitmap ? mApplication->GetIconManager()->
                              GetBitmap("install_welcome")
                          : *bitmap)
      {
         m_Type = type;
         m_First = first;
         m_Last = last;
         // NULL the pages array
         memset(m_WizardPages, 0, sizeof(m_WizardPages));
      }

   /** Returns a numeric type for the Wizard to allow pages to change
       their behaviour for different Wizards. No deeper meaning.
   */
   MWizardType GetType(void) const { return m_Type; }

   MWizardPageId GetFirstPageId(void) const { return m_First; }
   MWizardPageId GetLastPageId(void) const { return m_Last; }
   bool Run(void);
   class MWizardPage * GetPageById(MWizardPageId id);

private:
   MWizardType m_Type;
   class MWizardPage * m_WizardPages[MWizard_PagesMax];
   MWizardPageId m_First, m_Last;

   DECLARE_NO_COPY_CLASS(MWizard)
};


// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the base class for our wizards pages: it allows to return page ids (and not
// the pointers themselves) from GetPrev/Next and processes [Cancel] in a
// standard way an provides other useful functions for the derived classes

class MWizardPage : public wxWizardPage
{
public:
   MWizardPage(MWizard *wizard, MWizardPageId id)
      : wxWizardPage(wizard)
      {
         m_id = id;
         m_Wizard = wizard;
      }

    /* These are two wxWindows wxWizard functions that we must override
    */
   virtual wxWizardPage *GetPrev() const { return GetPageById(GetPreviousPageId()); }
   virtual wxWizardPage *GetNext() const { return GetPageById(GetNextPageId()); }

   /// Override these two in derived classes if needed:
   virtual MWizardPageId GetNextPageId() const;
   virtual MWizardPageId GetPreviousPageId() const;

   MWizardPageId GetPageId() const { return m_id; }
   virtual MWizard * GetWizard() const { return (MWizard *)m_Wizard; }

   void OnWizardCancel(wxWizardEvent& event);

protected:
   wxWizardPage *GetPageById(MWizardPageId id) const
      { return GetWizard()->GetPageById(id); }

   // set the label of the next button depending on whether we're going to
   // advance to the next page or if this is the last one
   void SetNextButtonLabel(bool isLast);

   // creates an "enhanced panel" for placing controls into under the static
   // text (explanation)
   wxEnhancedPanel *CreateEnhancedPanel(wxStaticText *text);

private:
   // id of this page
   MWizardPageId m_id;
   // the wizard
   MWizard *m_Wizard;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MWizardPage)
};

BEGIN_EVENT_TABLE(MWizardPage, wxWizardPage)
   EVT_WIZARD_CANCEL(-1, MWizardPage::OnWizardCancel)
END_EVENT_TABLE()


MWizardPageId
MWizardPage::GetPreviousPageId(void) const
{
   return  GetPageId() > GetWizard()->GetFirstPageId() ?
      (MWizardPageId) (GetPageId()-1) : GetWizard()->GetFirstPageId();
}

MWizardPageId
MWizardPage::GetNextPageId(void) const
{
   return GetPageId() < m_Wizard->GetLastPageId() ?
      (MWizardPageId) (GetPageId()+1)
      : m_Wizard->GetLastPageId();
}

void MWizardPage::OnWizardCancel(wxWizardEvent& event)
{
   if ( !MDialog_YesNoDialog(_("Do you really want to abort the wizard?")) )
   {
      // not confirmed
      event.Veto();
   }
}

wxEnhancedPanel *MWizardPage::CreateEnhancedPanel(wxStaticText *text)
{
   wxSize sizeLabel = text->GetSize();
   wxSize sizePage = ((wxWizard *)GetParent())->GetPageSize();
//   wxSize sizePage = ((wxWizard *)GetParent())->GetSize();
   wxCoord y = sizeLabel.y + 2*LAYOUT_Y_MARGIN;

   wxEnhancedPanel *panel = new wxEnhancedPanel(this, false /* no scrolling */);
   panel->SetSize(0, y, sizePage.x, sizePage.y - y);

   panel->SetAutoLayout(true);

   return panel;
}

void MWizardPage::SetNextButtonLabel(bool isLast)
{
   wxButton * const
      btn = wxDynamicCast(GetParent()->FindWindow(wxID_FORWARD), wxButton);
   if ( btn )
   {
      btn->SetLabel(isLast ? _("&Finish") : _("&Next >"));
   }
}


bool
MWizard::Run()
{
   return RunWizard(GetPageById(GetFirstPageId()));
}

// ----------------------------------------------------------------------------
// ImportFoldersWizard: propose to import existing folders
// ----------------------------------------------------------------------------

class ImportFoldersWizard : public MWizard
{
public:
   ImportFoldersWizard()
      : MWizard( MWizardType_ImportFolders,
                 MWizard_ImportFolders_First,
                 MWizard_ImportFolders_Last,
                 _("Import existing mail folders"))
      {
      }

   struct Params
   {
      // MH params
      String pathRootMH;
      bool takeAllMH;

      // what to do?
      bool importMH;

      // def ctor
      Params()
      {
         importMH = false;
      }
   };

   Params& GetParams() { return m_params; }

private:
   Params m_params;

   DECLARE_NO_COPY_CLASS(ImportFoldersWizard)
};

// MWizard_ImportFolders_ChoicePage
// ----------------------------------------------------------------------------

// first page: propose all folders we can import to the user
class MWizard_ImportFolders_ChoicePage : public MWizardPage
{
public:
   MWizard_ImportFolders_ChoicePage(MWizard *wizard);

   virtual MWizardPageId GetPreviousPageId() const { return MWizard_PageNone; }
   virtual MWizardPageId GetNextPageId() const;

   void OnCheckBox(wxCommandEvent& event);

private:
   wxCheckBox *m_checkMH;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MWizard_ImportFolders_ChoicePage)
};

BEGIN_EVENT_TABLE(MWizard_ImportFolders_ChoicePage, MWizardPage)
   EVT_CHECKBOX(-1, MWizard_ImportFolders_ChoicePage::OnCheckBox)
END_EVENT_TABLE()

MWizard_ImportFolders_ChoicePage::MWizard_ImportFolders_ChoicePage(MWizard *wizard)
                             : MWizardPage(wizard,
                                           MWizard_ImportFolders_Choice)
{
   m_checkMH = NULL;

   bool hasMH = false,
        hasSomethingToImport = false;

   if ( MailFolderCC::ExistsMH() )
   {
      hasMH = true;
      hasSomethingToImport = true;
   }

   wxString msgText;
   if ( hasSomethingToImport )
   {
      msgText = _("Please choose what would you like to do.");
   }
   else
   {
      msgText = _("Mahogany didn't find any folders to import on this system.");
   }

   wxStaticText *msg = new wxStaticText(this, -1, msgText);

   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   wxArrayString labels;
   if ( hasMH )
   {
      labels.Add(_("Import MH folders"));
   }

   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   maxwidth += 10;

   if ( hasMH )
   {
      m_checkMH = panel->CreateCheckBox(labels[0], maxwidth, NULL);

      // by default, import them all
      m_checkMH->SetValue(true);
   }

   panel->Layout();
}

MWizardPageId MWizard_ImportFolders_ChoicePage::GetNextPageId() const
{
   if ( m_checkMH && m_checkMH->GetValue() )
      return MWizard_ImportFolders_MH;

   //else if ( m_check && m_check->GetValue() ) ...

   return MWizard_PageNone;
}

void MWizard_ImportFolders_ChoicePage::OnCheckBox(wxCommandEvent&)
{
   SetNextButtonLabel(GetNextPageId() == MWizard_PageNone);
}

// MWizard_ImportFolders_MHPage
// ----------------------------------------------------------------------------

// first page: propose all folders we can import to the user
class MWizard_ImportFolders_MHPage : public MWizardPage
{
public:
   MWizard_ImportFolders_MHPage(MWizard *wizard);

   virtual MWizardPageId GetPreviousPageId() const
      { return MWizard_ImportFolders_Choice; }
   virtual MWizardPageId GetNextPageId() const
      { return MWizard_PageNone; }

   virtual bool TransferDataFromWindow();

private:
   wxTextCtrl *m_textTop;
   wxCheckBox *m_checkAll;

   DECLARE_NO_COPY_CLASS(MWizard_ImportFolders_MHPage)
};

MWizard_ImportFolders_MHPage::MWizard_ImportFolders_MHPage(MWizard *wizard)
                         : MWizardPage(wizard, MWizard_ImportFolders_MH)
{
   wxStaticText *msg = new wxStaticText(this, -1, _(
            "You may import either just the top level MH folder and\n"
            "later create manually all or some of its subfolders using\n"
            "the popup menu in the folder tree or import all existing\n"
            "MH subfolders at once.\n"
            "\n"
            "Don't change the default location of the top of MH tree\n"
            "(the MH root) unless you really know what you are doing."
      ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   wxArrayString labels;
   labels.Add(_("MH root"));
   labels.Add(_("Import all"));

   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   maxwidth += 5;

   m_textTop = panel->CreateDirEntry(labels[0], maxwidth, NULL);
   m_checkAll = panel->CreateCheckBox(labels[1], maxwidth, m_textTop);

   // init controls
   m_textTop->SetValue(MailFolderCC::InitializeMH());
   m_checkAll->SetValue(true);

   panel->Layout();
}

bool MWizard_ImportFolders_MHPage::TransferDataFromWindow()
{
   ImportFoldersWizard::Params& params =
      ((ImportFoldersWizard *)GetWizard())->GetParams();

   params.importMH = true;
   params.takeAllMH = m_checkAll->GetValue();
   params.pathRootMH = m_textTop->GetValue();

   return true;
}

// ============================================================================
// CreateFolderWizard and its pages
// ============================================================================

enum FolderEntryType
{
   FOLDERTYPE_FILE = 0,
   FOLDERTYPE_MH,
   FOLDERTYPE_IMAP,
   FOLDERTYPE_POP3,
   FOLDERTYPE_NNTP,
   FOLDERTYPE_NEWS,
   FOLDERTYPE_GROUP,
   FOLDERTYPE_MAX
};

// ----------------------------------------------------------------------------
// CreateFolderWizard
// ----------------------------------------------------------------------------

class CreateFolderWizard : public MWizard
{
public:
   CreateFolderWizard(MFolder *parentFolder,
                      wxWindow *parent = NULL)
      : MWizard( MWizardType_CreateFolder,
                 MWizard_CreateFolder_First,
                 MWizard_CreateFolder_Last,
                 _("Create a Mailbox Entry"),
                 NULL, parent)
      {
         m_wantsDialog = false;
         m_ParentFolder = parentFolder;
         SafeIncRef(m_ParentFolder);
      }
   ~CreateFolderWizard()
      {
         SafeDecRef(m_ParentFolder);
      }

   struct FolderParams
   {
      FolderParams()
         : m_Name(_("New Folder"))
      {
         m_FolderType = MF_ILLEGAL;
         m_FolderFlags = 0;

         m_LeaveOnServer =
         m_CheckOnStartup = false;
      }

      MFolderType m_FolderType;
      int m_FolderFlags;

      wxString m_Name;
      wxString m_Path;
      wxString m_Server;
      wxString m_Login;
      wxString m_Password;

      bool m_LeaveOnServer;
      bool m_CheckOnStartup;
   };

   FolderParams *GetParams() { return &m_Params; }
   MFolder *GetParentFolder() const
      { if ( m_ParentFolder ) m_ParentFolder->IncRef(); return m_ParentFolder; }

   void SetUserWantsDialog() { m_wantsDialog = true; }
   bool UserWantsDialog() const { return m_wantsDialog; }

   // accessors for the pages
   bool HasNewMailPage() const
      { return m_idServerPage <= MWizard_CreateFolder_LastNewMail; }

   void SetServerPageId(MWizardPageId id) { m_idServerPage = id; }
   MWizardPageId GetServerPageId() const { return m_idServerPage; }

protected:
   MFolder *m_ParentFolder;
   FolderParams m_Params;

   // the id of the server page we use, filled by MWizard_CreateFolder_TypePage
   // and used by the subsequent ones
   MWizardPageId m_idServerPage;

   // does the user want to use dialog instead of wizard?
   bool m_wantsDialog;

   DECLARE_NO_COPY_CLASS(CreateFolderWizard)
};


// ----------------------------------------------------------------------------
// CreateFolderWizardWelcomePage
// ----------------------------------------------------------------------------

// first page: welcome the user, explain what goes on
class MWizard_CreateFolder_WelcomePage : public MWizardPage
{
public:
   MWizard_CreateFolder_WelcomePage(MWizard *wizard);

   virtual MWizardPageId GetPreviousPageId() const { return MWizard_PageNone; }
   virtual MWizardPageId GetNextPageId() const;

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   void OnCheckbox(wxCommandEvent& event);

   bool ShouldSkipWizard() const
   {
      return m_checkNoWizard->GetValue() || m_checkNeverWizard->GetValue();
   }

private:
   wxCheckBox *m_checkNoWizard,
              *m_checkNeverWizard;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MWizard_CreateFolder_WelcomePage)
};

BEGIN_EVENT_TABLE(MWizard_CreateFolder_WelcomePage, MWizardPage)
   EVT_CHECKBOX(-1, MWizard_CreateFolder_WelcomePage::OnCheckbox)
END_EVENT_TABLE()

MWizard_CreateFolder_WelcomePage::
MWizard_CreateFolder_WelcomePage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Welcome)
{
   wxStaticText *msg = new wxStaticText(this, -1, _(
      "This wizard will help you to set up\n"
      "a new mailbox entry.\n"
      "This can be either for an existing\n"
      "mailbox or mail account or for a new\n"
      "mailbox that you want to create.\n"
      "\n"
      "You may also use the check box below to\n"
      "bypass the wizard and create the folder\n"
      "by manually filling in the required\n"
      "parameters (for advanced users only!)"
      ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);
   wxArrayString labels;
   labels.Add(_("&Don't use the wizard this time"));
   labels.Add(_("&Never use the wizard again"));

   long wMax = GetMaxLabelWidth(labels, panel->GetCanvas());

   m_checkNoWizard = panel->CreateCheckBox(labels[0], wMax, NULL);
   m_checkNeverWizard = panel->CreateCheckBox(labels[1], wMax, m_checkNoWizard);

   panel->Layout();
}

MWizardPageId MWizard_CreateFolder_WelcomePage::GetNextPageId() const
{
   if ( ShouldSkipWizard() )
   {
      // cancel wizard
      ((CreateFolderWizard*)GetWizard())->SetUserWantsDialog();

      return MWizard_PageNone;
   }

   return MWizardPage::GetNextPageId();
}

void MWizard_CreateFolder_WelcomePage::OnCheckbox(wxCommandEvent&)
{
   // if any of our checkboxes if checked, we close the wizard right after this
   // page so change the label to reflect it
   SetNextButtonLabel(ShouldSkipWizard());
}

bool MWizard_CreateFolder_WelcomePage::TransferDataToWindow()
{
   if ( !MWizardPage::TransferDataToWindow() )
      return false;

   // use the value of "use wizard" option to set the initial checkbox value
   if ( !READ_APPCONFIG(MP_USE_FOLDER_CREATE_WIZARD) )
      m_checkNoWizard->SetValue(true);

   return true;
}

bool MWizard_CreateFolder_WelcomePage::TransferDataFromWindow()
{
   if ( !MWizardPage::TransferDataFromWindow() )
      return false;

   if ( m_checkNeverWizard->GetValue() )
   {
      wxLogMessage(_("You have chosen to never use the wizard for the "
                     "folder creation.\n"
                     "\n"
                     "If you change your mind later, you can always go to\n"
                     "\"Reenable disabled message boxes\" dialog in the last\n"
                     "page of the \"Preferences\" dialog and enable it back."));

      // show it now, before showing the options dialog
      wxLog::FlushActive();

      const String path = GetPersMsgBoxName(M_MSGBOX_USE_FOLDER_CREATE_WIZARD);
      wxPMessageBoxDisable(path, wxNO);
   }
   else // we're not disabled
   {
      // save the checkbox value for the next run
      mApplication->GetProfile()->writeEntry(MP_USE_FOLDER_CREATE_WIZARD,
                                             !m_checkNoWizard->GetValue());
   }

   return true;
}

// ----------------------------------------------------------------------------
// CreateFolderWizardTypePage
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_TypePage : public MWizardPage
{
public:
   MWizard_CreateFolder_TypePage(MWizard *wizard);

   virtual MWizardPageId GetNextPageId() const;

private:
   wxChoice *m_TypeCtrl;

   DECLARE_NO_COPY_CLASS(MWizard_CreateFolder_TypePage)
};


MWizard_CreateFolder_TypePage::MWizard_CreateFolder_TypePage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Type)
{
   wxStaticText *msg = new wxStaticText(
      this, -1,
      _("A mailbox entry can represent different\n"
        "things: a mailbox file, remote servers\n"
        "or mailboxes, newsgroups or news hierarchies\n"
        "either locally or on a remote server.\n"
        "\n"
        "So, first you need to decide what kind\n"
        "of entry you would like to create:"));

   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);

   wxArrayString labels;
   labels.Add(_("Mailbox type"));

   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   // a choice control, the entries are taken from the label string which is
   // composed as: "LABEL:entry1:entry2:entry3:...."
   m_TypeCtrl = panel->CreateChoice(_("Entry type:"
                                      "Local Mailbox File:"
                                      "Local MH Folder:"
                                      "IMAP Mailbox:"
                                      "POP3 Server:"
                                      "NNTP Newsgroup:"
                                      "Local Newsgroup:"
                                      "Folder Group"),
                                    maxwidth, NULL);

   // should be always in sync
   ASSERT_MSG( m_TypeCtrl->GetCount() == FOLDERTYPE_MAX,
               _T("forgot to update something") );

   panel->Layout();
}

MWizardPageId
MWizard_CreateFolder_TypePage::GetNextPageId() const
{
   int sel = m_TypeCtrl->GetSelection();

   // assume first selection if we don't have any - we do have to return
   // some next page
   if ( sel == -1 )
       sel = 0;

   sel += MWizard_CreateFolder_FirstType;

   if ( sel < MWizard_CreateFolder_FirstType ||
         sel > MWizard_CreateFolder_LastType )
   {
      FAIL_MSG(_T("Unknown folder type"));

      sel = MWizard_CreateFolder_Final;
   }

   MWizardPageId id = (MWizardPageId)sel;

   // remember the server page id in the wizard
   ((CreateFolderWizard *)GetWizard())->SetServerPageId(id);

   return id;
}

// ----------------------------------------------------------------------------
// CreateFolderServerPage - common page for different server/hierarchy types
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_ServerPage : public MWizardPage
{
public:
   MWizard_CreateFolder_ServerPage(MWizard *wizard,
                                   MWizardPageId id,
                                   FolderEntryType type);

   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   virtual MWizardPageId GetNextPageId() const
   {
      CreateFolderWizard *wiz = (CreateFolderWizard*)GetWizard();

      return wiz->HasNewMailPage() ? MWizard_CreateFolder_NewMail
                                   : MWizard_CreateFolder_Final;
   }

   virtual MWizardPageId GetPreviousPageId() const
   {
      return MWizard_CreateFolder_Type;
   }

private:
   wxTextCtrl *m_Name,
              *m_Server,
              *m_UserId,
              *m_Password,
              *m_Path;

   wxCheckBox *m_LeaveOnServer;

   wxFileOrDirBrowseButton *m_browsePath;

   FolderEntryType m_Type;

   DECLARE_NO_COPY_CLASS(MWizard_CreateFolder_ServerPage)
};

// keep this enum in sync with the labels array in the function below
//
// NB: we can't move the enum inside the function because g++ dies with the
//     internal compiler error otherwise (when building with optimizations)
enum
{
  Label_Server,
  Label_Username,
  Label_Password,
  Label_Mailbox,
  Label_Name,
  Label_LeaveOnServer,
  Label_Max
};

MWizard_CreateFolder_ServerPage::
MWizard_CreateFolder_ServerPage(MWizard *wizard,
                                MWizardPageId id,
                                FolderEntryType type)
   : MWizardPage(wizard, id)
{
   m_Type = type;

   wxString
      entry,
      pathName = _("Mailbox name"),
      msg = _("To create %s\n"
              "the following access parameters\n"
              "are needed:\n");

   // initialize all the variables just to avoid the compiler warnings
   bool needsServer = false,
        needsUserId = false,
        needsPath = false,
        needsLeaveOnServer = false,
        canBrowse = false;

   switch(type)
   {
      case FOLDERTYPE_MAX:
         FAIL_MSG(_T("unknown folder type"));
         // fall through

      case FOLDERTYPE_GROUP:
         needsServer = false; // don't know which server :-)
         needsUserId = true;
         needsPath = false;
         canBrowse = false;
         break;

         // IMAP server, mailbox or group
      case FOLDERTYPE_IMAP:
         entry = _("an IMAP folder");
         needsServer = true;
         needsUserId = true;
         needsPath = true;
         canBrowse = true;
         break;

      case FOLDERTYPE_POP3:
         entry = _("a POP3 server");
         needsServer = true;
         needsUserId = true;
         needsPath = false;
         needsLeaveOnServer = true;
         canBrowse = false;
         break;

      case FOLDERTYPE_NNTP:
      case FOLDERTYPE_NEWS:
         if ( type == FOLDERTYPE_NEWS )
         {
            entry = _("a local newsgroup");
            needsServer = false;
         }
         else // NNTP
         {
            entry = _("a NNTP newsgroup");
            needsServer = true;
         }

         // NNTP may have authentification, but it is an advanced setting, don't
         // present it here
         needsUserId = false;
         needsPath = true;
         canBrowse = true;

         pathName = _("Newsgroup");
         msg += _("\n"
                  "Note that you can either leave newsgroup empty or enter\n"
                  "news., news.announce or news.announce.newgroups\n"
                  "here, for example");
         break;

      case FOLDERTYPE_MH:
      case FOLDERTYPE_FILE:
         entry = type == FOLDERTYPE_FILE ? _("a local mailbox file")
                                         : _("a local MH mailbox");
         needsServer = false;
         needsUserId = false;
         needsPath = true;
         canBrowse = type == FOLDERTYPE_MH;
         break;
   }

   wxString text;
   if ( type == FOLDERTYPE_GROUP )
   {
      text = _("A folder group is not a mailbox itself\n"
               "but simply helps you organise the tree.\n"
               "\n"
               "Its sub-entries can inherit values from\n"
               "the group, so you can fill in default\n"
               "values for the most important settings\n"
               "here, which will be inherited by all\n"
               "entries below this group.\n");
   }
   else
   {
      if ( needsUserId )
      {
         msg += _("\n"
                  "Password is not required, if you don't enter\n"
                  "it here you will be asked for it later.");
      }

      text.Printf(msg, entry.c_str());
   }

   wxStaticText *msgCtrl = new wxStaticText(this, -1, text);

   wxEnhancedPanel *panel = CreateEnhancedPanel(msgCtrl);

   // if you modify this array, don't forget to modify the enum just before
   // this function!
   wxArrayString labels;
   labels.Add(_("&Server host"));
   labels.Add(_("Your &login"));
   labels.Add(_("Your &password"));
   labels.Add(pathName);
   labels.Add(_("Folder &name"));
   labels.Add(_("&Leave mail on server"));

   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

#define CREATE_CTRL(name, creat) \
   if ( needs##name ) \
   { \
      m_##name = creat; \
      last = m_##name; \
   } \
   else \
      m_##name = NULL

   // if we ask for login name, we ask for password as well
   // (and we do need this var to be able to use the macro above)
   bool needsPassword = needsUserId;

   wxControl *last = NULL;
   CREATE_CTRL(Server,
               panel->CreateTextWithLabel(labels[Label_Server],
                                          maxwidth,
                                          last));
   CREATE_CTRL(UserId,
               panel->CreateTextWithLabel(labels[Label_Username],
                                          maxwidth,
                                          last));

   CREATE_CTRL(Password,
               panel->CreateTextWithLabel(labels[Label_Password],
                                          maxwidth,
                                          last,
                                          0,
                                          wxPASSWORD));

   CREATE_CTRL(Path,
               panel->CreateFileOrDirEntry(labels[Label_Mailbox],
                                           maxwidth,
                                           last,
                                           canBrowse ? &m_browsePath : NULL,
                                           true,
                                           false));

   CREATE_CTRL(LeaveOnServer,
               panel->CreateCheckBox(labels[Label_LeaveOnServer],
                                     maxwidth,
                                     last));

#undef CREATE_CTRL

   last = panel->CreateMessage(_("\nThe name which will appear in the tree:"),
                              last);

   m_Name = panel->CreateTextWithLabel(labels[Label_Name], maxwidth, last);

   switch ( type )
   {
      case FOLDERTYPE_MH:
         // the browser button should allow to select directories, not folders
         // then
         m_browsePath->BrowseForDirectories();
         break;

      case FOLDERTYPE_NEWS:
      case FOLDERTYPE_NNTP:
      case FOLDERTYPE_IMAP:
         // we don't support browsing the remote servers yet (TODO)
         m_browsePath->wxButton::Enable(false);
         break;

      default:
         // suppress gcc warning
         ;
   }

   panel->Layout();
}

bool
MWizard_CreateFolder_ServerPage::TransferDataFromWindow()
{
   CreateFolderWizard::FolderParams *params =
      ((CreateFolderWizard*)GetWizard())->GetParams();

   if(m_Name && m_Name->GetValue() != params->m_Name)
      params->m_Name = m_Name->GetValue();
   if(m_Path && params->m_Path != m_Path->GetValue())
      params->m_Path = m_Path->GetValue();
   if(m_Server && params->m_Server != m_Server->GetValue())
      params->m_Server = m_Server->GetValue();
   if(m_UserId && params->m_Login != m_UserId->GetValue())
      params->m_Login = m_UserId->GetValue();
   if(m_Password && params->m_Password != m_Password->GetValue())
   {
      wxString msg;
      msg = _("You have specified a password here, which is relatively unsafe.\n");
      msg << _("\n"
               "Notice that the password will be stored in your configuration with\n"
               "very weak encryption. If you are concerned about security, leave it\n"
               "empty and Mahogany will prompt you for it whenever needed.");
      msg << _("\nDo you want to save the password anyway?");
      if ( MDialog_YesNoDialog(msg, this, MDIALOG_YESNOTITLE,
                               M_DLG_YES_DEFAULT,
                               M_MSGBOX_SAVE_PWD) )
         params->m_Password = m_Password->GetValue();
   }

   if ( m_LeaveOnServer )
   {
      params->m_LeaveOnServer = m_LeaveOnServer->GetValue();
   }

   if(m_Server)
   {
      String server = params->m_Server;
      strutil_tolower(server);

      // TODO better check if the hostname resolves to the same address as
      //      localhost - my machine name is faster to type than localhost, so I
      //      always use it instead, yet the folders on this machien are local
      if(server== "localhost")
         params->m_FolderFlags |= MF_FLAGS_ISLOCAL;
   }

   switch(m_Type)
   {
      case FOLDERTYPE_MAX:
         FAIL_MSG(_T("unknown folder type"));
         // fall through

      case FOLDERTYPE_GROUP:
         params->m_FolderType = MF_GROUP;
         break;

      case FOLDERTYPE_IMAP:
         params->m_FolderType = MF_IMAP;
         params->m_FolderFlags |= MF_FLAGS_GROUP;
         break;

      case FOLDERTYPE_POP3:
         params->m_FolderType = MF_POP;
         break;

      case FOLDERTYPE_NNTP:
         params->m_FolderType = MF_NNTP;
         params->m_FolderFlags |= MF_FLAGS_ANON | MF_FLAGS_GROUP;
         break;

      case FOLDERTYPE_NEWS:
         params->m_FolderType = MF_NEWS;
         params->m_FolderFlags |= MF_FLAGS_GROUP;
         break;

      case FOLDERTYPE_FILE:
         params->m_FolderType = MF_FILE;
         break;

      case FOLDERTYPE_MH:
         {
            wxString name,
                     root = MailFolderCC::InitializeMH();
            if ( params->m_Path.StartsWith(root, &name) )
            {
               // remove extra slashes
               const char *p = name.c_str();
               while ( *p == '/' )
                  p++;
               if ( p != name.c_str() )
                  name = p;

               params->m_Path = name;
            }
            else
            {
               wxLogError(_("The path '%s' is invalid for a MH folder. All MH "
                            "folders should be under the directory '%s'."),
                         name.c_str(), root.c_str());

               return false;
            }

            params->m_FolderType = MF_MH;
         }
         break;
   }

   return true;
}

bool
MWizard_CreateFolder_ServerPage::TransferDataToWindow()
{
   CreateFolderWizard *wiz = (CreateFolderWizard*)GetWizard();
   CreateFolderWizard::FolderParams *params = wiz->GetParams();

   MFolder_obj f(wiz->GetParentFolder());
   Profile *profile;
   if ( f )
   {
      profile = f->GetProfile();
   }
   else
   {
      profile = mApplication->GetProfile();
      profile->IncRef();
   }

   CHECK(profile, false, _T("No profile?"));

   Profile_obj p(profile);

   // don't overwrite the settings previously entered by user
   if ( m_Name->GetValue().empty() )
   {
      m_Name->SetValue(params->m_Name);
   }

   if ( m_Path && m_Path->GetValue().empty() )
   {
      switch ( GetPageId() )
      {
         case MWizard_CreateFolder_MH:
            // start from the MHROOT
            m_Path->SetValue(MailFolderCC::InitializeMH());
            break;

         case MWizard_CreateFolder_Imap:
         case MWizard_CreateFolder_Nntp:
         case MWizard_CreateFolder_Pop3:
            // they have their own value, don't change
            break;

         default:
            m_Path->SetValue(READ_CONFIG(p, MP_FOLDER_PATH));
      }
   }

   if ( m_Server && m_Server->GetValue().empty() )
   {
      String serverKey;
      switch ( GetPageId() )
      {
         case MWizard_CreateFolder_Imap:
            serverKey = GetOptionName(MP_IMAPHOST);
            break;

         case MWizard_CreateFolder_Nntp:
            serverKey = GetOptionName(MP_NNTPHOST);
            break;

         case MWizard_CreateFolder_Pop3:
            serverKey = GetOptionName(MP_POPHOST);
            break;

         default:
            FAIL_MSG(_T("This folder has no server setting!"));
      }

      m_Server->SetValue(p->readEntry(serverKey, ""));
   }

   if ( m_UserId && m_UserId->GetValue().empty() )
   {
      m_UserId->SetValue(READ_CONFIG(p, MP_USERNAME));
   }

   return true;
}

// ----------------------------------------------------------------------------
// CreateFolderWizardXXXPage - the various folder types
// ----------------------------------------------------------------------------

#define DEFINE_TYPEPAGE(type,type2) \
class MWizard_CreateFolder_##type##Page : public \
  MWizard_CreateFolder_ServerPage \
{ \
public: \
   MWizard_CreateFolder_##type##Page(MWizard *wizard) \
      : MWizard_CreateFolder_ServerPage(wizard,\
                                        MWizard_CreateFolder_##type,\
                                        FOLDERTYPE_##type2)\
 { } \
 \
private: \
   DECLARE_NO_COPY_CLASS(MWizard_CreateFolder_##type##Page) \
}

DEFINE_TYPEPAGE(File,FILE);
DEFINE_TYPEPAGE(MH,MH);
DEFINE_TYPEPAGE(Imap,IMAP);
DEFINE_TYPEPAGE(Pop3,POP3);
DEFINE_TYPEPAGE(Nntp,NNTP);
DEFINE_TYPEPAGE(News,NEWS);
DEFINE_TYPEPAGE(Group,GROUP);

#undef DEFINE_TYPEPAGE

// ----------------------------------------------------------------------------
// CreateFolderWizard new mail page
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_NewMailPage : public MWizardPage
{
public:
   MWizard_CreateFolder_NewMailPage(MWizard *wizard);

   virtual bool TransferDataFromWindow();

   virtual MWizardPageId GetPreviousPageId() const
      { return ((CreateFolderWizard *)GetWizard())->GetServerPageId(); }

   virtual MWizardPageId GetNextPageId() const
      { return MWizard_CreateFolder_Final; }

protected:
   void OnCheckbox(wxCommandEvent& WXUNUSED(event))
   {
      m_checkOnStartup->Enable(m_checkMonitor->GetValue());
   }

private:
   wxCheckBox *m_checkMonitor,
              *m_checkOnStartup;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MWizard_CreateFolder_NewMailPage)
};

BEGIN_EVENT_TABLE(MWizard_CreateFolder_NewMailPage, MWizardPage)
   EVT_CHECKBOX(-1, MWizard_CreateFolder_NewMailPage::OnCheckbox)
END_EVENT_TABLE()

MWizard_CreateFolder_NewMailPage::
MWizard_CreateFolder_NewMailPage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_NewMail)
{
   wxStaticText *msg = new wxStaticText(this, -1, _(
                           "Mahogany may check this folder for new mail\n"
                           "automatically (otherwise, it will only be\n"
                           "detected when you open it). You can configure\n"
                           "the delay between the checks in the folder\n"
                           "properties dialog after creating it.\n"
                           "\n"
                           "Would you like to monitor this folder for new\n"
                           "messages in the background?\n"
                           ));

   wxEnhancedPanel *panel = CreateEnhancedPanel(msg);

   wxArrayString labels;
   labels.Add(_("&Monitor this folder:"));
   labels.Add(_("&Check it on startup:"));

   long maxwidth = GetMaxLabelWidth(labels, panel->GetCanvas());

   m_checkMonitor = panel->CreateCheckBox(labels[0],
                                          maxwidth, NULL);
   m_checkOnStartup = panel->CreateCheckBox(labels[1],
                                            maxwidth, m_checkMonitor);

   // should only be enabled if "monitor" is checked
   m_checkOnStartup->Disable();

   panel->Layout();
}

bool MWizard_CreateFolder_NewMailPage::TransferDataFromWindow()
{
   CreateFolderWizard::FolderParams *params =
      ((CreateFolderWizard*)GetWizard())->GetParams();

   if ( m_checkMonitor->GetValue() )
   {
      params->m_FolderFlags |= MF_FLAGS_MONITOR;
   }
   else
   {
      params->m_FolderFlags &= ~MF_FLAGS_MONITOR;
   }

   if ( (params->m_FolderFlags & MF_FLAGS_MONITOR) &&
         m_checkOnStartup->GetValue() )
   {
      params->m_CheckOnStartup = TRUE;
   }

   return true;
}

// ----------------------------------------------------------------------------
// CreateFolderWizard final page
// ----------------------------------------------------------------------------

class MWizard_CreateFolder_FinalPage : public MWizardPage
{
public:
   MWizard_CreateFolder_FinalPage(MWizard *wizard);

   virtual MWizardPageId GetPreviousPageId() const;
   virtual MWizardPageId GetNextPageId() const { return MWizard_PageNone; }

private:
   DECLARE_NO_COPY_CLASS(MWizard_CreateFolder_FinalPage)
};


MWizard_CreateFolder_FinalPage::MWizard_CreateFolder_FinalPage(MWizard *wizard)
   : MWizardPage(wizard, MWizard_CreateFolder_Final)
{
   CreateFolderWizard::FolderParams *params =
      ((CreateFolderWizard*)GetWizard())->GetParams();

   wxString msg =  wxString::Format(_(
      "You are now ready to create the new folder\n"
      "'%s', press \"Finish\" to do it.\n"
      "\n"
      "If you want to use any more advanced\n"
      "access options, you can modify the\n"
      "complete set of parameters by invoking\n"
      "the folder properties dialog later. Simply\n"
      "click the right mouse button on the\n"
      "entry in the tree and choose \"Properties\"."
   ), params->m_Name.c_str());

   MFolderType ftype = MF_ILLEGAL; // init it to suppress compiler warnings
   if ( CanHaveSubfolders(params->m_FolderType, params->m_FolderFlags, &ftype)
        && ftype != MF_ILLEGAL )
   {
      // TODO propose to add all subfolders to the created entry from here
      //      (or, better, show a separate page for this before)
      msg += _(
            "\n"
            "Note: this folder may contain the subfolders.\n"
            "If you want to add them to the tree as well,\n"
            "simply select the \"Browse\" command from the\n"
            "\"Folder\" menu or right click popup menu and\n"
            "follow the instructions."
      );
   }

   (void)new wxStaticText(this, -1, msg);
}

MWizardPageId MWizard_CreateFolder_FinalPage::GetPreviousPageId() const
{
   CreateFolderWizard *wiz = (CreateFolderWizard *)GetWizard();

   // if we have the new mail page, it is just before us - otherwise it is the
   // server/mailbox params page
   MWizardPageId id = wiz->HasNewMailPage() ? MWizard_CreateFolder_NewMail
                                            : wiz->GetServerPageId();

   return id;
}

// ============================================================================
// implementation
// ============================================================================


MWizardPage *
MWizard::GetPageById(MWizardPageId id)
{
   if ( id == GetLastPageId()+1 || id == MWizard_PageNone)
      return (MWizardPage *)NULL;

   if ( !m_WizardPages[id-GetFirstPageId()] )
   {
#define CREATE_PAGE(pid) \
      case MWizard_##pid: \
         m_WizardPages[MWizard_##pid-GetFirstPageId()] = \
            new MWizard_##pid##Page(this); \
         break

      switch ( id )
      {
         CREATE_PAGE(CreateFolder_Welcome);
         CREATE_PAGE(CreateFolder_Type);
         CREATE_PAGE(CreateFolder_NewMail);
         CREATE_PAGE(CreateFolder_Final);
         CREATE_PAGE(CreateFolder_File);
         CREATE_PAGE(CreateFolder_MH);
         CREATE_PAGE(CreateFolder_Imap);
         CREATE_PAGE(CreateFolder_Pop3);
         CREATE_PAGE(CreateFolder_Nntp);
         CREATE_PAGE(CreateFolder_News);
         CREATE_PAGE(CreateFolder_Group);

         CREATE_PAGE(ImportFolders_Choice);
         CREATE_PAGE(ImportFolders_MH);

      case MWizard_PageNone:
      case MWizard_PagesMax:
         ASSERT_MSG(0, _T("illegal MWizard PageId"));
      }
#undef CREATE_PAGE
   }
   return m_WizardPages[id-GetFirstPageId()];
}


// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

MFolder *
RunCreateFolderWizard(bool *wantsDialog, MFolder *parent, wxWindow *parentWin)
{
   MFolder *newfolder = NULL;

   // ensure that the pointer is always valid
   bool dummyWantsDialog;
   if ( !wantsDialog )
      wantsDialog = &dummyWantsDialog;

   // are we configured to skip the wizard completely?
   const String path = GetPersMsgBoxName(M_MSGBOX_USE_FOLDER_CREATE_WIZARD);
   if ( wxPMessageBoxIsDisabled(path) )
   {
      *wantsDialog = true;
   }
   else // wizard not disabled
   {
      CreateFolderWizard *wizard = new CreateFolderWizard(parent, parentWin);

      *wantsDialog = false;

      if ( wizard->Run() )
      {
         if ( wizard->UserWantsDialog() )
         {
            *wantsDialog = true;
         }
         else // wizard completed successfully, create folder
         {
            CreateFolderWizard::FolderParams *params = wizard->GetParams();
            MFolderType type = params->m_FolderType;

            // collect mail from the POP3 folders by default
            if ( type == MF_POP )
            {
               params->m_FolderFlags |= MF_FLAGS_INCOMING;
            }

            newfolder = CreateFolderTreeEntry(
               parent,
               params->m_Name,
               type,
               params->m_FolderFlags,
               params->m_Path,
               true // notify about folder creation
            );

            if ( newfolder )
            {
               Profile_obj p(newfolder->GetProfile());

               String serverKey;
               switch ( type )
               {
                  case MF_IMAP:
                     serverKey = GetOptionName(MP_IMAPHOST);
                     break;

                  case MF_NNTP:
                     serverKey = GetOptionName(MP_NNTPHOST);
                     break;

                  case MF_POP:
                     serverKey = GetOptionName(MP_POPHOST);
                     break;

                  default:
                     ; // nothing
               }

               if ( !serverKey.empty() )
               {
                  p->writeEntry(serverKey, params->m_Server);
               }

               if ( FolderTypeHasUserName(type) )
               {
                  p->writeEntry(MP_USERNAME, params->m_Login);

                  if ( !params->m_Password.empty() )
                  {
                     p->writeEntry(MP_FOLDER_PASSWORD,
                                   strutil_encrypt(params->m_Password));
                  }
               }

               if ( params->m_CheckOnStartup )
               {
                  p->writeEntry(MP_COLLECTATSTARTUP, false);
               }

               if ( params->m_LeaveOnServer )
               {
                  p->writeEntry(MP_MOVE_NEWMAIL, false);
               }
            }
         }
      }

      delete wizard;
   }

   return newfolder;
}

void RunImportFoldersWizard()
{
   ImportFoldersWizard *wizard = new ImportFoldersWizard();
   if ( wizard->Run() )
   {
      const ImportFoldersWizard::Params& params = wizard->GetParams();
      if ( params.importMH )
      {
         MailFolderCC::ImportFoldersMH(params.pathRootMH, params.takeAllMH);
      }
   }

   delete wizard;
}
