///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   wxMFolderDialogs.cpp - implementation of functions from
//              MFolderDialogs.h (ShowFolderSubfoldersDialog is in a separate
//              file wxSubfoldersDialog.cpp)
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
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "guidef.h"
#  include "strutil.h"

#  include <wx/dynarray.h>
#  include <wx/log.h>
#  include <wx/layout.h>
#  include <wx/stattext.h>
#  include <wx/radiobox.h>
#  include <wx/choice.h>
#endif

#include <wx/imaglist.h>
#include <wx/notebook.h>
#include "wx/persctrl.h"
#include <wx/statbmp.h>
#include <wx/tooltip.h>

#include "MDialogs.h"
#include "MFolderDialogs.h"
#include "MailFolder.h"
#include "MFolder.h"

#include "Mdefaults.h"
#include "MailCollector.h"

#include "MailFolderCC.h"        // for GetMHFolderName

#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxBrowseButton.h"
#include "gui/wxFolderTree.h"

// why is this conditional?
#define USE_LOCAL_CHECKBOX

// ============================================================================
// private classes
// ============================================================================

// -----------------------------------------------------------------------------
// dialog classes
// -----------------------------------------------------------------------------

// base class for folder creation and properties dialog
class wxFolderBaseDialog : public wxNotebookDialog
{
public:
   wxFolderBaseDialog(wxWindow *parent, const wxString& title);
   virtual ~wxFolderBaseDialog()
   {
      SafeDecRef(m_newFolder);
      SafeDecRef(m_parentFolder);
      SafeDecRef(m_profile);
   }

   // initialization (should be called before the dialog is shown)
   // set folder we're working with
   void SetFolder(MFolder *folder)
      { m_newFolder = folder; SafeIncRef(m_newFolder); }
   // set the parent folder: if it's !NULL, it can't be changed by user
   void SetParentFolder(MFolder *parentFolder)
      { m_parentFolder = parentFolder; }

   // get the parent folder name
   wxString GetParentFolderName() const { return m_parentName->GetValue(); }
   // get the folder name
   wxString GetFolderName() const { return m_folderName->GetValue(); }

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
   }

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxFolderBaseDialog() { }

protected:
   // return TRUE if the Ok/Apply buttons should be enabled (depending on the
   // state of the other controls)
   bool ShouldEnableOk() const;

   // base class pure virtual - return the profile we're working with
   virtual ProfileBase *GetProfile() const
   {
      if ( m_newFolder && !m_profile )
      {
         ((wxFolderBaseDialog *)this)->m_profile = // const_cast
            ProfileBase::CreateProfile(m_newFolder->GetFullName());
      }

      SafeIncRef(m_profile);

      // may be NULL if we're creaing the folder and it hasn't been created yet
      return m_profile;
   }

   // tell all notebook pages (except the first one) which profile we're
   // working with
   void SetPagesProfile(ProfileBase *profile);

   wxTextCtrl  *m_folderName,
               *m_parentName;

   wxFolderBrowseButton *m_btnParentFolder;

   MFolder *m_parentFolder,
           *m_newFolder;

   ProfileBase *m_profile;

   // FALSE if at least one of the pages contains incorrect information, if
   // it's TRUE it doesn't mean that the buttons are enabled - the global
   // dialog settings (folder name and parent) must be correct too
   bool m_mayEnableOk;

private:
   DECLARE_DYNAMIC_CLASS(wxFolderBaseDialog)
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

// folder creation dialog
class wxFolderCreateDialog : public wxFolderBaseDialog
{
public:
   wxFolderCreateDialog(wxWindow *parent,
                        MFolder *parentFolder);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // called by the page to create the new folder
   MFolder *DoCreateFolder(FolderType folderType);

   // set the folder name
   void SetFolderName(const String& name)
   {
      m_nameModifiedByUser = -1;
      m_folderName->SetValue(name);
   }

   // callbacks
   void OnFolderNameChange(wxCommandEvent& event);
   void OnUpdateButton(wxUpdateUIEvent& event);

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxFolderCreateDialog() { wxFAIL_MSG("not reached"); }

private:
   // set to TRUE if the user changed the folder name, FALSE otherwise and -1
   // if we're changing it programmatically
   int m_nameModifiedByUser;

   DECLARE_DYNAMIC_CLASS(wxFolderCreateDialog)
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// A notebook page containing the folder properties (access)
// ----------------------------------------------------------------------------

// HACK: the 3rd argument to the ctor tells us whether we're creating a new
//       folder (if it's !NULL) or just showing properties of an existing one.
//       Our UI behaves slightly differently in these 2 cases.
class wxFolderPropertiesPage : public wxNotebookPageBase
{
public:
   wxFolderPropertiesPage(wxNotebook *notebook,
                          ProfileBase *profile,
                          wxFolderCreateDialog *dlg = NULL);

   ~wxFolderPropertiesPage() { m_profile->DecRef(); }

   // are all the settings (more or less) correct?
   bool IsOk() const;

   // set the profile path to copy the default values from
   void SetFolderPath(const String& profilePath)
      { m_folderPath = profilePath; }

   virtual bool TransferDataToWindow(void);
   virtual bool TransferDataFromWindow(void);

   /// update controls after the current folder type changed
   void DoUpdateUI();

   /// update controls for the current folder type
   void DoUpdateUIForFolder();

   // handlers
   void OnChange(wxKeyEvent& event);
   void OnRadioBox(wxCommandEvent& WXUNUSED(event)) { OnEvent(); }
   void OnCheckBox(wxCommandEvent& WXUNUSED(event)) { OnEvent(); }
   void OnComboBox(wxCommandEvent& WXUNUSED(event)) { OnEvent(); }
   void OnChoice(wxCommandEvent& WXUNUSED(event)) { OnEvent(); }

   void UpdateOnFolderNameChange();

protected:
   // react to any event - update all the controls
   void OnEvent();

   // the radiobox indices
   enum RadioIndex
   {
      Radio_File,    // MBOX/MH/INBOX
      Radio_Pop,     // POP3
      Radio_Imap,    // IMAP4
      Radio_News,    // NNTP/NEWS
      Radio_Group,   // a folder group
      Radio_Max
   };

   // the indices of folder subtype combobox for different types
      // valid subtypes for Radio_File
   enum
   {
      FileFolderSubtype_Mbox,       // a standard MBOX style file folder
      FileFolderSubtype_MH,         // an MH directoryfolder
      FileFolderSubtype_Max
   };
      // valid subtypes for type Radio_News
   enum
   {
      NewsFolderSubtype_Nntp,       // NNTP group (remote)
      NewsFolderSubtype_News,       // News group (local newsspool)
      NewsFolderSubtype_Max
   };

   // fill the fields with the default value for the given folder type (the
   // current value of the type radiobox) when the type of the folder chosen
   // changes (which happens when the page is created too)
   void SetDefaultValues();

   // update the dialogs folder name
   void SetFolderName(const wxString& name);

   // write the entry into the profile but only if it has been changed: we
   // shouldn't write the unchanged entries because this breaks profile
   // inheritance (if we have nothing, we read parent's value)
   enum FolderProperty
   {
      Username,   // login name
      Password,   // password
      Server,     // server (for POP3 and IMAP4)
      ServerNews, // server (for NNTP)
      Path,       // path for file based folders, newsgroup for NEWS/NNTP
      MaxProperty
   };
   void WriteEntryIfChanged(FolderProperty entry, const wxString& value);

