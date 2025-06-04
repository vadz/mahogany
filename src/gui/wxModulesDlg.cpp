///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxModulesDlg.cpp
// Purpose:     dialog to configure the modules [not] to load
// Author:      Karsten Ballüder
// Modified by: Vadim Zeitlin on 2004-07-11 to select modules not to be loaded
// Created:     13.04.01 (extracted from src/gui/wxComposeView.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1999-2004 M-Team
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
#  include "MHelp.h"
#  include "strutil.h"
#  include "Mdefaults.h"
#  include "guidef.h"
#  include "MApplication.h"
#  include "Profile.h"

#  include <wx/stattext.h>      // for wxStaticText
#  include <wx/statbox.h>
#  include <wx/checklst.h>
#  include <wx/layout.h>
#endif // USE_PCH

#include "Mpers.h"
#include "MModule.h"

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// options and message boxes we use here
// ----------------------------------------------------------------------------

extern const MOption MP_MODULES_DONT_LOAD;

extern const MPersMsgBox *M_MSGBOX_MODULES_WARNING;

// ----------------------------------------------------------------------------
// wxModulesDialog
// ----------------------------------------------------------------------------

class wxModulesDialog : public wxManuallyLaidOutDialog
{
public:
   wxModulesDialog(wxWindow *parent);

   // check the modules which are been currently loaded
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnListBox(wxCommandEvent & ev);
   bool InternalUpdate(size_t n);

   virtual ~wxModulesDialog();

protected:
   MModuleListing *m_Listing;
   wxCheckListBox *m_checklistBox;
   wxTextCtrl     *m_textCtrl;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxModulesDialog)
};

BEGIN_EVENT_TABLE(wxModulesDialog, wxDialog)
   EVT_LISTBOX(-1, wxModulesDialog::OnListBox)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

wxModulesDialog::wxModulesDialog(wxWindow *parent)
               : wxManuallyLaidOutDialog(parent,
                                         _("Loadable Modules Configuration"),
                                         _T("ModulesDialog"))
{
   m_Listing = MModule::ListAvailableModules();

   // create controls
   wxStaticBox *box = CreateStdButtonsAndBox(_("Available modules"), FALSE,
                                             MH_DIALOG_MODULES);
   wxLayoutConstraints *c;

   // create a short help message above
   wxStaticText *
      msg = new wxStaticText
                (
                  this,
                  -1,
                  _("The modules checked in the list below will be used by\n"
                  "Mahogany as needed, uncheck a module to never load it.")
                );

   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   // create the checklistbox in the area which is left
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->height.PercentOf(box, wxHeight, 60);

   m_checklistBox = new wxCheckListBox(this, -1);
   m_checklistBox->SetConstraints(c);


   m_textCtrl = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
                               wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_checklistBox, 2*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_textCtrl->SetConstraints(c);

   SetDefaultSize(380, 400);
}

wxModulesDialog::~wxModulesDialog()
{
   if(m_Listing)
      m_Listing->DecRef();
}

void
wxModulesDialog::OnListBox(wxCommandEvent & ev)
{
   int n = ev.GetInt(); // which one was clicked

   InternalUpdate(n);
}

bool
wxModulesDialog::InternalUpdate(size_t n)
{
   ASSERT(n < m_Listing->Count());
   m_textCtrl->Clear();
   *m_textCtrl
      << _("Module: ") << (*m_Listing)[n].GetName() << '\n'
      << _("Interface: ") << (*m_Listing)[n].GetInterface() << '\n'
      << _("Version: ") <<  (*m_Listing)[n].GetVersion() << '\n'
      << _("Author: ") <<   (*m_Listing)[n].GetAuthor() << '\n'
      << '\n'
      << (*m_Listing)[n].GetDescription() << '\n';
   m_textCtrl->ShowPosition(0); // no effect for wxGTK :-(
   m_textCtrl->SetInsertionPoint(0);
   return TRUE;
}

bool wxModulesDialog::TransferDataToWindow()
{
   if ( !m_Listing )
      return FALSE;

   // get list of modules which shouldn't be loaded
   wxString modulesStr = READ_APPCONFIG(MP_MODULES_DONT_LOAD);
   wxArrayString modules = strutil_restore_array(modulesStr);

   // add the items to the checklistbox
   const size_t count = m_Listing->Count();
   for ( size_t n = 0; n < count; n++ )
   {
      const MModuleListingEntry& entry = (*m_Listing)[n];

      const wxString
            name(entry.GetName()),
            desc(entry.GetShortDescription());

      m_checklistBox->Append(desc.empty() ? name : desc);
      m_checklistBox->Check(n, modules.Index(name) == wxNOT_FOUND);
   }

   InternalUpdate(0);

   return TRUE;
}

bool wxModulesDialog::TransferDataFromWindow()
{
   wxArrayString modules;
   const size_t count = (size_t)m_checklistBox->GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( !m_checklistBox->IsChecked(n) )
      {
         modules.Add((*m_Listing)[n].GetName());
      }
   }

   mApplication->GetProfile()->
      writeEntry(MP_MODULES_DONT_LOAD, strutil_flatten_array(modules));

   return TRUE;
}

// ----------------------------------------------------------------------------
// our public API
// ----------------------------------------------------------------------------

extern
void ShowModulesDialog(wxFrame *parent)
{
   wxModulesDialog dlg(parent);
   if ( dlg.ShowModal() == wxID_OK )
   {
      MDialog_Message(
         _("Notice: most changes to the modules settings will only\n"
           "take effect the next time you start Mahogany."),
         parent,
         MDIALOG_MSGTITLE,
         GetPersMsgBoxName(M_MSGBOX_MODULES_WARNING));
   }
}
