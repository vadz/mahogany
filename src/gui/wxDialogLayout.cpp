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
#   include   "strutil.h"
#   include   "MHelp.h"
#   include   "gui/wxMIds.h"
#endif

#include   <wx/log.h>
#include   <wx/imaglist.h>
#include   <wx/notebook.h>
#include   <wx/persctrl.h>
#include   <wx/control.h>
#include   <wx/dcclient.h>
#include   <wx/layout.h>
#include   <wx/dynarray.h>
#include   <wx/stattext.h>
#include   <wx/settings.h>
#include   <wx/listbox.h>
#include   <wx/checkbox.h>
#include   <wx/radiobox.h>
#include   <wx/combobox.h>
#include   <wx/statbox.h>

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

IMPLEMENT_ABSTRACT_CLASS(wxNotebookDialog, wxDialog)

BEGIN_EVENT_TABLE(wxNotebookDialog, wxDialog)
   EVT_BUTTON(M_WXID_HELP, wxNotebookDialog::OnHelp)
   EVT_BUTTON(wxID_OK,     wxNotebookDialog::OnOK)
   EVT_BUTTON(wxID_APPLY,  wxNotebookDialog::OnApply)
   EVT_BUTTON(wxID_CANCEL, wxNotebookDialog::OnCancel)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxNotebookPageBase, wxPanel)
   EVT_TEXT    (-1, wxNotebookPageBase::OnChange)
   EVT_CHECKBOX(-1, wxNotebookPageBase::OnChange)
   EVT_RADIOBOX(-1, wxNotebookPageBase::OnChange)
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

void wxNotebookPageBase::OnChange(wxEvent& event)
{
   wxNotebookDialog *dlg = GET_PARENT_OF_CLASS(this, wxNotebookDialog);
   if ( dlg )
      dlg->SetDirty();
}

// the top item is positioned near the top of the page, the others are
// positioned from top to bottom, i.e. under the last one
void wxNotebookPageBase::SetTopConstraint(wxLayoutConstraints *c,
                                          wxControl *last,
                                          size_t extraSpace)
{
   if ( last == NULL )
      c->top.SameAs(this, wxTop, 2*LAYOUT_Y_MARGIN + extraSpace);
   else {
      size_t margin = LAYOUT_Y_MARGIN;
      if ( last->IsKindOf(CLASSINFO(wxListBox)) ) {
         // listbox has a surrounding box, so leave more space
         margin *= 2;
      }

      c->top.Below(last, margin + extraSpace);
   }
}

wxTextCtrl *
wxNotebookPageBase::CreateEntryWithButton(const char *label,
                                          long widthMax,
                                          wxControl *last,
                                          wxNotebookPageBase::BtnKind kind,
                                          wxBrowseButton **ppButton)
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
   wxBrowseButton *btn;
   switch ( kind )
   {
      case FileBtn:
         btn = new wxFileBrowseButton(text, this);
         break;

      case ColorBtn:
         btn = new wxColorBrowseButton(text, this);
         break;

      default:
         wxFAIL_MSG("unknown browse button kind");
         return NULL;
   }

   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->centreY.SameAs(text, wxCentreY);
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
                                                    size_t nRightMargin,
                                                    int style)
{
   wxLayoutConstraints *c;

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last, 3); // FIXME shouldn't hardcode this!
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(this, -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   // for the text control
   c = new wxLayoutConstraints;
   c->centreY.SameAs(pLabel, wxCentreY);
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.AsIs();
   wxTextCtrl *pText = new wxTextCtrl(this, -1, "",wxDefaultPosition,
                                      wxDefaultSize, style);
   pText->SetConstraints(c);

   return pText;
}

// create just some text
wxStaticText *wxNotebookPageBase::CreateMessage(const char *label,
                                                wxControl *last)
{
   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(this, -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_LEFT);
   pLabel->SetConstraints(c);

   return pLabel;
}

