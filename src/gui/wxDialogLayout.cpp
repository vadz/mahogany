///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxDialogLayout.cpp - implementation of functions from
//              wxDialogLayout.h
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
#   include   "Mcommon.h"
#   include   "MApplication.h"
#   include   "Profile.h"
#   include   "guidef.h"
#endif

#include   <wx/log.h>
#include   <wx/imaglist.h>
#include   <wx/notebook.h>
#include   <wx/persctrl.h>

#include   "gui/wxIconManager.h"
#include   "gui/wxDialogLayout.h"
#include   "gui/wxOptionsPage.h"
#include   "gui/wxBrowseButton.h"

// ============================================================================
// implementation
// ============================================================================

// -----------------------------------------------------------------------------
// event tables
// -----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxNotebookDialog, wxDialog)
   EVT_BUTTON(wxID_OK,     wxNotebookDialog::OnOK)
   EVT_BUTTON(wxID_APPLY,  wxNotebookDialog::OnApply)
   EVT_BUTTON(wxID_CANCEL, wxNotebookDialog::OnCancel)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// FIXME this has nothing to do here...
wxWindow *GetParentOfClass(wxWindow *win, wxClassInfo *classinfo)
{
   // find the frame we're in
   while ( win && !win->IsKindOf(classinfo) ) {
      win = win->GetParent();
   }

   // may be NULL!
   return win;
}

long GetMaxLabelWidth(const wxArrayString& labels, wxWindow *win)
{
   wxClientDC dc(win);
   dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
   long width, widthMax = 0;

   size_t nCount = labels.Count();
   for ( size_t n = 0; n < nCount; n++ )
   {
      dc.GetTextExtent(labels[n], &width, NULL);
      if ( width > widthMax )
         widthMax = width;
   }

   return widthMax;
}

// ----------------------------------------------------------------------------
// wxNotebookWithImages
// ----------------------------------------------------------------------------

wxNotebookWithImages::wxNotebookWithImages(const wxString& configPath,
                                           wxWindow *parent,
                                           const char *aszImages[])
                    : wxPNotebook(configPath, parent, -1)
{
   wxImageList *imageList = new wxImageList(32, 32, FALSE, WXSIZEOF(aszImages));
   wxIconManager *iconmanager = mApplication->GetIconManager();

   for ( size_t n = 0; aszImages[n]; n++ ) {
      imageList->Add(iconmanager->GetBitmap(aszImages[n]));
   }

   SetImageList(imageList);
}

wxNotebookWithImages::~wxNotebookWithImages()
{
   delete GetImageList();
}

// ----------------------------------------------------------------------------
// wxNotebookPageBase
// ----------------------------------------------------------------------------

// the top item is positioned near the top of the page, the others are
// positioned from top to bottom, i.e. under the last one
void wxNotebookPageBase::SetTopConstraint(wxLayoutConstraints *c,
                                         wxControl *last)
{
   if ( last == NULL )
      c->top.SameAs(this, wxTop, 2*LAYOUT_Y_MARGIN);
   else {
      size_t margin = LAYOUT_Y_MARGIN;
      if ( last->IsKindOf(CLASSINFO(wxListBox)) ) {
         // listbox has a surrounding box, so leave more space
         margin *= 2;
      }

      c->top.Below(last, margin);
   }
}