   // fills the subtype combobox with the available values for the
   // given radiobox selection
   void FillSubtypeCombo(RadioIndex sel);

   // clears the fields which don't make sense for the new selection
   void ClearInvalidFields(RadioIndex sel);

   // get the type of the folder chosen from the combination of the current
   // radiobox and combobox values or from the given one (if it's != Radio_Max)
   FolderType GetCurrentFolderType(RadioIndex selRadiobox = Radio_Max,
                                   int selChoice = -1) const;

   // inverse function of the above one: get the radiobox and choice indices
   // (if any) from the folder type
   RadioIndex GetRadioIndexFromFolderType(FolderType type,
                                          int *choiceIndex = NULL) const;

   // enable the controls which make sense for a NNTP/News folder
   void EnableControlsForNewsGroup(bool isNNTP = TRUE);

   // enable the controls which make sense for an POP or IMAP folder
   void EnableControlsForImapOrPop(bool isIMAP = TRUE);

   // hack: this flag tells whether we're in process of creating the folder or
   // just showing the properties for it. Ultimately, it shouldn't be
   // necessary, but for now we use it to adjust our behaviour depending on
   // what we're doing
   bool m_isCreating;

   /// the parent notebook control
   wxNotebook *m_notebook;

   /// the profile we use to read our settings from/write them to
   ProfileBase *m_profile;

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
   wxFileOrDirBrowseButton *m_browsePath;
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
   /// Keep it always open?
   wxCheckBox *m_keepOpen;
   /// Is folder hidden?
   wxCheckBox *m_isHidden;
   /// Force re-open on ping?
   wxCheckBox *m_forceReOpen;
   /// Use anonymous access for this folder?
   wxCheckBox *m_isAnonymous;
#ifdef USE_SSL
   /// Use SSL authentication for this folder?
   wxCheckBox *m_useSSL;
#endif

#ifdef USE_LOCAL_CHECKBOX
   /// Is folder local?
   wxCheckBox *m_isLocal;
#endif // USE_LOCAL_CHECKBOX
   /// Is this a directory (IMAP) or hierarchy (NNTP/News) and not a folder?
   wxCheckBox *m_isDir;
   /// The combobox for folder subtype
   wxChoice *m_folderSubtype;
   /// browse button for the icon
   wxIconBrowseButton *m_browseIcon;

   /// the path to the profile section with values for this folder
   wxString m_folderPath;

   /// the "create folder dialog" or NULL if we're showing folder properties
   wxFolderCreateDialog *m_dlgCreate;

   /// the array with the initial values of the settings for this folder
   wxString m_originalValues[MaxProperty];

   /// the initial value of the "anonymous" flag
   bool m_originalIsAnonymous;
#ifdef USE_SSL
   /// the initial value of the "use SSL" flag
   bool m_originalUseSSL;
#endif
/// the initial value of the "is incoming" flag
   bool m_originalIncomingValue;
   /// the initial value of the "keep open" flag
   bool m_originalKeepOpenValue;
   /// the initial value of the "force re-open" flag
   bool m_originalForceReOpenValue;
#ifdef USE_LOCAL_CHECKBOX
   /// the initial value of the "is local" flag
   bool m_originalIsLocalValue;
#endif // USE_LOCAL_CHECKBOX
   /// the initial value of the "hidden" flag
   bool m_originalIsHiddenValue;
   /// the initial value of the "is group" flag
   bool m_originalIsDir;

   /// the original value for the folder icon index (-1 if nothing special)
   int m_originalFolderIcon;

   /// the current folder type or MF_ILLEGAL if none
   FolderType m_folderType;

   // true if the user modified manually the filename, false otherwise and may
   // have temporary value of -1 if we modified it programmatically
   int m_userModifiedPath;

private:
   DECLARE_EVENT_TABLE()
};

// notebook for folder properties/creation: if the wxFolderCreateDialog pointer
// is NULL in the ctor, assume we're showing properties. Otherwise, we're
// creating a new folder.
class wxFolderCreateNotebook : public wxNotebookWithImages
{
public:
   // icon names
   static const char *s_aszImages[];
   static const char *s_aszImagesAdvanced[];

   wxFolderCreateNotebook(bool isAdvancedUser,
                          wxWindow *parent,
                          wxFolderCreateDialog *dlg = NULL);
};

// ----------------------------------------------------------------------------
// folder selection dialog: contains the folder tree inside it
// ----------------------------------------------------------------------------

static const int wxID_FILE_SELECT = 100;

class wxFolderSelectionDialog : public wxDialog
{
public:
   wxFolderSelectionDialog(wxWindow *parent,
                           MFolder *folder,
                           bool allowFiles = false);
   ~wxFolderSelectionDialog() { SafeDecRef(m_folder); }

   // what was selected, a real folder or a filename?
   bool IsFile() const { return !!m_filename; }

   // get the chosen folder
   MFolder *GetFolder() const
   {
      wxASSERT_MSG( !IsFile(), "file selected but retrieving folder" );

      SafeIncRef(m_folder);
      return m_folder;
   }

   // get the chosen file name
   wxString GetFile() const
   {
      wxASSERT_MSG( IsFile(), "folder selected but retrieving file" );

      return m_filename;
   }

   // callbacks
   void OnOK(wxCommandEvent& event);
   void OnCancel(wxCommandEvent& event);
   void OnFile(wxCommandEvent& event);

private:
   wxString      m_filename;  // if not empty, file was selected
   MFolder      *m_folder;
   wxFolderTree *m_tree;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// small utility class: a folder icon browse button which makes the dialog
// containing it dirty when the icon changes
// ----------------------------------------------------------------------------

class wxFolderIconBrowseButtonInDialog : public wxFolderIconBrowseButton
{
public:
   wxFolderIconBrowseButtonInDialog(wxFolderBaseDialog *dlg,
                                    wxWindow *parent,
                                    const wxString& tooltip)
      : wxFolderIconBrowseButton(parent, tooltip) { m_dlg = dlg; }

private:
   virtual void OnIconChange() { m_dlg->SetDirty(); }

