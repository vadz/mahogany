/*-*- c++ -*-********************************************************
 * wxMFilterDialog.cpp : dialog for setting up filter rules         *
 *                                                                  *
 * (C) 1998-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "Mdefaults.h"
#   include "guidef.h"
#   include "strutil.h"
#   include "MFrame.h"
#   include "MDialogs.h"
#   include "Profile.h"
#   include "MApplication.h"
#   include "MailFolder.h"
#   include "Profile.h"
#   include "MModule.h"
#   include "MHelp.h"
#endif

#include "MHelp.h"
#include "gui/wxMIds.h"

#include <wx/window.h>
#include <wx/confbase.h>
#include <wx/persctrl.h>
#include <wx/stattext.h>
#include <wx/layout.h>
#include <wx/checklst.h>
#include <wx/statbox.h>
#include <wx/sizer.h>

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// global vars and functions
// ----------------------------------------------------------------------------

/* Calculates a common button size for buttons with labels when given 
   an array with their labels. */
wxSize GetButtonSize(const char * labels[], wxWindow *win)
{
   long widthLabel, heightLabel;
   long heightBtn, widthBtn;
   long maxWidth = 0, maxHeight = 0;
   
   wxClientDC dc(win);
   dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
   for(size_t idx = 0; labels[idx]; idx++)
   {
      dc.GetTextExtent(_(labels[idx]), &widthLabel, &heightLabel);
      heightBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel);
      widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);
      if(widthBtn > maxWidth) maxWidth = widthBtn;
      if(heightBtn > maxHeight) maxHeight = heightBtn;
   }
   return wxSize(maxWidth, maxHeight);
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------


/** A class representing the configuration GUI for a single filter. */
class wxOneFilterDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor & dtor
   wxOneFilterDialog(const wxString & filterName,
                     wxWindow *parent);
   virtual ~wxOneFilterDialog() { }

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool WasChanged(void) { return m_Filter != m_OldFilter;}

   // event handlers
   void OnUpdate(wxCommandEvent& event) { }
protected:
   // data
   wxTextCtrl *m_NameCtrl;
   wxString m_Name;
   wxString  m_Filter,
             m_OldFilter;
private:
   DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(wxOneFilterDialog, wxManuallyLaidOutDialog)
END_EVENT_TABLE()



static
bool ConfigureOneFilter(const wxString &name,
                        wxWindow *parent)
{
   wxOneFilterDialog dlg(name, parent);
   return ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() );
}



class OneCritControls
{
public:
   OneCritControls(wxWindow *parent, wxLayoutConstraints *c);
   void Save(wxString *str);
   void Load(wxString *str);
private:
   wxCheckBox *m_Not; // Not
   wxChoice   *m_Type; // "Match", "Match substring", "Match RegExp",
   // "Greater than", "Smaller than", "Older than"; "Newer than"
   wxTextCtrl  *m_Argument; // string, number of days or bytes
   wxChoice   *m_Where; // "In Header", "In Subject", "In From..."
   wxWindow   *m_Parent;
};

static
wxString ORC_Types[] =
{ gettext_noop("Match"), gettext_noop("Match substring"),
  gettext_noop("Match RegExp"), gettext_noop("Larger than"),
  gettext_noop("Smaller than"), gettext_noop("Older than"),
  gettext_noop("Is SPAM")
};

static
size_t ORC_TypesCount = WXSIZEOF(ORC_Types);

static
wxString ORC_Where[] =
{
   gettext_noop("in Subject"),
   gettext_noop("in header"),
   gettext_noop("in From"),
   gettext_noop("in body"),
   gettext_noop("in message"),
   gettext_noop("in To")
};
static
size_t ORC_WhereCount = WXSIZEOF(ORC_Where);

#define CRIT_CTRLCONST(oldctrl,newctrl) \
   c = new wxLayoutConstraints;\
   c->left.RightOf(oldctrl, LAYOUT_X_MARGIN);\
   c->width.PercentOf(m_Parent, wxWidth, 25);\
   c->top.SameAs(m_Not, wxTop, 0);\
   c->height.AsIs();\
   newctrl->SetConstraints(c);


OneCritControls::OneCritControls(wxWindow *parent, wxLayoutConstraints *c)
{
   m_Parent = parent;
   m_Not = new wxCheckBox(m_Parent, -1, _("Not"));
   m_Not->SetConstraints(c);

   m_Type = new wxChoice(m_Parent, -1, wxDefaultPosition,
                         wxDefaultSize, ORC_TypesCount, ORC_Types);
   CRIT_CTRLCONST(m_Not, m_Type);
   
   m_Argument = new wxTextCtrl(m_Parent,-1,"", wxDefaultPosition);
   CRIT_CTRLCONST(m_Type, m_Argument);
   
   m_Where = new wxChoice(m_Parent, -1, wxDefaultPosition,
                          wxDefaultSize, ORC_WhereCount, ORC_Where);
   CRIT_CTRLCONST(m_Argument, m_Where);
}

