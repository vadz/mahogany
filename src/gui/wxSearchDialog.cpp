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
   #include "Mdefaults.h"
   #include "guidef.h"

   #include <wx/layout.h>
   #include <wx/stattext.h>     // for wxStaticText
   #include <wx/statbox.h>
#endif // USE_PCH

#include "MFolder.h"
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

static const wxChar *searchCriteria[] =
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
                         Profile *profile,
                         const MFolder *folder,
                         wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

protected:
   // event handlers
   void OnUpdateUIOk(wxUpdateUIEvent& event);
   void OnUpdateUIRemove(wxUpdateUIEvent& event);
   void OnButtonAdd(wxCommandEvent& event);
   void OnButtonRemove(wxCommandEvent& event);

   SearchCriterium *m_CritStruct;
   int           m_Criterium;
   String        m_Arg;

   // GUI controls
   wxChoice     *m_choiceWhere;
   wxCheckBox   *m_chkInvert;
   wxPTextEntry *m_textWhat;
   wxListBox    *m_lboxFolders;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxMessageSearchDialog)
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMessageSearchDialog, wxOptionsPageSubdialog)
   EVT_BUTTON(Btn_Add, wxMessageSearchDialog::OnButtonAdd)
   EVT_BUTTON(Btn_Remove, wxMessageSearchDialog::OnButtonRemove)

   EVT_UPDATE_UI(wxID_OK, wxMessageSearchDialog::OnUpdateUIOk)
   EVT_UPDATE_UI(Btn_Remove, wxMessageSearchDialog::OnUpdateUIRemove)
END_EVENT_TABLE()

// ============================================================================
// wxMessageSearchDialog implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor
// ----------------------------------------------------------------------------

