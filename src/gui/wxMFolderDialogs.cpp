///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   wxMFolderDialogs.cpp - implementation of functions from
//              MFolderDialogs.h
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.12.98
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
#   include "Mcommon.h"
#   include "MApplication.h"
#   include "Profile.h"
#   include "guidef.h"
#endif

#include <wx/log.h>
#include <wx/imaglist.h>
#include <wx/notebook.h>
#include <wx/persctrl.h>

#include "MFolderDialogs.h"
#include "MailFolder.h"
#include "MFolder.h"

#include "Mdefaults.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxBrowseButton.h"
#include "gui/wxFolderTree.h"

// -----------------------------------------------------------------------------
// private classes
// -----------------------------------------------------------------------------

// base class for folder creation and properties dialog
class wxFolderBaseDialog : public wxNotebookDialog
{
public:
   wxFolderBaseDialog(wxWindow *parent, const wxString& title);

   // initialization (should be called before the dialog is shown)
      // set folder we're working with
   void SetFolder(MFolder *folder)
      { m_newFolder = folder; }
      // set the parent folder: if it's !NULL, it can't be changed by user
   void SetParentFolder(MFolder *parentFolder)
      { m_parentFolder = parentFolder; }

   // after the dialog is closed, retrieve the folder which was created or
   // NULL if the user cancelled us without creating anything
   MFolder *GetFolder() const { return m_newFolder; }

   // override control creation functions
   virtual wxControl *CreateControlsAbove(wxPanel *panel);
   virtual void CreateNotebook(wxPanel *panel);

   // control ids
   enum
   {
      Folder_Name = 0x1000
   };

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxFolderBaseDialog() { }

protected:
   wxTextCtrl  *m_folderName,
               *m_parentName;

   wxFolderBrowseButton *m_btnParentFolder;

   MFolder     *m_parentFolder,
               *m_newFolder;
};

// folder creation dialog
class wxFolderCreateDialog : public wxFolderBaseDialog
{
public:
   wxFolderCreateDialog(wxWindow *parent,
                        MFolder *parentFolder);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // called by the page to create the new folder
   MFolder *DoCreateFolder(MFolder::Type typeFolder);

   // callbacks
   void OnFolderNameChange(wxEvent&);

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxFolderCreateDialog() { }

private:
   DECLARE_DYNAMIC_CLASS(wxFolderCreateDialog)
   DECLARE_EVENT_TABLE()
};

// folder properties dialog
class wxFolderPropertiesDialog : public wxFolderBaseDialog
{
public:
   wxFolderPropertiesDialog(wxWindow *parent,
                            MFolder *folder);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();
};

// A panel for defining a new Folder
class wxFolderPropertiesPage : public wxNotebookPageBase
{
public:
   wxFolderPropertiesPage(wxNotebook *notebook, ProfileBase *profile);

   // enable additional fields for "Create" dialog
   void EnableAllFields();

   // set the profile path to copy the default values from
   void SetDefaults(const String& profilePath)
      { m_defaultsPath = profilePath; }

   virtual bool TransferDataToWindow(void);
   virtual bool TransferDataFromWindow(void);

   /// update controls
   void UpdateUI(void);
   void OnEvent(wxCommandEvent& event);

protected:
   bool m_isCreating;

   wxNotebook * m_notebook;
   ProfileBase * m_profile;
   /// type of the folder
   int m_type;
   /// folder type
   wxRadioBox *m_radio;
   /// user name
   wxTextCtrl *m_login;
   /// password
   wxTextCtrl *m_password;
   /// folder path
   wxTextCtrl *m_path;
   /// and a browse button for it
   wxFileBrowseButton *m_browsePath;
   /// server name
   wxTextCtrl *m_server;
   /// newsgroup name
   wxTextCtrl *m_newsgroup;

   // comment for the folder
   wxTextCtrl *m_comment;

   // the path to the profile section with the defautl values
   wxString m_defaultsPath;

   DECLARE_EVENT_TABLE()
};

// notebook for folder properties/creation
// FIXME should it be used for showing folder properties too? Or is it for
//       creation only? Karsten, I'm lost...
class wxFolderCreateNotebook : public wxNotebookWithImages
{
public:
   // icon names
   static const char *s_aszImages[];

   wxFolderCreateNotebook(wxWindow *parent);
};

// folder selection dialog: contains the folder tree inside it
class wxFolderSelectionDialog : public wxDialog
{
public:
   wxFolderSelectionDialog(wxWindow *parent, MFolder *folder);

   // callbacks
   void OnOK(wxCommandEvent& event);
   void OnCancel(wxCommandEvent& event);

