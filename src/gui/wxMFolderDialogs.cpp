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

#   include <wx/wx.h>
#endif

#include <wx/dynarray.h>
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
   ~wxFolderBaseDialog()
   {
      SafeDecRef(m_newFolder);
      SafeDecRef(m_parentFolder);
   }

   // initialization (should be called before the dialog is shown)
      // set folder we're working with
   void SetFolder(MFolder *folder)
      { m_newFolder = folder; }
      // set the parent folder: if it's !NULL, it can't be changed by user
   void SetParentFolder(MFolder *parentFolder)
      { m_parentFolder = parentFolder; }

   // after the dialog is closed, retrieve the folder which was created or
   // NULL if the user cancelled us without creating anything
   MFolder *GetFolder() const { SafeIncRef(m_newFolder); return m_newFolder; }

   // override control creation functions
   virtual wxControl *CreateControlsAbove(wxPanel *panel);
   virtual void CreateNotebook(wxPanel *panel);

   // control ids
   enum
   {
      Folder_Name = 0x1000
   };

   // called by the pages
   void SetMayEnableOk(bool may)
   {
      m_mayEnableOk = may;

      DoUpdateButtons();
   }

   // set the folder name
   void SetFolderName(const String& name) { m_folderName->SetValue(name); }

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxFolderBaseDialog() { }

protected:
   // enable or disable [Ok] and [Apply] buttons
   void DoUpdateButtons()
      { EnableButtons(m_mayEnableOk && !m_folderName->GetValue().IsEmpty()); }

   wxTextCtrl  *m_folderName,
               *m_parentName;

   wxFolderBrowseButton *m_btnParentFolder;

   MFolder     *m_parentFolder,
               *m_newFolder;

   // FALSE if at least one of the pages contains incorrect information, if
   // it's TRUE it doesn't mean that the buttons are enabled - the global
   // dialog settings (folder name and parent) must be correct too
   bool m_mayEnableOk;

private:
   DECLARE_DYNAMIC_CLASS(wxFolderBaseDialog)
};

// folder creation dialog
//
// FIXME this dialog is also used for showing the folder properties, hence the
//       ugly hack with the 3rd parameter to the ctor (and which is passed
//       further to the pages too) - instead we should have a common base
//       class to the both kinds of notebooks and separate properties/creation
//       functionality for moving into derived classes
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

   // override control creation functions
   virtual void CreateNotebook(wxPanel *panel);

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
   wxFolderPropertiesPage(wxNotebook *notebook,
                          ProfileBase *profile,
                          wxFolderCreateDialog *dlg = NULL);

   // set the profile path to copy the default values from
   void SetDefaults(const String& profilePath)
      { m_defaultsPath = profilePath; }

   virtual bool TransferDataToWindow(void);
   virtual bool TransferDataFromWindow(void);

   /// update controls
   void UpdateUI(int selection = -1);
   void OnEvent(wxCommandEvent& event);
   void OnChange(wxKeyEvent& event);

protected:
   // fill the fields with the default value for the given folder type
   // (the current value of the type radiobox) either when the page is created
   // or when the type of the folder chosen changes
   void SetDefaultValues(bool firstTime = false);

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
   /// mailbox name for IMAP
   wxTextCtrl *m_mailboxname;
   // comment for the folder
   wxTextCtrl *m_comment;

   // the path to the profile section with the defautl values
   wxString m_defaultsPath;

   // the "create folder dialog" or NULL if we're showing folder properties
   wxFolderCreateDialog *m_dlgCreate;

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

   wxFolderCreateNotebook(wxWindow *parent,
                          wxFolderCreateDialog *dlg = NULL);
};

// folder selection dialog: contains the folder tree inside it
class wxFolderSelectionDialog : public wxDialog
{
public:
   wxFolderSelectionDialog(wxWindow *parent, MFolder *folder);
   ~wxFolderSelectionDialog() { SafeDecRef(m_folder); }

   // callbacks
   void OnOK(wxCommandEvent& event);
   void OnCancel(wxCommandEvent& event);