   wxFolderBaseDialog *m_dlg;
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
   EVT_UPDATE_UI(wxID_OK,    wxFolderCreateDialog::OnUpdateButton)
   EVT_UPDATE_UI(wxID_APPLY, wxFolderCreateDialog::OnUpdateButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderPropertiesPage, wxNotebookPageBase)
   EVT_RADIOBOX(-1, wxFolderPropertiesPage::OnRadioBox)
   EVT_CHECKBOX(-1, wxFolderPropertiesPage::OnCheckBox)
   EVT_COMBOBOX(-1, wxFolderPropertiesPage::OnComboBox)
   EVT_CHOICE  (-1, wxFolderPropertiesPage::OnChoice)
   EVT_TEXT    (-1, wxFolderPropertiesPage::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderSelectionDialog, wxDialog)
   EVT_BUTTON(wxID_OK,     wxFolderSelectionDialog::OnOK)
   EVT_BUTTON(wxID_CANCEL, wxFolderSelectionDialog::OnCancel)
   EVT_BUTTON(wxID_FILE_SELECT, wxFolderSelectionDialog::OnFile)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderBaseDialog
// ----------------------------------------------------------------------------

wxFolderBaseDialog::wxFolderBaseDialog(wxWindow *parent,
                                       const wxString& title)
                  : wxNotebookDialog(GET_PARENT_OF_CLASS(parent, wxFrame),
                                     title,
                                     "FolderProperties")
{
   m_notebook = NULL;
   m_parentFolder = NULL;
   m_newFolder = NULL;
   m_mayEnableOk = false;
   m_profile = NULL;
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
   // TODO let the user select himself which pages he wants to see... It's not
   //      as simple as it seems because currently the image list must be known
   //      statically, so it involves changing wxOptionsPage code (currently,
   //      it adds itself with the icon corresponding to the position of the
   //      page in the notebook instead of the fixed number)
   bool isAdvancedUser = READ_APPCONFIG(MP_USERLEVEL) == M_USERLEVEL_ADVANCED;

   m_notebook = new wxFolderCreateNotebook
                    (
                     isAdvancedUser,
                     panel,
                     wxDynamicCast(this, wxFolderCreateDialog)
                    );
}

void wxFolderBaseDialog::SetPagesProfile(ProfileBase *profile)
{
   // we assume that the first page is the "Access" one which is a bit special,
   // this is why we start from the second one
   wxASSERT_MSG( FolderCreatePage_Folder == 0, "this code must be fixed" );

   size_t nPageCount = m_notebook->GetPageCount();
   for ( size_t nPage = 1; nPage < nPageCount; nPage++ )
   {
      GET_OPTIONS_PAGE(m_notebook, nPage)->SetProfile(profile);
   }
}

bool wxFolderBaseDialog::ShouldEnableOk() const
{
   // first of all, we should be able to enable it in principle
   bool enable = m_mayEnableOk;

   // then the folder name can't be empty
   if ( enable )
   {
      enable = !!m_folderName->GetValue();
   }

   // finally, depending on the folder type, the file name or the path
   // shouldn't be empty neither - ask the folder access page if everything is
   // ok
   if ( enable && m_notebook )
   {
      wxFolderPropertiesPage *page = GET_FOLDER_PAGE(m_notebook);
      enable = page->IsOk();
   }

   return enable;
}

// -----------------------------------------------------------------------------
// wxFolderCreateDialog
// -----------------------------------------------------------------------------

wxFolderCreateDialog::wxFolderCreateDialog(wxWindow *parent,
                                           MFolder *parentFolder)
   : wxFolderBaseDialog(parent, _("Create new folder"))
{
   SetParentFolder(parentFolder);

   m_nameModifiedByUser = false;
}

MFolder *wxFolderCreateDialog::DoCreateFolder(FolderType folderType)
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
                                                 folderType);


   if ( m_newFolder )
   {
      // we shouldn't have had it yet
      ASSERT_MSG( !m_profile, "folder just created, but profile exists?" );

      // tell the other pages that we now have a folder (and hence a profile)
      String folderName = m_newFolder->GetFullName();
      m_profile = ProfileBase::CreateProfile(folderName);

      SetPagesProfile(m_profile);
   }

   return m_newFolder;
}

void wxFolderCreateDialog::OnFolderNameChange(wxCommandEvent& event)
{
   SetDirty();

   if ( m_nameModifiedByUser != -1 )
   {
      // the user changed it, take note of it and notify the page too
      m_nameModifiedByUser = true;

      wxFolderPropertiesPage *page = GET_FOLDER_PAGE(m_notebook);
      page->UpdateOnFolderNameChange();

   }
   else
   {
      // it comes from a call to SetFolderName() called by the page itself, no
      // need to update it
      m_nameModifiedByUser = false;
   }

   event.Skip();
}

void wxFolderCreateDialog::OnUpdateButton(wxUpdateUIEvent& event)
{
   event.Enable( ShouldEnableOk() );
}

bool wxFolderCreateDialog::TransferDataToWindow()
{
   // this is one of rare cases where persistent controls are more painful than
   // useful - when we create the folder, we always want to start with the
   // "Access" page, but the persistent notebook restores thep age we used the
   // last time. We still do use it, because it's nice that "Properties" dialog
   // will open on the page where we were last when it's opened the next time -
   // just override this behaviour here by setting selection explicitly.
   m_notebook->SetSelection(FolderCreatePage_Folder);

   // initialize our pages
   wxFolderPropertiesPage *page = GET_FOLDER_PAGE(m_notebook);

   // by default, take the same values as for the parent
   if ( m_parentFolder )
   {
      page->SetFolderPath(m_parentFolder->GetFullName());
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
   bool ok = TRUE;

   wxString folderName = m_folderName->GetValue();

   // due to a cclient limitation, only 7bit ASCII chars may be used in the
   // folder names (notice that it's important to use unsigned chars here as
   // we compare them to 127!)
   unsigned char ch = 0;
   for ( const unsigned char *p = (const unsigned char *)folderName.c_str();
         *p;
         p++ )
   {
      // do *not* use isalpha() here because it returns TRUE for the second
      // half of ASCII table as well in some locales
      if ( *p > 127 )
      {
         ch = *p;
         break;
      }
   }

   if ( ch )
   {
      wxLogError(_("Sorry, international characters (like '%c') are "
                   "not supported in the mailbox folder names, please "
                   "don't use them."), ch);

      ok = FALSE;
   }

   if ( ok )
   {
      ok = wxNotebookDialog::TransferDataFromWindow();
   }

   if ( ok )
   {
      CHECK( m_parentFolder, false, "should have created parent folder" );

      // new folder has already been created normally
      m_newFolder = m_parentFolder->GetSubfolder(m_folderName->GetValue());

      CHECK( m_newFolder, false, "new folder not created" );
   }

   return ok;
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
   ProfileBase *profile = GetProfile();

   // use SetFolderPath() so the page will read its data from the profile
   // section corresponding to our folder
   GET_FOLDER_PAGE(m_notebook)->SetFolderPath(folderName);

   SetPagesProfile(profile);

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
   int image = FolderCreatePage_Folder;
   notebook->AddPage(this, _("Access"), FALSE /* don't select */, image);

   // are we in "view properties" or "create" mode?
   m_dlgCreate = dlg;
   m_isCreating = m_dlgCreate != NULL;

   // init members
   // ------------
   m_notebook = notebook;
   m_folderType = MF_ILLEGAL;
   m_userModifiedPath = false;
   m_profile = profile;
   m_profile->IncRef();

   wxLayoutConstraints *c;

   // create controls
   // ---------------

   // radiobox of folder types
   wxString radioChoices[Radio_Max];
   radioChoices[Radio_File]  = _("File");
   radioChoices[Radio_Pop]   = _("POP3");
   radioChoices[Radio_Imap]  = _("IMAP");
   radioChoices[Radio_News]  = _("News");
   radioChoices[Radio_Group] = _("Group");

   m_radio = new wxRadioBox(GetCanvas(), -1, _("Folder &Type"),
                            wxDefaultPosition,wxDefaultSize,
                            Radio_Max, radioChoices,
                            // create a horizontal radio box
                            Radio_Max, wxRA_SPECIFY_COLS);

   c = new wxLayoutConstraints();
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(GetCanvas(), wxTop, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();
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
      Label_KeepOpen,
      Label_ForceReOpen,
      Label_IsAnonymous,
#ifdef USE_SSL
      Label_UseSSL,
#endif
#ifdef USE_LOCAL_CHECKBOX
      Label_IsLocal,
#endif // USE_LOCAL_CHECKBOX
      Label_IsHidden,
      Label_IsDir,
      Label_FolderSubtype,
      Label_FolderIcon,
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
      gettext_noop("&Keep folder always open: "),
      gettext_noop("Force &re-open on ping: "),
      gettext_noop("Anon&ymous access: "),
#ifdef USE_SSL
      gettext_noop("Use &Secure Sockets Layer (SSL): "),
#endif
#ifdef USE_LOCAL_CHECKBOX
      gettext_noop("Folder can be accessed &without network "),
#endif // USE_LOCAL_CHECKBOX
      gettext_noop("&Hide folder in tree"),
      gettext_noop("Is &directory: "),
      gettext_noop("Folder sub&type "),
      gettext_noop("&Icon for this folder: "),
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
   m_path = CreateFileOrDirEntry(labels[Label_Path], widthMax,
                                 m_comment, NULL,
                                 TRUE,    // open
                                 FALSE);  // allow non existing files

   m_isIncoming = CreateCheckBox(labels[Label_IsIncoming], widthMax, m_path);
   m_keepOpen = CreateCheckBox(labels[Label_KeepOpen], widthMax, m_isIncoming);
   m_forceReOpen = CreateCheckBox(labels[Label_ForceReOpen], widthMax, m_keepOpen);
   m_isAnonymous = CreateCheckBox(labels[Label_IsAnonymous], widthMax,
                                  m_forceReOpen);
   wxControl *lastCtrl = m_isAnonymous;
#ifdef USE_SSL
   m_useSSL = CreateCheckBox(labels[Label_UseSSL], widthMax,
                             lastCtrl);
   lastCtrl = m_useSSL;
#endif
#ifdef USE_LOCAL_CHECKBOX
   m_isLocal = CreateCheckBox(labels[Label_IsLocal], widthMax, lastCtrl);
   lastCtrl = m_isLocal;
#endif // USE_LOCAL_CHECKBOX/!USE_LOCAL_CHECKBOX
   m_isDir = CreateCheckBox(labels[Label_IsDir], widthMax, lastCtrl);
   m_isHidden = CreateCheckBox(labels[Label_IsHidden], widthMax, m_isDir);
   m_folderSubtype = CreateChoice(labels[Label_FolderSubtype], widthMax, m_isHidden);

   // the checkboxes might not be very clear, so add some explanations in the
   // form of tooltips
   m_forceReOpen->SetToolTip(_("Tick this box if Mahogany appears to have "
                               "problems updating the folder listing.\n"
                               "This is needed for some broken POP3 servers.\n"
                               "Normally this is not needed."));
   m_isAnonymous->SetToolTip(_("For the types of folders which require user "
                               "name and password check this to try to connect\n"
                               "without any - this might work or fail depending "
                               "on the server settings."));
#ifdef USE_SSL
   m_useSSL->SetToolTip(_("This will use SSL authentication and encryption\n"
                          "for communication with the server."));
#endif
   m_isDir->SetToolTip(_("Some folders are like directories and may only "
                         "contain other folders and not the messages.\n"
                         "Check this to create a folder of this kind."));

   wxFolderBaseDialog *dlgParent = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
   ASSERT_MSG( dlgParent, "should have a parent dialog!" );

   m_browseIcon = new wxFolderIconBrowseButtonInDialog(dlgParent,
                                                       GetCanvas(),
                                                       _("Choose folder icon"));

   (void)CreateIconEntry(labels[Label_FolderIcon], widthMax, m_folderSubtype, m_browseIcon);

   m_radio->Enable(m_isCreating);
}

void
wxFolderPropertiesPage::SetFolderName(const wxString& name)
{
   wxFolderCreateDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderCreateDialog);

   CHECK_RET( dlg, "SetFolderName() can be only called when creating" );

   dlg->SetFolderName(name);
}

void
wxFolderPropertiesPage::OnEvent()
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);

