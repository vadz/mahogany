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
#  include <wx/textctrl.h>
#endif

#include "Mdefaults.h"

#include "gui/wxDialogLayout.h"

#include "TemplateDialog.h"

// VZ: I'm working on this
#if 0

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the main dialog for editing templates: it contains a listbox for choosing
// the template and the text control to edit the selected template
class wxTemplateDialog : public wxOptionsPageSubdialog
{
public:
   wxTemplateDialog(ProfileBase *profile, wxWindow *parent);

private:
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxTemplateDialog, wxOptionsPageSubdialog)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

wxTemplateDialog::wxTemplateDialog(ProfileBase *profile, wxWindow *parent)
                : wxOptionsPageSubdialog(profile, parent,
                                         _("Configure message templates"),
                                         "ComposeTemplates")
{
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
                              "first. Then, right click the mouse in the text "
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
   wxListBox *listbox = new wxListBox(this, ListBox_Templates);
   static const char *templateNames[] =
   {
      gettext_noop("New message"),
      gettext_noop("New article"),
      gettext_noop("Reply"),
      gettext_noop("Forward")
   };
   for ( size_t n = 0; n < WXSIZEOF(templateNames); n++ )
   {
      listbox->Append(_(templateNames[n]));
   }

   c = new wxLayoutConstraints;
   c->top.Below(msg);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.Absolute(5*hBtn);
   listbox->SetConstraints(c);

   // to the right of it is the text control where template file can be edited
   m_textctrl = new wxTextCtrl(this, -1, "",
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE);
   c = new wxLayoutConstraints;
   c->top.SameAs(listbox, wxTop);
   c->height.SameAs(listbox, wxHeight);
   c->left.RightOf(listbox, LAYOUT_X_MARGIN);
   c->right.LeftOf(btnDelete, LAYOUT_X_MARGIN);
   m_textctrl->SetConstraints(c);
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

#endif // 0

bool ConfigureTemplates(ProfileBase *profile, wxWindow *parent)
{
#if 0
   wxTemplateDialog dlg(profile, parent);
   if ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() )
   {
      return TRUE;
   }
   else
#endif // 0
   {
      return FALSE;
   }
}

