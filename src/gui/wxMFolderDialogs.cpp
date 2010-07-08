//////////////////////////////////////////////////////////////////////////////
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

/*
   The GUI update logic in these dialogs is a horrible mess. I am not sure
   that even I understand any longer how it works but here is what I think it
   does:

   0. on creation, TransferDataToWindow() calles SetDefaultValues()
      to initialize the controls with the values either read from profile (for
      the existing ones) or the reasonable defaults (new ones) and then
      SetDefaultValues() calls DoUpdateUIForFolder() to enable/disable settings
      depending on the precise folder type

   1. when anything changes, DoUpdateUI() is called and does, roughly, this:
         if ( folder type changed )
            if ( radio box selection changed )
               reinit the folder subtype choice
               enable/disable entries depending on the radio selection

            SetDefaultValues()

   2. when creating a new folder the user may edit the folder name in the
      text control on top of the dialog which changes the value of the path
      field unless it had been changed

      vice versa, if the user edits the path field we try to generate the
      expected folder name from it - again, only if the user hadn't entered it
      manually before

      all this is done in UpdateOnFolderNameChange() which also takes care of
      preventing the infinite recursion which could happen as wxTextCtrl sends
      notifications even when its text is changed programmatically


   Now this is theory and in practice there were some really strange things
   going on with MH folder (granted, this is complicated as we want to check
   that the folder path is always under MHROOT) which I think are not needed
   any more. I keep the old code inside "#ifdef USE_OLD_MH" but if the logic
   above correponds to the code, it shouldn't be necessary to do all this.
 */

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"
#  include "MApplication.h"
#  include "strutil.h"
#  include "Mdefaults.h"

#  include <wx/layout.h>
#  include <wx/stattext.h>      // for wxStaticText
#endif // USE_PCH

#include <wx/persist/bookctrl.h>

#include "MFolderDialogs.h"
#include "MFolder.h"
#include "MailFolder.h"

#include "gui/wxOptionsPage.h"
#include "gui/wxBrowseButton.h"

// why is this conditional?
#define USE_LOCAL_CHECKBOX

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_FOLDER_FILE_DRIVER;
extern const MOption MP_FOLDER_LOGIN;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_FOLDER_PATH;
extern const MOption MP_FOLDER_TYPE;
extern const MOption MP_HOSTNAME;
extern const MOption MP_IMAPHOST;
extern const MOption MP_LAST_CREATED_FOLDER_TYPE;
extern const MOption MP_NNTPHOST;
extern const MOption MP_POPHOST;
extern const MOption MP_USERNAME;
extern const MOption MP_USE_SSL;
extern const MOption MP_USE_SSL_UNSIGNED;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_ASK_LOGIN;
extern const MPersMsgBox *M_MSGBOX_ASK_PWD;

// ============================================================================
// private classes
// ============================================================================

// ----------------------------------------------------------------------------
// dialog classes
// ----------------------------------------------------------------------------

// base class for folder creation and properties dialog
class wxFolderBaseDialog : public wxOptionsEditDialog
{
public:
   wxFolderBaseDialog(wxWindow *parent, const wxString& title);
   virtual ~wxFolderBaseDialog()
   {
      SafeDecRef(m_newFolder);
      SafeDecRef(m_parentFolder);
      SafeDecRef(m_profile);
   }

   // call this to show the specified page initially
   void ShowPage(FolderCreatePage page)
   {
      CreateAllControls();
      SetNotebookPage(page);
   }

   // initialization (should be called before the dialog is shown)
      // set folder we're working with
   void SetFolder(MFolder *folder)
      { SafeIncRef(m_newFolder = folder); }
      // set the parent folder
   void SetParentFolder(MFolder *parentFolder)
      { SafeIncRef(m_parentFolder = parentFolder); }

   // accessors
      // get the parent folder (may return NULL)
   MFolder *GetParentFolder() const
   {
      m_parentFolder->IncRef();
      return m_parentFolder;
   }
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

   // don't let the base class to enable the buttons if we can't allow it
   // because some entries are missing/incorrect
   virtual void EnableButtons(bool enable)
   {
      if ( !enable || ShouldEnableOk() )
         wxOptionsEditDialog::EnableButtons(enable);
      //else: ignore
   }

   // base class pure virtual: return the profile we're working with (IncRef'd)
   virtual Profile *GetProfile() const
   {
      if ( m_newFolder && !m_profile )
      {
         ((wxFolderBaseDialog *)this)->m_profile = // const_cast
            Profile::CreateProfile(m_newFolder->GetFullName());
      }

      SafeIncRef(m_profile);

      // may be NULL if we're creaing the folder and it hasn't been created yet
      return m_profile;
   }

protected:
   // return TRUE if the Ok/Apply buttons should be enabled (depending on the
   // state of the other controls)
   bool ShouldEnableOk() const;

   // tell all notebook pages (except the first one) which profile we're
   // working with
   void SetPagesProfile(Profile *profile);

   wxTextCtrl  *m_folderName,
               *m_parentName;

   wxFolderBrowseButton *m_btnParentFolder;

   MFolder *m_parentFolder,
           *m_newFolder;

   Profile *m_profile;

   // FALSE if at least one of the pages contains incorrect information, if
   // it's TRUE it doesn't mean that the buttons are enabled - the global
   // dialog settings (folder name and parent) must be correct too
   bool m_mayEnableOk;

private:
   DECLARE_ABSTRACT_CLASS(wxFolderBaseDialog)
   DECLARE_NO_COPY_CLASS(wxFolderBaseDialog)
};

// folder properties dialog
class wxFolderPropertiesDialog : public wxFolderBaseDialog
{
public:
   wxFolderPropertiesDialog(wxWindow *parent,
                            MFolder *folder);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   DECLARE_ABSTRACT_CLASS(wxFolderPropertiesDialog)
   DECLARE_NO_COPY_CLASS(wxFolderPropertiesDialog)
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
   MFolder *DoCreateFolder(MFolderType folderType);

   // set the folder name
   void SetFolderName(const String& name);

   // callbacks
   void OnFolderNameChange(wxCommandEvent& event);
   void OnUpdateButton(wxUpdateUIEvent& event);

private:
   // set to TRUE if the user changed the folder name, FALSE otherwise and -1
   // if we're changing it programmatically
   int m_nameModifiedByUser;