   // get the chosen folder
   MFolder *GetFolder() const { return m_folder; }

private:
   MFolder *m_folder;
   wxFolderTree *m_tree;

   DECLARE_EVENT_TABLE()
};

// ============================================================================
// implementation
// ============================================================================

// -----------------------------------------------------------------------------
// event tables
// -----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxFolderCreateDialog, wxNotebookDialog)

BEGIN_EVENT_TABLE(wxFolderCreateDialog, wxNotebookDialog)
   EVT_TEXT(wxFolderCreateDialog::Folder_Name,
            wxFolderCreateDialog::OnFolderNameChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderPropertiesPage, wxNotebookPageBase)
   EVT_RADIOBOX(-1, wxFolderPropertiesPage::OnEvent)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderSelectionDialog, wxDialog)
   EVT_BUTTON(wxID_OK,     wxFolderSelectionDialog::OnOK)
   EVT_BUTTON(wxID_CANCEL, wxFolderSelectionDialog::OnCancel)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderBaseDialog
// ----------------------------------------------------------------------------

wxFolderBaseDialog::wxFolderBaseDialog(wxWindow *parent,
                                       const wxString& title)
                  : wxNotebookDialog(GET_PARENT_OF_CLASS(parent, wxFrame),
                                     title)
{
   m_parentFolder = NULL;
   m_newFolder = NULL;
}

