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
#   include  "wx/window.h"
#   include  "wx/dialog.h"
#   include  "wx/checkbox.h"
#   include  "wx/utils.h"
#   include  "wx/layout.h"
#   include  "wx/button.h"
#   include  "wx/stattext.h"
#   include  "wx/statbmp.h"
#   include  "wx/intl.h"
#   include  "wx/dcclient.h"
#   include  "wx/settings.h"
#   include  "wx/statbox.h"
#   include  "wx/filedlg.h"
#endif //WX_PRECOMP

#include "wx/log.h"

#include "wx/config.h"
#include "wx/persctrl.h"

#ifndef MAX
#   define   MAX(a,b) (((a) > (b))?(a):(b))
#endif

// use icon in msg box? 
#define USE_ICON

// ----------------------------------------------------------------------------
// icons
// ----------------------------------------------------------------------------

#ifdef M_PREFIX
#   include   "Mcommon.h"
#   include   "kbList.h"
#   include   "gui/wxIconManager.h"
#   include   "MApplication.h"
#else
// MSW icons are in the ressources, for all other platforms - in XPM files
#ifndef __WXMSW__
#      include "wx/generic/info.xpm"
#      include "wx/generic/question.xpm"
#      include "wx/generic/warning.xpm"
#      include "wx/generic/error.xpm"
#endif // __WXMSW__
#endif // M_PREFIX

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxPNotebook, wxNotebook)
    EVT_SIZE(wxPNotebook::OnSize)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxPListCtrl, wxListCtrl)
    EVT_SIZE(wxPListCtrl::OnSize)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define LAYOUT_X_MARGIN       5
#define LAYOUT_Y_MARGIN       5

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
    void SetConfig(wxConfigBase *config) { m_config = config; }
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

void wxPHelper::SetPath(const wxString& path, const wxString& prefix)
{
    wxCHECK_RET( !path.IsEmpty(), "empty path in persistent ctrl code" );

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
        if ( m_path.IsEmpty() || (m_path.Last() != '/') ) {
            m_path += '/';
        }

        m_path << prefix << '/' << strPath;
        m_key = strKey;
    }
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
    m_config->SetPath(m_path);

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
const char *wxPNotebook::ms_path = "NotebookPages";

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
        if ( (nPage >= 0) && (nPage < GetPageCount()) ) {
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
    m_persist = new wxPHelper(configPath, "", config);
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
    m_persist->SetPath(path, "");
}

// ----------------------------------------------------------------------------
// wxPSplitterWindow
// ----------------------------------------------------------------------------

const char *wxPSplitterWindow::ms_path = "SashPositions";

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
// wxPListBox
// ----------------------------------------------------------------------------

const char *wxPListCtrl::ms_path = "ListCtrlColumns";

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
        if ( !str.IsEmpty() )
        {
            int countCol = GetColumnCount();
            char *p = str.GetWriteBuf(str.Len());
            for ( int col = 0; col < countCol; col++ )
            {
                if ( IsEmpty(p) )
                    break;

                char *end = strchr(p, ':');
                if ( end )
                    *end = '\0';    // temporarily truncate

                int width;
                if ( sscanf(p, "%d", &width) == 1 )
                    SetColumnWidth(col, width);
                else
                    wxFAIL_MSG("wxPListCtrl: corrupted config entry?");

                if ( end )
                    p = end + 1;
                else
                    break;
            }

            str.UngetWriteBuf();
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
        if ( !str.IsEmpty() )
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
                     bool persistent = TRUE);

    // accessors
    bool DontShowAgain() const { return m_checkBox->GetValue(); }

    // callbacks
    void OnButton(wxCommandEvent& event);

private:
    wxCheckBox *m_checkBox;

    long        m_dialogStyle;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxPMessageDialog, wxDialog)
    EVT_BUTTON(wxID_YES,    wxPMessageDialog::OnButton)
    EVT_BUTTON(wxID_NO,     wxPMessageDialog::OnButton)
    EVT_BUTTON(wxID_CANCEL, wxPMessageDialog::OnButton)
    EVT_BUTTON(wxID_OK,     wxPMessageDialog::OnButton)
END_EVENT_TABLE()

