/////////////////////////////////////////////////////////////////////////////
// Name:        generic/persctrl.cpp
// Purpose:     persistent classes implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     18/09/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license (part of wxExtra library)
/////////////////////////////////////////////////////////////////////////////

#ifdef    __GNUG__
#   pragma implementation "persctrl.h"
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
#   pragma hdrstop
#endif

// wxWindows
#ifndef WX_PRECOMP
#   include "wx/log.h"
#   include "wx/intl.h"
#   include "wx/config.h"

#   include "wx/sizer.h"

#   include "wx/button.h"
#   include "wx/radiobut.h"
#   include "wx/stattext.h"
#   include "wx/statbmp.h"
#   include "wx/statline.h"

#   include "wx/filedlg.h"
#   include "wx/dirdlg.h"
#   include "wx/msgdlg.h"
#endif //WX_PRECOMP

#include "wx/artprov.h"

#include "wx/tokenzr.h"

#include "wx/persctrl.h"

#ifdef __WINDOWS__
    #include <windows.h>
    #include "wx/msw/winundef.h"
#endif // __WINDOWS__

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxPChoice, wxChoice)
    EVT_SIZE(wxPChoice::OnSize)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxPNotebook, wxNotebook)
    EVT_SIZE(wxPNotebook::OnSize)
END_EVENT_TABLE()

#if wxUSE_LISTBOOK

BEGIN_EVENT_TABLE(wxPListbook, wxListbook)
    EVT_SIZE(wxPListbook::OnSize)
END_EVENT_TABLE()

#endif // wxUSE_LISTBOOK

BEGIN_EVENT_TABLE(wxPListCtrl, wxListCtrl)
    EVT_SIZE(wxPListCtrl::OnSize)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxPListBox, wxListBox)
    EVT_SIZE(wxPListBox::OnSize)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxPRadioBox, wxRadioBox)
    EVT_SIZE(wxPRadioBox::OnSize)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxPTreeCtrl, wxTreeCtrl)
    EVT_SIZE(wxPTreeCtrl::OnSize)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define LAYOUT_X_MARGIN       5
#define LAYOUT_Y_MARGIN       5

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// wxID_FOO -> wxFOO
static int TranslateBtnIdToMsgBox(int rc);

// translate the old wxID values to the new ones
static int ConvertId(long rc);

#if wxCHECK_VERSION(2, 3, 1)
    #define Number()    GetCount()
#endif

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
    // ctors and dtor
        // default: need to use Set() functions later
    wxPHelper() { }
        // the 'path' parameter may be either an absolute path or a relative
        // path, in which case it will go under wxPControls::GetSettingsPath()
        // + prefix.
        // If the config object is not given the global application one is
        // used instead.
    wxPHelper(const wxString& path,
              const wxString& prefix,
              wxConfigBase *config = NULL);
        // dtor automatically restores the path if not done yet
    ~wxPHelper();

    // accessors
        // get our config object
    wxConfigBase *GetConfig() const { return m_config; }
        // set the config object to use (must be !NULL)
    void SetConfig(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetPath(const wxString& path, const wxString& prefix);

    // operations
        // change path to the place where we will write our values, returns
        // FALSE if path couldn't be changed (e.g. no config object)
    bool ChangePath();
        // restore the old path (also done implicitly in dtor)
    void RestorePath();

    // get the key name
    const wxString& GetKey() const { return m_key; }

private:
    bool          m_pathRestored;
    wxString      m_key, m_path, m_oldPath;
    wxConfigBase *m_config;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxPControls
// ----------------------------------------------------------------------------

// don't allocate memory for static objects
wxString wxPControls::ms_pathPrefix = "";

// ----------------------------------------------------------------------------
// wxPHelper
// ----------------------------------------------------------------------------

wxPHelper::wxPHelper(const wxString& path,
                     const wxString& prefix,
                     wxConfigBase *config)
{
    SetConfig(config);
    SetPath(path, prefix);

    m_pathRestored = TRUE;  // it's not changed yet...
}

void wxPHelper::SetConfig(wxConfigBase *config)
{
    m_config = config;

    // if we have no config object, don't despair - perhaps we can use the
    // global one?
    if ( !m_config ) {
        m_config = wxConfigBase::Get();
    }
}

void wxPHelper::SetPath(const wxString& path, const wxString& prefix)
{
    wxCHECK_RET( !path.empty(), _T("empty path in persistent ctrl code") );

    wxString strKey, strPath = path.BeforeLast('/');
    if ( !strPath ) {
        strPath.Empty();
        strKey = path;
    }
    else {
        strKey = path.AfterLast('/');
    }

    // don't prepend settings path to the absolute paths
    if ( path[0u] == '/' ) {
        // absolute path given, ignore prefix and global settings path and
        // use it as "is"
        m_path = strPath;
        m_key = strKey;
    }
    else {
        m_path = wxPControls::GetSettingsPath();
        if ( m_path.empty() || (m_path.Last() != '/') ) {
            m_path += '/';
        }

        m_path << prefix << '/' << strPath;
        m_key = strKey;
    }

    ChangePath();
}

wxPHelper::~wxPHelper()
{
    // it will do nothing if path wasn't changed
    RestorePath();
}

bool wxPHelper::ChangePath()
{
    wxCHECK_MSG( m_config, FALSE, _T("can't change path without config!") );

    m_oldPath = m_config->GetPath();
    m_config->SetPath(m_path);

    m_pathRestored = FALSE;

    return TRUE;
}

void wxPHelper::RestorePath()
{
    if ( !m_pathRestored ) {
        wxCHECK_RET( m_config, _T("can't restore path without config!") );

        m_config->SetPath(m_oldPath);

        m_pathRestored = TRUE;  // to avoid restoring it next time
    }
    //else: path wasn't changed or was already restored since then
}

// ----------------------------------------------------------------------------
// wxPNotebook
// ----------------------------------------------------------------------------

// the key where we store our last selected page
const wxChar *wxPNotebook::ms_path = _T("NotebookPages");

wxPNotebook::wxPNotebook()
{
    m_bFirstTime = true;
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
    m_bFirstTime = true;
    m_persist = new wxPHelper(configPath, ms_path, config);
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
   m_persist->SetPath(configPath, ms_path);

   return wxNotebook::Create(parent, id, pos, size, style);
}

void wxPNotebook::RestorePage()
{
    if ( m_persist->ChangePath() ) {
        int nPage = (int)m_persist->GetConfig()->Read(m_persist->GetKey(), 0l);
        if ( (nPage >= 0) && (nPage < (int)GetPageCount()) ) {
            SetSelection(nPage);
        }
        else {
            // invalid page index, (almost) silently ignore
            wxLogDebug(_T("Couldn't restore wxPNotebook page %d."), nPage);
        }

        m_persist->RestorePath();
    }
}

wxPNotebook::~wxPNotebook()
{
    if ( m_persist->ChangePath() ) {
        int nSelection = GetSelection();
        m_persist->GetConfig()->Write(m_persist->GetKey(), (long)nSelection);
    }
    //else: couldn't change path, probably because there is no config object.

    delete m_persist;
}

// first time our OnSize() is called we restore the page: there is no other
// event sent specifically after window creation and we can't do in the ctor
// (too early) - this should change in wxWin 2.1...
void wxPNotebook::OnSize(wxSizeEvent& event)
{
    if ( m_bFirstTime ) {
        RestorePage();

        m_bFirstTime = FALSE;
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
    m_persist->SetPath(path, ms_path);
}

// ----------------------------------------------------------------------------
// wxPListbook
// ----------------------------------------------------------------------------

#if wxUSE_LISTBOOK

// the key where we store our last selected page
const wxChar *wxPListbook::ms_path = _T("ListbookPages");

wxPListbook::wxPListbook()
{
    m_bFirstTime = true;
    m_persist = new wxPHelper;
}

wxPListbook::wxPListbook(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         wxConfigBase *config)
          : wxListbook(parent, id, pos, size, style)
{
    m_bFirstTime = true;
    m_persist = new wxPHelper(configPath, ms_path, config);
}

bool wxPListbook::Create(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath, ms_path);

   return wxListbook::Create(parent, id, pos, size, style);
}

void wxPListbook::RestorePage()
{
    if ( m_persist->ChangePath() ) {
        int nPage = (int)m_persist->GetConfig()->Read(m_persist->GetKey(), 0l);
        if ( (nPage >= 0) && (nPage < (int)GetPageCount()) ) {
            SetSelection(nPage);
        }
        else {
            // invalid page index, (almost) silently ignore
            wxLogDebug(_T("Couldn't restore wxPListbook page %d."), nPage);
        }

        m_persist->RestorePath();
    }
}

wxPListbook::~wxPListbook()
{
    if ( m_persist->ChangePath() ) {
        int nSelection = GetSelection();
        m_persist->GetConfig()->Write(m_persist->GetKey(), (long)nSelection);
    }
    //else: couldn't change path, probably because there is no config object.

    delete m_persist;
}

// first time our OnSize() is called we restore the page: there is no other
// event sent specifically after window creation and we can't do in the ctor
// (too early) - this should change in wxWin 2.1...
void wxPListbook::OnSize(wxSizeEvent& event)
{
    if ( m_bFirstTime ) {
        RestorePage();

        m_bFirstTime = FALSE;
    }

    // important things are done in the base class version!
    event.Skip();
}

void wxPListbook::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

void wxPListbook::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path, ms_path);
}

