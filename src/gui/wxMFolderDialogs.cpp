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
#   include "strutil.h"
#   include <wx/wx.h>
#endif

#include <wx/dynarray.h>
#include <wx/log.h>
#include <wx/imaglist.h>
#include <wx/notebook.h>
#include <wx/persctrl.h>

#include "MDialogs.h"
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

   MFolder *m_parentFolder,
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
   MFolder *DoCreateFolder(FolderType typeFolder);

   // callbacks
   void OnFolderNameChange(wxEvent&);

   // override control creation functions
   virtual void CreateNotebook(wxPanel *panel);

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxFolderCreateDialog() { wxFAIL_MSG("not reached"); }

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

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxFolderPropertiesDialog() { wxFAIL_MSG("not reached"); }

private:
   DECLARE_DYNAMIC_CLASS(wxFolderPropertiesDialog)
};

// A panel for defining a new Folder
class wxFolderPropertiesPage : public wxNotebookPageBase
{
public:
   wxFolderPropertiesPage(wxNotebook *notebook,
                          ProfileBase *profile,
                          wxFolderCreateDialog *dlg = NULL);

   ~wxFolderPropertiesPage() { m_profile->DecRef(); }

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
   // fill the fields with the default value for the given folder type (the
   // current value of the type radiobox) when the type of the folder chosen
   // changes (which happens when the page is created too)
   void SetDefaultValues();

   // write the entry into the profile but only if it has been changed: we
   // shouldn't write the unchanged entries because this breaks profile
   // inheritance (if we have nothing, we read parent's value)
   enum FolderProperty
   {
      Username,   // login name
      Password,   // password
      Server,     // server (either POP3, IMAP4 or NNTP)
      Path,       // path for file based folders, newsgroup for NEWS/NNTP
      MaxProperty
   };
   void WriteEntryIfChanged(FolderProperty entry, const wxString& value);

   // hack: this flag tells whether we're in process of creating the folder or
   // just showing the properties for it. Ultimately, it shouldn't be
   // necessary, but for now we use it to adjust our behaviour depending on
   // what we're doing
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
   /// comment for the folder
   wxTextCtrl *m_comment;
   /// Is incoming folder?
   wxCheckBox *m_isIncoming;
   /// Use anonymous access for this folder?
   wxCheckBox *m_isAnonymous;

   /// the path to the profile section with the defautl values
   wxString m_defaultsPath;

   /// the "create folder dialog" or NULL if we're showing folder properties
   wxFolderCreateDialog *m_dlgCreate;

   /// the array with the initial values of the settings for this folder
   wxString m_originalValues[MaxProperty];

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
IMPLEMENT_DYNAMIC_CLASS(wxFolderPropertiesDialog, wxFolderBaseDialog)

BEGIN_EVENT_TABLE(wxFolderCreateDialog, wxNotebookDialog)
   EVT_TEXT(wxFolderCreateDialog::Folder_Name,
            wxFolderCreateDialog::OnFolderNameChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderPropertiesPage, wxNotebookPageBase)
   EVT_RADIOBOX(-1, wxFolderPropertiesPage::OnEvent)
   EVT_CHECKBOX(-1, wxFolderPropertiesPage::OnEvent)
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

MFolder *wxFolderCreateDialog::DoCreateFolder(FolderType typeFolder)
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


   if ( m_newFolder )
   {
      // tell the other pages that we now have a folder (and hence a profile)
      ProfileBase *profile = ProfileBase::CreateProfile(m_newFolder->GetFullName());
      GET_COMPOSE_PAGE(m_notebook)->SetProfile(profile);
      GET_MSGVIEW_PAGE(m_notebook)->SetProfile(profile);

      profile->DecRef();
   }

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
   wxFolderPropertiesPage *page = GET_FOLDER_PAGE(m_notebook);

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

   bool ok = TRUE;

   if ( wxNotebookDialog::TransferDataFromWindow() )
   {
      CHECK( m_parentFolder, false, "should have created parent folder" );

      // new folder has already been created normally
      m_newFolder = m_parentFolder->GetSubfolder(m_folderName->GetValue());

      CHECK( m_newFolder, false, "new folder not created" );
   }
   else
      ok = false;

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

   wxString folderName = m_newFolder->GetFullName();
   ProfileBase *profile = ProfileBase::CreateProfile(folderName);

   // lame hack: use SetDefaults() so the page will read its data from the
   // profile section corresponding to our folder
   GET_FOLDER_PAGE(m_notebook)->SetDefaults(folderName);
   GET_COMPOSE_PAGE(m_notebook)->SetProfile(profile);
   GET_MSGVIEW_PAGE(m_notebook)->SetProfile(profile);

   profile->DecRef();

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
   m_profile->IncRef();

   wxLayoutConstraints *c;

   // create controls
   // ---------------

   // radiobox of folder type
   // The indices into this array must match the folder types MF_XXX
   wxString radioChoices[6];
   radioChoices[0] = _("INBOX");
   radioChoices[1] = _("File");
   radioChoices[2] = _("POP3");
   radioChoices[3] = _("IMAP");
   radioChoices[4] = _("NNTP");
   radioChoices[5] = _("Newsspool");

