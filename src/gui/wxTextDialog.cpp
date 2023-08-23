///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   gui/wxTextDialog.cpp: implements MDialog_ShowText() function
// Purpose:     implements a simple function to show some text to the user
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.02.03 (extracted from src/gui/wxMDialogs.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2003 Vadim Zeitlin
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "gui/wxMFrame.h"

   #include <wx/sizer.h>
   #include <wx/button.h>
   #include <wx/filedlg.h>
   #include <wx/settings.h>
   #include <wx/textctrl.h>
#endif // USE_PCH

#include <wx/ffile.h>
#include <wx/regex.h>

#include <wx/fdrepdlg.h>

#include "wx/persctrl.h"

// ----------------------------------------------------------------------------
// MTextDialog - a dialog containing a multi-line text control (used to show
//               user some text)
// ----------------------------------------------------------------------------

class MTextDialog : public wxDialog
{
public:
   MTextDialog(wxWindow *parent,
               const wxString& title,
               const wxString& text,
               const char *configPath);

   virtual ~MTextDialog();

private:
   // button events

   // save the text controls contents to file
   void OnSave(wxCommandEvent& event);

   // show the "Find" dialog
   void OnFind(wxCommandEvent& event);

   // close the dialog
   void OnCancel(wxCommandEvent& event);


   // find dialog events

   // find next occurrence
   void OnFindDialogNext(wxFindDialogEvent& event);

   // find dialog has been closed
   void OnFindDialogClose(wxFindDialogEvent& event);


   // the text we show
   wxTextCtrl *m_text;

   // the find dialog or NULL if none is shown
   wxFindReplaceDialog *m_dlgFind;

   // find dialog data used by m_dlgFind
   wxFindReplaceData m_dataFind;

   // last search string
   wxString m_strFind;

   // last search regex
   wxRegEx m_regexFind;

   // end of the last match (0 if none)
   size_t m_posFind;

   // flags of the last match (combination of wxFR_XXX)
   int m_flagsFind;

   // the path for our size/position info in config (may be NULL)
   const char *m_configPath;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MTextDialog)
};

BEGIN_EVENT_TABLE(MTextDialog, wxDialog)
   EVT_BUTTON(wxID_SAVE, MTextDialog::OnSave)
   EVT_BUTTON(wxID_FIND, MTextDialog::OnFind)
   EVT_BUTTON(wxID_CANCEL, MTextDialog::OnCancel)

   EVT_FIND(-1, MTextDialog::OnFindDialogNext)
   EVT_FIND_NEXT(-1, MTextDialog::OnFindDialogNext)
   EVT_FIND_CLOSE(-1, MTextDialog::OnFindDialogClose)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MTextDialog ctor and dtor
// ----------------------------------------------------------------------------

MTextDialog::MTextDialog(wxWindow *parent,
                         const wxString& title,
                         const wxString& text,
                         const char *configPath)
           : wxDialog(parent,
                      -1,
                      _T("Mahogany: ") + title,
                      wxDefaultPosition,
                      wxDefaultSize,
                      wxDEFAULT_DIALOG_STYLE |
                      wxRESIZE_BORDER)
{
   // init members
   // ------------

   m_dlgFind = NULL;

   m_posFind = 0;

   m_flagsFind = 0;

   m_configPath = configPath;

   // create controls
   // ---------------

   m_text = new wxTextCtrl(this, -1, wxEmptyString,
                           wxDefaultPosition,
                           wxDefaultSize,
                           wxTE_MULTILINE |
                           wxTE_READONLY |
                           wxTE_NOHIDESEL |
                           wxTE_RICH2);

   // use fixed-width font and latin1 encoding in which all text is valid:
   // without encoding information (which wouldn't make sense anyhow as we can
   // have multiple parts using different encodings) we must do this to at
   // least show something to the user while using the default UTF-8 encoding
   // of GTK+ 2 could result in nothing being shown at all
   //
   // also use wxSYS_ANSI_VAR_FONT and not wxSYS_DEFAULT_GUI_FONT (also used by
   // wxNORMAL_FONT) because its size is too small for fixed width font under
   // Windows currently
   m_text->SetFont(wxFont
                   (
                     wxSystemSettings::GetFont(wxSYS_ANSI_VAR_FONT).
                        GetPointSize(),
                     wxFONTFAMILY_TELETYPE,
                     wxFONTSTYLE_NORMAL,
                     wxFONTWEIGHT_NORMAL,
                     false /* not underlined */,
                     wxEmptyString,
                     wxFONTENCODING_ISO8859_1
                   ));

   // now that the encoding is set, we can show the text
   m_text->SetValue(text);

   // in TAB order we want "Save" to get focus before "Close", so create
   // them in order
   wxButton *btnSave = new wxButton(this, wxID_SAVE, _("&Save...")),
            *btnFind = new wxButton(this, wxID_FIND, _("&Find...")),
            *btnClose = new wxButton(this, wxID_CANCEL, _("Close"));


   // layout them
   // -----------

   wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL),
           *sizerBtns = new wxBoxSizer(wxHORIZONTAL);

   sizerBtns->Add(btnSave, 0, wxRIGHT, LAYOUT_X_MARGIN);
   sizerBtns->Add(btnFind, 0, wxLEFT | wxRIGHT, LAYOUT_X_MARGIN);
   sizerBtns->Add(btnClose, 0, wxLEFT, LAYOUT_X_MARGIN);

   sizerTop->Add(m_text, 1, wxEXPAND);
   sizerTop->Add(sizerBtns, 0, wxCENTRE | wxTOP | wxBOTTOM, LAYOUT_Y_MARGIN);

   SetSizer(sizerTop);

   // final initialization
   // --------------------

   m_text->SetFocus();

   // under wxGTK SetFocus() scrolls the control to the bottom for some reason
   // while we always want to show the top of the message
   m_text->SetInsertionPoint(0);

   // we may have or not the location in config where the dialogs position/size
   // are stored
   int x, y, w, h;
   if ( m_configPath )
   {
      wxMFrame::RestorePosition(configPath, &x, &y, &w, &h);
   }
   else
   {
      x =
      y = -1;
      w = 500;
      h = 300;
   }

   SetSize(x, y, w, h);

   Show(TRUE);
}

