//////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxRenameDialog.cpp - implements ShowFolderRenameDialog()
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.09.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M licence
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
#  include "guidef.h"

#  include <wx/log.h>
#  include <wx/layout.h>

#  include <wx/choice.h>
#  include <wx/statbox.h>
#  include <wx/stattext.h>
#  include <wx/radiobox.h>

#  include <wx/textdlg.h>
#endif // USE_PCH

#include <wx/filename.h>

#include "MFolder.h"

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static String GetRenameDialogTitle(const MFolder *folder)
{
   wxString title;
   title.Printf(_("Rename folder '%s'"), folder->GetFullName().c_str());
   return title;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the dialog box allowing to rename the folder
class wxFolderRenameDialog : public wxManuallyLaidOutDialog
{
public:
   wxFolderRenameDialog(wxWindow *parent,
                        const MFolder *folder,
                        String *folderName,
                        String *mboxName);

   // did anything change?
   bool HasChanges() const { return m_hasChanges; }

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   void OnCheckbox(wxCommandEvent& event);
   void OnText(wxCommandEvent& event);

private:
   // enable/disable mbox renaming controls depending on m_chkRenameMbox value
   void DoUpdateUI();

   // set the value of mbox path using the current folder name
   void DoUpdateMboxPath(const String& folderName);

   // data
   const MFolder *m_folder;

   bool m_hasChanges;

   char m_chDelim;         // the folder path delimiter

   String *m_folderName,
          *m_mboxName;

   // GUI controls
   wxTextCtrl *m_textFolder,
              *m_textMbox;

   wxStaticText *m_labelMbox;

   wxCheckBox *m_chkRenameMbox;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxFolderRenameDialog)
};

// ============================================================================
// implementation
// ============================================================================

BEGIN_EVENT_TABLE(wxFolderRenameDialog, wxManuallyLaidOutDialog)
   EVT_CHECKBOX(-1, wxFolderRenameDialog::OnCheckbox)
   EVT_TEXT(-1, wxFolderRenameDialog::OnText)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderRenameDialog
// ----------------------------------------------------------------------------