#endif // wxUSE_LISTBOOK

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
    wxString realConfigPath(configPath);
    if ( !realConfigPath.empty() && realConfigPath.Last() != '/' ) {
        // we need a subgroup name, not a key name
        realConfigPath += '/';
    }

    m_persist = new wxPHelper(realConfigPath, "", config);
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
   m_persist->SetPath(configPath, "");

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
        if ( !text.empty() ) {
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
                key.Printf("%lu", (unsigned long)numKey++);
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
            key.Printf("%lu", (unsigned long)n);
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
    m_persist->SetPath(path, "");
}

// ----------------------------------------------------------------------------
// wxPSplitterWindow
// ----------------------------------------------------------------------------

const wxChar *wxPSplitterWindow::ms_path = _T("SashPositions");

wxPSplitterWindow::wxPSplitterWindow()
{
    m_persist = new wxPHelper;
}

wxPSplitterWindow::wxPSplitterWindow(const wxString& configPath,
                                     wxWindow *parent,
                                     wxWindowID id,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style,
                                     wxConfigBase *config)
                 : wxSplitterWindow(parent, id, pos, size, style)
{
    m_wasSplit = FALSE;

    m_persist = new wxPHelper(configPath, ms_path, config);
}

bool wxPSplitterWindow::Create(const wxString& configPath,
                               wxWindow *parent,
                               wxWindowID id,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style,
                               wxConfigBase *config)
{
   m_wasSplit = FALSE;

   m_persist->SetConfig(config);
   m_persist->SetPath(configPath, ms_path);

   return wxSplitterWindow::Create(parent, id, pos, size, style);
}

wxPSplitterWindow::~wxPSplitterWindow()
{
    SavePosition();

    delete m_persist;
}

void wxPSplitterWindow::SavePosition()
{
    // only store the position if we're split
    if ( m_wasSplit && m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();
        config->Write(m_persist->GetKey(), (long)GetSashPosition());

        m_persist->RestorePath();
    }
}

int wxPSplitterWindow::GetStoredPosition(int defaultPos) const
{
    if ( m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();
        int pos = config->Read(m_persist->GetKey(), defaultPos);

        m_persist->RestorePath();

        return pos;
    }

    return defaultPos;
}

bool wxPSplitterWindow::SplitVertically(wxWindow *window1,
                                        wxWindow *window2,
                                        int sashPosition)
{
    int pos;

    if ( !m_wasSplit ) {
        m_wasSplit = TRUE;

        pos = GetStoredPosition(sashPosition);
    }
    else {
        pos = sashPosition;
    }

    return wxSplitterWindow::SplitVertically(window1, window2, pos);
}

bool wxPSplitterWindow::SplitHorizontally(wxWindow *window1,
                                          wxWindow *window2,
                                          int sashPosition)
{
    int pos;

    if ( !m_wasSplit ) {
        m_wasSplit = TRUE;

        pos = GetStoredPosition(sashPosition);
    }
    else {
        pos = sashPosition;
    }

    return wxSplitterWindow::SplitHorizontally(window1, window2, pos);
}

void wxPSplitterWindow::OnUnsplit(wxWindow *removed)
{
    SavePosition();

    m_wasSplit = FALSE;

    wxSplitterWindow::OnUnsplit(removed);
}

