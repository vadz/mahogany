// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M
// File name:   gui/wxBrowseButton.h - button for browsing for text field
//              contents
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     24.12.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef _GUI_WXBROWSEBUTTON_H_
#define _GUI_WXBROWSEBUTTON_H_

// forward declaration
class MFolder;

// A base class for "browse" buttons: these are the small buttons positioned
// near a text control and which may be used to browse for text control
// contents (e.g. choose a file, directory or a mail folder from a dialog
// instead of entering its name).
//
// The wxBrowseButton also manages enabled/disabled state of the text control:
// it enables it when it is enabled and disables otherwise.
//
// This class can not be used directly, DoBrowse() must be overriden in the
// derived classes.
class wxBrowseButton : public wxButton
{
public:
   // ctor: text is the associated text control
   wxBrowseButton(wxTextCtrl *text, wxWindow *parent, const wxString& tooltip)
      : wxButton(parent, -1, ">>") { m_text = text; SetToolTip(tooltip); }

   // function which shows the file selection dialog and fills the associated
   // text field; it is called in response to the click on the button, but may
   // be also called directly
   virtual void DoBrowse() = 0;

   // enable/disable associated text control with us
   virtual void Enable(bool enable)
   {
      m_text->Enable(enable);

      wxButton::Enable(enable);
   }

protected:
   // callback
   void OnButtonClick(wxCommandEvent&) { DoBrowse(); }

   // for derived class usage
   wxString GetText() const { return m_text->GetValue(); }
   void SetText(const wxString& text) { m_text->SetValue(text); }

private:
   wxTextCtrl *m_text;

   DECLARE_EVENT_TABLE()
};

// this button is used to browse for files: it shows the file selection dialog
// and fills the associated text control with the full file name
class wxFileBrowseButton : public wxBrowseButton
{
public:
   wxFileBrowseButton(wxTextCtrl *text, wxWindow *parent)
      : wxBrowseButton(text, parent, _("Browse for the file")) { }

   // show the file selection dialog and fill the associated text control with
   // the name of the selected file
   virtual void DoBrowse();
};

// this button is used to browse for folders: it shows the folder selection
// dialog and fill the associated text control with the full folder name. It
// also has a function to directly retrieve a pointer to the folder chosen
// (which may be NULL)
class wxFolderBrowseButton : public wxBrowseButton
{
public:
   // we may be optionally given a default folder
   wxFolderBrowseButton(wxTextCtrl *text, wxWindow *parent,
                        MFolder *folder = NULL);

   virtual ~wxFolderBrowseButton();

   // show the folder selection dialog
   virtual void DoBrowse();

   // get the folder chosen by user (may be NULL)
   MFolder *GetFolder() const;

private:
   MFolder *m_folder;
};

#endif // _GUI_WXBROWSEBUTTON_H_
