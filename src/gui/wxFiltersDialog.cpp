///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxFiltersDialog.cpp - GUI for editing mail filters
// Purpose:     implements all the dialogs for configuring filters
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1999
// CVS-ID:      $Id$
// Copyright:   (c) 1999-2001 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

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
#   include "guidef.h"
#   include "strutil.h"
#   include "Profile.h"
#   include "MApplication.h"
#   include "Profile.h"
#   include "strutil.h"
#   include "MHelp.h"

#   include <wx/stattext.h>        // for wxStaticText
#   include <wx/layout.h>
#   include <wx/checklst.h>
#   include <wx/statbox.h>
#endif // USE_PCH

#include "MFilter.h"
#include "MFolder.h"
#include "MailFolder.h"
#include "MModule.h"

#include "gui/wxBrowseButton.h"
#include "gui/wxDialogLayout.h"
#include "gui/wxFiltersDialog.h"
#include "gui/wxSelectionDlg.h"

class WXDLLEXPORT wxWindow;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_FILTER_NOT_USED_YET;
extern const MPersMsgBox *M_MSGBOX_FILTER_CONFIRM_OVERWRITE;
extern const MPersMsgBox *M_MSGBOX_FILTER_REPLACE;

// ---------------------------------------------------------------------------
// macros to allow choice labels in other than enum order
// ---------------------------------------------------------------------------

/// Run in constructor to eliminate unimplemented operations from label list
/// arrayL = Label array, arrayS = Swap array, count = count of arrays
/// implemented = function which returns true if enum is implemented
#define SKIP_UNIMPLEMENTED_LABELS( arrayL, arrayS, count, implemented ) \
 { \
   size_t from, to; \
   for ( from = to = 0; from < count; ++from ) \
   { \
      if ( implemented(arrayS[from]) ) \
      { \
         if ( to != from ) \
         { \
            arrayL[to] = arrayL[from]; \
            arrayS[to] = arrayS[from]; \
         } \
         ++to; \
      } \
   } \
   count = to; \
}

