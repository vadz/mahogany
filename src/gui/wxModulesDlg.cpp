/*-*- c++ -*-********************************************************
 * wxModulesDlg.cpp -  a dialog to choose which modules to load     *
 *                                                                  *
 * (C) 1999      by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "MApplication.h"
#   include "gui/wxMApp.h"
#   include "MHelp.h"
#   include "gui/wxMIds.h"
#   include "strutil.h"
#endif

#include "Mdefaults.h"

#include "MModule.h"
#include "gui/wxMDialogs.h"

#include <wx/defs.h>
#include <wx/event.h>
#include <wx/log.h>
#include <wx/persctrl.h>
#include <wx/control.h>
#include <wx/layout.h>
#include <wx/stattext.h>
#include <wx/listbox.h>
#include <wx/checkbox.h>
#include <wx/radiobox.h>
#include <wx/combobox.h>
#include <wx/statbox.h>
#include <wx/statbmp.h>
#include <wx/checklst.h>
#include <wx/dialog.h>

#include "gui/wxDialogLayout.h"

class wxModulesDialog : public wxManuallyLaidOutDialog
{
public:
   wxModulesDialog(wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();

   bool Update(wxCommandEvent & ev);
   bool InternalUpdate(size_t n);
   ~wxModulesDialog();
protected:
   MModuleListing *m_Listing;
   kbStringList    m_Modules;
   wxCheckListBox *m_checklistBox;
   wxTextCtrl     *m_textCtrl;
   kbStringList::iterator FindInList(const wxString &module) const;
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxModulesDialog, wxDialog)
//   EVT_CHECKLISTBOX(-1, wxModulesDialog::Update)
   EVT_LISTBOX(-1, wxModulesDialog::Update)
END_EVENT_TABLE()


wxModulesDialog::wxModulesDialog(wxWindow *parent)
   : wxManuallyLaidOutDialog( parent,
                              _("Extension Modules Configuration"),
                              "ModulesDialog")
{
   SetDefaultSize(380,400);

   m_Listing = MModule::GetListing();

   // get list of configured modules
   wxString modules = READ_APPCONFIG(MP_MODULES);
   char *tmp = strutil_strdup(modules);
   strutil_tokenise(tmp, ":", m_Modules);

   wxStaticBox *box = CreateStdButtonsAndBox(_("Available modules"),MH_DIALOG_MODULES);
   wxLayoutConstraints *c;

   // create a short help message above
   wxStaticText *msg = new wxStaticText( this, -1, _("The selected modules will be loaded\n"
                                                     "into Mahogany at the next program start.") );

   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   // create the checklistbox in the area which is left
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, -2*LAYOUT_X_MARGIN);
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->height.PercentOf(box, wxHeight, 60);

   m_checklistBox = new wxCheckListBox(this, -1);
   m_checklistBox->SetConstraints(c);


   m_textCtrl = new wxTextCtrl(this, -1, "", wxDefaultPosition,
                               wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 3*LAYOUT_X_MARGIN);
   c->top.Below(m_checklistBox, 2*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_textCtrl->SetConstraints(c);
   
   size_t count = m_Listing ? m_Listing->Count() : 0;

   kbStringList::iterator i;
   // add the items to the checklistbox
   for ( size_t n = 0; n < count; n++ )
   {
      m_checklistBox->Append((*m_Listing)[n].GetShortDescription());
      i = FindInList((*m_Listing)[n].GetName());
      if(i != m_Modules.end())
         m_checklistBox->Check(n, TRUE);
   }
   if(count)
      InternalUpdate(0);
}

wxModulesDialog::~wxModulesDialog()
{
   if(m_Listing)
      m_Listing->DecRef();
}

bool
wxModulesDialog::Update(wxCommandEvent & ev)
{
   int n = ev.GetInt(); // which one was clicked
   return InternalUpdate(n);
}

bool
wxModulesDialog::InternalUpdate(size_t n)
{
   ASSERT(n < m_Listing->Count());
   m_textCtrl->Clear();
   *m_textCtrl
      << _("Module: ") << (*m_Listing)[n].GetName() << '\n'
      << _("Version: ") <<  (*m_Listing)[n].GetVersion() << '\n'
      << _("Author: ") <<   (*m_Listing)[n].GetAuthor() << '\n'
      << '\n'
      << (*m_Listing)[n].GetDescription() << '\n';
   m_textCtrl->ShowPosition(0); // no effect for wxGTK :-(
   m_textCtrl->SetInsertionPoint(0);
   return TRUE;
}
   


bool wxModulesDialog::TransferDataFromWindow()
{
   wxString setting;
   size_t count = (size_t)m_checklistBox->Number();
   for ( size_t n = 0; n < count; n++ )
   {
      if(m_checklistBox->IsChecked(n))
      {
         setting << (*m_Listing)[n].GetName();
         if(n != count-1) setting << ':';
      }
   }
   mApplication->GetProfile()->writeEntry(MP_MODULES, setting);
   return TRUE;
}

kbStringList::iterator
wxModulesDialog::FindInList(const wxString &module) const
{
   kbStringList::iterator i;
   for(i = m_Modules.begin(); i != m_Modules.end(); i++)
      if( (**i) == module)
         return i;
   return m_Modules.end();
}


/// creates and shows the dialog allowing to choose modules
extern
void ShowModulesDialog(wxFrame *parent)
{
   wxModulesDialog dlg(parent);
   (void)dlg.ShowModal();
}
