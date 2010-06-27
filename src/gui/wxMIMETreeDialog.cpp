///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMIMETreeDialog.cpp: implements MIME tree dialog
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-08-13 (extracted from src/gui/wxMsgCmdProc.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2004 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     Mahogany license
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

   #include <wx/layout.h>
   #include <wx/statbox.h>
   #include <wx/textctrl.h>
   
   #include <gui/wxIconManager.h>
   #include <wx/sizer.h>
   #include "guidef.h"
   #include "pointers.h"
   #include <wx/filename.h>
   #include <wx/filedlg.h>
#endif // USE_PCH

#include <wx/imaglist.h>
#include <gui/wxDialogLayout.h>

#include "ClickAtt.h"

#include "HeaderInfo.h"
#include "MFolder.h"
#include "MailFolder.h"
#include "MessageView.h"
#include "MimePart.h"
#include "UIdArray.h"

#include "MIMETreeDialog.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   wxMIMETree_BtnSave = 100
};

// ----------------------------------------------------------------------------
// tree client data containing the associated MIME part
// ----------------------------------------------------------------------------

class wxMIMETreeData : public wxTreeItemData
{
public:
   wxMIMETreeData(const MimePart *mimepart) { m_mimepart = mimepart; }

   const MimePart *GetMimePart() const { return m_mimepart; }

private:
   const MimePart *m_mimepart;
};

// ----------------------------------------------------------------------------
// tree containing MIME parts
// ----------------------------------------------------------------------------

class wxMIMETreeCtrl : public wxTreeCtrl
{
public:
   wxMIMETreeCtrl(wxWindow *parent);

   // return the MIME part associated with the given item
   const MimePart *GetMIMEData(const wxTreeItemId& id) const
   {
      return (static_cast<wxMIMETreeData *>(GetItemData(id)))->GetMimePart();
   }

private:
   DECLARE_NO_COPY_CLASS(wxMIMETreeCtrl)
};

// ----------------------------------------------------------------------------
// MIME tree dialog itself
// ----------------------------------------------------------------------------

class wxMIMETreeDialog : public wxPDialog
{
public:
   wxMIMETreeDialog(const MimePart *partRoot,
                    wxWindow *parent,
                    MessageView *msgView);

protected:
   // event handlers
   void OnTreeItemRightClick(wxTreeEvent& event);
   void OnSave(wxCommandEvent& event);

   // save selected attachments (to files) or messages (to folder)
   void SaveAttachments(size_t count, const MimePart **parts);
   void SaveMessages(size_t count, const MimePart **parts);

private:
   // fill the tree
   void AddToTree(wxTreeItemId id, const MimePart *mimepart);

   // total number of parts
   size_t m_countParts;

   // GUI controls
   wxMIMETreeCtrl *m_treectrl;

   // the parent message view
   MessageView *m_msgView;


   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxMIMETreeDialog)
};

BEGIN_EVENT_TABLE(wxMIMETreeDialog, wxPDialog)
   EVT_TREE_ITEM_RIGHT_CLICK(-1, wxMIMETreeDialog::OnTreeItemRightClick)

   EVT_BUTTON(wxMIMETree_BtnSave, wxMIMETreeDialog::OnSave)
END_EVENT_TABLE()


// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMIMETreeCtrl
// ----------------------------------------------------------------------------

wxMIMETreeCtrl::wxMIMETreeCtrl(wxWindow *parent)
              : wxTreeCtrl
                (
                  parent,
                  -1,
                  wxDefaultPosition,
                  wxSize(400, 200),
                  wxTR_DEFAULT_STYLE | wxTR_MULTIPLE
                )
{
   wxIconManager *iconManager = mApplication->GetIconManager();

   wxImageList *imaglist = new wxImageList(32, 32);

   for ( int i = MimeType::TEXT; i <= MimeType::OTHER; i++ )
   {
      // this is a big ugly: we need to get just the primary MIME type
      MimeType mt((MimeType::Primary)i, wxEmptyString);
      wxIcon icon = iconManager->GetIconFromMimeType(mt.GetType());
      imaglist->Add(icon);
   }

   // and the last icon for the unknown MIME images
   imaglist->Add(iconManager->GetIcon(_T("unknown")));

   AssignImageList(imaglist);
}

