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

#ifdef    __GNUG__
    #pragma implementation "persctrl.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

// wxWindows
#ifndef WX_PRECOMP
  #include  "wx/window.h"
#endif //WX_PRECOMP

#include "wx/log.h"

#include "wx/config.h"
#include "wx/persctrl.h"

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(wxPNotebook, wxNotebook)
    EVT_SIZE(wxPNotebook::OnSize)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// This is a helper class for all "persistent" window classes, i.e. the classes
// which save their last state in a wxConfig object. The information saved
// depends on each derived class, the base class only provides the functions to
// change to the path in the config object where the information is saved (use
// them to ensure that all derived classes behave in a consistent way).
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

    bool          m_pathRestored;
    wxString      m_path, m_oldPath;
    wxConfigBase *m_config;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxPHelper
// ----------------------------------------------------------------------------

wxString wxPHelper::ms_pathPrefix = "/Settings/";

wxPHelper::wxPHelper(const wxString& path, wxConfigBase *config)
         : m_path(path), m_config(config)
{
   m_pathRestored = TRUE;  // it's not changed yet...
}

wxPHelper::~wxPHelper()
{
    // it will do nothing if path wasn't changed
    RestorePath();
}

bool wxPHelper::ChangePath()
{
    // if we have no config object, don't despair - perhaps we can use the
    // global one?
    if ( m_config == NULL ) {
        m_config = wxConfigBase::Get();
    }

    wxCHECK_MSG( m_config, FALSE, "can't change path without config!" );

    m_oldPath = m_config->GetPath();
    wxString path = ms_pathPrefix;
    if ( path.IsEmpty() || (path.Last() != '/') ) {
        path += '/';
    }

    path += m_path;
    m_config->SetPath(path);

    m_pathRestored = FALSE;

    return TRUE;
}

void wxPHelper::RestorePath()
{
    if ( !m_pathRestored ) {
        wxCHECK_RET( m_config, "can't restore path without config!" );

        m_config->SetPath(m_oldPath);

        m_pathRestored = TRUE;  // to avoid restoring it next time
    }
    //else: path wasn't changed or was already restored since then
}

// ----------------------------------------------------------------------------
// wxPNotebook
// ----------------------------------------------------------------------------

// the key where we store our last selected page
const char *wxPNotebook::ms_pageKey = "Page";

wxPNotebook::wxPNotebook()
{
    m_persist = new wxPHelper;
}

wxPNotebook::wxPNotebook(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         wxConfigBase *config)
          : wxNotebook(parent, id, pos, size, style)
{
    m_persist = new wxPHelper(configPath, config);
}

bool wxPNotebook::Create(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath);

   return wxNotebook::Create(parent, id, pos, size, style);
}

void wxPNotebook::RestorePage()
{
    if ( m_persist->ChangePath() ) {
        int nPage = (int)m_persist->GetConfig()->Read(ms_pageKey, 0l);
        if ( (nPage > 0) && (nPage < GetPageCount()) ) {
            SetSelection(nPage);
        }
        else {
            // invalid page index, (almost) silently ignore
            wxLogDebug("Couldn't restore wxPNotebook page %d.", nPage);
        }

        m_persist->RestorePath();
    }
}

wxPNotebook::~wxPNotebook()
{
    if ( m_persist->ChangePath() ) {
        int nSelection = GetSelection();
        m_persist->GetConfig()->Write(ms_pageKey, (long)nSelection);
    }
    //else: couldn't change path, probably because there is no config object.

    delete m_persist;
}

void wxPNotebook::OnSize(wxSizeEvent& event)
{
    static bool s_bFirstTime = TRUE;

    if ( s_bFirstTime ) {
        RestorePage();

        s_bFirstTime = FALSE;
    }

    // important things are done in the base class version!
    event.Skip();
}

void wxPNotebook::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

void wxPNotebook::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path);
}

// ----------------------------------------------------------------------------
// wxPTextEntry
// ----------------------------------------------------------------------------

// how many strings do we save?
size_t wxPTextEntry::ms_countSaveDefault = 16;

wxPTextEntry::wxPTextEntry()
{
    m_persist = new wxPHelper;
    m_countSaveMax = ms_countSaveDefault;
}

wxPTextEntry::wxPTextEntry(const wxString& configPath,
                           wxWindow *parent,
                           wxWindowID id,
                           const wxString& value,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style,
                           wxConfigBase *config)
           : wxComboBox(parent, id, value, pos, size, style)
{
    m_persist = new wxPHelper(configPath, config);
    m_countSaveMax = ms_countSaveDefault;

    RestoreStrings();
}

wxPTextEntry::~wxPTextEntry()
{
    SaveSettings();

    delete m_persist;
}

bool wxPTextEntry::Create(const wxString& configPath,
                          wxWindow *parent,
                          wxWindowID id,
                          const wxString& value,
                          const wxPoint& pos,
                          const wxSize& size,
                          long style,
                          wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath);

   if ( wxComboBox::Create(parent, id, value, pos, size, style) ) {
       RestoreStrings();

       return TRUE;
   }
   else {
       // the control couldn't be created, don't even attempt anything
       return FALSE;
   }
}

void wxPTextEntry::SaveSettings()
{
    if ( m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();

        // save it compare later with other strings
        wxString text = GetValue();
        if ( !text.IsEmpty() ) {
            config->Write("0", text);
        }

        size_t count = (size_t)Number();
        if ( count > m_countSaveMax ) {
            // too many entries, leave out the oldest ones
            count = m_countSaveMax;
        }

        wxString key, value;

        // this is not the same as n because of duplicate entries (and it
        // starts from 1 because 0 is written above)
        size_t numKey = 1;
        for ( size_t n = 0; n < count; n++ ) {
            value = GetString(n);
            if ( value != text ) {
                key.Printf("%d", numKey++);
                config->Write(key, value);
            }
            //else: don't store duplicates
        }

        m_persist->RestorePath();
    }
}

void wxPTextEntry::RestoreStrings()
{
    if ( m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();

        // read them all
        wxString key, val, text;
        for ( size_t n = 0; ; n++ ) {
            key.Printf("%d", n);
            if ( !config->HasEntry(key) )
                break;
            val = config->Read(key);

            // the first one is the text zone itself
            if ( n == 0 ) {
                text = val;
            }

            Append(val);
        }

        SetValue(text);
        m_persist->RestorePath();
    }
}

void wxPTextEntry::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

void wxPTextEntry::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path);
}