   DECLARE_ABSTRACT_CLASS(wxFolderCreateDialog)
   DECLARE_NO_COPY_CLASS(wxFolderCreateDialog)
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// A notebook page containing the folder properties (access)
// ----------------------------------------------------------------------------

// HACK: the 3rd argument to the ctor tells us whether we're creating a new
//       folder (if it's !NULL) or just showing properties of an existing one.
//       Our UI behaves slightly differently in these 2 cases.
class wxFolderPropertiesPage : public MBookCtrlPageBase
{
public:
   wxFolderPropertiesPage(MBookCtrl *notebook,
                          Profile *profile,
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
   void OnChange(wxCommandEvent& event);
   void OnRadioBox(wxCommandEvent& WXUNUSED(event)) { OnEvent(); }
   void OnComboBox(wxCommandEvent& WXUNUSED(event)) { OnEvent(); }
   void OnChoice(wxCommandEvent& event);

   void OnCheckBox(wxCommandEvent& event)
   {
      if ( event.GetEventObject() == m_isAnonymous )
      {
         m_login->Enable( !m_isAnonymous->GetValue() );
         m_password->Enable( !m_isAnonymous->GetValue() );
      }

      OnEvent();
   }

   void UpdateOnFolderNameChange();

protected:
   // react to any event - update all the controls
   void OnEvent();

   // hack: this method tells whether we're in process of creating the folder
   // or just showing the properties for it. Ultimately, it shouldn't be
   // necessary, but for now we use it to adjust our behaviour depending on
   // what we're doing
   bool IsCreating() const { return m_dlgCreate != NULL; }

   // the radiobox indices
   enum RadioIndex
   {
      Radio_File,    // MBX/MH/INBOX
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
      FileFolderSubtype_Mbx,        // default format for Mahogany folders
      FileFolderSubtype_Mbox,       // a standard MBOX style file folder
      FileFolderSubtype_Mmdf,       // SCO Unix format
      FileFolderSubtype_Tenex,      // Tenex format
      FileFolderSubtype_MH,         // an MH directoryfolder
#ifdef EXPERIMENTAL_MFormat
      FileFolderSubtype_MDir,       // Mahogany folder directory format
      FileFolderSubtype_MFile,      // Mahogany folder file format
#endif // EXPERIMENTAL_MFormat
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
      HostName,   // hostname (default value for all servers)
      ServerPop,  // server (for POP3)
      ServerImap, // server (for IMAP)
      ServerNews, // server (for NNTP)
      Path,       // path for file based folders, newsgroup for NEWS/NNTP
      MaxProperty
   };

   void WriteEntryIfChanged(FolderProperty entry, const wxString& value);

#ifdef USE_SSL
   // same as above but for int properties
   enum FolderIntProperty
   {
      SSL,              // SSL support
      AcceptUnsigned,   // accept unsigned SSL certs?
      MaxIntProperty
   };

   void WriteEntryIfChanged(FolderIntProperty entry, int value);
#endif // USE_SSL

   // fills the subtype combobox with the available values for the
   // given radiobox selection
   void FillSubtypeCombo(RadioIndex sel);

   // clears the fields which don't make sense for the new selection
   void ClearInvalidFields(RadioIndex sel);

   // get the type of the folder chosen from the combination of the current
   // radiobox and combobox values or from the given one (if it's != Radio_Max)
   MFolderType GetCurrentFolderType(RadioIndex selRadiobox = Radio_Max,
                                   int selChoice = -1) const;

   // inverse function of the above one: get the radiobox and choice indices
   // (if any) from the folder type
   RadioIndex GetRadioIndexFromFolderType(MFolderType type,
                                          int *choiceIndex = NULL) const;

   // enable the controls which make sense for a NNTP/News folder
   void EnableControlsForNewsGroup(bool isNNTP = TRUE);

   // enable the controls which make sense for an POP or IMAP folder
   void EnableControlsForImapOrPop(bool isIMAP = TRUE);

   // enable the controls which make sense for MBOX or MH folder
   void EnableControlsForFileFolder(MFolderType folderType);

   /// the parent notebook control
   MBookCtrl *m_notebook;

   /// the profile we use to read our settings from/write them to
   Profile *m_profile;

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
   /// Keep it always open?
   wxCheckBox *m_keepOpen;
   /// Is folder hidden?
   wxCheckBox *m_isHidden;
   /// Use anonymous access for this folder?
   wxCheckBox *m_isAnonymous;
#ifdef USE_SSL
   /// degree of SSL support for this folder
   wxChoice *m_useSSL;
   /// Accept unsigned certificates?
   wxCheckBox *m_acceptUnsignedSSL;
#endif // USE_SSL

#ifdef USE_LOCAL_CHECKBOX
   /// Is folder local?
   wxCheckBox *m_isLocal;
#endif // USE_LOCAL_CHECKBOX
   /// Can this folder be opened?
   wxCheckBox *m_canBeOpened;
   /// Can this folder have subfolders?
   wxCheckBox *m_isGroup;
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

#ifdef USE_SSL
   /// and another one with the integer values (currently ionly used for SSL)
   int m_originalIntValues[MaxIntProperty];
#endif // USE_SSL

   /// the original folder type for file-based folders
   FileMailboxFormat m_originalMboxFormat;

   /// the initial value of the "anonymous" flag
   bool m_originalIsAnonymous;
#ifdef USE_LOCAL_CHECKBOX
   /// the initial value of the "is local" flag
   bool m_originalIsLocalValue;
#endif // USE_LOCAL_CHECKBOX
   /// the initial value of the "hidden" flag
   bool m_originalIsHiddenValue;
   /// the initial value of the "can be opened" flag
   bool m_originalCanBeOpened;
   /// the initial value of the "is group" flag
   bool m_originalIsGroup;

   /// the original value for the folder icon index (-1 if nothing special)
   int m_originalFolderIcon;

   /// the current folder type or MF_ILLEGAL if none
   MFolderType m_folderType;

   // true if the user modified manually the filename, false otherwise and may
   // have temporary value of -1 if we modified it programmatically
   int m_userModifiedPath;

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxFolderPropertiesPage)
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

   wxFolderCreateNotebook(wxWindow *parent, wxFolderCreateDialog *dlg = NULL);

private:
   DECLARE_NO_COPY_CLASS(wxFolderCreateNotebook)
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

   DECLARE_NO_COPY_CLASS(wxFolderIconBrowseButtonInDialog)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxFolderBaseDialog, wxOptionsEditDialog)
IMPLEMENT_ABSTRACT_CLASS(wxFolderCreateDialog, wxFolderBaseDialog)
IMPLEMENT_ABSTRACT_CLASS(wxFolderPropertiesDialog, wxFolderBaseDialog)

BEGIN_EVENT_TABLE(wxFolderCreateDialog, wxOptionsEditDialog)
   EVT_TEXT(wxFolderCreateDialog::Folder_Name,
            wxFolderCreateDialog::OnFolderNameChange)
   EVT_UPDATE_UI(wxID_OK,    wxFolderCreateDialog::OnUpdateButton)
   EVT_UPDATE_UI(wxID_APPLY, wxFolderCreateDialog::OnUpdateButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderPropertiesPage, MBookCtrlPageBase)
   EVT_RADIOBOX(-1, wxFolderPropertiesPage::OnRadioBox)
   EVT_CHECKBOX(-1, wxFolderPropertiesPage::OnCheckBox)
   EVT_COMBOBOX(-1, wxFolderPropertiesPage::OnComboBox)
   EVT_CHOICE  (-1, wxFolderPropertiesPage::OnChoice)
   EVT_TEXT    (-1, wxFolderPropertiesPage::OnChange)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderBaseDialog
// ----------------------------------------------------------------------------

wxFolderBaseDialog::wxFolderBaseDialog(wxWindow *parent,
                                       const wxString& title)
                  : wxOptionsEditDialog(GET_PARENT_OF_CLASS(parent, wxFrame),
                                     title,
                                     _T("FolderProperties"))
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
      m_folderName = new wxTextCtrl(panel, Folder_Name, wxEmptyString);
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