   CHECK_RET( dlg, "folder page should be in folder dialog!" );

   dlg->SetDirty();

   // folder might have changed, so call DoUpdateUI() which will just call
   // DoUpdateUIForFolder() if this isn't the case
   DoUpdateUI();
}

void
wxFolderPropertiesPage::OnChange(wxKeyEvent& event)
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);

   CHECK_RET( dlg, "folder page should be in folder dialog!" );

   wxObject *objEvent = event.GetEventObject();

   dlg->SetDirty();

   // the rest doesn't make any sense for the "properties" dialog because the
   // text in the path field can't change anyhow
   if ( !m_isCreating )
      return;

   // does the event come from the user or from ourselves?
   if ( m_userModifiedPath == -1 )
   {
      // from the program, nothing to do
      m_userModifiedPath = false;

      return;
   }

   // if the path text changed, try to set the folder name automatically
   switch ( GetCurrentFolderType() )
   {
      case MF_FILE:
      case MF_MH:
         // set the file name as the default folder name
         if ( objEvent == m_path )
         {
            wxString name, fullname = m_path->GetValue();
            wxSplitPath(fullname, NULL, &name, NULL);

            if ( !fullname )
            {
               // path has become empty (again), so allow setting it
               // automatically from the folder name
               m_userModifiedPath = false;
            }
            else
            {
               // this change has been done by the user, don't override the
               // value
               m_userModifiedPath = true;
            }

            SetFolderName(name);
         }
         break;

      case MF_IMAP:
         if ( objEvent == m_mailboxname )
         {
            SetFolderName(m_mailboxname->GetValue().AfterLast('/'));
         }
         break;

      case MF_NEWS:
      case MF_NNTP:
         // set the newsgroup name as the default folder name
         if ( objEvent == m_newsgroup )
         {
            SetFolderName(m_newsgroup->GetValue());
         }
         break;

      default:
         // nothing
         ;
   }
}

void
wxFolderPropertiesPage::UpdateOnFolderNameChange()
{
   // if the path wasn't set by the user, set it to correspond to the folder
   // name
   if ( !m_userModifiedPath )
   {
      FolderType folderType = GetCurrentFolderType();
      if ( folderType == MF_FILE || folderType == MF_MH )
      {
         // ... try to set it from the folder name
         wxFolderCreateDialog *dlg =
            GET_PARENT_OF_CLASS(this, wxFolderCreateDialog);

         CHECK_RET( dlg, "we should be only called when creating" );

         // modify the text even if the folder name is empty because
         // otherwise entering a character into "Folder name" field and
         // erasing it wouldn't restore the original "Path" value
         wxString folderName;

         // MH folder should be created under its parent by default
         if ( folderType == MF_MH )
         {
            // AfterFirst() removes the MH root prefix
            folderName << dlg->GetParentFolderName().AfterFirst('/') << '/';
         }
         //else: MBOX folders don't have hierarchical structure

         folderName += dlg->GetFolderName();

         // this will tell SetValue() that we modified it ourselves, not the
         // user
         m_userModifiedPath = -1;

         m_path->SetValue(strutil_expandfoldername(folderName, folderType));
      }
   }
}

