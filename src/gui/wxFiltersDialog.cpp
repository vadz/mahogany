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

#ifdef __GNUG__
   #pragma implementation "wxFiltersDialog.h"
#endif

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
#include "Mpers.h"
#include "MFilter.h"
#include "MFolder.h"

#include <wx/window.h>
#include <wx/confbase.h>
#include "wx/persctrl.h"
#include <wx/stattext.h>
#include <wx/layout.h>
#include <wx/checklst.h>
#include <wx/statbox.h>

#include "gui/wxBrowseButton.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxFiltersDialog.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   // for wxFiltersDialog and wxFolderFiltersDialog
   Button_Add = 100,
   Button_Delete,
   Button_Edit,

   // for wxOneFilterDialog
   Text_Program = 200
};

// the config entries under[Filters/N]
#define FILTER_NAME        "Name"
#define FILTER_CRITERIUM   "Criterium"
#define FILTER_ACTION      "Action"
#define FILTER_ACTIVE      "Active"

// data for the OneCritControl

static const
wxString ORC_Types[] =
{
   gettext_noop("Always"),
   gettext_noop("Match"),
   gettext_noop("Contains"),
   gettext_noop("Match Case"),
   gettext_noop("Contains Case"),
   gettext_noop("Match RegExp Case"),
   gettext_noop("Larger than"),
   gettext_noop("Smaller than"),
   gettext_noop("Older than"),
   gettext_noop("Newer than"),
   gettext_noop("Is RBL filtered (SPAM)"),
   gettext_noop("Python"),
   gettext_noop("Match RegExp"),
   gettext_noop("Score above"),
   gettext_noop("Score below")
};

static const
size_t ORC_TypesCount = WXSIZEOF(ORC_Types);

static const
wxString ORC_Where[] =
{
   gettext_noop("in subject"),
   gettext_noop("in header"),
   gettext_noop("in from"),
   gettext_noop("in body"),
   gettext_noop("in message"),
   gettext_noop("in to"),
   gettext_noop("in sender"),
   gettext_noop("in any recipient")
};

static const
size_t ORC_WhereCount = WXSIZEOF(ORC_Where);

static const
wxString ORC_Logical[] =
{
   gettext_noop("or"),
   gettext_noop("and"),
};

static const
size_t ORC_LogicalCount = WXSIZEOF(ORC_Logical);

// and for OneActionControl

static const
wxString OAC_Types[] =
{
   gettext_noop("Delete"),
   gettext_noop("Copy to"),
   gettext_noop("Move to"),
   gettext_noop("Expunge"),
   gettext_noop("MessageBox"),
   gettext_noop("Log Entry"),
   gettext_noop("Python"),
   gettext_noop("Change Score"),
   gettext_noop("Set Colour"),
   gettext_noop("Delete Duplicates"),
   gettext_noop("Print")
};

static const
size_t OAC_TypesCount = WXSIZEOF(OAC_Types);

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// check whether a filter with the given name exists
static bool FilterExists(const String& name);

// create a new filter, return its name (or an empty string if the filter
// creation was cancelled)
static String CreateNewFilter(wxWindow *parent);

// edit the filter with given name, return TRUE if anything changed
static bool EditFilter(const String& name, wxWindow *parent);

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// wxOneFilterDialog - dialog for exactly one filter rule
// ----------------------------------------------------------------------------

#define MAX_CONTROLS   256

/** A class representing the configuration GUI for a single filter. */
class wxOneFilterDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor & dtor
   wxOneFilterDialog(class MFilterDesc *fd, wxWindow *parent);

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool HasChanges() const { return !(*m_FilterData == m_OriginalFilterData);}

   // event handlers
   void OnUpdateUI(wxUpdateUIEvent& event);
   void OnButton(wxCommandEvent& event);
   void OnProgramTextUpdate(wxCommandEvent& event);

   void OnText(wxCommandEvent& event) { UpdateProgram(event); }
   void OnChoice(wxCommandEvent& event) { UpdateProgram(event); }
   void OnCheckBox(wxCommandEvent& event) { UpdateProgram(event); }

protected:
   void AddOneControl();
   void RemoveOneControl();

   void LayoutControls();

   // helper of TransferDataToWindow() and UpdateProgram()
   bool DoTransferDataFromWindow(MFilterDesc *filterData);

   // update the program text from the controls values
   void UpdateProgram(wxCommandEvent& event);

   // true if we have a simple (expressed with dialog controls) filter
   bool m_isSimple;

   // true until the end of TransferDataToWindow()
   bool m_initializing;

   // the new and original data describing the filter
   MFilterDesc *m_FilterData;
   MFilterDesc  m_OriginalFilterData;

   // GUI controls
   wxButton *m_ButtonLess,
            *m_ButtonMore;

   wxStaticText *m_IfMessage,
                *m_DoThis,
                *m_msgCantEdit;

   // controls containing data
   wxTextCtrl *m_NameCtrl;

   size_t      m_nControls;
   class OneCritControl *m_CritControl[MAX_CONTROLS];
   class OneActionControl *m_ActionControl;

   wxTextCtrl *m_textProgram;

   wxEnhancedPanel *m_Panel;

private:
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxOneFilterDialog, wxManuallyLaidOutDialog)
   EVT_UPDATE_UI(-1, wxOneFilterDialog::OnUpdateUI)

   // More/Less button
   EVT_BUTTON(-1, wxOneFilterDialog::OnButton)

   // changes to the program text
   EVT_TEXT(Text_Program, wxOneFilterDialog::OnProgramTextUpdate)

   // changes to any other controls
   EVT_TEXT(-1, wxOneFilterDialog::OnText)
   EVT_CHOICE(-1, wxOneFilterDialog::OnChoice)
   EVT_CHECKBOX(-1, wxOneFilterDialog::OnCheckBox)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// OneCritControl: a control handling one test from MFDialogSettings