   m_parentName = new wxTextCtrl(panel, -1, wxEmptyString);
   if ( m_parentFolder )
   {
      m_parentName->SetValue(m_parentFolder->GetFullName());
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

   // for the "Show folder properties" dialog it doesn't make sense to keep it
   // enabled as the parent of an existing folder cannot be changed -- and for
   // the "Create folder" dialog it is reenabled later
   m_btnParentFolder->Disable();

   // return the last control created
   return m_parentName;
}

void wxFolderBaseDialog::CreateNotebook(wxPanel *panel)
{
   m_notebook = new wxFolderCreateNotebook
                    (
                     panel,
                     wxDynamicCast(this, wxFolderCreateDialog)
                    );
}

void wxFolderBaseDialog::SetPagesProfile(Profile *profile)
{
   // we assume that the first page is the "Access" one which is a bit special,
   // this is why we start from the second one
   wxASSERT_MSG( FolderCreatePage_Folder == 0, _T("this code must be fixed") );

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

// ----------------------------------------------------------------------------
// wxFolderCreateDialog
// ----------------------------------------------------------------------------

wxFolderCreateDialog::wxFolderCreateDialog(wxWindow *parent,
                                           MFolder *parentFolder)
   : wxFolderBaseDialog(parent, _("Create new folder"))
{
   SetParentFolder(parentFolder);

   m_nameModifiedByUser = false;
}

MFolder *wxFolderCreateDialog::DoCreateFolder(MFolderType folderType)
{
   if ( !m_parentFolder )
   {
      m_parentFolder = m_btnParentFolder->GetFolder();
   }

   if ( !m_parentFolder )
   {
      // take the root by default
      m_parentFolder = MFolder::Get(wxEmptyString);
   }

   m_newFolder = m_parentFolder->CreateSubfolder(m_folderName->GetValue(),
                                                 folderType);


   if ( m_newFolder )
   {
      // we shouldn't have had it yet
      ASSERT_MSG( !m_profile, _T("folder just created, but profile exists?") );

      // tell the other pages that we now have a folder (and hence a profile)
      String folderName = m_newFolder->GetFullName();
      m_profile = Profile::CreateProfile(folderName);
      CHECK( m_profile, NULL, "failed to create profile for new folder" );

      ApplyConfigSourceSelectedByUser(*m_profile);
      SetPagesProfile(m_profile);
   }

   return m_newFolder;
}

void wxFolderCreateDialog::SetFolderName(const String& name)
{
   // resist any attempt to change the folder name which was entered by user
   // (and not resulted from previous calls to this function)
   if ( m_nameModifiedByUser )
      return;

   // take the last component of the folder name only considering that only
   // '/' and '.' are valid separators - this is surely false, at least for
   // IMAP folders which can have arbitrary delimiter characters, but this is
   // all we can do now as currently we don't know the server name yet and so
   // we can't connect to it and ask it for the proper delimiter
   String nameFolder = name.AfterLast('/').AfterLast('.');

   m_nameModifiedByUser = -1;
   m_folderName->SetValue(nameFolder);
   m_nameModifiedByUser = false;
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
   //else: it comes from a call to SetFolderName() called by the page itself,
   //      no need to update it

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
   // "Access" page, but the persistent notebook restores the page we used the
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

   if ( !wxOptionsEditDialog::TransferDataToWindow() )
      return FALSE;

   // enable changing the parent folder -- this can't be done for an
   // already existing folder so the base class ctor disables it, but we are
   // creating a new folder and so can choose its parent to be whatever we want
   m_btnParentFolder->Enable(TRUE);

   m_folderName->SetFocus();

   return TRUE;
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
      //TODO Nerijus  we should convert them to UTF-7
      wxLogError(_("Sorry, international characters (like '%c') are "
                   "not supported in the mailbox folder names, please "
                   "don't use them."), ch);

      ok = FALSE;
   }

   SafeDecRef(m_parentFolder);
   m_parentFolder = MFolder::Get(m_parentName->GetValue());
   if ( !m_parentFolder )
   {
      wxLogError(_("Folder '%s' specified as the parent for the new folder "
                   "doesn't exist. Please choose an existing folder as "
                   "parent or leave it blank."), folderName.c_str());
      ok = FALSE;
   }

   if ( ok )
   {
      ok = wxOptionsEditDialog::TransferDataFromWindow();
   }

   if ( ok )
   {
      CHECK( m_parentFolder, false, _T("should have created parent folder") );

      // new folder has already been created normally
      m_newFolder = m_parentFolder->GetSubfolder(m_folderName->GetValue());

      CHECK( m_newFolder, false, _T("new folder not created") );
   }

   return ok;
}

// ----------------------------------------------------------------------------
// wxFolderPropertiesDialog
// ----------------------------------------------------------------------------

wxFolderPropertiesDialog::wxFolderPropertiesDialog(wxWindow *frame,
                                                   MFolder *folder)
                        : wxFolderBaseDialog(frame, _("Folder properties"))
{
   SetFolder(folder);
}

bool wxFolderPropertiesDialog::TransferDataToWindow()
{
   CHECK( m_newFolder, FALSE, _T("no folder in folder properties dialog") );

   wxString folderName = m_newFolder->GetFullName();
   Profile_obj profile(GetProfile());

   // use SetFolderPath() so the page will read its data from the profile
   // section corresponding to our folder
   GET_FOLDER_PAGE(m_notebook)->SetFolderPath(folderName);

   SetPagesProfile(profile);

   return wxFolderBaseDialog::TransferDataToWindow();
}

bool wxFolderPropertiesDialog::TransferDataFromWindow()
{
   CHECK( m_newFolder, FALSE, _T("no folder in folder properties dialog") );

   return wxFolderBaseDialog::TransferDataFromWindow();
}

// ----------------------------------------------------------------------------
// wxFolderPropertiesPage
// ----------------------------------------------------------------------------

wxFolderPropertiesPage::wxFolderPropertiesPage(MBookCtrl *notebook,
                                               Profile *profile,
                                               wxFolderCreateDialog *dlg)
                      : MBookCtrlPageBase(notebook)
{
   // add us to the notebook
   int image = FolderCreatePage_Folder;
   notebook->AddPage(this, _("Access"), FALSE /* don't select */, image);

   // are we in "view properties" or "create" mode?
   m_dlgCreate = dlg;

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
#ifdef USE_SSL
      Label_UseSSL,
      Label_AcceptUnsignedSSL,
#endif // USE_SSL
      Label_KeepOpen,
      Label_IsAnonymous,
#ifdef USE_LOCAL_CHECKBOX
      Label_IsLocal,
#endif // USE_LOCAL_CHECKBOX
      Label_IsHidden,
      Label_CanBeOpened,
      Label_IsGroup,
      Label_FolderSubtype,
      Label_FolderIcon,
      Label_Max
   };

   // the remaining unused accel letters (remove one if you add new label):
   //    ADGJQVXZ
   static const char *szLabels[Label_Max] =
   {
      gettext_noop("&User name"),
      gettext_noop("&Password"),
      gettext_noop("&File name"),
      gettext_noop("&Server"),
      gettext_noop("&Mailbox"),
      gettext_noop("&Newsgroup"),
      gettext_noop("&Comment"),
#ifdef USE_SSL
      gettext_noop("SS&L/TLS usage"),
      gettext_noop("Accept &unsigned certificates"),
#endif // USE_SSL
      gettext_noop("&Keep connection when idle"),
      gettext_noop("Anon&ymous access"),
#ifdef USE_LOCAL_CHECKBOX
      gettext_noop("Can be accessed &without network"),
#endif // USE_LOCAL_CHECKBOX
      gettext_noop("&Hide folder in tree"),
      gettext_noop("Can &be opened"),
      gettext_noop("Contains subfold&ers"),
      gettext_noop("Folder sub&type"),
      gettext_noop("&Icon for this folder"),
   };

   wxArrayString labels;
   for ( size_t n = 0; n < Label_Max; n++ )
   {
      labels.Add(wxGetTranslation(szLabels[n]));
   }

   // determine the longest label
   long widthMax = GetMaxLabelWidth(labels, this);

   m_login = CreateTextWithLabel(labels[Label_Login], widthMax, m_radio);
   m_password = CreateTextWithLabel(labels[Label_Password], widthMax, m_login,
                                    0, wxTE_PASSWORD);
   m_server = CreateTextWithLabel(labels[Label_Server], widthMax, m_password);
   m_mailboxname = CreateTextWithLabel(labels[Label_Mailboxname], widthMax, m_server);
   m_newsgroup = CreateTextWithLabel(labels[Label_Newsgroup], widthMax, m_mailboxname);
   m_comment = CreateTextWithLabel(labels[Label_Comment], widthMax, m_newsgroup);
   m_path = CreateFileOrDirEntry(labels[Label_Path], widthMax,
                                 m_comment, &m_browsePath,
                                 TRUE,    // open
                                 FALSE);  // allow non existing files

