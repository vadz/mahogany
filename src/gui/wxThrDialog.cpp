///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   gui/wxThrDialog.cpp - implements ConfigureThreading()
// Purpose:     implements the dialog for configuring the message threading
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.09.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "Mdefaults.h"
   #include "guidef.h"
   #include "Profile.h"

   #include <wx/layout.h>
   #include <wx/statbox.h>
   #include <wx/stattext.h>             // for wxStaticText
#endif // USE_PCH

#include "gui/wxDialogLayout.h"

// currently we allow the user to specify the RE to apply for subject
// simplification but this possibility is going to be disabled as it is much
// slower than hard coding it (and the speed matters for threading) so we don't
// show these options any more in the dialog
#undef USE_EXTRA_THREADING_OPTIONS

// ----------------------------------------------------------------------------
// options we use
// ----------------------------------------------------------------------------

extern const MOption MP_MSGS_SERVER_THREAD;
extern const MOption MP_MSGS_SERVER_THREAD_REF_ONLY;

#if wxUSE_REGEX
   extern const MOption MP_MSGS_SIMPLIFYING_REGEX;
   extern const MOption MP_MSGS_REPLACEMENT_STRING;
#else // !wxUSE_REGEX
   extern const MOption MP_MSGS_REMOVE_LIST_PREFIX_GATHERING;
   extern const MOption MP_MSGS_REMOVE_LIST_PREFIX_BREAKING;
#endif // wxUSE_REGEX/!wxUSE_REGEX

extern const MOption MP_MSGS_GATHER_SUBJECTS;
extern const MOption MP_MSGS_BREAK_THREAD;
extern const MOption MP_MSGS_INDENT_IF_DUMMY;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// dialog field ids
enum
{
   Field_ThreadOnServer,
   Field_ThreadByRefOnly,
#if wxUSE_REGEX
   Field_SubjectRegex,
   Field_SubjectRegexRepl,
#else // !wxUSE_REGEX
   Field_RemoveSubjectListPrefix,
   Field_RemoveSubjectPrefixToCmp,
#endif // wxUSE_REGEX/!wxUSE_REGEX
   Field_GatherSubjects,
   Field_BreakIfSubjectChanges,
   Field_IndentMissingRoot,
   Field_Max
};

// ----------------------------------------------------------------------------
// wxMessageThreadingDialog
// ----------------------------------------------------------------------------

class wxMessageThreadingDialog : public wxOptionsPageSubdialog
{
public:
   wxMessageThreadingDialog(Profile *profile, wxWindow *parent);

   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   bool WasChanged() const { return m_wasChanged; }

protected:
   // event handler
   void OnCheckBox(wxCommandEvent& event);

   // enable/disable controls as needed
   void DoUpdateUI();

   // the dialog data
   // ---------------

   // dirty flag
   bool m_wasChanged;

   bool m_ThrOnServer,
        m_ThrByRefOnly,
        m_Gather,
        m_Break;

#ifdef USE_EXTRA_THREADING_OPTIONS

#if wxUSE_REGEX
   wxString m_Regex,
            m_RegexRepl;
#else
   bool m_Subject1,
        m_Subject2;
#endif // wxUSE_REGEX

   bool m_Indent;

#endif // USE_EXTRA_THREADING_OPTIONS

   // the dialog controls
   // -------------------

   // parent of all controls
   wxEnhancedPanel *m_panel;

   // controls corresponding to the options
   wxCheckBox *m_chkThrOnServer,
              *m_chkThrByRefOnly,
              *m_chkGather,
              *m_chkBreak;

#ifdef USE_EXTRA_THREADING_OPTIONS

#if wxUSE_REGEX
   wxTextCtrl *m_txtRegex,
              *m_txtRegexRepl;
#else // !wxUSE_REGEX
   wxCheckBox *m_chkSubject1,
              *m_chkSubject2;
#endif // wxUSE_REGEX