/// Define function T##_fromSelect to convert selection value to enum
#define ENUM_fromSelect( T, arrayS, count, default ) \
static \
T T##_fromSelect ( size_t value ) \
{ \
   CHECK(value < count, default, _T(#T " out of range for choice")); \
   return arrayS[value]; \
}

/// Define function T##_toSelect to convert enum to selection value
#define ENUM_toSelect( T, arrayS, count, default ) \
static \
int T##_toSelect ( T value ) \
{ \
   size_t i; \
   for ( i = 0; i < count; ++i ) \
   { \
      if ( arrayS[i] == value ) \
         return i; \
   } \
   CHECK(false, default, _T("choice not implemented in " #T)); \
}

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   // for wxAllFiltersDialog and wxFolderFiltersDialog
   Button_Add = 100,
   Button_Delete,
   Button_Edit,
   Button_Rename,
   Button_Copy,

   Button_MoreTests,
   Button_LessTests,

   // for wxOneFilterDialog
   Text_Program = 200
};

// data for the OneCritControl

static
wxString ORC_Types[] =
{
   // Must be in the same order as ORC_T_Swap[]
   gettext_noop("Always"),              // ORC_T_Always
   gettext_noop("Match"),               // ORC_T_Match
   gettext_noop("Match Case"),          // ORC_T_MatchC
   gettext_noop("Contains"),            // ORC_T_Contains
   gettext_noop("Contains Case"),       // ORC_T_ContainsC
   gettext_noop("Match RegExp"),        // ORC_T_MatchRegEx
   gettext_noop("Match RegExp Case"),   // ORC_T_MatchRegExC
   gettext_noop("Larger than"),         // ORC_T_LargerThan
   gettext_noop("Smaller than"),        // ORC_T_SmallerThan
   gettext_noop("Older than"),          // ORC_T_OlderThan
   gettext_noop("Newer than"),          // ORC_T_NewerThan
   gettext_noop("Seems to be SPAM"),    // ORC_T_IsSpam
   gettext_noop("Python"),              // ORC_T_Python
   gettext_noop("Score above"),         // ORC_T_ScoreAbove
   gettext_noop("Score below"),         // ORC_T_ScoreBelow
   gettext_noop("Sent to me"),          // ORC_T_IsToMe
   gettext_noop("Sent from me"),        // ORC_T_IsFromMe
   gettext_noop("Flag is Set")          // ORC_T_HasFlag
};

static const
size_t ORC_TypesCount  = WXSIZEOF(ORC_Types);
static
size_t ORC_TypesCountS = WXSIZEOF(ORC_Types);

static 
MFDialogTest ORC_T_Swap[] =
{
   // Must be in the same order as ORC_Types[]
   ORC_T_Always,
   ORC_T_Match,
   ORC_T_MatchC,
   ORC_T_Contains,
   ORC_T_ContainsC,
   ORC_T_MatchRegEx,
   ORC_T_MatchRegExC,
   ORC_T_LargerThan,
   ORC_T_SmallerThan,
   ORC_T_OlderThan,
   ORC_T_NewerThan,
   ORC_T_IsSpam,
   ORC_T_Python,
   ORC_T_ScoreAbove,
   ORC_T_ScoreBelow,
   ORC_T_IsToMe,
   ORC_T_IsFromMe,
   ORC_T_HasFlag
};
ENUM_fromSelect(MFDialogTest, ORC_T_Swap, ORC_TypesCountS, ORC_T_Always)
ENUM_toSelect(  MFDialogTest, ORC_T_Swap, ORC_TypesCountS, ORC_T_Always)

static const
wxString ORC_Msg_Flag[] =
{
   gettext_noop("Unseen"),    // ORC_MF_Unseen (inverted)
   gettext_noop("Deleted"),   // ORC_MF_Deleted
   gettext_noop("Answered"),  // ORC_MF_Answered
// gettext_noop("Searched"),  // ORC_MF_Searched
   gettext_noop("Important"), // ORC_MF_Important
   gettext_noop("Recent")     // ORC_MF_Recent
};

static const
size_t ORC_Msg_Flag_Count = WXSIZEOF(ORC_Msg_Flag);

static
wxString ORC_Where[] =
{
   // Must be in the same order as ORC_W_Swap[]
   gettext_noop("in from"),             // ORC_W_From
   gettext_noop("in to"),               // ORC_W_To
   gettext_noop("in subject"),          // ORC_W_Subject
   gettext_noop("in sender"),           // ORC_W_Sender
   gettext_noop("in any recipient"),    // ORC_W_Recipients
   gettext_noop("in header"),           // ORC_W_Header
   gettext_noop("in body"),             // ORC_W_Body
   gettext_noop("in message")           // ORC_W_Message
};

static const
size_t ORC_WhereCount  = WXSIZEOF(ORC_Where);
static
size_t ORC_WhereCountS = WXSIZEOF(ORC_Where);

static
MFDialogTarget ORC_W_Swap[] =
{
   // Must be in the same order as ORC_Where[]
   ORC_W_From,
   ORC_W_To,
   ORC_W_Subject,
   ORC_W_Sender,
   ORC_W_Recipients,
   ORC_W_Header,
   ORC_W_Body,
   ORC_W_Message
};
ENUM_fromSelect(MFDialogTarget, ORC_W_Swap, ORC_WhereCountS, ORC_W_Subject)
ENUM_toSelect(  MFDialogTarget, ORC_W_Swap, ORC_WhereCountS, ORC_W_Subject)

static const
wxString ORC_Logical[] =
{
   gettext_noop("or"),  // ORC_L_Or
   gettext_noop("and")  // ORC_L_And
};

static const
size_t ORC_LogicalCount = WXSIZEOF(ORC_Logical);

// and for OneActionControl

static
wxString OAC_Types[] =
{
   // Must be in the same order as OAC_T_Swap[]
   gettext_noop("Move to"),             // OAC_T_MoveTo
   gettext_noop("Copy to"),             // OAC_T_CopyTo
   gettext_noop("Delete"),              // OAC_T_Delete
   gettext_noop("Expunge"),             // OAC_T_Expunge
   gettext_noop("Zap"),                 // OAC_T_Zap
   gettext_noop("MessageBox"),          // OAC_T_MessageBox
   gettext_noop("Log Entry"),           // OAC_T_LogEntry
   gettext_noop("Python"),              // OAC_T_Python
   gettext_noop("Set Score"),           // OAC_T_SetScore
   gettext_noop("Add to Score"),        // OAC_T_AddScore
   gettext_noop("Set Colour"),          // OAC_T_SetColour
   gettext_noop("Print"),               // OAC_T_Print
   gettext_noop("Set Flag"),            // OAC_T_SetFlag
   gettext_noop("Clear Flag"),          // OAC_T_ClearFlag
   gettext_noop("Do nothing"),          // OAC_T_NOP
};

static const
size_t OAC_TypesCount  = WXSIZEOF(OAC_Types);
static
size_t OAC_TypesCountS = WXSIZEOF(OAC_Types);

static
MFDialogAction OAC_T_Swap[] =
{
   // Must be in the same order as OAC_Types[]
   OAC_T_MoveTo,
   OAC_T_CopyTo,
   OAC_T_Delete,
   OAC_T_Expunge,
   OAC_T_Zap,
   OAC_T_MessageBox,
   OAC_T_LogEntry,
   OAC_T_Python,
   OAC_T_SetScore,
   OAC_T_AddScore,
   OAC_T_SetColour,
   OAC_T_Print,
   OAC_T_SetFlag,
   OAC_T_ClearFlag,
   OAC_T_NOP,
};

ENUM_fromSelect(MFDialogAction, OAC_T_Swap, OAC_TypesCountS, OAC_T_MoveTo)
ENUM_toSelect(  MFDialogAction, OAC_T_Swap, OAC_TypesCountS, OAC_T_MoveTo)

static const
wxString OAC_Msg_Flag[] =
{
   gettext_noop("Unseen"),    // OAC_MF_Unseen (inverted)
   gettext_noop("Deleted"),   // OAC_MF_Deleted
   gettext_noop("Answered"),  // OAC_MF_Answered
// gettext_noop("Searched"),  // OAC_MF_Searched
   gettext_noop("Important")  // OAC_MF_Important
// gettext_noop("Recent")     // OAC_MF_Recent (can't set)
};

static const
size_t OAC_Msg_Flag_Count = WXSIZEOF(OAC_Msg_Flag);

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

class OneCritControl;
class OneActionControl;

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
   virtual ~wxOneFilterDialog();

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool HasChanges() const { return !(*m_FilterData == m_OriginalFilterData);}

   // update the program text from the controls values
   void UpdateProgram();

   // event handlers
   void OnButtonMoreOrLess(wxCommandEvent& event);
   void OnProgramTextUpdate(wxCommandEvent& event);

   void OnText(wxCommandEvent& event);
   void OnChoice(wxCommandEvent&) { UpdateProgram(); }
   void OnCheckBox(wxCommandEvent&) { UpdateProgram(); }

protected:
   void DoUpdateUI();

   void AddOneControl();
   void RemoveOneControl();

   void LayoutControls();

   // helper of TransferDataToWindow() and UpdateProgram()
   bool DoTransferDataFromWindow(MFilterDesc *filterData);

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
   OneCritControl *m_CritControl[MAX_CONTROLS];
   OneActionControl *m_ActionControl;

   wxTextCtrl *m_textProgram;

   wxEnhancedPanel *m_Panel;

private:
   DECLARE_DYNAMIC_CLASS_NO_COPY(wxOneFilterDialog)
   DECLARE_EVENT_TABLE()
};

// abstract because we don't want to create it via wxRTTI
IMPLEMENT_ABSTRACT_CLASS(wxOneFilterDialog, wxManuallyLaidOutDialog)

BEGIN_EVENT_TABLE(wxOneFilterDialog, wxManuallyLaidOutDialog)
   // More/Less button
   EVT_BUTTON(Button_MoreTests, wxOneFilterDialog::OnButtonMoreOrLess)
   EVT_BUTTON(Button_LessTests, wxOneFilterDialog::OnButtonMoreOrLess)

   // changes to the program text
   EVT_TEXT(Text_Program, wxOneFilterDialog::OnProgramTextUpdate)

   // changes to any other controls
   EVT_TEXT(-1, wxOneFilterDialog::OnText)
   EVT_CHOICE(-1, wxOneFilterDialog::OnChoice)
   EVT_CHECKBOX(-1, wxOneFilterDialog::OnCheckBox)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// CritDetailsButton: a helper button used by OneCritControl
// ----------------------------------------------------------------------------

class CritDetailsButton : public wxButton
{
public:
   CritDetailsButton(wxWindow *parent, OneCritControl *ctrl)
      : wxButton(parent, -1, _("Configure &details..."))
      {
         m_ctrl = ctrl;
      }

protected:
   void OnClick(wxCommandEvent& event);

private:
   OneCritControl *m_ctrl;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(CritDetailsButton)
};

BEGIN_EVENT_TABLE(CritDetailsButton, wxButton)
   EVT_BUTTON(-1, CritDetailsButton::OnClick)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// OneCritControl: a control handling one criterium from MFDialogSettings
// ----------------------------------------------------------------------------

class OneCritControl
{
public:
   /// create a new test; previous is the previous test or NULL
   OneCritControl(wxWindow *parent, OneCritControl *previous);
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
      return MFDialogTest_fromSelect(m_Type->GetSelection());
   }

   MFDialogTarget GetTarget() const
   {
      return MFDialogTarget_fromSelect(m_Where->GetSelection());
   }

   String GetArgument()
   {
      MFDialogTest test = GetTest();
      if ( ! FilterTestNeedsArgument(test) )
         return _T(""); // Don't return the value if it won't be used

      switch ( test )
      {
         // spam tests or message flags are decoded separately
         case ORC_T_IsSpam: return m_spam->ToString();
         case ORC_T_HasFlag:
            switch ( m_choiceFlags->GetSelection() )
            {
               case ORC_MF_Unseen:    return _T("U");
               case ORC_MF_Deleted:   return _T("D");
               case ORC_MF_Answered:  return _T("A");
//             case ORC_MF_Searched:  return _T("S");
               case ORC_MF_Important: return _T("*");
               case ORC_MF_Recent:    return _T("R");
            }
            CHECK( false, _T(""), _T("Invalid test message flag") );

         // Argument is used, but not for spam or message flag
         default: return m_Argument->GetValue();
      }
   }

   // enable the controls corresponding to the current m_Type value
   void UpdateUI(wxTextCtrl *textProgram);

   // show the details dialog: called by CritDetailsButton::OnClick()
   void ShowDetails();

   // disable all the controls
   void Disable();

   /// layout the current controls under the window *last
   void LayoutControls(wxWindow **last);
   wxWindow *GetLastCtrl() { return m_Where; }

private:
   // create the spam button and position it correctly
   void CreateSpamButton(const String& rule);

   // initialize m_checkXXX vars(called from CreateSpamButton())
   void InitSpamOptions(const String& rule);

   // the controls which are always shown

   wxCheckBox *m_Not;      // invert the condition if checked
   wxChoice   *m_Logical;  // corresponds to ORC_Logical
   wxChoice   *m_Type;     // corresponds to ORC_Types

   // the controls which are shown for most of ORC_T_XXX values
   wxTextCtrl *m_Argument; // string, number of days or bytes
   wxChoice   *m_Where;    // corresponds to ORC_Where

   // special controls
   wxChoice   *m_choiceFlags; // used for ORC_T_HasFlag
   wxButton   *m_btnSpam;  // configure the ORC_T_IsSpam test

   // the parent for all these controls
   wxWindow   *m_Parent;


   // the spam test data
   RefCounter<SpamOptionManager> m_spam;
};

wxCOMPILE_TIME_ASSERT( ORC_LogicalCount == ORC_L_Max, MismatchInLogicOps );
wxCOMPILE_TIME_ASSERT( ORC_TypesCount == ORC_T_Max, MismatchInTestTypes );
wxCOMPILE_TIME_ASSERT( ORC_Msg_Flag_Count == ORC_MF_Max, MismatchInHasFlag );
wxCOMPILE_TIME_ASSERT( ORC_WhereCount == ORC_W_Max, MismatchInTargets );

OneCritControl::OneCritControl(wxWindow *parent, OneCritControl *previous)
   : m_spam(SpamOptionManager::Create())
{
   m_Parent = parent;

   // don't create it yet as it might be not needed at all, so postpone it
   m_btnSpam = NULL;

   // only create the logical condition (And/Or) control if we have
   // something to combine this one with
   if ( previous )
   {
      // create a separate array with the translated strings
      wxString logicTrans[ORC_TypesCount];
      for ( size_t nLogical = 0; nLogical < ORC_LogicalCount; nLogical++ )
      {
         logicTrans[nLogical] = wxGetTranslation(ORC_Logical[nLogical]);
      }

      m_Logical = new wxChoice(parent, -1, wxDefaultPosition, wxDefaultSize,
                               ORC_LogicalCount, logicTrans);

      // take the value from the preceding control, if any, instead of default
      // as it is usually more convenient (user usually creates filter of the
      // form "foo AND bar AND baz" or "foo OR bar OR baz"...)
      wxChoice *prevLogical = previous->m_Logical;
      m_Logical->SetSelection(prevLogical ? prevLogical->GetSelection()
                              : ORC_L_Or);
   }
   else
      m_Logical = NULL;

   m_Not = new wxCheckBox(parent, -1, _("Not"));

   // create a separate array with the translated strings
   wxString typesTrans[ORC_TypesCount];
   for ( size_t nType = 0; nType < ORC_TypesCountS; nType++ )
   {
      typesTrans[nType] = wxGetTranslation(ORC_Types[nType]);
   }

   m_Type = new wxChoice(parent, -1, wxDefaultPosition,
                         wxDefaultSize, ORC_TypesCountS, typesTrans);

   wxString msgflagsTrans[ORC_Msg_Flag_Count];
   for ( size_t nMsgFlag = 0; nMsgFlag < ORC_Msg_Flag_Count; nMsgFlag++ )
   {
      msgflagsTrans[nMsgFlag] = wxGetTranslation(ORC_Msg_Flag[nMsgFlag]);
   }

   m_choiceFlags = new wxChoice(parent, -1, wxDefaultPosition,
                         wxDefaultSize, ORC_Msg_Flag_Count, msgflagsTrans);
   m_Argument = new wxTextCtrl(parent,-1, _T(""), wxDefaultPosition);

   wxString whereTrans[ORC_WhereCount];
   for ( size_t nWhere = 0; nWhere < ORC_WhereCountS; nWhere++ )
   {
      whereTrans[nWhere] = wxGetTranslation(ORC_Where[nWhere]);
   }

   m_Where = new wxChoice(parent, -1, wxDefaultPosition,
                          wxDefaultSize, ORC_WhereCountS, whereTrans);

   // set up the initial values or the code in UpdateProgram() would complain
   // about invalid values
   m_Type->SetSelection(MFDialogTest_toSelect(ORC_T_Contains));
   m_choiceFlags->SetSelection(ORC_MF_Unseen);
   m_Where->SetSelection(MFDialogTarget_toSelect(ORC_W_Subject));
}

OneCritControl::~OneCritControl()
{
   // we need a destructor to clean up our controls because even though they
   // would be deleted by their parent later, it would mean that
   // adding/removing a test would result in "temporarily" leaking memory
   delete m_Logical;
   delete m_Not;
   delete m_Type;

   delete m_Argument;
   delete m_Where;

   delete m_choiceFlags;
   delete m_btnSpam;
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
      c->centreY.SameAs(m_Logical, wxCentreY);
      c->height.AsIs();
      m_Not->SetConstraints(c);
   }
   else
      m_Not->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.RightOf(m_Not, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->centreY.SameAs(m_Not, wxCentreY);
   c->height.AsIs();
   m_Type->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->right.SameAs(m_Parent, wxRight, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->centreY.SameAs(m_Not, wxCentreY);
   c->height.AsIs();
   m_Where->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.RightOf(m_Type, LAYOUT_X_MARGIN);
   c->right.LeftOf(m_Where, LAYOUT_X_MARGIN);
   c->centreY.SameAs(m_Not, wxCentreY);
   c->height.AsIs();
   m_Argument->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->left.RightOf(m_Type, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->centreY.SameAs(m_Not, wxCentreY);
   c->height.AsIs();
   m_choiceFlags->SetConstraints(c);

   *last = m_Where;
}

void
OneCritControl::CreateSpamButton(const String& rule)
{
   ASSERT_MSG( !m_btnSpam, _T("CreateSpamButton() called twice?") );

   InitSpamOptions(rule);

   m_btnSpam = new CritDetailsButton(m_Parent, this);

   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->left.RightOf(m_Type, LAYOUT_X_MARGIN);
   c->right.SameAs(m_Parent, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(m_Not, wxCentreY);
   c->height.AsIs();
   m_btnSpam->SetConstraints(c);

   m_Parent->Layout();
}

void
OneCritControl::UpdateUI(wxTextCtrl *textProgram)
{
   // decide what to show and enable
   MFDialogTest test   = GetTest();
   bool enable_isspam  = test == ORC_T_IsSpam;
   bool enable_msgflag = test == ORC_T_HasFlag;
   bool enable_arg     = ! enable_isspam && ! enable_msgflag &&
                         FilterTestNeedsArgument(test);
   bool enable_target  = FilterTestNeedsTarget(test);

   // disable everything if test not implemented
   if ( ! FilterTestImplemented(test) )
   {
      enable_msgflag = false;
      enable_arg     = false;
      enable_target  = false;
      enable_isspam  = false;
   }

   m_choiceFlags->Show(enable_msgflag);
   m_choiceFlags->Enable(enable_msgflag);
   m_Argument->Show(enable_arg);
   m_Argument->Enable(enable_arg);
   m_Where->Show(enable_target);
   m_Where->Enable(enable_target);
   if ( enable_isspam && ! m_btnSpam )
      CreateSpamButton(textProgram->GetValue());
   if ( m_btnSpam )
   {
      m_btnSpam->Show(enable_isspam);
      m_btnSpam->Enable(enable_isspam);
   }
}

void
OneCritControl::Disable()
{
   m_Not->Disable();
   m_Type->Disable();
   m_choiceFlags->Disable();
   m_Argument->Disable();
   m_Where->Disable();
   if ( m_Logical )
      m_Logical->Disable();
}

void
OneCritControl::SetValues(const MFDialogSettings& settings, size_t n)
{
   MFDialogLogical logical = settings.GetLogical(n);
   if ( m_Logical && logical != ORC_L_None )
      m_Logical->SetSelection(logical);

   MFDialogTest test = settings.GetTest(n);
   m_Not->SetValue(settings.IsInverted(n));
   m_Type->SetSelection(MFDialogTest_toSelect(test));
   m_Where->SetSelection(MFDialogTarget_toSelect(settings.GetTestTarget(n)));
   String argument = settings.GetTestArgument(n);
   if ( test == ORC_T_HasFlag )
   {
      MFDialogHasFlag flag = ORC_MF_Illegal; // silent the compiler warning
      if      ( argument == _T("U") ) flag = ORC_MF_Unseen;
      else if ( argument == _T("D") ) flag = ORC_MF_Deleted;
      else if ( argument == _T("A") ) flag = ORC_MF_Answered;
//    else if ( argument == _T("S") ) flag = ORC_MF_Searched;
      else if ( argument == _T("*") ) flag = ORC_MF_Important;
      else if ( argument == _T("R") ) flag = ORC_MF_Recent;
      else
      {
         CHECK_RET( false, _T("Invalid test message flag character") );
      }

      m_choiceFlags->SetSelection(flag);
   }
   else
      m_Argument->SetValue(argument);
}

// ----------------------------------------------------------------------------
// spam details dialog code
// ----------------------------------------------------------------------------

void
OneCritControl::InitSpamOptions(const String& /* rule */)
{
   CHECK_RET( m_Argument, _T("no argument control in the spam test?") );

   m_spam->FromString(m_Argument->GetValue());
}

void
OneCritControl::ShowDetails()
{
   if ( m_spam->ShowDialog(GetFrame(m_Parent)) )
   {
      wxOneFilterDialog *dlg =
         GET_PARENT_OF_CLASS(m_Parent, wxOneFilterDialog);
      CHECK_RET( dlg, _T("should be a child of wxOneFilterDialog") );

      dlg->UpdateProgram();
   }
   //else: cancelled
}

void CritDetailsButton::OnClick(wxCommandEvent& WXUNUSED(event))
{
   m_ctrl->ShowDetails();
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
      MFDialogAction action = settings.GetAction();
      String argument = settings.GetActionArgument();
      m_Type->SetSelection(MFDialogAction_toSelect(action));
      if ( FilterActionMsgFlag(action) )
      {
         MFDialogSetFlag flag = OAC_MF_Illegal; // silent the compiler warning
         if      ( argument == _T("U") ) flag = OAC_MF_Unseen;
         else if ( argument == _T("D") ) flag = OAC_MF_Deleted;
         else if ( argument == _T("A") ) flag = OAC_MF_Answered;
//       else if ( argument == _T("S") ) flag = OAC_MF_Searched;
         else if ( argument == _T("*") ) flag = OAC_MF_Important;
      // else if ( argument == _T("R") ) flag = OAC_MF_Recent; // can't set
         else
         {
            CHECK_RET( false, _T("Invalid action message flag character") );
         }
         m_choiceFlags->SetSelection(flag);
      }
      else
         m_Argument->SetValue(argument);
   }

   /// get the action
   MFDialogAction GetAction() const
      { return MFDialogAction_fromSelect(m_Type->GetSelection()); }

   /// get the action argument
   String GetArgument() const
   {
      MFDialogAction action = GetAction();
      if ( ! FilterActionNeedsArg(action) )
         return _T(""); // Don't return the value if it won't be used
 
      // message flags are decoded separately
      if ( FilterActionMsgFlag(GetAction()) )
      {
         switch ( m_choiceFlags->GetSelection() )
         {
            case OAC_MF_Unseen:    return _T("U");
            case OAC_MF_Deleted:   return _T("D");
            case OAC_MF_Answered:  return _T("A");
//          case OAC_MF_Searched:  return _T("S");
            case OAC_MF_Important: return _T("*");
         // case OAC_MF_Recent:    return _T("R"); // can't set
         }
         CHECK( false, _T(""), _T("Invalid action message flag") );
      }

      // Argument is used, but not for message flag
      return m_Argument->GetValue();
   }

   void UpdateUI(void);
   void Hide()
   {
      m_Type->Hide();
      m_Argument->Hide();
      m_choiceFlags->Hide();
      m_btnFolder->Hide();
      m_btnColour->Hide();
   }
   void Disable()
   {
      m_Type->Disable();
      m_Argument->Disable();
      m_choiceFlags->Disable();
      m_btnFolder->Disable();
      m_btnColour->Disable();
   }

   void LayoutControls(wxWindow **last,
                       int leftMargin = 8*LAYOUT_X_MARGIN,
                       int rightMargin = 2*LAYOUT_X_MARGIN);

private:
   wxChoice             *m_Type;       // Which action to perform
   wxTextCtrl           *m_Argument;   // string, number of days or bytes
   wxChoice             *m_choiceFlags;   // Which message flag to set or clear

   // browse buttons: only one of them is currently shown
   wxFolderBrowseButton *m_btnFolder;
   wxColorBrowseButton  *m_btnColour;

   wxWindow             *m_Parent;     // the parent for all these controls
};

void
OneActionControl::UpdateUI()
{
   MFDialogAction type = GetAction();
   bool enable_msgflag = FilterActionMsgFlag(type);
   bool enable_arg     = ! enable_msgflag && FilterActionNeedsArg(type);
   bool enable_colour  = FilterActionUsesColour(type);
   bool enable_folder  = FilterActionUsesFolder(type);

   // disable everything if action not implemented
   if ( ! FilterActionImplemented(type) )
   {
      enable_msgflag = false;
      enable_arg     = false;
      enable_colour  = false;
      enable_folder  = false;
   }

   m_choiceFlags->Show(enable_msgflag);
   m_choiceFlags->Enable(enable_msgflag);
   m_btnColour->Show(enable_colour);
   m_btnColour->Enable(enable_colour);
   m_btnFolder->Show(enable_folder);
   m_btnFolder->Enable(enable_folder);

   // update the argument *after* updating the browse buttons because their
   // Enable() disables the text control as well if they're disabled and so the
   // text could end up disabled even when it should be enabled
   m_Argument->Show(enable_arg);
   m_Argument->Enable(enable_arg);
}

OneActionControl::OneActionControl(wxWindow *parent)
{
   wxASSERT_MSG( OAC_TypesCount == OAC_T_Max,
                 _T("forgot to update something") );

   m_Parent = parent;

   // create a separate array with the translated strings
   wxString typesTrans[OAC_TypesCount];
   for ( size_t nType = 0; nType < OAC_TypesCountS; nType++ )
   {
      typesTrans[nType] = wxGetTranslation(OAC_Types[nType]);
   }

   m_Type = new wxChoice(parent, -1, wxDefaultPosition, wxDefaultSize,
                         OAC_TypesCountS, typesTrans);

   wxString msgflagsTrans[OAC_Msg_Flag_Count];
   for ( size_t nMsgFlag = 0; nMsgFlag < OAC_Msg_Flag_Count; nMsgFlag++ )
   {
      msgflagsTrans[nMsgFlag] = wxGetTranslation(OAC_Msg_Flag[nMsgFlag]);
   }
   m_choiceFlags = new wxChoice(parent, -1, wxDefaultPosition, wxDefaultSize,
                             OAC_Msg_Flag_Count, msgflagsTrans);

   m_Argument = new wxTextCtrl(parent, -1, _T(""));
   m_btnFolder = new wxFolderBrowseButton(m_Argument, parent);
   m_btnColour = new wxColorBrowseButton(m_Argument, parent);

   // select something or UpdateProgram() would complain about invalid action
   m_Type->SetSelection(MFDialogAction_toSelect(OAC_T_MoveTo));
   m_choiceFlags->SetSelection(OAC_MF_Unseen);
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
   c->left.SameAs(m_Type, wxRight, LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.SameAs(m_Type, wxTop, 0);
   c->height.AsIs();
   m_choiceFlags->SetConstraints(c);

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
                                           _T("OneFilterDialog"))
{
   m_isSimple = true;
   m_initializing = true;
   m_nControls = 0;
   m_FilterData = fd;
   SetAutoLayout( TRUE );
   wxLayoutConstraints *c;

   // Remove unimplemented labels for tests and actions
   SKIP_UNIMPLEMENTED_LABELS( ORC_Types, ORC_T_Swap, ORC_TypesCountS,
                              FilterTestImplemented );
   SKIP_UNIMPLEMENTED_LABELS( OAC_Types, OAC_T_Swap, OAC_TypesCountS,
                              FilterActionImplemented );

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

   if ( m_FilterData->GetName().empty() )
   {
      // new filter
      m_NameCtrl->SetFocus();
   }
   else // existing filter
   {
      // newly created filters can be given any name, of course, but if the
      // filter already exists under some name it can't be renamed as this will
      // break all the folders using it, so disable the control in this case
      m_NameCtrl->Disable();
   }

   // the control allowing to edit directly the filter program
   m_textProgram = new wxTextCtrl(this, Text_Program,
                                  _T(""),
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
   m_ButtonMore = new wxButton(canvas, Button_MoreTests, _("&More"));
   m_ButtonLess = new wxButton(canvas, Button_LessTests, _("&Fewer"));
#if wxUSE_TOOLTIPS
   m_ButtonMore->SetToolTip(_("Add another condition"));
   m_ButtonLess->SetToolTip(_("Remove the last condition"));
#endif // wxUSE_TOOLTIPS

   SetDefaultSize(8*wBtn, 18*hBtn);
   m_Panel->Layout();

   SetFocus();
}

wxOneFilterDialog::~wxOneFilterDialog()
{
   for ( size_t idx = 0; idx < m_nControls; idx++ )
   {
      delete m_CritControl[idx];
   }

   delete m_ActionControl;
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
   }

   Layout();
   m_Panel->Layout();
}

void
wxOneFilterDialog::AddOneControl()
{
   ASSERT_MSG( m_nControls < MAX_CONTROLS, _T("too many filter controls") );

   OneCritControl *prev = m_nControls == 0 ? NULL
                                           : m_CritControl[m_nControls - 1];
   m_CritControl[m_nControls] = new OneCritControl(m_Panel->GetCanvas(), prev);
   m_nControls++;
}


void
wxOneFilterDialog::RemoveOneControl()
{
   ASSERT_MSG( m_nControls > 1, _T("can't remove control -- no more left") );

   m_nControls--;
   delete m_CritControl[m_nControls];
}

void wxOneFilterDialog::UpdateProgram()
{
   // show/create the appropriate controls first
   DoUpdateUI();

   if ( !m_initializing )
   {
      // next update the program text
      MFilterDesc filterData;
      if ( DoTransferDataFromWindow(&filterData) &&
               (filterData != *m_FilterData) )
      {
         *m_FilterData = filterData;
         MFDialogSettings *settings = m_FilterData->GetSettings();

         CHECK_RET( settings,
                    _T("can't update program for a non-simple filter rule!") );

         m_initializing = true;
         m_textProgram->SetValue(settings->WriteRule());
         m_initializing = false;

         settings->DecRef();
      }
   }
}

void
wxOneFilterDialog::DoUpdateUI()
{
   if ( m_isSimple )
   {
      for ( size_t idx = 0; idx < m_nControls; idx++ )
      {
         m_CritControl[idx]->UpdateUI(m_textProgram);
      }

      m_ActionControl->UpdateUI();
      m_ButtonLess->Enable(m_nControls > 1);
      m_ButtonMore->Enable(m_nControls < MAX_CONTROLS);
   }
}

void
wxOneFilterDialog::OnProgramTextUpdate(wxCommandEvent& /* event */)
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
wxOneFilterDialog::OnText(wxCommandEvent& event)
{
   event.Skip();

   if ( event.GetEventObject() == m_NameCtrl )
   {
      wxWindow *btnOk = FindWindow(wxID_OK);
      CHECK_RET( btnOk, _T("no [Ok] button in the dialog?") );

      // the filter name can't be empty
      btnOk->Enable( !m_NameCtrl->GetValue().empty() );
   }
   else
   {
      UpdateProgram();
   }
}

void
wxOneFilterDialog::OnButtonMoreOrLess(wxCommandEvent &event)
{
   if ( event.GetId() == Button_LessTests )
   {
      RemoveOneControl();

      // make the and/or clause disappear
      UpdateProgram();
   }
   else // Button_MoreTests
   {
      AddOneControl();

      // we must hide/show the new test controls and also update the state of
      // the less button
      DoUpdateUI();
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

      m_Panel->Layout();
      Layout();

      m_textProgram->SetValue(m_FilterData->GetProgram());
   }

   // show/enable the controls as needed
   DoUpdateUI();

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

// ============================================================================
// wxAllFiltersDialog - dialog for managing all filter rules in the program
// ============================================================================

// ----------------------------------------------------------------------------
// declaration
// ----------------------------------------------------------------------------

class wxAllFiltersDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor & dtor
   wxAllFiltersDialog(wxWindow *parent);

   // transfer data to/from dialog
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // returns TRUE if the format string was changed
   bool HasChanges(void) const { return m_hasChanges; }

   // event handlers
   void OnAddFiter(wxCommandEvent& event);
   void OnCopyFiter(wxCommandEvent& event);
   void OnEditFiter(wxCommandEvent& event);
   void OnRenameFiter(wxCommandEvent& event);
   void OnDeleteFiter(wxCommandEvent& event);
   void OnUpdateButtons(wxUpdateUIEvent& event);

protected:
   bool DoCopyFilter(const wxString& nameOld, const wxString& nameNew);
   bool DoDeleteFilter(const wxString& name);

   // listbox contains the names of all filters
   wxListBox *m_lboxFilters;

   // did anything change?
   bool m_hasChanges;

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxAllFiltersDialog)
};

BEGIN_EVENT_TABLE(wxAllFiltersDialog, wxManuallyLaidOutDialog)
   EVT_BUTTON(Button_Add, wxAllFiltersDialog::OnAddFiter)
   EVT_BUTTON(Button_Copy, wxAllFiltersDialog::OnCopyFiter)
   EVT_BUTTON(Button_Edit, wxAllFiltersDialog::OnEditFiter)
   EVT_BUTTON(Button_Delete, wxAllFiltersDialog::OnDeleteFiter)
   EVT_BUTTON(Button_Rename, wxAllFiltersDialog::OnRenameFiter)

   EVT_UPDATE_UI(Button_Edit, wxAllFiltersDialog::OnUpdateButtons)
   EVT_UPDATE_UI(Button_Copy, wxAllFiltersDialog::OnUpdateButtons)
   EVT_UPDATE_UI(Button_Delete, wxAllFiltersDialog::OnUpdateButtons)
   EVT_UPDATE_UI(Button_Rename, wxAllFiltersDialog::OnUpdateButtons)

   EVT_LISTBOX_DCLICK(-1, wxAllFiltersDialog::OnEditFiter)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxAllFiltersDialog creation
// ----------------------------------------------------------------------------

wxAllFiltersDialog::wxAllFiltersDialog(wxWindow *parent)
                  : wxManuallyLaidOutDialog(parent,
                                            _("Configure Filter Rules"),
                                            _T("FilterDialog"))
{
   m_hasChanges = false;

   wxLayoutConstraints *c;

   wxStaticBox *box = CreateStdButtonsAndBox(_T(""), FALSE,
                                             MH_DIALOG_FILTERS);

   /* This dialog is supposed to look like this:

      +------------------+
      |listbox of        |  [add]
      |existing filters  |  [copy]
      |                  |  [edit]
      |                  |  [rename]
      |                  |  [delete]
      +------------------+

                [help][ok][cancel]
   */

   // start with the buttons

   // position "Edit" in the middle and the others relatively to it
   wxButton *btnEdit = new wxButton(this, Button_Edit, _("&Edit..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->centreY.SameAs(box, wxCentreY);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   btnEdit->SetConstraints(c);

   // above "Edit" we have "Add" and "Copy"
   wxButton *btnCopy = new wxButton(this, Button_Copy, _("&Copy..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(btnEdit, wxRight);
   c->bottom.Above(btnEdit, -LAYOUT_Y_MARGIN);
   btnCopy->SetConstraints(c);

   wxButton *btnAdd = new wxButton(this, Button_Add, _("&Add..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(btnEdit, wxRight);
   c->bottom.Above(btnCopy, -LAYOUT_Y_MARGIN);
   btnAdd->SetConstraints(c);

   // and below - "Rename" and "Delete"
   wxButton *btnRename = new wxButton(this, Button_Rename, _("&Rename..."));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(btnEdit, wxRight);
   c->top.Below(btnEdit, LAYOUT_Y_MARGIN);
   btnRename->SetConstraints(c);

   wxButton *btnDelete = new wxButton(this, Button_Delete, _("&Delete"));
   c = new wxLayoutConstraints;
   c->width.Absolute(wBtn);
   c->height.Absolute(hBtn);
   c->right.SameAs(btnEdit, wxRight);
   c->top.Below(btnRename, LAYOUT_Y_MARGIN);
   btnDelete->SetConstraints(c);

   // create the listbox in the left side of the dialog
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(btnEdit, 3*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_lboxFilters = new wxPListBox(_T("FiltersList"),
                                  this, -1,
                                  wxDefaultPosition, wxDefaultSize,
                                  0, NULL,
                                  wxLB_SORT);
   m_lboxFilters->SetConstraints(c);

   SetDefaultSize(5*wBtn, 11*hBtn);

   m_lboxFilters->SetFocus();
}

// ----------------------------------------------------------------------------
// wxAllFiltersDialog event handlers
// ----------------------------------------------------------------------------

void
wxAllFiltersDialog::OnAddFiter(wxCommandEvent& /* event */)
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
                               M_DLG_YES_DEFAULT,
                               M_MSGBOX_FILTER_NOT_USED_YET
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
}

void
wxAllFiltersDialog::OnCopyFiter(wxCommandEvent& /* event */)
{
   wxString name = m_lboxFilters->GetStringSelection();
   CHECK_RET( !!name, _T("must have selection in the listbox") );

   wxString nameNew = name;
   for ( ;; )
   {
      if ( !MInputBox(&nameNew,
                      _("Copy filter rule"),
                      _("New filter name:"),
                      this) )
      {
         // cancelled
         return;
      }

      if ( nameNew != name )
      {
         break;
      }

      wxLogError(_("Impossible to copy the filter to itself, "
                   "please enter a different name."));
   }

   (void)DoCopyFilter(name, nameNew);
}

void
wxAllFiltersDialog::OnEditFiter(wxCommandEvent& /* event */)
{
   String name = m_lboxFilters->GetStringSelection();
   CHECK_RET( !!name, _T("must have selection in the listbox") );

   if ( EditFilter(name, this) )
   {
      // filter changed
      m_hasChanges = true;
   }
}


#if !defined(NO_RENAME_FILTER_TRAVERSAL)
class RenameAFilterTraversal : public MFolderTraversal
{
private:
  wxString m_name;
  wxString m_nameNew;
public:
   RenameAFilterTraversal(MFolder* folder, wxString name, wxString nameNew) 
      : MFolderTraversal(*folder)
      , m_name(name)
      , m_nameNew(nameNew)
   {  }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         MFolder* folder = MFolder::Get(folderName);
         CHECK( folder, false, _T("RenameAFilterTraversal: NULL folder") );
         wxArrayString filters = folder->GetFilters();
         size_t countFilters = filters.GetCount();
         int foundInThisFolder = 0;
         for ( size_t nFilter = 0; nFilter < countFilters; nFilter++ )
         {
            if (filters[nFilter] == m_name)
            {
                filters[nFilter] = m_nameNew;
                foundInThisFolder = 1;
            }
         }
         if (foundInThisFolder)
         {
             folder->SetFilters(filters);
         }
         return true;
      }
};
#endif

void
wxAllFiltersDialog::OnRenameFiter(wxCommandEvent& /* event */)
{
   wxString name = m_lboxFilters->GetStringSelection();
   CHECK_RET( !!name, _T("must have selection in the listbox") );

   wxString nameNew = name;
   if ( !MInputBox(&nameNew,
                   _("Rename filter rule"),
                   _("New filter name:"),
                   this) ||
        (nameNew == name) )
   {
      // cancelled or name unchanged
      return;
   }

   if ( DoCopyFilter(name, nameNew) )
   {
      // delete the old one now that we copied it successfully
      DoDeleteFilter(name);
   }

#if defined(NO_RENAME_FILTER_TRAVERSAL)
   MDialog_Message(_("Please note that the renamed filter is not used by "
                     "any folder any more, you should probably go to the\n"
                     "\"Folder|Filters\" dialog and change it later.\n"
                     "\n"
                     "(sorry for the inconvenience, this is going to be "
                     "changed in the future version of the program)"),
                   this,
                   _("Filter renamed"),
                   _T("FilterRenameWarn"));
#else
   MFolder_obj folderRoot(_T(""));
   RenameAFilterTraversal traverse(folderRoot, name, nameNew);
   traverse.Traverse();
#endif

}

void
wxAllFiltersDialog::OnDeleteFiter(wxCommandEvent& /* event */)
{
   String name = m_lboxFilters->GetStringSelection();
   CHECK_RET( !!name, _T("must have selection in the listbox") );

   (void)DoDeleteFilter(name);
}

void
wxAllFiltersDialog::OnUpdateButtons(wxUpdateUIEvent& event)
{
   // only enable the buttons working with the current filter if we have any
   event.Enable( m_lboxFilters->GetSelection() != -1 );
}

// ----------------------------------------------------------------------------
// wxAllFiltersDialog operations
// ----------------------------------------------------------------------------

bool
wxAllFiltersDialog::DoCopyFilter(const wxString& nameOld,
                                 const wxString& nameNew)
{
   // caller is supposed to check for this
   CHECK( nameOld != nameNew, false, _T("can't copy filter to itself") );

   // check if the filter with the new name doesn't already exist
   int idx = m_lboxFilters->FindString(nameNew);
   if ( idx != wxNOT_FOUND )
   {
      String msg;
      msg.Printf(_("Filter '%s' already exists, are you sure you want "
                   "to overwrite it with the filter '%s'?"),
                nameOld.c_str(), nameNew.c_str());
      if ( !MDialog_YesNoDialog(msg,
                                this,
                                _("Overwrite filter?"),
                                M_DLG_NO_DEFAULT,
                                M_MSGBOX_FILTER_CONFIRM_OVERWRITE
                               ) )
      {
         // cancelled
         return false;
      }
   }

   if ( !MFilter::Copy(nameOld, nameNew) )
   {
      return false;
   }

   m_hasChanges = true;

   if ( idx == wxNOT_FOUND )
   {
      // insert it just after the original filter, this ensures that when we
      // do a rename (== copy + delete) the filter doesn't jump to the end of
      // the list (as it owuld do if we used simple Append() here)
      idx = m_lboxFilters->FindString(nameOld);

      ASSERT_MSG( idx != wxNOT_FOUND,
                  _T("copied a filter which doesn't exist??") );

      m_lboxFilters->Insert(nameNew, idx + 1);
   }
   //else: we already have the filter with this name in the listbox

   return true;
}

bool
wxAllFiltersDialog::DoDeleteFilter(const wxString& name)
{
   if ( !MFilter::DeleteFromProfile(name) )
      return false;

   m_hasChanges = true;

   m_lboxFilters->Delete(m_lboxFilters->GetSelection());

   return true;
}

// ----------------------------------------------------------------------------
// wxAllFiltersDialog data transfer
// ----------------------------------------------------------------------------

bool
wxAllFiltersDialog::TransferDataToWindow()
{
   wxArrayString allFilters = Profile::GetAllFilters();
   size_t count = allFilters.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      m_lboxFilters->Append(allFilters[n]);
   }

   return true;
}

bool
wxAllFiltersDialog::TransferDataFromWindow()
{
   // we don't do anything here as everything is done in the button handlers
   // anyhow...

   return true;
}

// ============================================================================
// wxFolderFiltersDialog - dialog to configure filters to use for this folder
// ============================================================================

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

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxFolderFiltersDialog)
};

BEGIN_EVENT_TABLE(wxFolderFiltersDialog, wxSelectionsOrderDialog)
   EVT_BUTTON(Button_Add, wxFolderFiltersDialog::OnAddButton)
   EVT_BUTTON(Button_Edit, wxFolderFiltersDialog::OnEditFiter)
   EVT_BUTTON(Button_Delete, wxFolderFiltersDialog::OnDeleteFiter)

   EVT_UPDATE_UI(Button_Delete, wxFolderFiltersDialog::OnUpdateEditBtn)
   EVT_UPDATE_UI(Button_Edit, wxFolderFiltersDialog::OnUpdateEditBtn)

   EVT_LISTBOX_DCLICK(-1, wxFolderFiltersDialog::OnEditFiter)
END_EVENT_TABLE()

wxFolderFiltersDialog::wxFolderFiltersDialog(MFolder *folder, wxWindow *parent)
                     : wxSelectionsOrderDialog(parent,
                                               _("Select the filters to use:"),
                                               GetCaption(folder),
                                               _T("FolderFilters"))
{
   m_folder = folder;
   m_folder->IncRef();

   // we hijack the dialog layout here by inserting some additional controls
   // into it
   wxLayoutConstraints *c;

   // assume there is only one of them
   wxWindow *boxTop = FindWindow(wxStaticBoxNameStr);
   wxCHECK_RET( boxTop, _T("can't find static box in wxFolderFiltersDialog") );

   wxWindow *btnOk = FindWindow(wxID_OK);
   wxCHECK_RET( btnOk, _T("can't find ok button in wxFolderFiltersDialog") );

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
   c->bottom.SameAs(boxBottom, wxBottom, 2*LAYOUT_Y_MARGIN);
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
   enum FilterControl
   {
      Filter_From,
      Filter_Subject,
      Filter_To,
      Filter_Max
   };

   wxQuickFilterDialog(MFolder *folder,
                       const String& from,
                       const String& subject,
                       const String& to,
                       wxWindow *parent);

   virtual ~wxQuickFilterDialog();

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   void DoUpdateUI() { m_action->UpdateUI(); }

   // event handlers
   void OnText(wxCommandEvent& event);
   void OnChoice(wxCommandEvent&) { DoUpdateUI(); }
   void OnUpdateOk(wxUpdateUIEvent& event);

   // add a test if the given checkbox is checked and the text is not empty
   void AddTest(MFDialogSettings **settings,
                wxString *name,
                FilterControl which);

private:
   // GUI controls
   wxCheckBox *m_check[Filter_Max];
   wxTextCtrl *m_text[Filter_Max];

   OneActionControl *m_action;

   MFolder *m_folder;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxQuickFilterDialog)
};

BEGIN_EVENT_TABLE(wxQuickFilterDialog, wxManuallyLaidOutDialog)
   EVT_CHOICE(-1, wxQuickFilterDialog::OnChoice)
   EVT_TEXT(-1, wxQuickFilterDialog::OnText)

   EVT_UPDATE_UI(wxID_OK, wxQuickFilterDialog::OnUpdateOk)
END_EVENT_TABLE()

wxQuickFilterDialog::wxQuickFilterDialog(MFolder *folder,
                                         const String& from,
                                         const String& subject,
                                         const String& to,
                                         wxWindow *parent)
                   : wxManuallyLaidOutDialog(parent,
                                             _("Create quick filter"),
                                             _T("QuickFilter"))
{
   m_folder = folder;
   m_folder->IncRef();

   // this is used in OnText() to check if we had finished with initializing
   // the dialog
   m_action = NULL;

   wxLayoutConstraints *c;

   wxStaticBox *box = CreateStdButtonsAndBox(_("Apply this filter"),
                                             FALSE, MH_DIALOG_QUICK_FILTERS);

   wxStaticText *msg = new wxStaticText
                           (
                            this, -1,
                            _("Only if all of the below conditions are true:")
                           );

   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   wxArrayString labels;
   labels.Add(_("the message was sent &from"));
   labels.Add(_("the message &subject contains"));
   labels.Add(_("the message was sent &to"));
   long widthMax = GetMaxLabelWidth(labels, this) + 4*LAYOUT_X_MARGIN;

   wxArrayString text;
   text.Add(from);
   text.Add(subject);
   text.Add(to);

   for ( size_t n = 0; n < Filter_Max; n++ )
   {
      m_text[n] = new wxTextCtrl(this, -1, text[n]);
      c = new wxLayoutConstraints;

      if ( n == 0 )
      {
         // top control is positioned relatively to the box
         c->top.Below(msg, 2*LAYOUT_Y_MARGIN);
      }
      else
      {
         // others are below the previous control
         c->top.Below(m_text[n - 1], LAYOUT_Y_MARGIN);
      }

      c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
      c->left.SameAs(box, wxLeft,
                     2*LAYOUT_X_MARGIN + widthMax + LAYOUT_X_MARGIN);
      c->height.AsIs();
      m_text[n]->SetConstraints(c);

      m_check[n] = new wxCheckBox(this, -1, labels[n]);
      c = new wxLayoutConstraints;
      c->centreY.SameAs(m_text[n], wxCentreY);
      c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
      c->width.Absolute(widthMax);
      c->height.AsIs();
      m_check[n]->SetConstraints(c);
   }

   msg = new wxStaticText(this, -1, _("&And if they are, then:"));
   c = new wxLayoutConstraints;
   c->top.Below(m_check[Filter_Max - 1], 3*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.AsIs();
   msg->SetConstraints(c);

   m_action = new OneActionControl(this);

   wxWindow *last = msg;
   m_action->LayoutControls(&last, 2*LAYOUT_X_MARGIN, 3*LAYOUT_X_MARGIN);
   SetDefaultSize(8*wBtn, 11*hBtn);

   DoUpdateUI();
}

void wxQuickFilterDialog::AddTest(MFDialogSettings **settings,
                                  wxString *name,
                                  FilterControl which)
{
   // the part of the name of the filter for the specified criterium
   static const wxChar *names[Filter_Max] =
   {
      gettext_noop("from"),
      gettext_noop("subject"),
      gettext_noop("to"),
   };

   // the AddTest() argument for it
   static const MFDialogTarget where[Filter_Max] =
   {
      ORC_W_From,
      ORC_W_Subject,
      ORC_W_Recipients,
   };

   wxString text = m_text[which]->GetValue();
   if ( m_check[which]->GetValue() && !text.empty() )
   {
      if ( !*settings )
         *settings = MFDialogSettings::Create();
      else
         *name << _(" and ");

      (*settings)->AddTest(ORC_L_And, false /* not inverted */,
                           ORC_T_Contains, where[which], text);

      *name << wxGetTranslation(names[which]) << ' ' << text;
   }
}

bool wxQuickFilterDialog::TransferDataToWindow()
{
   // set focus to some control which we're likely to use -- by default it
   // would have been on the "Ok" button but as it's initially disabled, it
   // doesn't make much sense
   m_check[Filter_From]->SetFocus();

   return true;
}

bool wxQuickFilterDialog::TransferDataFromWindow()
{
   MFDialogSettings *settings = NULL;
   String name = _("quick filter ");

   for ( size_t n = 0; n < Filter_Max; n++ )
   {
      AddTest(&settings, &name, (FilterControl)n);
   }

   CHECK( settings, false,
          _T("the [Ok] button not supposed to be enabled in this case") );

   settings->SetAction(m_action->GetAction(), m_action->GetArgument());

   MFilter_obj filter(name);
   MFilterDesc fd;
   fd.SetName(name);
   fd.Set(settings);
   filter->Set(fd);

   // add it to the beginning of the filter list as it should override any
   // other (presumably more generic) filters
   m_folder->PrependFilter(name);

   return true;
}

wxQuickFilterDialog::~wxQuickFilterDialog()
{
   m_folder->DecRef();
}

void wxQuickFilterDialog::OnText(wxCommandEvent& event)
{
   if ( !m_action )
   {
      // the dialog hasn't been fully initialized yet so this change comes from
      // our code and not from user -- ignore it for our purposes here
      return;
   }

   // if the user starts editing the text field, chances are that he wants to
   // use it as the filter condition, so check the corresponding checkbox
   // automatically to save him a few keystrokes/mouse movements
   for ( size_t n = 0; n < Filter_Max; n++ )
   {
      if ( m_text[n] == event.GetEventObject() )
      {
         m_check[n]->SetValue(true);

         return;
      }
   }

   // some other button, e.g. the browse button of the folder entry: ignore it
}

void wxQuickFilterDialog::OnUpdateOk(wxUpdateUIEvent& event)
{
   // only enable the ok button if we have some condition
   bool isEnabled = false;
   for ( size_t n = 0; n < Filter_Max; n++ )
   {
      if ( m_check[n]->GetValue() && !m_text[n]->GetValue().empty() )
      {
         isEnabled = true;

         break;
      }
   }

   event.Enable(isEnabled);
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
                                   M_DLG_NO_DEFAULT,
                                   M_MSGBOX_FILTER_REPLACE) )
         {
            return _T("");
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
   CHECK( filter, false, _T("filter unexpectedly missing") );

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
   wxAllFiltersDialog dlg(parent);
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
                              const String& to,
                              wxWindow *parent)
{
   wxQuickFilterDialog dlg(folder, from, subject, to, parent);

   return dlg.ShowModal() == wxID_OK;
}