void wxPSplitterWindow::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

void wxPSplitterWindow::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path, ms_path);
}

// ----------------------------------------------------------------------------
// wxPListCtrl
// ----------------------------------------------------------------------------

const wxChar *wxPListCtrl::ms_path = _T("ListCtrlColumns");

// default ctor
wxPListCtrl::wxPListCtrl()
{
    m_bFirstTime = true;
    m_persist = new wxPHelper;
}

// standard ctor
wxPListCtrl::wxPListCtrl(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxPoint &pos,
                         const wxSize &size,
                         long style,
                         const wxValidator& validator,
                         wxConfigBase *config)
           : wxListCtrl(parent, id, pos, size, style, validator)
{
    m_bFirstTime = true;
    m_persist = new wxPHelper(configPath, ms_path, config);
}

// pseudo ctor
bool wxPListCtrl::Create(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxPoint &pos,
                         const wxSize &size,
                         long style,
                         const wxValidator& validator,
                         wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath, ms_path);

   return wxListCtrl::Create(parent, id, pos, size, style, validator);
}

// dtor saves the settings
wxPListCtrl::~wxPListCtrl()
{
    SaveWidths();

    delete m_persist;
}

// set the config object to use (must be !NULL)
void wxPListCtrl::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

// set the path to use (either absolute or relative)
void wxPListCtrl::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path, ms_path);
}

// first time our OnSize() is called we restore the page: there is no other
// event sent specifically after window creation and we can't do in the ctor
// (too early) - this should change in wxWin 2.1...
void wxPListCtrl::OnSize(wxSizeEvent& event)
{
    if ( m_bFirstTime ) {
        RestoreWidths();

        m_bFirstTime = FALSE;
    }

    // important things are done in the base class version!
    event.Skip();
}

// retrieve the column widths from config
void wxPListCtrl::RestoreWidths()
{
    if ( m_persist->ChangePath() ) {
        wxString str = m_persist->GetConfig()->Read(m_persist->GetKey());
        if ( !str.empty() )
        {
            int countCol = GetColumnCount();
            char *p = (char *)str.c_str();
            for ( int col = 0; col < countCol; col++ )
            {
                if ( IsEmpty(p) )
                    break;

                char *end = strchr(p, ':');
                if ( end )
                    *end = '\0';    // temporarily truncate

                int width;
                if ( sscanf(p, "%d", &width) == 1 )
                {
                    SetColumnWidth(col, width);
                }
                else
                {
                    wxFAIL_MSG(_T("wxPListCtrl: corrupted config entry?"));
                }

                if ( !end )
                    break;

                p = end + 1;
            }
        }

        m_persist->RestorePath();
    }
}

// save the column widths to config as ':' delimited sequence of numbers
void wxPListCtrl::SaveWidths()
{
    wxString str;

    int count = GetColumnCount();
    if ( count == 0 )
    {
        // probably the control couldn't be created at all?
        return;
    }

    for ( int col = 0; col < count; col++ )
    {
        if ( !str.empty() )
            str << ':';

        str << GetColumnWidth(col);
    }

    if ( m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();
        config->Write(m_persist->GetKey(), str);

        m_persist->RestorePath();
    }
}

// ----------------------------------------------------------------------------
// persistent checkbox
// ----------------------------------------------------------------------------

const wxChar *wxPCheckBox::ms_path = _T("Checkboxes");

// default ctor
wxPCheckBox::wxPCheckBox()
{
    m_persist = new wxPHelper;
}

// standard ctor
wxPCheckBox::wxPCheckBox(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxString& label,
                         const wxPoint &pos,
                         const wxSize &size,
                         long style,
                         const wxValidator& validator,
                         wxConfigBase *config)
           : wxCheckBox(parent, id, label, pos, size, style, validator)
{
    m_persist = new wxPHelper(configPath, ms_path, config);

    RestoreValue();
}

// pseudo ctor
bool wxPCheckBox::Create(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxString& label,
                         const wxPoint &pos,
                         const wxSize &size,
                         long style,
                         const wxValidator& validator,
                         wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath, ms_path);

   if ( !wxCheckBox::Create(parent, id, label, pos, size, style, validator) ) {
       // failed to create the control
       return FALSE;
   }

   RestoreValue();

   return TRUE;
}

// dtor saves the settings
wxPCheckBox::~wxPCheckBox()
{
    SaveValue();

    delete m_persist;
}

// set the config object to use (must be !NULL)
void wxPCheckBox::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

// set the path to use (either absolute or relative)
void wxPCheckBox::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path, ms_path);
}

void wxPCheckBox::RestoreValue()
{
    if ( m_persist->ChangePath() ) {
        bool value = m_persist->GetConfig()->Read(m_persist->GetKey(), 0l) != 0;
        SetValue(value);

        m_persist->RestorePath();
    }
}

void wxPCheckBox::SaveValue()
{
    if ( m_persist->ChangePath() ) {
        bool value = GetValue();
        m_persist->GetConfig()->Write(m_persist->GetKey(), (long)value);

        m_persist->RestorePath();
    }
    //else: couldn't change path, probably because there is no config object.
}

// ----------------------------------------------------------------------------
// wxPListBox
// ----------------------------------------------------------------------------

const wxChar *wxPListBox::ms_path = _T("ListBoxSelection");

// default ctor
wxPListBox::wxPListBox()
{
    m_bFirstTime = true;
    m_persist = new wxPHelper;
}

// standard ctor
wxPListBox::wxPListBox(const wxString& configPath,
                       wxWindow *parent,
                       wxWindowID id,
                       const wxPoint &pos,
                       const wxSize &size,
                       int n,
                       const wxString *items,
                       long style,
                       const wxValidator& validator,
                       wxConfigBase *config)
           : wxListBox(parent, id, pos, size, n, items, style, validator)
{
    m_bFirstTime = true;
    m_persist = new wxPHelper(configPath, ms_path, config);
}

// pseudo ctor
bool wxPListBox::Create(const wxString& configPath,
                        wxWindow *parent,
                        wxWindowID id,
                        const wxPoint &pos,
                        const wxSize &size,
                        int n,
                        const wxString *items,
                        long style,
                        const wxValidator& validator,
                        wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath, ms_path);

   return wxListBox::Create(parent, id, pos, size, n, items, style, validator);
}

// dtor saves the settings
wxPListBox::~wxPListBox()
{
    SaveSelection();

    delete m_persist;
}

