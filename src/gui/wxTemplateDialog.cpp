///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   TemplateDialog.cpp - the template configuration dialog
// Purpose:     these dialogs are used mainly by the options dialog, but may be
//              also used from elsewhere
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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

#  include <wx/combobox.h>
#  include <wx/layout.h>
#  include <wx/statbox.h>
#  include <wx/stattext.h>
#  include <wx/statbmp.h>
#  include <wx/textctrl.h>
#endif

#include "Mdefaults.h"

#include "MessageTemplate.h"

#include "MDialogs.h"
#include "gui/wxDialogLayout.h"

#include "TemplateDialog.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the text window for editing templates: it has a popup menu which allows the
// user to insert any ob existing macors
class TemplateEditor : public wxTextCtrl
{
public:
   // ctor
   TemplateEditor(wxWindow *parent) : wxTextCtrl(parent, -1,
                                                 "",
                                                 wxDefaultPosition,
                                                 wxDefaultSize,
                                                 wxTE_MULTILINE)
   {
      m_menu = NULL;
   }

   virtual ~TemplateEditor() { if ( m_menu ) delete m_menu; }

   // callbacks
   void OnRClick(wxMouseEvent& event); // show the popup menu
   void OnMenu(wxCommandEvent& event); // insert the selection into text

private:
   // creates the popup menu if it doesn't exist yet
   void CreatePopupMenu();

   // the popup menu
   wxMenu *m_menu;

   DECLARE_EVENT_TABLE()
};

// the main dialog for editing templates: it contains a listbox for choosing
// the template and the text control to edit the selected template
class wxTemplateDialog : public wxOptionsPageSubdialog
{
public:
   wxTemplateDialog(ProfileBase *profile, wxWindow *parent);

   // did the user really change anything?
   bool WasChanged() const { return m_wasChanged; }

   // called by wxWindows when [Ok] button was pressed
   virtual bool TransferDataFromWindow();

   // callbacks
   void OnListboxSelection(wxCommandEvent& event);

private:
   // saves changes to current template
   void SaveChanges();

   ProfileBase        *m_profile;
   MessageTemplateKind m_kind;         // of template being edited
   wxTextCtrl         *m_textctrl;
   bool                m_wasChanged;

   static const char *ms_templateNames[MessageTemplate_Max];

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(TemplateEditor, wxTextCtrl)
   EVT_MENU(-1, TemplateEditor::OnMenu)
   EVT_RIGHT_DOWN(TemplateEditor::OnRClick)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxTemplateDialog, wxOptionsPageSubdialog)
   EVT_LISTBOX(-1, wxTemplateDialog::OnListboxSelection)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// TemplateEditor
// ----------------------------------------------------------------------------

void TemplateEditor::OnRClick(wxMouseEvent& event)
{
}

void TemplateEditor::OnMenu(wxCommandEvent& event)
{
}

// ----------------------------------------------------------------------------
// wxTemplateDialog
// ----------------------------------------------------------------------------

const char *wxTemplateDialog::ms_templateNames[] =
{
   gettext_noop("New message"),
   gettext_noop("New article"),
   gettext_noop("Reply"),
   gettext_noop("Follow-up"),
   gettext_noop("Forward")
};

wxTemplateDialog::wxTemplateDialog(ProfileBase *profile, wxWindow *parent)
                : wxOptionsPageSubdialog(profile, parent,
                                         _("Configure message templates"),
                                         "ComposeTemplates")
{
   // init members
   // ------------
   m_kind = MessageTemplate_Max;
   m_profile = profile;
   m_wasChanged = FALSE;

   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // first the box around everything
   wxStaticBox *box = CreateStdButtonsAndBox("");

   // then a short help message
   wxStaticText *msg = new wxStaticText
                           (
                            this, -1,
                            _("Please select the template to edit in the list "
                              "first. Then right click the mouse in the text "
                              "control to get the list of all available "
                              "macros")
                           );
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   // on the left side is the listbox with all available templates
   wxListBox *listbox = new wxListBox(this, -1);

   // this array should be in sync with MessageTemplateKind enum
   ASSERT_MSG( WXSIZEOF(ms_templateNames) == MessageTemplate_Max,
               "forgot to update the labels array?" );
   for ( size_t n = 0; n < WXSIZEOF(ms_templateNames); n++ )
   {
      listbox->Append(_(ms_templateNames[n]));
   }

   c = new wxLayoutConstraints;
   c->top.Below(msg, LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.Absolute(5*hBtn);
   listbox->SetConstraints(c);

   // to the right of it is the text control where template file can be edited
   m_textctrl = new TemplateEditor(this);
   c = new wxLayoutConstraints;
   c->top.SameAs(listbox, wxTop);
   c->height.SameAs(listbox, wxHeight);
   c->left.RightOf(listbox, LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   m_textctrl->SetConstraints(c);

   SetDefaultSize(6*wBtn, 10*hBtn);
}

void wxTemplateDialog::SaveChanges()
{
   m_wasChanged = TRUE;

   SetMessageTemplate(m_textctrl->GetValue(), m_kind, m_profile);
}

void wxTemplateDialog::OnListboxSelection(wxCommandEvent& event)
{
   if ( m_textctrl->IsModified() )
   {
      // save it if the user doesn't veto it
      String msg;
      msg.Printf(_("You have modified the template for message of type "
                   "'%s', would you like to save it?"),
                 ms_templateNames[m_kind]);
      if ( MDialog_YesNoDialog(msg, this,
                               MDIALOG_YESNOTITLE, true, "SaveTemplate") )
      {
         SaveChanges();
      }
   }

   // load the template for the selected type into the text ctrl
   m_kind = (MessageTemplateKind)event.GetInt();
   String templateValue = GetMessageTemplate(m_kind, m_profile);

   m_textctrl->SetValue(templateValue);
   m_textctrl->DiscardEdits();
}

bool wxTemplateDialog::TransferDataFromWindow()
{
   if ( m_textctrl->IsModified() )
   {
      // don't ask - if the user pressed ok, he does want to save changes,
      // otherwise he would have chosen cancel
      SaveChanges();
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

bool ConfigureTemplates(ProfileBase *profile, wxWindow *parent)
{
   wxTemplateDialog dlg(profile, parent);
   if ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() )
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