wxControl *wxFolderBaseDialog::CreateControlsAbove(wxPanel *panel)
{
   wxLayoutConstraints *c;
   wxStaticText *pLabel;

   static size_t widthBtn = 0;
   if ( widthBtn == 0 ) {
      // calculate it only once, it's almost a constant
      widthBtn = 2*GetCharWidth();
   }

   // box with the folder name
   // ------------------------

   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   pLabel = new wxStaticText(panel, -1, _("Folder Name: "),
                             wxDefaultPosition, wxDefaultSize,
                             wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   if ( m_newFolder )
   {
      // a folder specified from the very beginning, can't be changed
      m_folderName = new wxTextCtrl(panel, Folder_Name, 
                                    m_newFolder->GetFullName(),
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_READONLY);

      m_parentFolder = m_newFolder->GetParent();
   }
   else
   {
      // no folder specified, it can be changed
      m_folderName = new wxTextCtrl(panel, Folder_Name, "");
   }

   c = new wxLayoutConstraints;
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
   c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_folderName->SetConstraints(c);

   // and with parent folder name
   // ---------------------------

   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(m_folderName, wxBottom, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   pLabel = new wxStaticText(panel, -1, _("Parent folder: "),
                             wxDefaultPosition, wxDefaultSize,
                             wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   // don't allow changing it if parent folder is fixed
   if ( m_parentFolder )
   {
      m_parentName = new wxTextCtrl(panel, -1,
                                    m_parentFolder->GetFullName(),
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_READONLY);
   }
   else
   {
      m_parentName = new wxTextCtrl(panel, -1, "");
   }

   c = new wxLayoutConstraints;
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(panel, wxRight, widthBtn + 3*LAYOUT_X_MARGIN);
   c->top.SameAs(m_folderName, wxBottom, LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_parentName->SetConstraints(c);

   // and also create a button for browsing for parent folder
   m_btnParentFolder = new wxFolderBrowseButton(m_parentName, panel,
                                                m_parentFolder);
   c = new wxLayoutConstraints;
   c->top.SameAs(m_parentName, wxTop);
   c->left.RightOf(m_parentName, LAYOUT_X_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
   c->height.SameAs(m_parentName, wxHeight);
   m_btnParentFolder->SetConstraints(c);

   m_btnParentFolder->Enable(FALSE);

   // return the last control created
   return m_parentName;
}

void wxFolderBaseDialog::CreateNotebook(wxPanel *panel)
{
   m_notebook = new wxFolderCreateNotebook(panel);
}

// -----------------------------------------------------------------------------
// wxFolderCreateDialog
// -----------------------------------------------------------------------------

wxFolderCreateDialog::wxFolderCreateDialog(wxWindow *parent,
                                           MFolder *parentFolder)
                    : wxFolderBaseDialog(parent, _("Create new folder"))
{
   SetParentFolder(parentFolder);
}

MFolder *wxFolderCreateDialog::DoCreateFolder(MFolder::Type typeFolder)
{
   if ( !m_parentFolder )
   {
      m_parentFolder = m_btnParentFolder->GetFolder();
   }

   m_newFolder = m_parentFolder->CreateSubfolder(m_folderName->GetValue(),
                                                 typeFolder);

   return m_newFolder;
}

void wxFolderCreateDialog::OnFolderNameChange(wxEvent& event)
{
   SetDirty();

   EnableButtons(!m_folderName->GetValue().IsEmpty());

   event.Skip();
}

bool wxFolderCreateDialog::TransferDataToWindow()
{
   EnableButtons(FALSE);   // initially, we have no folder name

   // initialize our pages
   wxFolderPropertiesPage *page = (wxFolderPropertiesPage *)(m_notebook->
                                    GetPage(FolderCreatePage_Folder));
   page->EnableAllFields();

   // by default, take the same values as for the parent
   if ( m_parentFolder )
   {
      wxString profilePath;
      profilePath << M_FOLDER_CONFIG_SECTION << m_parentFolder->GetName();
      page->SetDefaults(profilePath);
   }

   if ( !m_parentFolder )
   {
      // enable changing the parent folder to choose one
      m_btnParentFolder->Enable(TRUE);
   }
   //else: the parent folder is fixed, don't let user change it

   return wxNotebookDialog::TransferDataToWindow();
}

bool wxFolderCreateDialog::TransferDataFromWindow()
{
   // for the creation of a folder we don't use the toplevel config
   // section but create a profile of that name first
   ProfileBase *profile = ProfileBase::CreateProfile
                           (
                            m_folderName->GetValue(),
                            mApplication->GetProfile()
                           );
   profile->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);

   // tell the pages to use this profile instead of the global one
   ((wxOptionsPage *)m_notebook->GetPage(FolderCreatePage_Compose))
      ->SetProfile(profile);

   if ( !wxNotebookDialog::TransferDataFromWindow() )
      return FALSE;

   profile->DecRef();

   // do create the new folder
   CHECK( m_parentFolder, FALSE, "should have created parent folder" );

   int index = m_parentFolder->GetSubfolder(m_folderName->GetValue());

   CHECK( index != -1, FALSE, "new folder not created" );

   m_newFolder = m_parentFolder->GetSubfolder((size_t)index);

   return TRUE;
}

// -----------------------------------------------------------------------------
// wxFolderPropertiesDialog
// -----------------------------------------------------------------------------

wxFolderPropertiesDialog::wxFolderPropertiesDialog(wxWindow *frame,
                                                   MFolder *folder)
                        : wxFolderBaseDialog(frame, _("Folder properties"))
{
   SetFolder(folder);
}

bool wxFolderPropertiesDialog::TransferDataToWindow()
{
   CHECK( m_newFolder, FALSE, "no folder i folder properties dialog" );

   return TRUE;
}

bool wxFolderPropertiesDialog::TransferDataFromWindow()
{
   CHECK( m_newFolder, FALSE, "no folder i folder properties dialog" );

   // FIXME must change m_newFolder if something changed!

   return TRUE;
}

// -----------------------------------------------------------------------------
// wxFolderPropertiesPage
// -----------------------------------------------------------------------------

wxFolderPropertiesPage::wxFolderPropertiesPage(wxNotebook *notebook,
                                               ProfileBase *profile)
                      : wxNotebookPageBase(notebook)
{
   // add us to the notebook
   int image = notebook->GetPageCount();
   notebook->AddPage(this, _("Access"), FALSE /* don't select */, image);

   // init members
   // ------------
   m_notebook = notebook;
   m_profile = profile;

   wxLayoutConstraints *c;

   // create controls
   // ---------------

   // radiobox of folder type
   wxString radioChoices[5];
   radioChoices[0] = _("INBOX");
   radioChoices[1] = _("File");
   radioChoices[2] = _("POP3");
   radioChoices[3] = _("IMAP");
   radioChoices[4] = _("Newsgroup");

   m_radio = new wxRadioBox(this,-1,_("Folder Type"),
                            wxDefaultPosition,wxDefaultSize,
                            WXSIZEOF(radioChoices), radioChoices,
                            // this 5 is not WXSIZEOF(radioChoices), it's just
                            // the number of radiobuttons per line
                            5, wxRA_SPECIFY_COLS);

   c = new wxLayoutConstraints();
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(this, wxTop, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->height.Absolute(40); // FIXME: AsIs() doesn't work for wxGTK
   m_radio->SetConstraints(c);

   // text entries
   enum
   {
      Label_Login,
      Label_Password,
      Label_Path,
      Label_Server,
      Label_Newsgroup,
      Label_Comment,
      Label_Max
   };

   static const char *szLabels[Label_Max] =
   {
      "User name: ",
      "Password: ",
      "File name: ",
      "Server: ",
      "Newsgroup: ",
      "Comment: "
   };

   wxArrayString labels;
   for ( size_t n = 0; n < Label_Max; n++ )
   {
      labels.Add(_(szLabels[n]));
   }

   // determine the longest label
   long widthMax = GetMaxLabelWidth(labels, this);

   m_login = CreateTextWithLabel(labels[Label_Login], widthMax, m_radio);
   m_password = CreateTextWithLabel(labels[Label_Password], widthMax, m_login);
   m_server = CreateTextWithLabel(labels[Label_Server], widthMax, m_password);
   m_newsgroup = CreateTextWithLabel(labels[Label_Newsgroup], widthMax, m_server);
   m_comment = CreateTextWithLabel(labels[Label_Comment], widthMax, m_newsgroup);
   m_path = CreateFileEntry(labels[Label_Path], widthMax, m_comment, &m_browsePath);

   // by default, we're in "view properties" mode
   m_radio->Enable(FALSE);
   m_isCreating = FALSE;

   UpdateUI();
}

void
wxFolderPropertiesPage::EnableAllFields()
{
   m_radio->Enable(TRUE);
   m_isCreating = TRUE;
   UpdateUI();
}

void
wxFolderPropertiesPage::OnEvent(wxCommandEvent& event)
{
   UpdateUI();
}

void
wxFolderPropertiesPage::UpdateUI(void)
{
   switch(m_radio->GetSelection())
   {
      case MailFolder::MF_POP:
      case MailFolder::MF_IMAP:
         m_login->Enable(TRUE);
         m_password->Enable(TRUE);
         m_server->Enable(TRUE);
         m_newsgroup->Enable(FALSE);

         m_browsePath->Enable(FALSE);
         break;

      case MailFolder::MF_NNTP:
         m_login->Enable(FALSE);
         m_password->Enable(FALSE);
         m_server->Enable(TRUE);
         m_newsgroup->Enable(TRUE);

         m_browsePath->Enable(FALSE);
         break;

      case MailFolder::MF_FILE:
         m_login->Enable(FALSE);
         m_password->Enable(FALSE);
         m_server->Enable(FALSE);
         m_newsgroup->Enable(FALSE);

         // this can not be changed for an already existing folder
         m_browsePath->Enable(TRUE & m_isCreating);
         break;

      case MailFolder::MF_INBOX:
         m_login->Enable(FALSE);
         m_password->Enable(FALSE);
         m_server->Enable(FALSE);
         m_newsgroup->Enable(FALSE);

         m_browsePath->Enable(FALSE);
         break;

      default:
         wxFAIL_MSG("Unexpected folder type.");
   }
}


bool
wxFolderPropertiesPage::TransferDataToWindow(void)
{
   ProfilePathChanger pathChanger(m_profile, m_defaultsPath);

   m_login->SetValue(READ_CONFIG(m_profile,MP_POP_LOGIN));
   m_password->SetValue(READ_CONFIG(m_profile,MP_POP_PASSWORD));
   m_server->SetValue(READ_CONFIG(m_profile,MP_POP_HOST));
   m_path->SetValue(READ_CONFIG(m_profile,MP_FOLDER_PATH));
   m_newsgroup->SetValue(READ_CONFIG(m_profile,MP_FOLDER_PATH));
   m_comment->SetValue(READ_CONFIG(m_profile, MP_FOLDER_COMMENT));

   return TRUE;
}

bool
wxFolderPropertiesPage::TransferDataFromWindow(void)
{
   // 1st step: create the folder in the MFolder sense. For this we need only
   // the name and the type
   MFolder::Type typeFolder = (MFolder::Type)m_radio->GetSelection();
   wxFolderCreateDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderCreateDialog);

   CHECK( dlg, FALSE, "folder properties page should be in a folder dlg" );

   MFolder *newFolder = dlg->DoCreateFolder(typeFolder);
   if ( !newFolder )
   {
      return FALSE;
   }

   // 2nd step: put what we can in MFolder
   newFolder->SetComment(m_comment->GetValue());

   // 3rd step: store additional data about the newly created folder directly
   // in the profile
   wxString profilePath;
   profilePath << M_FOLDER_CONFIG_SECTION << newFolder->GetName();
   ProfilePathChanger pathChanger(m_profile, profilePath);
   switch ( typeFolder )
   {
      case MFolder::POP:
      case MFolder::IMAP:
         m_profile->writeEntry(MP_POP_LOGIN,m_login->GetValue());
         m_profile->writeEntry(MP_POP_PASSWORD,m_password->GetValue());
         m_profile->writeEntry(MP_POP_HOST,m_server->GetValue());
         break;

      case MFolder::News:
         m_profile->writeEntry(MP_NNTPHOST,m_server->GetValue());
         m_profile->writeEntry(MP_FOLDER_PATH,m_newsgroup->GetValue());
         break;

      case MFolder::File:
         m_profile->writeEntry(MP_FOLDER_PATH,m_newsgroup->GetValue());
         break;

      case MFolder::Inbox:
         // FIXME VZ: this doesn't make any sense, does it?
         m_path->SetValue("INBOX");
         break;

      default:
         wxFAIL_MSG("Unexpected folder type.");
   }

   return true;
}

// -----------------------------------------------------------------------------
// wxFolderCreateNotebook
// -----------------------------------------------------------------------------

// should be in sync with the enum FolderCreatePage!
const char *wxFolderCreateNotebook::s_aszImages[] =
{
   "compose",
   "access",
   NULL
};

// create the control and add pages too
wxFolderCreateNotebook::wxFolderCreateNotebook(wxWindow *parent)
                      : wxNotebookWithImages("FolderCreateNotebook",
                                             parent,
                                             s_aszImages)
{
   // don't forget to update both the array above and the enum!
   wxASSERT( WXSIZEOF(s_aszImages) == FolderCreatePage_Max + 1 );

   ProfileBase *profile = mApplication->GetProfile();

   // create and add the pages
   (void)new wxOptionsPageCompose(this, profile);
   (void)new wxFolderPropertiesPage(this, profile);
}

// -----------------------------------------------------------------------------
// wxFolderSelectionDialog
// -----------------------------------------------------------------------------
wxFolderSelectionDialog::wxFolderSelectionDialog(wxWindow *parent,
                                                 MFolder *folder)
                       : wxDialog(GET_PARENT_OF_CLASS(parent, wxFrame), -1,
                                  _("Please choose a folder"),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL)
{
   SetAutoLayout(TRUE);
   m_folder = folder;

   // basic unit is the height of a char, from this we fix the sizes of all
   // other controls
   size_t heightLabel = AdjustCharHeight(GetCharHeight());
   int hBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       wBtn = BUTTON_WIDTH_FROM_HEIGHT(hBtn);

   // layout the controls: the folder tree takes all the dialog except for the
   // buttons below it
   wxLayoutConstraints *c;

   wxButton *btnOk = new wxButton(this, wxID_OK, _("OK"));
   btnOk->SetDefault();
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxRight, -2*(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
   btnOk->SetConstraints(c);

   wxButton *btnCancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxRight, -(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
   btnCancel->SetConstraints(c);

   m_tree = new wxFolderTree(this);
   c = new wxLayoutConstraints();
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(btnOk, wxTop, LAYOUT_Y_MARGIN);
   m_tree->GetWindow()->SetConstraints(c);

   // set the (initial) window size
   wxWindow::SetSize(4*wBtn, 10*hBtn);
   Centre(wxCENTER_FRAME | wxBOTH);
}

void wxFolderSelectionDialog::OnOK(wxCommandEvent& event)
{
   m_folder = m_tree->GetSelection();

   EndModal(TRUE);
}

void wxFolderSelectionDialog::OnCancel(wxCommandEvent& event)
{
   m_folder = NULL;

   EndModal(FALSE);
}

// -----------------------------------------------------------------------------
// our public interface
// -----------------------------------------------------------------------------

// helper function: common part of ShowFolderCreateDialog and
// ShowFolderPropertiesDialog
static MFolder *DoShowFolderDialog(wxFolderBaseDialog& dlg,
                                   FolderCreatePage page)
{
   dlg.CreateAllControls();
   dlg.SetNotebookPage(page);

   if ( dlg.ShowModal() )
      return dlg.GetFolder();
   else
      return (MFolder *)NULL;
}

MFolder *ShowFolderCreateDialog(wxWindow *parent,
                                FolderCreatePage page,
                                MFolder *parentFolder)
{
   wxFolderCreateDialog dlg(parent, parentFolder);

   return DoShowFolderDialog(dlg, page);
}

bool ShowFolderPropertiesDialog(MFolder *folder, wxWindow *parent)
{
   wxFolderPropertiesDialog dlg(parent, folder);

   return DoShowFolderDialog(dlg, FolderCreatePage_Folder) ? TRUE : FALSE;
}

MFolder *ShowFolderSelectionDialog(MFolder *folder, wxWindow *parent)
{
   wxFolderSelectionDialog dlg(parent, folder);

   (void)dlg.ShowModal();

   return dlg.GetFolder();
}
