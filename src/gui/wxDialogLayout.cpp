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
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "guidef.h"
#  include "strutil.h"
#  include "MHelp.h"
#  include "gui/wxMIds.h"

#  include <wx/log.h>
#  include <wx/choice.h>
#  include <wx/control.h>
#  include <wx/dcclient.h>
#  include <wx/layout.h>
#  include <wx/dynarray.h>
#  include <wx/stattext.h>
#  include <wx/settings.h>
#  include <wx/listbox.h>
#  include <wx/checkbox.h>
#  include <wx/radiobox.h>
#  include <wx/combobox.h>
#  include <wx/statbox.h>
#  include <wx/statbmp.h>
#endif

#include <wx/imaglist.h>
#include <wx/notebook.h>
#include "wx/persctrl.h"

#include <wx/menuitem.h>
#include <wx/checklst.h>

#include "gui/wxIconManager.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxBrowseButton.h"

#include "MEvent.h"

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static size_t GetBrowseButtonWidth(wxWindow *win);

// ============================================================================
// implementation
// ============================================================================

// -----------------------------------------------------------------------------
// event tables
// -----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxOptionsPageSubdialog, wxProfileSettingsEditDialog)
   EVT_CHECKBOX(-1, wxOptionsPageSubdialog::OnChange)
   EVT_RADIOBOX(-1, wxOptionsPageSubdialog::OnChange)
   EVT_TEXT(-1,     wxOptionsPageSubdialog::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxNotebookDialog, wxDialog)
   EVT_BUTTON(M_WXID_HELP, wxNotebookDialog::OnHelp)
   EVT_BUTTON(wxID_OK,     wxNotebookDialog::OnOK)
   EVT_BUTTON(wxID_APPLY,  wxNotebookDialog::OnApply)
   EVT_BUTTON(wxID_CANCEL, wxNotebookDialog::OnCancel)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxEnhancedPanel, wxPanel)
   EVT_SIZE(wxEnhancedPanel::OnSize)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxNotebookPageBase, wxEnhancedPanel)
   EVT_TEXT    (-1, wxNotebookPageBase::OnChange)
   EVT_CHECKBOX(-1, wxNotebookPageBase::OnChange)
   EVT_RADIOBOX(-1, wxNotebookPageBase::OnChange)
   EVT_COMBOBOX(-1, wxNotebookPageBase::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxManuallyLaidOutDialog, wxDialog)
   EVT_BUTTON  (wxID_HELP, wxManuallyLaidOutDialog::OnHelp)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxManuallyLaidOutDialog, wxDialog)
IMPLEMENT_ABSTRACT_CLASS(wxNotebookDialog, wxManuallyLaidOutDialog)

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// FIXME this has nothing to do here...
wxWindow *GetParentOfClass(const wxWindow *win, wxClassInfo *classinfo)
{
   // find the frame we're in
   while ( win && !win->IsKindOf(classinfo) ) {
      win = win->GetParent();
   }

   // may be NULL!
   return (wxWindow *)win; // const_cast
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
// wxPDialog
// ----------------------------------------------------------------------------

wxPDialog::wxPDialog(const wxString& profileKey,
                     wxWindow *parent,
                     const wxString& title,
                     long style)
         : wxSMDialog(parent, -1, title,
                    wxDefaultPosition, wxDefaultSize,
                    style),
           m_profileKey(profileKey)
{
   int x, y, w, h;

   m_didRestoreSize = !m_profileKey.IsEmpty() &&
                      wxMFrame::RestorePosition(m_profileKey, &x, &y, &w, &h);

   if ( m_didRestoreSize )
   {
      SetSize(x, y, w, h);
   }
   //else: it should be set by the derived class or the default size will be
   //      used - not too bad neither
}

wxPDialog::~wxPDialog()
{
   if ( !m_profileKey.IsEmpty() )
   {
      wxMFrame::SavePosition(m_profileKey, this);
   }
}

// ----------------------------------------------------------------------------
// wxOptionsPageSubdialog - the common base class all the dialogs which may be
// shown from the options dialog (i.e. whose parent is an options page)
// ----------------------------------------------------------------------------

wxOptionsPageSubdialog::wxOptionsPageSubdialog(Profile *profile,
                                               wxWindow *parent,
                                               const wxString& label,
                                               const wxString& windowName)
                      : wxProfileSettingsEditDialog
                        (
                           profile,
                           windowName,
                           GET_PARENT_OF_CLASS(parent, wxFrame),
                           label
                        )
{
}

void wxOptionsPageSubdialog::OnChange(wxEvent&)
{
   // we don't do anything, but just eat these messages - otherwise they will
   // confuse wxOptionsPage which is our parent because it only processes
   // messages from its own controls
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
// wxEnhancedPanel
// ----------------------------------------------------------------------------

wxEnhancedPanel::wxEnhancedPanel(wxWindow *parent, bool enableScrolling)
               : wxPanel(parent, -1), m_canvas(NULL)
{
   if ( enableScrolling )
   {
      m_canvas = new wxScrolledWindow(this);
   }
   //else: no need for it... just use the panel directly (canvas is already
   //      initialized to NULL above)
}

void wxEnhancedPanel::OnSize(wxSizeEvent& event)
{
   if ( !m_canvas )
   {
      event.Skip();

      return;
   }

#ifdef __WXGTK__
   // FIXME the notebook pages get some bogus size events under wxGTK...
   if ( event.GetSize() == wxSize(16, 16) )
   {
      event.Skip();

      return;
   }
#endif // wxGTK

   // first lay out the controls or RefreshScrollbar() won't work properly
   Layout();
   RefreshScrollbar(event.GetSize());

   // don't call Skip() as we already did what wxPanel::OnSize() does
}

void wxEnhancedPanel::RefreshScrollbar(const wxSize& size)
{
   // find the total height of this panel
   int height = 0;
   for ( wxWindowList::Node *node = GetCanvas()->GetChildren().GetFirst();
         node;
         node = node->GetNext() )
   {
      wxWindow *child = node->GetData();
      if ( !child->IsTopLevel() )
      {
         int y = child->GetPosition().y + child->GetSize().y;
         if ( y > height )
         {
            height = y;
         }
      }
      //else: don't deal with dialogs/frames here
   }

   // a small margin to aviod that the canvas just fits into the panel
   height += 2*LAYOUT_Y_MARGIN;

   if ( height > size.y )
   {
      // why 10? well, it seems a reasonable value and changing it doesn't
      // change much anyhow...
      static const int nPageSize = 10;

      m_canvas->SetScrollbars(0, nPageSize, 0, height / nPageSize);
      m_canvas->EnableScrolling(FALSE, TRUE);
   }
   else
   {
      m_canvas->EnableScrolling(FALSE, FALSE);
   }

   GetCanvas()->SetSize(size);

   GetCanvas()->Layout();
}

// the top item is positioned near the top of the page, the others are
// positioned from top to bottom, i.e. under the last one
void wxEnhancedPanel::SetTopConstraint(wxLayoutConstraints *c,
                                          wxControl *last,
                                          size_t extraSpace)
{
   if ( last == NULL )
      c->top.SameAs(GetCanvas(), wxTop, 2*LAYOUT_Y_MARGIN + extraSpace);
   else {
      size_t margin = LAYOUT_Y_MARGIN;
      if ( last->IsKindOf(CLASSINFO(wxListBox)) ) {
         // listbox has a surrounding box, so leave more space
         margin *= 2;
      }

      c->top.Below(last, margin + extraSpace);
   }
}

// create a text entry with some browse button
wxTextCtrl *
wxEnhancedPanel::CreateEntryWithButton(const char *label,
                                       long widthMax,
                                       wxControl *last,
                                       wxEnhancedPanel::BtnKind kind,
                                       wxTextBrowseButton **ppButton)
{
   // create the label and text zone, as usually
   wxTextCtrl *text = CreateTextWithLabel(label, widthMax, last,
                                          GetBrowseButtonWidth(this) +
                                          2*LAYOUT_X_MARGIN);

   // and also create a button for browsing for file
   wxTextBrowseButton *btn;
   switch ( kind )
   {
      case FileBtn:
      case FileNewBtn:
      case FileSaveBtn:
         btn = new wxFileBrowseButton(text, GetCanvas(),
                                      kind != FileSaveBtn, // open
                                      kind != FileNewBtn); // existing only
         break;

      case FileOrDirBtn:
      case FileOrDirNewBtn:
      case FileOrDirSaveBtn:
         btn = new wxFileOrDirBrowseButton(text, GetCanvas(),
                                           kind != FileOrDirSaveBtn,
                                           kind != FileOrDirNewBtn);
         break;

      case ColorBtn:
         btn = new wxColorBrowseButton(text, GetCanvas());
         break;

      case FolderBtn:
         btn = new wxFolderBrowseButton(text, GetCanvas());
         break;

      case DirBtn:
         btn = new wxDirBrowseButton(text, GetCanvas());
         break;

      default:
         wxFAIL_MSG("unknown browse button kind");
         return NULL;
   }

   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->centreY.SameAs(text, wxCentreY);
   c->left.RightOf(text, LAYOUT_X_MARGIN);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN);
   c->height.SameAs(text, wxHeight);
   btn->SetConstraints(c);

   if ( ppButton )
   {
      *ppButton = btn;
   }

   return text;
}

// create a static bitmap with a label and position them and the browse button
// passed to us
wxStaticBitmap *wxEnhancedPanel::CreateIconEntry(const char *label,
                                                    long widthMax,
                                                    wxControl *last,
                                                    wxIconBrowseButton *btnIcon)
{
   wxLayoutConstraints *c;

   // create controls
   wxStaticBitmap *bitmap = new wxStaticBitmap(GetCanvas(), -1, wxNullBitmap);
   wxStaticText *pLabel = new wxStaticText(GetCanvas(), -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);

   // for the label
   c = new wxLayoutConstraints;
   c->centreY.SameAs(bitmap, wxCentreY);
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   pLabel->SetConstraints(c);

   // for the static bitmap
   int sizeIcon = 32;      // this is the standard icon size

   c = new wxLayoutConstraints;
   SetTopConstraint(c, last, LAYOUT_Y_MARGIN);
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->width.Absolute(sizeIcon);
   c->height.Absolute(sizeIcon);
   bitmap->SetConstraints(c);

   // for the browse button
   c = new wxLayoutConstraints;
   c->centreY.SameAs(bitmap, wxCentreY);
   c->left.RightOf(bitmap, LAYOUT_X_MARGIN);
   c->width.Absolute(GetBrowseButtonWidth(this) + LAYOUT_X_MARGIN);
   c->height.AsIs();
   btnIcon->SetConstraints(c);

   // the browse button will now update the bitmap when the icon changes
   btnIcon->SetAssociatedStaticBitmap(bitmap);

   return bitmap;
}

// create a single-line text control with a label
wxTextCtrl *wxEnhancedPanel::CreateTextWithLabel(const char *label,
                                                    long widthMax,
                                                    wxControl *last,
                                                    size_t nRightMargin,
                                                    int style)
{
   wxLayoutConstraints *c;

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last, 3); // FIXME shouldn't hardcode this!
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(GetCanvas(), -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   // for the text control
   c = new wxLayoutConstraints;
   c->centreY.SameAs(pLabel, wxCentreY);
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.AsIs();
   wxTextCtrl *pText = new wxTextCtrl(GetCanvas(), -1, "",wxDefaultPosition,
                                      wxDefaultSize, style);
   pText->SetConstraints(c);

   return pText;
}

// create just some text
wxStaticText *wxEnhancedPanel::CreateMessage(const char *label,
                                                wxControl *last)
{
   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(GetCanvas(), -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_LEFT);
   pLabel->SetConstraints(c);

   return pLabel;
}

// create a button
wxButton *wxEnhancedPanel::CreateButton(const wxString& labelAndId,
                                           wxControl *last)
{
   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();

   // split the label into the real label and the button id
   wxString label(labelAndId.BeforeFirst(':')),
            strId(labelAndId.AfterFirst(':'));
   int id;
   if ( !strId || !sscanf(strId, "%d", &id) )
      id = -1;

   wxButton *btn = new wxButton(GetCanvas(), id, label);
   btn->SetConstraints(c);

   return btn;
}


// create a button
wxXFaceButton *wxEnhancedPanel::CreateXFaceButton(const wxString&
                                                  labelAndId,
                                                  long widthMax,
                                                  wxControl *last)
{
   wxLayoutConstraints *c;

   // split the label into the real label and the button id
   wxString label(labelAndId.BeforeFirst(':')),
            strId(labelAndId.AfterFirst(':'));
   int id;
   if ( !strId || !sscanf(strId, "%d", &id) )
      id = -1;

   wxStaticText *pLabel = new wxStaticText(GetCanvas(), -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);

   wxXFaceButton *btn = new wxXFaceButton(GetCanvas(), id, wxString(""));

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   c->centreY.SameAs(btn, wxCentreY);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   pLabel->SetConstraints(c);

   c = new wxLayoutConstraints;
   SetTopConstraint(c, last, LAYOUT_Y_MARGIN);
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   btn->SetConstraints(c);

   return btn;
}
// create a checkbox
wxCheckBox *wxEnhancedPanel::CreateCheckBox(const char *label,
                                              long widthMax,
                                              wxControl *last)
{
   static size_t widthCheck = 0;
   if ( widthCheck == 0 ) {
      // calculate it only once, it's almost a constant
      widthCheck = AdjustCharHeight(GetCharHeight()) + 1;
   }

   wxCheckBox *checkbox = new wxCheckBox(GetCanvas(), -1, label,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxALIGN_RIGHT);

   wxLayoutConstraints *c = new wxLayoutConstraints;
   SetTopConstraint(c, last);
   c->width.AsIs();
   c->right.SameAs(GetCanvas(), wxLeft, -(int)(2*LAYOUT_X_MARGIN + widthMax
                                        + widthCheck));
   c->height.AsIs();

   checkbox->SetConstraints(c);

   return checkbox;
}

// create a radiobox control with 3 choices and a label for it
wxRadioBox *wxEnhancedPanel::CreateActionChoice(const char *label,
                                                   long widthMax,
                                                   wxControl *last,
                                                   size_t nRightMargin)
{
   wxLayoutConstraints *c;

   // for the radiobox
   c = new wxLayoutConstraints;
   SetTopConstraint(c, last, LAYOUT_Y_MARGIN);
   c->left.SameAs(GetCanvas(), wxLeft, 2*LAYOUT_X_MARGIN + widthMax );
   c->width.AsIs();
   c->height.AsIs();

   // FIXME we assume that if there other controls dependent on this one in
   //       the options dialog, then only the first choice ("No") means that
   //       they should be disabled
   wxString choices[3];
   choices[0] = _("No");
   choices[1] = _("Ask");
   choices[2] = _("Yes");
   wxRadioBox *radiobox = new wxRadioBox(GetCanvas(), -1, "",
                                         wxDefaultPosition, wxDefaultSize,
                                         3,
                                         choices,
                                         1,wxRA_SPECIFY_ROWS);

   radiobox->SetConstraints(c);

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   c->centreY.SameAs(radiobox, wxCentreY);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(GetCanvas(), -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   return radiobox;
}

// create a combobox
wxControl *wxEnhancedPanel::CreateComboBoxOrChoice(bool createCombobox,
                                                   const char *label,
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
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last, LAYOUT_Y_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(GetCanvas(), -1, choices[0],
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   // for the combobox
   c = new wxLayoutConstraints;

   c->centreY.SameAs(pLabel, wxCentreY);
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.AsIs();

   wxControl *combobox;
   if ( createCombobox )
   {
      combobox = new wxComboBox(GetCanvas(), -1, "",
                                wxDefaultPosition, wxDefaultSize,
                                n-1,
                                choices+1,
                                wxCB_DROPDOWN | wxCB_READONLY);
   }
   else
   {
      combobox = new wxChoice(GetCanvas(), -1,
                              wxDefaultPosition, wxDefaultSize,
                              n-1, choices+1);
   }


   combobox->SetConstraints(c);
   delete [] choices;
   return combobox;
}

wxTextCtrl *wxEnhancedPanel::CreateColorEntry(const char *label,
                                              long widthMax,
                                              wxControl *last)
{
   return CreateEntryWithButton(label, widthMax, last, ColorBtn);
}

// create a listbox and the buttons to work with it
// NB: we consider that there is only one listbox (at most) per page, so
//     the button ids are always the same
wxListBox *wxEnhancedPanel::CreateListbox(const char *label,
                                            wxControl *last)
{
   // a box around all this stuff
   wxStaticBox *box = new wxStaticBox(GetCanvas(), -1, label);

   wxLayoutConstraints *c;
   c = new wxLayoutConstraints;
   SetTopConstraint(c, last);
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN);
   c->height.PercentOf(GetCanvas(), wxHeight, 50);
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
      button = new wxButton(GetCanvas(), wxOptionsPage_BtnNew + nBtn, labels[nBtn]);
      button->SetConstraints(c);
   }

   // and the listbox itself
   wxListBox *listbox = new wxListBox(GetCanvas(), -1);
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, LAYOUT_X_MARGIN);
   c->right.LeftOf(button, LAYOUT_X_MARGIN);;
   c->bottom.SameAs(box, wxBottom, LAYOUT_Y_MARGIN);
   listbox->SetConstraints(c);

   return listbox;
}