// set the config object to use (must be !NULL)
void wxPListBox::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

// set the path to use (either absolute or relative)
void wxPListBox::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path, ms_path);
}

// first time our OnSize() is called we restore the seection: we can't do it
// before because we don't know when all the items will be added to the listbox
// (surely they may be added after ctor call)
void wxPListBox::OnSize(wxSizeEvent& event)
{
    if ( m_bFirstTime ) {
        RestoreSelection();

        m_bFirstTime = FALSE;
    }

    // important things may be done in the base class version!
    event.Skip();
}

// retrieve the selection from config
void wxPListBox::RestoreSelection()
{
    if ( m_persist->ChangePath() ) {
        long sel = m_persist->GetConfig()->Read(m_persist->GetKey(), 0l);

        if ( (sel != -1) && (sel < Number()) ) {
            SetSelection(sel);

            // emulate the event which would have resulted if the user selected
            // the listbox
            wxCommandEvent event(wxEVT_COMMAND_LISTBOX_SELECTED, GetId());
            event.m_commandInt = sel;
            if ( HasClientUntypedData() )
                event.m_clientData = GetClientData(sel);
            else if ( HasClientObjectData() )
                event.m_clientData = GetClientObject(sel);
            event.m_commandString = GetString(sel);
            event.SetEventObject( this );
            (void)ProcessEvent(event);
        }

        m_persist->RestorePath();
    }
}

// save the selection to config
void wxPListBox::SaveSelection()
{
    if ( m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();
        config->Write(m_persist->GetKey(), (long)GetSelection());

        m_persist->RestorePath();
    }
}

// ----------------------------------------------------------------------------
// wxPChoice
// ----------------------------------------------------------------------------

const wxChar *wxPChoice::ms_path = _T("ChoiceSelection");

// default ctor
wxPChoice::wxPChoice()
{
    m_bFirstTime = true;
    m_persist = new wxPHelper;
}

// standard ctor
wxPChoice::wxPChoice(const wxString& configPath,
                     wxWindow *parent,
                     wxWindowID id,
                     const wxPoint &pos,
                     const wxSize &size,
                     int n,
                     const wxString *items,
                     long style,
                     const wxValidator& validator,
                     wxConfigBase *config)
           : wxChoice(parent, id, pos, size, n, items, style, validator)
{
    m_bFirstTime = true;
    m_persist = new wxPHelper(configPath, ms_path, config);
}

// pseudo ctor
bool wxPChoice::Create(const wxString& configPath,
                        wxWindow *parent,
                        wxWindowID id,
                        const wxPoint &pos,
                        const wxSize &size,
                        int n,
                        const wxString *items,
                        long style,
                        const wxValidator& validator,
                        wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath, ms_path);

   return wxChoice::Create(parent, id, pos, size, n, items, style, validator);
}

// dtor saves the settings
wxPChoice::~wxPChoice()
{
    SaveSelection();

    delete m_persist;
}

// set the config object to use (must be !NULL)
void wxPChoice::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

// set the path to use (either absolute or relative)
void wxPChoice::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path, ms_path);
}

// first time our OnSize() is called we restore the seection: we can't do it
// before because we don't know when all the items will be added to the listbox
// (surely they may be added after ctor call)
void wxPChoice::OnSize(wxSizeEvent& event)
{
    if ( m_bFirstTime ) {
        RestoreSelection();

        m_bFirstTime = FALSE;
    }

    // important things may be done in the base class version!
    event.Skip();
}

// retrieve the selection from config
void wxPChoice::RestoreSelection()
{
    if ( m_persist->ChangePath() ) {
        long sel = m_persist->GetConfig()->Read(m_persist->GetKey(), 0l);

        if ( (sel != -1) && (sel < Number()) ) {
            SetSelection(sel);

            // emulate the event which would have resulted if the user selected
            // the string from the choice
            wxCommandEvent event(wxEVT_COMMAND_CHOICE_SELECTED, GetId());
            event.m_commandInt = sel;
            if ( HasClientUntypedData() )
                event.m_clientData = GetClientData(sel);
            else if ( HasClientObjectData() )
                event.m_clientData = GetClientObject(sel);
            event.m_commandString = GetString(sel);
            event.SetEventObject( this );
            (void)ProcessEvent(event);
        }

        m_persist->RestorePath();
    }
}

// save the selection to config
void wxPChoice::SaveSelection()
{
    if ( m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();
        config->Write(m_persist->GetKey(), (long)GetSelection());

        m_persist->RestorePath();
    }
}

// ----------------------------------------------------------------------------
// wxPRadioBox
// ----------------------------------------------------------------------------

const wxChar *wxPRadioBox::ms_path = _T("RadioBoxSelection");

// default ctor
wxPRadioBox::wxPRadioBox()
{
    m_bFirstTime = true;
    m_persist = new wxPHelper;
}

// standard ctor
wxPRadioBox::wxPRadioBox(const wxString& configPath,
                       wxWindow *parent,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint &pos,
                       const wxSize &size,
                       int n,
                       const wxString *items,
                       int majorDim,
                       long style,
                       const wxValidator& validator,
                       wxConfigBase *config)
           : wxRadioBox(parent, id, title, pos, size, n, items,
                        majorDim, style, validator)
{
    m_bFirstTime = true;
    m_persist = new wxPHelper(configPath, ms_path, config);
}

// pseudo ctor
bool wxPRadioBox::Create(const wxString& configPath,
                        wxWindow *parent,
                        wxWindowID id,
                        const wxString& title,
                        const wxPoint &pos,
                        const wxSize &size,
                        int n,
                        const wxString *items,
                        int majorDim,
                        long style,
                        const wxValidator& validator,
                        wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath, ms_path);

   return wxRadioBox::Create(parent, id, title, pos, size, n, items,
                             majorDim, style, validator);
}

// dtor saves the settings
wxPRadioBox::~wxPRadioBox()
{
    SaveSelection();

    delete m_persist;
}

// set the config object to use (must be !NULL)
void wxPRadioBox::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

// set the path to use (either absolute or relative)
void wxPRadioBox::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path, ms_path);
}

// first time our OnSize() is called we restore the seection: we can't do it
// before because we don't know when all the items will be added to the radiobox
// (surely they may be added after ctor call)
void wxPRadioBox::OnSize(wxSizeEvent& event)
{
    if ( m_bFirstTime ) {
        RestoreSelection();

        m_bFirstTime = FALSE;
    }

    // important things may be done in the base class version!
    event.Skip();
}

