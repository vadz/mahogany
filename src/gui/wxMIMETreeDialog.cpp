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
#endif // USE_PCH

#include <wx/imaglist.h>
#include <gui/wxDialogLayout.h>

#include "ClickAtt.h"

#include "MimePart.h"
#include "MIMETreeDialog.h"

// ----------------------------------------------------------------------------
// MIME dialog classes
// ----------------------------------------------------------------------------

class wxMIMETreeCtrl : public wxTreeCtrl
{
public:
   wxMIMETreeCtrl(wxWindow *parent) : wxTreeCtrl(parent, -1)
   {
      InitImageLists();
   }

private:
   void InitImageLists();

   DECLARE_NO_COPY_CLASS(wxMIMETreeCtrl)
};

class wxMIMETreeData : public wxTreeItemData
{
public:
   wxMIMETreeData(const MimePart *mimepart) { m_mimepart = mimepart; }

   const MimePart *GetMimePart() const { return m_mimepart; }

private:
   const MimePart *m_mimepart;
};

class wxMIMETreeDialog : public wxManuallyLaidOutDialog
{
public:
   wxMIMETreeDialog(const MimePart *partRoot,
                    wxWindow *parent,
                    MessageView *msgView);

protected:
   // event handlers
   void OnTreeItemRightClick(wxTreeEvent& event);

private:
   // fill the tree
   void AddToTree(wxTreeItemId id, const MimePart *mimepart);

   // total number of parts
   size_t m_countParts;

   // GUI controls
   wxStaticBox *m_box;
   wxTreeCtrl *m_treectrl;

   // the parent message view
   MessageView *m_msgView;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxMIMETreeDialog)
};

BEGIN_EVENT_TABLE(wxMIMETreeDialog, wxManuallyLaidOutDialog)
   EVT_TREE_ITEM_RIGHT_CLICK(-1, wxMIMETreeDialog::OnTreeItemRightClick)
END_EVENT_TABLE()


// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMIMETreeCtrl
// ----------------------------------------------------------------------------

void wxMIMETreeCtrl::InitImageLists()
{
   wxIconManager *iconManager = mApplication->GetIconManager();

   wxImageList *imaglist = new wxImageList(32, 32);

   for ( int i = MimeType::TEXT; i <= MimeType::OTHER; i++ )
   {
      // this is a big ugly: we need to get just the primary MIME type
      MimeType mt((MimeType::Primary)i, _T(""));
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
                : wxManuallyLaidOutDialog(parent,
                                          _("MIME structure of the message"),
                                          _T("MimeTreeDialog"))
{
   // init members
   m_msgView = msgView;
   m_countParts = 0;
   m_box = NULL;
   m_treectrl = NULL;

   // create controls
   wxLayoutConstraints *c;

   // label will be set later below
   m_box = CreateStdButtonsAndBox(_T(""), StdBtn_NoCancel);

   m_treectrl = new wxMIMETreeCtrl(this);
   c = new wxLayoutConstraints;
   c->top.SameAs(m_box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(m_box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_treectrl->SetConstraints(c);

   // initialize them
   AddToTree(wxTreeItemId(), partRoot);

   m_treectrl->Expand(m_treectrl->GetRootItem());

   m_box->SetLabel(wxString::Format(_("%u MIME parts"), m_countParts));

   SetDefaultSize(4*wBtn, 10*hBtn);
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
      wxMIMETreeData *data =
         (wxMIMETreeData *)m_treectrl->GetItemData(event.GetItem());

      ClickableAttachment att(m_msgView, data->GetMimePart());
      att.ShowPopupMenu(m_treectrl, event.GetPoint());
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
