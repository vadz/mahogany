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
#endif

#include <wx/dynarray.h>
#include <wx/log.h>
#include <wx/imaglist.h>
#include <wx/notebook.h>
#include <wx/persctrl.h>
#include <wx/statbmp.h>
#include <wx/tooltip.h>
#include <wx/layout.h>
#include <wx/stattext.h>
#include <wx/radiobox.h>

#include "MDialogs.h"
#include "MFolderDialogs.h"
#include "MailFolder.h"
#include "MFolder.h"

#include "Mdefaults.h"
#include "MailCollector.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxBrowseButton.h"
#include "gui/wxFolderTree.h"

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
   }

   // initialization (should be called before the dialog is shown)
   // set folder we're working with
   void SetFolder(MFolder *folder)
      { m_newFolder = folder; SafeIncRef(m_newFolder); }
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

   // base class pure virtual - return the profile we're working with
   virtual ProfileBase *GetProfile() const
   {
      return m_newFolder ? ProfileBase::CreateProfile(m_newFolder->GetFullName())
                         : (ProfileBase *)NULL;
   }

   // tell all notebook pages (except the first one) which profile we're
   // working with
   void SetProfile(ProfileBase *profile);

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
   MFolder *DoCreateFolder(FolderType typeFolder);

   // callbacks
   void OnFolderNameChange(wxEvent&);

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxFolderCreateDialog() { wxFAIL_MSG("not reached"); }

private:
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

   // set the profile path to copy the default values from
   void SetFolderPath(const String& profilePath)
      { m_folderPath = profilePath; }

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
      Server,     // server (for POP3 and IMAP4)
      ServerNews, // server (for NNTP)
      Path,       // path for file based folders, newsgroup for NEWS/NNTP
      MaxProperty
   };
   void WriteEntryIfChanged(FolderProperty entry, const wxString& value);

   // get the type of the folder chosen from the current radiobox value or from
   // the given one (which is supposed to be the radiobox selection)
   FolderType GetCurrentFolderType(int sel = -1) const;

   // inverse function of the above one: get the radiobox index from the folder
   // type
   int GetRadioIndexFromFolderType(FolderType type) const;

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
   /// Keep it always open?
   wxCheckBox *m_keepOpen;
   /// Force re-open on ping?
   wxCheckBox *m_forceReOpen;
   /// Use anonymous access for this folder?
   wxCheckBox *m_isAnonymous;
   /// browse button for the icon
   wxIconBrowseButton *m_browseIcon;

   /// the path to the profile section with values for this folder
   wxString m_folderPath;

   /// the "create folder dialog" or NULL if we're showing folder properties
   wxFolderCreateDialog *m_dlgCreate;

   /// the array with the initial values of the settings for this folder
   wxString m_originalValues[MaxProperty];

   /// the initial value of the "is incoming" flag
   bool m_originalIncomingValue;
   /// the initial value of the "keep open" flag
   bool m_originalKeepOpenValue;
   /// the initial value of the "force re-open" flag
   bool m_originalForceReOpenValue;

   /// the original value for the folder icon index (-1 if nothing special)
   int m_originalFolderIcon;

   /// the current folder type or -1 if none
   int m_folderType;

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
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderPropertiesPage, wxNotebookPageBase)
   EVT_RADIOBOX(-1, wxFolderPropertiesPage::OnEvent)
   EVT_CHECKBOX(-1, wxFolderPropertiesPage::OnEvent)
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