   wxCheckBox *m_chkIndent;

#endif // USE_EXTRA_THREADING_OPTIONS

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxMessageThreadingDialog)
};

// ============================================================================
// implementation
// ============================================================================

BEGIN_EVENT_TABLE(wxMessageThreadingDialog, wxOptionsPageSubdialog)
   EVT_CHECKBOX(-1, wxMessageThreadingDialog::OnCheckBox)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxMessageThreadingDialog ctor
// ----------------------------------------------------------------------------

wxMessageThreadingDialog::wxMessageThreadingDialog(Profile *profile,
                                                   wxWindow *parent)
                        : wxOptionsPageSubdialog(profile,
                                                 parent,
                                                 _("Message threading"),
                                                 _T("MsgThrDlg"))
{
   m_wasChanged = false;

   wxStaticBox *box = CreateStdButtonsAndBox(_("Threading options"));

   m_panel = new wxEnhancedPanel(this);

   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_panel->SetConstraints(c);

   static const wxChar *fieldLabels[Field_Max] =
   {
      gettext_noop("Thread messages on &server"),
      gettext_noop("Thread messages by &references only"),

#if wxUSE_REGEX
      gettext_noop("Apply this RE to simplif&y subjects"),
      gettext_noop("And &replace the matched part with"),
#else // !wxUSE_REGEX
      gettext_noop("&Remove list prefix from subjects"),
      gettext_noop("Remove list prefi&x to compare subjects to break threads"),
#endif // wxUSE_REGEX/!wxUSE_REGEX

      gettext_noop("&Gather messages with same subject"),
      gettext_noop("&Break thread when subject changes"),
      gettext_noop("&Indent messages with missing ancestor"),
   };

   wxArrayString aLabels;
   for ( size_t n = 0; n < Field_Max; n++ )
   {
      aLabels.Add(wxGetTranslation(fieldLabels[n]));
   }
   long widthMax = GetMaxLabelWidth(aLabels, this);

   wxControl *last = NULL;

   last = m_panel->CreateMessage
          (
            _("The options below allow to configure server side "
              "threading available with some IMAP servers. It is\n"
              "recommended that you enable the first option as "
              "server side threading is, generally, much faster\n"
              "(especially for the large folders).\n"
              "\n"
              "IMAP servers support two threading algorithms: "
              "a correct one (threading by reference) and a simple\n"
              "emulation of it (threading by subject). Some servers "
              "only provide the simple version and the second\n"
              "option allows you to disable it if you prefer to always "
              "thread messages correctly, even if it is slower."),
            last
          );

   m_chkThrOnServer = m_panel->CreateCheckBox(fieldLabels[Field_ThreadOnServer],
                                     widthMax, last);
   last = m_chkThrOnServer;

   m_chkThrByRefOnly = m_panel->CreateCheckBox(fieldLabels[Field_ThreadByRefOnly],
                                      widthMax, last);
   last = m_chkThrByRefOnly;


#ifdef USE_EXTRA_THREADING_OPTIONS
   last = m_panel->CreateMessage
          (
            _("The remaining options allow to configure the details "
              "of the threading algorithm used when threading\n"
              "the messages locally (only the last one is also used "
              "with server side threading). You shouldn't have\n"
              "to modify them normally."),
            last
          );
#else // !USE_EXTRA_THREADING_OPTIONS
   last = m_panel->CreateMessage
          (
            _("\n"
              "\n"
              "The remaining options allow to configure the details "
              "of the threading algorithm used when threading\n"
              "the messages locally only (we don't have such fine "
              "control over server side threading).\n"
              "\n"
              "Disable them if you don't want to consider the subject "
              "field at all while threading."),
            last
          );
#endif // USE_EXTRA_THREADING_OPTIONS/!USE_EXTRA_THREADING_OPTIONS

#ifdef USE_EXTRA_THREADING_OPTIONS

#if wxUSE_REGEX
   last = m_panel->CreateMessage
          (
            _("Mahogany may apply a regular expression to the "
              "subjects before comparing them: this allows to get\n"
              "rid of non meaningful parts such as mailing list "
              "prefix or reply marker easily."),
            last
          );

   m_txtRegex = m_panel->CreateTextWithLabel(fieldLabels[Field_SubjectRegex],
                                             widthMax, last);
   last = m_txtRegex;

   m_txtRegexRepl = m_panel->CreateTextWithLabel(fieldLabels[Field_SubjectRegexRepl],
                                                 widthMax, last);
   last = m_txtRegexRepl;
#else // !wxUSE_REGEX
   m_chkSubject1 = m_panel->CreateCheckBox(fieldLabels[Field_RemoveSubjectListPrefix],
                                           widthMax, last);
   last = m_chkSubject1;

   m_chkSubject2 = m_panel->CreateCheckBox(fieldLabels[Field_RemoveSubjectPrefixToCmp],
                                           widthMax, last);
   last = m_chkSubject2;
#endif // wxUSE_REGEX/!wxUSE_REGEX

#endif // USE_EXTRA_THREADING_OPTIONS

   m_chkGather = m_panel->CreateCheckBox(fieldLabels[Field_GatherSubjects],
                                         widthMax, last);
   last = m_chkGather;

   m_chkBreak = m_panel->CreateCheckBox(fieldLabels[Field_BreakIfSubjectChanges],
                                        widthMax, last);
   last = m_chkBreak;


#ifdef USE_EXTRA_THREADING_OPTIONS
   last = m_panel->CreateMessage
          (
            _("What happens if the root of the thread is not present "
              "in the folder? If the option below is on, the messages\n"
              "are still indented like if it were there, otherwise they "
              "are shifted to the left."),
            last
          );

   m_chkIndent = m_panel->CreateCheckBox(fieldLabels[Field_IndentMissingRoot],
                                         widthMax, last);
#endif // USE_EXTRA_THREADING_OPTIONS

   SetDefaultSize(8*wBtn, 20*hBtn);

   TransferDataToWindow();
}

