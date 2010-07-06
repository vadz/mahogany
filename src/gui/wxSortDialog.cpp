///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   gui/wxSortDialog.cpp - implements ConfigureSorting()
// Purpose:     implements the dialog for configuring the message sorting
// Author:      Vadim Zeitlin
// Modified by:
// Created:     30.08.01 (extracted from gui/wxMFolderDialogs.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "Sorting.h"
   #include "Mdefaults.h"
   #include "MHelp.h"
   #include "guidef.h"
   #include "Profile.h"

   #include <wx/layout.h>
   #include <wx/stattext.h>     // for wxStaticText
   #include <wx/statbox.h>
   #include <wx/dcclient.h>     // for wxClientDC
   #include <wx/settings.h>     // for wxSystemSettings
#endif // USE_PCH

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// options we use
// ----------------------------------------------------------------------------

extern const MOption MP_MSGS_SERVER_SORT;
extern const MOption MP_MSGS_SORTBY;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define NUM_SORTLEVELS 5

/* These must not be more than 16, as they are stored in a 4-bit
   value! They must be in sync with the enum in MailFolder.h.
*/
static const char *sortCriteria[] =
{
   gettext_noop("Arrival order"),
   gettext_noop("Date"),
   gettext_noop("Subject"),
   gettext_noop("Author"),
   gettext_noop("Status"),
   gettext_noop("Score"),
   gettext_noop("Size"),
};

static const size_t NUM_CRITERIA  = WXSIZEOF(sortCriteria);

#define NUM_LABELS 2
static wxString labels[NUM_LABELS] =
{
   gettext_noop("First, sort by"),
   gettext_noop("then, sort by")
};

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class wxMessageSortingDialog : public wxOptionsPageSubdialog
{
public:
   wxMessageSortingDialog(Profile *profile, wxWindow *parent);

   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   bool WasChanged(void) const { return m_wasChanged; }

protected:
   wxChoice    *m_Choices[NUM_CRITERIA];     // sort by what?
   wxCheckBox  *m_Checkboxes[NUM_CRITERIA];  // reverse sort order?

   wxCheckBox  *m_checkUseServerSort;        // use server side sorting?

   // dirty flag
   bool         m_wasChanged;

   // the dialog data
   bool         m_UseServerSort;
   long         m_SortOrder;

   DECLARE_NO_COPY_CLASS(wxMessageSortingDialog)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMessageSortingDialog
// ----------------------------------------------------------------------------

wxMessageSortingDialog::wxMessageSortingDialog(Profile *profile,
                                               wxWindow *parent)
                      : wxOptionsPageSubdialog(profile,parent,
                                               _("Message sorting"),
                                               _T("MessageSortingDialog"))
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("&Sort messages by"), FALSE,
                                             MH_DIALOG_SORTING);

   wxClientDC dc(this);
   dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
   wxCoord width, widthMax = 0;

   // see the comment near sortCriteria definition
   ASSERT_MSG( NUM_CRITERIA < 16, _T("too many sort criteria") );

   // should have enough space for them in a long
   ASSERT_MSG( NUM_SORTLEVELS < 8, _T("too many sort levels") );

   size_t n;
   for ( n = 0; n < NUM_LABELS; n++ )
   {
      dc.GetTextExtent(labels[n], &width, NULL);
      if ( width > widthMax ) widthMax = width;
   }

   wxLayoutConstraints *c;
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();

   wxStaticText *msg = new wxStaticText
                           (
                              this,
                              -1,
                              _("\"Arrival order\" sorting order means that "
                                "the messages are not sorted at all. Please\n"
                                "note that using any other sorting method may "
                                "slow down the program significantly,\n"
                                "especially for remote folders without "
                                "server-side sorting support.")
                           );
   msg->SetConstraints(c);

   for( n = 0; n < NUM_SORTLEVELS; n++)
   {
      wxStaticText *txt = new wxStaticText(this, -1,
                                           n < NUM_LABELS
                                           ? wxGetTranslation(labels[n])
                                           : wxGetTranslation(labels[NUM_LABELS-1]));
      c = new wxLayoutConstraints;
      c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
      c->width.Absolute(widthMax);
      if ( n == 0 )
         c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
      else
         c->top.Below(m_Choices[n-1], 2*LAYOUT_Y_MARGIN);
      c->height.AsIs();
      txt->SetConstraints(c);

      m_Checkboxes[n] = new wxCheckBox(this, -1, _("in &reverse order"),
                                       wxDefaultPosition, wxDefaultSize);
      c = new wxLayoutConstraints;
      c->width.AsIs();
      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      c->centreY.SameAs(txt, wxCentreY);
      c->height.AsIs();
      m_Checkboxes[n]->SetConstraints(c);

      wxString sortCriteriaTrans[NUM_CRITERIA];
      for ( size_t i = 0; i < NUM_CRITERIA; i++ )
      {
         sortCriteriaTrans[i] = wxGetTranslation(sortCriteria[i]);
      }

      m_Choices[n] = new wxChoice(this, -1,
                                  wxDefaultPosition, wxDefaultSize,
                                  NUM_CRITERIA, sortCriteriaTrans);
      c = new wxLayoutConstraints;
      c->left.RightOf(txt, 2*LAYOUT_X_MARGIN);
      c->right.SameAs(m_Checkboxes[n], wxLeft, 2*LAYOUT_X_MARGIN);
      c->centreY.SameAs(txt, wxCentreY);
      c->height.AsIs();
      m_Choices[n]->SetConstraints(c);
   }

   msg = new wxStaticText
             (
               this,
               -1,
               _("Some IMAP servers support server side sorting. It may "
                 "be much faster to use it in this\n"
                 "case as it avoids having to download all messages just "
                 "to sort them.")
             );
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_Choices[n - 1], 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   m_checkUseServerSort = new wxCheckBox
                              (
                                 this,
                                 -1,
                                 _("Use &server-side sorting if available")
                              );

   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_checkUseServerSort->SetConstraints(c);

   SetDefaultSize(7*wBtn, 18*hBtn);

   m_wasChanged = false;
}