// create a button
wxButton *wxNotebookPageBase::CreateButton(const wxString& labelAndId,
                                           wxControl *last)
{
   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();

   // split the label into the real label and the button id
   wxString label(labelAndId.BeforeFirst(':')),
            strId(labelAndId.AfterFirst(':'));
   int id;
   if ( !strId || !sscanf(strId, "%d", &id) )
      id = -1;

   wxButton *btn = new wxButton(this, id, label);
   btn->SetConstraints(c);

   return btn;
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

// create a radiobox control with 3 choices and a label for it
wxRadioBox *wxNotebookPageBase::CreateActionChoice(const char *label,
                                                   long widthMax,
                                                   wxControl *last,
                                                   size_t nRightMargin)
{
   wxLayoutConstraints *c;

   // for the radiobox
   c = new wxLayoutConstraints;
   SetTopConstraint(c, last, LAYOUT_Y_MARGIN);
   c->left.SameAs(this, wxLeft, 2*LAYOUT_X_MARGIN + widthMax );
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.AsIs();

   // FIXME we assume that if there other controls dependent on this one in
   //       the options dialog, then only the first choice ("No") means that
   //       they should be disabled
   wxString choices[3];
   choices[0] = _("No");
   choices[1] = _("Ask");
   choices[2] = _("Yes");
   wxRadioBox *radiobox = new wxRadioBox(this, -1, "",
                                         wxDefaultPosition, wxDefaultSize,
                                         3,
                                         choices,
                                         1,wxRA_SPECIFY_ROWS);

   radiobox->SetConstraints(c);

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->centreY.SameAs(radiobox, wxCentreY);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(this, -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   return radiobox;
}

// create a combobox
wxComboBox *wxNotebookPageBase::CreateComboBox(const char *label,
                                               long widthMax,
                                               wxControl *last,
                                               size_t nRightMargin)
{
   // split the "label" into the real label and the choices:
   kbStringList tlist;
   char *buf = strutil_strdup(label);
   strutil_tokenise(buf, ":", tlist);
   delete [] buf;

   size_t n = tlist.size(); // number of elements
   wxString *choices = new wxString[n];
   wxString *sptr;
   size_t i = 0;
   for(i = 0; i < n; i++)
   {
      sptr = tlist.pop_front();
      choices[i] = *sptr;
      delete sptr;
   }
   // the real choices start at 1, 0 is the label

   wxLayoutConstraints *c;

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last, LAYOUT_Y_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(this, -1, choices[0],
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   // for the combobox
   c = new wxLayoutConstraints;

   c->centreY.SameAs(pLabel, wxCentreY);
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.AsIs();
   wxComboBox *combobox = new wxComboBox(this, -1, "",
                                         wxDefaultPosition, wxDefaultSize,
                                         n-1,
                                         choices+1,
                                         wxCB_DROPDOWN|wxCB_READONLY);

   combobox->SetConstraints(c);
   delete [] choices;
   return combobox;
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
   wxButton *button = NULL;
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

   widthMax += 15; // loks better like this
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

// enable/disable the text control and its label
void wxNotebookPageBase::EnableTextWithButton(wxTextCtrl *control, bool bEnable)
{
   // NB: we assume that the control ids are consecutif
   long id = control->GetId() + 1;
   wxWindow *win = FindWindow(id);

   if ( win == NULL ) {
      wxFAIL_MSG("can't find browse button for the text entry zone");
   }
   else {
      wxASSERT( win->IsKindOf(CLASSINFO(wxButton)) );

      win->Enable(bEnable);
   }

   EnableTextWithLabel(control, bEnable);
}

// enable/disable the text control with label and button
void wxNotebookPageBase::EnableTextWithLabel(wxTextCtrl *control, bool bEnable)
{
   // not only enable/disable it, but also make (un)editable because it gives
   // visual feedback
   control->SetEditable(bEnable);

   // disable the label too: this will grey it out

   // NB: we assume that the control ids are consecutif
   long id = control->GetId() - 1;
   wxWindow *win = FindWindow(id);

   if ( win == NULL ) {
      wxFAIL_MSG("can't find label for the text entry zone");
   }
   else {
      // did we find the right one?
      wxASSERT( win->IsKindOf(CLASSINFO(wxStaticText)) );

      win->Enable(bEnable);
   }
}

// -----------------------------------------------------------------------------
// wxNotebookDialog
// -----------------------------------------------------------------------------

wxNotebookDialog::wxNotebookDialog(wxFrame *parent, const wxString& title)
                : wxDialog(parent, -1, title,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
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
   const int hDlg = 27*hBtn;

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
   m_btnHelp = new wxButton(panel, M_WXID_HELP, _("&Help"));
   c = new wxLayoutConstraints;
   // 5 to have extra space:
   c->left.SameAs(panel, wxLeft, (LAYOUT_X_MARGIN));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
   m_btnHelp->SetConstraints(c);

   m_btnOk = new wxButton(panel, wxID_OK, _("&OK"));
   m_btnOk->SetDefault();
   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxRight, -3*(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
   m_btnOk->SetConstraints(c);

   wxButton *btn = new wxButton(panel, wxID_CANCEL, _("&Cancel"));
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

void wxNotebookDialog::OnHelp(wxCommandEvent& /* event */)
{
   int pid = m_notebook->GetSelection();
   if(pid == -1) // no page selected??
      mApplication->Help(MH_OPTIONSNOTEBOOK,this);
   else
      mApplication->Help(((wxOptionsPage *)m_notebook->GetPage(pid))->HelpId(),this);
}
