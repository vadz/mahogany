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

   #include "wx/layout.h"
   #include "wx/statbox.h"
   #include "wx/stattext.h"             // for wxStaticText
   #include "wx/checklst.h"
#endif // USE_PCH

#include "gui/wxSelectionDlg.h"

#include "wx/spinctrl.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// dialog control ids
enum
{
   Check_MakeDefault,
   Check_Reset
};

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// this class adds a lower pane to wxSelectionsOrderDialog which allows to
// configure the widths of the columns we show as well
class wxFolderViewColumnsDialog : public wxSelectionsOrderDialogSimple
{
public:
   wxFolderViewColumnsDialog(const String& folderName,
                             wxArrayString *names,
                             wxArrayInt *status,
                             wxArrayInt *widths,
                             bool *asDefault,
                             wxWindow *parent);

   virtual ~wxFolderViewColumnsDialog();

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   // update m_idxTrans table here
   virtual void OnItemSwap(size_t item1, size_t item2);

   // event handlers
   void OnCheckListbox(wxCommandEvent& event);
   void OnMakeDefault(wxCommandEvent& event);
   void OnReset(wxCommandEvent& event);
   void OnSpinChanged(wxCommandEvent& WXUNUSED(event)) { m_hasChanges = true; }

   // enable or disable the controls for editing width of this column depending
   // on the current values of the other controls
   void UpdateWidthState(size_t n);

private:
   // the number of columns
   size_t      m_countCol;

   // the index of the width field (i.e. index into m_spins and m_labels
   // arrays) corresponding to the given check list box item
   //
   // this is needed because although initially the columns and the widths are
   // in sync, the user may change the columns order using the buttons in the
   // dialog later and we should still be able to disable the correct field in
   // UpdateWidthState() when he hides/shows a column
   //
   // another solution would be to move the width controls themselves when the
   // columns are moved (or not really move but just change label/value) but I
   // think it would be confusing to the user if the controls started jumping
   // around
   size_t     *m_idxTrans;

   // pointer to callers "make default" flag
   bool       *m_asDefault;

   // the columns widths
   wxArrayInt *m_widths;

   // and the controls to edit them with their labels
   wxSpinCtrl **m_spins;
   wxStaticText **m_labels;

   // the checkboxes to make default/reset to default the widths
   wxCheckBox *m_chkMakeDef,
              *m_chkReset;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxFolderViewColumnsDialog)
};

// ============================================================================
// implementation
// ============================================================================

BEGIN_EVENT_TABLE(wxFolderViewColumnsDialog, wxSelectionsOrderDialogSimple)
   EVT_CHECKLISTBOX(-1, wxFolderViewColumnsDialog::OnCheckListbox)

   EVT_CHECKBOX(Check_MakeDefault, wxFolderViewColumnsDialog::OnMakeDefault)
   EVT_CHECKBOX(Check_Reset,       wxFolderViewColumnsDialog::OnReset)

   EVT_TEXT(-1, wxFolderViewColumnsDialog::OnSpinChanged)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFolderViewColumnsDialog
// ----------------------------------------------------------------------------

wxFolderViewColumnsDialog::
wxFolderViewColumnsDialog(const String& folderName,
                          wxArrayString* names,
                          wxArrayInt* status,
                          wxArrayInt* widths,
                          bool *asDefault,
                          wxWindow *parent)
    : wxSelectionsOrderDialogSimple
      (
         _("&Select the columns to show:"),
         String::Format(_("Configure columns for '%s'"), folderName.c_str()),
         names,
         status,
         "FolderViewCol",
         parent
      )
{
   m_countCol = m_choices->GetCount();
   m_idxTrans = new size_t[m_countCol];
   m_widths = widths;

   ASSERT_MSG( m_countCol == widths->GetCount(),
               _T("arrays should have the same size") );

   m_asDefault = asDefault;

   wxLayoutConstraints *c;

   // add another static box below the existing one
   wxStaticBox *boxLower = new wxStaticBox(this, -1,
                                           _("Set column &widths (in pixels)"));
   c = new wxLayoutConstraints();
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->top.Below(m_box, 2*LAYOUT_Y_MARGIN);
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
   m_spins = new wxSpinCtrl *[m_countCol];
   m_labels = new wxStaticText *[m_countCol];

   long widthMax = GetMaxLabelWidth(*m_choices, this) + 10; // for ": "

   for ( size_t n = 0; n < m_countCol; n++ )
   {
      // create and position the label
      m_labels[n] = new wxStaticText(this, -1, m_choices->Item(n) + ": ");

      c = new wxLayoutConstraints();
      c->left.SameAs(boxLower, wxLeft, LAYOUT_X_MARGIN);
      c->width.Absolute(widthMax);
      c->height.AsIs();
      if ( n )
      {
         // not the top one, put it below the previous one
         c->top.Below(m_labels[n - 1], 2*LAYOUT_Y_MARGIN);
      }
      else // top label
      {
         c->top.SameAs(boxLower, wxTop, 4*LAYOUT_Y_MARGIN);
      }

      m_labels[n]->SetConstraints(c);

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
      c->left.RightOf(m_labels[n], LAYOUT_X_MARGIN);
      c->right.SameAs(boxLower, wxRight, LAYOUT_X_MARGIN);
      c->height.AsIs();
      c->centreY.SameAs(m_labels[n], wxCentreY);

      m_spins[n]->SetConstraints(c);

      // also init m_idxTrans while we're in the loop: initially the order is
      // the same for the columns and the widths
      m_idxTrans[n] = n;
   }

   m_chkMakeDef = new wxCheckBox(this, Check_MakeDefault, _("Save as &default"));
   c = new wxLayoutConstraints();
   c->width.AsIs();
   c->centreX.SameAs(boxLower, wxCentreX);
   c->height.AsIs();
   c->top.Below(m_spins[m_countCol - 1], 2*LAYOUT_Y_MARGIN);
   m_chkMakeDef->SetConstraints(c);

   m_chkReset = new wxCheckBox(this, Check_Reset, _("&Reset to default"));
   c = new wxLayoutConstraints();
   c->width.AsIs();
   c->centreX.SameAs(boxLower, wxCentreX);
   c->height.AsIs();
   c->top.Below(m_chkMakeDef, LAYOUT_Y_MARGIN);
   m_chkReset->SetConstraints(c);

   // set the minimal window size
   SetDefaultSize(5*wBtn, 19*hBtn);
}

