// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////
// Name:        wx/persctrl.h
// Purpose:     persistent classes declarations
// Author:      Vadim Zeitlin
// Modified by:
// Created:     18/09/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license (part of wxExtra library)
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

#ifndef   _WX_PWINDOW_H_
#define   _WX_PWINDOW_H_

#ifdef    __GNUG__
#   pragma interface "persctrl.h"
#endif

// forward declarations
class wxConfigBase;
class wxPHelper;

// the headers we really need
#include <wx/notebook.h>
#include <wx/control.h>
#include <wx/combobox.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>

// ----------------------------------------------------------------------------
// a helper class for persistent controls
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxPControls
{
public:
    // static functions
        // set the default prefix for storing the configuration settings
    static void SetSettingsPath(const wxString& path) { ms_pathPrefix = path; }
        // retrieve the default prefix for storing the configuration settings
    static const wxString& GetSettingsPath() { return ms_pathPrefix; }

private:
    // the default value is empty
    static wxString ms_pathPrefix;
};

/*
   The persistent classes themselves start from here. They behave exactly in
   the same was as the standard wxWindows classes they derive from except that
   they save some configuration information when deleted and restore it when
   recreated. Each of them takes 2 additional parameters in the ctor and
   Create() function (which all of wxWindows controls have):

    1. The first one is the path where the control should save/look for the
       configuration info, in general a descriptive name of the control is
       probably enough. By default, the prefix returned by
       wxPControls::GetSettingsPath() is prepended to this parameter if it
       doesn't start with a backslash, so you can choose another location for
       all this stuff with wxPControls::SetSettingsPath().

    2. The last one is the config object to use, by default the global
       application one will be used which is the easiest (and so recommended)
       way to use these classes.

   Both of these can be set with the corresponding accessor functions, but it,
   of course, should be done _before_ the control is really created for the
   restored settings to take effect.
*/

// ----------------------------------------------------------------------------
// persistent notebook remembers the last active page
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxPNotebook : public wxNotebook
{
public:
    // ctors and dtor
    wxPNotebook();

    wxPNotebook(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                wxConfigBase *config = NULL);

    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                wxConfigBase *config = NULL);

    ~wxPNotebook();

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

    // callbacks
        // when we're resized the first time we restore our page
    void OnSize(wxSizeEvent& event);

protected:
    static const char *ms_pageKey;

    void RestorePage();

    bool       m_bFirstTime;  // FIXME hack used in OnSize()
    wxPHelper *m_persist;

private:
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// persistent text entry zone: it remembers a given number of strings entered
// ----------------------------------------------------------------------------

// Although this class inherits from wxComboBox, it's designed to be used
// rather as a (single line) text entry, as its name suggests. In particular,
// you shouldn't add/remove strings from this combobox manually (but using
// SetValue (and, of course, GetValue) is perfectly ok).
class WXDLLEXPORT wxPTextEntry : public wxComboBox
{
public:
    // ctors
        // default, use Create() after it
    wxPTextEntry();
        // normal ctor
    wxPTextEntry(const wxString& configPath,
                 wxWindow *parent,
                 wxWindowID id = -1,
                 const wxString& value = wxEmptyString,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = 0,
                 wxConfigBase *config = NULL);

        // to be used if object was created with default ctor
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxString& value = wxEmptyString,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                wxConfigBase *config = NULL);

        // dtor saves the strings
    ~wxPTextEntry();

    // accessors
        // set how many of last strings will be saved (if 0, only the text sone
        // value is saved, but not the strings in the combobox)
    void SetCountOfStringsToSave(size_t n) { m_countSaveMax = n; }
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

protected:
    void SaveSettings();
    void RestoreStrings();

    static size_t ms_countSaveDefault;  // (default)
    size_t m_countSaveMax;              // max number of strings to save

    wxPHelper *m_persist;
};

