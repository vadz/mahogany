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
class WXDLLEXPORT wxStaticBitmap;

// ----------------------------------------------------------------------------
// A base class for all "browse" buttons: these are the small buttons positioned
// near another control and which may be used to browse for this controls value
// (e.g. choose a file, directory or a mail folder from a dialog instead of
// entering its name).
//
// wxBrowseButton creates a tooltip for the button which should give the user
// information about what exactly this button does (as all browse buttons look
// alike, there is no other way to know it).
//
// This class can not be used directly, DoBrowse() must be overriden in the
// derived classes.
// ----------------------------------------------------------------------------

class wxBrowseButton : public wxButton
{
public:
   wxBrowseButton(wxWindow *parent, const wxString& tooltip)
      : wxButton(parent, -1, ">>") { SetToolTip(tooltip); }

   // function which shows the file selection dialog and changes the associated
   // controls contents; it is called in response to the click on the button,
   // but may be also called directly
   virtual void DoBrowse() = 0;

protected:
   // callback
   void OnButtonClick(wxCommandEvent&) { DoBrowse(); }

private:
   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxTextBrowseButton is the base class for the browse buttons which control
// the text controls contents: DoBrowse() is supposed to update it.
//
// The wxTextBrowseButton also manages enabled/disabled state of the text control:
// it enables it when it is enabled and disables otherwise.
//
// This class can not be used directly, DoBrowse() must be overriden in the
// derived classes.
// ----------------------------------------------------------------------------

class wxTextBrowseButton : public wxBrowseButton
{
public:
   // ctor: text is the associated text control
   wxTextBrowseButton(wxTextCtrl *text,
                      wxWindow *parent,
                      const wxString& tooltip)
      : wxBrowseButton(parent, tooltip) { m_text = text; }

   // enable/disable associated text control with us
   virtual bool Enable(bool enable)
   {
      if ( !wxButton::Enable(enable) )
         return FALSE;

      m_text->Enable(enable);
      return wxButton::Enable(enable);
   }

protected:
   // for derived class usage
   wxString GetText() const { return m_text->GetValue(); }
   void SetText(const wxString& text) { m_text->SetValue(text); }

private:
   wxTextCtrl *m_text;
};

// ----------------------------------------------------------------------------
// this button is used to browse for files: it shows the file selection dialog
// and fills the associated text control with the full file name
// ----------------------------------------------------------------------------

class wxFileBrowseButton : public wxTextBrowseButton
{
public:
   wxFileBrowseButton(wxTextCtrl *text, wxWindow *parent)
      : wxTextBrowseButton(text, parent, _("Browse for the file")) { }

   // show the file selection dialog and fill the associated text control with
   // the name of the selected file
   virtual void DoBrowse();
};

// ----------------------------------------------------------------------------
// this button is used to browse for folders: it shows the folder selection
// dialog and fill the associated text control with the full folder name. It
// also has a function to directly retrieve a pointer to the folder chosen
// (which may be NULL)
// ----------------------------------------------------------------------------

class wxFolderBrowseButton : public wxTextBrowseButton
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

// ----------------------------------------------------------------------------
// this button is used to browse for colours: the text control can be used to
// enter the color name
// ----------------------------------------------------------------------------

class wxColorBrowseButton : public wxTextBrowseButton
{
public:
   wxColorBrowseButton(wxTextCtrl *text, wxWindow *parent)
      : wxTextBrowseButton(text, parent, _("Choose colour")) { }

   // get the colour chosen
   wxColour GetColor() const { return m_color; }

   // show the color selection dialog
   virtual void DoBrowse();

private:
   wxColour m_color;
};

// ----------------------------------------------------------------------------
// Icon browse button allows the user to choose an icon from the list of icons.
// It's associated with a wxStaticBitmap control whose contents it updates to
// reflect the currently chosen icon. It also stores the name of the chosen
// icon which is returned from GetIconName().
//
// TODO: allow choosing arbitrary icons from arbitrary files here
// ----------------------------------------------------------------------------

class wxIconBrowseButton : public wxBrowseButton
{
public:
   // ctors
      // takes the list of icons to choose from
   wxIconBrowseButton(wxWindow *parent,
                      const wxString& tooltip,
                      const wxArrayString& iconNames,
                      wxStaticBitmap *staticBitmap = NULL)
      : wxBrowseButton(parent, tooltip)
   {
      Init();
      SetIcons(iconNames);
      SetAssociatedStaticBitmap(staticBitmap);
   }

      // SetIcons should be called if you use this ctor
   wxIconBrowseButton(wxWindow *parent,
                      const wxString& tooltip,
                      wxStaticBitmap *staticBitmap = NULL)
      : wxBrowseButton(parent, tooltip)
   {
      Init();
      SetAssociatedStaticBitmap(staticBitmap);
   }

   // initializers
      // set the list of icons to choose from
   void SetIcons(const wxArrayString& iconNames) { m_iconNames = iconNames; }
      // set the initially selected icon
   void SetIcon(size_t nIcon);
      // sets the associated control
   void SetAssociatedStaticBitmap(wxStaticBitmap *staticBitmap)
      { m_staticBitmap = staticBitmap; }

   // accessors
      // get the selected icon name
   wxString GetIconName() const
      { return m_nIcon == -1 ? wxString("") : m_iconNames[(size_t)m_nIcon]; }

      // get the selected icon index in the array passed to the ctor or -1 if none
   int GetIconIndex() const { return m_nIcon; }

   // show the icon selection dialog
   virtual void DoBrowse();

protected:
   // common part of all ctors
   void Init() { m_nIcon = -1; }

   // get the icon by index
   wxBitmap GetBitmap(size_t nIcon) const;

private:
   // this function is called when the selected icon changes - you need to
   // override it and do something useful there because there is no other way
   // to be notified about the icon chaneg (no wxWin event carries this info)
   virtual void OnIconChange() { }

   wxArrayString m_iconNames;
   int           m_nIcon;
   wxStaticBitmap *m_staticBitmap;
};

// ----------------------------------------------------------------------------
// A special kind of icon browse button which allows selecting an icon for a
// folder - i.e. choose from any of predefined folder icons.
// ----------------------------------------------------------------------------

class wxFolderIconBrowseButton : public wxIconBrowseButton
{
public:
   wxFolderIconBrowseButton(wxWindow *parent, const wxString& tooltip);
};

#endif // _GUI_WXBROWSEBUTTON_H_