bool wxMessageSortingDialog::TransferDataToWindow()
{
   long sortOrder = READ_CONFIG(GetProfile(), MP_MSGS_SORTBY);

   // remmeber the initial values
   m_SortOrder = sortOrder;
   m_UseServerSort = READ_CONFIG_BOOL(GetProfile(), MP_MSGS_SERVER_SORT);

   /* Sort order is stored as 4 bits per hierarchy:
      0xdcba --> 1. sort by "a", then by "b", ...
   */

   for ( int n = 0; n < NUM_SORTLEVELS; n++)
   {
      MessageSortOrder crit = GetSortCritDirect(sortOrder);

      ASSERT(n < NUM_SORTLEVELS);

      if ( IsSortCritReversed(sortOrder) )
      {
         // it is MSO_XXX_REV
         m_Checkboxes[n]->SetValue(TRUE);
      }

      m_Choices[n]->SetSelection(crit / 2);

      sortOrder = GetSortNextCriterium(sortOrder);
   }

   m_checkUseServerSort->SetValue(m_UseServerSort);

   return TRUE;
}

bool wxMessageSortingDialog::TransferDataFromWindow()
{
   bool uses_scoring = false;
   int selection;
   long sortOrder = 0;
   for( int n = NUM_SORTLEVELS-1; n >= 0; n--)
   {
      sortOrder <<= 4;
      selection = 2*m_Choices[n]->GetSelection();
      if( selection == MSO_SCORE )
         uses_scoring = true;

      if ( m_Checkboxes[n]->GetValue() )
      {
         // reverse the sort order: MSO_XXX_REV == MSO_XXX + 1
         selection++;
      }

      sortOrder += selection;
   }

   // write the data to config if anything changed (and update the dirty flag)
   if ( sortOrder != m_SortOrder )
   {
      m_SortOrder = sortOrder;
      m_wasChanged = true;

      GetProfile()->writeEntry(MP_MSGS_SORTBY, m_SortOrder);
   }

   if ( m_checkUseServerSort->GetValue() != m_UseServerSort )
   {
      m_UseServerSort = !m_UseServerSort;
      m_wasChanged = true;

      GetProfile()->writeEntry(MP_MSGS_SERVER_SORT, m_UseServerSort);
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern
bool ConfigureSorting(Profile *profile, wxWindow *parent)
{
   wxMessageSortingDialog dlg(profile, parent);

   return dlg.ShowModal() == wxID_OK && dlg.WasChanged();
}

