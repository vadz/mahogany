/////////////////////////////////////////////////////////////////////////////
// Name:        wx/pwindow.h
// Purpose:     wxPWindow class declaration
// Author:      Vadim Zeitlin
// Modified by:
// Created:     18/09/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license (part of wxExtra library)
/////////////////////////////////////////////////////////////////////////////

#ifndef   _WX_PWINDOW_H_
#define   _WX_PWINDOW_H_

#ifdef    __GNUG__
    #pragma interface "persctrl.h"
#endif

// forward declarations
class wxConfigBase;
class wxPHelper;

// the headers we really need
#include <wx/notebook.h>
#include <wx/combobox.h>

/*
   The persistent classes themselves start from here. They behave exactly in
   the same was as the standard wxWindows they derive from except that they
   save some configuration information when deleted and restore it when
   recreated. Each of them takes 2 additional parameters in the ctor and
   Create() function (which all of wxWindows controls have):
    1. The first one is the path where the control should save/look for the
       configuration info, in general a descriptive name of the control is
       probably enough.
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

class wxPNotebook : public wxNotebook
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
class wxPTextEntry : public wxComboBox
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

#endif // _WX_PWINDOW_H_