wxPMessageDialog::wxPMessageDialog(wxWindow *parent,
                                   const wxString& message,
                                   const wxString& caption,
                                   long style, bool persistent)
                : wxDialog(parent, -1, caption,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL)
{
    m_dialogStyle = style;  // we need it in OnButton

    wxLayoutConstraints *c;
    SetAutoLayout(TRUE);

    // the static box should be created first - otherwise it will hide the
    // controls which are disposed on top of it
    wxStaticBox *box = new wxStaticBox(this, -1, "");

    // create an icon
    enum
    {
        Icon_Information,
        Icon_Question,
        Icon_Warning,
        Icon_Error
    } which;
    
#ifdef M_PREFIX
#ifdef __WXMSW__
    static char *icons[] =
    {
        "wxICON_INFO",
        "wxICON_QUESTION",
        "wxICON_WARNING",
        "wxICON_ERROR",
    };
#else // XPM icons
    static char *icons[] =
    {
        "msg_info",
        "msg_question",
        "msg_warning",
        "msg_error"
    };
#endif
#else
#ifdef __WXMSW__
    static char *icons[] =
    {
        "wxICON_INFO",
        "wxICON_QUESTION",
        "wxICON_WARNING",
        "wxICON_ERROR",
    };
#else // XPM icons
    static char **icons[] =
    {
        info,
        question,
        warning,
        error,
    };
#endif // !XPM/XPM
#endif // M_PREFIX
    
    if ( style & wxICON_EXCLAMATION )
        which = Icon_Warning;
    else if ( style & wxICON_HAND )
        which = Icon_Error;
    else if ( style & wxICON_QUESTION )
        which = Icon_Question;
    else
        which = Icon_Information;

#ifdef USE_ICON
#   ifdef M_PREFIX
    wxStaticBitmap *icon = new wxStaticBitmap(this, -1, ICON(icons[which]));
#   else
    wxStaticBitmap *icon = new wxStaticBitmap(this, -1, wxIcon(icons[which]));
#   endif
    const int iconSize = icon->GetIcon().GetWidth();
#endif // use icon
    
    // split the message in lines
    // --------------------------
    wxClientDC dc(this);
    dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));

    wxArrayString lines;
    wxString curLine;
    long width, widthTextMax = 0;
    for ( const char *pc = message; ; pc++ ) {
        if ( *pc == '\n' || *pc == '\0' ) {
            dc.GetTextExtent(curLine, &width, NULL);
            if ( width > widthTextMax )
                widthTextMax = width;

            lines.Add(curLine);

            if ( *pc == '\n' ) {
               curLine.Empty();
            }
            else {
               // the end of string
               break;
            }
        }
        else {
            curLine += *pc;
        }
    }

    // calculate the total dialog size
    enum
    {
        Btn_Ok,
        Btn_Yes,
        Btn_No,
        Btn_Cancel,
        Btn_Max
    };
    wxButton *buttons[Btn_Max] = { NULL, NULL, NULL, NULL };
    int nDefaultBtn = -1;

    // some checks are in order...
    wxASSERT_MSG( !(style & wxOK) || !(style & wxYES_NO),
                  "don't create dialog with both Yes/No and Ok buttons!" );

    wxASSERT_MSG( (style & wxOK ) || (style & wxYES_NO),
                  "don't create dialog with only the Cancel button!" );

    if ( style & wxYES_NO ) {
       buttons[Btn_Yes] = new wxButton(this, wxID_YES, _("Yes"));
       buttons[Btn_No] = new wxButton(this, wxID_NO, _("No"));


       if(style & wxNO_DEFAULT)
          nDefaultBtn = Btn_No;
       else
          nDefaultBtn = Btn_Yes;
    }

    if (style & wxOK) {
        buttons[Btn_Ok] = new wxButton(this, wxID_OK, _("OK"));

        if ( nDefaultBtn == -1 )
            nDefaultBtn = Btn_Ok;
    }

    if (style & wxCANCEL) {
        buttons[Btn_Cancel] = new wxButton(this, wxID_CANCEL, _("Cancel"));
    }

    // get the longest caption and also calc the number of buttons
    size_t nBtn, nButtons = 0;
    long widthBtnMax = 0;
    for ( nBtn = 0; nBtn < Btn_Max; nBtn++ ) {
        if ( buttons[nBtn] ) {
            nButtons++;
            dc.GetTextExtent(buttons[nBtn]->GetLabel(), &width, NULL);
            if ( width > widthBtnMax )
                widthBtnMax = width;
        }
    }

    // now we can place the buttons
    if ( widthBtnMax < 75 )
        widthBtnMax = 75;
    else
        widthBtnMax += 10;
    long heightButton = widthBtnMax*23/75;

    long heightTextLine = 0;
    wxString textCheckbox = _("Don't show this message again ");
    dc.GetTextExtent(textCheckbox, &width, &heightTextLine);

    if(persistent)
    {
       // extra space for the check box
       width += 15;

       // *1.2 baselineskip
       heightTextLine *= 12;
       heightTextLine /= 10;

    }
    size_t nLineCount = lines.Count();

    long widthButtonsTotal = nButtons * (widthBtnMax + LAYOUT_X_MARGIN) -
                             LAYOUT_X_MARGIN;

    // the initial (and minimal possible) size of the dialog
    long widthDlg = MAX(widthTextMax + iconSize + 4*LAYOUT_X_MARGIN,
                        MAX(widthButtonsTotal, width)) +
                    6*LAYOUT_X_MARGIN,
         heightDlg = 12*LAYOUT_Y_MARGIN + heightButton +
                     heightTextLine*(nLineCount + 1);

    // create the controls
    // -------------------

    // a box around text and buttons
    c = new wxLayoutConstraints;
    c->top.SameAs(this, wxTop/*, LAYOUT_Y_MARGIN*/);
    c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
    c->bottom.SameAs(this, wxBottom, heightTextLine + 3*LAYOUT_Y_MARGIN);
    c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
    box->SetConstraints(c);

    c = new wxLayoutConstraints;
    c->width.Absolute(iconSize);
    c->height.Absolute(iconSize);
    c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
    c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);