   // get the chosen folder
   MFolder *GetFolder() const { SafeIncRef(m_folder); return m_folder; }

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

IMPLEMENT_DYNAMIC_CLASS(wxFolderBaseDialog, wxNotebookDialog)
IMPLEMENT_DYNAMIC_CLASS(wxFolderCreateDialog, wxFolderBaseDialog)

BEGIN_EVENT_TABLE(wxFolderCreateDialog, wxNotebookDialog)
   EVT_TEXT(wxFolderCreateDialog::Folder_Name,
            wxFolderCreateDialog::OnFolderNameChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderPropertiesPage, wxNotebookPageBase)
   EVT_RADIOBOX(-1, wxFolderPropertiesPage::OnEvent)
   EVT_TEXT    (-1, wxFolderPropertiesPage::OnChange)
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
   m_mayEnableOk = false;
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

   if ( !m_parentFolder )
   {
      // take the root by default
      m_parentFolder = MFolder::Get("");
   }

   m_newFolder = m_parentFolder->CreateSubfolder(m_folderName->GetValue(),
                                                 typeFolder);

   return m_newFolder;
}

void wxFolderCreateDialog::OnFolderNameChange(wxEvent& event)
{
   SetDirty();

   DoUpdateButtons();

   event.Skip();
}

bool wxFolderCreateDialog::TransferDataToWindow()
{
   EnableButtons(FALSE);   // initially, we have no folder name

   // initialize our pages
   wxFolderPropertiesPage *page = (wxFolderPropertiesPage *)(m_notebook->
                                    GetPage(FolderCreatePage_Folder));

   // by default, take the same values as for the parent
   if ( m_parentFolder )
   {
      page->SetDefaults(m_parentFolder->GetFullName());
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
   wxString folderName = m_folderName->GetValue();

   // for the creation of a folder we don't use the toplevel config
   // section but create a profile of that name first
   ProfileBase *profile = ProfileBase::CreateProfile
                           (
                            folderName,
                            mApplication->GetProfile()
                           );

   bool ok = TRUE;

   // we must restore the path before the profile is released with DecRef(),
   // so take all this in a block
   {
       FolderPathChanger changePath(profile, folderName);
       profile->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);

       // tell the pages to use this profile instead of the global one
       ((wxOptionsPage *)m_notebook->GetPage(FolderCreatePage_Compose))
          ->SetProfile(profile);

       if ( wxNotebookDialog::TransferDataFromWindow() )
       {
           // do create the new folder
           CHECK( m_parentFolder, false, "should have created parent folder" );

           m_newFolder = m_parentFolder->GetSubfolder(m_folderName->GetValue());

           CHECK( m_newFolder, false, "new folder not created" );
       }
       else
       {
           ok = false;
       }
   }

   profile->DecRef();

   return ok;
}

void wxFolderCreateDialog::CreateNotebook(wxPanel *panel)
{
   m_notebook = new wxFolderCreateNotebook(panel, this);
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
   CHECK( m_newFolder, FALSE, "no folder in folder properties dialog" );

   // lame hack: use SetDefaults() so the page will read its data from the
   // profile section corresponding to our folder
   ((wxFolderPropertiesPage *)m_notebook->GetPage(FolderCreatePage_Folder))
      ->SetDefaults(m_newFolder->GetFullName());

   return wxFolderBaseDialog::TransferDataToWindow();
}

bool wxFolderPropertiesDialog::TransferDataFromWindow()
{
   CHECK( m_newFolder, FALSE, "no folder in folder properties dialog" );

   return wxFolderBaseDialog::TransferDataFromWindow();
}

// -----------------------------------------------------------------------------
// wxFolderPropertiesPage
// -----------------------------------------------------------------------------