// ----------------------------------------------------------------------------
// wxMessageThreadingDialog data xfer
// ----------------------------------------------------------------------------

bool wxMessageThreadingDialog::TransferDataToWindow()
{
   // read the options
   Profile *profile = GetProfile();

   m_ThrOnServer = READ_CONFIG_BOOL(profile, MP_MSGS_SERVER_THREAD);
   m_ThrByRefOnly = READ_CONFIG_BOOL(profile, MP_MSGS_SERVER_THREAD_REF_ONLY);

   m_Gather = READ_CONFIG_BOOL(profile, MP_MSGS_GATHER_SUBJECTS);
   m_Break = READ_CONFIG_BOOL(profile, MP_MSGS_BREAK_THREAD);

#ifdef USE_EXTRA_THREADING_OPTIONS

   m_Indent = READ_CONFIG_BOOL(profile, MP_MSGS_INDENT_IF_DUMMY);

#if wxUSE_REGEX
   m_Regex = READ_CONFIG_TEXT(profile, MP_MSGS_SIMPLIFYING_REGEX);
   m_RegexRepl = READ_CONFIG_TEXT(profile, MP_MSGS_REPLACEMENT_STRING);
#else // !wxUSE_REGEX
   m_Subject1 = READ_CONFIG_BOOL(profile, MP_MSGS_REMOVE_LIST_PREFIX_GATHERING);
   m_Subject2 = READ_CONFIG_BOOL(profile, MP_MSGS_REMOVE_LIST_PREFIX_BREAKING);
#endif // wxUSE_REGEX/!wxUSE_REGEX

#endif // USE_EXTRA_THREADING_OPTIONS

   // populate the dialog with them

   m_chkThrOnServer->SetValue(m_ThrOnServer);
   m_chkThrByRefOnly->SetValue(m_ThrByRefOnly);

   m_chkGather->SetValue(m_Gather);
   m_chkBreak->SetValue(m_Break);

#ifdef USE_EXTRA_THREADING_OPTIONS

#if wxUSE_REGEX
   m_txtRegex->SetValue(m_Regex);
   m_txtRegexRepl->SetValue(m_RegexRepl);
#else // !wxUSE_REGEX
   m_chkSubject1->SetValue(m_Subject1);
   m_chkSubject2->SetValue(m_Subject2);
#endif // wxUSE_REGEX/!wxUSE_REGEX

   m_chkIndent->SetValue(m_Indent);

#endif // USE_EXTRA_THREADING_OPTIONS

   DoUpdateUI();

   return TRUE;
}

