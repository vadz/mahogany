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
#  include "guidef.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "strutil.h"
#  include "MHelp.h"
#  include "gui/wxMIds.h"
#  include "Mdefaults.h"
#  include "gui/wxMFrame.h"
#  include "gui/wxIconManager.h"

#  include <wx/wxchar.h>         // for wxPrintf/Scanf
#  include <wx/layout.h>
#  include <wx/stattext.h>
#  include <wx/statbox.h>
#  include <wx/statbmp.h>
#  include <wx/dcclient.h>       // for wxClientDC
#  include <wx/settings.h>       // for wxSystemSettings
#ifdef __WINE__
#  include <wx/scrolwin.h>       // for wxScrolledWindow
#endif // __WINE__
#endif // USE_PCH

#include <wx/imaglist.h>
#include <wx/statline.h>

#include "gui/wxOptionsPage.h"
#include "gui/wxBrowseButton.h"
#include "gui/wxMenuDefs.h"

#include "ConfigSourcesAll.h"

#include "Mupgrade.h"      // for VerifyEMailSendingWorks()

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// returns the width of the ">>" button
static size_t GetBrowseButtonWidth(wxWindow *win);

// takes a label followed by a colon separated list of strings, i.e.
// "LABEL:choice1:...:choiceN" and returns the array of choices and modifies
// the passed in string to contain just the label
static wxArrayString SplitLabelWithChoices(wxString *label);

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_TBARIMAGES;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_OPT_TEST_ASK;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// disable wxScrolledWindow::Layout() (added in wxWin 2.3.3) as we handle the
// window resizing ourselves
class wxEnhScrolledWindow : public wxScrolledWindow
{
public:
   wxEnhScrolledWindow(wxWindow *parent)
      : wxScrolledWindow(parent, -1,
                         wxDefaultPosition, wxDefaultSize,
                         wxHSCROLL | wxVSCROLL | wxTAB_TRAVERSAL)
      {
      }

   virtual bool Layout() { return wxScrolledWindow::Layout(); }

private:
   DECLARE_NO_COPY_CLASS(wxEnhScrolledWindow)
};

// ============================================================================
// implementation
// ============================================================================

// -----------------------------------------------------------------------------
// event tables
// -----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxOptionsPageSubdialog, wxManuallyLaidOutDialog)
   EVT_CHECKBOX(-1, wxOptionsPageSubdialog::OnChange)
   EVT_RADIOBOX(-1, wxOptionsPageSubdialog::OnChange)
   EVT_TEXT(-1,     wxOptionsPageSubdialog::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsEditDialog, wxProfileSettingsEditDialog)
   EVT_BUTTON(M_WXID_HELP, wxOptionsEditDialog::OnHelp)
   EVT_BUTTON(wxID_OK,     wxOptionsEditDialog::OnOK)
   EVT_BUTTON(wxID_APPLY,  wxOptionsEditDialog::OnApply)
   EVT_BUTTON(wxID_CANCEL, wxOptionsEditDialog::OnCancel)
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
IMPLEMENT_ABSTRACT_CLASS(wxOptionsEditDialog, wxManuallyLaidOutDialog)

// ----------------------------------------------------------------------------
// control creation functions
// ----------------------------------------------------------------------------

long GetMaxLabelWidth(const wxArrayString& labels, wxWindow *win)
{
   wxClientDC dc(win);
   dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
   wxCoord width, widthMax = 0;

   size_t nCount = labels.Count();
   for ( size_t n = 0; n < nCount; n++ )
   {
      dc.GetTextExtent(labels[n], &width, NULL);
      if ( width > widthMax )
         widthMax = width;
   }

   return widthMax;
}

static void SetTopConstraint(wxWindow *parent,
                             wxLayoutConstraints *c,
                             wxControl *last,
                             int extraSpace)
{
   if ( last == NULL )
   {
      c->top.SameAs(parent, wxTop, 2*LAYOUT_Y_MARGIN + extraSpace);
   }
   else // have last control above
   {
      if ( wxDynamicCast(last, wxStaticBox) )
      {
         // special case: last is not the control above us but the static box
         // containing us
         c->top.SameAs(last, wxTop, 3*LAYOUT_Y_MARGIN);
      }
      else
      {
         size_t margin = LAYOUT_Y_MARGIN;
         if ( last->IsKindOf(CLASSINFO(wxListBox)) )
         {
            // listbox has a surrounding box, so leave more space
            margin *= 4;
         }

         c->top.Below(last, margin + extraSpace);
      }
   }
}

