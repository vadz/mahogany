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
   wxModulesDialog(wxFrame *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();

   bool Update(wxCommandEvent & ev);
   ~wxModulesDialog();
protected:
   MModuleListing *m_Listing;
   kbStringList    m_Modules;
   wxCheckListBox *m_checklistBox;
   
   kbStringList::iterator FindInList(const wxString &module) const;
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxModulesDialog, wxDialog)
   EVT_CHECKLISTBOX(-1, wxModulesDialog::Update)
END_EVENT_TABLE()


wxModulesDialog::wxModulesDialog(wxFrame *parent)
   : wxManuallyLaidOutDialog( parent,
                              _("Mahogany : Extension Modules Configuration"),
                              "ModulesDialog")
{
   SetDefaultSize(380,400);

   m_Listing = MModule::GetListing();

   // get list of configured modules
   wxString modules = READ_APPCONFIG(MP_MODULES);
   char *tmp = strutil_strdup(modules);
   strutil_tokenise(tmp, ":", m_Modules);

   wxStaticBox *box = CreateStdButtonsAndBox(_("Available modules"));
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
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);

   m_checklistBox = new wxCheckListBox(this, -1);
   m_checklistBox->SetConstraints(c);


   size_t count = m_Listing ? m_Listing->Count() : 0;

   kbStringList::iterator i;
   // add the items to the checklistbox
   for ( size_t n = 0; n < count; n++ )
   {
      m_checklistBox->Append((*m_Listing)[n].GetName());
      i = FindInList((*m_Listing)[n].GetName());
      if(i != m_Modules.end())
         m_checklistBox->Check(n, TRUE);
   }
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
   bool selected = m_checklistBox->IsChecked(n);
   kbStringList::iterator i =
      FindInList(m_checklistBox->GetString(n)); 

   if(selected && i == m_Modules.end())
      m_Modules.push_back(new wxString(m_checklistBox->GetString(n)));
   if(! selected && i != m_Modules.end())
      m_Modules.erase(i);
   return TRUE;
}
   


bool wxModulesDialog::TransferDataFromWindow()
{
   wxString setting;
   size_t count = (size_t)m_checklistBox->Number();
   for ( size_t n = 0; n < count; n++ )
   {
      setting << m_checklistBox->GetString(n);
      if(n != count-1) setting << ':';
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
