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
   wxOneFilterDialog(ProfileBase *profile,
                     wxWindow *parent);
   virtual ~wxOneFilterDialog()
      {
         SafeDecRef(m_Profile);
      }

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool WasChanged(void) { return m_Filter != m_OldFilter;}

   // event handlers
   void OnUpdate(wxCommandEvent& event) { }

   ProfileBase * GetProfile() const { return m_Profile; }
protected:
   ProfileBase *m_Profile;
   // data
   wxTextCtrl *m_NameCtrl;
   wxString  m_Name;
   wxString  m_Filter,
             m_OldFilter;
   class OneCritControl *m_CritControl;
   class OneActionControl *m_ActionControl;
private:
   DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(wxOneFilterDialog, wxManuallyLaidOutDialog)
END_EVENT_TABLE()



static
bool ConfigureOneFilter(ProfileBase *profile,
                        wxWindow *parent)
{
   wxOneFilterDialog dlg(profile, parent);
   return ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() );
}



class OneCritControl
{
public:
   OneCritControl(wxWindow *parent, wxLayoutConstraints *c);
   void Save(wxString *str);
   void Load(wxString *str);

   /// translate a storage string into a proper rule
   static wxString TranslateToString(const wxString & criterium);

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

enum ORC_Types_Enum
{
   ORC_T_Match = 0,
   ORC_T_MatchSub,
   ORC_T_MatchRegEx,
   ORC_T_LargerThan,
   ORC_T_SmallerThan,
   ORC_T_OlderThan,
   ORC_T_IsSpam
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

enum ORC_Where_Enum
{
   ORC_W_Subject = 0,
   ORC_W_Header,
   ORC_W_From,
   ORC_W_Body,
   ORC_W_Message,
   ORC_W_To
};

#define CRIT_CTRLCONST(oldctrl,newctrl) \
   c = new wxLayoutConstraints;\
   c->left.RightOf(oldctrl, LAYOUT_X_MARGIN);\
   c->width.PercentOf(m_Parent, wxWidth, 25);\
   c->top.SameAs(m_Not, wxTop, 0);\
   c->height.AsIs();\
   newctrl->SetConstraints(c);


OneCritControl::OneCritControl(wxWindow *parent, wxLayoutConstraints *c)
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
OneCritControl::Save(wxString *str)
{
   str->Printf("%d %d \"%s\" %d",
              (int)m_Not->GetValue(),
              m_Type->GetSelection(),
              strutil_escapeString(m_Argument->GetValue()).c_str(),
              m_Where->GetSelection());
}

void
OneCritControl::Load(wxString *str)
{
   long number = strutil_readNumber(*str);
   m_Not->SetValue(number != 0);
   number = strutil_readNumber(*str);
   m_Type->SetSelection(number);
   m_Argument->SetValue(strutil_readString(*str));
   number = strutil_readNumber(*str);
   m_Where->SetSelection(number);
}

/* static */
wxString
OneCritControl::TranslateToString(const wxString & criterium)
{
   String str = criterium;
   String program;
   // This returns the bit to go into an if between the brackets:
   // if ( .............. )
   if(strutil_readNumber(str) != 0)
      program << '!';
   
   long type = strutil_readNumber(str);
   wxString argument = strutil_readString(str);
   long where = strutil_readNumber(str);
   bool needsWhere = true;
   switch(type)
   {
   case ORC_T_Match:
      program << "matchi(" << '"' << argument << "\","; break;
   case ORC_T_MatchSub:
   case ORC_T_MatchRegEx:
   case ORC_T_LargerThan:
   case ORC_T_SmallerThan:
   case ORC_T_OlderThan:
      wxASSERT_MSG(0,"not yet implemented");
      break;
   case ORC_T_IsSpam:
      program << "isspam(";
      needsWhere = false;
      break;
   default:
      wxASSERT_MSG(0,"unknown rule"); // FIXME: give meaningful error message
   }
   if(needsWhere)
   {
      switch(where)
      {
      case ORC_W_Subject:
         program << "subject()"; break;
      case ORC_W_Header:
         program << "header()"; break;
      case ORC_W_From:
         program << "from()"; break;
      case ORC_W_Body:
         program << "body()"; break;
      case ORC_W_Message:
         program << "message()"; break;
      case ORC_W_To:
         program << "to()"; break;
      };
   }
   program << ')'; // end of function call
   return program;
}


class OneActionControl
{
public:
   OneActionControl(wxWindow *parent, wxLayoutConstraints *c);
   