wxTextCtrl *wxNotebookPageBase::CreateFileEntry(const char *label,
                                               long widthMax,
                                               wxControl *last,
                                               wxFileBrowseButton **ppButton)
{
   static size_t widthBtn = 0;
   if ( widthBtn == 0 ) {
      // calculate it only once, it's almost a constant
      widthBtn = 2*GetCharWidth();
   }

   // create the label and text zone, as usually
   wxTextCtrl *text = CreateTextWithLabel(label, widthMax, last,
                                          widthBtn + 2*LAYOUT_X_MARGIN);

   // and also create a button for browsing for file
   wxFileBrowseButton *btn = new wxFileBrowseButton(text, this);
   wxLayoutConstraints *c = new wxLayoutConstraints;
   SetTopConstraint(c, last);
   c->left.RightOf(text, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->height.SameAs(text, wxHeight);
   btn->SetConstraints(c);

   if ( ppButton )
   {
      *ppButton = btn;
   }

   return text;
}

// create a single-line text control with a label
wxTextCtrl *wxNotebookPageBase::CreateTextWithLabel(const char *label,
                                                    long widthMax,
                                                    wxControl *last,
                                                    size_t nRightMargin)
{
   wxLayoutConstraints *c;

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(this, -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   // for the text control
   c = new wxLayoutConstraints;
   SetTopConstraint(c, last);
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.AsIs();
   wxTextCtrl *pText = new wxTextCtrl(this, -1, "");
   pText->SetConstraints(c);

   return pText;
}

// create a checkbox
wxCheckBox *wxNotebookPageBase::CreateCheckBox(const char *label,
                                              long widthMax,
                                              wxControl *last)
{
   static size_t widthCheck = 0;
   if ( widthCheck == 0 ) {
      // calculate it only once, it's almost a constant
      widthCheck = AdjustCharHeight(GetCharHeight()) + 1;
   }

   wxCheckBox *checkbox = new wxCheckBox(this, -1, label,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxALIGN_RIGHT);

   wxLayoutConstraints *c = new wxLayoutConstraints;
   SetTopConstraint(c, last);
   c->width.AsIs();
   c->right.SameAs(this, wxLeft, -(int)(2*LAYOUT_X_MARGIN + widthMax
                                        + widthCheck));
   c->height.AsIs();

   checkbox->SetConstraints(c);

   return checkbox;
}

// create a listbox and the buttons to work with it
// NB: we consider that there is only one listbox (at most) per page, so
//     the button ids are always the same
wxListBox *wxNotebookPageBase::CreateListbox(const char *label,
                                            wxControl *last)
{
   // a box around all this stuff
   wxStaticBox *box = new wxStaticBox(this, -1, label);

   wxLayoutConstraints *c;
   c = new wxLayoutConstraints;
   SetTopConstraint(c, last);
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->height.PercentOf(this, wxHeight, 50);
   box->SetConstraints(c);

   // the buttons vertically on the right of listbox
   wxButton *button;
   static const char *aszLabels[] =
   {
      "&Add",
      "&Modify",
      "&Delete",
   };

   // determine the longest button label
   wxArrayString labels;
   size_t nBtn;
   for ( nBtn = 0; nBtn < WXSIZEOF(aszLabels); nBtn++ ) {
      labels.Add(_(aszLabels[nBtn]));
   }

   long widthMax = GetMaxLabelWidth(labels, this);

   widthMax += 15; // @@ loks better like this
   for ( nBtn = 0; nBtn < WXSIZEOF(aszLabels); nBtn++ ) {
      c = new wxLayoutConstraints;
      if ( nBtn == 0 )
         c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
      else
         c->top.Below(button, LAYOUT_Y_MARGIN);
      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      c->width.Absolute(widthMax);
      c->height.AsIs();
      button = new wxButton(this, wxOptionsPage_BtnNew + nBtn, labels[nBtn]);
      button->SetConstraints(c);
   }

   // and the listbox itself
   wxListBox *listbox = new wxListBox(this, -1);
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, LAYOUT_X_MARGIN);
   c->right.LeftOf(button, LAYOUT_X_MARGIN);;
   c->bottom.SameAs(box, wxBottom, LAYOUT_Y_MARGIN);
   listbox->SetConstraints(c);

   return listbox;
}


// -----------------------------------------------------------------------------
// wxNotebookDialog
// -----------------------------------------------------------------------------

wxNotebookDialog::wxNotebookDialog(wxFrame *parent, const wxString& title)
                : wxDialog(parent, -1, title)
{
   SetAutoLayout(TRUE);

   m_btnOk =
   m_btnApply = NULL;
}

void wxNotebookDialog::CreateAllControls()
{
   wxLayoutConstraints *c;

   // calculate the controls size
   // ---------------------------

   // basic unit is the height of a char, from this we fix the sizes of all
   // other controls
   size_t heightLabel = AdjustCharHeight(GetCharHeight());
   int hBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       wBtn = BUTTON_WIDTH_FROM_HEIGHT(hBtn);

   // FIXME these are more or less arbitrary numbers
   const int wDlg = 6*wBtn;
   const int hDlg = 25*hBtn;

   // create the panel
   // ----------------
   wxPanel *panel = new wxPanel(this, -1);
   panel->SetAutoLayout(TRUE);
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft);
   c->right.SameAs(this, wxRight);
   c->top.SameAs(this, wxTop);
   c->bottom.SameAs(this, wxBottom);
   panel->SetConstraints(c);

   // optional controls above the notebook
   wxControl *last = CreateControlsAbove(panel);

   // the notebook itself is created by this function
   CreateNotebook(panel);

   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
   if ( last )
      c->top.SameAs(last, wxBottom, 2*LAYOUT_Y_MARGIN);
   else
      c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
   c->bottom.SameAs(panel, wxBottom, 2*LAYOUT_Y_MARGIN + hBtn);
   m_notebook->SetConstraints(c);

   // create the buttons
   // ------------------

   // we need to create them from left to right to have the correct tab order
   // (although it would have been easier to do it from right to left)
   m_btnOk = new wxButton(panel, wxID_OK, _("OK"));
   m_btnOk->SetDefault();
   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxRight, -3*(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
   m_btnOk->SetConstraints(c);

   wxButton *btn = new wxButton(panel, wxID_CANCEL, _("Cancel"));
   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxRight, -2*(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
   btn->SetConstraints(c);

   m_btnApply = new wxButton(panel, wxID_APPLY, _("&Apply"));
   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxRight, -(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
   m_btnApply->SetConstraints(c);

   Layout();

   // set position
   // ------------
   wxWindow::SetSize(wDlg, hDlg);
   SetSizeHints(wDlg, hDlg);
   Centre(wxCENTER_FRAME | wxBOTH);
}

// transfer the data to/from notebook pages
// ----------------------------------------
bool wxNotebookDialog::TransferDataToWindow()
{
   for ( int nPage = 0; nPage < m_notebook->GetPageCount(); nPage++ ) {
      if ( !m_notebook->GetPage(nPage)->TransferDataToWindow() ) {
         return FALSE;
      }
   }

   ResetDirty();

   return TRUE;
}

bool wxNotebookDialog::TransferDataFromWindow()
{
   for ( int nPage = 0; nPage < m_notebook->GetPageCount(); nPage++ )
   {
      if ( !m_notebook->GetPage(nPage)->TransferDataFromWindow() )
         return FALSE;
   }

   return TRUE;
}

// button event handlers
// ---------------------
void wxNotebookDialog::OnOK(wxCommandEvent& event)
{
   if ( m_bDirty )
      OnApply(event);

   if ( !m_bDirty )
   {
      // ok, changes accepted (or there were no changes to begin with)
      EndModal(TRUE);
   }
   //else: test done from OnApply() failed, don't close the dialog
}

void wxNotebookDialog::OnApply(wxCommandEvent& /* event */)
{
   ASSERT_MSG( m_bDirty, "'Apply' should be disabled!" );

   TransferDataFromWindow();

   if ( OnSettingsChange() )
   {
      m_bDirty = FALSE;
      m_btnApply->Enable(FALSE);
   }
   else
   {
      // don't reset the m_bDirty flag so that OnOk() will know we failed
   }
}

void wxNotebookDialog::OnCancel(wxCommandEvent& /* event */)
{
   // FIXME this should restore the old settings, even if "Apply" has been
   //       done!
   EndModal(FALSE);
}