// ----------------------------------------------------------------------------

class OneCritControl
{
public:
   OneCritControl(wxWindow *parent,
                  bool followup = FALSE);
   ~OneCritControl();

   /// init the control values from n-th test in MFDialogSettings
   void SetValues(const MFDialogSettings& settings, size_t n);

   /// accessors
   MFDialogLogical GetLogical() const
   {
      return m_Logical ? (MFDialogLogical)m_Logical->GetSelection()
                       : ORC_L_None;
   }

   bool IsInverted() const
   {
      return m_Not->GetValue();
   }

   MFDialogTest GetTest() const
   {
      return (MFDialogTest)m_Type->GetSelection();
   }

   MFDialogTarget GetTarget() const
   {
      return (MFDialogTarget)m_Where->GetSelection();
   }

   String GetArgument() const
   {
      return m_Argument->GetValue();
   }

   void UpdateUI();
   void Disable()
   {
      m_Not->Disable();
      m_Type->Disable();
      m_Argument->Disable();
      m_Where->Disable();
      if ( m_Logical )
         m_Logical->Disable();
   }

   /// layout the current controls under the window *last
   void LayoutControls(wxWindow **last);
   wxWindow *GetLastCtrl() { return m_Where; }

private:
   wxCheckBox *m_Not;   // Not
   wxChoice   *m_Type;  // "Match", "Match substring", "Match RegExp",
                        // "Greater than", "Smaller than", "Older than"; "Newer than"
   wxTextCtrl *m_Argument; // string, number of days or bytes
   wxChoice   *m_Where; // "In Header", "In Subject", "In From..."
   wxChoice   *m_Logical;

   wxWindow   *m_Parent; // the parent for all these controls
};