// retrieve the selection from config
void wxPRadioBox::RestoreSelection()
{
    if ( m_persist->ChangePath() ) {
        long sel = m_persist->GetConfig()->Read(m_persist->GetKey(), 0l);

        if ( sel < Number() ) {
            SetSelection(sel);

            // emulate the event which would have resulted if the user selected
            // the radiobox
            wxCommandEvent event(wxEVT_COMMAND_RADIOBOX_SELECTED, GetId());
            event.SetInt(sel);
            event.SetEventObject(this);
            (void)ProcessEvent(event);
        }

        m_persist->RestorePath();
    }
}

// save the selection to config
void wxPRadioBox::SaveSelection()
{
    if ( m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();
        config->Write(m_persist->GetKey(), (long)GetSelection());

        m_persist->RestorePath();
    }
}

// ----------------------------------------------------------------------------
// wxPTreeCtrl
// ----------------------------------------------------------------------------

const wxChar *wxPTreeCtrl::ms_path = _T("TreeCtrlExp");

// default ctor
wxPTreeCtrl::wxPTreeCtrl()
{
    m_bFirstTime = true;
    m_persist = new wxPHelper;
}

// standard ctor
wxPTreeCtrl::wxPTreeCtrl(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxPoint &pos,
                         const wxSize &size,
                         long style,
                         const wxValidator& validator,
                         wxConfigBase *config)
           : wxTreeCtrl(parent, id, pos, size, style, validator)
{
    m_bFirstTime = true;
    m_persist = new wxPHelper(configPath, ms_path, config);
}

// pseudo ctor
bool wxPTreeCtrl::Create(const wxString& configPath,
                         wxWindow *parent,
                         wxWindowID id,
                         const wxPoint &pos,
                         const wxSize &size,
                         long style,
                         const wxValidator& validator,
                         wxConfigBase *config)
{
   m_persist->SetConfig(config);
   m_persist->SetPath(configPath, ms_path);

   return wxTreeCtrl::Create(parent, id, pos, size, style, validator);
}

// dtor saves the settings
wxPTreeCtrl::~wxPTreeCtrl()
{
    SaveExpandedBranches();

    delete m_persist;
}

// set the config object to use (must be !NULL)
void wxPTreeCtrl::SetConfigObject(wxConfigBase *config)
{
    m_persist->SetConfig(config);
}

// set the path to use (either absolute or relative)
void wxPTreeCtrl::SetConfigPath(const wxString& path)
{
    m_persist->SetPath(path, ms_path);
}

// first time our OnSize() is called we restore the seection: we can't do it
// before because we don't know when all the items will be added to the radiobox
// (surely they may be added after ctor call)
void wxPTreeCtrl::OnSize(wxSizeEvent& event)
{
    if ( m_bFirstTime ) {
        // reset the flag first as the calls to Expand() below may generate in
        // other OnSize()s
        m_bFirstTime = FALSE;

        RestoreExpandedBranches();
    }

    // important things may be done in the base class version!
    event.Skip();
}

// we remember all extended branches by their indices: this is not as reliable
// as using names because if something somehow changes we will reexpand a
// wrong branch, but this is not supposed to happen [often] and using labels
// would take up much more space in the config

// find all expanded branches under the given tree item, return TRUE if this
// item is expanded at all (then we added something to branches array) and
// FALSE otherwise
bool wxPTreeCtrl::GetExpandedBranches(const wxTreeItemId& id,
                                      wxArrayString& branches)
{
    if ( !IsExpanded(id) )
        return FALSE;

    bool hasExpandedChildren = FALSE;

    size_t nChild = 0;
#if wxCHECK_VERSION(2, 5, 0)
    wxTreeItemIdValue cookie;
#else
    long cookie;
#endif
    wxTreeItemId idChild = GetFirstChild(id, cookie);
    while ( idChild.IsOk() )
    {
        wxArrayString subbranches;
        if ( GetExpandedBranches(idChild, subbranches) )
        {
            wxString prefix = wxString::Format(_T("%lu,"),
                                               (unsigned long)nChild);
            size_t count = subbranches.GetCount();
            for ( size_t n = 0; n < count; n++ )
            {
                branches.Add(prefix + subbranches[n]);
            }

            hasExpandedChildren = TRUE;
        }

        idChild = GetNextChild(id, cookie);
        nChild++;
    }

    // we don't need to add a branch for this item if we have expanded
    // children because we will get expanded anyhow - but we do have to do it
    // otherwise
    if ( !hasExpandedChildren )
    {
        branches.Add(_T("0"));
    }

    return TRUE;
}

wxString wxPTreeCtrl::SaveExpandedBranches(const wxTreeItemId& itemRoot)
{
    wxArrayString branches;
    GetExpandedBranches(itemRoot, branches);

    // we store info about all extended branches of max length separating
    // the indices in one branch by ',' and branches by ':'
    wxString data;
    size_t count = branches.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        if ( n != 0 )
            data += _T(':');

        data += branches[n];
    }

    return data;
}

void wxPTreeCtrl::SaveExpandedBranches()
{
    if ( m_persist->ChangePath() ) {
        wxConfigBase *config = m_persist->GetConfig();

        config->Write(m_persist->GetKey(), SaveExpandedBranches(GetRootItem()));

        m_persist->RestorePath();
    }
}

void wxPTreeCtrl::RestoreExpandedBranches(const wxTreeItemId& itemRoot,
                                          const wxString& expState)
{
    wxStringTokenizer tkBranches(expState, _T(":"));
    while ( tkBranches.HasMoreTokens() )
    {
        wxStringTokenizer tkNodes(tkBranches.GetNextToken(), _T(","));

        wxTreeItemId id = itemRoot;
        while ( tkNodes.HasMoreTokens() && id.IsOk() )
        {
            Expand(id);

            unsigned long index;
            wxString node = tkNodes.GetNextToken();
            if ( !node.ToULong(&index) )
            {
                wxLogDebug(_T("Corrupted config data: '%s'."),
                           node.c_str());
                break;
            }

            // get the child with the given index
#if wxCHECK_VERSION(2, 5, 0)
            wxTreeItemIdValue cookie;
#else
            long cookie;
#endif
            wxTreeItemId idChild = GetFirstChild(id, cookie);
            while ( idChild.IsOk() )
            {
                if ( index-- == 0 )
                    break;

                idChild = GetNextChild(id, cookie);
            }

            id = idChild;

            if ( !idChild.IsOk() )
            {
                wxLogDebug(_T("Number of items in wxPTreeCtrl changed "
                              "unexpectedly."));
                break;
            }
        }
    }
}

