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

// the headers we really need
#include <wx/notebook.h>
#include <wx/combobox.h>

// ----------------------------------------------------------------------------
// This is a helper class for all "persistent" window classes, i.e. the classes
// which save their last state in a wxConfig object. The information saved
// depends on each derived class, the base class only provides the functions to
// change to the path in the config object where the information is saved (using
// them ensures that all derived classes behave in more or less consistent way).
// ----------------------------------------------------------------------------

class wxPHelper
{
public:
    // static functions
        // set the default prefix for storing the configuration settings
    static void SetSettingsPath(const wxString& path) { ms_pathPrefix = path; }
        // retrieve the default prefix for storing the configuration settings
    static const wxString& GetSettingsPath() { return ms_pathPrefix; }

    // ctors and dtor
        // default: need to use Set() functions later
    wxPHelper() { }
        // the 'path' parameter may be either an absolute path or a relative
        // path, in which case it will go under wxPHelper::GetSettingsPath()
        // branch. If the config object is not given the global application one
        // is used.
    wxPHelper(const wxString& path, wxConfigBase *config = NULL);
        // dtor automatically restores the path if not done yet
    ~wxPHelper();

    // accessors
        // get our config object
    wxConfigBase *GetConfig() const { return m_config; }
        // set the config object to use (must be !NULL)
    void SetConfig(wxConfigBase *config) { m_config = config; }
        // set the path to use (either absolute or relative)
    void SetPath(const wxString& path) { m_path = path; }

    // operations
        // change path to the place where we will write our values, returns
        // FALSE if path couldn't be changed (e.g. no config object)
    bool ChangePath();
        // restore the old path (also done implicitly in dtor)
    void RestorePath();

private:
    static wxString ms_pathPrefix;

    wxString      m_path, m_oldPath;
    wxConfigBase *m_config;
};

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
    wxPNotebook() { }

    wxPNotebook(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                wxConfigBase *config = NULL)
              : wxNotebook(parent, id, pos, size, style),
                m_persist(configPath, config)
    { }

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
    void SetConfigObject(wxConfigBase *config) { m_persist.SetConfig(config); }
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path) { m_persist.SetPath(path); }

    // callbacks
        // when we're resized the first time we restore our page
    void OnSize(wxSizeEvent& event);

protected:
    static const char *ms_pageKey;

    void RestorePage();

    wxPHelper m_persist;

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
    wxPTextEntry() { m_countSaveMax = ms_countSaveDefault; }
        // normal ctor
    wxPTextEntry(const wxString& configPath,
                 wxWindow *parent,
                 wxWindowID id = -1,
                 const wxString& value = wxEmptyString,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = 0,
                 wxConfigBase *config = NULL)
               : wxComboBox(parent, id, value, pos, size, style),
                 m_persist(configPath, config)
    {
        m_countSaveMax = ms_countSaveDefault;

        RestoreStrings();
    }
        // to be used if object was created with default ctor
    Create(const wxString& configPath,
           wxWindow *parent,
           wxWindowID id = -1,
           const wxString& value = wxEmptyString,
           const wxPoint& pos = wxDefaultPosition,
           const wxSize& size = wxDefaultSize,
           long style = 0,
           wxConfigBase *config = NULL);
        // dtor saves the strings
    ~wxPTextEntry() { SaveSettings(); }

    // accessors
        // set how many of last strings will be saved (if 0, only the text sone
        // value is saved, but not the strings in the combobox)
    void SetCountOfStringsToSave(size_t n) { m_countSaveMax = n; }
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config) { m_persist.SetConfig(config); }
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path) { m_persist.SetPath(path); }

protected:
    void SaveSettings();
    void RestoreStrings();

    static size_t ms_countSaveDefault;  // (default)
    size_t m_countSaveMax;              // max number of strings to save

    wxPHelper m_persist;
};

#endif // _WX_PWINDOW_H_