#ifdef USE_ICON
    icon->SetConstraints(c);
#endif

    wxStaticText *text = NULL;
    for ( size_t nLine = 0; nLine < nLineCount; nLine++ ) {
        c = new wxLayoutConstraints;
        if ( text == NULL )
            c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
        else
            c->top.Below(text);

#ifdef USE_ICON
        c->left.RightOf(icon, 2*LAYOUT_X_MARGIN);
#else
        c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
#endif

        c->width.Absolute(widthTextMax);
        c->height.Absolute(heightTextLine);
        text = new wxStaticText(this, -1, lines[nLine]);
        text->SetConstraints(c);
    }

    // create the buttons
    wxButton *btnPrevious = (wxButton *)NULL;
    for ( nBtn = 0; nBtn < Btn_Max; nBtn++ ) {
        if ( buttons[nBtn] ) {
            c = new wxLayoutConstraints;

            if ( btnPrevious ) {
                c->left.RightOf(btnPrevious, LAYOUT_X_MARGIN);
            }
            else {
                c->left.SameAs(this, wxLeft,
                               (widthDlg - widthButtonsTotal) / 2);
            }

            c->width.Absolute(widthBtnMax);
            c->top.Below(text, 4*LAYOUT_Y_MARGIN);
            c->height.Absolute(heightButton);
            buttons[nBtn]->SetConstraints(c);

            btnPrevious = buttons[nBtn];
        }
    }

    // and finally create the check box
    if(persistent)
    {
       c = new wxLayoutConstraints;
       c->width.AsIs();
       c->height.AsIs();
       c->top.Below(box, LAYOUT_Y_MARGIN);
       c->centreX.SameAs(this, wxCentreX);
       m_checkBox = new wxCheckBox(this, -1, textCheckbox);
       m_checkBox->SetConstraints(c);
    }
    // set default button
    // ------------------

    if ( nDefaultBtn != -1 ) {
        buttons[nDefaultBtn]->SetDefault();
        buttons[nDefaultBtn]->SetFocus();
    }
    else {
        wxFAIL_MSG( "can't find default button for this dialog." );
    }

    // position the controls and the dialog itself
    // -------------------------------------------

    SetClientSize(widthDlg, heightDlg);

    // SetSizeHints() wants the size of the whole dialog, not just client size
    wxSize sizeTotal = GetSize(),
           sizeClient = GetClientSize();
    SetSizeHints(widthDlg + sizeTotal.GetWidth() - sizeClient.GetWidth(),
                 heightDlg + sizeTotal.GetHeight() - sizeClient.GetHeight());

    Layout();

    Centre(wxCENTER_FRAME | wxBOTH);
}

void wxPMessageDialog::OnButton(wxCommandEvent& event)
{
    // which button?
    switch ( event.GetId() ) {
        case wxID_YES:
        case wxID_NO:
        case wxID_OK:
            EndModal(event.GetId());
            break;

        default:
            wxFAIL_MSG("unexpected button id in wxPMessageDialog.");
            // fall through nevertheless

        case wxID_CANCEL:
            // allow to cancel the dialog with [Cancel] only if it's not a
            // "Yes/No" type dialog
            if ( (m_dialogStyle & wxYES_NO) != wxYES_NO ||
                 (m_dialogStyle & wxCANCEL) ) {
                EndModal(wxID_CANCEL);
            }
            break;
    }
}

static const char *gs_MessageBoxPath = "MessageBox";