// ----------------------------------------------------------------------------
// A splitter control which remembers its last position
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxPSplitterWindow : public wxSplitterWindow
{
public:
    // ctors
        // default, use Create() after it
    wxPSplitterWindow();
        // normal ctor
    wxPSplitterWindow(const wxString& configPath,
                      wxWindow *parent,
                      wxWindowID id = -1,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = wxSP_3D | wxCLIP_CHILDREN,
                      wxConfigBase *config = NULL);

        // to be used if object was created with default ctor
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxSP_3D | wxCLIP_CHILDREN,
                wxConfigBase *config = NULL);

        // dtor saves the strings
    ~wxPSplitterWindow();

    // when the window is split for the first time we restore the previously
    // saved position of the splitter
    virtual bool SplitVertically(wxWindow *window1, wxWindow *window2,
                                 int sashPosition = 0);
    virtual bool SplitHorizontally(wxWindow *window1, wxWindow *window2,
                                   int sashPosition = 0);

    // we need to update our m_wasSplit here
    virtual void OnUnsplit(wxWindow *removed);

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

protected:
    // retrieve the position of the sash from config
    int GetStoredPosition(int defaultPos) const;
    // save current position there
    void SavePosition();

    bool       m_wasSplit;
    wxPHelper *m_persist;

private:
    // the config key where we store the sash position
    static const char *ms_sashKey;
};

// ----------------------------------------------------------------------------
// a list control which remembers the widths of its columns
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxPListCtrl : public wxListCtrl
{
public:
    // ctors
        // default, use Create() after it
    wxPListCtrl();
        // standard ctor
    wxPListCtrl(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                wxConfigBase *config = NULL);
        // pseudo ctor
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                wxConfigBase *config = NULL);

    // dtor saves the settings
    virtual ~wxPListCtrl();

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

protected:
    // retrieve the position of the sash from config
    void RestoreWidths();
    // save current position there
    void SavePosition();

    wxPHelper *m_persist;

private:
    static const char *ms_listctrlKey;
};

// ----------------------------------------------------------------------------
// Persistent file selector functions: remember the last directory and file
// name used.
//
// See wxWindows docs and/or wx/filedlg.h for the meaning of all parameters
// except the first and the last one. Also, "defname" (default value)
// parameter is overriden by the last entry saved in the config, so it only
// matters when the function is called for the very first time.
// ----------------------------------------------------------------------------

extern WXDLLEXPORT wxString wxPFileSelector(const wxString& configPath,
                                            const wxString& title,
                                            const char *defpath = NULL,
                                            const char *defname = NULL,
                                            const char *extension = NULL,
                                            const char *filter = NULL,
                                            int flags = 0,
                                            wxWindow *parent = NULL,
                                            wxConfigBase *config = NULL);

// ----------------------------------------------------------------------------
// Persistent (a.k.a. "don't remind me again") message boxes functions.
//
// This function shows to the user a message box with a "don't show this
// message again" check box. If the user checks, it's state will be saved to
// the config object and the next call to this function will return the value
// which was selected the last time (Yes/No/Ok/Cancel...) without showing the
// message box at all.
//
// To reset it (i.e. make message box appear again) just delete the value
// config key where the last value is stored. This key is specified by the
// configPath parameter according to the following rules:
//    1. If configPath is an absolute path, it's used as is
//    2. If it's a relative path (i.e. doesn't start with '/'), it's appended
//       to the default config path wxPControls::GetSettingsPath()
//
// The wontShowAgain parameter may be used to pass in a pointer to a bool which
// will be true if the user checked the check box.
// ----------------------------------------------------------------------------

extern WXDLLEXPORT int wxPMessageBox(const wxString& configPath,
                                     const wxString& message,
                                     const wxString& caption,
                                     long style = wxYES_NO | wxICON_QUESTION,
                                     wxWindow *parent = NULL,
                                     wxConfigBase *config = NULL);

// was the message box disabled?
extern WXDLLEXPORT bool wxPMessageBoxEnabled(const wxString& configPath,
                                             wxConfigBase *config = NULL);

// make sure that the next call to wxPMessageBox(configPath) will show the
// message box (by erasing the stored answer in it)
extern WXDLLEXPORT void wxPMessageBoxEnable(const wxString& configPath,
                                            wxConfigBase *config = NULL);

#endif // _WX_PWINDOW_H_