wxMessageSearchDialog::wxMessageSearchDialog(SearchCriterium *crit,
                                             Profile *profile,
                                             const MFolder *folder,
                                             wxWindow *parent)
                     : wxOptionsPageSubdialog(profile, parent,
                                              _("Search for messages"),
                                              "MessageSearchDialog")
{
   ASSERT_MSG( crit, _T("NULL SearchCriterium pointer in wxMessageSearchDialog") );

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
   m_chkInvert = new wxCheckBox(this, -1, _("&not"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.SameAs(box, wxTop, 5*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_chkInvert->SetConstraints(c);

   label = new wxStaticText(this, -1, _("&containing"));
   c = new wxLayoutConstraints;
   c->left.RightOf(m_chkInvert);
   c->width.AsIs();
   // this shouldn't be needed but otherwise under MSW the static text is one
   // pixel lower than the checkbox which is really ugly -- apparently there is
   // an off by 1 error somewhere in the constraints calculation code?
#ifdef OS_WIN
   c->top.SameAs(m_chkInvert, wxTop);
#else
   c->centreY.SameAs(m_chkInvert, wxCentreY);
#endif
   c->height.AsIs();
   label->SetConstraints(c);

   m_textWhat = new wxPTextEntry("SearchFor", this, -1);
   c = new wxLayoutConstraints;
   c->left.RightOf(label, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(label, wxCentreY);
   c->height.AsIs();
   m_textWhat->SetConstraints(c);

   // second line: In [Where___]
   wxString searchCriteriaTrans[WXSIZEOF(searchCriteria)];
   for ( size_t n = 0; n < WXSIZEOF(searchCriteria); n++ )
   {
      searchCriteriaTrans[n] = _(searchCriteria[n]);
   }

   m_choiceWhere = new wxPChoice("SearchWhere", this, -1,
                                 wxDefaultPosition, wxDefaultSize,
                                 WXSIZEOF(searchCriteria), searchCriteriaTrans);
   c = new wxLayoutConstraints;
   c->left.SameAs(m_textWhat, wxLeft);
   c->right.SameAs(m_textWhat, wxRight);
   c->top.Below(m_textWhat, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_choiceWhere->SetConstraints(c);

   label = new wxStaticText(this, -1, _("&in"));
   c = new wxLayoutConstraints;
   c->right.LeftOf(m_choiceWhere, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->centreY.SameAs(m_choiceWhere, wxCentreY);
   c->height.AsIs();
   label->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->bottom.SameAs(m_choiceWhere, wxBottom, -3*LAYOUT_Y_MARGIN);
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

   wxButton *btnRemove = new wxButton(this, Btn_Remove, _("&Remove"));
   c = new wxLayoutConstraints;
   c->width.AsIs();
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxCentreY, LAYOUT_Y_MARGIN);
   c->height.AsIs();
   btnRemove->SetConstraints(c);

   m_lboxFolders = new wxListBox(this, -1);
   if ( folder )
      m_lboxFolders->Append('/' + folder->GetFullName());

   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(btnAdd, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 5*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 3*LAYOUT_Y_MARGIN);
   m_lboxFolders->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->top.SameAs(m_choiceWhere, wxBottom, 5*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(FindWindow(wxID_OK), wxTop, 3*LAYOUT_Y_MARGIN);
   box->SetConstraints(c);

   SetDefaultSize(5*wBtn, 14*hBtn);
}

// ----------------------------------------------------------------------------
// wxMessageSearchDialog data transfer
// ----------------------------------------------------------------------------

bool wxMessageSearchDialog::TransferDataToWindow()
{
   // see TransferDataFromWindow() for the explanation of SEARCH_CRIT_MASK: now
   // we simply unpack that word back
   m_Criterium = READ_CONFIG(GetProfile(), MP_MSGS_SEARCH_CRIT);
   m_choiceWhere->SetSelection(m_Criterium & SEARCH_CRIT_MASK);

   m_chkInvert->SetValue((m_Criterium & SEARCH_CRIT_INVERT_FLAG) != 0);

   m_Arg = READ_CONFIG_TEXT(GetProfile(), MP_MSGS_SEARCH_ARG);
   m_textWhat->SetValue(m_Arg);

   m_choiceWhere->SetFocus();

   return TRUE;
}

bool wxMessageSearchDialog::TransferDataFromWindow()
{
   m_Criterium = m_choiceWhere->GetSelection();
   m_CritStruct->m_What = (SearchCriterium::Type) m_Criterium;

   m_CritStruct->m_Invert = m_chkInvert->GetValue();
   if ( m_CritStruct->m_Invert )
   {
      // we combine the values of m_chkInvert and m_choiceWhere into one config
      // entry
      m_Criterium |= SEARCH_CRIT_INVERT_FLAG;
   }

   m_Arg = m_textWhat->GetValue();

   GetProfile()->writeEntry(MP_MSGS_SEARCH_CRIT, m_Criterium);
   GetProfile()->writeEntry(MP_MSGS_SEARCH_ARG, m_Arg);

   m_CritStruct->m_Key = m_Arg;

   wxString s;
   m_CritStruct->m_Folders.Empty();
   size_t count = m_lboxFolders->GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      s = m_lboxFolders->GetString(n);
      m_CritStruct->m_Folders.Add(s.c_str() + 1); // skip leading slash
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// wxMessageSearchDialog event handlers
// ----------------------------------------------------------------------------

void wxMessageSearchDialog::OnUpdateUIOk(wxUpdateUIEvent& event)
{
   // we must have something and somewhere to search for
   event.Enable( !m_textWhat->GetValue().empty() &&
                     m_lboxFolders->GetCount() != 0 );
}

void wxMessageSearchDialog::OnUpdateUIRemove(wxUpdateUIEvent& event)
{
   event.Enable( m_lboxFolders->GetSelection() != -1 );
}

void wxMessageSearchDialog::OnButtonAdd(wxCommandEvent& /* event */)
{
   MFolder_obj folder(MDialog_FolderChoose(this, NULL, MDlg_Folder_NoFiles));
   if ( folder )
   {
      m_lboxFolders->Append('/' + folder->GetFullName());
   }
}

void wxMessageSearchDialog::OnButtonRemove(wxCommandEvent& /* event */)
{
   int sel = m_lboxFolders->GetSelection();
   if ( sel == -1 )
      return;

   m_lboxFolders->Delete(sel);
}

// ----------------------------------------------------------------------------
// wxMessageSearchDialog public API
// ----------------------------------------------------------------------------

// Configuration dialog for searching message
extern
bool ConfigureSearchMessages(SearchCriterium *crit,
                             Profile *profile,
                             const MFolder *folder,
                             wxWindow *parent)
{
   wxMessageSearchDialog dlg(crit, profile, folder, parent);

   return dlg.ShowModal() == wxID_OK;
}