MTextDialog::~MTextDialog()
{
   if ( m_dlgFind )
      m_dlgFind->Destroy();

   if ( m_configPath )
   {
      wxMFrame::SavePosition(m_configPath, this);
   }
}

// ----------------------------------------------------------------------------
// MTextDialog button event handlers
// ----------------------------------------------------------------------------

// save the text controls contents to file
void MTextDialog::OnSave(wxCommandEvent&)
{
   String filename = wxPSaveFileSelector
                     (
                       this,
                       _T("RawText"),
                       _("Mahogany: Please choose where to save the text")
                     );
   if ( !filename.empty() )
   {
      wxFFile fileOut(filename, _T("w"));
      if ( !fileOut.IsOpened() || !fileOut.Write(m_text->GetValue()) )
      {
         wxLogError(_("Failed to save the dialog contents."));
      }
   }
}

void MTextDialog::OnCancel(wxCommandEvent&)
{
   Destroy();
}

void MTextDialog::OnFind(wxCommandEvent&)
{
   if ( !m_dlgFind )
   {
      // create the find dialog (we can't search up, so use wxFR_NOUPDOWN)
      m_dlgFind = new wxFindReplaceDialog(this,
                                          &m_dataFind,
                                          _("Mahogany: Find regular expression"),
                                          wxFR_NOUPDOWN);
      m_dlgFind->Show(TRUE);
   }
   else // dialog already exists
   {
      // just bring it in front
      m_dlgFind->Raise();
      m_dlgFind->SetFocus();
   }
}

// ----------------------------------------------------------------------------
// MTextDialog find event handlers
// ----------------------------------------------------------------------------

void MTextDialog::OnFindDialogNext(wxFindDialogEvent& event)
{
   // build the regex we're looking for
   wxString strFind = event.GetFindString();
   const int flagsFind = event.GetFlags();
   if ( strFind != m_strFind || flagsFind != m_flagsFind )
   {
      // need to (re)compile because either the text or the flags changed
      int flagsRE = wxRE_EXTENDED | wxRE_NEWLINE;
      if ( !(flagsFind & wxFR_MATCHCASE) )
         flagsRE |= wxRE_ICASE;

      if ( flagsFind & wxFR_WHOLEWORD )
         strFind = wxString(_T("[[:<:]]")) + strFind + _T("[[:>:]]");

      if ( !m_regexFind.Compile(strFind, flagsRE) )
      {
         wxLogError(_("Invalid regular expression \"%s\"."), strFind);
         return;
      }

      m_strFind = strFind;
      m_flagsFind = flagsFind;
   }
   //else: ok, continue with the old regex

   // start searching for it
   const wxString text = m_text->GetValue();
   if ( !m_regexFind.Matches(text.c_str() + m_posFind) )
   {
      // reset the selection but use m_posFind and not 0 to prevent the control
      // from scrolling back to the origin if it had been scrolled down
      m_text->SetSelection(m_posFind, m_posFind);
      m_strFind.clear();
      m_posFind = 0;

      wxLogError(m_posFind ? _("No more matches") : _("No match"));
   }
   else // highlight the found text
   {
      size_t pos, len;
      m_regexFind.GetMatch(&pos, &len);

      // note that pos is relative to the start of search, i.e. m_posFind
      m_posFind += pos;
      m_text->SetSelection(m_posFind, m_posFind + len);
      m_posFind += len;
   }
}

void MTextDialog::OnFindDialogClose(wxFindDialogEvent&)
{
   m_dlgFind->Destroy();
   m_dlgFind = NULL;
}

// ----------------------------------------------------------------------------
// our public API
// ----------------------------------------------------------------------------

extern "C"
void MDialog_ShowText(wxWindow *parent,
                      const wxString& title,
                      const wxString& text,
                      const char *configPath)
{
   // show the dialog modelessly because otherwise we wouldn't be able to show
   // a find dialog from it
   new MTextDialog(GetFrame(parent), title, text, configPath);
}

