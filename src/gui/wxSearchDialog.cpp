///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   gui/wxSearchDialog.cpp - implements ShowSearchDialog()
// Purpose:     implements the dialog for specifying the search criteria
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.07.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
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

   #include "MHelp.h"

   #include "guidef.h"

   #include <wx/window.h>
   #include <wx/layout.h>

   #include <wx/checkbox.h>
   #include <wx/choice.h>
   #include <wx/stattext.h>
   #include <wx/statbox.h>
#endif // USE_PCH

#include "Mdefaults.h"

#include "MSearch.h"

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_MSGS_SEARCH_ARG;
extern const MOption MP_MSGS_SEARCH_CRIT;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const wxString searchCriteria[] =
{
   gettext_noop("Full Message"),
   gettext_noop("Message Content"),
   gettext_noop("Message Header"),
   gettext_noop("Subject Field"),
   gettext_noop("To Field"),
   gettext_noop("From Field"),
   gettext_noop("CC Field"),
};

#define SEARCH_CRIT_INVERT_FLAG  0x1000
#define SEARCH_CRIT_MASK  0x0FFF

enum
{
   Btn_Add = 100,
   Btn_Remove
};

// ----------------------------------------------------------------------------
// wxMessageSearchDialog
// ----------------------------------------------------------------------------

class wxMessageSearchDialog : public wxOptionsPageSubdialog
{
public:
   wxMessageSearchDialog(SearchCriterium *crit,
                         Profile *profile, wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

protected:
   void UpdateCritStruct(void)
   {
      m_CritStruct->m_What = (SearchCriterium::Type)
         m_Choices->GetSelection();
      m_CritStruct->m_Invert = m_Invert->GetValue();
      m_CritStruct->m_Key = m_Keyword->GetValue();
   }

   SearchCriterium *m_CritStruct;
   int           m_Criterium;
   String        m_Arg;

   // GUI controls
   wxChoice     *m_Choices;
   wxCheckBox   *m_Invert;
   wxPTextEntry *m_Keyword;
   wxListBox *m_lboxFolders;
};

// ============================================================================
// wxMessageSearchDialog implementation
// ============================================================================

wxMessageSearchDialog::wxMessageSearchDialog(SearchCriterium *crit,
                                             Profile *profile,
                                             wxWindow *parent)
                     : wxOptionsPageSubdialog(profile,parent,
                                              _("Search for messages"),
                                              "MessageSearchDialog")
{
   ASSERT_MSG( crit, "NULL SearchCriterium pointer in wxMessageSearchDialog" );

   m_CritStruct = crit;

   // create and layout the controls
   // ------------------------------

   wxLayoutConstraints *c;
   wxStaticText *label;

   // the buttons at the bottom
   CreateStdButtonsAndBox("", StdBtn_NoBox, MH_DIALOG_SEARCHMSGS);

   // the "where to search" box
   wxStaticBox *box = new wxStaticBox(this, -1, _("&Search for messages"));

   // first line: [x] Not containing [Text___]
   m_Invert = new wxCheckBox(this, -1, _("&Not"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.SameAs(box, wxTop, 5*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_Invert->SetConstraints(c);

   label = new wxStaticText(this, -1, _("&containing"));
   c = new wxLayoutConstraints;
   c->left.RightOf(m_Invert, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->centreY.SameAs(m_Invert, wxCentreY);
   c->height.AsIs();
   label->SetConstraints(c);

   m_Keyword = new wxPTextEntry("SearchFor", this, -1);
   c = new wxLayoutConstraints;
   c->left.RightOf(label, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(label, wxCentreY);
   c->height.AsIs();
   m_Keyword->SetConstraints(c);

   // second line: In [Where___]
   label = new wxStaticText(this, -1, _("&In"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.Below(m_Keyword, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   label->SetConstraints(c);

   m_Choices = new wxPChoice("SearchWhere", this, -1,
                             wxDefaultPosition, wxDefaultSize,
                             WXSIZEOF(searchCriteria), searchCriteria);
   c = new wxLayoutConstraints;
   c->left.RightOf(label, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(label, wxCentreY);
   c->height.AsIs();
   m_Choices->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->bottom.SameAs(m_Choices, wxBottom, -3*LAYOUT_Y_MARGIN);
   box->SetConstraints(c);


   // second box
   box = new wxStaticBox(this, -1, _("In these &folders:"));

   wxButton *btnAdd = new wxButton(this, Btn_Add, _("&Add..."));
   c = new wxLayoutConstraints;
   c->width.AsIs();
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxCentreY, LAYOUT_Y_MARGIN);
   c->height.AsIs();
   btnAdd->SetConstraints(c);

   m_lboxFolders = new wxListBox(this, -1);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(btnAdd, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 5*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 3*LAYOUT_Y_MARGIN);
   m_lboxFolders->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->top.SameAs(m_Choices, wxBottom, 5*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(FindWindow(wxID_OK), wxTop, 3*LAYOUT_Y_MARGIN);
   box->SetConstraints(c);

   SetDefaultSize(5*wBtn, 14*hBtn);
}


bool wxMessageSearchDialog::TransferDataFromWindow()
{
   m_Criterium = m_Choices->GetSelection();
   if(m_Invert->GetValue() != 0)
      m_Criterium |= SEARCH_CRIT_INVERT_FLAG;

   m_Arg = m_Keyword->GetValue();
   GetProfile()->writeEntry(MP_MSGS_SEARCH_CRIT, m_Criterium);
   GetProfile()->writeEntry(MP_MSGS_SEARCH_ARG, m_Arg);

   UpdateCritStruct();
   return TRUE;
}

bool wxMessageSearchDialog::TransferDataToWindow()
{
   m_Criterium = READ_CONFIG(GetProfile(), MP_MSGS_SEARCH_CRIT);
   m_Arg = READ_CONFIG_TEXT(GetProfile(), MP_MSGS_SEARCH_ARG);

   if ( m_Criterium & SEARCH_CRIT_MASK )
      m_Choices->SetSelection(m_Criterium & SEARCH_CRIT_MASK);

   m_Invert->SetValue((m_Criterium & SEARCH_CRIT_INVERT_FLAG) != 0);

   if ( !m_Arg.empty() )
      m_Keyword->SetValue(m_Arg);

   UpdateCritStruct();

   m_Choices->SetFocus();

   return TRUE;
}

/* Configuration dialog for searching messages. */
extern
bool ConfigureSearchMessages(SearchCriterium *crit,
                             Profile *profile, wxWindow *parent)
{
   wxMessageSearchDialog dlg(crit, profile, parent);
   return dlg.ShowModal() == wxID_OK;
}