// enable the controls which make sense for a NNTP or News folder
void
wxFolderPropertiesPage::EnableControlsForNewsGroup(bool isNNTP)
{
   EnableTextWithLabel(m_mailboxname, FALSE);
   EnableTextWithLabel(m_server, TRUE);
   EnableTextWithLabel(m_newsgroup, TRUE);
   EnableTextWithButton(m_path, FALSE);
   m_forceReOpen->Enable(isNNTP);

#ifdef USE_LOCAL_CHECKBOX
   m_isLocal->Enable(isNNTP);
#endif // USE_LOCAL_CHECKBOX

   // "directory" here means a news hierarchy
   m_isDir->Enable(TRUE);
}

// enable the controls which make sense for an IMAP folder
void
wxFolderPropertiesPage::EnableControlsForImapOrPop(bool isIMAP)
{
   EnableTextWithLabel(m_mailboxname, isIMAP);
   EnableTextWithLabel(m_server, TRUE);
   EnableTextWithLabel(m_newsgroup, FALSE);
   EnableTextWithButton(m_path, FALSE);
   m_forceReOpen->Enable(TRUE);

#ifdef USE_LOCAL_CHECKBOX
   m_isLocal->Enable(TRUE);
#endif // USE_LOCAL_CHECKBOX

   // POP3 doesn't have directories, IMAP4 does
   m_isDir->Enable(isIMAP);
}

// called when radiobox/choice selection changes
void
wxFolderPropertiesPage::DoUpdateUI()
{
   // get the current folder type from dialog controls
   RadioIndex selRadio = (RadioIndex)m_radio->GetSelection();
   FolderType folderType = GetCurrentFolderType(selRadio);

   if ( folderType != m_folderType )
   {
      // depending on the folder type we may have subtypes
      RadioIndex selRadioOld = GetRadioIndexFromFolderType(m_folderType);
      if ( selRadioOld != selRadio )
      {
         FillSubtypeCombo(selRadio);
         ClearInvalidFields(selRadio);
      }
      //else: the subtype choice contents didn't change

      m_folderType = folderType;

      if ( folderType == MF_GROUP )
      {
         // explain to the user the "special" meaning of the fields in this
         // case (it may be helpful - and if the user finds it annoying, he may
         // always disable it)
         MDialog_Message(_(
            "All the fields shown in this dialog have a slightly different\n"
            "meaning for the folders of type \"Group\". Instead of applying\n"
            "to this folder (which wouldn't make sense, as such folders don't\n"
            "contain any mail messages at all, but only other folders) these\n"
            "values will be used as defaults for all folders created under\n"
            "this group folder."),
                         this, _("Group folders hint"), "FolderGroupHint");
      }

      // set the defaults for this kind of folder
      SetDefaultValues();
   }

   DoUpdateUIForFolder();
}

// this function only updates the controls for the current folder, it never
// changes m_folderType or radiobox/choice selections
void
wxFolderPropertiesPage::DoUpdateUIForFolder()
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
   CHECK_RET( dlg, "wxFolderPropertiesPage without parent dialog?" );

   bool enableAnonymous, enableLogin;
#ifdef USE_SSL
   bool enableSSL;
#endif

   if ( m_folderType == MF_GROUP )
   {
      // enable all fields for these folders (see the message above)
      enableAnonymous = enableLogin = true;
#ifdef USE_SSL
      enableSSL = false;
#endif
   }
   else
   {
      // if it has user name, it has password as well
      bool hasPassword = FolderTypeHasUserName(m_folderType);

      // and if it has password, there must be anon access too
      enableAnonymous = hasPassword;

      // only enable password and login fields if anonymous access is disabled
      bool isAnon = m_isAnonymous->GetValue();
      enableLogin = hasPassword && !isAnon;

#ifdef USE_SSL
      enableSSL = FolderTypeSupportsSSL(m_folderType);
#endif

   }

#ifdef USE_SSL
   m_useSSL->Enable(enableSSL);
#endif
   m_isAnonymous->Enable(enableAnonymous);
   EnableTextWithLabel(m_password, enableLogin);
   EnableTextWithLabel(m_login, enableLogin);

   switch ( m_folderType )
   {
      case MF_IMAP:
      case MF_POP:
         EnableControlsForImapOrPop(m_folderType == MF_IMAP);
         break;

      case MF_NNTP:
      case MF_NEWS:
         EnableControlsForNewsGroup(m_folderType == MF_NNTP);
         break;

      case MF_FILE:
      case MF_MH:
         EnableTextWithLabel(m_mailboxname, FALSE);
         EnableTextWithLabel(m_server, FALSE);
         EnableTextWithLabel(m_newsgroup, FALSE);

         // this can not be changed for an already existing folder
         EnableTextWithButton(m_path, m_isCreating);
         m_forceReOpen->SetValue(FALSE);
         m_forceReOpen->Enable(FALSE);

#ifdef USE_LOCAL_CHECKBOX
         m_isLocal->Enable(FALSE);
         m_isLocal->SetValue(FALSE);
#endif // USE_LOCAL_CHECKBOX

         // file folders come in several flavours
         switch ( m_folderType )
         {
            case MF_FILE:
               m_browsePath->BrowseForFiles();
               m_folderSubtype->SetSelection(FileFolderSubtype_Mbox);
               m_isDir->Enable(FALSE);
               break;

            case MF_MH:
               {
                  m_browsePath->BrowseForDirectories();
                  m_folderSubtype->SetSelection(FileFolderSubtype_MH);

                  MFolder_obj folderParent(dlg->GetParentFolderName());
                  Profile_obj profile(folderParent->GetFullName());

                  wxString path;
                  path << MailFolderCC::InitializeMH()
                     << READ_CONFIG(profile, MP_FOLDER_PATH);
                  if ( !!path && !wxIsPathSeparator(path.Last()) )
                     path << '/';
                  path << dlg->GetFolderName();

                  m_path->SetValue(path);

                  m_isDir->Enable(TRUE);
               }
               break;

            default:
               FAIL_MSG( "new file folder type?" );
         }

         // not yet
         m_userModifiedPath = false;

         break;

      case MF_GROUP:
         // for a simple grouping folder, all fields make sense because
         // they will be inherited by the children and the children
         // folders may have any type
         EnableTextWithLabel(m_mailboxname, TRUE);
         EnableTextWithLabel(m_server, TRUE);
         EnableTextWithLabel(m_newsgroup, TRUE);
         EnableTextWithButton(m_path, TRUE);
         m_forceReOpen->Enable(TRUE);
         m_isDir->Enable(TRUE);
         break;

      default:
         wxFAIL_MSG("Unexpected folder type.");
   }

   // enable folder subtype combobox only if we're creating because (folder
   // type can't be changed later) and if there are any subtypes
   EnableControlWithLabel(m_folderSubtype,
                          m_isCreating && (m_folderSubtype->Number() > 0));

   dlg->SetMayEnableOk(TRUE);
}