void wxPTreeCtrl::RestoreExpandedBranches()
{
    if ( m_persist->ChangePath() ) {
        // when we run for the first time, expand the top level of the
        // tree by default - this is why we use "0" as default value
        wxString data = m_persist->GetConfig()->Read(m_persist->GetKey(), _T("0"));

#ifdef __WXMSW__
        // under Windows, expanding several branches provokes terrible flicker
        // which can be [partly] avoided by hiding and showing it
        Hide();
#endif // __WXMSW__

        RestoreExpandedBranches(GetRootItem(), data);

#ifdef __WXMSW__
        Show();
#endif // __WXMSW__

        m_persist->RestorePath();
    }
}

// ----------------------------------------------------------------------------
// peristent message box stuff
// ----------------------------------------------------------------------------

// just a message box with a checkbox
class wxPMessageDialog : public wxDialog
{
public:
    wxPMessageDialog(wxWindow *parent,
                     const wxString& message,
                     const wxString& caption,
                     long style,
                     const wxPMessageBoxParams& params);

    // return the index of the checked radio button or -1 if not disabled
    int GetDisabledIndex() const;

    // callbacks
    void OnCheckBox(wxCommandEvent& event)
        { UpdateUIOnCheck(event.IsChecked()); }
    void OnButton(wxCommandEvent& event);

private:
    void UpdateUIOnCheck(bool checked);


    wxCheckBox *m_chkDisable;

    size_t m_countRadioBtns;
    wxRadioButton *m_radiobuttons[6]; // should be enough choices...

    bool m_dontDisableOnNo;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPMessageDialog)
};

BEGIN_EVENT_TABLE(wxPMessageDialog, wxDialog)
    EVT_BUTTON(-1,      wxPMessageDialog::OnButton)
    EVT_CHECKBOX(-1,    wxPMessageDialog::OnCheckBox)
END_EVENT_TABLE()

// although this enum really should be local in wxPMessageDialog ctor, this
// causes internal compiler errors with gcc 2.96, so put it here instead
enum
{
    Btn_Ok,
    Btn_Yes,
    Btn_No,
    Btn_Cancel,
    Btn_Max
};

wxPMessageDialog::wxPMessageDialog(wxWindow *parent,
                                   const wxString& message,
                                   const wxString& caption,
                                   long style,
                                   const wxPMessageBoxParams& params)
                : wxDialog(parent, -1, caption)
{
    m_chkDisable = NULL;
    m_countRadioBtns = 0;
    m_dontDisableOnNo = params.dontDisableOnNo;

    // the dialog consists of the main, msg box, part and optionally a lower
    // part allowing to disable it, sizerTop contains both of them
    wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);

    // create the main part
    wxSizer *sizerMsgBox = new wxBoxSizer(wxVERTICAL);
    wxSizer *sizerMsgAndText = new wxBoxSizer(wxHORIZONTAL),
            *sizerButtons = CreateButtonSizer(style);

    wxArtID id;
    switch ( style & wxICON_MASK )
    {
        case wxICON_WARNING:
            id = wxART_WARNING;
            break;

        case wxICON_ERROR:
            id = wxART_ERROR;
            break;

        case wxICON_QUESTION:
            id = wxART_QUESTION;
            break;

        default:
            wxFAIL_MSG( _T("unknown icon flag") );
            // fall through

        case wxICON_INFORMATION:
            id = wxART_INFORMATION;
            break;
    }

    wxBitmap bmp = wxArtProvider::GetBitmap(id);

    sizerMsgAndText->Add(new wxStaticBitmap(this, -1, bmp), 0,
                            wxALIGN_TOP | wxRIGHT, LAYOUT_X_MARGIN);
    sizerMsgAndText->Add(new wxStaticText(this, -1, message), 1,
                            wxALIGN_TOP | wxALL, LAYOUT_Y_MARGIN);

    sizerMsgBox->Add(sizerMsgAndText, 1, wxEXPAND);
    sizerMsgBox->Add(0, LAYOUT_Y_MARGIN);
    sizerMsgBox->Add(sizerButtons, 0,
                        wxALIGN_CENTER | (wxALL & ~wxBOTTOM), LAYOUT_Y_MARGIN);

    sizerTop->Add(sizerMsgBox, 1, wxEXPAND | wxALL, LAYOUT_Y_MARGIN);

    // create the lower part if needed
    if ( !params.disableMsg.empty() )
    {
        sizerTop->Add(new wxStaticLine(this, -1), 0, wxEXPAND);

        wxSizer *sizerDontShow = new wxBoxSizer(wxHORIZONTAL);
        m_chkDisable = new wxCheckBox(this, -1, params.disableMsg);
        sizerDontShow->Add(m_chkDisable, 0, wxALIGN_CENTRE_VERTICAL);

        m_countRadioBtns = params.disableOptions.GetCount();
        if ( m_countRadioBtns >= 2 )
        {
            if ( m_countRadioBtns > WXSIZEOF(m_radiobuttons) )
            {
                wxFAIL_MSG( _T("too many ways to disable") );

                m_countRadioBtns = WXSIZEOF(m_radiobuttons);
            }

            // several ways to disable this message box
            wxSizer *sizerOpts = new wxBoxSizer(wxVERTICAL);
            for ( size_t n = 0; n < m_countRadioBtns; n++ )
            {
                m_radiobuttons[n]
                    = new wxRadioButton(this, -1, params.disableOptions[n]);
                sizerOpts->Add(m_radiobuttons[n], 0, wxBOTTOM, LAYOUT_Y_MARGIN);

                if ( (int)n == params.indexDisable )
                {
                    m_radiobuttons[n]->SetValue(true);
                }
            }

            sizerDontShow->Add(sizerOpts, 0,
                                wxALIGN_CENTRE_VERTICAL | wxLEFT, LAYOUT_X_MARGIN);
        }
        else // m_countRadioBtns < 2
        {
            wxASSERT_MSG( !m_countRadioBtns,
                            _T("one string in disableOptions doesn't make sense") );

            m_countRadioBtns = 0;
        }

        const bool enable = params.indexDisable != -1;
        m_chkDisable->SetValue(enable);
        UpdateUIOnCheck(enable);

        sizerTop->Add(sizerDontShow, 0, wxALIGN_CENTER | wxALL, LAYOUT_Y_MARGIN);
    }

    SetSizerAndFit(sizerTop);
    Centre(wxCENTER_FRAME | wxBOTH);
}