int wxPMessageBox(const wxString& configPath,
                  const wxString& message,
                  const wxString& caption,
                  long style,
                  wxWindow *parent,
                  wxConfigBase *config)
{
   if ( configPath.Length() )
   {
      wxPHelper persist(configPath, gs_MessageBoxPath, config);
      persist.ChangePath();

      wxString configValue = persist.GetKey();

      long rc; // return code

      // if config was NULL, wxPHelper already has the global one
      config = persist.GetConfig();

      // disabled?
      if ( config && config->Exists(configValue) ) {
         // don't show it, it was disabled
         rc = config->Read(configValue, 0l);
      }
      else {
         // do show the msg box
         wxPMessageDialog dlg(parent, message, caption, style);
         rc = dlg.ShowModal();

         // ignore checkbox value if the dialog was cancelled
         if ( config && rc != wxID_CANCEL && dlg.DontShowAgain() ) {
            // next time we won't show it
            config->Write(configValue, rc);
         }
      }
      return rc;
   }
   else
   {
#ifdef __WXMSW__
      // use the system standard message box
      wxMessageDialog dlg(parent, message, caption, style);
#else
      // if no config path specified, we just work as a normal message 
      // dialog, but with nicer layout
      wxPMessageDialog dlg(parent, message, caption, style, false);
#endif

      return dlg.ShowModal();
   }
}

bool wxPMessageBoxEnabled(const wxString& configPath, wxConfigBase *config)
{
    wxPHelper persist(configPath, gs_MessageBoxPath, config);
    persist.ChangePath();
    wxString configValue = persist.GetKey();

    // if config was NULL, wxPHelper already has the global one
    config = persist.GetConfig();

    return !(config && config->Exists(configValue));
}

void wxPMessageBoxEnable(const wxString& configPath,
                         bool enable, 
                         wxConfigBase *config)
{
    wxPHelper persist(configPath, gs_MessageBoxPath, config);
    persist.ChangePath();
    wxString configValue = persist.GetKey();

    // if config was NULL, wxPHelper already has the global one
    config = persist.GetConfig();

    if ( enable ) {
        // (re)enable
        if ( config && config->Exists(configValue) ) {
           // delete stored value
           config->DeleteEntry(configValue);
        }
    }
    else {
       // disable
       if ( config ) {
           // assume it's a Yes/No dialog box
           config->Write(configValue, (long)wxID_YES);
       }
    }
}

// -----------------------------------------------------------------------------
// Persistent file selector boxes stuff
// -----------------------------------------------------------------------------

// get the path to use for file prompts settings
static void wxPFileSelectorHelper(const wxString& configPath,
                                  wxString& ourPath,
                                  wxString& ourValue)
{
    wxASSERT_MSG( ourPath.IsEmpty(), "should be passed in an empty string" );

    if ( configPath[0u] != '/' ) {
        // prepend some common prefix
        ourPath = "FilePrompts/";
    }

    ourPath += configPath.BeforeLast('/');
    ourValue = configPath.AfterLast('/');
}

wxString wxPFileSelector(const wxString& configPath,
                         const wxString& title,
                         const char *defpath,
                         const char *defname,
                         const char *defext,
                         const char *filter,
                         int flags,
                         wxWindow *parent,
                         wxConfigBase *config)
{
    wxString ourPath,           // path in the config
             configValueFile,   // name of the entry where filename is stored
             configValuePath;   // name of the entry where path is stored
    wxPFileSelectorHelper(configPath, ourPath, configValueFile);
    configValuePath << configValueFile << "Path";

    wxPHelper persist(ourPath, "", config);
    persist.ChangePath();

    // if config was NULL, wxPHelper already has the global one
    config = persist.GetConfig();

    // use the stored value for the default name/path and fall back to the
    // given one if there is none
    wxString defaultName, defaultPath, defName;
    defName << defname << '.' << defext;
    if ( config ) {
        defaultName = config->Read(configValueFile, defName);
        defaultPath = config->Read(configValuePath, defpath);
    }
    else {
        defaultName = defName;
        defaultPath = defpath;
    }

    // do show the file selector box
    wxString filename = wxFileSelector(title, defaultPath, defaultName, NULL,
                                       filter
                                        ? filter
                                        : wxFileSelectorDefaultWildcardStr,
                                       flags, parent);

    // and save the file name if it was entered
    if ( !filename.IsEmpty() && config ) {
        wxString path, name, ext;
        wxSplitPath(filename, &path, &name, &ext);

        if(ext.Length())
           name << '.' << ext;
        config->Write(configValueFile, name);
        config->Write(configValuePath, path);
    }

    return filename;
}