   void Save(wxString *str);
   void Load(wxString *str);

   /// translate a storage string into a proper rule
   static wxString TranslateToString(const wxString & action);
private:
   /// Which action to perform
   wxChoice   *m_Type;
   wxTextCtrl  *m_Argument; // string, number of days or bytes
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

enum OAC_Types_Enum
{
   OAC_T_Delete = 0,
   OAC_T_CopyTo,
   OAC_T_MoveTo,
   OAC_T_Expunge,
   OAC_T_MessageBox,
   OAC_T_LogEntry
};

void
OneActionControl::Save(wxString *str)
{
   str->Printf("%d \"%s\"",
              m_Type->GetSelection(),
              strutil_escapeString(m_Argument->GetValue()).c_str());
}

void
OneActionControl::Load(wxString *str)
{
   m_Type->SetSelection(strutil_readNumber(*str));
   m_Argument->SetValue(strutil_readString(*str));
}


/* static */
wxString
OneActionControl::TranslateToString(const wxString & action)
{
   wxString str = action;
   long type = strutil_readNumber(str);
   wxString argument = strutil_readString(str);
   wxString program;

   bool needsArgument = true;
   switch(type)
   {
   case OAC_T_Delete:
      program << "delete("; needsArgument = false; break;
   case OAC_T_CopyTo:
      program << "copy("; break;
   case OAC_T_MoveTo:
      program << "move("; break;
   case OAC_T_Expunge:
      program << "expunge("; needsArgument = false; break;
   case OAC_T_MessageBox:
      program << "message("; break;
   case OAC_T_LogEntry:
      program << "log("; break;
   }
   if(needsArgument)
      program << '"' << argument << '"';
   program << ");";
   return program;
}



OneActionControl::OneActionControl(wxWindow *parent, wxLayoutConstraints *c)
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
  Contains several lines of "OneCritControl" and "OneActionControl":

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

wxOneFilterDialog::wxOneFilterDialog(ProfileBase *profile,
                                     wxWindow *parent)
   : wxManuallyLaidOutDialog(parent,
                            _("Filter rule"),
                            "OneFilterDialog")
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("Filter Rule"), FALSE,
                                             MH_DIALOG_FILTERS);

   m_Profile = profile;
   m_Profile->IncRef();
   m_Name = m_Profile->readEntry("Name","");
   
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
   m_CritControl = new OneCritControl(this, c);