wxFolderViewColumnsDialog::~wxFolderViewColumnsDialog()
{
   delete [] m_idxTrans;
   delete [] m_spins;
   delete [] m_labels;
}

// ----------------------------------------------------------------------------
// wxFolderViewColumnsDialog data transfer
// ----------------------------------------------------------------------------

void wxFolderViewColumnsDialog::OnItemSwap(size_t item1, size_t item2)
{
   CHECK_RET( item1 < m_countCol && item2 < m_countCol,
              _T("invalid item in OnItemSwap") );

   ASSERT_MSG( item1 != item2, _T("swapping an item with itself?") );

   // swap the corresponding items in our translation table
   size_t tmp = m_idxTrans[item1];
   m_idxTrans[item1] = m_idxTrans[item2];
   m_idxTrans[item2] = tmp;

   UpdateWidthState(item1);
   UpdateWidthState(item2);
}

bool wxFolderViewColumnsDialog::TransferDataToWindow()
{
   if ( !wxSelectionsOrderDialogSimple::TransferDataToWindow() )
      return false;

   // disable the controls corresponding to the hidden columns
   for ( size_t n = 0; n < m_countCol; n++ )
   {
      UpdateWidthState(n);
   }

   // the widths are already there as we set the values of the spin controls in
   // the ctor, so nothing more to do

   return true;
}

bool wxFolderViewColumnsDialog::TransferDataFromWindow()
{
   if ( !wxSelectionsOrderDialogSimple::TransferDataFromWindow() )
      return false;

   m_widths->Empty();

   // by convention, if the widths are reset to default, we don't fill the
   // array at all
   if ( !m_chkReset->IsChecked() )
   {
      for ( size_t n = 0; n < m_countCol; n++ )
      {
         if ( m_checklstBox->IsChecked(n) )
         {
            m_widths->Add(m_spins[m_idxTrans[n]]->GetValue());
         }
         //else: don't return the widths for the columns which are not shown
      }
   }

   // return the "make default" checkbox value
   *m_asDefault = m_chkMakeDef->IsChecked();

   return true;
}

// ----------------------------------------------------------------------------
// wxFolderViewColumnsDialog event handlers
// ----------------------------------------------------------------------------

void wxFolderViewColumnsDialog::UpdateWidthState(size_t n)
{
   bool shouldEnable = !m_chkReset->IsChecked() && m_checklstBox->IsChecked(n);

   n = m_idxTrans[n];

   m_labels[n]->Enable(shouldEnable);
   m_spins[n]->Enable(shouldEnable);
}

void wxFolderViewColumnsDialog::OnCheckListbox(wxCommandEvent& event)
{
   // if the column is not shown, don't allow editing its width
   UpdateWidthState(event.GetInt());

   event.Skip();
}

void wxFolderViewColumnsDialog::OnMakeDefault(wxCommandEvent& WXUNUSED(event))
{
   // we can't reset the column widths if we want to make them default
   m_chkReset->Enable( !m_chkMakeDef->IsChecked() );
}

void wxFolderViewColumnsDialog::OnReset(wxCommandEvent& WXUNUSED(event))
{
   // neither does it make sense to set as default the widths if we want to
   // reset them to, precisely, the default values
   m_chkMakeDef->Enable( !m_chkReset->IsChecked() );

   // also, if we reset them changing them is futile
   for ( size_t n = 0; n < m_countCol; n++ )
   {
      UpdateWidthState(n);
   }
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern
bool ShowFolderViewColumnDialog(const String& folderName,
                                wxArrayString *names,
                                wxArrayInt *status,
                                wxArrayInt *widths,
                                bool *asDefault,
                                wxWindow *parent)
{
   CHECK( names && status && widths && asDefault, false,
          _T("NULL pointer in ShowFolderViewColumnDialog") );

   wxFolderViewColumnsDialog dlg(folderName, names, status, widths,
                                 asDefault, parent);

   // don't return false if either checkbox was checked as otherwise
   // the caller is not going to do anything at all, although it should
   return dlg.ShowModal() == wxID_OK &&
            (*asDefault || widths->IsEmpty() || dlg.HasChanges());
}