wxFolderPropertiesPage::wxFolderPropertiesPage(wxNotebook *notebook,
                                               ProfileBase *profile,
                                               wxFolderCreateDialog *dlg)
                      : wxNotebookPageBase(notebook)
{
   // add us to the notebook
   int image = notebook->GetPageCount();
   notebook->AddPage(this, _("Access"), FALSE /* don't select */, image);

   m_dlgCreate = dlg;

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
   radioChoices[4] = _("News");

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
#ifdef __WXGTK__
   c->height.Absolute(40); // FIXME: AsIs() doesn't work for wxGTK
#else
   c->height.AsIs();
#endif
   m_radio->SetConstraints(c);

   // text entries
   enum
   {
      Label_Login,
      Label_Password,
      Label_Path,
      Label_Server,
      Label_Mailboxname,
      Label_Newsgroup,
      Label_Comment,
      Label_Max
   };

   static const char *szLabels[Label_Max] =
   {
      gettext_noop("User name: "),
      gettext_noop("Password: "),
      gettext_noop("File name: "),
      gettext_noop("Server: "),
      gettext_noop("Mailbox: "),
      gettext_noop("Newsgroup: "),
      gettext_noop("Comment: ")
   };

   wxArrayString labels;
   for ( size_t n = 0; n < Label_Max; n++ )
   {
      labels.Add(_(szLabels[n]));
   }

   // determine the longest label
   long widthMax = GetMaxLabelWidth(labels, this);

   m_login = CreateTextWithLabel(labels[Label_Login], widthMax, m_radio);
   m_password = CreateTextWithLabel(labels[Label_Password], widthMax, m_login,
                                    0, wxPASSWORD);
   m_server = CreateTextWithLabel(labels[Label_Server], widthMax, m_password);
   m_mailboxname = CreateTextWithLabel(labels[Label_Mailboxname], widthMax, m_server);
   m_newsgroup = CreateTextWithLabel(labels[Label_Newsgroup], widthMax, m_mailboxname);
   m_comment = CreateTextWithLabel(labels[Label_Comment], widthMax, m_newsgroup);
   m_path = CreateFileEntry(labels[Label_Path], widthMax, m_comment, &m_browsePath);

   // are we in "view properties" or "create" mode?
   m_isCreating = m_dlgCreate != NULL;
   m_radio->Enable(m_isCreating);

   UpdateUI();
}

void
wxFolderPropertiesPage::OnEvent(wxCommandEvent& event)
{
   int sel = -1;
   if ( event.GetEventObject() == m_radio )
   {
      // tell UpdateUI() about the new selection
      sel = event.GetInt();
   }

   UpdateUI(sel);
}

void
wxFolderPropertiesPage::OnChange(wxKeyEvent& event)
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);

   CHECK_RET( dlg, "folder page should be in folder dialog!" );

   dlg->SetDirty();

   // the rest doesn't make any sense for the "properties" dialog because the
   // text in the path field can't change anyhow
   if ( !m_isCreating )
      return;

   // if the path text changed, try to set the folder name
   wxObject *objEvent = event.GetEventObject();
   if ( objEvent == m_path || objEvent == m_newsgroup )
   {
      switch ( m_radio->GetSelection() )
      {
         case MFolder::File:
            // set the file name as the default folder name
            {
               wxString name;
               wxSplitPath(m_path->GetValue(), NULL, &name, NULL);

               dlg->SetFolderName(name);
            }
            break;

         case MFolder::News:
            // set the newsgroup name as the default folder name
            dlg->SetFolderName(m_newsgroup->GetValue());
            break;
      case MFolder::IMAP:
         break;
         default:
            // nothing
            ;
      }
   }
}