   wxControl *lastCtrl = m_path;

#ifdef USE_SSL
   m_useSSL = CreateChoice(labels[Label_UseSSL] +
                           _(":Don't use SSL at all"
                             ":Use TLS if available"
                             ":Use TLS only"
                             ":Use SSL only"),
                           widthMax, lastCtrl);
   m_acceptUnsignedSSL = CreateCheckBox(labels[Label_AcceptUnsignedSSL], widthMax, m_useSSL);
   lastCtrl = m_acceptUnsignedSSL;
#endif // USE_SSL

   m_keepOpen = CreateCheckBox(labels[Label_KeepOpen], widthMax, lastCtrl);
   m_isAnonymous = CreateCheckBox(labels[Label_IsAnonymous], widthMax, m_keepOpen);

   lastCtrl = m_isAnonymous;
#ifdef USE_LOCAL_CHECKBOX
   m_isLocal = CreateCheckBox(labels[Label_IsLocal], widthMax, lastCtrl);
   lastCtrl = m_isLocal;
#endif // USE_LOCAL_CHECKBOX

   m_isHidden = CreateCheckBox(labels[Label_IsHidden], widthMax, lastCtrl);
   m_canBeOpened = CreateCheckBox(labels[Label_CanBeOpened], widthMax, m_isHidden);
   m_isGroup = CreateCheckBox(labels[Label_IsGroup], widthMax, m_canBeOpened);
   m_folderSubtype = CreateChoice(labels[Label_FolderSubtype], widthMax, m_isGroup);

#if wxUSE_TOOLTIPS
   // the checkboxes might not be very clear, so add some explanations in the
   // form of tooltips
   m_keepOpen->SetToolTip(_("Check this to always maintain connection "
                            "to the server once it was established."));

   m_isAnonymous->SetToolTip(_("For the types of folders which require user "
                               "name and password check this to try to connect\n"
                               "without any - this might work or fail depending "
                               "on the server settings."));

#ifdef USE_SSL
   m_useSSL->SetToolTip(_("Choose degree of SSL/TLS support\n"
                          "for communication with the server."));
   m_acceptUnsignedSSL->SetToolTip(_("Accept unsigned (self-signed) SSL certificates?"));
#endif // USE_SSL
#endif // wxUSE_TOOLTIPS

   wxFolderBaseDialog *dlgParent = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
   ASSERT_MSG( dlgParent, _T("should have a parent dialog!") );

   m_browseIcon = new wxFolderIconBrowseButtonInDialog(dlgParent,
                                                       GetCanvas(),
                                                       _("Choose folder icon"));

   (void)CreateIconEntry(labels[Label_FolderIcon], widthMax, m_folderSubtype, m_browseIcon);

   m_radio->Enable(IsCreating());
}

void
wxFolderPropertiesPage::SetFolderName(const wxString& name)
{
   wxFolderCreateDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderCreateDialog);

   CHECK_RET( dlg, _T("SetFolderName() can be only called when creating") );

   dlg->SetFolderName(name);
}

void
wxFolderPropertiesPage::OnChoice(wxCommandEvent& event)
{
#ifdef USE_SSL
   if ( event.GetEventObject() == m_useSSL )
   {
      m_acceptUnsignedSSL->Enable(m_useSSL->GetSelection() != SSLSupport_None);
   }
#endif // USE_SSL

   OnEvent();
}

void
wxFolderPropertiesPage::OnEvent()
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);

   CHECK_RET( dlg, _T("folder page should be in folder dialog!") );

   dlg->SetDirty();

   // folder might have changed, so call DoUpdateUI() which will call
   // SetDefaultValues() if this is really the case
   DoUpdateUI();
}

void
wxFolderPropertiesPage::OnChange(wxCommandEvent& event)
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);

   CHECK_RET( dlg, _T("folder page should be in folder dialog!") );

   event.Skip();

   wxObject *objEvent = event.GetEventObject();

   // the rest doesn't make any sense for the "properties" dialog because the
   // text in the path field can't change anyhow
   if ( !IsCreating() )
   {
      // OTOH, call SetDirty() only we're changing the properites of an already
      // existing folder - when we create a new folder it is always dirty
      dlg->SetDirty();

      return;
   }

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
#ifdef EXPERIMENTAL_MFormat
      case MF_MDIR:
      case MF_MFILE:
#endif // EXPERIMENTAL_MFormat
         // set the file name as the default folder name
         if ( objEvent == m_path )
         {
            wxString fullname = m_path->GetValue();
            if ( fullname.empty() )
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

            SetFolderName(wxFileName(fullname).GetName());
         }
         break;

      case MF_IMAP:
         if ( objEvent == m_mailboxname )
         {
            // this is completely bogus as IMAP folders can have _any_ symbol
            // as separator, but these two are the only ones in common use
            wxString name, path = m_mailboxname->GetValue();
            if ( wxStrchr(path, '/') )
               name = path.AfterLast('/');
            else if ( wxStrchr(path, '.') )
               name = path.AfterLast('.');
            else
               name = path;

            SetFolderName(path);
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
      // ... try to set it from the folder name
      wxFolderCreateDialog *dlg =
         GET_PARENT_OF_CLASS(this, wxFolderCreateDialog);

      CHECK_RET( dlg, _T("we should be only called when creating") );

      MFolder_obj parent(dlg->GetParentFolder());

      // NB: we modify the text even if the folder name is empty because
      //     otherwise entering a character into "Folder name" field and
      //     erasing it wouldn't restore the original "Path" value
      wxString folderName;

      // the control whose value we will automatically set
      wxTextCtrl *textToSet = NULL;

      MFolderType folderType = GetCurrentFolderType();
      switch ( folderType )
      {
         case MF_MH:
            // MH folder should be created under its parent by default
            if ( parent )
               folderName = parent->GetPath();

            // AfterFirst() removes the MH root prefix
            folderName = folderName.AfterFirst('/');

            if ( !folderName.empty() )
              folderName += '/';

            // fall through

         case MF_FILE:
            // MBOX folders don't have hierarchical structure, use as is
            textToSet = m_path;
            break;

         case MF_NNTP:
         case MF_NEWS:
            textToSet = m_newsgroup;
            // fall through

         case MF_IMAP:
            if ( !textToSet )
               textToSet = m_mailboxname;

            if ( parent )
               folderName = MailFolder::GetLogicalMailboxName(parent->GetPath());

            if ( !folderName.empty() )
            {
               // we have a problem with the IMAP delimiters here as don't
               // have the MailFolder to query - use this simple heuristic
               // and hope for the best
               char chDelim;

               if ( folderType == MF_IMAP )
               {
                  if ( wxStrchr(folderName, '/') )
                  {
                     // if it already has a slash, chances are that it is
                     // used as the path separator
                     chDelim = '/';
                  }
                  else if ( wxStrchr(folderName, '.') )
                  {
                     // same as above
                     chDelim = '.';
                  }
                  else
                  {
                     // can't set the name automatically, better not to do
                     // anything than generating a wrong folder path
                     return;
                  }
               }
               else // news
               {
                  chDelim = '.';
               }

               folderName += chDelim;
            }
            break;

         default:
            FAIL_MSG( _T("unexpected folder type in UpdateOnFolderNameChange") );
            // fall through

         case MF_POP:
         case MF_GROUP:
            // don't modify the mailbox name at all: either because there is
            // nothing to modify (MF_POP) or because it doesn't make sense
            // (MF_GROUP)
            return;
      }

      CHECK_RET( textToSet, _T("must have been set above") );

      folderName += dlg->GetFolderName();

      // leave the relative path, it will be normalized when we open the folder
      // anyhow and like this we don't have to change the paths of all the
      // folders if we want to change the folder directory
#if 0
      if ( folderType == MF_MH || folderType == MF_FILE )
      {
         // strutil_expandfoldername() will normalize the path, i.e. make it
         // absolute prepending the correct prefix depending on the folder type
         folderName = strutil_expandfoldername(folderName, folderType);
      }
#endif // 0

      // this will tell SetValue() that we modified it ourselves, not the user
      m_userModifiedPath = -1;

      textToSet->SetValue(folderName);
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

#ifdef USE_LOCAL_CHECKBOX
   m_isLocal->Enable(isNNTP);
#endif // USE_LOCAL_CHECKBOX

   // "group" here means a news hierarchy
   m_isGroup->Enable(TRUE);

   // the news hierarchies can't be opened, but sometimes a newsgroup is a
   // hierarchy too (news.groups is a group, but there is also
   // news.groups.questions, so it is a hierarchy as well), so we need to
   // allow the user to set this flag as well
   m_canBeOpened->Enable(TRUE);
}