   c = new wxLayoutConstraints;
   c->left.SameAs(actions, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(actions, wxTop, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_ActionControl = new OneActionControl(this, c);
   
   TransferDataToWindow();
   m_OldFilter = m_Filter;
}

bool
wxOneFilterDialog::TransferDataFromWindow()
{
   GetProfile()->writeEntry("Name", m_NameCtrl->GetValue().c_str());

   String str;
   m_CritControl->Save(&str); GetProfile()->writeEntry("Criterium", str);
   m_ActionControl->Save(&str); GetProfile()->writeEntry("Action", str);
   return TRUE;
}

bool
wxOneFilterDialog::TransferDataToWindow()
{
   m_NameCtrl->SetValue(GetProfile()->readEntry("Name", _("unnamed")));

   String str;
   str = GetProfile()->readEntry("Criterium", ""); m_CritControl->Load(&str);
   str = GetProfile()->readEntry("Action", ""); m_ActionControl->Load(&str);
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
   virtual ~wxFiltersDialog()
      {
         m_FiltersProfile->DecRef();
      }

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool WasChanged(void) { return m_Filter != m_OldFilter;}

   // event handlers
   void Update(wxCommandEvent& event) { }
   void OnButton(wxCommandEvent & event );
protected:

   wxListBox *m_ListBox;
   wxButton       *m_Buttons[WXSIZEOF(ButtonLabels)];
   
// data
   wxString  m_Filter,
      m_OldFilter;
   /// an array holding the names of all filters:
   wxArrayString m_FilterGroups, m_FilterNames;
   ProfileBase *m_FiltersProfile;
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
   m_FiltersProfile = ProfileBase::CreateProfile("Filters", profile);
   
   wxLayoutConstraints *c;
   wxStaticBox *box = CreateStdButtonsAndBox(_("Filter Rules"), FALSE,
                                             MH_DIALOG_FILTERS);

   /* This dialog is supposed to look like this:

      +------------------+  [new]
      |listbox of        |  [del]
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

   // create the listbox in the area which is left
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(m_Buttons[0], wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_ListBox = new wxListBox(this, -1);
   m_ListBox->SetConstraints(c);

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
         // Either edit an existing folder or create a new one:
         wxString name;
         if(idx == Button_New)
            name = m_FiltersProfile->GetUniqueGroupName();
         else if(idx == Button_Edit)
         {
            int idx = m_ListBox->GetSelection();
            if(idx >= 0)
               name = m_FilterGroups[idx];
            else
               return; //FIXME: disable button if none is selected
         }
         else
            return;
         // now we have a valid name:
         ProfileBase *fprof = ProfileBase::CreateProfile(name, m_FiltersProfile);
         ConfigureOneFilter(fprof, this);
         fprof->DecRef();
         TransferDataToWindow(); // update the view
      }
   event.Skip();
}

bool
wxFiltersDialog::TransferDataFromWindow()
{
   return TRUE;
}

bool
wxFiltersDialog::TransferDataToWindow()
{
   /* Filters are all stored as subgroups under the group "Filters" in 
      the current profile. I.e. ./Filters/a ./Filters/b etc
      Fitler names and group names do not correspond, group names are
      just created to be unique.
   */

   m_FilterGroups.Clear();
   m_FilterNames.Clear();
   m_ListBox->Clear();
   
   long ref;
   wxString name;
   for(bool found = m_FiltersProfile->GetFirstGroup(name, ref);
       found;
       found = m_FiltersProfile->GetNextGroup(name, ref))
   {
      m_FilterGroups.Add(name);
      name = m_FiltersProfile->readEntry(name+"/Name",_("unnamed"));
      m_FilterNames.Add(name);
      m_ListBox->Append(name);
   }
   return TRUE;
}

static
String TranslateOneRuleToProgram(const wxString &name,
                                 ProfileBase *profile)
{
   wxString program;
   ProfileBase *p = ProfileBase::CreateProfile(name, profile);
   wxString criterium = p->readEntry("Criterium","");
   wxString action = p->readEntry("Action","");
   if(criterium.Length() > 0 && action.Length() > 0)
   {
      program = "if(";
      program << OneCritControl::TranslateToString(criterium)
              << ')'
              << '{'
              << OneActionControl::TranslateToString(action)
              << '}';
      
   }
   p->DecRef();
   return program;
}

// ----------------------------------------------------------------------------
// exported function
// ----------------------------------------------------------------------------
/** Set up filters rules. In wxMFilterDialog.cpp */
extern
bool ConfigureFilterRules(ProfileBase *profile, wxWindow *parent)
{
   wxFiltersDialog dlg(profile, parent);
   bool dirty = ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() );

   //FIXME: for debugging purposes only:

   wxString pgm = GetFilterProgram(profile);
   INFOMESSAGE(("Resulting filter program:\n%s", pgm.c_str()));
   
   return dirty;
}

/** Read a filter program from the data.
 This takes the sub-groups under "Filters/" and assembles a filter
 program from them. */
extern
String GetFilterProgram(ProfileBase *profile)
{
   ProfileBase * filtersProfile =
      ProfileBase::CreateProfile("Filters", profile);

   String program;
   
   long ref;
   wxString name;
   for(bool found = filtersProfile->GetFirstGroup(name, ref);
       found;
       found = filtersProfile->GetNextGroup(name, ref))
   {
      program += TranslateOneRuleToProgram(name, filtersProfile);
   }
   filtersProfile->DecRef();
   return program;
}