void
wxFolderPropertiesPage::FillSubtypeCombo(RadioIndex sel)
{
   // do it first anyhow - it might have contained something before
   m_folderSubtype->Clear();

   switch ( sel )
   {
      case Radio_File:
         // NB: the strings should be in sync with FileFolderSubtype_XXX enum
         m_folderSubtype->Append(_("MBOX folder"));
         m_folderSubtype->Append(_("MH folder"));
         break;

      case Radio_News:
         // NB: the strings should be in sync with NewsFolderSubtype_XXX enum
         m_folderSubtype->Append(_("NNTP group (remote server)"));
         m_folderSubtype->Append(_("News group (local newsspool)"));
         break;

      default:
         // nothing to do - leave the wxChoice empty and return, skipping
         // SetSelection() below
         return;
   }

   m_folderSubtype->SetSelection(0);
}

FolderType
wxFolderPropertiesPage::GetCurrentFolderType(RadioIndex selRadio,
                                             int selChoice) const
{
   if ( selRadio == Radio_Max )
   {
      selRadio = (RadioIndex)m_radio->GetSelection();
   }

   if ( selChoice == -1 )
   {
      selChoice = m_folderSubtype->GetSelection();

      // still no selection? pretend that it's the first choice item (which
      // should be the default one)
      if ( selChoice == -1 )
      {
         selChoice = 0;
      }
   }

   switch ( selRadio )
   {
      case Radio_File:
         // we never return MF_INBOX but this is ok because we'll never create
         // one anyhow
         switch ( selChoice )
         {
            default:
               FAIL_MSG("invalid file folder subtype");
               // fall through

            case FileFolderSubtype_Mbox:
               return MF_FILE;

            case FileFolderSubtype_MH:
               return MF_MH;
         }

      case Radio_Pop:
         return MF_POP;

      case Radio_Imap:
         return MF_IMAP;

      case Radio_News:
         switch ( selChoice )
         {
            default:
               FAIL_MSG("invalid news folder subtype");
               // fall through

            case NewsFolderSubtype_Nntp:
               return MF_NNTP;

            case NewsFolderSubtype_News:
               return MF_NEWS;
         }

      case Radio_Group:
         return MF_GROUP;

      case Radio_Max:
      default:
         FAIL_MSG("invalid folder radio box selection");
         return MF_ILLEGAL;
   }
}

wxFolderPropertiesPage::RadioIndex
wxFolderPropertiesPage::GetRadioIndexFromFolderType(FolderType type,
                                                    int *choiceIndex) const
{
   if ( choiceIndex )
   {
      // by default, no subtype
      *choiceIndex = -1;
   }

   switch ( type )
   {
      case MF_INBOX:
      case MF_FILE:
      case MF_MH:
         if ( choiceIndex )
         {
            *choiceIndex = type == MF_MH ? FileFolderSubtype_MH
                                         : FileFolderSubtype_Mbox;
         }
         return Radio_File;

      case MF_POP:
         return Radio_Pop;

      case MF_IMAP:
         return Radio_Imap;

      case MF_NNTP:
      case MF_NEWS:
         if ( choiceIndex )
         {
            *choiceIndex = type == MF_NNTP ? NewsFolderSubtype_Nntp
                                           : NewsFolderSubtype_News;
         }
         return Radio_News;

      case MF_GROUP:
         return Radio_Group;

      default:
         FAIL_MSG("unexpected folder type value");
         return Radio_Max;
   }
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
      MP_NNTPHOST,
      MP_FOLDER_PATH,
   };

   if ( value != m_originalValues[property] )
   {
      m_profile->writeEntry(profileKeys[property], value);

      if ( !m_isCreating )
      {
         // this function has a side effect: it also sets the "modified" flag
         // so that the other functions know that the folder settings have
         // been changed and so the "unaccessible" flag may be not valid any
         // longer
         MFolder_obj folder(m_folderPath);

         folder->AddFlags(MF_FLAGS_MODIFIED);
      }
      //else: this flag is unnecessary for just created folder
   }
   //else: it didn't change, don't write it back
}

void
wxFolderPropertiesPage::ClearInvalidFields(RadioIndex sel)
{
   // clear the fields which don't make sense for new selection

   // all fields make sense for groups
   if ( sel == Radio_Group )
      return;

   if ( sel != Radio_News )
   {
      // this is only for news
      m_newsgroup->SetValue("");
   }

   if ( sel != Radio_File )
   {
      // this is only for files
      m_path->SetValue("");
   }

   if ( sel == Radio_File )
   {
      // this is for everything except local folders
      m_server->SetValue("");
      m_login->SetValue("");
      m_password->SetValue("");
   }

   if ( sel != Radio_Imap )
   {
      // this is only for IMAP
      m_mailboxname->SetValue("");
   }

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
   profile->SetPath(m_folderPath);

   wxLogDebug("Reading the folder settings from '%s'...", m_folderPath.c_str());

   RadioIndex selRadio = (RadioIndex)m_radio->GetSelection();
   FolderType folderType = GetCurrentFolderType(selRadio);

   String value;
   if ( FolderTypeHasUserName(folderType) )
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

   if ( FolderTypeHasServer(folderType) )
   {
      value = profile->readEntry(MP_FOLDER_HOST, "");
      if ( !value )
      {
         // take the global server setting for this protocol
         switch ( folderType )
         {
            case MF_NNTP:
               value = READ_CONFIG(profile, MP_NNTPHOST);
               break;

            case MF_POP:
               value = READ_CONFIG(profile, MP_POPHOST);
               break;

            case MF_IMAP:
               value = READ_CONFIG(profile, MP_IMAPHOST);
               break;

            default:
               // suppress warnings
               break;
         }
      }

      if ( !value )
      {
         // set to this host by default
         value = READ_CONFIG(profile, MP_HOSTNAME);
      }

      m_server->SetValue(value);
      m_originalValues[Server] = value;
   }

   value = READ_CONFIG(profile, MP_FOLDER_PATH);
   if ( (selRadio == Radio_File) && !m_isCreating )
   {
      m_path->SetValue(value);
   }
   else if ( selRadio == Radio_News )
   {
      m_newsgroup->SetValue(value);
   }
   else if ( selRadio == Radio_Imap )
   {
      m_mailboxname->SetValue(value);
   }

   m_originalValues[Path] = value;

   if ( !m_isCreating )
      m_comment->SetValue(READ_CONFIG(profile, MP_FOLDER_COMMENT));

   // set the initial values for all checkboxes and remember them: we will only
   // write it back if it changes later
   int flags = GetFolderFlags(READ_CONFIG(profile, MP_FOLDER_TYPE));
   m_originalIncomingValue = (flags & MF_FLAGS_INCOMING) != 0;
   m_originalIsHiddenValue = (flags & MF_FLAGS_HIDDEN) != 0;
   m_isIncoming->SetValue(m_originalIncomingValue);

   m_originalKeepOpenValue = (flags & MF_FLAGS_KEEPOPEN) != 0;
   m_keepOpen->SetValue(m_originalKeepOpenValue);
   m_originalForceReOpenValue = (flags & MF_FLAGS_KEEPOPEN) != 0;
   m_forceReOpen->SetValue(m_originalForceReOpenValue);

#ifdef USE_LOCAL_CHECKBOX
   m_originalIsLocalValue = (flags & MF_FLAGS_ISLOCAL) != 0;
   m_isLocal->SetValue(m_originalIsLocalValue);
#endif // USE_LOCAL_CHECKBOX

   m_originalIsDir = (flags & MF_FLAGS_GROUP) != 0;
   m_isHidden->SetValue(m_originalIsHiddenValue);
   m_isDir->SetValue(m_originalIsDir);

   // although NNTP servers do support password-protected access, this
   // is so rare that anonymous should really be the default
   m_originalIsAnonymous = (flags & MF_FLAGS_ANON) ||
                           (m_isCreating && folderType == MF_NNTP);
   m_isAnonymous->SetValue(m_originalIsAnonymous);

#ifdef USE_SSL
   m_originalUseSSL = ((flags & MF_FLAGS_SSLAUTH) != 0);
   m_useSSL->SetValue(m_originalUseSSL);
#endif

   // update the folder icon
   if ( m_isCreating )
   {
      // use default icon for the chosen folder type
      m_originalFolderIcon = GetDefaultFolderTypeIcon(folderType);
   }
   else
   {
      // use the folders icon
      MFolder_obj folder(m_folderPath);
      m_originalFolderIcon = GetFolderIconForDisplay(folder);
   }

   m_browseIcon->SetIcon(m_originalFolderIcon);
}