void
OneCritControls::Save(wxString *str)
{
   str->Printf("%d %d \"%s\" %d",
              (int)m_Not->GetValue(),
              m_Type->GetSelection(),
              m_Argument->GetValue().c_str(),
              m_Where->GetSelection());
}

void
OneCritControls::Load(wxString *str)
{
}
class OneActionControls
{
public:
   OneActionControls(wxWindow *parent, wxLayoutConstraints *c);
   
private:
   /// Which action to perform
   wxChoice   *m_Type;
   wxTextCtrl  *m_Argument; // string, number of days or bytes
   wxChoice   *m_Where; // "In Header", "In Subject", "In From..."
   wxWindow   *m_Parent;
};

static
wxString OAC_Types[] =
{ gettext_noop("Delete"),
  gettext_noop("Copy to"),
  gettext_noop("Move to"),
  gettext_noop("Expunge"),
  gettext_noop("MessageBox"),
  gettext_noop("Log Entry"),
};
static
size_t OAC_TypesCount = WXSIZEOF(OAC_Types);


OneActionControls::OneActionControls(wxWindow *parent, wxLayoutConstraints *c)
{
   m_Parent = parent;

   m_Type = new wxChoice(m_Parent, -1, wxDefaultPosition,
                         wxDefaultSize, OAC_TypesCount, OAC_Types);
   c->width.PercentOf(m_Parent, wxWidth, 30);
   m_Type->SetConstraints(c);

   m_Argument = new wxTextCtrl(m_Parent,-1,"", wxDefaultPosition);
   c = new wxLayoutConstraints;
   c->left.RightOf(m_Type, LAYOUT_X_MARGIN);
   c->width.PercentOf(m_Parent, wxWidth, 60);
   c->top.SameAs(m_Type, wxTop, 0);
   c->height.AsIs();
   m_Argument->SetConstraints(c);
}

/*
  Contains several lines of "OneCritControls" and "OneActionControls":

  +--------------------------------+
  |                                |
  | txt                            |
  |                                |
  | +----------------------------+ |
  | |  filter patterns           | |
  | +----------------------------+ |
  |                                |
  |                                |
  | +----------------------------+ |
  | |  action rules              | |
  | +----------------------------+ |
  |                                |
  |                                |
  |                                |
  |                                |
  +--------------------------------+

*/

