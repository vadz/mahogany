/*-*- c++ -*-********************************************************
 * wxMFilterDialog.cpp : dialog for setting up filter rules         *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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
#   include "strutil.h"
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


class FilterEntryData
{
public:
   inline void SetName(const String &name) { m_Name = name; }
   inline void SetCriterium(const String &str) { m_Criterium = str; }
   inline void SetAction(const String &str) { m_Action = str; }
   inline void SetActive(bool a) { m_Active = a; }
   
   inline const String & GetName() const
      { return m_Name; }
   inline const String & GetCriterium() const
      { return m_Criterium; }
   inline const String & GetAction() const
      { return m_Action; }
   inline bool GetActive(void) const
      { return m_Active; }
   
   FilterEntryData(ProfileBase *profile)
      {
         SetName( profile->readEntry("Name",_("unnamed")) );
         SetCriterium( profile->readEntry("Criterium","") );
         SetAction( profile->readEntry("Action","") );
         SetActive( profile->readEntry("Active", 1L) != 0 );
      }

   FilterEntryData()
      {
         m_Active = true;
      }

   bool operator==(const FilterEntryData b)
      {
         return m_Name == b.m_Name
            && m_Criterium == b.m_Criterium
            && m_Action == b.m_Action
            && m_Active == b.m_Active;
      }
#if 0 // use default constructor instead
   FilterEntryData & operator=(const FilterEntryData &in)
      {
         SetName( in.GetName() );
         SetCriterium( in.GetCriterium() );
         SetAction( in.GetAction() );
         SetActive( in.GetActive() );
         return *this;
      }
#endif
private:
   String m_Name;
   String m_Criterium;
   String m_Action;
   bool   m_Active;
};

// ----------------------------------------------------------------------------
// wxOneFilterDialog - dialog for exactly one filter rule
// ----------------------------------------------------------------------------

#define MAX_CONTROLS   256

/** A class representing the configuration GUI for a single filter. */
class wxOneFilterDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor & dtor
   wxOneFilterDialog(class FilterEntryData *fed,
                     wxWindow *parent);

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool WasChanged(void) { return ! (*m_FilterData == m_OriginalFilterData);}

   // event handlers
   void OnUpdateUI(wxUpdateUIEvent& event);
   void OnCancel(wxCommandEvent& event);
   void OnButton(wxCommandEvent& event);
protected:
   void AddOneControl();
   void RemoveOneControl();

   void LayoutControls(); 
   
   wxButton *m_ButtonLess, *m_ButtonMore;
   FilterEntryData *m_FilterData;
   FilterEntryData m_OriginalFilterData;
   // data
   wxTextCtrl *m_NameCtrl;
   wxStaticText *m_IfMessage, *m_DoThis;
   wxString  m_Name;
   size_t   m_nControls;
   class OneCritControl *m_CritControl[MAX_CONTROLS];
   class OneActionControl *m_ActionControl;
   wxEnhancedPanel *m_Panel;
private:
   DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(wxOneFilterDialog, wxManuallyLaidOutDialog)
   EVT_BUTTON(-1, wxOneFilterDialog::OnButton)
   EVT_UPDATE_UI(-1, wxOneFilterDialog::OnUpdateUI)
END_EVENT_TABLE()



   
static
bool ConfigureOneFilter(class FilterEntryData *fed,
                        wxWindow *parent)
{
   wxOneFilterDialog dlg(fed, parent);
   return ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() );
}



class OneCritControl
{
public:
   OneCritControl(wxWindow *parent, 
                  bool followup = FALSE);
   ~OneCritControl();
   void Save(wxString *str);
   void Load(wxString *str);

   /// translate a storage string into a proper rule
   static wxString TranslateToString(wxString & criterium);
   void UpdateUI();

   /// layout the current controls under the window *last
   void LayoutControls(wxWindow **last);
   wxWindow *GetLastCtrl() { return m_Where; }

private:
   wxCheckBox *m_Not; // Not
   wxChoice   *m_Type; // "Match", "Match substring", "Match RegExp",
   // "Greater than", "Smaller than", "Older than"; "Newer than"
   wxTextCtrl  *m_Argument; // string, number of days or bytes
   wxChoice   *m_Where; // "In Header", "In Subject", "In From..."
   wxWindow   *m_Parent;
   wxChoice   *m_Logical;
};