bool
wxFolderPropertiesPage::IsOk() const
{
   switch ( GetCurrentFolderType() )
   {
      case Nntp:
      case News:
         // it's valid to have an empty name for the newsgroup hierarchy - the
         // entry will represent the whole news server - but not for a
         // newsgroup
         return m_isDir->GetValue() || !!m_newsgroup->GetValue();

      case File:
         // file should have a non empty name
         return !!m_path->GetValue();

      default:
         // nothing to check for the other types
         return TRUE;
   }
}

bool
wxFolderPropertiesPage::TransferDataToWindow(void)
{
   Profile_obj profile("");
   if ( !!m_folderPath )
      profile->SetPath(m_folderPath);

   m_folderType = GetFolderType(READ_CONFIG(profile, MP_FOLDER_TYPE));
   if ( m_folderType == FolderInvalid )
   {
      // this may only happen if we're creating the folder
      ASSERT_MSG( m_isCreating, "invalid folder type" );

      m_folderType = (FolderType)READ_APPCONFIG(MP_LAST_CREATED_FOLDER_TYPE);
   }

   if ( (m_folderType == MF_INBOX) && m_isCreating )
   {
      // FAIL_MSG("how did we manage to create an INBOX folder?"); --
      // obviously by using a corrupted config file... no need to crash though

      m_folderType = (FolderType)MP_LAST_CREATED_FOLDER_TYPE_D;
   }

   if ( m_folderType == MF_INBOX )
   {
      // we don't have any special properties for INBOX, so just treat it as
      // file folder
      m_folderType = MF_FILE;
   }

   // init the dialog controls
   int selChoice;
   RadioIndex selRadio = GetRadioIndexFromFolderType(m_folderType, &selChoice);
   m_radio->SetSelection(selRadio);
   FillSubtypeCombo(selRadio);
   if ( selChoice != -1 )
   {
      m_folderSubtype->SetSelection(selChoice);
   }

   if ( m_isCreating && (selRadio == Radio_File) )
   {
      // set the default value for m_path field
      UpdateOnFolderNameChange();
   }

   SetDefaultValues();

   // now let it set the correct state of all controls
   DoUpdateUIForFolder();

   return TRUE;
}