wxOneFilterDialog::wxOneFilterDialog(const wxString &filterName,
                                     wxWindow *parent)
   : wxManuallyLaidOutDialog(parent,
                            _("Filter rule"),
                            "OneFilterDialog")
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("Filter Rule"), FALSE,
                                             MH_DIALOG_FILTERS);

   m_Name = filterName;

   SetDefaultSize(380, 240, FALSE /* not minimal */);

   SetAutoLayout( TRUE );
   wxLayoutConstraints *c;

   m_NameCtrl = new wxTextCtrl(this, -1);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_NameCtrl->SetConstraints(c);

   wxStaticBox *criteria = new wxStaticBox(this, -1, _("Filter Criteria"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_NameCtrl, 2*LAYOUT_Y_MARGIN);
   c->height.PercentOf(box, wxHeight, 45);
   criteria->SetConstraints(c);

/*
  Add a scrolled window with
   [test to apply] [where to apply] [match what]
   [logical operator] [test] [where] [what]
   ...T
   [more rules] [less rules]

   E.g.
   [Match substring] [Subject] ["Mahogany"]
   [ AND ] [Match substring] [Subject] ["bug"]

   ["More Tests"] [ "Less Tests" ]
*/
   wxStaticBox *actions = new wxStaticBox(this, -1, _("Action Rules"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(criteria, 2*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 4*LAYOUT_Y_MARGIN);
   actions->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(criteria, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(criteria, wxTop, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   OneCritControls * orc = new OneCritControls(this, c);

   c = new wxLayoutConstraints;
   c->left.SameAs(actions, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(actions, wxTop, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   OneActionControls * oac = new OneActionControls(this, c);
   
   TransferDataToWindow();
   m_OldFilter = m_Filter;
}

bool
wxOneFilterDialog::TransferDataFromWindow()
{
#if 0
   m_Filter = m_textctrl->GetValue();
   GetProfile()->writeEntry(MP_DATE_FMT, m_Filter);
   GetProfile()->writeEntry(MP_DATE_GMT, m_UseGMT->GetValue());
#endif
   
   return TRUE;
}

bool
wxOneFilterDialog::TransferDataToWindow()
{
   m_NameCtrl->SetValue(m_Name);
#if 0
   m_Filter = READ_CONFIG(GetProfile(), MP_DATE_FMT);
   m_UseGMT->SetValue( READ_CONFIG(GetProfile(), MP_DATE_GMT) != 0);
   m_textctrl->SetValue(m_Filter);
#endif
   
   return TRUE;
}







static const char *ButtonLabels [] =
{ gettext_noop("New"), gettext_noop("Delete"), gettext_noop("Edit"),
  gettext_noop("Up"), gettext_noop("Down"), NULL };

enum ButtonIndices
{
   Button_New = 0,
   Button_Delete,
   Button_Edit,
   Button_Up,
   Button_Down
};

class wxFiltersDialog : public wxOptionsPageSubdialog
{
public:
   // ctor & dtor
   wxFiltersDialog(ProfileBase *profile, wxWindow *parent);
   virtual ~wxFiltersDialog() {}

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool WasChanged(void) { return m_Filter != m_OldFilter;}

   // event handlers
   void Update(wxCommandEvent& event) { }
   void OnButton(wxCommandEvent & event );
protected:

   wxCheckListBox *m_ChecklistBox;
   wxButton       *m_Buttons[WXSIZEOF(ButtonLabels)];
   
// data
   wxString  m_Filter,
m_OldFilter;
private:
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxFiltersDialog, wxOptionsPageSubdialog)
   EVT_LISTBOX(-1, wxFiltersDialog::Update)
   EVT_BUTTON(-1,  wxFiltersDialog::OnButton)
END_EVENT_TABLE()


   
wxFiltersDialog::wxFiltersDialog(ProfileBase *profile, wxWindow *parent)
   : wxOptionsPageSubdialog(profile,
                            parent,
                            _("DO NOT USE - CODE UNDER DEVELOPMENT!!! Filter rules"), //FIXME
                            "FilterDialog")
{
   wxLayoutConstraints *c;
   
   wxStaticBox *box = CreateStdButtonsAndBox(_("Filter Rules"), FALSE,
                                             MH_DIALOG_FILTERS);

   /* This dialog is supposed to look like this:

      +------------------+  [new]
      |checklistbox of   |  [del]
      |existing filters  |  [edit]
      |                  |  
      |                  |  [up]
      +------------------+  [down]

      [help][ok][cancel]
   */

   wxSize ButtonSize = ::GetButtonSize(ButtonLabels, this);
   for(size_t idx = 0; ButtonLabels[idx]; idx++)
   {
      c = new wxLayoutConstraints;
      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      c->width.Absolute(ButtonSize.GetX());
      if(idx == 0)
         c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
      else
      {
         if(idx == 3) // the up button, add more space
            c->top.Below(m_Buttons[idx-1],
                         ButtonSize.GetY()+4*LAYOUT_Y_MARGIN);
         else            
            c->top.Below(m_Buttons[idx-1], 2*LAYOUT_Y_MARGIN);
      }
      c->height.AsIs();
      m_Buttons[idx] = new wxButton(this, -1, _(ButtonLabels[idx]),
                                    wxDefaultPosition, ButtonSize);
      m_Buttons[idx]->SetConstraints(c);
   }

   // create the checklistbox in the area which is left
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(m_Buttons[0], wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_ChecklistBox = new wxCheckListBox(this, -1);
   m_ChecklistBox->SetConstraints(c);

   SetDefaultSize(380, 240, FALSE /* not minimal */);
   TransferDataToWindow();
   m_OldFilter = m_Filter;
}

void
wxFiltersDialog::OnButton( wxCommandEvent &event )
{
   wxObject *obj = event.GetEventObject();
   
   for(size_t idx = 0; ButtonLabels[idx]; idx++)
      if(obj == m_Buttons[idx])
      {
         if(idx == Button_New)
            ConfigureOneFilter(_("New Filter"), this);
         else if(idx == Button_Edit)
            ConfigureOneFilter("XXX", this);
         else
            return;
      }
   event.Skip();
}

bool
wxFiltersDialog::TransferDataFromWindow()
{
#if 0
   m_Filter = m_textctrl->GetValue();
   GetProfile()->writeEntry(MP_DATE_FMT, m_Filter);
   GetProfile()->writeEntry(MP_DATE_GMT, m_UseGMT->GetValue());
#endif
   
   return TRUE;
}

bool
wxFiltersDialog::TransferDataToWindow()
{
#if 0
   m_Filter = READ_CONFIG(GetProfile(), MP_DATE_FMT);
   m_UseGMT->SetValue( READ_CONFIG(GetProfile(), MP_DATE_GMT) != 0);
   m_textctrl->SetValue(m_Filter);
#endif
   
   return TRUE;
}

// ----------------------------------------------------------------------------
// exported function
// ----------------------------------------------------------------------------
/** Set up filters rules. In wxMFilterDialog.cpp */
extern
bool ConfigureFilterRules(ProfileBase *profile, wxWindow *parent)
{
   wxFiltersDialog dlg(profile, parent);
   return ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() );
}