// enable the controls which make sense for a POP or IMAP folder
void
wxFolderPropertiesPage::EnableControlsForImapOrPop(bool isIMAP)
{
   EnableTextWithLabel(m_mailboxname, isIMAP);
   EnableTextWithLabel(m_server, TRUE);
   EnableTextWithLabel(m_newsgroup, FALSE);
   EnableTextWithButton(m_path, FALSE);

#ifdef USE_LOCAL_CHECKBOX
   m_isLocal->Enable(TRUE);
#endif // USE_LOCAL_CHECKBOX

   // this makes no sense for POP
   if ( isIMAP )
   {
      if ( IsCreating() )
      {
         // we can't change this setting when creating an IMAP folder
         m_isGroup->Enable(FALSE);
      }
      else
      {
         // we should have auto detected if this folder could have children or
         // not
         m_isGroup->SetValue(m_originalIsGroup);

         // but allow the user to override it if we're mistaken
         m_isGroup->Enable(TRUE);
      }
   }
   else // POP
   {
      // no folder hierarchies under POP
      m_isGroup->SetValue(FALSE);
      m_isGroup->Enable(FALSE);

      // can't keep a POP3 folder always opened
      m_keepOpen->Enable(TRUE);
   }
}

void
wxFolderPropertiesPage::EnableControlsForFileFolder(MFolderType /* type */)
{
   EnableTextWithLabel(m_mailboxname, FALSE);
   EnableTextWithLabel(m_server, FALSE);
   EnableTextWithLabel(m_newsgroup, FALSE);

   // the path can't be changed for an already existing folder
   EnableTextWithButton(m_path, IsCreating());

   // file folders are always local
#ifdef USE_LOCAL_CHECKBOX
   m_isLocal->SetValue(FALSE);
   m_isLocal->Disable();
#endif // USE_LOCAL_CHECKBOX

   // all file folders can be opened
   m_canBeOpened->SetValue(TRUE);
   m_canBeOpened->Disable();

   // the value is fixed (whatever it is) by the folder type
   m_isGroup->Disable();

   bool isGroup = m_folderType == MF_MH
#ifdef EXPERIMENTAL_MFormat
                  || m_folderType == MF_MDIR
#endif // EXPERIMENTAL_MFormat
                  ;

   if ( isGroup )
   {
      m_browsePath->BrowseForDirectories();
   }
   else
   {
      m_browsePath->BrowseForFiles();
   }

   m_isGroup->SetValue(isGroup);
}

// called when radiobox/choice selection changes
void
wxFolderPropertiesPage::DoUpdateUI()
{
   // get the current folder type from dialog controls
   RadioIndex selRadio = (RadioIndex)m_radio->GetSelection();
   MFolderType folderType = GetCurrentFolderType(selRadio);

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
}

// this function only updates the controls for the current folder, it never
// changes m_folderType or radiobox/choice selections
void
wxFolderPropertiesPage::DoUpdateUIForFolder()
{
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
   CHECK_RET( dlg, _T("wxFolderPropertiesPage without parent dialog?") );

   bool enableAnonymous, enableLogin;
#ifdef USE_SSL
   bool enableSSL;
#endif // USE_SSL

   if ( m_folderType == MF_GROUP )
   {
      // enable all fields for these folders because, as a group can contain
      // anything at all, it may make sense to set them to be inherited by the
      // subfolders
      enableAnonymous =
      enableLogin = true;

#ifdef USE_SSL
      enableSSL = true;
#endif // USE_SSL
   }
   else // !group
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
#endif // USE_SSL
   }

#ifdef USE_SSL
   m_useSSL->Enable(enableSSL);
   if ( !enableSSL || m_useSSL->GetSelection() == SSLSupport_None )
      m_acceptUnsignedSSL->Disable();
#endif // USE_SSL

   m_keepOpen->Enable(TRUE);

   m_isAnonymous->Enable(enableAnonymous);
   m_login->Enable(enableLogin);
   m_password->Enable(enableLogin);

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
#ifdef EXPERIMENTAL_MFormat
      case MF_MFILE:
      case MF_MDIR:
#endif // EXPERIMENTAL_MFormat
         EnableControlsForFileFolder(m_folderType);

#ifdef USE_OLD_MH
         if ( m_folderType == MF_MH || m_folderType == MF_MDIR )
         {
            // set the path to MHROOT by default
            if ( !m_path->GetValue() )
            {
               MFolder_obj folderParent(dlg->GetParentFolderName());
               Profile_obj profile(folderParent->GetProfile());

               wxString path;
               path << MailFolder::InitializeMH()
                    << READ_CONFIG(profile, MP_FOLDER_PATH);
               if ( !!path && !wxIsPathSeparator(path.Last()) )
                  path << '/';
               path << dlg->GetFolderName().AfterLast('/');

               m_path->SetValue(path);
            }
         }
#endif // USE_OLD_MH

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
         m_isGroup->Enable(TRUE);

         // a group can never be opened
         m_canBeOpened->Enable(FALSE);
         break;

      default:
         wxFAIL_MSG(_T("Unexpected folder type."));
   }

   // enable folder subtype combobox only if we're creating because (folder
   // type can't be changed later) and if there are any subtypes
   EnableControlWithLabel(m_folderSubtype,
                          IsCreating() && !m_folderSubtype->IsEmpty());

   dlg->SetMayEnableOk(TRUE);
}

void
wxFolderPropertiesPage::FillSubtypeCombo(RadioIndex sel)
{
#ifdef __WXGTK__
   // this is a work around wxGTK bug: if the control is disabled before the
   // items are added, reenabling it later doesn't work properly, so enable it
   // ourselves if needed
   bool wasDisabled = !m_folderSubtype->IsEnabled();
   if ( wasDisabled )
      m_folderSubtype->Enable();
#endif // __WXGTK__

   // do it first anyhow - it might have contained something before
   m_folderSubtype->Clear();

   switch ( sel )
   {
      case Radio_File:
         // NB: the strings should be in sync with FileFolderSubtype_XXX enum
         m_folderSubtype->Append(_("MBX folder (best)"));
         m_folderSubtype->Append(_("MBOX (traditional Unix)"));
         m_folderSubtype->Append(_("MMDF (SCO Unix)"));
         m_folderSubtype->Append(_("Tenex folder"));
         m_folderSubtype->Append(_("MH folder"));
#ifdef EXPERIMENTAL_MFormat
         m_folderSubtype->Append(_("Mahogany MH folder"));
         m_folderSubtype->Append(_("Mahogany file folder"));
#endif // EXPERIMENTAL_MFormat
         break;

      case Radio_News:
         // NB: the strings should be in sync with NewsFolderSubtype_XXX enum
         m_folderSubtype->Append(_("NNTP group (remote server)"));
         m_folderSubtype->Append(_("News group (local newsspool)"));
         break;

      default:
         // nothing to do - leave the wxChoice empty and return, skipping
         // SetSelection() below

#ifdef __WXGTK__
         // disable it back if we should
         if ( wasDisabled )
            m_folderSubtype->Disable();
#endif // __WXGTK__
         return;
   }

   // m_folderType hadn't been updated yet, so use GetCurrentFolderType()!
   int subtype;
   GetRadioIndexFromFolderType(GetCurrentFolderType(sel), &subtype);

   m_folderSubtype->SetSelection(subtype);
}