bool
wxFolderPropertiesPage::TransferDataFromWindow(void)
{
   FolderType folderType = GetCurrentFolderType();

   // it's impossible to create INBOX folder
   CHECK( !m_dlgCreate || folderType != MF_INBOX, false,
          "Ok button should be disabled" );

   // 0th step: verify if the settings are self-consistent
   {
       wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
       if (folderType == MF_FILE && dlg->GetFolderName() == "INBOX")
           folderType = MF_INBOX;
   }

   // is the folder name valid?
   wxString path;
   if ( folderType == MF_MH )
   {
      // MH folder name is always relative to the MH root path
      path = m_path->GetValue();
      if ( !MailFolderCC::GetMHFolderName(&path) )
      {
         wxLogError(_("Impossible to create MH folder '%s'."),
                    path.c_str());

         wxLog::FlushActive();

         return FALSE;
      }
   }

   // folder flags
   int flags = 0;
   if ( m_isIncoming->GetValue() )
      flags |= MF_FLAGS_INCOMING;
   if ( m_keepOpen->GetValue() )
      flags |= MF_FLAGS_KEEPOPEN;
   if ( m_forceReOpen->GetValue() )
      flags |= MF_FLAGS_REOPENONPING;
   if ( m_isDir->GetValue() )
      flags |= MF_FLAGS_GROUP;
   if ( m_isHidden->GetValue() )
      flags |= MF_FLAGS_HIDDEN;

#ifdef USE_LOCAL_CHECKBOX
   if ( m_isLocal->GetValue() )
      flags |= MF_FLAGS_ISLOCAL;
#endif // USE_LOCAL_CHECKBOX

   // check that we have the username/password
   String loginName = m_login->GetValue(),
          password = m_password->GetValue();

   // For normal folders, we make sure that a password is specified if
   // needed:
   bool hasUsername = FolderTypeHasUserName(folderType);
   if( folderType != MF_GROUP )
   {
      if ( hasUsername )
      {
         // anonymous access?
         bool anonymous = m_isAnonymous->GetValue() || loginName == "anonymous";
#ifdef USE_SSL
         if(m_useSSL->GetValue() != 0)
            flags |= MF_FLAGS_SSLAUTH;
#endif
         if ( anonymous )
            flags |= MF_FLAGS_ANON;
         else
         {
            wxString what,    // what did the user forget to specify
            keyname; // for MDialog_YesNoDialog
            if ( !loginName )
            {
               what = _("a login name");
               keyname = "AskLogin";
            }
            else if ( !m_password->GetValue() )
            {
               what = _("a password");
               keyname = "AskPwd";
            }

            if ( !!what )
            {
               wxString msg;
               msg.Printf(_("You have not specified %s for this folder, although it requires one.\n"
                        "Alternatively, you might want to select anonymous access.\n"
                        "Would you like to do change this now?"),
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

      folder = m_dlgCreate->DoCreateFolder(folderType);
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

   // common for all folders: remember the login info for the folders for which
   // it makes sense or for folder groups (for which we always remember
   // everything)
   if ( (hasUsername && !(flags & MF_FLAGS_ANON)) || folderType == MF_GROUP )
   {
      WriteEntryIfChanged(Username, loginName);
      WriteEntryIfChanged(Password, strutil_encrypt(password));
   }

   m_profile->writeEntry(MP_FOLDER_TYPE, folderType | flags);

   String server = m_server->GetValue();
   if ( FolderTypeHasServer(folderType) )
   {
      FolderProperty serverType;
      if ( folderType == MF_NNTP )
      {
         serverType = ServerNews;
      }
      else
      {
         serverType = Server;
      }

      WriteEntryIfChanged(serverType, server);
   }
   else if ( folderType == MF_GROUP )
   {
      // the right thing to do is to write the server value into both profile
      // entries: for POP/IMAP server and for NNTP one because like this
      // everybody will inherit it
      WriteEntryIfChanged(ServerNews, server);
      WriteEntryIfChanged(Server, server);
   }

   switch ( folderType )
   {
      case MF_POP:
      case MF_IMAP:
         WriteEntryIfChanged(Path, m_mailboxname->GetValue());
         break;

      case MF_NNTP:
      case MF_NEWS:
         WriteEntryIfChanged(Path, m_newsgroup->GetValue());
         break;

      case MF_FILE:
         path = m_path->GetValue();
         // fall through

      case MF_MH:
         // for MH path had been set in thevery beginning
         WriteEntryIfChanged(Path, path);
         break;

      case MF_INBOX:
         if ( !m_dlgCreate )
            break;
         //else: can't create INBOX folder!

      case MF_GROUP:
         WriteEntryIfChanged(Path, m_mailboxname->GetValue());
         WriteEntryIfChanged(Path, m_newsgroup->GetValue());
         WriteEntryIfChanged(Path, m_path->GetValue());
         break;

      default:
         wxFAIL_MSG("Unexpected folder type.");
   }

   // mark the folder as being autocollectable or not
   bool isIncoming = m_isIncoming->GetValue();
   if ( m_originalIncomingValue != isIncoming )
   {
      MailCollector *collector = mApplication->GetMailCollector();

      if ( collector )
      {
         if ( isIncoming )
         {
            collector->AddIncomingFolder(fullname);

            folder->AddFlags(MF_FLAGS_INCOMING);
         }
         else
         {
            collector->RemoveIncomingFolder(fullname);

            folder->ResetFlags(MF_FLAGS_INCOMING);
         }
      }
      else
      {
         wxFAIL_MSG("can't set the isIncoming setting: no mail collector");
      }
   }

   bool isKeepOpen = m_keepOpen->GetValue();
   if ( m_originalKeepOpenValue != isKeepOpen )
   {
      if ( isKeepOpen )
         mApplication->AddKeepOpenFolder(fullname);
      else
      {
         VERIFY(mApplication->RemoveKeepOpenFolder(fullname),
                "failed to reset 'keep open' property");
      }
   }
   //else: nothing changed, nothing to do

   bool isAnonymous = m_isAnonymous->GetValue();
   if ( isAnonymous != m_originalIsAnonymous )
   {
      if ( isAnonymous )
         folder->AddFlags(MF_FLAGS_ANON);
      else
         folder->ResetFlags(MF_FLAGS_ANON);
   }

#ifdef USE_SSL
   bool useSSL = m_useSSL->GetValue();
   if ( useSSL != m_originalUseSSL )
   {
      if ( useSSL )
         folder->AddFlags(MF_FLAGS_SSLAUTH);
      else
         folder->ResetFlags(MF_FLAGS_SSLAUTH);
   }
#endif

   bool isDir = m_isDir->GetValue();
   if ( isDir != m_originalIsDir )
   {
      if ( isDir )
         folder->AddFlags(MF_FLAGS_GROUP);
      else
         folder->ResetFlags(MF_FLAGS_GROUP);
   }

   int folderIcon = m_browseIcon->GetIconIndex();
   if ( folderIcon != m_originalFolderIcon )
   {
      folder->SetIcon(folderIcon);
   }

   if ( m_dlgCreate )
   {
      // generate an event notifying everybody that a new folder has been
      // created
      MEventManager::Send(
         new MEventFolderTreeChangeData(fullname,
                                        MEventFolderTreeChangeData::Create)
         );
      MEventManager::DispatchPending();
   }

   folder->DecRef();

   // remember the type of this folder - will be the default one the next time
   mApplication->GetProfile()->writeEntry(MP_LAST_CREATED_FOLDER_TYPE, folderType);

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

const char *wxFolderCreateNotebook::s_aszImagesAdvanced[] =
{
   "access",
   "ident",
   "network",
   "compose",
   "msgview",
   "helpers",
   NULL
};

// create the control and add pages too
wxFolderCreateNotebook::wxFolderCreateNotebook(bool isAdvancedUser,
                                               wxWindow *parent,
                                               wxFolderCreateDialog *dlg)
                      : wxNotebookWithImages
                        (
                         "FolderCreateNotebook",
                         parent,
                         isAdvancedUser ? s_aszImagesAdvanced : s_aszImages
                        )
{
   // don't forget to update both the array above and the enum!
   wxASSERT( WXSIZEOF(s_aszImages) == FolderCreatePage_Max + 1 );

   ProfileBase *profile = ProfileBase::CreateProfile("");

   // create and add the pages: some are always present, others are only shown
   // to "advanced" users (because they're not generally useful and may confuse
   // the novices)
   (void)new wxFolderPropertiesPage(this, profile, dlg);
   if ( isAdvancedUser )
   {
      (void)new wxOptionsPageIdent(this, profile);
      (void)new wxOptionsPageNetwork(this, profile);
   }
   (void)new wxOptionsPageCompose(this, profile);
   (void)new wxOptionsPageMessageView(this, profile);
   if ( isAdvancedUser )
   {
      (void)new wxOptionsPageHelpers(this, profile);
   }

   profile->DecRef();
}

// -----------------------------------------------------------------------------
// wxFolderSelectionDialog
// -----------------------------------------------------------------------------

wxFolderSelectionDialog::wxFolderSelectionDialog(wxWindow *parent,
                                                 MFolder *folder,
                                                 bool allowFiles)
                       : wxDialog(GET_PARENT_OF_CLASS(parent, wxFrame), -1,
                                  _("Please choose a folder"),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxDEFAULT_DIALOG_STYLE |
                                  wxRESIZE_BORDER |
                                  wxDIALOG_MODAL)
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

   if ( allowFiles )
   {
      wxButton *btnFile = new wxButton(this, wxID_FILE_SELECT, _("File..."));
      c = new wxLayoutConstraints;
      c->left.SameAs(this, wxRight, -3*(LAYOUT_X_MARGIN + wBtn));
      c->width.Absolute(wBtn);
      c->height.Absolute(hBtn);
      c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
      btnFile->SetConstraints(c);
   }

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

   // set the initial and minimal window size
   int wDlg = 4*wBtn,
       hDlg = 10*hBtn;

   if ( allowFiles )
   {
      wDlg += 2*wBtn;
   }

   wxWindow::SetSize(wDlg, hDlg);
   SetSizeHints(wDlg, hDlg);
   Centre(wxCENTER_FRAME | wxBOTH);
}

void wxFolderSelectionDialog::OnFile(wxCommandEvent& /* event */)
{
   wxFileDialog dialog(this, "", "", "",
                       _(wxALL_FILES),
                       wxHIDE_READONLY | wxFILE_MUST_EXIST);

   if ( dialog.ShowModal() == wxID_OK )
   {
      m_filename = dialog.GetPath();

      EndModal(TRUE);
   }
}

void wxFolderSelectionDialog::OnOK(wxCommandEvent& /* event */)
{
   m_folder = m_tree->GetSelection();

   EndModal(TRUE);
}

void wxFolderSelectionDialog::OnCancel(wxCommandEvent& /* event */)
{
   m_filename.Empty();
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

   MFolder *folderNew = DoShowFolderDialog(dlg, FolderCreatePage_Default);
   if ( folderNew != NULL )
   {
      // what else can it return?
      ASSERT_MSG( folderNew == folder, "unexpected folder change" );

      // as DoShowFolderDialog() calls GetFolder() which calls IncRef() on
      // folder, compensate for it here
      folderNew->DecRef();

      return TRUE;
   }
   else
   {
      // dialog was cancelled
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