bool wxMessageThreadingDialog::TransferDataFromWindow()
{
   Profile *profile = GetProfile();

#define WRITE_IF_CHANGE(ctrl, data, opt)  \
   if ( ctrl->GetValue() != data ) \
   { \
      m_wasChanged = true; \
      profile->writeEntry(opt, ctrl->GetValue()); \
   }

   WRITE_IF_CHANGE(m_chkThrOnServer, m_ThrOnServer, MP_MSGS_SERVER_THREAD)
   WRITE_IF_CHANGE(m_chkThrByRefOnly, m_ThrByRefOnly,
                   MP_MSGS_SERVER_THREAD_REF_ONLY)

   WRITE_IF_CHANGE(m_chkGather, m_Gather, MP_MSGS_GATHER_SUBJECTS)
   WRITE_IF_CHANGE(m_chkBreak, m_Break, MP_MSGS_BREAK_THREAD)

#ifdef USE_EXTRA_THREADING_OPTIONS

#if wxUSE_REGEX
   WRITE_IF_CHANGE(m_txtRegex, m_Regex, MP_MSGS_SIMPLIFYING_REGEX)
   WRITE_IF_CHANGE(m_txtRegexRepl, m_RegexRepl, MP_MSGS_REPLACEMENT_STRING)
#else // !wxUSE_REGEX
   WRITE_IF_CHANGE(m_Subject1, m_Subject1, MP_MSGS_REMOVE_LIST_PREFIX_GATHERING)
   WRITE_IF_CHANGE(m_Subject2, m_Subject2, MP_MSGS_REMOVE_LIST_PREFIX_BREAKING)
#endif // wxUSE_REGEX/!wxUSE_REGEX

   WRITE_IF_CHANGE(m_chkIndent, m_Indent, MP_MSGS_INDENT_IF_DUMMY)

#endif // USE_EXTRA_THREADING_OPTIONS

#undef WRITE_IF_CHANGE

   return TRUE;
}

// ----------------------------------------------------------------------------
// wxMessageThreadingDialog event handlers
// ----------------------------------------------------------------------------

void wxMessageThreadingDialog::DoUpdateUI()
{
   bool on = m_chkThrOnServer->GetValue();

   m_chkThrByRefOnly->Enable(on);

#ifdef USE_EXTRA_THREADING_OPTIONS

#if wxUSE_REGEX
   m_txtRegex->Enable(!on);
   m_txtRegexRepl->Enable(!on);
#else // !wxUSE_REGEX
   m_chkSubject1->Enable(!on);
   m_chkSubject2->Enable(!on);
#endif // wxUSE_REGEX/!wxUSE_REGEX

#endif // USE_EXTRA_THREADING_OPTIONS

   m_chkBreak->Enable(!on);
   m_chkGather->Enable(!on);
}

void wxMessageThreadingDialog::OnCheckBox(wxCommandEvent& event)
{
   wxCheckBox *chk = wxStaticCast(event.GetEventObject(), wxCheckBox);

   if ( chk == m_chkThrOnServer )
   {
      DoUpdateUI();
   }

   event.Skip();
}

// ----------------------------------------------------------------------------
// public API
// ----------------------------------------------------------------------------

extern
bool ConfigureThreading(Profile *profile, wxWindow *parent)
{
   wxMessageThreadingDialog dlg(profile, parent);

   return dlg.ShowModal() == wxID_OK && dlg.WasChanged();
}