MFolderType
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
               FAIL_MSG(_T("invalid file folder subtype"));
               // fall through

#ifdef EXPERIMENTAL_MFormat
            case FileFolderSubtype_MFile:
               return MF_MFILE;

            case FileFolderSubtype_MDir:
               return MF_MDIR;
#endif // EXPERIMENTAL_MFormat

            case FileFolderSubtype_Mbx:
            case FileFolderSubtype_Mbox:
            case FileFolderSubtype_Mmdf:
            case FileFolderSubtype_Tenex:
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
               FAIL_MSG(_T("invalid news folder subtype"));
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
         FAIL_MSG(_T("invalid folder radio box selection"));
         return MF_ILLEGAL;
   }
}

wxFolderPropertiesPage::RadioIndex
wxFolderPropertiesPage::GetRadioIndexFromFolderType(MFolderType type,
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
            // file folders come in several flavours, so choose the default
            // one if no selection yet
            int subtype;
            switch ( type )
            {
               default:
                  FAIL_MSG( _T("unknown file folder type?") );
                  // fall through

               case MF_INBOX:
               case MF_FILE:
                  if ( IsCreating() )
                  {
                     subtype = READ_CONFIG(m_profile, MP_FOLDER_FILE_DRIVER);
                     if ( subtype < 0  ||
                           (size_t)subtype > FileFolderSubtype_Max )
                     {
                        FAIL_MSG( _T("invalid mailbox format") );
                        subtype = FileFolderSubtype_Mbx;
                     }
                  }
                  else // !creating
                  {
                     // FIXME: how to get the type of the existing folder?
                     subtype = FileFolderSubtype_Mbx;
                  }
                  break;

               case MF_MH:
                  subtype = FileFolderSubtype_MH;
                  break;
            }

            *choiceIndex = subtype;
         }

         return Radio_File;

#ifdef EXPERIMENTAL_MFormat
      case MF_MFILE:
      case MF_MDIR:
         if ( choiceIndex )
         {
            *choiceIndex = type == MF_MDIR ? FileFolderSubtype_MDir
                                           : FileFolderSubtype_MFile;
         }

         return Radio_File;