void wxFolderBaseDialog::SetProfile(ProfileBase *profile)
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
      String folderName = m_newFolder->GetFullName();
      ProfileBase *profile = ProfileBase::CreateProfile(folderName);

      SetProfile(profile);

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

   SetProfile(profile);

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
   m_folderType = -1;
   m_profile = profile;
   m_profile->IncRef();

   wxLayoutConstraints *c;

   // create controls
   // ---------------

   // radiobox of folder types

   // NB: the indices into this array must match the folder types MF_XXX
   //     except for the last entry ("Group") and that if we're creating a
   //     folder (and not just viewing its properties), we don't allow INBOX
   //     type thus shifting all values by 1.
   wxString radioChoices[7];
   size_t n = 0;
   if ( !m_isCreating )
      radioChoices[n++] = _("INBOX");
   radioChoices[n++] = _("File");
   radioChoices[n++] = _("POP3");
   radioChoices[n++] = _("IMAP");
   radioChoices[n++] = _("NNTP");
   radioChoices[n++] = _("News"); // don't call this newsspool because under
                                  // Windows all items in a radiobox have the
                                  // same width and this one is much wider than
                                  // all the others
   radioChoices[n++] = _("Group");

   m_radio = new wxRadioBox(GetCanvas(), -1, _("Folder Type"),
                            wxDefaultPosition,wxDefaultSize,
                            n, radioChoices,
                            // create a horizontal radio box
                            n, wxRA_SPECIFY_COLS);

   c = new wxLayoutConstraints();
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(GetCanvas(), wxTop, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN);
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
      Label_KeepOpen,
      Label_ForceReOpen,
      Label_IsAnonymous,
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
      gettext_noop("&Force re-open on ping: "),
      gettext_noop("&Anonymous access: "),
      gettext_noop("Icon for this folder: "),
   };

   wxArrayString labels;
   for ( n = 0; n < Label_Max; n++ )
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
   m_keepOpen = CreateCheckBox(labels[Label_KeepOpen], widthMax, m_isIncoming);
   m_forceReOpen = CreateCheckBox(labels[Label_ForceReOpen], widthMax, m_keepOpen);
   m_isAnonymous = CreateCheckBox(labels[Label_IsAnonymous], widthMax, m_forceReOpen);


   m_forceReOpen->SetToolTip(_("Tick this box if Mahogany appears to have "
                               "problems updating the folder listing.\n"
                               "This is needed for some broken POP3 servers.\n"
                               "Normally this is not needed."));
   
   wxFolderBaseDialog *dlgParent = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
   ASSERT_MSG( dlgParent, "should have a parent dialog!" );

   m_browseIcon = new wxFolderIconBrowseButtonInDialog(
                                                       dlgParent,
                                                       GetCanvas(),
                                                       _("Choose folder icon")
                                                      );
   (void)CreateIconEntry(labels[Label_FolderIcon], widthMax, m_isAnonymous, m_browseIcon);

   m_radio->Enable(m_isCreating);
}

void
wxFolderPropertiesPage::OnEvent(wxCommandEvent& event)
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);

   CHECK_RET( dlg, "folder page should be in folder dialog!" );

   dlg->SetDirty();

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
      switch ( GetCurrentFolderType() )
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
         case Nntp:
            // set the newsgroup name as the default folder name
            dlg->SetFolderName(m_newsgroup->GetValue());
            break;

         default:
            // nothing
            ;
      }
   }
}

// our argument here is the radiobox selection
void
wxFolderPropertiesPage::UpdateUI(int sel)
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
   CHECK_RET( dlg, "wxFolderPropertiesPage without parent dialog?" );

   FolderType folderType = GetCurrentFolderType(sel);

   if ( folderType != m_folderType )
   {
      // clear all fields because some of them won't make sense for new
      // selection unless the selection didn't change
      m_newsgroup->SetValue("");
      m_path->SetValue("");
      m_login->SetValue("");
      m_password->SetValue("");
      m_server->SetValue("");
      m_mailboxname->SetValue("");
      m_folderType = folderType;

      if ( folderType == MF_NNTP )
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

      if ( folderType == FolderGroup )
      {
         // explain to the user the "special" meaning of the fields in this
         // case (it may be helpful - and if the user finds it annoying, he may
         // always disable it)
         MDialog_Message(_(
            "All the fields shown in this dialog have a slightly different\n"
            "meaning for the folders of type \"Group\". Instead of applying\n"
            "to this folder (which wouldn't make sense, as such folders don't\n"
            "contain any mail messages at all, but only other folders, these\n"
            "values will be used as defaults for all folders created under\n"
            "this group folder."
                         ), this, _("Group folders hint"), "FolderGroupHint");
      }

      // set the defaults for this kind of folder
      SetDefaultValues();
   }

   bool enableAnonymous, enableLogin;

   if ( folderType == FolderGroup )
   {
      // enable all fields for these folders (see the message above)
      enableAnonymous =
      enableLogin = true;
   }
   else
   {
      // if it has user name, it has password as well
      bool hasPassword = FolderTypeHasUserName(folderType);

      // and if it has password, there must be anon access too
      enableAnonymous = hasPassword;

      // only enable password and login fields if anonymous access is disabled
      bool isAnon = m_isAnonymous->GetValue();
      enableLogin = hasPassword && !isAnon;
   }

   m_isAnonymous->Enable(enableAnonymous);
   EnableTextWithLabel(m_password, enableLogin);
   EnableTextWithLabel(m_login, enableLogin);

   switch ( folderType )
   {
      case MF_IMAP:
         EnableTextWithLabel(m_mailboxname, TRUE); // only difference from POP
         m_forceReOpen->Enable(TRUE);
         // fall through

      case MF_POP:
         EnableTextWithLabel(m_server, TRUE);
         EnableTextWithLabel(m_newsgroup, FALSE);
         EnableTextWithButton(m_path, FALSE);
         m_forceReOpen->Enable(TRUE);
         break;

      case MF_NNTP:
      case MF_NEWS:
         EnableTextWithLabel(m_mailboxname, FALSE);
         EnableTextWithLabel(m_server, TRUE);
         EnableTextWithLabel(m_newsgroup, TRUE);
         EnableTextWithButton(m_path, FALSE);
         m_forceReOpen->Enable( folderType == MF_NNTP );
         break;

      case MF_FILE:
         EnableTextWithLabel(m_mailboxname, FALSE);
         EnableTextWithLabel(m_server, FALSE);
         EnableTextWithLabel(m_newsgroup, FALSE);

         // this can not be changed for an already existing folder
         EnableTextWithButton(m_path, m_isCreating);
         m_forceReOpen->Enable(FALSE);
         break;

      case MF_INBOX:
         EnableTextWithLabel(m_mailboxname, FALSE);
         EnableTextWithLabel(m_server, FALSE);
         EnableTextWithLabel(m_newsgroup, FALSE);

         EnableTextWithButton(m_path, FALSE);
         m_forceReOpen->Enable(FALSE);
         break;

      case FolderGroup:
         EnableTextWithLabel(m_mailboxname, TRUE);
         EnableTextWithLabel(m_server, TRUE);
         EnableTextWithLabel(m_newsgroup, TRUE);
         EnableTextWithButton(m_path, TRUE);
         m_forceReOpen->Enable(TRUE);
         break;

      default:
         wxFAIL_MSG("Unexpected folder type.");
   }

   dlg->SetMayEnableOk(TRUE);
}