// ----------------------------------------------------------------------------
// wxMIMETreeDialog
// ----------------------------------------------------------------------------

wxMIMETreeDialog::wxMIMETreeDialog(const MimePart *partRoot,
                                   wxWindow *parent,
                                   MessageView *msgView)
                : wxPDialog
                  (
                     _T("MimeTreeDialog"),
                     parent,
                     String(M_TITLE_PREFIX) + _("MIME structure of the message"),
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER
                  )
{
   // init members
   m_msgView = msgView;
   m_countParts = 0;
   m_treectrl = new wxMIMETreeCtrl(this);


   // create and lay out the controls
   // -------------------------------

   // the box containing the tree control
   wxStaticBox *box = new wxStaticBox(this, wxID_ANY, wxEmptyString);
   wxStaticBoxSizer *sizerBox = new wxStaticBoxSizer(box, wxHORIZONTAL);
   sizerBox->Add(m_treectrl, 1, wxEXPAND | wxALL, LAYOUT_MARGIN);

   // the buttons
   wxSizer *sizerButtons = new wxBoxSizer(wxHORIZONTAL);
   sizerButtons->AddStretchSpacer();
   if ( m_msgView )
   {
      sizerButtons->Add(new wxButton(this, wxMIMETree_BtnSave, _("&Save...")),
                           0, wxALL, LAYOUT_MARGIN);
      sizerButtons->AddSpacer(2*LAYOUT_X_MARGIN);
   }
   sizerButtons->Add(new wxButton(this, wxID_CANCEL, _("&Close")),
                        0, wxALL, LAYOUT_MARGIN);

   // put everything together
   wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
   sizerTop->Add(sizerBox, 1, wxEXPAND | wxALL, LAYOUT_MARGIN);
   sizerTop->Add(sizerButtons, 0, wxEXPAND | (wxALL & ~wxTOP), LAYOUT_MARGIN);


   // initialize the tree
   AddToTree(wxTreeItemId(), partRoot);

   m_treectrl->Expand(m_treectrl->GetRootItem());

   box->SetLabel(wxString::Format(_("%lu MIME parts"),
                                  static_cast<unsigned long>(m_countParts)));


   SetSizer(sizerTop);
   sizerTop->SetSizeHints(this);
}

void
wxMIMETreeDialog::AddToTree(wxTreeItemId idParent, const MimePart *mimepart)
{
   int image = mimepart->GetType().GetPrimary();
   if ( image > MimeType::OTHER + 1 )
   {
      image = MimeType::OTHER + 1;
   }

   wxString label = ClickableAttachment::GetLabelFor(mimepart);

   wxTreeItemId id  = m_treectrl->AppendItem(idParent,
                                             label,
                                             image, image,
                                             new wxMIMETreeData(mimepart));

   m_countParts++;

   const MimePart *partChild = mimepart->GetNested();
   while ( partChild )
   {
      AddToTree(id, partChild);

      partChild = partChild->GetNext();
   }
}

void wxMIMETreeDialog::OnTreeItemRightClick(wxTreeEvent& event)
{
   if ( m_msgView )
   {
      ClickableAttachment att(m_msgView,
                              m_treectrl->GetMIMEData(event.GetItem()));
      att.ShowPopupMenu(m_treectrl, event.GetPoint());
   }
}

void wxMIMETreeDialog::OnSave(wxCommandEvent& WXUNUSED(event))
{
   CHECK_RET( m_msgView, _T("button shouldn't exist if no msg view") );

   // get the selected items
   wxArrayTreeItemIds selections;
   const size_t count = m_treectrl->GetSelections(selections);
   if ( !count )
      return;

   // get all MIME parts to save and also check if all of them are messages
   bool allMsgs = true;
   size_t nParts = 0;
   scoped_array<const MimePart *> parts(new const MimePart *[count]);
   for ( size_t n = 0; n < count; n++ )
   {
      const MimePart *mimepart = m_treectrl->GetMIMEData(selections[n]);
      if ( !mimepart )
      {
         wxLogWarning(_("Failed to save MIME part \"%s\", skipping."),
                      m_treectrl->GetItemText(selections[n]).c_str());
         continue;
      }

      parts[nParts++] = mimepart;

      allMsgs &= mimepart->GetType().GetPrimary() == MimeType::MESSAGE;
   }

   // if they're, then we're going to copy them to another folder instead of
   // saving them to file(s)
   if ( allMsgs )
   {
      SaveMessages(nParts, parts.get());
   }
   else // save attachments to file(s)
   {
      SaveAttachments(nParts, parts.get());
   }
}