#endif // EXPERIMENTAL_MFormat

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
         FAIL_MSG(_T("unexpected folder type value"));
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
      MP_HOSTNAME,
      MP_POPHOST,
      MP_IMAPHOST,
      MP_NNTPHOST,
      MP_FOLDER_PATH,
   };

   if ( value != m_originalValues[property] )
   {
      if ( property == Password )
         m_profile->writeEntry(profileKeys[property], strutil_encrypt(value));
      else
         m_profile->writeEntry(profileKeys[property], value);

      if ( !IsCreating() )
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

#ifdef USE_SSL

void
wxFolderPropertiesPage::WriteEntryIfChanged(FolderIntProperty property,
                                            int value)
{
   static const char *profileKeys[MaxIntProperty] =
   {
      MP_USE_SSL,
      MP_USE_SSL_UNSIGNED,
   };

   if ( value != m_originalIntValues[property] )
   {
      if ( !m_profile->writeEntry(profileKeys[property], value) )
         return;

      if ( !IsCreating() )
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

#endif // USE_SSL

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
      m_newsgroup->SetValue(wxEmptyString);
   }

   if ( sel != Radio_File )
   {
      // this is only for files
      m_path->SetValue(wxEmptyString);
   }

   if ( sel == Radio_File )
   {
      // this is for everything except local folders
      m_server->SetValue(wxEmptyString);
      m_login->SetValue(wxEmptyString);
      m_password->SetValue(wxEmptyString);
   }

   if ( sel != Radio_Imap )
   {
      // this is only for IMAP
      m_mailboxname->SetValue(wxEmptyString);
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

   MFolder_obj folder(m_folderPath);
   CHECK_RET( folder, _T("can't create MFolder to read initial values") );

   RadioIndex selRadio = (RadioIndex)m_radio->GetSelection();
   MFolderType folderType = GetCurrentFolderType(selRadio);

   String value;
   if ( FolderTypeHasUserName(folderType) )
   {
      value = folder->GetLogin();
      m_login->SetValue(value);
      m_originalValues[Username] = value;

      value = folder->GetPassword();
      m_password->SetValue(value);
      m_originalValues[Password] = value;
   }

   if ( FolderTypeHasServer(folderType) )
   {
      value = folder->GetServer();
      m_server->SetValue(value);

      switch ( folderType )
      {
         case MF_NNTP:
            m_originalValues[ServerNews] = value;
            break;
         case MF_POP:
            m_originalValues[ServerPop] = value;
            break;
         case MF_IMAP:
            m_originalValues[ServerImap] = value;
            break;

         default: // suppress warnings

         case MF_GROUP:
            m_originalValues[HostName] = value;
      }
   }

   wxTextCtrl *textToSet;
   value = MailFolder::GetLogicalMailboxName(folder->GetPath());
   switch ( selRadio )
   {
      case Radio_File:
         if ( IsCreating() )
         {
            // switch the browse to the correct mode
            if ( folderType == MF_MH )
               m_browsePath->BrowseForDirectories();
            else
               m_browsePath->BrowseForFiles();

            // don't need to set anything
            textToSet = NULL;
            break;
         }

#ifdef USE_OLD_MH
         // MH complications: must prepend MHROOT to relative paths
         if ( folderType == MF_MH )
         {
            wxString mhRoot = MailFolder::InitializeMH();
            if ( !value.StartsWith(mhRoot) && !wxIsAbsolutePath(value) )
            {
               value.Prepend(mhRoot);
            }
         }
#endif // USE_OLD_MH

         // fall through

      case Radio_Group:
         textToSet = m_path;
         break;

      case Radio_News:
         textToSet = m_newsgroup;
         break;

      case Radio_Imap:
         textToSet = m_mailboxname;
         break;

      default:
         // nothing special to do
         textToSet = NULL;
   }

   if ( textToSet )
   {
      m_userModifiedPath = -1;
      textToSet->SetValue(value);
   }

   m_originalValues[Path] = value;

   if ( !IsCreating() )
      m_comment->SetValue(folder->GetComment());

   // remember the original folder type
   m_originalMboxFormat = folder->GetFileMboxFormat();

   // set the initial values for all checkboxes and remember them: we will only
   // write it back if it changes later
   const int flags = folder->GetFlags();
   m_originalIsHiddenValue = (flags & MF_FLAGS_HIDDEN) != 0;

   m_keepOpen->SetValue((flags & MF_FLAGS_KEEPOPEN) != 0);

#ifdef USE_LOCAL_CHECKBOX
   m_originalIsLocalValue = (flags & MF_FLAGS_ISLOCAL) != 0;
   m_isLocal->SetValue(m_originalIsLocalValue);
#endif // USE_LOCAL_CHECKBOX

   if ( IsCreating() )
   {
      // always create folders which can be opened by default (i.e. even if
      // our parent can't be opened, it doesn't mean that we can't)
      m_originalCanBeOpened = true;
   }
   else
   {
      // use the current value
      m_originalCanBeOpened = (flags & MF_FLAGS_NOSELECT) == 0;
   }

   m_originalIsGroup = (flags & MF_FLAGS_GROUP) != 0;

   m_canBeOpened->SetValue(m_originalCanBeOpened);
   m_isHidden->SetValue(m_originalIsHiddenValue);
   m_isGroup->SetValue(m_originalIsGroup);

   // although NNTP servers do support password-protected access, this
   // is so rare that anonymous should really be the default
   m_originalIsAnonymous = (flags & MF_FLAGS_ANON) ||
                           (IsCreating() && folderType == MF_NNTP);
   m_isAnonymous->SetValue(m_originalIsAnonymous);

#ifdef USE_SSL
   SSLCert certAcceped;
   m_originalIntValues[SSL] = folder->GetSSL(&certAcceped);
   m_useSSL->SetSelection(m_originalIntValues[SSL]);

   m_originalIntValues[AcceptUnsigned] = certAcceped;
   m_acceptUnsignedSSL->SetValue(certAcceped == SSLCert_AcceptUnsigned);
#endif // USE_SSL

   // update the folder icon
   if ( IsCreating() )
   {
      // use default icon for the chosen folder type
      m_originalFolderIcon = GetDefaultFolderTypeIcon(folderType);
   }
   else
   {
      // use the folders icon
      m_originalFolderIcon = GetFolderIconForDisplay(folder);
   }

   m_browseIcon->SetIcon(m_originalFolderIcon);

   // enable/disable the controls for this folder type
   DoUpdateUIForFolder();
}

bool
wxFolderPropertiesPage::IsOk() const
{
   switch ( GetCurrentFolderType() )
   {
      case MF_NNTP:
      case MF_NEWS:
         // it's valid to have an empty name for the newsgroup hierarchy - the
         // entry will represent the whole news server - but not for a
         // newsgroup
         return m_isGroup->GetValue() || !!m_newsgroup->GetValue();

      case MF_FILE:
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
   Profile_obj profile(m_folderPath);

   if ( IsCreating() )
   {
      // if we're creating a folder under a parent with some defined
      // hierarchical type (i.e. not root or group but IMAP or NNTP, for
      // example), default to the parents type initially
      MFolderType typeOfParentChildren = MF_ILLEGAL; // suppress warning
      MFolder_obj folderParent(m_dlgCreate->GetParentFolder());
      if ( folderParent &&
            CanHaveSubfolders(folderParent->GetType(),
                              folderParent->GetFlags(),
                              &typeOfParentChildren) &&
            typeOfParentChildren != MF_ILLEGAL )
      {
         m_folderType = typeOfParentChildren;
      }
      else
      {
         // use the type of the folder last created
         m_folderType =
            (MFolderType)(long)READ_APPCONFIG(MP_LAST_CREATED_FOLDER_TYPE);
      }
   }
   else
   {
      // use the current folder type
      m_folderType = GetFolderType(READ_CONFIG(profile, MP_FOLDER_TYPE));
   }

   if ( (m_folderType == MF_INBOX) && IsCreating() )
   {
      // FAIL_MSG(_T("how did we manage to create an INBOX folder?")); --
      // obviously by using a corrupted config file... no need to crash though

      m_folderType =
         (MFolderType)GetNumericDefault(MP_LAST_CREATED_FOLDER_TYPE);
   }

   if ( m_folderType == MF_INBOX )
   {
      // this is checked for above!
      wxASSERT_MSG( !IsCreating(), _T("can't create INBOX") );

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

   if ( IsCreating() && (selRadio == Radio_File) )
   {
      // set the default value for m_path field
      UpdateOnFolderNameChange();
   }

   SetDefaultValues();

   return TRUE;
}

bool
wxFolderPropertiesPage::TransferDataFromWindow(void)
{
   MFolderType folderType = GetCurrentFolderType();

   // it's impossible to create INBOX folder
   CHECK( !m_dlgCreate || folderType != MF_INBOX, false,
          _T("Ok button should be disabled") );

   // 0th step: verify if the settings are self-consistent
   wxFolderBaseDialog *dlg = GET_PARENT_OF_CLASS(this, wxFolderBaseDialog);
   CHECK( dlg, false, _T("folder page should be in folder dialog!") );
   if (folderType == MF_FILE && dlg->GetFolderName() == _T("INBOX"))
      folderType = MF_INBOX;

   // is the folder name valid?
   wxString path;
   if ( folderType == MF_MH
#ifdef EXPERIMENTAL_MFormat
        || folderType == MF_MDIR
#endif // EXPERIMENTAL_MFormat
      )
   {
      // MH folder name must be relative to MH root, check it
      path = m_path->GetValue();
      wxString mhName = path;
      if ( !MailFolder::GetMHFolderName(&mhName) )
      {
         wxLogError(_("Impossible to create MH folder '%s'."),
                    path.c_str());

         wxLog::FlushActive();

         return FALSE;
      }
   }

   // folder flags

   bool isGroup = m_isGroup->GetValue();
   bool canBeOpened = m_canBeOpened->GetValue();

   // some flags are not set from here, keep their existing value
   int flags = 0;

   if ( m_keepOpen->IsEnabled() && m_keepOpen->GetValue() )
      flags |= MF_FLAGS_KEEPOPEN;
   if ( m_isHidden->GetValue() )
      flags |= MF_FLAGS_HIDDEN;
   if ( isGroup )
      flags |= MF_FLAGS_GROUP;

   // TODO: for IMAP folders, we should autodetect this flag value by checking
   //       for \NoSelect folder flag on server
   if ( !canBeOpened )
      flags |= MF_FLAGS_NOSELECT;

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
         bool anonymous = m_isAnonymous->GetValue() || loginName == _T("anonymous");
         if ( anonymous )
            flags |= MF_FLAGS_ANON;
         else
         {
            wxString what;    // what did the user forget to specify

            // the key to use for MDialog_YesNoDialog
            const MPersMsgBox *msgbox = NULL; // suppress compiler warning

            if ( !loginName )
            {
               what = _("login name");
               msgbox = M_MSGBOX_ASK_LOGIN;
            }
            else if ( !password )
            {
               what = _("password");
               msgbox = M_MSGBOX_ASK_PWD;
            }

            if ( !what.empty() )
            {
               wxString msg;
               msg.Printf
                   (
                     _("You have not specified a %s for this folder, although "
                       "it requires one.\n"
                       "If you leave it empty, Mahogany asks you for it every "
                       "time when opening this folder and\n"
                       "if you don't want this to happen you should fill "
                       "it in here or,\n"
                       "alternatively, select anonymous access.\n"),
                     what.c_str()
                   );

               if ( msgbox == M_MSGBOX_ASK_PWD )
               {
                  msg << _("\n"
                        "Please do notice however that the passwords are "
                        "stored using weak encryption.\n"
                        "So if you are concerned about security, it is "
                        "indeed better to leave it empty\n"
                        "and let Mahogany prompt you for it "
                        "whenever needed.\n");
               }

               msg << _T('\n')
                   << wxString::Format
                      (
                        _("So would you like to leave the %s empty?"),
                        what.c_str()
                      );

               if ( !MDialog_YesNoDialog(msg, this, MDIALOG_YESNOTITLE,
                                         M_DLG_YES_DEFAULT | M_DLG_NOT_ON_NO,
                                         msgbox) )
               {
                  return false;
               }
            }
         }
      }
   }

   // 1st step: create the folder in the MFolder sense. For this we need only
   // the name and the type

   // FIXME instead of this `if' we should have a virtual function in the
   //       base class to either create or return the folder object
   MFolder *folder;
   if ( m_dlgCreate )
   {
      ASSERT_MSG( m_dlgCreate == dlg, _T("GET_PARENT_OF_CLASS broken?") );

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

   CHECK( folder, false, _T("must have folder by this point") );

   Profile_obj profileDlg(dlg->GetProfile());
   ConfigSource * const config = profileDlg->GetConfigSourceForWriting();

   Profile_obj profileFolder(folder->GetProfile());
   CHECK( profileFolder, false, "folder must have profile here" );
   profileFolder->SetConfigSourceForWriting(config);

   // 2nd step: put what we can in MFolder
   folder->SetComment(m_comment->GetValue());

   // 3rd step: store additional data about the newly created folder directly
   // in the profile

   String fullname = folder->GetFullName();
   m_profile->DecRef();
   m_profile = Profile::CreateProfile(fullname);
   CHECK( m_profile, false, _T("profile creation shouldn't fail") );
   m_profile->SetConfigSourceForWriting(config);

   // some flags are not set from here, keep their existing value
   flags |= GetFolderFlags(READ_CONFIG(m_profile, MP_FOLDER_TYPE)) &
                  (MF_FLAGS_UNACCESSIBLE  |
                   MF_FLAGS_NEWMAILFOLDER |
                   MF_FLAGS_DONTDELETE    |
                   MF_FLAGS_INCOMING      |
                   MF_FLAGS_MONITOR);

   // common for all folders: remember the login info for the folders for which
   // it makes sense or for folder groups (for which we always remember
   // everything)
   if ( (hasUsername && !(flags & MF_FLAGS_ANON)) || folderType == MF_GROUP )
   {
      WriteEntryIfChanged(Username, loginName);
      WriteEntryIfChanged(Password, password);
   }

   m_profile->writeEntry(MP_FOLDER_TYPE, folderType | flags);

   String server = m_server->GetValue();
   if ( folderType == MF_GROUP )
   {
      // the right thing to do is to write the server value into all profile
      // entries: for POP/IMAP server and for NNTP one because like this
      // everybody will inherit it
      WriteEntryIfChanged(ServerNews, server);
      WriteEntryIfChanged(ServerPop, server);
      WriteEntryIfChanged(ServerImap, server);
      WriteEntryIfChanged(HostName, server);
   }
   else if ( FolderTypeHasServer(folderType) )
   {
      FolderProperty serverType;
      switch(folderType)
      {
         default:
            FAIL_MSG( _T("new foldertype with server added") );
            // fall through, otherwise we will crash with uninit serverType
            // anyhow

         case MF_NNTP:
            serverType = ServerNews;
            break;
         case MF_POP:
            serverType = ServerPop;
            break;
         case MF_IMAP:
            serverType = ServerImap;
            break;
      }

      WriteEntryIfChanged(serverType, server);
   }

   switch ( folderType )
   {
      case MF_IMAP:
         // it's a bad idea to always connect to the server from here, now we
         // always assume the the folder does have children - and if later we
         // find out that it doesn't we just reset MF_FLAGS_GROUP then
         WriteEntryIfChanged(Path, m_mailboxname->GetValue());
         break;

      case MF_POP:
         break;

      case MF_NNTP:
      case MF_NEWS:
         WriteEntryIfChanged(Path, m_newsgroup->GetValue());
         break;

      case MF_FILE:
         // write the folder format if needed
         {
            long format = m_folderSubtype->GetSelection();
            ASSERT_MSG( format >= 0 && format < FileMbox_Max,
                        _T("invalid folder format selection") );

            // we used to only write the entry if it was different from the
            // value inherited from parent but it was wrong as a folder of
            // incorrect type risked to be created if the user changed the
            // default folder type after creating a folder but before
            // accessing it - so now always do it (if it changed, of course)
            //
            //if ( format != READ_CONFIG(m_profile, MP_FOLDER_FILE_DRIVER) )
            if ( format != m_originalMboxFormat )
            {
               m_profile->writeEntry(MP_FOLDER_FILE_DRIVER, format);
            }
         }

#ifdef EXPERIMENTAL_MFormat
      case MF_MFILE:
#endif // EXPERIMENTAL_MFormat
         path = m_path->GetValue();
         // fall through

      case MF_MH:
#ifdef EXPERIMENTAL_MFormat
      case MF_MDIR:
#endif // EXPERIMENTAL_MFormat
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
         wxFAIL_MSG(_T("Unexpected folder type."));
   }

   bool isAnonymous = m_isAnonymous->GetValue();
   if ( isAnonymous != m_originalIsAnonymous )
   {
      if ( isAnonymous )
         folder->AddFlags(MF_FLAGS_ANON);
      else
         folder->ResetFlags(MF_FLAGS_ANON);
   }

#ifdef USE_SSL
   WriteEntryIfChanged(SSL, m_useSSL->GetSelection());
   WriteEntryIfChanged(AcceptUnsigned, m_acceptUnsignedSSL->GetValue());
#endif // USE_SSL

   if ( canBeOpened != m_originalCanBeOpened )
   {
      if ( canBeOpened )
         folder->ResetFlags(MF_FLAGS_NOSELECT);
      else
         folder->AddFlags(MF_FLAGS_NOSELECT);
   }

   if ( isGroup != m_originalIsGroup )
   {
      if ( isGroup )
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
      MEventData *data = new MEventFolderTreeChangeData
                             (
                              fullname,
                              MEventFolderTreeChangeData::Create
                             );
      if ( !MEventManager::Dispatch(data) )
      {
         // oops... we're suspended - don't lose the event, add it to the queue
         // instead
         MEventManager::Send(data);
      }
   }

   folder->DecRef();

   // remember the type of this folder - will be the default one the next time
   mApplication->GetProfile()->writeEntry(MP_LAST_CREATED_FOLDER_TYPE, folderType);

   return true;
}

// ----------------------------------------------------------------------------
// wxFolderCreateNotebook
// ----------------------------------------------------------------------------

// should be in sync with the enum FolderCreatePage!
const char *wxFolderCreateNotebook::s_aszImages[] =
{
   "access",
   "ident",
   "network",
   "newmail",
   "compose",
   "reply",
   "folders",
   "msgview",
   "folderview",
   "foldertree",
   "adrbook",
   "helpers",
#ifdef USE_PYTHON
   "python",
#endif // USE_PYTHON
   NULL
};

// create the control and add pages too
wxFolderCreateNotebook::wxFolderCreateNotebook(wxWindow *parent,
                                               wxFolderCreateDialog *dlg)
                      : wxNotebookWithImages
                        (
                         parent,
                         s_aszImages
                        )
{
   // use the parent profile for the default values if we have any
   Profile_obj profile(dlg ? dlg->GetParentFolderName() : String());
   CHECK_RET( profile, _T("failed to create profile in wxFolderCreateNotebook") );

   // create and add the pages
   (void)new wxFolderPropertiesPage(this, profile, dlg);
   (void)new wxOptionsPageIdent(this, profile);
   (void)new wxOptionsPageNetwork(this, profile);
   (void)new wxOptionsPageNewMail(this, profile);
   (void)new wxOptionsPageCompose(this, profile);
   (void)new wxOptionsPageReply(this, profile);
   (void)new wxOptionsPageFolders(this, profile);
   (void)new wxOptionsPageMessageView(this, profile);
   (void)new wxOptionsPageFolderView(this, profile);
   (void)new wxOptionsPageFolderTree(this, profile);
   (void)new wxOptionsPageAdb(this, profile);
   (void)new wxOptionsPageHelpers(this, profile);
#ifdef USE_PYTHON
   (void)new wxOptionsPagePython(this, profile);
#endif // USE_PYTHON

   wxPersistentRegisterAndRestore(this, "FolderCreateNotebook");
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

// helper function: common part of ShowFolderCreateDialog and
// ShowFolderPropertiesDialog
//
// the caller should DecRef() the returned pointer
static MFolder *DoShowFolderDialog(wxFolderBaseDialog& dlg,
                                   FolderCreatePage page)
{
   dlg.ShowPage(page);

   if ( dlg.ShowModal() == wxID_OK )
   {
      MFolder *folder = dlg.GetFolder();

      return folder;
   }
   else
   {
      return (MFolder *)NULL;
   }
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
      ASSERT_MSG( folderNew == folder, _T("unexpected folder change") );

      // as DoShowFolderDialog() calls GetFolder() which calls IncRef() on
      // folder, compensate for it here
      folderNew->DecRef();

      return TRUE;
   }

   // dialog was cancelled
   return FALSE;
}