FolderType
wxFolderPropertiesPage::GetCurrentFolderType(int sel) const
{
   if ( sel == -1 )
   {
      sel = m_radio->GetSelection();
   }

   if ( m_isCreating )
   {
      // no entry 0 (INBOX) in this case
      sel++;
   }

   switch ( sel )
   {
      case Inbox:
      case File:
      case POP:
      case IMAP:
      case Nntp:
      case News:
         return (FolderType)sel;

      case -1:
         return FolderInvalid;

      default:
         return FolderGroup;
   }
}

int
wxFolderPropertiesPage::GetRadioIndexFromFolderType(FolderType type) const
{
   switch ( type )
   {
      case Inbox:
         ASSERT_MSG( !m_isCreating, "can't set radiobox selection to INBOX" );
         // still fall through - this will result in returning -1

      case File:
      case POP:
      case IMAP:
      case Nntp:
      case News:
         // -1 because when we're creating folder, there is no INBOX entry
         return m_isCreating ? type - 1 : type;

      case FolderGroup:
         // assume it follows "News"
         return m_isCreating ? News : News + 1;

      case FolderInvalid:
      default:
         FAIL_MSG("invalid folder type in GetRadioIndexFromFolderType");

         return -1;
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

   FolderType typeFolder = GetCurrentFolderType();

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
      value = profile->readEntry(MP_FOLDER_HOST, "");
      if ( !value )
      {
         // take the global server setting for this protocol
         switch ( typeFolder )
         {
            case Nntp:
               value = READ_CONFIG(profile, MP_NNTPHOST);
               break;

            case POP:
               value = READ_CONFIG(profile, MP_POPHOST);
               break;

            case IMAP:
               value = READ_CONFIG(profile, MP_IMAPHOST);
               break;
         default:
            ; // suppress warnings
            break;}
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
   if ( typeFolder == File && !m_isCreating )
   {
      m_path->SetValue(value);
   }
   else if ( typeFolder == News || typeFolder == Nntp )
   {
      m_newsgroup->SetValue(value);
   }
   else if (typeFolder == IMAP)
      m_mailboxname->SetValue(value);

   m_originalValues[Path] = value;

   if ( !m_isCreating )
      m_comment->SetValue(READ_CONFIG(profile, MP_FOLDER_COMMENT));

   // set the incoming value and remember it: we will only write it back if it
   // changes later
   int flags = GetFolderFlags(READ_CONFIG(profile, MP_FOLDER_TYPE));
   m_originalIncomingValue = (flags & MF_FLAGS_INCOMING) != 0;
   m_isIncoming->SetValue(m_originalIncomingValue);

   // set the keepOpen value and remember it: we will only write it back if it
   // changes later
   m_originalKeepOpenValue = (flags & MF_FLAGS_KEEPOPEN) != 0;
   m_keepOpen->SetValue(m_originalKeepOpenValue);
   m_originalForceReOpenValue = (flags & MF_FLAGS_KEEPOPEN) != 0;
   m_forceReOpen->SetValue(m_originalForceReOpenValue);
   
   // update the folder icon
   if ( m_isCreating )
   {
      // use default icon for the chosen folder type
      m_originalFolderIcon = GetDefaultFolderTypeIcon(typeFolder);
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
wxFolderPropertiesPage::TransferDataToWindow(void)
{
   Profile_obj profile("");
   if ( m_folderPath )
      profile->SetPath(m_folderPath);

   FolderType typeFolder = GetFolderType(READ_CONFIG(profile, MP_FOLDER_TYPE));
   if ( typeFolder == FolderInvalid )
   {
      // get the last used value from the profile
      typeFolder = (FolderType)READ_APPCONFIG(MP_LAST_CREATED_FOLDER_TYPE);
   }

   if ( typeFolder == Inbox && m_isCreating )
   {
      FAIL_MSG("how did we manage to create an INBOX folder?");

      typeFolder = (FolderType)MP_LAST_CREATED_FOLDER_TYPE_D;
   }

   // this will also call SetDefaultValues()
   m_radio->SetSelection(GetRadioIndexFromFolderType(typeFolder));

   if ( m_folderPath )
   {
      // set incoming checkbox to right value
      MFolder_obj folder(m_folderPath);

      m_isIncoming->SetValue( (folder->GetFlags() & MF_FLAGS_INCOMING) != 0 );
      m_keepOpen->SetValue( (folder->GetFlags() & MF_FLAGS_KEEPOPEN) != 0 );
      m_forceReOpen->SetValue( (folder->GetFlags() & MF_FLAGS_REOPENONPING) != 0 );
   }

#ifdef __WXMSW__
   // the notification is sent automatically under GTK
   UpdateUI(m_radio->GetSelection());
#endif // MSW

   return TRUE;
}

bool
wxFolderPropertiesPage::TransferDataFromWindow(void)
{
   FolderType typeFolder = GetCurrentFolderType();

   // it's impossible to create INBOX folder
   CHECK( !m_dlgCreate || typeFolder != Inbox, false,
          "Ok button should be disabled" );

   // 0th step: verify if the settings are self-consistent

   // doesn't the folder by this name already exist?

   // folder flags
   int flags = 0;
   if ( m_isIncoming->GetValue() )
      flags |= MF_FLAGS_INCOMING;
   if ( m_keepOpen->GetValue() )
      flags |= MF_FLAGS_KEEPOPEN;
   if ( m_forceReOpen->GetValue() )
      flags |= MF_FLAGS_REOPENONPING;

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

   // common for all folders: remember the login info for the folders for which
   // it makes sense or for folder groups (for which we always remember
   // everything)
   if ( (hasUsername && !(flags & MF_FLAGS_ANON)) || typeFolder == FolderGroup )
   {
      WriteEntryIfChanged(Username, loginName);
      WriteEntryIfChanged(Password, strutil_encrypt(password));
   }

   m_profile->writeEntry(MP_FOLDER_TYPE, typeFolder | flags);

   String server = m_server->GetValue();
   if ( FolderTypeHasServer(typeFolder) )
   {
      WriteEntryIfChanged(typeFolder == Nntp ? ServerNews : Server, server);
   }
   else if ( typeFolder == FolderGroup )
   {
      // the right thing to do is to write the server value into both profile
      // entries: for POP/IMAP server and for NNTP one because like this
      // everybody will inherit it
      WriteEntryIfChanged(ServerNews, server);
      WriteEntryIfChanged(Server, server);
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

      case FolderGroup:
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
      if(isKeepOpen)
         mApplication->AddKeepOpenFolder(fullname);
      else
      {
         VERIFY(mApplication->RemoveKeepOpenFolder(fullname),
                "failed to reset 'keep open' property");
      }
   }
   //else: nothing changed, nothing to do

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
   }

   folder->DecRef();

   // remember the type of the last created folder - it will be the default
   // type the next time (TODO we don't have persistent radioboxes yet)
   mApplication->GetProfile()->writeEntry(MP_LAST_CREATED_FOLDER_TYPE,
                                          typeFolder);

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
                       _("All files (*.*)|*.*"),
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
