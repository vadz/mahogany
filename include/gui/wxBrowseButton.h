///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

#ifndef _GUI_WXBROWSEBUTTON_H_
#define _GUI_WXBROWSEBUTTON_H_

#ifndef USE_PCH
#   include <wx/button.h>
#   include <wx/textctrl.h>
#endif // USE_PCH

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
      : wxButton(parent, -1, _T(">>")) { SetToolTip(tooltip); }

   // function which shows the file selection dialog and changes the associated
   // controls contents; it is called in response to the click on the button,
   // but may be also called directly
   virtual void DoBrowse() = 0;

protected:
   // callback
   void OnButtonClick(wxCommandEvent&) { DoBrowse(); }

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxBrowseButton)
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

   // direct access to the text controls contents
   wxString GetText() const { return m_text->GetValue(); }
   void SetText(const wxString& text) { m_text->SetValue(text); }

   // text control we're associated with
   wxTextCtrl *GetTextCtrl() const { return m_text; }

private:
   wxTextCtrl *m_text;

   DECLARE_NO_COPY_CLASS(wxTextBrowseButton)
};

// ----------------------------------------------------------------------------
// this button is used to browse for files: it shows the file selection dialog
// and fills the associated text control with the full file name
// ----------------------------------------------------------------------------

class wxFileBrowseButton : public wxTextBrowseButton
{
public:
   // if open parameter is TRUE, it means that we only want to open an existing
   // file, so the file dialog won't allow selecting the non existing filename.
   // OTOH, if open is FALSE, we're using this button for choosing the name of
   // the file to save something to, so any filename (even a non existing one
   // may be specified) and the user will have to confirm the file overwrite if
   // he does specify an existing file name. Finally, there may be situations
   // where we want to allow choosing any filename for opening, even a non
   // existing one - this is achieved by setting existingOnly param to FALSE
   // (only for "open" buttons, it doesn't make sense for "save" ones)
   wxFileBrowseButton(wxTextCtrl *text, wxWindow *parent,
                      bool open = TRUE, bool existingOnly = TRUE)
      : wxTextBrowseButton(text, parent, _("Browse for the file"))
   {
      m_open = open;
      m_existingOnly = existingOnly;
   }

   // show the file selection dialog and fill the associated text control with
   // the name of the selected file
   virtual void DoBrowse();

private:
   bool m_open,
        m_existingOnly;

   DECLARE_NO_COPY_CLASS(wxFileBrowseButton)
};

// ----------------------------------------------------------------------------
// this button is used to browse for directories: it shows the file selection
// dialog and fills the associated text control with the directory path
// (without trailing backslash - except for the root directory when it's also
// the initial one)
// ----------------------------------------------------------------------------

class wxDirBrowseButton : public wxTextBrowseButton
{
public:
   wxDirBrowseButton(wxTextCtrl *text, wxWindow *parent)
      : wxTextBrowseButton(text, parent, _("Browse for a directory")) { }

   // show the file selection dialog and fill the associated text control with
   // the name of the selected directory
   virtual void DoBrowse();

private:
   // a hack for wxFileOrDirBrowseButton convenience
   friend class wxFileOrDirBrowseButton;
   static void DoBrowseHelper(wxTextBrowseButton *browseBtn);

   DECLARE_NO_COPY_CLASS(wxDirBrowseButton)
};

// ----------------------------------------------------------------------------
// a hybrid of wxFileBrowseButton and wxDirBrowseButton: the user can choose
// either file or directory with this button depending on its current mode
// (which is "File" by default, but can be changed as often as needed)
// ----------------------------------------------------------------------------

class wxFileOrDirBrowseButton : public wxFileBrowseButton
{
public:
   wxFileOrDirBrowseButton(wxTextCtrl *text, wxWindow *parent,
                           bool open = TRUE, bool existingOnly = TRUE)
      : wxFileBrowseButton(text, parent, open, existingOnly)
   {
      m_browseForFile = TRUE;
   }

   // get or change the current browsing mode
      // returns TRUE if in "file" mode, FALSE if in "directory" one
   bool IsBrowsingForFiles() const { return m_browseForFile; }
      // change the current mode
   void BrowseForFiles() { m_browseForFile = TRUE; UpdateTooltip(); }
   void BrowseForDirectories() { m_browseForFile = FALSE; UpdateTooltip(); }

   // show the file selection dialog and fill the associated text control with
   // the name of the selected file or directory
   virtual void DoBrowse();

private:
   // set the tooltip corresponding to our current browse mode
   void UpdateTooltip();

   bool m_browseForFile;

   DECLARE_NO_COPY_CLASS(wxFileOrDirBrowseButton)
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

   DECLARE_NO_COPY_CLASS(wxFolderBrowseButton)
};

// ----------------------------------------------------------------------------
// this button is used to browse for colours: the text control can be used to
// enter the color name
// ----------------------------------------------------------------------------

class wxColorBrowseButton : public wxTextBrowseButton
{
public:
   wxColorBrowseButton(wxTextCtrl *text, wxWindow *parent);
   virtual ~wxColorBrowseButton();

   // get the colour chosen
   wxColour GetColor() const { return m_color; }

   // show the color selection dialog
   virtual void DoBrowse();

   // get/set the text value: must use these functions instead of wxTextCtrl
   // methods to update the button colour as well!
   void SetValue(const wxString& text);
   wxString GetValue() const { return GetTextCtrl()->GetValue(); }

private:
   // update the button colour on screen to match m_color
   void UpdateColor();

   // update the button colour to match the text control contents
   void UpdateColorFromText();

   // notify us that the associated text control was deleted
   void OnTextDelete();

   // the current colour (may be invalid if none)
   wxColour m_color;

   // do we still have a valid text control?
   bool m_hasText;

   // the custom event handler for the associated text control
   class wxColorTextEvtHandler *m_evtHandlerText;

   // it calls our UpdateColorFromText() and OnTextDelete()
   friend class wxColorTextEvtHandler;

   // abstract because we have no default ctor
   DECLARE_ABSTRACT_CLASS(wxColorBrowseButton)
   DECLARE_NO_COPY_CLASS(wxColorBrowseButton)
};

// ----------------------------------------------------------------------------
// font browser button allows the user to select a font by popping up a font
// selection dialog when clicked - the text control may be used to enter the
// font name directly even though it's not really convenient right now
// ----------------------------------------------------------------------------

class wxFontBrowseButton : public wxTextBrowseButton
{
public:
   wxFontBrowseButton(wxTextCtrl *text, wxWindow *parent);

   // show the font selection dialog
   virtual void DoBrowse();

   // convert between wxNativeFontInfo description and the string we show to
   // the user in the text control
   static String FontDescToUser(const String& desc);
   static String FontDescFromUser(const String& desc);

private:
   DECLARE_NO_COPY_CLASS(wxFontBrowseButton)
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
      { return m_nIcon == -1 ? wxString(_T("")) : m_iconNames[(size_t)m_nIcon]; }

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

   DECLARE_NO_COPY_CLASS(wxIconBrowseButton)
};

// ----------------------------------------------------------------------------
// A special kind of icon browse button which allows selecting an icon for a
// folder - i.e. choose from any of predefined folder icons.
// ----------------------------------------------------------------------------

class wxFolderIconBrowseButton : public wxIconBrowseButton
{
public:
   wxFolderIconBrowseButton(wxWindow *parent, const wxString& tooltip);

private:
   DECLARE_NO_COPY_CLASS(wxFolderIconBrowseButton)
};

#endif // _GUI_WXBROWSEBUTTON_H_