OneCritControl::OneCritControl(wxWindow *parent,
                               bool followup)
{
   m_Parent = parent;

   if(! followup)
   {
      wxASSERT_MSG( ORC_LogicalCount == ORC_L_Max, "forgot to update something" );

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

   wxASSERT_MSG( ORC_TypesCount == ORC_T_Max, "forgot to update something" );
   m_Type = new wxChoice(parent, -1, wxDefaultPosition,
                         wxDefaultSize, ORC_TypesCount, ORC_Types);
   m_Argument = new wxTextCtrl(parent,-1,"", wxDefaultPosition);

   wxASSERT_MSG( ORC_WhereCount == ORC_W_Max, "forgot to update something" );
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

   c = new wxLayoutConstraints;
   c->left.RightOf(m_Not, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.SameAs(m_Not, wxTop, 0);
   c->height.AsIs();
   m_Type->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->right.SameAs(m_Parent, wxRight, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.SameAs(m_Not, wxTop, 0);
   c->height.AsIs();
   m_Where->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.RightOf(m_Type, LAYOUT_X_MARGIN);
   c->right.LeftOf(m_Where, LAYOUT_X_MARGIN);
   c->top.SameAs(m_Not, wxTop, 0);
   c->height.AsIs();
   m_Argument->SetConstraints(c);

   *last = m_Where;
}

void
OneCritControl::UpdateUI()
{
   int type = m_Type->GetSelection();
   m_Argument->Enable(
      !(type == ORC_T_Always
        || type == ORC_T_IsSpam
        || type == ORC_T_Python
         )
      );
   m_Where->Enable(
      type == ORC_T_Match
      || type == ORC_T_Contains
      || type == ORC_T_MatchRegEx
      || type == ORC_T_MatchC
      || type == ORC_T_ContainsC
      || type == ORC_T_MatchRegExC
      );
}

void
OneCritControl::SetValues(const MFDialogSettings& settings, size_t n)
{
   MFDialogLogical logical = settings.GetLogical(n);
   if ( m_Logical && logical != ORC_L_None )
      m_Logical->SetSelection(logical);

   m_Not->SetValue(settings.IsInverted(n));
   m_Type->SetSelection(settings.GetTest(n));
   m_Where->SetSelection(settings.GetTestTarget(n));
   m_Argument->SetValue(settings.GetTestArgument(n));
}

// ----------------------------------------------------------------------------
// OneActionControl: allows to edit the action part of MFDialogSettings
// ----------------------------------------------------------------------------

class OneActionControl
{
public:
   OneActionControl(wxWindow *parent);

   /// init from MFDialogSettings
   void SetValues(const MFDialogSettings& settings)
   {
      m_Type->SetSelection(settings.GetAction());
      m_Argument->SetValue(settings.GetActionArgument());
   }

   /// get the action
   MFDialogAction GetAction() const
      { return (MFDialogAction)m_Type->GetSelection(); }

   /// get the action argument
   String GetArgument() const { return m_Argument->GetValue(); }

   void UpdateUI(void);
   void Hide()
   {
      m_Type->Hide();
      m_Argument->Hide();
      m_btnFolder->Hide();
      m_btnColour->Hide();
   }
   void Disable()
   {
      m_Type->Disable();
      m_Argument->Disable();
      m_btnFolder->Disable();
      m_btnColour->Disable();
   }

   void LayoutControls(wxWindow **last,
                       int leftMargin = 8*LAYOUT_X_MARGIN,
                       int rightMargin = 2*LAYOUT_X_MARGIN);

private:
   wxChoice             *m_Type;       // Which action to perform
   wxTextCtrl           *m_Argument;   // string, number of days or bytes

   // browse buttons: only one of them is currently shown
   wxFolderBrowseButton *m_btnFolder;
   wxColorBrowseButton  *m_btnColour;

   wxWindow             *m_Parent;     // the parent for all these controls
};

void
OneActionControl::UpdateUI()
{
   int type = m_Type->GetSelection();

   bool enable = !(type == OAC_T_Delete
                   || type == OAC_T_Expunge
                   //|| type == OAC_T_Python
                   || type == OAC_T_Uniq
                   || type == OAC_T_Print
      );
   m_Argument->Enable(enable);

   switch ( type )
   {
      case OAC_T_CopyTo:
      case OAC_T_MoveTo:
         // browse for folder
         m_btnColour->Hide();
         m_btnFolder->Show();
         m_btnFolder->Enable(TRUE);
         break;

      case OAC_T_SetColour:
         // browse for colour
         m_btnFolder->Hide();
         m_btnColour->Show();
         m_btnColour->Enable(TRUE);
         break;

      default:
         // nothing to browse for
         m_btnColour->Disable();
         m_btnFolder->Disable();
   }
}

OneActionControl::OneActionControl(wxWindow *parent)
{
   wxASSERT_MSG( OAC_TypesCount == OAC_T_Max, "forgot to update something" );

   m_Parent = parent;

   m_Type = new wxChoice(parent, -1, wxDefaultPosition, wxDefaultSize,
                         OAC_TypesCount, OAC_Types);
   m_Argument = new wxTextCtrl(parent, -1, "");
   m_btnFolder = new wxFolderBrowseButton(m_Argument, parent);
   m_btnColour = new wxColorBrowseButton(m_Argument, parent);
}

void
OneActionControl::LayoutControls(wxWindow **last,
                                 int leftMargin,
                                 int rightMargin)
{
   wxLayoutConstraints *c = new wxLayoutConstraints;

   c->left.SameAs(m_Parent, wxLeft, leftMargin);
   c->top.Below(*last, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_Type->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->right.SameAs(m_Parent, wxRight, rightMargin);
   c->width.AsIs();
   c->top.SameAs(m_Type, wxTop, 0);
   c->height.AsIs();
   m_btnFolder->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.SameAs(m_btnFolder, wxLeft);
   c->width.SameAs(m_btnFolder, wxWidth);
   c->top.SameAs(m_btnFolder, wxTop);
   c->height.SameAs(m_btnFolder, wxHeight);
   m_btnColour->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.RightOf(m_Type, LAYOUT_X_MARGIN);
   c->right.LeftOf(m_btnFolder, LAYOUT_X_MARGIN);
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

// ----------------------------------------------------------------------------
// wxOneFilterDialog
// ----------------------------------------------------------------------------

wxOneFilterDialog::wxOneFilterDialog(MFilterDesc *fd, wxWindow *parent)
                 : wxManuallyLaidOutDialog(parent,
                                           _("Filter Rule"),
                                           "OneFilterDialog")
{
   m_isSimple = true;
   m_initializing = true;
   m_nControls = 0;
   m_FilterData = fd;
   SetAutoLayout( TRUE );
   wxLayoutConstraints *c;

   wxStaticBox *box = CreateStdButtonsAndBox(_("Filter Rule"), FALSE,
                                             MH_DIALOG_FILTERS_DETAILS);

   /// The name of the filter rule:
   wxStaticText *msg = new wxStaticText(this, -1, _("&Name: "));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   m_NameCtrl = new wxTextCtrl(this, -1);
   c = new wxLayoutConstraints;
   c->left.RightOf(msg, LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(msg, wxCentreY);
   c->height.AsIs();
   m_NameCtrl->SetConstraints(c);

   // newly created filters can be given any name, of course, but if the
   // filter already exists under some name it can't be renamed as this will
   // break all the folders using it, so disable the control in this case
   if ( !!m_FilterData->GetName() )
      m_NameCtrl->Disable();

   // the control allowing to edit directly the filter program
   m_textProgram = new wxTextCtrl(this, Text_Program,
                                  "",
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE);
   c = new wxLayoutConstraints;
   c->height.Absolute(4*hBtn);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   m_textProgram->SetConstraints(c);

   m_Panel = new wxEnhancedPanel(this, TRUE);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_NameCtrl,  2*LAYOUT_Y_MARGIN);
   c->bottom.Above(m_textProgram, -2*LAYOUT_Y_MARGIN);
   m_Panel->SetConstraints(c);
   m_Panel->SetAutoLayout(TRUE);

   m_OriginalFilterData = *m_FilterData;

   wxWindow *canvas = m_Panel->GetCanvas();
   m_ActionControl = new OneActionControl(canvas);
   m_IfMessage = new wxStaticText(canvas, -1, _("If message..."));
   m_DoThis = new wxStaticText(canvas, -1, _("Then do this:"));
   m_ButtonMore = new wxButton(canvas, -1, _("&More"));
   m_ButtonLess = new wxButton(canvas, -1, _("&Fewer"));
   m_ButtonMore->SetToolTip(_("Add another condition"));
   m_ButtonLess->SetToolTip(_("Remove the last condition"));

   SetDefaultSize(8*wBtn, 18*hBtn);
}

void
wxOneFilterDialog::LayoutControls()
{
   wxLayoutConstraints *c;
   wxWindow *canvas = m_Panel->GetCanvas();

   if ( m_isSimple )
   {
      wxWindow *last = NULL;

      c = new wxLayoutConstraints;
      c->left.SameAs(canvas, wxLeft, 2*LAYOUT_X_MARGIN);
      c->top.SameAs(canvas, wxTop, 2*LAYOUT_Y_MARGIN);
      c->width.AsIs();
      c->height.AsIs();
      m_IfMessage->SetConstraints(c);

      last = m_IfMessage;
      for ( size_t idx = 0; idx < m_nControls; idx++ )
      {
         m_CritControl[idx]->LayoutControls(&last);
      }

      c = new wxLayoutConstraints;
      c->right.SameAs(canvas, wxCentreX, -LAYOUT_X_MARGIN);
      c->top.Below(last, 3*LAYOUT_Y_MARGIN);
      c->width.Absolute(wBtn);
      c->height.AsIs();
      m_ButtonMore->SetConstraints(c);

      c = new wxLayoutConstraints;
      c->left.RightOf(m_ButtonMore, 2*LAYOUT_X_MARGIN);
      c->top.Below(last, 3*LAYOUT_Y_MARGIN);
      c->width.Absolute(wBtn);
      c->height.AsIs();
      m_ButtonLess->SetConstraints(c);

      last = m_ButtonLess;
      c = new wxLayoutConstraints();
      c->left.SameAs(canvas, wxLeft, 2*LAYOUT_X_MARGIN);
      c->top.Below(last, 4*LAYOUT_Y_MARGIN);
      c->width.AsIs();
      c->height.AsIs();
      m_DoThis->SetConstraints(c);

      last = m_DoThis;
      m_ActionControl->LayoutControls(&last);

      m_Panel->ForceLayout();
      Layout();
   }
}

void
wxOneFilterDialog::AddOneControl()
{
   ASSERT_MSG( m_nControls < MAX_CONTROLS, "too many filter controls" );

   m_CritControl[m_nControls] = new OneCritControl(m_Panel->GetCanvas(),
                                                   m_nControls == 0);
   m_nControls++;
}


void
wxOneFilterDialog::RemoveOneControl()
{
   ASSERT(m_nControls > 1);
   m_nControls--;
   delete m_CritControl[m_nControls];
}

void wxOneFilterDialog::UpdateProgram(wxCommandEvent& event)
{
   if ( !m_initializing )
   {
      MFilterDesc filterData;
      if ( DoTransferDataFromWindow(&filterData) && (filterData != *m_FilterData) )
      {
         *m_FilterData = filterData;
         MFDialogSettings *settings = m_FilterData->GetSettings();

         m_initializing = true;
         m_textProgram->SetValue(settings->WriteRule());
         m_initializing = false;

         settings->DecRef();
      }
   }

   event.Skip();
}

void
wxOneFilterDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   if ( m_isSimple )
   {
      for ( size_t idx = 0; idx < m_nControls; idx++ )
      {
         m_CritControl[idx]->UpdateUI();
      }

      m_ActionControl->UpdateUI();
      m_ButtonLess->Enable(m_nControls > 1);
      m_ButtonMore->Enable(m_nControls < MAX_CONTROLS);
   }

   FindWindow(wxID_OK)->Enable( m_NameCtrl->GetValue().Length() != 0 );
}

void
wxOneFilterDialog::OnProgramTextUpdate(wxCommandEvent& event)
{
   // catches the case of SetValue() from TransferDataToWindow()
   if ( m_initializing )
   {
      return;
   }

   // disable all the other controls
   for ( size_t idx = 0; idx < m_nControls; idx++ )
   {
      m_CritControl[idx]->Disable();
   }

   m_ActionControl->Disable();
   m_ButtonLess->Disable();
   m_ButtonMore->Disable();

   // and set the flag telling us to use the program text and not the controls
   m_isSimple = false;
}

void
wxOneFilterDialog::OnButton(wxCommandEvent &event)
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
   {
      event.Skip();

      return;
   }

   LayoutControls();
}

bool
wxOneFilterDialog::TransferDataToWindow()
{
   m_NameCtrl->SetValue(m_FilterData->GetName());
   if ( m_FilterData->IsSimple() )
   {
      MFDialogSettings *settings = m_FilterData->GetSettings();

      size_t count = settings->CountTests();
      for ( size_t n = 0; n < count; n++ )
      {
         AddOneControl();
         m_CritControl[n]->SetValues(*settings, n);
      }

      m_ActionControl->SetValues(*settings);

      // show the program text too
      m_textProgram->SetValue(settings->WriteRule());
      settings->DecRef();

      LayoutControls();
   }
   else if ( m_FilterData->IsEmpty() )
   {
      AddOneControl();
      LayoutControls();
   }
   else
   {
      m_ActionControl->Hide();
      m_ButtonMore->Hide();
      m_ButtonLess->Hide();
      m_IfMessage->Hide();
      m_DoThis->Hide();

      m_isSimple = false;

      wxWindow *canvas = m_Panel->GetCanvas();
      m_msgCantEdit = new wxStaticText(canvas, -1,
                                       _("This filter rule can only be "
                                         "edited directly."));
      wxLayoutConstraints *c = new wxLayoutConstraints;
      c->centreY.SameAs(canvas, wxCentreY);
      c->centreX.SameAs(canvas, wxCentreX);
      c->width.AsIs();
      c->height.AsIs();
      m_msgCantEdit->SetConstraints(c);

      m_Panel->ForceLayout();
      Layout();

      m_textProgram->SetValue(m_FilterData->GetProgram());
   }

   // now any updates come from user, not from program
   m_initializing = false;

   return TRUE;
}

bool
wxOneFilterDialog::DoTransferDataFromWindow(MFilterDesc *filterData)
{
   filterData->SetName(m_NameCtrl->GetValue());

   // if the user used the text control to enter the filter program directly,
   // just remember it
   if ( !m_isSimple )
   {
      // TODO try to parse the program here
      filterData->Set(m_textProgram->GetValue());
   }
   else // it is a simple (i.e. made from dialog controls) filter
   {
      MFDialogSettings *settings = MFDialogSettings::Create();

      for ( size_t idx = 0; idx < m_nControls; idx++ )
      {
         OneCritControl *ctrl = m_CritControl[idx];
         settings->AddTest(ctrl->GetLogical(),
                           ctrl->IsInverted(),
                           ctrl->GetTest(),
                           ctrl->GetTarget(),
                           ctrl->GetArgument());
      }

      settings->SetAction(m_ActionControl->GetAction(),
                          m_ActionControl->GetArgument());

      filterData->Set(settings);
   }

   return TRUE;
}

bool
wxOneFilterDialog::TransferDataFromWindow()
{
   return DoTransferDataFromWindow(m_FilterData);
}

// ----------------------------------------------------------------------------
// wxFiltersDialog - dialog for managing all filter rules in the program
// ----------------------------------------------------------------------------

class wxFiltersDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor & dtor
   wxFiltersDialog(wxWindow *parent);

   // transfer data to/from dialog
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // returns TRUE if the format string was changed
   bool HasChanges(void) const { return m_hasChanges; }

   // event handlers
   void OnAddFiter(wxCommandEvent& event);
   void OnEditFiter(wxCommandEvent& event);
   void OnDeleteFiter(wxCommandEvent& event);

   void OnListboxChange(wxCommandEvent& event) { DoUpdate(); }

protected:
   // update the buttons state depending on the lbox selection
   void DoUpdate();

   // listbox contains the names of all filters
   wxListBox *m_lboxFilters;

   // the Add/Edit/Delete buttons
   wxButton *m_btnAdd,
            *m_btnEdit,
            *m_btnDelete;

   // did anything change?
   bool m_hasChanges;

private:
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxFiltersDialog, wxManuallyLaidOutDialog)
   EVT_BUTTON(Button_Add, wxFiltersDialog::OnAddFiter)
   EVT_BUTTON(Button_Edit, wxFiltersDialog::OnEditFiter)
   EVT_BUTTON(Button_Delete, wxFiltersDialog::OnDeleteFiter)

   EVT_LISTBOX(-1, wxFiltersDialog::OnListboxChange)
   EVT_LISTBOX_DCLICK(-1, wxFiltersDialog::OnEditFiter)
END_EVENT_TABLE()

wxFiltersDialog::wxFiltersDialog(wxWindow *parent)
               : wxManuallyLaidOutDialog(parent,
                                         _("Configure Filter Rules"),
                                         "FilterDialog")
{
   m_hasChanges = false;

   wxLayoutConstraints *c;

   wxStaticBox *box = CreateStdButtonsAndBox("", FALSE,
                                             MH_DIALOG_FILTERS);

   /* This dialog is supposed to look like this:

      +------------------+
      |listbox of        |
      |existing filters  |  [new]
      |                  |  [mod]
      |                  |  [del]
      |                  |
      +------------------+

                [help][ok][cancel]
   */

   // start with the buttons
   m_btnDelete = new wxButton(this, Button_Delete, _("&Delete"));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->centreY.SameAs(box, wxCentreY);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   m_btnDelete->SetConstraints(c);

   m_btnAdd = new wxButton(this, Button_Add, _("&Add..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(m_btnDelete, wxRight);
   c->bottom.Above(m_btnDelete, -LAYOUT_Y_MARGIN);
   m_btnAdd->SetConstraints(c);

   m_btnEdit = new wxButton(this, Button_Edit, _("&Edit..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(m_btnDelete, wxRight);
   c->top.Below(m_btnDelete, LAYOUT_Y_MARGIN);
   m_btnEdit->SetConstraints(c);

   // create the listbox in the left side of the dialog
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(m_btnDelete, 3*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_lboxFilters = new wxPListBox("FiltersList",
                                  this, -1,
                                  wxDefaultPosition, wxDefaultSize,
                                  0, NULL,
                                  wxLB_SORT);
   m_lboxFilters->SetConstraints(c);

   SetDefaultSize(5*wBtn, 9*hBtn);
   m_lboxFilters->SetFocus();
}

void
wxFiltersDialog::OnAddFiter(wxCommandEvent &event)
{
   String name = CreateNewFilter(this);
   if ( !name )
   {
      // cancelled
      return;
   }

   int idx = m_lboxFilters->FindString(name);
   if ( idx == -1 )
   {
      m_lboxFilters->Append(name);

      // a newly added filter is not used by any folders yet, but this could
      // be surprizing (well, in fact, it's really bad design and we should
      // just allow to configure the folders which use this filter in the
      // wxOneFilterDialog - TODO)
      String msg;
      msg.Printf(_("The filter '%s' which you have just created is not\n"
                   "yet used by any folder. Would you like to select\n"
                   "a folder to which you'd like to assign this filter\n"
                   "right now (otherwise you can do it later by using\n"
                   "the \"Filters\" entry in the \"Folder\" menu)?"),
                 name.c_str());

      if ( MDialog_YesNoDialog(msg,
                               this,
                               _("Filter is not used"),
                               false /* yes default */,
                               GetPersMsgBoxName(M_MSGBOX_FILTER_NOT_USED_YET)
                              ) )
      {
         MFolder_obj folder(MDialog_FolderChoose(this));
         if ( folder )
         {
            // activate the filter for this folder
            folder->AddFilter(name);
         }
      }
      //else: leave the filter unused
   }
   //else: it already exists in the message box

   m_hasChanges = true;

   DoUpdate();
}

void
wxFiltersDialog::OnEditFiter(wxCommandEvent &event)
{
   String name = m_lboxFilters->GetStringSelection();
   CHECK_RET( !!name, "must have selection in the listbox" );

   if ( EditFilter(name, this) )
   {
      // filter changed
      m_hasChanges = true;
   }
}

void
wxFiltersDialog::OnDeleteFiter(wxCommandEvent &event)
{
   String name = m_lboxFilters->GetStringSelection();
   CHECK_RET( !!name, "must have selection in the listbox" );

   if ( MFilter::DeleteFromProfile(name) )
   {
      m_hasChanges = true;

      m_lboxFilters->Delete(m_lboxFilters->GetSelection());

      DoUpdate();
   }
}

void
wxFiltersDialog::DoUpdate()
{
   bool hasSel = m_lboxFilters->GetSelection() != -1;

   // Add is always enabled
   m_btnEdit->Enable(hasSel);
   m_btnDelete->Enable(hasSel);
}

bool
wxFiltersDialog::TransferDataToWindow()
{
   wxArrayString allFilters = Profile::GetAllFilters();
   size_t count = allFilters.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      m_lboxFilters->Append(allFilters[n]);
   }

   DoUpdate();

   return true;
}

bool
wxFiltersDialog::TransferDataFromWindow()
{
   // we don't do anything here as everything is done in the button handlers
   // anyhow...

   return true;
}

// ----------------------------------------------------------------------------
// wxFolderFiltersDialog - dialog to configure filters to use for this folder
// ----------------------------------------------------------------------------

class wxFolderFiltersDialog : public wxSelectionsOrderDialog
{
public:
   wxFolderFiltersDialog(MFolder *folder, wxWindow *parent);
   virtual ~wxFolderFiltersDialog();

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   // event handlers
   void OnAddButton(wxCommandEvent& event);
   void OnEditFiter(wxCommandEvent& event);
   void OnDeleteFiter(wxCommandEvent& event);

   void OnUpdateEditBtn(wxUpdateUIEvent& event);

private:
   // construct the caption for the dialog
   static String GetCaption(MFolder *folder)
   {
      return wxString::Format(_("Configure filters for '%s'"),
                              folder->GetName().c_str());
   }

   // the folder which we are working with
   MFolder *m_folder;

   // all filters for this folder: initially, the old settings, after the
   // dialog is dismissed the new ones, use HasChanges() to determine if they
   // are different from the old settings
   wxArrayString m_filters;

   // the buttons to add/edit/delete a filter
   wxButton *m_btnAdd,
            *m_btnEdit,
            *m_btnDelete;

   DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(wxFolderFiltersDialog, wxSelectionsOrderDialog)
   EVT_BUTTON(Button_Add, wxFolderFiltersDialog::OnAddButton)
   EVT_BUTTON(Button_Edit, wxFolderFiltersDialog::OnEditFiter)
   EVT_BUTTON(Button_Delete, wxFolderFiltersDialog::OnDeleteFiter)

   EVT_UPDATE_UI(Button_Edit, wxFolderFiltersDialog::OnUpdateEditBtn)
   EVT_UPDATE_UI(Button_Delete, wxFolderFiltersDialog::OnUpdateEditBtn)

   EVT_LISTBOX_DCLICK(-1, wxFolderFiltersDialog::OnEditFiter)
END_EVENT_TABLE()

wxFolderFiltersDialog::wxFolderFiltersDialog(MFolder *folder, wxWindow *parent)
                     : wxSelectionsOrderDialog(parent,
                                               _("Select the filters to use:"),
                                               GetCaption(folder),
                                               "FolderFilters")
{
   m_folder = folder;
   m_folder->IncRef();

   // we hijack the dialog layout here by inserting some additional controls
   // into it
   wxLayoutConstraints *c;

   // assume there is only one of them
   wxWindow *boxTop = FindWindow(wxStaticBoxNameStr);
   wxCHECK_RET( boxTop, "can't find static box in wxFolderFiltersDialog" );

   wxWindow *btnOk = FindWindow(wxID_OK);
   wxCHECK_RET( btnOk, "can't find ok button in wxFolderFiltersDialog" );

   // create additional pane below the check list box with existing filters
   wxStaticBox *boxBottom = new wxStaticBox(this, -1, _("Instructions:"));
   c = new wxLayoutConstraints();
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->height.Absolute(5*hBtn);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(btnOk, wxTop, 4*LAYOUT_Y_MARGIN);
   boxBottom->SetConstraints(c);

   // change the top panes constraint
   c = new wxLayoutConstraints(*(boxTop->GetConstraints()));
   c->bottom.SameAs(boxBottom, wxTop, 4*LAYOUT_Y_MARGIN);
   boxTop->SetConstraints(c);

   // create controls inside the bottom static box
   wxStaticText *statText = new wxStaticText(this, -1,
         _("You may check the filters you want to use in\n"
           "the list above. Additionally, use the buttons\n"
           "to the right of it to choose the filter priority.\n"
           "Use the buttons to the right to create, delete\n"
           "or modify a filter."));
   c = new wxLayoutConstraints();
   c->left.SameAs(boxBottom, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.SameAs(boxBottom, wxTop, 4*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(boxBottom, wxBottom, -2*LAYOUT_Y_MARGIN);
   statText->SetConstraints(c);

   m_btnDelete = new wxButton(this, Button_Delete, _("&Delete"));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->centreY.SameAs(boxBottom, wxCentreY);
   c->right.SameAs(boxBottom, wxRight, 2*LAYOUT_X_MARGIN);
   m_btnDelete->SetConstraints(c);

   m_btnAdd = new wxButton(this, Button_Add, _("&Insert..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(m_btnDelete, wxRight);
   c->bottom.Above(m_btnDelete, -LAYOUT_Y_MARGIN);
   m_btnAdd->SetConstraints(c);

   m_btnEdit = new wxButton(this, Button_Edit, _("&Edit..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(m_btnDelete, wxRight);
   c->top.Below(m_btnDelete, LAYOUT_Y_MARGIN);
   m_btnEdit->SetConstraints(c);

   // increase min horz size
   SetDefaultSize(5*wBtn, 19*hBtn);
}

wxFolderFiltersDialog::~wxFolderFiltersDialog()
{
   m_folder->DecRef();
}

bool wxFolderFiltersDialog::TransferDataToWindow()
{
   // first put all the filters we already have
   m_filters = m_folder->GetFilters();
   size_t n, count = m_filters.GetCount();
   for ( n = 0; n < count; n++ )
   {
      m_checklstBox->Append(m_filters[n]);
      m_checklstBox->Check(n);
   }

   // then all the other ones
   wxArrayString allFilters = Profile::GetAllFilters();
   count = allFilters.GetCount();
   for ( n = 0; n < count; n++ )
   {
      const wxString& f = allFilters[n];
      if ( m_filters.Index(f) == wxNOT_FOUND )
         m_checklstBox->Append(f);
      //else: we already have it
   }

   return TRUE;
}

bool wxFolderFiltersDialog::TransferDataFromWindow()
{
   wxArrayString filters;
   size_t count = m_checklstBox->GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( m_checklstBox->IsChecked(n) )
      {
         filters.Add(m_checklstBox->GetString(n));
      }
   }

   if ( filters == m_filters )
   {
      // nothing changed for us
      m_hasChanges = false;
   }
   else
   {
      m_hasChanges = true;
      m_filters = filters;

      m_folder->SetFilters(m_filters);
   }

   return TRUE;
}

void wxFolderFiltersDialog::OnAddButton(wxCommandEvent& event)
{
   if ( event.GetEventObject() == m_btnAdd )
   {
      wxArrayString allFiltersOld = Profile::GetAllFilters();
      String name = CreateNewFilter(this);
      if ( !name.empty() )
      {
         int idx = m_checklstBox->FindString(name);
         if ( idx == wxNOT_FOUND )
         {
            // insert the new filter before the current selection
            idx = m_checklstBox->GetSelection();
            if ( idx == -1 )
            {
               // maybe it was empty?
               idx = 0;
            }

            m_checklstBox->Insert(name, idx);
         }
         //else: we already had it

         m_checklstBox->Check(idx);
      }
      //else: cancelled
   }
   else
   {
      // not our event
      event.Skip();
   }
}

void wxFolderFiltersDialog::OnDeleteFiter(wxCommandEvent& event)
{
   int sel = m_checklstBox->GetSelection();
   if ( sel == -1 )
   {
      event.Skip();

      return;
   }

   String name = m_checklstBox->GetString(sel);
   MFilter::DeleteFromProfile(name);

   m_checklstBox->Delete(sel);
}

void wxFolderFiltersDialog::OnEditFiter(wxCommandEvent& event)
{
   String name = m_checklstBox->GetStringSelection();
   if ( name.empty() )
   {
      event.Skip();
   }
   else
   {
      (void)EditFilter(name, this);
   }
}

void wxFolderFiltersDialog::OnUpdateEditBtn(wxUpdateUIEvent& event)
{
   event.Enable( m_checklstBox->GetSelection() != -1 );
}

// ----------------------------------------------------------------------------
// wxQuickFilterDialog
// ----------------------------------------------------------------------------

class wxQuickFilterDialog : public wxManuallyLaidOutDialog
{
public:
   wxQuickFilterDialog(MFolder *folder,
                       const String& from,
                       const String& subject,
                       wxWindow *parent);

   virtual ~wxQuickFilterDialog();

   virtual bool TransferDataFromWindow();

protected:
   void DoUpdateUI() { m_action->UpdateUI(); }

   void OnChoice(wxCommandEvent&) { DoUpdateUI(); }

private:
   // GUI controls
   wxCheckBox *m_checkSubj,
              *m_checkFrom;
   wxTextCtrl *m_textSubj,
              *m_textFrom;

   OneActionControl *m_action;

   MFolder *m_folder;

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxQuickFilterDialog, wxManuallyLaidOutDialog)
   EVT_CHOICE(-1, wxQuickFilterDialog::OnChoice)
END_EVENT_TABLE()

wxQuickFilterDialog::wxQuickFilterDialog(MFolder *folder,
                                         const String& from,
                                         const String& subject,
                                         wxWindow *parent)
                   : wxManuallyLaidOutDialog(parent,
                                             _("Create quick filter"),
                                             "QuickFilter")
{
   m_folder = folder;
   m_folder->IncRef();

   wxLayoutConstraints *c;

   wxStaticBox *box = CreateStdButtonsAndBox(_("Apply this filter"),
                                             FALSE, MH_DIALOG_QUICK_FILTERS);

   wxStaticText *msg = new wxStaticText
                           (
                            this, -1,
                            _("Only if:")
                           );

   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   wxArrayString labels;
   labels.Add(_("the message was sent from"));
   labels.Add(_("the message subject contains"));
   long widthMax = GetMaxLabelWidth(labels, this) + 4*LAYOUT_X_MARGIN;

   m_textFrom = new wxTextCtrl(this, -1, from);
   c = new wxLayoutConstraints;
   c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN + widthMax + LAYOUT_X_MARGIN);
   c->height.AsIs();
   m_textFrom->SetConstraints(c);

   m_checkFrom = new wxCheckBox(this, -1, labels[0]);
   c = new wxLayoutConstraints;
   c->centreY.SameAs(m_textFrom, wxCentreY);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   m_checkFrom->SetConstraints(c);

   m_textSubj = new wxTextCtrl(this, -1, subject);
   c = new wxLayoutConstraints;
   c->top.Below(m_textFrom, LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN + widthMax + LAYOUT_X_MARGIN);
   c->height.AsIs();
   m_textSubj->SetConstraints(c);

   m_checkSubj = new wxCheckBox(this, -1, labels[1]);
   c = new wxLayoutConstraints;
   c->centreY.SameAs(m_textSubj, wxCentreY);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   m_checkSubj->SetConstraints(c);

   msg = new wxStaticText(this, -1, _("And then:"));
   c = new wxLayoutConstraints;
   c->top.Below(m_checkSubj, 3*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   m_action = new OneActionControl(this);

   wxWindow *last = msg;
   m_action->LayoutControls(&last, 2*LAYOUT_X_MARGIN, 3*LAYOUT_X_MARGIN);
   SetDefaultSize(8*wBtn, 10*hBtn);

   DoUpdateUI();
}

bool wxQuickFilterDialog::TransferDataFromWindow()
{
   MFDialogSettings *settings = NULL;
   String name,
          from = m_textFrom->GetValue(),
          subj = m_textSubj->GetValue();

   if ( m_checkFrom->GetValue() && !!from )
   {
      if ( !settings )
         settings = MFDialogSettings::Create();

      settings->AddTest(ORC_L_And, false, ORC_T_Contains, ORC_W_From, from);

      if ( !!name )
         name << ' ';
      name << _("from ") << from;
   }
   if ( m_checkSubj->GetValue() && !!subj )
   {
      if ( !settings )
         settings = MFDialogSettings::Create();

      settings->AddTest(ORC_L_And, false, ORC_T_Contains, ORC_W_Subject, subj);

      if ( !!name )
         name << ' ';
      name << _("subject ") << subj;
   }

   if ( !settings )
   {
      MDialog_ErrorMessage(_("Please specify either subject or address "
                             "to create the filter."), this);

      return false;
   }

   settings->SetAction(m_action->GetAction(), m_action->GetArgument());

   name.Prepend(_("quick filter "));

   MFilter_obj filter(name);
   MFilterDesc fd;
   fd.SetName(name);
   fd.Set(settings);
   filter->Set(fd);

   m_folder->AddFilter(name);

   return true;
}

wxQuickFilterDialog::~wxQuickFilterDialog()
{
   m_folder->DecRef();
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static bool FilterExists(const String& name)
{
   wxArrayString allFilters = Profile::GetAllFilters();
   size_t count = allFilters.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( allFilters[n] == name )
         return true;
   }

   return false;
}

static String CreateNewFilter(wxWindow *parent)
{
   String name;
   MFilterDesc fd;
   if ( ConfigureFilter(&fd, parent) )
   {
      name = fd.GetName();

      // check if the filter with this name already exists
      if ( FilterExists(name) )
      {
         String msg;
         msg.Printf(_("The filter '%s' already exists, do you want "
                      "to replace it?"), name.c_str());
         if ( !MDialog_YesNoDialog(msg, parent, _("Replace filter?"),
                                   false,
                                   GetPersMsgBoxName(M_MSGBOX_FILTER_REPLACE)) )
         {
            return "";
         }
      }

      // create the new filter
      MFilter_obj filter(name);
      filter->Set(fd);
   }
   //else: cancelled

   return name;
}

static bool EditFilter(const String& name, wxWindow *parent)
{
   MFilter_obj filter(name);
   CHECK( filter, false, "filter unexpectedly missing" );

   MFilterDesc fd = filter->GetDesc();
   if ( !ConfigureFilter(&fd, parent) )
   {
      // cancelled
      return false;
   }

   filter->Set(fd);

   return true;
}

// ----------------------------------------------------------------------------
// public function
// ----------------------------------------------------------------------------

extern
bool ConfigureAllFilters(wxWindow *parent)
{
   wxFiltersDialog dlg(parent);
   return dlg.ShowModal() == wxID_OK && dlg.HasChanges();
}

extern
bool ConfigureFiltersForFolder(MFolder *folder, wxWindow *parent)
{
   wxFolderFiltersDialog dlg(folder, parent);
   return dlg.ShowModal() == wxID_OK && dlg.HasChanges();
}

extern
bool ConfigureFilter(MFilterDesc *filterDesc,
                     wxWindow *parent)
{
   wxOneFilterDialog dlg(filterDesc, parent);
   return dlg.ShowModal() == wxID_OK && dlg.HasChanges();
}

extern bool CreateQuickFilter(MFolder *folder,
                              const String& from,
                              const String& subject,
                              wxWindow *parent)
{
   wxQuickFilterDialog dlg(folder, from, subject, parent);
   return dlg.ShowModal() == wxID_OK;
}