int wxPMessageDialog::GetDisabledIndex() const
{
    if ( m_chkDisable->GetValue() )
    {
        if ( !m_countRadioBtns )
        {
            // if there are no radio buttons there is only one way in which we
            // can be disabled
            return 0;
        }

        for ( size_t n = 0; n < m_countRadioBtns; n++ )
        {
            if ( m_radiobuttons[n]->GetValue() )
                return n;
        }
    }

    return -1;
}

void wxPMessageDialog::UpdateUIOnCheck(bool checked)
{
    for ( size_t n = 0; n < m_countRadioBtns; n++ )
    {
        m_radiobuttons[n]->Enable(checked);
    }

    if ( m_dontDisableOnNo )
    {
        wxWindow *win = FindWindow(wxID_NO);
        if ( win )
        {
            win->Enable(!checked);
        }
    }
}

void wxPMessageDialog::OnButton(wxCommandEvent& event)
{
    // which button?
    switch ( event.GetId() ) {
        case wxID_NO:
        case wxID_YES:
        case wxID_OK:
            break;

        default:
            wxFAIL_MSG(_T("unexpected button id in wxPMessageDialog."));
            // fall through nevertheless

        case wxID_CANCEL:
            // allow to cancel the dialog with [Cancel] only if it's not a
            // "Yes/No" type dialog
            if ( (GetWindowStyle() & wxYES_NO) != wxYES_NO || HasFlag(wxCANCEL) )
            {
                break;
            }

            // skip EndModal()
            return;
    }

    EndModal(TranslateBtnIdToMsgBox(event.GetId()));
}

static const wxChar *gs_MessageBoxPath = _T("MessageBox");
static const int DONT_PREDISABLE = -1;

int wxPMessageBox(const wxString& configPath,
                  const wxString& message,
                  const wxString& caption,
                  long style,
                  wxWindow *parent,
                  wxConfigBase *config,
                  wxPMessageBoxParams *paramsUser)
{
    if ( configPath.Length() ) {
        wxPHelper persist(configPath, gs_MessageBoxPath, config);

        // if config was NULL, wxPHelper already has the global one
        config = persist.GetConfig();

        if ( config ) {
            wxPMessageBoxParams paramsDef;
            wxPMessageBoxParams *params = paramsUser ? paramsUser : &paramsDef;

            wxString configValue = persist.GetKey();

            int rc = config->Read(configValue, 0l);

            // special hack: we use -1 as an indicator that the msg box should
            // not be "pre disabled", see also below
            if ( rc == DONT_PREDISABLE )
            {
                // don't pre disable anything
                params->indexDisable = -1;

                rc = 0;
            }
            else
            {
                rc = ConvertId(rc);
            }

            if ( !rc ) {
#ifdef __WINDOWS__
                // hack: we're called with mouse being captured by the list
                // ctrl under Windows and we must release it to allow it to
                // work normally in the dialog which we show below
               ::ReleaseCapture();
#endif // __WINDOWS__

                // do show the msg box
                wxPMessageDialog dlg(parent, message, caption, style, *params);
                rc = dlg.ShowModal();

                // forget everything if the dialog was cancelled
                if ( rc != wxCANCEL )
                {
                    // remember if the user doesn't want to see this dialog
                    // again
                    params->indexDisable = dlg.GetDisabledIndex();
                    if ( params->indexDisable != -1 )
                    {
                        if ( rc != wxNO || !params->dontDisableOnNo )
                        {
                            // next time we won't show it
                            persist.ChangePath();
                            config->Write(configValue, rc);
                        }
                        //else: don't allow remembering "No" as the answer
                    }
                    else // user didn't disable it
                    {
                        // remember that we shouldn't pre-disable this message
                        // box by default when it is called for the next time
                        // by writing a special value
                        persist.ChangePath();
                        config->Write(configValue, DONT_PREDISABLE);
                    }
                }
            }
            //else: don't show it, it was disabled

            return rc;
        }
    }

    // use the system standard message box
    wxMessageDialog dlg(parent, message, caption, style);

    return TranslateBtnIdToMsgBox(dlg.ShowModal());
}

int wxPMessageBoxIsDisabled(const wxString& configPath, wxConfigBase *config)
{
    if ( configPath.empty() )
    {
        // non persistent msg boxes are always enabled
        return 0;
    }

    wxPHelper persist(configPath, gs_MessageBoxPath, config);
    wxString configValue = persist.GetKey();

    // if config was NULL, wxPHelper already has the global one
    config = persist.GetConfig();

    wxCHECK_MSG( config, 0, _T("no config in wxPMessageBoxEnabled") );

    return ConvertId(config->Read(configValue, 0l));
}

void wxPMessageBoxDisable(const wxString& configPath,
                          int value,
                          wxConfigBase *config)
{
    wxPHelper persist(configPath, gs_MessageBoxPath, config);
    wxString configValue = persist.GetKey();

    // if config was NULL, wxPHelper already has the global one
    config = persist.GetConfig();

    if ( !config )
        return;

    if ( !value ) {
        // (re)enable
        if ( config->Exists(configValue) ) {
           // delete stored value
           config->DeleteEntry(configValue);
        }
    }
    else {
       // disable by writing the specified value for it
       config->Write(configValue, (long)value);
    }
}

void wxPMessageBoxEnable(const wxString& configPath, wxConfigBase *config)
{
    // giving the value of 0 means to delete any existing value
    wxPMessageBoxDisable(configPath, 0, config);
}

// -----------------------------------------------------------------------------
// Persistent file selector boxes stuff
// -----------------------------------------------------------------------------