// enable/disable the text control and its label
void wxEnhancedPanel::EnableTextWithButton(wxTextCtrl *control, bool bEnable)
{
   // NB: we assume that the control ids are consecutive
   long id = wxWindow::NextControlId(control->GetId());
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
void wxEnhancedPanel::EnableTextWithLabel(wxTextCtrl *control, bool bEnable)
{
   // not only enable/disable it, but also make (un)editable because it gives
   // visual feedback
   control->SetEditable(bEnable);

   // disable the label too: this will grey it out

   // NB: we assume that the control ids are consecutive
   long id = wxWindow::PrevControlId(control->GetId());
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

// enable/disable any control with label
void wxEnhancedPanel::EnableControlWithLabel(wxControl *control, bool bEnable)
{
   // first enable the combobox itself
   control->Enable(bEnable);

   // then its label

   // NB: we assume that the control ids are consecutive
   long id = wxWindow::PrevControlId(control->GetId());
   wxWindow *win = FindWindow(id);

   if ( win == NULL ) {
      wxFAIL_MSG("can't find label for the control");
   }
   else {
      // did we find the right one?
      wxASSERT( win->IsKindOf(CLASSINFO(wxStaticText)) );

      win->Enable(bEnable);
   }
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

// ----------------------------------------------------------------------------
// wxManuallyLaidOutDialog
// ----------------------------------------------------------------------------

#define DIALOG_STYLE (wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLIP_CHILDREN)

wxManuallyLaidOutDialog::wxManuallyLaidOutDialog(wxWindow *parent,
                                                 const wxString& title,
                                                 const wxString& profileKey)
                       : wxPDialog(
                                   profileKey,
                                   parent,
                                   wxString("Mahogany: ") + title,
                                   DIALOG_STYLE
                                  )
{
   // basic unit is the height of a char, from this we fix the sizes of all
   // other controls
   size_t heightLabel = AdjustCharHeight(GetCharHeight());

   hBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
   wBtn = BUTTON_WIDTH_FROM_HEIGHT(hBtn);

   m_helpId = -1; // no help id by default
   // the controls will be positioned with the constraints
   SetAutoLayout(TRUE);
}

void wxManuallyLaidOutDialog::SetDefaultSize(int width, int height,
                                             bool setAsMinimalSizeToo)
{
   if ( !LastSizeRestored() )
   {
      int heightScreen = (9*wxGetDisplaySize().y) / 10;
      int heightInitital;
      if ( height > heightScreen )
      {
         // don't create dialogs taller than the screen
         heightInitital = heightScreen;
      }
      else
      {
         heightInitital = height;
      }

      SetSize(width, heightInitital);

      Centre(wxCENTER_FRAME | wxBOTH);
   }

   if ( setAsMinimalSizeToo )
   {
      // do allow making the dialog smaller because the height might be too
      // big for the screen - but still set some minimal height to prevent it
      // from being shrunk to nothing at all
      SetSizeHints(width, 10*hBtn);

      m_sizeMin = wxSize(width, height);
   }
}

wxStaticBox *
wxManuallyLaidOutDialog::CreateStdButtonsAndBox(const wxString& boxTitle,
                                                bool noBox,
                                                int helpId)
{
   wxLayoutConstraints *c;

   // first the 2 buttons in the bottom/right corner
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

   // add a help button?
   if(helpId != -1)
   {
      wxButton *btnHelp = new wxButton(this, wxID_HELP, _("Help"));
      c = new wxLayoutConstraints;
      c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
      c->width.Absolute(wBtn);
      c->height.Absolute(hBtn);
      c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
      btnHelp->SetConstraints(c);
      m_helpId = helpId;
   }

   // a box around all the other controls
   if ( noBox )
      return NULL;

   wxStaticBox *box = new wxStaticBox(this, -1, boxTitle);
   c = new wxLayoutConstraints();
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(btnOk, wxTop, 4*LAYOUT_Y_MARGIN);
   box->SetConstraints(c);

   return box;
}

// -----------------------------------------------------------------------------
// wxNotebookDialog
// -----------------------------------------------------------------------------

wxNotebookDialog::wxNotebookDialog(wxFrame *parent,
                                   const wxString& title,
                                   const wxString& profileKey)
                : wxManuallyLaidOutDialog(parent, title, profileKey)
{
   m_btnOk =
   m_btnApply = NULL;

   m_profileForButtons = NULL;

   m_lastBtn = MEventOptionsChangeData::Invalid;
}

void wxNotebookDialog::CreateAllControls()
{
   wxLayoutConstraints *c;

   // calculate the controls size
   // ---------------------------

   // create the panel
   // ----------------
   wxPanel *panel = new wxPanel(this, -1);

#ifdef DEBUG
   // this makes wxWin debug messages about unsatisfied constraints a bit more
   // informative
   panel->SetName("MainNbookDlgPanel");
#endif

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
   c->bottom.SameAs(panel, wxBottom, 4*LAYOUT_Y_MARGIN + hBtn);
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

   // set dialog size (FIXME these are more or less arbitrary numbers)
   SetDefaultSize(6*wBtn, 27*hBtn, TRUE /* set as min size too */);
}

// transfer the data to/from notebook pages
// ----------------------------------------
bool wxNotebookDialog::TransferDataToWindow()
{
   for ( int nPage = 0; nPage < m_notebook->GetPageCount(); nPage++ ) {
      wxWindow *page = m_notebook->GetPage(nPage);
      if ( !page->TransferDataToWindow() ) {
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

/*
   A few words about handling of Ok/Apply/Cancel buttons are in order.

   First, here is what we want to do:

   1. Apply should result in immediate update of all objects which use
      the options which were changed
   2. Ok should behave the same as Apply and close the dialog
   3. Cancel should restore the settings to the state of before the last Apply

   Here is what we do to achieve this:

   1. Apply puts the profile which we use for storing the options into
      "suspend" mode
   2. Ok calls Commit() thus making all changes definitive
   3. Cancel calls Discard() returning the profile to the previous state
*/

void
wxNotebookDialog::SendOptionsChangeEvent()
{
   ASSERT_MSG( m_lastBtn != MEventOptionsChangeData::Invalid,
               "this should be only called when a button is pressed" );

   // notify everybody who cares about the change
   Profile *profile = GetProfile();

   if ( !profile )
   {
      // this may happen when cancelling the creation of a folder, so
      // just ignore it (nobody can be interested in it anyhow, this
      // event doesn't carry any useful information)
      ASSERT_MSG( m_lastBtn == MEventOptionsChangeData::Cancel,
                  "event from Apply or Ok should have a profile!" );
   }
   else
   {
      MEventOptionsChangeData *data = new MEventOptionsChangeData
                                          (
                                           profile,
                                           m_lastBtn
                                          );

      MEventManager::Send(data);
      profile->DecRef();
   }

   // event sent, reset the flag value
   m_lastBtn = MEventOptionsChangeData::Invalid;
}

// button event handlers
// ---------------------

void wxNotebookDialog::OnOK(wxCommandEvent& /* event */)
{
   m_lastBtn = MEventOptionsChangeData::Ok;

   if ( m_bDirty )
      DoApply();

   if ( !m_bDirty )
   {
      // ok, changes accepted (or there were no changes to begin with) -
      // anyhow, DoApply() succeeded
      if ( m_profileForButtons )
      {
         m_profileForButtons->Commit();
         m_profileForButtons->DecRef();
         m_profileForButtons = NULL;
      }

      EndModal(TRUE);
   }
   //else: test done from DoApply() failed, don't close the dialog
   //      nor free m_profileForButtons (this will be done in OnCancel or in
   //      a more successful call to OnOk)
}

void wxNotebookDialog::OnApply(wxCommandEvent& /* event */)
{
   ASSERT_MSG( m_bDirty, "'Apply' should be disabled!" );

   m_lastBtn = MEventOptionsChangeData::Apply;

   (void)DoApply();
}

bool wxNotebookDialog::DoApply()
{
   if ( !m_profileForButtons )
   {
      m_profileForButtons = GetProfile();
   }

   if ( m_profileForButtons )
      m_profileForButtons->Suspend();

   if ( TransferDataFromWindow() )
   {
      if ( OnSettingsChange() )
      {
         m_bDirty = FALSE;
         m_btnApply->Enable(FALSE);

         SendOptionsChangeEvent();

         return TRUE;
      }
   }
   // If OnSettingsChange() or the Transfer function failed, we
   // don't reset the m_bDirty flag so that OnOk() will know we failed

   if ( m_profileForButtons )
   {
      m_profileForButtons->Discard();
   }

   // don't m_profileForButtons->DecRef() - this will be done in OnOk or
   // OnCancel later

   return FALSE;
}

void wxNotebookDialog::OnCancel(wxCommandEvent& /* event */)
{
   if ( m_profileForButtons )
   {
      // [Apply] has been called before, undo it now
      m_profileForButtons->Discard();
      m_profileForButtons->DecRef();

      m_profileForButtons = NULL;
   }

   // note that as [Apply] hasn't been pressed if m_profileForButtons is NUL,
   // we might not send the event below neither in this case - should we?

   m_lastBtn = MEventOptionsChangeData::Cancel;
   SendOptionsChangeEvent();
   // allow the event to be processed before we are gone
   MEventManager::DispatchPending();

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

static size_t GetBrowseButtonWidth(wxWindow *win)
{
   static size_t s_widthBtn = 0;
   if ( s_widthBtn == 0 ) {
      // calculate it only once, it's almost a constant
      s_widthBtn = 2*win->GetCharWidth();
   }

   return s_widthBtn;
}