static
wxString ORC_Types[] =
{ gettext_noop("Always"),
  gettext_noop("Match"), gettext_noop("Contains"),
  gettext_noop("Match Case"), gettext_noop("Contains Case"),
  gettext_noop("Match RegExp"),
  gettext_noop("Larger than"), gettext_noop("Smaller than"),
  gettext_noop("Older than"), gettext_noop("Newer than"),
  gettext_noop("Is SPAM")
#ifdef USE_PYTHON
  ,gettext_noop("Python")
#endif
};

enum ORC_Types_Enum
{
   ORC_T_Always = 0,
   ORC_T_Match,
   ORC_T_Contains,
   ORC_T_MatchC,
   ORC_T_ContainsC,
   ORC_T_MatchRegEx,
   ORC_T_LargerThan,
   ORC_T_SmallerThan,
   ORC_T_OlderThan,
   ORC_T_NewerThan,
   ORC_T_IsSpam
#ifdef USE_PYTHON
   ,ORC_T_Python
#endif 
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

static
wxString ORC_Logical[] =
{
   gettext_noop("or"),
   gettext_noop("and"),
};
static
size_t ORC_LogicalCount = WXSIZEOF(ORC_Logical);

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


OneCritControl::OneCritControl(wxWindow *parent,
                               bool followup)
{
   m_Parent = parent;
   if(! followup)
   {
      m_Logical = new wxChoice(parent, -1, wxDefaultPosition,
                               wxDefaultSize, ORC_LogicalCount,
                               ORC_Logical);
      m_Not = new wxCheckBox(parent, -1, _("Not"));
   }
   else
   {
      m_Logical = NULL;
      m_Not = new wxCheckBox(parent, -1, _("Not"));
   }
   m_Type = new wxChoice(parent, -1, wxDefaultPosition,
                         wxDefaultSize, ORC_TypesCount, ORC_Types);
   m_Argument = new wxTextCtrl(parent,-1,"", wxDefaultPosition);
   m_Where = new wxChoice(parent, -1, wxDefaultPosition,
                          wxDefaultSize, ORC_WhereCount, ORC_Where);
}

OneCritControl::~OneCritControl()
{
   // we need a destructor to clean up our controls:
   if(m_Logical) delete m_Logical;
   delete m_Not;
   delete m_Type;
   delete m_Argument;
   delete m_Where;
}

void
OneCritControl::LayoutControls(wxWindow **last)
{
   wxLayoutConstraints *c;

   c = new wxLayoutConstraints;
   c->left.SameAs(m_Parent, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   c->top.Below(*last, 2*LAYOUT_Y_MARGIN);
   if(m_Logical)
   {
      m_Logical->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->left.RightOf(m_Logical, LAYOUT_X_MARGIN);
      c->width.AsIs();
      c->top.SameAs(m_Logical, wxTop, 0);
      c->height.AsIs();
      m_Not->SetConstraints(c);
   }
   else
      m_Not->SetConstraints(c);
   CRIT_CTRLCONST(m_Not, m_Type);
   CRIT_CTRLCONST(m_Type, m_Argument);
   CRIT_CTRLCONST(m_Argument, m_Where);
   *last = m_Where;
}

void
OneCritControl::UpdateUI()
{
   int type = m_Type->GetSelection();
   m_Argument->Enable(
      !(type == ORC_T_Always
        || type == ORC_T_IsSpam
#ifdef USE_PYTHON
        || type == ORC_T_Python
#endif
         )
      );
   m_Where->Enable(
      type == ORC_T_Match
      || type == ORC_T_Contains
      || type == ORC_T_MatchC
      || type == ORC_T_ContainsC
      || type == ORC_T_MatchRegEx
      );
}


void
OneCritControl::Save(wxString *str)
{
   String tmp;
   tmp.Printf(" %d %d %d \"%s\" %d",
              (int)(m_Logical ? m_Logical->GetSelection() : -1),
              (int)m_Not->GetValue(),
              m_Type->GetSelection(),
              strutil_escapeString(m_Argument->GetValue()).c_str(),
              m_Where->GetSelection());
   *str << tmp;
}

void
OneCritControl::Load(wxString *str)
{
   bool success;
   long number = strutil_readNumber(*str, &success);
   if(success && number != -1 && m_Logical)
      m_Logical->SetSelection(number);
   number = strutil_readNumber(*str, &success);
   if(success)
      m_Not->SetValue(number != 0);
   number = strutil_readNumber(*str);
   m_Type->SetSelection(number);
   m_Argument->SetValue(strutil_readString(*str));
   number = strutil_readNumber(*str);
   m_Where->SetSelection(number);
}

/* static */
wxString
OneCritControl::TranslateToString(wxString & criterium)
{
   String program;
   // This returns the bit to go into an if between the brackets:
   // if ( .............. )

   long logical = strutil_readNumber(criterium);
   switch(logical)
   {
   case 0: // OR:
      program << "||";
      break;
   case 1: // AND:
      program << "&&";
      break;
   case -1: // none:
      break;
   default:
      ASSERT(0); // not possible
   }
   program << '(';
   if(strutil_readNumber(criterium) != 0)
      program << '!';
   
   long type = strutil_readNumber(criterium);
   wxString argument = strutil_readString(criterium);
   long where = strutil_readNumber(criterium);
   bool needsWhere = true;
   bool needsArgument = true;
   switch(type)
   {
   case ORC_T_Always:
      needsWhere = false;
      needsArgument = false;
      program << '1'; // true
      break;
   case ORC_T_Match:
      program << "matchi("; break;
   case ORC_T_Contains:
      program << "containsi("; break;
   case ORC_T_MatchC:
      program << "match("; break;
   case ORC_T_ContainsC:
      program << "contains("; break;
   case ORC_T_MatchRegEx:
      program << "matchregex("; break;
#ifdef USE_PYTHON
   case ORC_T_Python:
      program << "python(";
      needsWhere = false;
      break;
#endif
   case ORC_T_LargerThan:
   case ORC_T_SmallerThan:
   case ORC_T_OlderThan:
      needsArgument = true;
      needsWhere = false;
      switch(type)
      {
      case ORC_T_LargerThan:
         program << "size() > "; break;
      case ORC_T_SmallerThan:
         program << "size() < "; break;
      case ORC_T_OlderThan:
         program << "(date()-now()) > "; break;
      case ORC_T_NewerThan:
         program << "(date()-now()) < "; break;
      }
      break;
   case ORC_T_IsSpam:
      program << "isspam()";
      needsWhere = false;
      needsArgument = false;
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
   if(needsWhere && needsArgument)
      program << ',';
   if(needsArgument)
   {
      program << '"' << argument << '"';
   }
   if(needsWhere || needsArgument)
      program << ')'; // end of function call
   program << ')';
   return program;
}


class OneActionControl
{
public:
   OneActionControl(wxWindow *parent);
   
   void Save(wxString *str);
   void Load(wxString *str);

   void UpdateUI(void);
   void LayoutControls(wxWindow **last);

   /// translate a storage string into a proper rule
   static wxString TranslateToString(wxString & action);
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
  gettext_noop("Log Entry")
#ifdef USE_PYTHON
  ,gettext_noop("Python")
#endif
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
#ifdef USE_PYTHON
   ,OAC_T_Python
#endif
};

void
OneActionControl::UpdateUI()
{
   int type = m_Type->GetSelection();
   m_Argument->Enable(
      !(type == OAC_T_Delete || type == OAC_T_Expunge
#ifdef USE_PYTHON
        || type == OAC_T_Python
#endif
         )
      );
}

void
OneActionControl::Save(wxString *str)
{
   String tmp;
   tmp.Printf("%d \"%s\"",
              m_Type->GetSelection(),
              strutil_escapeString(m_Argument->GetValue()).c_str());
   *str << tmp;
}

void
OneActionControl::Load(wxString *str)
{
   m_Type->SetSelection(strutil_readNumber(*str));
   m_Argument->SetValue(strutil_readString(*str));
}


/* static */
wxString
OneActionControl::TranslateToString(wxString & action)
{
   long type = strutil_readNumber(action);
   wxString argument = strutil_readString(action);
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
#ifdef USE_PYTHON
   case OAC_T_Python:
      program << "python("; break;
#endif
   }
   if(needsArgument)
      program << '"' << argument << '"';
   program << ");";
   return program;
}



OneActionControl::OneActionControl(wxWindow *parent)
{
   m_Parent = parent;

   m_Type = new wxChoice(m_Parent, -1, wxDefaultPosition,
                         wxDefaultSize, OAC_TypesCount, OAC_Types);

   m_Argument = new wxTextCtrl(m_Parent,-1,"", wxDefaultPosition);
}

void
OneActionControl::LayoutControls(wxWindow **last)
{
   wxLayoutConstraints *c = new wxLayoutConstraints;

   c->left.SameAs(m_Parent, wxLeft, 8*LAYOUT_X_MARGIN);
   c->top.Below(*last, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   //c->width.PercentOf(m_Parent, wxWidth, 30);
   m_Type->SetConstraints(c);
   
   c = new wxLayoutConstraints;
   c->left.RightOf(m_Type, LAYOUT_X_MARGIN);
   c->width.PercentOf(m_Parent, wxWidth, 60);
   c->top.SameAs(m_Type, wxTop, 0);
   c->height.AsIs();
   m_Argument->SetConstraints(c);
   *last = m_Argument;
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

static const char * wxOneFButtonLabels[] =
{ gettext_noop("More"), gettext_noop("Less") };

wxOneFilterDialog::wxOneFilterDialog(class FilterEntryData *fed,
                                     wxWindow *parent)
   : wxManuallyLaidOutDialog(parent,
                            _("Filter rule"),
                            "OneFilterDialog")
{
   m_FilterData = fed;
   SetDefaultSize(380, 240, FALSE /* not minimal */);
   SetAutoLayout( TRUE );
   wxLayoutConstraints *c;

   wxStaticBox *box = CreateStdButtonsAndBox(_("Filter Rule"), FALSE,
                                             MH_DIALOG_FILTERS);

   /// The name of the filter rule:
   m_NameCtrl = new wxTextCtrl(this, -1);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_NameCtrl->SetConstraints(c);

   m_Panel = new wxEnhancedPanel(this, TRUE);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_NameCtrl,  2*LAYOUT_Y_MARGIN);
   c->bottom.Above(FindWindow(wxID_OK), -6*LAYOUT_Y_MARGIN);
   m_Panel->SetConstraints(c);
   m_Panel->SetAutoLayout(TRUE);


   wxWindow *canvas = m_Panel->GetCanvas();

   m_IfMessage = new wxStaticText(canvas,-1,_("If Message..."));

   m_DoThis = new wxStaticText(canvas,-1,_("Then do this:"));
   m_ActionControl = new OneActionControl(canvas);

   m_ButtonMore = new wxButton(canvas, -1, _(wxOneFButtonLabels[0]));
   m_ButtonLess = new wxButton(canvas, -1, _(wxOneFButtonLabels[1]));

   TransferDataToWindow();
   m_OriginalFilterData = *m_FilterData;
}


void
wxOneFilterDialog::LayoutControls()
{
   wxWindow *last = NULL;
   wxLayoutConstraints *c = new wxLayoutConstraints;

   c->left.SameAs(m_Panel->GetCanvas(), wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(m_Panel->GetCanvas(), wxTop, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_IfMessage->SetConstraints(c);

   last = m_IfMessage;
 
   for(size_t idx = 0; idx < m_nControls; idx++)
   {
      m_CritControl[idx]->LayoutControls(&last);
   }

   c = new wxLayoutConstraints();
   c->left.SameAs(m_Panel->GetCanvas(), wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(last, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_DoThis->SetConstraints(c);

   last = m_DoThis;
   m_ActionControl->LayoutControls(&last);
   
   wxSize ButtonSize = ::GetButtonSize(wxOneFButtonLabels,
                                       m_Panel->GetCanvas()); 

   c = new wxLayoutConstraints;
   c->left.SameAs(m_Panel->GetCanvas(), wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(last, 2*LAYOUT_Y_MARGIN);
   c->width.Absolute(ButtonSize.GetX());
   c->height.AsIs();
   m_ButtonMore->SetConstraints(c);
   
   c = new wxLayoutConstraints;
   c->left.RightOf(m_ButtonMore, 2*LAYOUT_X_MARGIN);
   c->top.Below(last, 2*LAYOUT_Y_MARGIN);
   c->width.Absolute(ButtonSize.GetX());
   c->height.AsIs();
   m_ButtonLess->SetConstraints(c);

   m_Panel->Layout();
   Layout();
}

void
wxOneFilterDialog::AddOneControl()
{
   ASSERT(m_nControls < MAX_CONTROLS);
   m_CritControl[m_nControls] = new
      OneCritControl(m_Panel->GetCanvas(),
                     m_nControls == 0); 
   m_nControls++;
   LayoutControls();
}


void
wxOneFilterDialog::RemoveOneControl()
{
   ASSERT(m_nControls > 1);
   m_nControls--;
   delete m_CritControl[m_nControls];
   LayoutControls();
}

void
wxOneFilterDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   for(size_t idx = 0; idx < m_nControls; idx++)
   {
      m_CritControl[idx]->UpdateUI();
   }
   m_ActionControl->UpdateUI();
   m_ButtonLess->Enable(m_nControls > 1);
   m_ButtonMore->Enable(m_nControls < MAX_CONTROLS);
}

void
wxOneFilterDialog::OnCancel(wxCommandEvent &event)
{
   *m_FilterData = m_OriginalFilterData;
   TransferDataToWindow();
   event.Skip();
}

void
wxOneFilterDialog::OnButton(wxCommandEvent &event)
{
   if(event.GetId() == wxID_CANCEL)
      OnCancel(event);
   else
   {
      if(event.GetEventObject() == m_ButtonLess)
      {
         RemoveOneControl();
      }
      else if(event.GetEventObject() == m_ButtonMore)
      {
         AddOneControl();
      }
      else
         event.Skip();
   }
}

bool
wxOneFilterDialog::TransferDataFromWindow()
{
   m_FilterData->SetName(m_NameCtrl->GetValue().c_str());

   String str1, str2;
   for(size_t idx = 0; idx < m_nControls; idx++)
   {
      m_CritControl[idx]->Save(&str1); 
   }
   m_ActionControl->Save(&str2);
   m_FilterData->SetCriterium(str1);
   m_FilterData->SetAction(str2);
   return TRUE;
}

bool
wxOneFilterDialog::TransferDataToWindow()
{
   m_NameCtrl->SetValue(m_FilterData->GetName());
   String str1, str2;
   str1 = m_FilterData->GetCriterium();
   str2 = m_FilterData->GetAction();

   m_nControls = 0;
   do
   {
      AddOneControl();
      m_CritControl[m_nControls-1]->Load(&str1);
      strutil_delwhitespace(str1);
   }while(str1.Length() > 0);

   LayoutControls();
   m_ActionControl->Load(&str2);
   return TRUE;
}




static
String TranslateOneRuleToProgram(const wxString &criterium,
                                 const wxString &action)
{
   wxString program;
   wxString
      str1 = criterium,
      str2 = action;
   if(criterium.Length() > 0 && action.Length() > 0)
   {
      program = "if(";
      do
      {
         program << OneCritControl::TranslateToString(str1);
         strutil_delwhitespace(str1);
      }while(str1.Length());
            
      program << ')'
              << '{'
              << OneActionControl::TranslateToString(str2)
              << '}';
      
   }
   return program;
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

// ----------------------------------------------------------------------------
// wxFiltersDialog - dialog for managing all filter rules for one folder
// ----------------------------------------------------------------------------


#define MAX_FILTERS   128 // who would want more?

class wxFiltersDialog : public wxOptionsPageSubdialog
{
public:
   // ctor & dtor
   wxFiltersDialog(ProfileBase *profile, wxWindow *parent);
   virtual ~wxFiltersDialog()
      {
         m_FiltersProfile->DecRef();
         m_Profile->DecRef();
      }

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool WasChanged(void) { return m_Filter != m_OriginalFilter;}

   // event handlers
   void Update(wxCommandEvent& ) { DoUpdate(); }
   void DoUpdate(void);
   void OnButton(wxCommandEvent & event );
   void OnCancel(wxCommandEvent & event );
protected:
   void UpdateButtons(void);
   void DeleteEntry(size_t idx);
   void MoveEntryUp(size_t idx);
   void MoveEntryDown(size_t idx);
   
   /// ugly static array, but makes life simple
   FilterEntryData m_FilterData[MAX_FILTERS];
   size_t          m_FilterDataCount;

   FilterEntryData m_OriginalFilterData[MAX_FILTERS];
   size_t          m_OriginalFilterDataCount;
   
   wxCheckListBox *m_ListBox;
   wxButton       *m_Buttons[WXSIZEOF(ButtonLabels)];
   
// data
   wxString m_Filter, m_OriginalFilter;
   ProfileBase *m_FiltersProfile, *m_Profile;
private:
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxFiltersDialog, wxOptionsPageSubdialog)
   EVT_LISTBOX(-1, wxFiltersDialog::UpdateButtons)
   EVT_BUTTON(-1,  wxFiltersDialog::OnButton)
   EVT_BUTTON(wxID_CANCEL, wxFiltersDialog::OnCancel)
END_EVENT_TABLE()


   
wxFiltersDialog::wxFiltersDialog(ProfileBase *profile, wxWindow *parent)
   : wxOptionsPageSubdialog(profile,
                            parent,
                            _("Configure Filter Rules"),
                            "FilterDialog")
{
   m_FiltersProfile = ProfileBase::CreateProfile("Filters", profile);
   m_Profile = profile;
   m_Profile->IncRef();
   
   wxLayoutConstraints *c;
   wxString name, pname;
   const char * prefix = "/M/Profiles/";
   size_t prefixLen = strlen(prefix);
   
   pname = profile->GetName();
   if(pname.Length() > prefixLen 
      && pname.Left(prefixLen) == prefix)
   {
      pname = pname.Mid(prefixLen);
      name = _("Filter Rules for folder: ");
      name << pname;
   }
   else
   {
      name = _("Filter Rules for: ");
      name << pname;
   }
   
   wxStaticBox *box = CreateStdButtonsAndBox(name, FALSE,
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
   m_ListBox = new wxCheckListBox(this, -1);
   m_ListBox->SetConstraints(c);

   SetDefaultSize(380, 240, FALSE /* not minimal */);
   TransferDataToWindow();

   for(size_t i = 0; i < m_FilterDataCount; i++)
      m_OriginalFilterData[i] = m_FilterData[i];
   m_OriginalFilterDataCount = m_FilterDataCount;
   
   m_OriginalFilter = m_Filter;
}


void
wxFiltersDialog::DeleteEntry(size_t idx)
{
   wxASSERT(idx >= 0); wxASSERT(idx < m_FilterDataCount);
   for(size_t i = idx; i < m_FilterDataCount-1; i++)
      m_FilterData[i] = m_FilterData[i+1];
   m_FilterDataCount--;
}

void
wxFiltersDialog::MoveEntryUp(size_t idx)
{
   wxASSERT(idx > 0); wxASSERT(idx < m_FilterDataCount);
   FilterEntryData fed;
   fed = m_FilterData[idx];
   m_FilterData[idx] = m_FilterData[idx-1];
   m_FilterData[idx-1] = fed;
   m_ListBox->SetSelection(idx-1);
}

void
wxFiltersDialog::MoveEntryDown(size_t idx)
{
   wxASSERT(idx >= 0); wxASSERT(idx < m_FilterDataCount-1);
   FilterEntryData fed;
   fed = m_FilterData[idx];
   m_FilterData[idx] = m_FilterData[idx+1];
   m_FilterData[idx+1] = fed;
   m_ListBox->SetSelection(idx+1);
}


void
wxFiltersDialog::OnCancel( wxCommandEvent &event )
{
   m_FilterDataCount = m_OriginalFilterDataCount;
   for(size_t i = 0; i < m_FilterDataCount; i++)
      m_FilterData[i] = m_OriginalFilterData[i];
   m_Filter = m_OriginalFilter;
   event.Skip();
}

void
wxFiltersDialog::OnButton( wxCommandEvent &event )
{
   wxObject *obj = event.GetEventObject();
   wxString name;
   int pos = m_ListBox->GetSelection();
   if(pos >= 0)
      name = m_ListBox->GetString(pos);

   for(size_t idx = 0; ButtonLabels[idx]; idx++)
      if(obj == m_Buttons[idx])
      {
         if(idx == Button_New && idx < MAX_FILTERS)
         {
            if(ConfigureOneFilter(&(m_FilterData[m_FilterDataCount]), this))
               m_FilterDataCount++;
         }
         else if(idx == Button_Edit) // edit existing
         {
            wxASSERT(pos < (long int) m_FilterDataCount);
            wxASSERT(pos > -1);
            ConfigureOneFilter(& (m_FilterData[pos]), this );
         }
         else if(idx == Button_Delete)
            DeleteEntry(pos);
         else if(idx == Button_Up)
            MoveEntryUp(pos);
         else if(idx == Button_Down)
            MoveEntryDown(pos);
         DoUpdate();
      }
   event.Skip();
}

void
wxFiltersDialog::DoUpdate(void )
{
   // First, make sure we have the activation flags set correctly:
   for(int i = 0; i < m_ListBox->Number(); i++)
      m_FilterData[i].SetActive(
         m_ListBox->IsChecked(i) );

   int selection = m_ListBox->GetSelection();
   m_ListBox->Clear();
   for(size_t i = 0; i < m_FilterDataCount; i++)
   {
      m_ListBox->Append(m_FilterData[i].GetName());
      m_ListBox->Check(i, m_FilterData[i].GetActive());
   }
   if(selection > -1 && selection < m_ListBox->Number())
      m_ListBox->SetSelection(selection);
   UpdateButtons();
}

void
wxFiltersDialog::UpdateButtons(void)
{
   size_t nEntries = m_FilterDataCount;
   int selection = m_ListBox->GetSelection();


   if(selection >= 0)
      m_FilterData[selection].SetActive(
         m_ListBox->IsChecked(selection) );
   // in case we hit the limit:
   m_Buttons[Button_New]->Enable( nEntries < MAX_FILTERS );
   
   if(nEntries == 0)
   {
      m_Buttons[Button_Edit]->Enable(false);
      m_Buttons[Button_Up]->Enable(false);
      m_Buttons[Button_Down]->Enable(false);
      m_Buttons[Button_Delete]->Enable(false);
      return;
   }
   if( selection != -1)
   {
      m_Buttons[Button_Edit]->Enable(true);
      m_Buttons[Button_Delete]->Enable(true);
      m_Buttons[Button_Up]->Enable(selection > 0);
      m_Buttons[Button_Down]->Enable(selection < (int) (nEntries-1));
   }
}

bool
wxFiltersDialog::TransferDataFromWindow()
{
   DoUpdate();
   
   /* We need to remove all old subgroups and write the new
      settings. */
   size_t i;
   wxArrayString groups;
   wxString name;
   long ref;
   for(bool found = m_FiltersProfile->GetFirstGroup(name, ref);
       found;
       found = m_FiltersProfile->GetNextGroup(name, ref))
      groups.Add(name);
   for(i = 0; i < groups.Count(); i++)
      m_FiltersProfile->DeleteGroup(groups[i]);

   m_Filter = "";
   // now they are all gone, we can write the new groups:
   for(i = 0; i < m_FilterDataCount; i++)
   {
      wxString name;
      name.Printf("%ld", (long int) i);
      m_FiltersProfile->writeEntry(name+"/Name", m_FilterData[i].GetName());
      m_FiltersProfile->writeEntry(name+"/Criterium", m_FilterData[i].GetCriterium());
      m_FiltersProfile->writeEntry(name+"/Action", m_FilterData[i].GetAction());
      m_FiltersProfile->writeEntry(name+"/Active", m_FilterData[i].GetActive());
      if(m_FilterData[i].GetActive())
         m_Filter <<
            TranslateOneRuleToProgram(m_FilterData[i].GetCriterium(),
                                      m_FilterData[i].GetAction());
   }
   m_Profile->writeEntry("Filter", m_Filter);
   return TRUE;
}


bool
wxFiltersDialog::TransferDataToWindow()
{
   /* Filters are all stored as subgroups under the group "Filters" in 
      the current profile. I.e. ./Filters/a ./Filters/b etc
      Fitler names and group names do not correspond, group names are
      just created to be unique.

      Group names are 0, 1, 2, ...
      Here, we simply copy all that information into the array of
      filter data.
   */

   m_Filter = m_Profile->readEntry("Filter","");
   
   m_ListBox->Clear();

   m_FilterDataCount = 0;
   for(size_t i = 0; i < MAX_FILTERS; i++)
   {
      wxString name;
      name.Printf("%ld", (long int) i);
      if(! m_FiltersProfile->HasGroup(name))
         break;
      m_FilterData[i].SetName(
         m_FiltersProfile->readEntry(name+"/Name",_("unnamed")));
      m_FilterData[i].SetCriterium(
         m_FiltersProfile->readEntry(name+"/Criterium",""));
      m_FilterData[i].SetAction(
         m_FiltersProfile->readEntry(name+"/Action",""));
      m_FilterData[i].SetActive(
         m_FiltersProfile->readEntry(name+"/Active", 0L) != 0);
      m_FilterDataCount++;
   }
   DoUpdate();
   return true;
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