// common part of wxPFileSelector and wxPFilesSelector
static wxFileDialog *wxShowFileSelectorDialog(const wxString& configPath,
                                              const wxString& title,
                                              const wxChar *defpath,
                                              const wxChar *defname,
                                              const wxChar *defext,
                                              const wxChar *filter,
                                              int flags,
                                              wxWindow *parent,
                                              wxConfigBase *config)
{
    wxCHECK_MSG( !!configPath, NULL, _T("configPath can't be empty") );

    wxString ourPath,           // path in the config
             configValueFile,   // name of the entry where filename is stored
             configValuePath;   // name of the entry where path is stored

    if ( configPath[0u] != '/' ) {
        // prepend some common prefix
        ourPath = "FilePrompts/";
    }
    //else: absolute path - use as is

    ourPath += configPath.BeforeLast('/');
    configValueFile = configPath.AfterLast('/');
    configValuePath << configValueFile << "Path";

    wxPHelper persist(ourPath, "", config);

    // if config was NULL, wxPHelper already has the global one
    config = persist.GetConfig();

    // use the stored value for the default name/path and fall back to the
    // given one if there is none
    wxString defaultName, defaultPath, defName;
    if ( !wxIsEmpty(defname) || !wxIsEmpty(defext) )
    {
        // only do it if either name of extension are given
        defName << defname << '.' << defext;
    }

    // use the values stored from the last time unless the special flag is
    // given and we do have the name/path
    defaultName = defName;
    defaultPath = defpath;
    if ( config ) {
        if ( !(flags & wxFILEDLG_USE_FILENAME) || wxIsEmpty(defname) )
            defaultName = config->Read(configValueFile, defName);
        if ( !(flags & wxFILEDLG_USE_FILENAME) || wxIsEmpty(defpath) )
            defaultPath = config->Read(configValuePath, defpath);
    }

    wxFileDialog *dialog = new wxFileDialog(parent,
                                            title,
                                            defaultPath,
                                            defaultName,
                                            filter
                                            ? filter
                                            : wxFileSelectorDefaultWildcardStr,
                                            flags);
    if ( dialog->ShowModal() != wxID_OK ) {
        // cancelled
        dialog->Destroy();

        dialog = NULL;
    }
    else {
        // remember the last filename/path chosen
        wxString filename = dialog->GetPath();
        if ( !filename.empty() && config ) {
            wxString path, name, ext;
            wxSplitPath(filename, &path, &name, &ext);

            if( ext.Length() )
               name << '.' << ext;

            persist.ChangePath();
            config->Write(configValueFile, name);
            config->Write(configValuePath, path);
        }
    }

    return dialog;
}

wxString wxPFileSelector(const wxString& configPath,
                         const wxString& title,
                         const wxChar *defpath,
                         const wxChar *defname,
                         const wxChar *defext,
                         const wxChar *filter,
                         int flags,
                         wxWindow *parent,
                         wxConfigBase *config)
{
    // show the file selector box
    wxFileDialog *dialog = wxShowFileSelectorDialog(configPath,
                                                    title,
                                                    defpath,
                                                    defname,
                                                    defext,
                                                    filter,
                                                    flags,
                                                    parent,
                                                    config);

    if ( !dialog )
        return wxEmptyString;

    wxString filename = dialog->GetPath();
    dialog->Destroy();

    return filename;
}

size_t wxPFilesSelector(wxArrayString& filenames,
                        const wxString& configPath,
                        const wxString& title,
                        const wxChar *defpath,
                        const wxChar *defname,
                        const wxChar *defext,
                        const wxChar *filter,
                        int flags,
                        wxWindow *parent,
                        wxConfigBase *config)
{
    wxFileDialog *dialog = wxShowFileSelectorDialog(configPath,
                                                    title,
                                                    defpath,
                                                    defname,
                                                    defext,
                                                    filter,
                                                    flags | wxMULTIPLE,
                                                    parent,
                                                    config);
    if ( !dialog )
        return 0;

    dialog->GetPaths(filenames);
    dialog->Destroy();

    return filenames.GetCount();
}

// ----------------------------------------------------------------------------
// Persistent directory selector dialog box
// ----------------------------------------------------------------------------

// wxPDirSelector helper
static wxString wxGetDirectory(wxWindow *parent,
                               const wxString& message,
                               const wxString& path)
{
    wxString dir;
    wxDirDialog dlg(parent, message, path);

    if ( dlg.ShowModal() == wxID_OK )
    {
        dir = dlg.GetPath();
    }

    return dir;
}

wxString wxPDirSelector(const wxString& configPath,
                        const wxString& message,
                        const wxString& pathDefault,
                        wxWindow *parent,
                        wxConfigBase *config)
{
    // the directory the user chooses
    wxString dir;

    if ( !configPath.empty() )
    {
        wxPHelper persist(configPath, _T("DirPrompt"), config);
        wxString configKey = persist.GetKey();

        // if config was NULL, wxPHelper already has the global one
        config = persist.GetConfig();

        wxString path = pathDefault;
        if ( path.empty() && !configPath.empty() )
        {
            // use the last directory
            path = config->Read(configKey, _T(""));
        }

        dir = wxGetDirectory(parent, message, path);

        if ( !dir.empty() )
        {
            persist.ChangePath();
            config->Write(configKey, dir);
        }
    }
    else
    {
        dir = wxGetDirectory(parent, message, pathDefault);
    }

    return dir;
}

// ----------------------------------------------------------------------------
// misc helper functions
// ----------------------------------------------------------------------------

static int TranslateBtnIdToMsgBox(int rc)
{
    switch ( rc ) {
        case wxID_YES:
            return wxYES;

        case wxID_NO:
            return wxNO;

        case wxID_OK:
            return wxOK;

        default:
            wxFAIL_MSG(_T("unexpected button id in TranslateBtnIdToMsgBox."));
            // fall through nevertheless

        case wxID_CANCEL:
            return wxCANCEL;
    }
}

// the numeric values of wxYES, wxNO and others have changed in wxWindows in
// 2.3 and we use the conversion function to deal with the files produced by
// the earlier versions
static int ConvertId(long rc)
{
    switch ( rc )
    {
#if wxCHECK_VERSION(2, 3, 1)
        case 0x0020:
#endif
        case wxYES:
            return wxYES;

#if wxCHECK_VERSION(2, 3, 1)
        case 0x0040:
#endif
        case wxNO:
            return wxNO;

        case DONT_PREDISABLE:
            return 0;

        default:
            wxFAIL_MSG( _T("unexpected persistent message box value") );
            // fall through

        case wxOK:
        case 0:
            return rc;
    }
}

/* vi: set ts=4 sw=4: */
