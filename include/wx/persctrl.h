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
    #pragma interface "pcontrol.h"
#endif

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
    static void GetSettingsPath() { return ms_pathPrefix; }

    // ctor
        // default: need to use Set() functions later
    wxPHelper();
        // the 'path' parameter may be either an absolute path or a relative
        // path, in which case it will go under wxPHelper::GetSettingsPath()
        // branch. If the config object is not given the global application one
        // is used.
    wxPHelper(const wxString& path, wxConfigBase *config = NULL);

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfig(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetPath(const wxString& path);

    // operations
        // change path to the place where we will write our values
    void ChangePath();
        // restore the old path (also done implicitly in dtor)
    void RestorePath();
    
private:
    static wxString ms_pathPrefix;

    wxString      m_oldPath;
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
    // ctors
    wxPNotebook() { }

    wxPNotebook(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = "notebook",
                wxConfigBase *config  NULL)
               : wxNotebook(parent, id, pos, size, style, name),
                 m_persist(configPath, config)
    {
    }
    
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = "notebook",
                wxConfigBase *config = NULL)
    {
       m_persist.SetConfig(config);
       m_persist.SetPath(configPath);

       return wxNotebook::Create(parent, id, pos, size, style, name);
    }

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config) { m_persist.SetConfig(config); }
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path) { m_persist.SetPath(path); }

private:
    wxPHelper m_persist;
};

// ----------------------------------------------------------------------------
// persistent text entry zone: it remembers a given number of strings entered
// ----------------------------------------------------------------------------

#endif // _WX_PWINDOW_H_
