///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxColumnsDlg.cpp
// Purpose:     implementation of the folder view column configuration dialog
// Author:      Vadim Zeitlin <vadim@wxwindows.org>
// Modified by:
// Created:     12.03.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Mahogany team
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
   #include "Mcommon.h"

   #include "wx/window.h"
   #include "wx/layout.h"
   #include "wx/statbox.h"
   #include "wx/stattext.h"
#endif

#include "gui/wxSelectionDlg.h"

#include "wx/checklst.h"
#include "wx/spinctrl.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// this class adds a lower pane to wxSelectionsOrderDialog which allows to
// configure the widths of the columns we show as well
class wxFolderViewColumnsDialog : public wxSelectionsOrderDialogSimple
{
public:
   wxFolderViewColumnsDialog(wxArrayString* names,
                             wxArrayInt* status,
                             wxArrayInt* widths,
                             wxWindow *parent);

   virtual ~wxFolderViewColumnsDialog();

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   wxArrayInt *m_widths;
   wxSpinCtrl **m_spins;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFolderViewColumnsDialog
// ----------------------------------------------------------------------------

wxFolderViewColumnsDialog::wxFolderViewColumnsDialog(wxArrayString* names,
                                                     wxArrayInt* status,
                                                     wxArrayInt* widths,
                                                     wxWindow *parent)
    : wxSelectionsOrderDialogSimple(_("&Select the columns to show:"),
                                    // TODO: show folder name in the caption
                                    _("Configure folder view columns"),
                                    names,
                                    status,
                                    "FolderViewCol",
                                    parent)
{
   m_widths = widths;

   wxLayoutConstraints *c;

   // add another static box below the existing one
   wxStaticBox *boxLower = new wxStaticBox(this, -1, _("Set column &widths"));
   c = new wxLayoutConstraints();
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->top.Below(m_box, 4*LAYOUT_Y_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(FindWindow(wxID_OK), wxTop, 4*LAYOUT_Y_MARGIN);
   boxLower->SetConstraints(c);

   // we have to fix the constraints of the controls created by the base class
   c = m_checklstBox->GetConstraints();
   c->bottom.Unconstrained();
   c->height.Absolute(120);

   c = m_box->GetConstraints();
   c->bottom.SameAs(m_checklstBox, wxBottom, -4*LAYOUT_Y_MARGIN);

   // create the controls for the widths
   size_t count = m_choices->GetCount();
   m_spins = new wxSpinCtrl *[count];

   long widthMax = GetMaxLabelWidth(*m_choices, this) + 10; // for ": "
   wxStaticText *label = NULL;

   for ( size_t n = 0; n < count; n++ )
   {
      // create and position the label
      c = new wxLayoutConstraints();
      c->left.SameAs(boxLower, wxLeft, LAYOUT_X_MARGIN);
      c->width.Absolute(widthMax);
      c->height.AsIs();
      if ( label )
      {
         // not the top one, put it below the previous one
         c->top.Below(label, 2*LAYOUT_Y_MARGIN);
      }
      else // top label
      {
         c->top.SameAs(boxLower, wxTop, 4*LAYOUT_Y_MARGIN);
      }

      label = new wxStaticText(this, -1, m_choices->Item(n) + ": ");
      label->SetConstraints(c);

      // create the corresponding spin ctrl
      //
      // max value is, of course, arbitraty, but what can we do?
      int w = m_widths->Item(n);
      m_spins[n] = new wxSpinCtrl(this, -1,
                                  wxString::Format("%d", w),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxSP_ARROW_KEYS,
                                  0, 1000, w);

      // ... and position it accordingly
      c = new wxLayoutConstraints();
      c->left.RightOf(label, LAYOUT_X_MARGIN);
      c->right.SameAs(boxLower, wxRight, LAYOUT_X_MARGIN);
      c->height.AsIs();
      c->centreY.SameAs(label, wxCentreY);

      m_spins[n]->SetConstraints(c);
   }

   // set the minimal window size
   SetDefaultSize(3*wBtn, 15*hBtn);
}

bool wxFolderViewColumnsDialog::TransferDataToWindow()
{
   if ( !wxSelectionsOrderDialogSimple::TransferDataToWindow() )
      return false;

   return true;
}

bool wxFolderViewColumnsDialog::TransferDataFromWindow()
{
   if ( !wxSelectionsOrderDialogSimple::TransferDataFromWindow() )
      return false;

   return true;
}

wxFolderViewColumnsDialog::~wxFolderViewColumnsDialog()
{
   delete [] m_spins;
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern
bool ShowFolderViewColumnDialog(wxArrayString* names,
                                wxArrayInt* status,
                                wxArrayInt* widths,
                                wxWindow *parent)
{
   wxFolderViewColumnsDialog dlg(names, status, widths, parent);

   return dlg.ShowModal() == wxID_OK && dlg.HasChanges();
}