void
wxFolderPropertiesPage::UpdateUI(int sel)
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
   CHECK_RET( dlg, "wxFolderPropertiesPage without parent dialog?" );

   bool enableOk = true;

   static int s_selection = -1;

   int selection = sel == -1 ? m_radio->GetSelection() : sel;

   if ( selection != s_selection )
   {
      // clear all fields because some of them won't make sense for new
      // selection unless the selection didn't change
      m_newsgroup->SetValue("");
      m_path->SetValue("");
      m_login->SetValue("");
      m_password->SetValue("");
      m_server->SetValue("");
      m_mailboxname->SetValue("");
      s_selection = selection;
   }

   SetDefaultValues();

   switch ( selection )
   {
   case MailFolder::MF_IMAP:
      m_mailboxname->Enable(TRUE); //only difference from POP
   case MailFolder::MF_POP:
      m_login->Enable(TRUE);
      m_password->Enable(TRUE);
      m_server->Enable(TRUE);
      m_newsgroup->Enable(FALSE);
      m_browsePath->Enable(FALSE);
      break;

   case MailFolder::MF_NNTP:
      m_mailboxname->Enable(FALSE);
      m_login->Enable(FALSE);
      m_password->Enable(FALSE);
      m_server->Enable(TRUE);
      m_newsgroup->Enable(TRUE);
      m_browsePath->Enable(FALSE);
      break;

   case MailFolder::MF_FILE:
      m_mailboxname->Enable(FALSE);
      m_login->Enable(FALSE);
      m_password->Enable(FALSE);
      m_server->Enable(FALSE);
      m_newsgroup->Enable(FALSE);

      // this can not be changed for an already existing folder
      m_browsePath->Enable(TRUE & m_isCreating);
      break;

   case MailFolder::MF_INBOX:
      m_mailboxname->Enable(FALSE);
      m_login->Enable(FALSE);
      m_password->Enable(FALSE);
      m_server->Enable(FALSE);
      m_newsgroup->Enable(FALSE);

      m_browsePath->Enable(FALSE);
      if ( m_isCreating )
      {
         // INBOX folder can't be created by the user
         enableOk = false;
      }
      break;

   default:
      wxFAIL_MSG("Unexpected folder type.");
   }

   dlg->SetMayEnableOk(enableOk);
}

void
wxFolderPropertiesPage::SetDefaultValues(bool firstTime)
{
   // we want to read settings from the default section under
   // M_FOLDER_CONFIG_SECTION or from M_PROFILE_CONFIG_SECTION if the section
   // is empty (i.e. we have no parent folder)
   String profilePath = !m_defaultsPath ? String(M_PROFILE_CONFIG_SECTION)
                        : String(M_FOLDER_CONFIG_SECTION) + m_defaultsPath;
   ProfilePathChanger fpc(m_profile, profilePath);

   MFolder::Type typeFolder;
   if ( firstTime )
   {
      typeFolder = (MFolder::Type)READ_CONFIG(m_profile, MP_FOLDER_TYPE);
      if(typeFolder == MFolder::Invalid)
         typeFolder = MFolder::File;
      m_radio->SetSelection(typeFolder);
   }
   else
   {
      typeFolder = (MFolder::Type)m_radio->GetSelection();
   }

   if ( MFolder::TypeHasUserName(typeFolder) )
   {
      String value = READ_CONFIG(m_profile,MP_POP_LOGIN);
      if ( !value )
         value = READ_APPCONFIG(MP_USERNAME);
      m_login->SetValue(value);

      m_password->SetValue(READ_CONFIG(m_profile,MP_POP_PASSWORD));

      if ( typeFolder == MFolder::POP || typeFolder == MFolder::IMAP )
         value = READ_CONFIG(m_profile, MP_POP_HOST);
      else if ( typeFolder == MFolder::News )
         value = READ_CONFIG(m_profile, MP_NNTPHOST);
      else
         value.Empty();
      m_server->SetValue(value);
   }

   if ( typeFolder == MFolder::File && !m_isCreating )
      m_path->SetValue(READ_CONFIG(m_profile,MP_FOLDER_PATH));
   else if ( typeFolder == MFolder::News )
      m_newsgroup->SetValue(READ_CONFIG(m_profile,MP_FOLDER_PATH));

   if ( !m_isCreating )
      m_comment->SetValue(READ_CONFIG(m_profile, MP_FOLDER_COMMENT));
}

bool
wxFolderPropertiesPage::TransferDataToWindow(void)
{
   SetDefaultValues(true);

   return TRUE;
}