wxFolderRenameDialog::wxFolderRenameDialog(wxWindow *parent,
                                           const MFolder *folder,
                                           String *folderName,
                                           String *mboxName)
                    : wxManuallyLaidOutDialog(parent,
                                              GetRenameDialogTitle(folder),
                                              "RenameDialog")
{
   // init members
   m_folder = folder;
   m_chDelim = '/';
   m_hasChanges = false;
   m_folderName = folderName;
   m_mboxName = mboxName;

   // first create the box around everything
   wxLayoutConstraints *c;
   wxStaticBox *box = CreateStdButtonsAndBox(_("&Rename folder:"));

   // create the checkbox first because OnText() uses it and so it must be
   // created before the text controls or we'd crash
   m_chkRenameMbox = new wxPCheckBox
                         (
                           "RenameMbox",
                           this,
                           -1,
                           _("Rename mailbox &separately")
                         );

   // create the zone for folder renaming
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   wxStaticText *label = new wxStaticText(this, -1, _("New &folder name:"));
   label->SetConstraints(c);

   *m_folderName = folder->GetName();
   m_textFolder = new wxTextCtrl(this, -1, *m_folderName);
   c = new wxLayoutConstraints;
   c->left.RightOf(label, LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(label, wxCentreY);
   c->height.AsIs();
   m_textFolder->SetConstraints(c);

   // create the explanation message and checkbox enabling/disabling
   // mailbox renaming controls
   wxStaticText *msg = new wxStaticText
                           (
                            this,
                            -1,
                            _("Normally the name of the folder shown "
                              "in the folder tree is just the same as\n"
                              "the name of the underlying mailbox, but "
                              "by activating the checkbox below you may,\n"
                              "if you wish, give them different names.\n"
                              "\n"
                              "Otherwise the mailbox will be renamed to "
                              "the same name as the folder.")
                           );

   c = new wxLayoutConstraints;
   c->left.SameAs(label, wxLeft);
   c->right.SameAs(m_textFolder, wxRight);
   c->top.Below(m_textFolder, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(label, wxLeft);
   c->width.AsIs();
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_chkRenameMbox->SetConstraints(c);

   // create the mbox renaming controls
   c = new wxLayoutConstraints;
   c->left.SameAs(label, wxLeft);
   c->top.Below(m_chkRenameMbox, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_labelMbox = new wxStaticText(this, -1, _("New &mailbox name:"));
   m_labelMbox->SetConstraints(c);

   *m_mboxName = folder->GetPath();
   m_textMbox = new wxTextCtrl(this, -1, *m_mboxName);
   c = new wxLayoutConstraints;
   c->left.RightOf(m_labelMbox, LAYOUT_X_MARGIN);
   c->right.SameAs(m_textFolder, wxRight);
   c->centreY.SameAs(m_labelMbox, wxCentreY);
   c->height.AsIs();
   m_textMbox->SetConstraints(c);

   // set the initial state of the controls
   DoUpdateUI();

   m_textFolder->SetFocus();

   SetDefaultSize(7*wBtn, 10*hBtn);
}

void wxFolderRenameDialog::OnCheckbox(wxCommandEvent& WXUNUSED(event))
{
   DoUpdateUI();
}

void wxFolderRenameDialog::OnText(wxCommandEvent& event)
{
   // update the mailbox path if the user doesn't want to modify it himself
   if ( !m_chkRenameMbox->GetValue() &&
        event.GetEventObject() == m_textFolder )
   {
      DoUpdateMboxPath(event.GetString());
   }

   event.Skip();
}

void wxFolderRenameDialog::DoUpdateMboxPath(const String& folderName)
{
   String mboxName,
          path = m_textMbox->GetValue();
   switch ( m_folder->GetType() )
   {
      case MF_FILE:
      case MF_MH:
         // the file names are more complicated: we have to deal with different
         // delimiters depending on platform and so on
         wxFileName::SplitPath(path, &mboxName, NULL, NULL);
         break;

      default:
         // just take all components but the last one
         mboxName = path.BeforeLast(m_chDelim);
   }

   mboxName << m_chDelim << folderName;

   m_textMbox->SetValue(mboxName);
}

void wxFolderRenameDialog::DoUpdateUI()
{
   // if we have the checkbox, we must have the other two controls as well
   CHECK_RET( m_textMbox && m_labelMbox, _T("where are our controls?") );

   bool enable = m_chkRenameMbox->GetValue();

   m_textMbox->Enable(enable);
   m_labelMbox->Enable(enable);
}

bool wxFolderRenameDialog::TransferDataToWindow()
{
   // get the folder delimiter
   MailFolder *mf = MailFolder::OpenFolder(m_folder, MailFolder::ReadOnly);
   if ( !mf )
   {
      // probably will fail to rename too, don't even try
      wxLogError(_("Impossible to rename the mailbox '%s'."),
                 m_mboxName->c_str());

      return false;
   }

   m_chDelim = mf->GetFolderDelimiter();

   mf->DecRef();

   return true;
}

bool wxFolderRenameDialog::TransferDataFromWindow()
{
   String folderNameNew = m_textFolder->GetValue();
   if ( folderNameNew != *m_folderName )
   {
      m_hasChanges = true;
      *m_folderName = folderNameNew;
   }
   else
   {
      // meaning it didn't change
      m_folderName->clear();
   }

   if ( m_chkRenameMbox->GetValue() )
   {
      String mboxNameNew = m_textMbox->GetValue();
      if ( mboxNameNew != *m_mboxName )
      {
         m_hasChanges = true;
         *m_mboxName = mboxNameNew;
      }
      else
      {
         m_mboxName->clear();
      }
   }
   else // rename the mailbox following the folder
   {
      if ( m_hasChanges )
      {
         // it should be already up to date as it is updated from OnText()
         *m_mboxName = m_textMbox->GetValue();
      }
      //else: nothing to do at all
   }

   return true;
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern bool ShowFolderRenameDialog(const MFolder *folder,
                                   String *folderName,
                                   String *mboxName,
                                   wxWindow *parent)
{
   CHECK( folderName, false, _T("invalid parameter in ShowFolderRenameDialog") );

   // only allow to rename the mailbox if we have a valid mboxName pointer
   // and if the mailbox can be renamed
   //
   // NB: assume that the folder can be renamed if it can be deleted
   MFolderType folderType = folder->GetType();
   if ( mboxName &&
        folderType != MF_GROUP &&
        CanDeleteFolderOfType(folderType) )
   {
      wxFolderRenameDialog dlg(parent, folder, folderName, mboxName);

      return dlg.ShowModal() == wxID_OK && dlg.HasChanges();
   }
   else // rename the folder only
   {
      String name = wxGetTextFromUser
                    (
                     _("New folder name:"),
                     GetRenameDialogTitle(folder),
                     folder->GetName(),
                     parent
                    );

      if ( name.empty() || name == *folderName )
      {
         // cancelled or not changed
         return false;
      }

      *folderName = name;

      return true;
   }
}