   m_radio = new wxRadioBox(this,-1,_("Folder Type"),
                            wxDefaultPosition,wxDefaultSize,
                            WXSIZEOF(radioChoices), radioChoices,
                            // this 5 is not WXSIZEOF(radioChoices), it's just
                            // the number of radiobuttons per line
                            6, wxRA_SPECIFY_COLS);

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
      Label_IsIncoming,
      Label_IsAnonymous,
      Label_Max
   };

   static const char *szLabels[Label_Max] =
   {
      gettext_noop("&User name: "),
      gettext_noop("&Password: "),
      gettext_noop("&File name: "),
      gettext_noop("&Server: "),
      gettext_noop("&Mailbox: "),
      gettext_noop("&Newsgroup: "),
      gettext_noop("&Comment: "),
      gettext_noop("C&ollect all mail from this folder: "),
      gettext_noop("&Anonymous access: ")
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
   m_isIncoming = CreateCheckBox(labels[Label_IsIncoming], widthMax, m_path);
   m_isAnonymous = CreateCheckBox(labels[Label_IsAnonymous], widthMax, m_isIncoming);

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
      case File:
         // set the file name as the default folder name
      {
         wxString name;
         wxSplitPath(m_path->GetValue(), NULL, &name, NULL);

         dlg->SetFolderName(name);
      }
      break;

      case News:
      case MF_NNTP:
         // set the newsgroup name as the default folder name
         dlg->SetFolderName(m_newsgroup->GetValue());
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

      if ( selection == MF_NNTP )
      {
         // although NNTP servers do support password-protected access, this
         // is so rare that anonymous is the default
         m_isAnonymous->SetValue(TRUE);
      }
      else
      {
         // by default it's off for other types of folders
         m_isAnonymous->SetValue(FALSE);
      }

      // set the defaults for this kind of folder
      SetDefaultValues();
   }

   // if it has user name, it has password as well
   bool hasPassword = FolderTypeHasUserName((FolderType)selection);

   m_isAnonymous->Enable(hasPassword);

   // only enable password and login fields if anonymous access is disabled
   EnableTextWithLabel(m_password, hasPassword && !m_isAnonymous->GetValue());
   EnableTextWithLabel(m_login, hasPassword && !m_isAnonymous->GetValue());

   switch ( selection )
   {
      case MF_IMAP:
         EnableTextWithLabel(m_mailboxname, TRUE); // only difference from POP
         // fall through

      case MF_POP:
         EnableTextWithLabel(m_server, TRUE);
         EnableTextWithLabel(m_newsgroup, FALSE);
         EnableTextWithButton(m_path, FALSE);
         break;

      case MF_NNTP:
      case MF_NEWS:
         EnableTextWithLabel(m_mailboxname, FALSE);
         EnableTextWithLabel(m_server, TRUE);
         EnableTextWithLabel(m_newsgroup, TRUE);
         EnableTextWithButton(m_path, FALSE);
         break;

      case MF_FILE:
         EnableTextWithLabel(m_mailboxname, FALSE);
         EnableTextWithLabel(m_server, FALSE);
         EnableTextWithLabel(m_newsgroup, FALSE);

         // this can not be changed for an already existing folder
         EnableTextWithButton(m_path, m_isCreating);
         break;

      case MF_INBOX:
         EnableTextWithLabel(m_mailboxname, FALSE);
         EnableTextWithLabel(m_server, FALSE);
         EnableTextWithLabel(m_newsgroup, FALSE);

         EnableTextWithButton(m_path, FALSE);
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
wxFolderPropertiesPage::WriteEntryIfChanged(FolderProperty property,
                                            const wxString& value)
{
   static const char *profileKeys[MaxProperty] =
   {
      MP_FOLDER_LOGIN,
      MP_FOLDER_PASSWORD,
      MP_FOLDER_HOST,
      MP_FOLDER_PATH
   };

   if ( value != m_originalValues[property] )
   {
      m_profile->writeEntry(profileKeys[property], value);
   }
   //else: it didn't change, don't write it back
}

void
wxFolderPropertiesPage::SetDefaultValues()
{
   // clear the initial values
   for ( size_t n = 0; n < WXSIZEOF(m_originalValues); n++ )
   {
      m_originalValues[n].Empty();
   }

   // we want to read settings from the default section under
   // M_FOLDER_CONFIG_SECTION or from M_PROFILE_CONFIG_SECTION if the section
   // is empty (i.e. we have no parent folder)

   Profile_obj profile("");
   if ( m_defaultsPath )
      profile->SetPath(m_defaultsPath);

   FolderType typeFolder = (FolderType)m_radio->GetSelection();

   String value;
   if ( FolderTypeHasUserName(typeFolder) )
   {
      value = READ_CONFIG(profile, MP_FOLDER_LOGIN);
      if ( !value )
         value = READ_APPCONFIG(MP_USERNAME);
      m_login->SetValue(value);
      m_originalValues[Username] = value;

      String pwd = strutil_decrypt(READ_CONFIG(profile, MP_FOLDER_PASSWORD));
      m_password->SetValue(pwd);
      m_originalValues[Password] = pwd;
   }

   if ( FolderTypeHasServer(typeFolder) )
   {
      value = READ_CONFIG(profile, MP_FOLDER_HOST);
      if ( !value )
      {
         if ( typeFolder == Nntp )
         {
            // take the global NNTP server setting for this
            value = READ_CONFIG(profile, MP_NNTPHOST);
         }
         else
         {
            // this host by default
            value = READ_CONFIG(profile, MP_HOSTNAME);
         }
      }

      m_server->SetValue(value);
      m_originalValues[Server] = value;
   }

   value = READ_CONFIG(profile, MP_FOLDER_PATH);
   if ( typeFolder == File && !m_isCreating )
   {
      m_path->SetValue(value);
   }
   else if ( typeFolder == News || typeFolder == Nntp )
   {
      m_newsgroup->SetValue(value);
   }

   m_originalValues[Path] = value;

   if ( !m_isCreating )
      m_comment->SetValue(READ_CONFIG(profile, MP_FOLDER_COMMENT));
}

bool
wxFolderPropertiesPage::TransferDataToWindow(void)
{
   Profile_obj profile("");
   if ( m_defaultsPath )
      profile->SetPath(m_defaultsPath);

   FolderType typeFolder = GetFolderType(READ_CONFIG(profile, MP_FOLDER_TYPE));
   if ( typeFolder == FolderInvalid )
      typeFolder = File;

   // this will also call SetDefaultValues()
   m_radio->SetSelection(typeFolder);

   return TRUE;
}

bool
wxFolderPropertiesPage::TransferDataFromWindow(void)
{
   // even though we propose the choice of INBOX it can't be created currently
   // FIXME may be should remove it from the radiobox then?
   FolderType typeFolder = (FolderType)m_radio->GetSelection();

   // ... but its properties (comment) may still be changed, so check for this
   // only if we're creating it
   CHECK( !m_dlgCreate || typeFolder != Inbox, false,
          "Ok button should be disabled" );

   // 0th step: verify if the settings are self-consistent

   // doesn't the folder by this name already exist?

   // folder flags
   int flags = 0;
   if(m_isIncoming->GetValue())
      flags |= MF_FLAGS_INCOMING;

   // check that we have the username/password
   String loginName = m_login->GetValue(),
          password = m_password->GetValue();
   bool hasUsername = FolderTypeHasUserName(typeFolder);
   if ( hasUsername )
   {
      // anonymous access?
      bool anonymous = m_isAnonymous->GetValue() || loginName == "anonymous";

      if ( anonymous )
         flags |= MF_FLAGS_ANON;
      else
      {
         wxString what,    // what did the user forget to specify
                  keyname; // for MDialog_YesNoDialog
         if ( !loginName )
         {
            what = _("login name");
            keyname = "AskLogin";
         }
         else if ( !m_password->GetValue() )
         {
            what = _("password");
            keyname = "AskPwd";
         }

         if ( !!what )
         {
            wxString msg;
            msg.Printf(_("You didn't specify the %s for this folder although it requires one.\n"
                         "\n"
                         "Would you like to do it now?"),
                       what.c_str());

            if ( keyname == "AskPwd" )
            {
               msg << _("\n\n"
                        "Notice that the password will be stored in your configuration with\n"
                        "very weak encryption. If you are concerned about security, leave it\n"
                        "empty and Mahogany will prompt you for it whenever needed.");
            }

            if ( MDialog_YesNoDialog(msg, this, MDIALOG_YESNOTITLE,
                                     true, keyname) )
            {
               return false;
            }
         }
      }
   }

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
   m_profile->DecRef();
   m_profile = ProfileBase::CreateProfile(fullname);

   // common for all folders
   if ( hasUsername && !(flags & MF_FLAGS_ANON) )
   {
      WriteEntryIfChanged(Username, loginName);
      WriteEntryIfChanged(Password, strutil_encrypt(password));
   }

   m_profile->writeEntry(MP_FOLDER_TYPE, typeFolder | flags);

   if ( FolderTypeHasServer(typeFolder) )
   {
      WriteEntryIfChanged(Server, m_server->GetValue());
   }

   switch ( typeFolder )
   {
      case POP:
      case IMAP:
         WriteEntryIfChanged(Path, m_mailboxname->GetValue());
         break;

      case Nntp:
      case News:
         WriteEntryIfChanged(Path, m_newsgroup->GetValue());
         break;

      case File:
         WriteEntryIfChanged(Path, m_path->GetValue());
         break;

      case Inbox:
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
      MEventFolderTreeChangeData event(fullname,
                                       MEventFolderTreeChangeData::Create);
      MEventManager::Send(event);
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
   "access",
   "compose",
   "msgview",
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

   ProfileBase *profile = ProfileBase::CreateProfile("");

   // create and add the pages
   (void)new wxFolderPropertiesPage(this, profile, dlg);
   (void)new wxOptionsPageCompose(this, profile);
   (void)new wxOptionsPageMessageView(this, profile);

   profile->DecRef();
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