bool
wxFolderPropertiesPage::TransferDataFromWindow(void)
{
   // even though we propose the choice of INBOX it can't be created currently
   // FIXME may be should remove it from the radiobox then?
   MFolder::Type typeFolder = (MFolder::Type)m_radio->GetSelection();

   // ... but its properties (comment) may still be changed, so check for this
   // only if we're creating it
   CHECK( !m_dlgCreate || typeFolder != MFolder::Inbox, false,
          "Ok button should be disabled" );

   // 1st step: create the folder in the MFolder sense. For this we need only
   // the name and the type
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);

   CHECK( dlg, false, "folder page should be in folder dialog!" );

   // FIXME instead of this `if' we should have a virtual function in the
   //       base class to either create or return the folder object
   MFolder *folder;
   if ( m_dlgCreate )
   {
      ASSERT_MSG( m_dlgCreate == dlg, "GET_PARENT_OF_CLASS broken?" );

      folder = m_dlgCreate->DoCreateFolder(typeFolder);
      if ( !folder )
      {
         return false;
      }
   }
   else
   {
      folder = dlg->GetFolder();
   }

   CHECK( folder, false, "must have folder by this point" );

   // 2nd step: put what we can in MFolder
   folder->SetComment(m_comment->GetValue());

   // 3rd step: store additional data about the newly created folder directly
   // in the profile
   String fullname = folder->GetFullName();
   FolderPathChanger changePath(m_profile, fullname);
   switch ( typeFolder )
   {
      case MFolder::POP:
      case MFolder::IMAP:
         m_profile->writeEntry(MP_POP_LOGIN,m_login->GetValue());
         m_profile->writeEntry(MP_POP_PASSWORD,m_password->GetValue());
         m_profile->writeEntry(MP_POP_HOST,m_server->GetValue());
         m_profile->writeEntry(MP_FOLDER_PATH,m_mailboxname->GetValue());
         break;

      case MFolder::News:
         m_profile->writeEntry(MP_NNTPHOST,m_server->GetValue());
         m_profile->writeEntry(MP_FOLDER_PATH,m_newsgroup->GetValue());
         break;

      case MFolder::File:
         m_profile->writeEntry(MP_FOLDER_PATH,m_path->GetValue());
         break;

      case MFolder::Inbox:
         if ( !m_dlgCreate )
            break;
         //else: can't create INBOX folder!

      default:
         wxFAIL_MSG("Unexpected folder type.");
   }

   if ( m_dlgCreate )
   {
      // generate an event notifying everybody that a new folder has been
      // created
      EventFolderTreeChangeData event(fullname,
                                      EventFolderTreeChangeData::Create);
      EventManager::Send(event);
   }

   folder->DecRef();

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
wxFolderCreateNotebook::wxFolderCreateNotebook(wxWindow *parent,
                                               wxFolderCreateDialog *dlg)
                      : wxNotebookWithImages("FolderCreateNotebook",
                                             parent,
                                             s_aszImages)
{
   // don't forget to update both the array above and the enum!
   wxASSERT( WXSIZEOF(s_aszImages) == FolderCreatePage_Max + 1 );

   ProfileBase *profile = mApplication->GetProfile();

   // create and add the pages
   (void)new wxOptionsPageCompose(this, profile);
   (void)new wxFolderPropertiesPage(this, profile, dlg);
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

void wxFolderSelectionDialog::OnOK(wxCommandEvent& /* event */)
{
   m_folder = m_tree->GetSelection();

   EndModal(TRUE);
}

void wxFolderSelectionDialog::OnCancel(wxCommandEvent& /* event */)
{
   m_folder = NULL;

   EndModal(FALSE);
}

// -----------------------------------------------------------------------------
// our public interface
// -----------------------------------------------------------------------------

// helper function: common part of ShowFolderCreateDialog and
// ShowFolderPropertiesDialog
//
// the caller should DecRef() the returned pointer
static MFolder *DoShowFolderDialog(wxFolderBaseDialog& dlg,
                                   FolderCreatePage page)
{
   dlg.CreateAllControls();
   dlg.SetNotebookPage(page);

   if ( dlg.ShowModal() )
   {
      MFolder *folder = dlg.GetFolder();

      return folder;
   }
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

   MFolder *newfolder = DoShowFolderDialog(dlg, FolderCreatePage_Folder);
   if ( newfolder )
   {
      newfolder->DecRef();

      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

MFolder *ShowFolderSelectionDialog(MFolder *folder, wxWindow *parent)
{
   wxFolderSelectionDialog dlg(parent, folder);

   (void)dlg.ShowModal();

   MFolder *newfolder = dlg.GetFolder();

   return newfolder;
}