wxStaticText *
CreateMessage(wxWindow *parent, const wxChar *label, wxControl *last)
{
   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->left.SameAs(parent, wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(parent, c, last, 0);
   c->right.SameAs(parent, wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(parent, -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_LEFT);
   pLabel->SetConstraints(c);

   return pLabel;
}

wxStaticText *CreateMessageForControl(wxWindow *parent,
                                      wxWindow *control,
                                      const wxChar *label,
                                      long widthMax,
                                      wxControl *last,
                                      wxCoord nRightMargin)
{
   wxLayoutConstraints *c;

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(parent, wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(parent, c, last, 3); // FIXME shouldn't hardcode this!
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(parent, -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   // for the control
   c = new wxLayoutConstraints;
   c->centreY.SameAs(pLabel, wxCentreY);
   c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
   c->right.SameAs(parent, wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.AsIs();
   control->SetConstraints(c);

   pLabel->MoveBeforeInTabOrder(control);

   // we don't use ids to find controls with wx 2.9+
#if !wxCHECK_VERSION(2, 9, 0)
   // we have to make sure that the control id follows the label id as the code
   // elsewhere relies on this
   int idLabel = pLabel->GetId();
   ASSERT_MSG( control->GetId() == wxWindow::PrevControlId(idLabel),
               _T("control should have been created right before") );
   pLabel->SetId(control->GetId());
   control->SetId(idLabel);
#endif // wx < 2.9

   return pLabel;
}

wxTextCtrl *CreateTextWithLabel(wxWindow *parent,
                                const wxChar *label,
                                long widthMax,
                                wxControl *last,
                                wxCoord nRightMargin,
                                int style)
{
   wxTextCtrl *text = new wxTextCtrl(parent, -1, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize, style);

   CreateMessageForControl(parent, text, label, widthMax, last, nRightMargin);

   return text;
}

wxCheckBox *CreateCheckBox(wxWindow *parent,
                           const wxChar *label,
                           long widthMax,
                           wxControl *last,
                           wxCoord nRightMargin)
{
   static size_t widthCheck = 0;
   if ( widthCheck == 0 ) {
      // calculate it only once, it's almost a constant
      widthCheck = AdjustCharHeight(parent->GetCharHeight()) + 1;
   }

   wxCheckBox *checkbox = new wxCheckBox(parent, -1, label,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxALIGN_RIGHT);

   wxLayoutConstraints *c = new wxLayoutConstraints;
   SetTopConstraint(parent, c, last, 0);
   c->width.AsIs();
   if ( widthMax == -1 )
      c->left.SameAs(parent, wxLeft, LAYOUT_X_MARGIN);
   else
      c->right.SameAs(parent, wxLeft, -(int)(2*LAYOUT_X_MARGIN + widthMax
                                              + widthCheck));
   c->height.AsIs();

   checkbox->SetConstraints(c);

   return checkbox;
}

wxRadioBox *CreateRadioBox(wxWindow *parent,
                           const wxChar *labelFull,
                           long widthMax,
                           wxControl *last,
                           wxCoord nRightMargin)
{
   // split the "label" into the real label and the choices:
   wxString label = labelFull;
   wxArrayString choices = SplitLabelWithChoices(&label);

   wxLayoutConstraints *c;

   // for the radiobox
   c = new wxLayoutConstraints;
   SetTopConstraint(parent, c, last,
#ifdef __WXMSW__
                    -LAYOUT_Y_MARGIN
#else
                    0
#endif // __WXMSW__
                    );
   c->left.SameAs(parent, wxLeft, 2*LAYOUT_X_MARGIN + widthMax);
   c->right.SameAs(parent, wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.AsIs();

   // FIXME we assume that if there other controls dependent on this one in
   //       the options dialog, then only the first choice ("No" for the action
   //       choice) means that they should be disabled but this is really just
   //       a dirty hack
   wxRadioBox *radiobox = new wxRadioBox(parent, -1, wxEmptyString,
                                         wxDefaultPosition, wxDefaultSize,
                                         choices,
                                         1, wxRA_SPECIFY_ROWS);

   radiobox->SetConstraints(c);

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(parent, wxLeft, LAYOUT_X_MARGIN);
   c->centreY.SameAs(radiobox, wxCentreY
                     // needed to make it look right under wxGTK
#ifdef __WXGTK__
                     , 4
#endif // __WXGTK__
                     );
   c->width.Absolute(widthMax - 2*LAYOUT_X_MARGIN); // looks better like this
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(parent, -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);
   pLabel->SetConstraints(c);

   return radiobox;
}

wxTextCtrl *
CreateEntryWithButton(wxWindow *parent,
                      const wxChar *label,
                      long widthMax,
                      wxControl *last,
                      wxCoord nRightMargin,
                      BtnKind kind,
                      wxTextBrowseButton **ppButton)
{
   // create the label and text zone, as usually
   wxTextCtrl *text = CreateTextWithLabel(parent,
                                          label, widthMax, last,
                                          GetBrowseButtonWidth(parent) +
                                          2*LAYOUT_X_MARGIN + nRightMargin);

   // and also create a button for browsing for file
   wxTextBrowseButton *btn;
   switch ( kind )
   {
      case FileBtn:
      case FileNewBtn:
      case FileSaveBtn:
         btn = new wxFileBrowseButton(text, parent,
                                      kind != FileSaveBtn, // open
                                      kind != FileNewBtn); // existing only
         break;

      case FileOrDirBtn:
      case FileOrDirNewBtn:
      case FileOrDirSaveBtn:
         btn = new wxFileOrDirBrowseButton(text, parent,
                                           kind != FileOrDirSaveBtn,
                                           kind != FileOrDirNewBtn);
         break;

      case ColorBtn:
         btn = new wxColorBrowseButton(text, parent);
         break;

      case FontBtn:
         btn = new wxFontBrowseButton(text, parent);
         break;

      case FolderBtn:
         btn = new wxFolderBrowseButton(text, parent);
         break;

      case DirBtn:
         btn = new wxDirBrowseButton(text, parent);
         break;

      default:
         wxFAIL_MSG(_T("unknown browse button kind"));
         return NULL;
   }

   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->centreY.SameAs(text, wxCentreY);
   c->left.RightOf(text, LAYOUT_X_MARGIN);
   c->right.SameAs(parent, wxRight, LAYOUT_X_MARGIN + nRightMargin);
   c->height.SameAs(text, wxHeight);
   btn->SetConstraints(c);

   if ( ppButton )
   {
      *ppButton = btn;
   }

   return text;
}

wxTextCtrl *
CreateFileEntry(wxWindow *parent,
                const wxChar *label,
                long widthMax,
                wxControl *last,
                wxCoord nRightMargin,
                wxFileBrowseButton **ppButton,
                int flags)
{
   BtnKind btn;
   if ( flags & FileEntry_Save )
      btn = FileSaveBtn;
   else
      btn = flags & FileEntry_ExistingOnly ? FileBtn : FileNewBtn;

   return CreateEntryWithButton(parent, label, widthMax, last, nRightMargin,
                                btn, (wxTextBrowseButton **)ppButton);
}

// this function enables or disables the window label, i.e. the static text
// control which is supposed to precede it
static
void EnableWindowLabel(wxWindow *parent, wxWindow *control, bool bEnable)
{
#if wxCHECK_VERSION(2, 9, 0)
   wxWindow *win = control->GetPrevSibling();
#else // wx 2.8
   // NB: we assume that the control ids are consecutive
   long id = wxWindow::PrevControlId(control->GetId());
   wxWindow *win = parent->FindWindow(id);
#endif // wx 2.9/2.8

   if ( win == NULL ) {
      wxFAIL_MSG(_T("can't find label for the text entry zone"));
   }
   else {
      // did we find the right one?
      wxASSERT( win->IsKindOf(CLASSINFO(wxStaticText)) );

      win->Enable(bEnable);
   }
}

void EnableTextWithLabel(wxWindow *parent, wxTextCtrl *control, bool bEnable)
{
   // not only enable/disable it, but also make (un)editable because it gives
   // visual feedback
   control->SetEditable(bEnable);

   EnableWindowLabel(parent, control, bEnable);
}

void EnableTextWithButton(wxWindow *parent, wxTextCtrl *control, bool bEnable)
{
#if wxCHECK_VERSION(2, 9, 0)
   wxWindow *win = control->GetNextSibling();
#else // wx 2.8
   // NB: we assume that the control ids are consecutive
   long id = wxWindow::NextControlId(control->GetId());
   wxWindow *win = parent->FindWindow(id);
#endif // wx 2.9/2.8

   if ( win == NULL ) {
      wxFAIL_MSG(_T("can't find browse button for the text entry zone"));
   }
   else {
      wxASSERT( win->IsKindOf(CLASSINFO(wxButton)) );

      win->Enable(bEnable);
   }

   EnableTextWithLabel(parent, control, bEnable);
}

// ----------------------------------------------------------------------------
// wxPDialog
// ----------------------------------------------------------------------------

wxPDialog::wxPDialog(const wxString& profileKey,
                     wxWindow *parent,
                     const wxString& title,
                     long style)
         : wxMDialog(parent, -1, title,
                     wxDefaultPosition, wxDefaultSize,
                     style),
           m_profileKey(profileKey)
{
   if ( m_profileKey.empty() )
   {
      m_didRestoreSize = false;
   }
   else
   {
      int x, y, w, h;
      m_didRestoreSize = wxMFrame::RestorePosition(m_profileKey, &x, &y, &w, &h);

      if ( m_didRestoreSize )
      {
         SetSize(x, y, w, h);
      }
   }
   //else: it should be set by the derived class or the default size will be
   //      used -- not catastrophic neither
}

wxPDialog::~wxPDialog()
{
   // save the dialog position if we can and if the dialog wasn't dismissed by
   // Esc/Cancel - this indicates that the user doesn't want to remember
   // anything at all!
   if ( !m_profileKey.empty() && GetReturnCode() != wxID_CANCEL )
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
                      : wxManuallyLaidOutDialog
                        (
                           GET_PARENT_OF_CLASS(parent, wxFrame),
                           label,
                           windowName
                        ),
                        ProfileHolder(profile)
{
}

void wxOptionsPageSubdialog::OnChange(wxCommandEvent&)
{
   // we don't do anything, but just eat these messages - otherwise they will
   // confuse wxOptionsPage which is our parent because it only processes
   // messages from its own controls
}

// ----------------------------------------------------------------------------
// wxNotebookWithImages
// ----------------------------------------------------------------------------

/* static */
bool wxNotebookWithImages::ShouldShowIcons()
{
   // assume that if the user doesn't want to see the images in the toolbar, he
   // doesn't want to see them elsewhere, in particular in the notebook tabs
   // neither
   //
   // NB: the config values are shifted related to the enum values, hence "+1"
   return (int)READ_APPCONFIG(MP_TBARIMAGES) + 1 != TbarShow_Text;
}

wxNotebookWithImages::wxNotebookWithImages(const wxString& configPath,
                                           wxWindow *parent,
                                           const char *aszImages[])
                    : wxPNotebook(configPath, parent, -1)
{
   if ( ShouldShowIcons() )
   {
      wxImageList *imageList = new wxImageList(32, 32, TRUE, WXSIZEOF(aszImages));
      wxIconManager *iconmanager = mApplication->GetIconManager();

      for ( size_t n = 0; aszImages[n]; n++ ) {
         imageList->Add(iconmanager->GetBitmap(aszImages[n]));
      }

      SetImageList(imageList);
   }
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
      m_canvas = new wxEnhScrolledWindow(this);
   }
   else
   {
      // no need for a separate canvas... just use the panel directly (canvas
      // is already initialized to NULL above)
      SetAutoLayout(true);
   }
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

   // lay out the controls and update the scrollbar
   DoLayout(event.GetSize());

   // don't call Skip() as we already did what wxPanel::OnSize() does
}

bool wxEnhancedPanel::DoLayout(const wxSize& size)
{
   // be careful not to do a recursive call here as we're called from our
   // Layout()!
   bool ok = m_canvas ? m_canvas->Layout() : wxPanel::Layout();
   if ( ok && m_canvas )
   {
      RefreshScrollbar(size);
   }
   //else: can it really fail?

   return ok;
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
   }
   else // remove scrollbar completely, we don't need it
   {
      m_canvas->SetScrollbars(0, 0, 0, 0);
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
   ::SetTopConstraint(GetCanvas(), c, last, extraSpace);
}

// create a text entry with some browse button
wxTextCtrl *
wxEnhancedPanel::CreateEntryWithButton(const wxChar *label,
                                       long widthMax,
                                       wxControl *last,
                                       BtnKind kind,
                                       wxTextBrowseButton **ppButton)
{
   return ::CreateEntryWithButton(GetCanvas(), label, widthMax, last, 0,
                                  kind, ppButton);
}

// create a static bitmap with a label and position them and the browse button
// passed to us
wxStaticBitmap *wxEnhancedPanel::CreateIconEntry(const wxChar *label,
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
wxTextCtrl *wxEnhancedPanel::CreateTextWithLabel(const wxChar *label,
                                                 long widthMax,
                                                 wxControl *last,
                                                 wxCoord nRightMargin,
                                                 int style)
{
   return ::CreateTextWithLabel(GetCanvas(), label, widthMax, last,
                                nRightMargin, style);
}

// create just some text
wxStaticText *
wxEnhancedPanel::CreateMessage(const wxChar *label, wxControl *last)
{
   return ::CreateMessage(GetCanvas(), label, last);
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
   if ( !strId || !wxSscanf(strId, _T("%d"), &id) )
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
   if ( !strId || !wxSscanf(strId, _T("%d"), &id) )
      id = -1;

   wxStaticText *pLabel = new wxStaticText(GetCanvas(), -1, label,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_RIGHT);

   wxXFaceButton *btn = new wxXFaceButton(GetCanvas(), id, wxEmptyString);

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
wxCheckBox *wxEnhancedPanel::CreateCheckBox(const wxChar *label,
                                            long widthMax,
                                            wxControl *last)
{
   return ::CreateCheckBox(GetCanvas(), label, widthMax, last);
}

// create a radiobox control with 3 choices and a label for it
wxRadioBox *
wxEnhancedPanel::CreateActionChoice(const wxChar *label,
                                    long widthMax,
                                    wxControl *last,
                                    wxCoord nRightMargin)
{
   wxString labelFull = label;
   labelFull << ':' << _("No") << ':' << _("Ask") << ':' << _("Yes");

   return CreateRadioBox(labelFull, widthMax, last, nRightMargin);
}

// create a generic radiobox control
wxRadioBox *
wxEnhancedPanel::CreateRadioBox(const wxChar *label,
                                long widthMax,
                                wxControl *last,
                                wxCoord nRightMargin)
{
   return ::CreateRadioBox(GetCanvas(), label, widthMax, last, nRightMargin);
}

// create a combobox
wxControl *wxEnhancedPanel::CreateComboBoxOrChoice(bool createCombobox,
                                                   const wxChar *labelFull,
                                                   long widthMax,
                                                   wxControl *last,
                                                   wxCoord nRightMargin)
{
   // split the "label" into the real label and the choices:
   wxString label = labelFull;
   wxArrayString choices = SplitLabelWithChoices(&label);

   wxLayoutConstraints *c;

   // for the label
   c = new wxLayoutConstraints;
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   SetTopConstraint(c, last, LAYOUT_Y_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   wxStaticText *pLabel = new wxStaticText(GetCanvas(), -1, label,
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
      combobox = new wxComboBox(GetCanvas(), -1, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                choices,
                                wxCB_DROPDOWN | wxCB_READONLY);
   }
   else
   {
      combobox = new wxChoice(GetCanvas(), -1,
                              wxDefaultPosition, wxDefaultSize,
                              choices);
   }


   combobox->SetConstraints(c);

   return combobox;
}

wxColorBrowseButton *wxEnhancedPanel::CreateColorEntry(const wxChar *label,
                                                       long widthMax,
                                                       wxControl *last)
{
   wxTextBrowseButton *btn;
   (void)CreateEntryWithButton(label, widthMax, last, ColorBtn, &btn);

   return wxStaticCast(btn, wxColorBrowseButton);
}

// create a listbox and the buttons to work with it
// NB: we consider that there is only one listbox (at most) per page, so
//     the button ids are always the same
wxListBox *wxEnhancedPanel::CreateListbox(const wxChar *label, wxControl *last)
{
   // a box around all this stuff
   wxStaticBox *box = new wxStaticBox(GetCanvas(), -1, label);

   wxLayoutConstraints *c;
   c = new wxLayoutConstraints;
   SetTopConstraint(c, last);
   c->left.SameAs(GetCanvas(), wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(GetCanvas(), wxRight, LAYOUT_X_MARGIN);
   c->height.Absolute(150);   // well, what else can we do here?
   box->SetConstraints(c);

   // the buttons vertically on the right of listbox (note that the labels
   // correspond to the order of wxOptionsPage_BtnXXX enum)
   static const char *aszLabels[] =
   {
      gettext_noop("&Add..."),
      gettext_noop("&Modify..."),
      gettext_noop("&Delete"),
   };

   // determine the longest button label
   wxArrayString labels;
   size_t nBtn;
   for ( nBtn = 0; nBtn < WXSIZEOF(aszLabels); nBtn++ ) {
      labels.Add(wxGetTranslation(aszLabels[nBtn]));
   }

   long widthMax = GetMaxLabelWidth(labels, this);

   widthMax += 3*LAYOUT_X_MARGIN; // looks better like this

   // create the buttons: [Modify] in the middle, [Add] above it and [Delete]
   // below
   wxButton *buttonModify
      = new wxButton
            (
             GetCanvas(),
             wxOptionsPage_BtnModify,
             labels[wxOptionsPage_BtnModify - wxOptionsPage_BtnNew]
            );

   c = new wxLayoutConstraints;
   c->centreY.SameAs(box, wxCentreY);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   buttonModify->SetConstraints(c);

   wxButton *buttonNew = new wxButton(GetCanvas(),
                                      wxOptionsPage_BtnNew,
                                      labels[0]);
   c = new wxLayoutConstraints;
   c->bottom.Above(buttonModify, -2*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   buttonNew->SetConstraints(c);

   wxButton *buttonDelete
      = new wxButton
            (
             GetCanvas(),
             wxOptionsPage_BtnDelete,
             labels[wxOptionsPage_BtnDelete - wxOptionsPage_BtnNew]
            );

   c = new wxLayoutConstraints;
   c->top.Below(buttonModify, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   buttonDelete->SetConstraints(c);

   // and the listbox itself
   wxListBox *listbox = new wxListBox(GetCanvas(), -1);
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, LAYOUT_X_MARGIN);
   c->right.LeftOf(buttonModify, LAYOUT_X_MARGIN);;
   c->bottom.SameAs(box, wxBottom, LAYOUT_Y_MARGIN);
   listbox->SetConstraints(c);

   // make it possible to retrieve it later
   buttonNew->SetClientData(listbox);
   buttonModify->SetClientData(listbox);
   buttonDelete->SetClientData(listbox);

   return listbox;
}

// enable/disable the text control and its label
void wxEnhancedPanel::EnableTextWithButton(wxTextCtrl *control, bool bEnable)
{
   ::EnableTextWithButton(GetCanvas(), control, bEnable);
}

// enable/disable the colour browse button and its text with label
void wxEnhancedPanel::EnableColourBrowseButton(wxColorBrowseButton *btn,
                                               bool bEnable)
{
   btn->Enable(bEnable);

   wxWindow * const win = btn->GetWindow();
   win->Enable(bEnable);

   EnableWindowLabel(GetCanvas(), win, bEnable);
}

// enable/disable the text control with label and button
void wxEnhancedPanel::EnableTextWithLabel(wxTextCtrl *control, bool bEnable)
{
   ::EnableTextWithLabel(GetCanvas(), control, bEnable);
}

wxStaticText *wxEnhancedPanel::GetLabelForControl(wxControl *control)
{
#if wxCHECK_VERSION(2, 9, 0)
   wxWindow *win = control->GetPrevSibling();
#else // wx 2.8
   long id = wxWindow::PrevControlId(control->GetId());
   wxWindow *win = FindWindow(id);
#endif // wx 2.9/2.8

   return wxDynamicCast(win, wxStaticText);
}

// enable/disable any control with label
void wxEnhancedPanel::EnableControlWithLabel(wxControl *control, bool bEnable)
{
   // first enable the combobox itself
   control->Enable(bEnable);

   // then its label
   wxStaticText *label = GetLabelForControl(control);
   CHECK_RET( label, _T("controls label not found?") );

   label->Enable(bEnable);
}

// enable/disable the listbox and its buttons
void wxEnhancedPanel::EnableListBox(wxListBox *control, bool bEnable)
{
   wxCHECK_RET( control, _T("NULL pointer in EnableListBox") );

   control->Enable(bEnable);

   for ( int id = wxOptionsPage_BtnNew; id <= wxOptionsPage_BtnDelete; id++ )
   {
      wxWindow *btn = FindWindow(id);
      wxASSERT_MSG( wxDynamicCast(btn, wxButton), _T("listbox btn not found") );
      if ( btn )
         btn->Enable(bEnable);
   }
}

// ----------------------------------------------------------------------------
// wxNotebookPageBase
// ----------------------------------------------------------------------------

void wxNotebookPageBase::OnChange(wxCommandEvent& /* event */)
{
   wxOptionsEditDialog *dlg = GET_PARENT_OF_CLASS(this, wxOptionsEditDialog);
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
                                   String(M_TITLE_PREFIX) + title,
                                   DIALOG_STYLE
                                  )
{
   // this doesn't give corret results under MSW
#if 0
   // basic unit is the height of a char, from this we fix the sizes of all
   // other controls
   size_t heightLabel = AdjustCharHeight(GetCharHeight());

   hBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
   wBtn = BUTTON_WIDTH_FROM_HEIGHT(hBtn);
#else // 1
   // basic unit is the standard button
   wxSize size = wxButton::GetDefaultSize();
   wBtn = size.x;
   hBtn = size.y;
#endif // 0/1

   m_helpId = -1; // no help id by default

   // the controls will be positioned with the constraints
   SetAutoLayout(TRUE);
}

void wxManuallyLaidOutDialog::SetDefaultSize(int width, int height,
                                             bool setAsMinimalSizeToo)
{
   if ( !LastSizeRestored() || setAsMinimalSizeToo )
   {
      int heightScreen = (9*wxGetDisplaySize().y) / 10;
      if ( height > heightScreen )
      {
         // don't create dialogs taller than the screen
         height = heightScreen;
      }
   }

   if ( !LastSizeRestored() )
   {
      SetClientSize(width, height);

      Centre(wxCENTER_FRAME | wxBOTH);
   }

   if ( setAsMinimalSizeToo )
   {
      // do allow making the dialog smaller because the height might be too
      // big for the screen - but still set some minimal height to prevent it
      // from being shrunk to nothing at all
      SetSizeHints(width, wxMin(height, 40*hBtn));

      m_sizeMin = wxSize(width, height);
   }

   Layout();
}

wxStaticBox *
wxManuallyLaidOutDialog::CreateStdButtonsAndBox(const wxString& boxTitle,
                                                int flags,
                                                int helpId)
{
   wxLayoutConstraints *c;

   // the buttons in the bottom/right corner

   // we always have at least the [Ok] button
   wxButton *btnOk = new wxButton(this, wxID_OK, _("OK"));
   btnOk->SetDefault();
   c = new wxLayoutConstraints;
   if ( flags & StdBtn_NoCancel )
      c->right.SameAs(this, wxRight, 2*LAYOUT_X_MARGIN);
   else
      c->left.SameAs(this, wxRight, -2*(LAYOUT_X_MARGIN + wBtn));
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
   btnOk->SetConstraints(c);

   // and usually the [Cancel] one
   if ( !(flags & StdBtn_NoCancel) )
   {
      wxButton *btnCancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
      c = new wxLayoutConstraints;
      c->left.SameAs(this, wxRight, -(LAYOUT_X_MARGIN + wBtn));
      c->width.Absolute(wBtn);
      c->height.Absolute(hBtn);
      c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
      btnCancel->SetConstraints(c);
   }

   // and, if we have a help id to invoke help, the [Help] one as well
   if ( helpId != -1 )
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
   if ( flags & StdBtn_NoBox )
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

void wxManuallyLaidOutDialog::OnHelp(wxCommandEvent & /*ev*/)
{
   mApplication->Help(m_helpId, this);
}

// ----------------------------------------------------------------------------
// wxProfileSettingsEditDialog
// ----------------------------------------------------------------------------

wxProfileSettingsEditDialog::wxProfileSettingsEditDialog(wxWindow *parent,
                                                         const wxString& title,
                                                         const wxString& key)
   : wxManuallyLaidOutDialog(parent, title, key)
{
   m_bDirty = false;
   m_changedConfigSource = false;
}

void wxProfileSettingsEditDialog::OnConfigSourceChange(wxCommandEvent& event)
{
   if ( event.GetEventObject() != m_chcSources )
   {
      event.Skip();
      return;
   }

   Profile_obj profile(GetProfile());
   if ( profile )
   {
      ApplyConfigSourceSelectedByUser(*profile);
   }
   //else: this can happen when we're creating a new folder, its profile isn't
   //      created before the folder itself is
}

void
wxProfileSettingsEditDialog::ApplyConfigSourceSelectedByUser(Profile& profile)
{
   const int sel = m_chcSources->GetSelection();
   ConfigSource *config = NULL;
   if ( sel != -1 )
   {
      AllConfigSources::List::iterator
         i = AllConfigSources::Get().GetSources().begin();
      for ( int n = 0; n < sel; n++ )
         ++i;

      config = i.operator->();
   }

   ConfigSource *configOld = profile.SetConfigSourceForWriting(config);

   // remember the original config source if this is the first time we change
   // it
   if ( !m_changedConfigSource )
   {
      m_configOld = configOld;
      m_changedConfigSource = true;
   }
}

void wxProfileSettingsEditDialog::EndModal(int rc)
{
   if ( m_changedConfigSource )
   {
      Profile_obj profile(GetProfile());
      if ( profile )
      {
         profile->SetConfigSourceForWriting(m_configOld);
      }
      else
      {
         FAIL_MSG( "no profile in wxOptionsEditDialog?" );
      }

      m_changedConfigSource = false;
   }

   wxManuallyLaidOutDialog::EndModal(rc);
}


wxControl *wxProfileSettingsEditDialog::CreateControlsBelow(wxPanel *panel)
{
   const AllConfigSources::List& sources = AllConfigSources::Get().GetSources();
   if ( sources.size() == 1 )
   {
      m_chcSources = NULL;
      return NULL;
   }

   m_chcSources = new wxChoice(panel, -1);

   Connect(wxEVT_COMMAND_CHOICE_SELECTED,
             wxCommandEventHandler(
                wxProfileSettingsEditDialog::OnConfigSourceChange));

   for ( AllConfigSources::List::iterator i = sources.begin(),
                                        end = sources.end();
         i != end;
         ++i )
   {
      m_chcSources->Append(i->GetName());
   }

   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   c->bottom.SameAs(panel, wxBottom, 5*LAYOUT_Y_MARGIN + hBtn);
   m_chcSources->SetConstraints(c);

   wxStaticText *label = new wxStaticText(panel, -1, _("&Save changes to:"));
   c = new wxLayoutConstraints;
   c->right.LeftOf(m_chcSources, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   c->centreY.SameAs(m_chcSources, wxCentreY);
   label->SetConstraints(c);

   wxStaticLine *line = new wxStaticLine(panel, -1);
   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();
   c->bottom.Above(m_chcSources, -LAYOUT_Y_MARGIN);
   line->SetConstraints(c);

   return line;
}

void wxProfileSettingsEditDialog::CreateAllControls()
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
   panel->SetName(_T("MainNbookDlgPanel"));
#endif

   panel->SetAutoLayout(TRUE);
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft);
   c->right.SameAs(this, wxRight);
   c->top.SameAs(this, wxTop);
   c->bottom.SameAs(this, wxBottom);
   panel->SetConstraints(c);

   // optional controls above/below the notebook
   wxControl *top = CreateControlsAbove(panel),
             *bottom = CreateControlsBelow(panel);

   // the notebook itself is created by this function
   wxWindow * const winMain = CreateMainWindow(panel);

   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
   if ( top )
      c->top.SameAs(top, wxBottom, 2*LAYOUT_Y_MARGIN);
   else
      c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
   if ( bottom )
      c->bottom.SameAs(bottom, wxTop, 2*LAYOUT_Y_MARGIN);
   else
      c->bottom.SameAs(panel, wxBottom, 4*LAYOUT_Y_MARGIN + hBtn);
   winMain->SetConstraints(c);

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

   // set dialog size (FIXME these are more or less arbitrary numbers)
   SetDefaultSize(6*wBtn, 27*hBtn, TRUE /* set as min size too */);
}

// -----------------------------------------------------------------------------
// wxOptionsEditDialog
// -----------------------------------------------------------------------------

wxOptionsEditDialog::wxOptionsEditDialog(wxFrame *parent,
                                   const wxString& title,
                                   const wxString& profileKey)
                   : wxProfileSettingsEditDialog(parent, title, profileKey)
{
   m_btnOk =
   m_btnApply = NULL;

   m_profileForButtons = NULL;

   m_lastBtn = MEventOptionsChangeData::Invalid;
}

// transfer the data to/from notebook pages
// ----------------------------------------

bool wxOptionsEditDialog::TransferDataToWindow()
{
   const int count = m_notebook->GetPageCount();
   for ( int nPage = 0; nPage < count; nPage++ ) {
      wxWindow *page = m_notebook->GetPage(nPage);
      if ( !page->TransferDataToWindow() ) {
         return FALSE;
      }
   }

   ResetDirty();

   // suspend the profile before anything is written to it
   m_profileForButtons = GetProfile();
   if ( m_profileForButtons )
      m_profileForButtons->Suspend();

   return TRUE;
}

bool wxOptionsEditDialog::TransferDataFromWindow()
{
   const int count = m_notebook->GetPageCount();
   for ( int nPage = 0; nPage < count; nPage++ )
   {
      wxWindow * const page = m_notebook->GetPage(nPage);
      if ( !page->TransferDataFromWindow() )
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

   1. Initially we put the profile which we use for storing the options into
      "suspend" mode (this is done above)
   2. Apply just writes the new settings to the profile as usual (but they go
      to the "suspended" section and so can be discarded later)
   3. Ok calls Commit() thus making all changes definitive
   4. Cancel calls Discard() returning the profile to the previous state
*/

void
wxOptionsEditDialog::SendOptionsChangeEvent()
{
   ASSERT_MSG( m_lastBtn != MEventOptionsChangeData::Invalid,
               _T("this should be only called when a button is pressed") );

   // note that we might actually not have this profile when creating the
   // folder (there is no profile yet then), so it's not an error - just don't
   // do anything in that case
   if ( m_profileForButtons )
   {
      // notify everybody who cares about the change
      MEventOptionsChangeData *data = new MEventOptionsChangeData
                                          (
                                           m_profileForButtons,
                                           m_lastBtn
                                          );

      MEventManager::Send(data);
   }
}

void wxOptionsEditDialog::EnableButtons(bool enable)
{
   if ( m_btnApply )
   {
       m_btnApply->Enable(enable);
       m_btnOk->Enable(enable);
   }
}

// button event handlers
// ---------------------

void wxOptionsEditDialog::OnOK(wxCommandEvent& /* event */)
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

      EndModal(wxID_OK);
   }
   //else: test done from DoApply() failed, don't close the dialog
   //      nor free m_profileForButtons (this will be done in OnCancel or in
   //      a more successful call to OnOk)
}

void wxOptionsEditDialog::OnApply(wxCommandEvent& /* event */)
{
   ASSERT_MSG( m_bDirty, _T("'Apply' should be disabled!") );

   m_lastBtn = MEventOptionsChangeData::Apply;

   (void)DoApply();
}

bool wxOptionsEditDialog::DoApply()
{
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

   // don't do m_profileForButtons->DecRef() neither - this will be done in
   // OnOk or OnCancel later

   return FALSE;
}

void wxOptionsEditDialog::OnCancel(wxCommandEvent& /* event */)
{
   // discard any changes
   if ( m_profileForButtons )
   {
      m_profileForButtons->Discard();
   }

   // but then only do something non trivial if [Apply] had been pressed
   if ( m_lastBtn == MEventOptionsChangeData::Apply )
   {
      // then send the event to allow everything to return to the old state
      m_lastBtn = MEventOptionsChangeData::Cancel;
      SendOptionsChangeEvent();

      // allow the event to be processed before we are gone
      MEventManager::ForceDispatchPending();
   }

   // release the profile anyhow (notice that this should be done after
   // SendOptionsChangeEvent() as it uses m_profileForButtons)
   if ( m_profileForButtons )
   {
      m_profileForButtons->DecRef();

      m_profileForButtons = NULL;
   }

   EndModal(wxID_CANCEL);
}

void wxOptionsEditDialog::OnHelp(wxCommandEvent& /* event */)
{
   int pid = m_notebook->GetSelection();
   if(pid == -1) // no page selected??
      mApplication->Help(MH_OPTIONSNOTEBOOK,this);
   else
      mApplication->Help(((wxOptionsPage *)m_notebook->GetPage(pid))->HelpId(),this);
}

bool wxOptionsEditDialog::OnSettingsChange()
{
   if ( m_bTest )
   {
      if ( MDialog_YesNoDialog(_("Some important program settings were changed.\n"
                                 "\nWould you like to test the new setup "
                                 "(recommended)?"),
                               this,
                               _("Test setup?"),
                               M_DLG_YES_DEFAULT,
                               M_MSGBOX_OPT_TEST_ASK) )
      {
         if ( !VerifyEMailSendingWorks() )
         {
            return FALSE;
         }
      }
      else
      {
         // no test was done, assume it's ok...
         m_bTest = FALSE;
      }
   }

   if ( m_bRestartWarning )
   {
      MDialog_Message(_("Some of the changes to the program options will\n"
                        "only take effect when the progam will be run the\n"
                        "next time and not during this session."),
                      this, MDIALOG_MSGTITLE, _T("WarnRestartOpt"));
      m_bRestartWarning = FALSE;
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

static size_t GetBrowseButtonWidth(wxWindow *win)
{
   static size_t s_widthBtn = 0;
   if ( s_widthBtn == 0 ) {
      // calculate it only once, it's almost a constant
      s_widthBtn = 2*win->GetCharWidth();
   }

   return s_widthBtn;
}

static wxArrayString SplitLabelWithChoices(wxString *label)
{
   wxArrayString choices;

   CHECK( label, choices, _T("NULL label in SplitLabelWithChoices") );

   choices = strutil_restore_array(*label);
   if ( !choices.IsEmpty() )
   {
      *label = choices[0u];
      choices.RemoveAt(0u);
   }

   return choices;
}