void wxMIMETreeDialog::SaveMessages(size_t count, const MimePart **parts)
{
   // get the folder to save messages to
   MFolder_obj folderDst(MDialog_FolderChoose(this));
   if ( !folderDst )
   {
      // cancelled by user
      return;
   }

   // copy them to a temp file
   wxString filename = wxFileName::CreateTempFileName("Mtemp");
   if ( filename.empty() )
   {
      wxLogError(_("Failed to save messages to temporary file."));

      return;
   }

   for ( size_t n = 0; n < count; n++ )
   {
      const MimePart * const mimepart = parts[n];

      unsigned long len;
      const void *content = mimepart->GetContent(&len);
      if ( !content || !MailFolder::SaveMessageAsMBOX(filename, content, len) )
      {
         wxLogWarning(_("Failed to save attachment %u"), (unsigned)n);
      }
   }

   // now copy them from there to the folder
   MFolder_obj folderSrc(MFolder::CreateTempFile(wxEmptyString, filename));
   if ( folderSrc )
   {
      MailFolder_obj mf(MailFolder::OpenFolder(folderSrc));
      if ( mf )
      {
         // FIXME:horrible inefficient... we really should have some
         //       MailFolder::SaveAllMessages() as we don't need UIDs here at
         //       all, just msgnos
         HeaderInfoList_obj hil(mf->GetHeaders());
         if ( hil )
         {
            const size_t count = hil->Count();
            UIdArray all;
            all.Alloc(count);
            for ( size_t n = 0; n < count; n++ )
            {
               HeaderInfo *hi = hil->GetItemByIndex(n);
               if ( hi )
                  all.Add(hi->GetUId());
            }

            if ( mf->SaveMessages(&all, folderDst) )
            {
               // everything is ok
               return;
            }
         }
      }
   }

   // if we got here, something failed above
   wxLogError(_("Copying messages failed."));
}

void wxMIMETreeDialog::SaveAttachments(size_t count, const MimePart **parts)
{
   String dir = MDialog_DirRequester
                (
                  _("Choose directory to save attachments to:"),
                  wxEmptyString,
                  this,
                  "MimeSave"
                );
   if ( dir.empty() )
   {
      // cancelled by user
      return;
   }

   for ( size_t n = 0; n < count; n++ )
   {
      const MimePart * const mimepart = parts[n];

      String name = mimepart->GetFilename();
      bool needToAsk = name.empty();

      wxFileName fn(dir, name);
      if ( !needToAsk )
      {
         // never ever overwrite files silently here, this is a security
         // risk
         needToAsk = fn.FileExists();
      }

      String filename;
      if ( needToAsk )
      {
         filename = wxPSaveFileSelector
                    (
                        this,
                        _T("MimeSave"),
                        _("Attachment has a name of existing file,\n"
                          "please choose another one:"),
                        dir  // default path
                    );

         if ( filename.empty() )
         {
            // cancelled
            continue;
         }
      }
      else
      {
         filename = fn.GetFullPath();
      }

      if ( !m_msgView->MimeSave(mimepart, filename) )
      {
         wxLogWarning(_("Failed to save attachment %u"), (unsigned)n);
      }
   }
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

void
ShowMIMETreeDialog(const MimePart *part, wxFrame *parent, MessageView *msgView)
{
   CHECK_RET( part, _T("ShowMIMETreeDialog(): NULL MimePart") );

   wxMIMETreeDialog dialog(part, parent, msgView);

   (void)dialog.ShowModal();
}
