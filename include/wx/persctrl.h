/////////////////////////////////////////////////////////////////////////////
// Name:        wx/persctrl.h
// Purpose:     persistent classes declarations
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
#   pragma interface "persctrl.h"
#endif

// forward declarations
class wxConfigBase;
class wxPHelper;

// the headers we really need
#ifndef USE_PCH
#  include <wx/defs.h>
#  include <wx/checkbox.h>
#  include <wx/choice.h>
#  include <wx/control.h>
#  include <wx/combobox.h>
#  include <wx/listbox.h>
#  include <wx/radiobox.h>
#endif // USE_PCH

#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#if wxUSE_LISTBOOK
    #include <wx/listbook.h>
#endif

// we can be compiled inside wxWin or not
#ifdef WXMAKINGDLL
    #define WXDLLMAYEXP WXDLLEXPORT
#else
    #define WXDLLMAYEXP
#endif

// ----------------------------------------------------------------------------
// a helper class for persistent controls
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxPControls
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
       probably enough. By default, the full path for theconfig entry is
       constructed by combining the prefix returned by wxPControls::
       GetSettingsPath() with the class specific part (e.g. "NotebookPages")
       and then appending this parameter to it. But if the path starts with
       a backslash, it's used as is so you can store the information anywhere
       (but if you only want to change the common prefix which is the root for
       all wxPControl entries you may just use wxPControls::SetSettingsPath)

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

class WXDLLMAYEXP wxPNotebook : public wxNotebook
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
    static const wxChar *ms_path;

    void RestorePage();

    bool       m_bFirstTime;  // FIXME hack used in OnSize()
    wxPHelper *m_persist;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPNotebook)
};

// ----------------------------------------------------------------------------
// exactly the same as wxPNotebook but for wxListbook
// ----------------------------------------------------------------------------

#if wxUSE_LISTBOOK

class WXDLLMAYEXP wxPListbook : public wxListbook
{
public:
    // ctors and dtor
    wxPListbook();

    wxPListbook(const wxString& configPath,
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

    ~wxPListbook();

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

    // callbacks
        // when we're created, we restore our page
    void OnCreate(wxWindowCreateEvent& event);

protected:
    static const wxChar *ms_path;

    void RestorePage();

    bool       m_bFirstTime;  // FIXME hack used in OnSize()
    wxPHelper *m_persist;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPListbook)
};

#endif // wxUSE_LISTBOOK

// ----------------------------------------------------------------------------
// persistent text entry zone: it remembers a given number of strings entered
// ----------------------------------------------------------------------------

// Although this class inherits from wxComboBox, it's designed to be used
// rather as a (single line) text entry, as its name suggests. In particular,
// you shouldn't add/remove strings from this combobox manually (but using
// SetValue (and, of course, GetValue) is perfectly ok).
class WXDLLMAYEXP wxPTextEntry : public wxComboBox
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

    DECLARE_NO_COPY_CLASS(wxPTextEntry)
};

// ----------------------------------------------------------------------------
// A persistent choice control: remembers the last string choosen
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxPChoice : public wxChoice
{
public:
    // ctors
        // default, use Create() after it
    wxPChoice();
        // standard ctor
    wxPChoice(const wxString& configPath,
              wxWindow *parent,
              wxWindowID id = -1,
              const wxPoint &pos = wxDefaultPosition,
              const wxSize &size = wxDefaultSize,
              int n = 0,
              const wxString *items = NULL,
              long style = 0,
              const wxValidator& validator = wxDefaultValidator,
              wxConfigBase *config = NULL);
        // pseudo ctor
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                int n = 0,
                const wxString *items = NULL,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                wxConfigBase *config = NULL);

    // dtor saves the settings
    virtual ~wxPChoice();

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

    // callbacks
        // when we're resized the first time we restore our page
    void OnSize(wxSizeEvent& event);

protected:
    bool       m_bFirstTime;  // FIXME hack used in OnSize()
    wxPHelper *m_persist;

    // do remember/restore the selection
       // retrieve the column widths from config
    void RestoreSelection();
       // save the column widths to config
    void SaveSelection();

private:
    static const wxChar *ms_path;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPChoice)
};

// ----------------------------------------------------------------------------
// A splitter control which remembers its last position
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxPSplitterWindow : public wxSplitterWindow
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
    static const wxChar *ms_path;

    DECLARE_NO_COPY_CLASS(wxPSplitterWindow)
};

// ----------------------------------------------------------------------------
// a list control which remembers the widths of its columns
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxPListCtrl : public wxListCtrl
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

    // callbacks
        // when we're resized the first time we restore our page
    void OnSize(wxSizeEvent& event);

    // as an exception, these functions are public in this class to account
    // for situations when the list ctrl columns are deleted and then
    // recreated
       // retrieve the column widths from config
    void RestoreWidths();
       // save the column widths to config
    void SaveWidths();

protected:
    bool       m_bFirstTime;  // FIXME hack used in OnSize()
    wxPHelper *m_persist;

private:
    static const wxChar *ms_path;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPListCtrl)
};

// ----------------------------------------------------------------------------
// persistent checkbox remembers its last value
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxPCheckBox : public wxCheckBox
{
public:
    // ctors
        // default, use Create() after it
    wxPCheckBox();
        // standard ctor
    wxPCheckBox(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxString& label = _T(""),
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                wxConfigBase *config = NULL);
        // pseudo ctor
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxString& label = _T(""),
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                wxConfigBase *config = NULL);

    // dtor saves the settings
    virtual ~wxPCheckBox();

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

protected:
    // save and restore value from config
    void SaveValue();
    void RestoreValue();

    wxPHelper *m_persist;

private:
    static const wxChar *ms_path;

    DECLARE_NO_COPY_CLASS(wxPCheckBox)
};

// ----------------------------------------------------------------------------
// persistent listbox remembers its last selection
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxPListBox : public wxListBox
{
public:
    // ctors
        // default, use Create() after it
    wxPListBox();
        // standard ctor
    wxPListBox(const wxString& configPath,
               wxWindow *parent,
               wxWindowID id = -1,
               const wxPoint &pos = wxDefaultPosition,
               const wxSize &size = wxDefaultSize,
               int n = 0,
               const wxString *items = NULL,
               long style = 0,
               const wxValidator& validator = wxDefaultValidator,
               wxConfigBase *config = NULL);
        // pseudo ctor
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                int n = 0,
                const wxString *items = NULL,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                wxConfigBase *config = NULL);

    // dtor saves the settings
    virtual ~wxPListBox();

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);


    // restore the selection: this may be called manually if it needs to be
    // done before first wxEVT_SIZE, otherwise it's done automatically then
    void RestoreSelection();

protected:
    bool       m_bFirstTime;  // FIXME hack used in OnSize()
    wxPHelper *m_persist;

    // call RestoreSelection() once we're fully initialized
    void OnSize(wxSizeEvent& event);

    // save the current selection
    void SaveSelection();

private:
    static const wxChar *ms_path;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPListBox)
};

// ----------------------------------------------------------------------------
// persistent radiobox remembers its last selection
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxPRadioBox : public wxRadioBox
{
public:
    // ctors
        // default, use Create() after it
    wxPRadioBox();
        // standard ctor
    wxPRadioBox(const wxString& configPath,
               wxWindow *parent,
               wxWindowID id = -1,
               const wxString& title = _T(""),
               const wxPoint &pos = wxDefaultPosition,
               const wxSize &size = wxDefaultSize,
               int n = 0,
               const wxString *items = NULL,
               int majorDim = 0,
               long style = wxRA_HORIZONTAL,
               const wxValidator& validator = wxDefaultValidator,
               wxConfigBase *config = NULL);
        // pseudo ctor
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxString& title = _T(""),
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                int n = 0,
                const wxString *items = NULL,
                int majorDim = 0,
                long style = wxRA_HORIZONTAL,
                const wxValidator& validator = wxDefaultValidator,
                wxConfigBase *config = NULL);

    // dtor saves the settings
    virtual ~wxPRadioBox();

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

    // callbacks
        // when we're resized the first time we restore our selection
    void OnSize(wxSizeEvent& event);

protected:
    bool       m_bFirstTime;  // FIXME hack used in OnSize()
    wxPHelper *m_persist;

    // do remember/restore the selection
       // retrieve the column widths from config
    void RestoreSelection();
       // save the column widths to config
    void SaveSelection();

private:
    static const wxChar *ms_path;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPRadioBox)
};

// ----------------------------------------------------------------------------
// wxPTreeCtrl: remembers which of tree branches were expanded
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxPTreeCtrl : public wxTreeCtrl
{
public:
    // ctors
        // default, use Create() after it
    wxPTreeCtrl();
        // standard ctor
    wxPTreeCtrl(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT,
                const wxValidator &validator = wxDefaultValidator,
                wxConfigBase *config = NULL);
        // pseudo ctor
    bool Create(const wxString& configPath,
                wxWindow *parent,
                wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT,
                const wxValidator &validator = wxDefaultValidator,
                wxConfigBase *config = NULL);

    // dtor saves the settings
    virtual ~wxPTreeCtrl();

    // accessors
        // set the config object to use (must be !NULL)
    void SetConfigObject(wxConfigBase *config);
        // set the path to use (either absolute or relative)
    void SetConfigPath(const wxString& path);

    // operations
        // this function may be used to save the state of all expanded
        // branches under the given one
    wxString SaveExpandedBranches(const wxTreeItemId& itemRoot);
        // and this one restores the previously expanded branches: the string
        // passed to it must have been returned by SaveExpandedBranches()
    void RestoreExpandedBranches(const wxTreeItemId& itemRoot,
                                 const wxString& expState);

    // callbacks
        // when we're resized the first time we reexpand
    void OnSize(wxSizeEvent& event);

protected:
    bool       m_bFirstTime;  // FIXME hack used in OnSize()
    wxPHelper *m_persist;

    // do remember/restore the state of the branches
       // retrieve it from config
    void RestoreExpandedBranches();
       // save to config
    void SaveExpandedBranches();

    // SaveExpandedBranches() helper
    bool GetExpandedBranches(const wxTreeItemId& id, wxArrayString& branches);

private:
    static const wxChar *ms_path;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxPTreeCtrl)
};

// ----------------------------------------------------------------------------
// Persistent file selector functions: remember the last directory and file
// name used.
//
// See wxWindows docs and/or wx/filedlg.h for the meaning of all parameters
// except the first and the last one. Also, "defname" (default value)
// parameter is overriden by the last entry saved in the config, so it only
// matters when the function is called for the very first time - unless the
// flag wxFILEDLG_USE_FILENAME is given in flags in which case the name is used
// if provided.
// ----------------------------------------------------------------------------

enum
{
    wxFILEDLG_USE_FILENAME = 0x2000 // don't override with config setting
};

extern WXDLLMAYEXP wxString wxPFileSelector(const wxString& configPath,
                                            const wxString& title,
                                            const wxChar *defpath = NULL,
                                            const wxChar *defname = NULL,
                                            const wxChar *extension = NULL,
                                            const wxChar *filter = NULL,
                                            int flags = 0,
                                            wxWindow *parent = NULL,
                                            wxConfigBase *config = NULL);

// return the number of filenames selected, the filenames themselves are in
// the array passed by reference
extern WXDLLMAYEXP size_t wxPFilesSelector(wxArrayString& filenames,
                                           const wxString& configPath,
                                           const wxString& title,
                                           const wxChar *defpath = NULL,
                                           const wxChar *defname = NULL,
                                           const wxChar *extension = NULL,
                                           const wxChar *filter = NULL,
                                           int flags = 0,
                                           wxWindow *parent = NULL,
                                           wxConfigBase *config = NULL);

// ----------------------------------------------------------------------------
// Persistent directory selector: remember the last directory entered
// ----------------------------------------------------------------------------

// return the selected directory or an empty string if the dialog was
// cancelled
extern WXDLLMAYEXP wxString wxPDirSelector(const wxString& configPath,
                                           const wxString& message,
                                           const wxString& pathDefault = _T(""),
                                           wxWindow *parent = NULL,
                                           wxConfigBase *config = NULL);

// ----------------------------------------------------------------------------
// Persistent (a.k.a. "don't remind me again") message boxes functions.
//
// This function shows to the user a message box with a "don't show this
// message again" check box. If the user checks it, its state will be saved to
// the config object and the next call to this function will return the value
// which was selected the last time (Yes/No/Ok/Cancel...) without showing the
// message box at all.
//
// See wxPMessageBoxEnable() for how to reset it (i.e. make message box appear
// again) later if needed. You may provide wontShowAgain pointer if you want to
// know if the message box was disabled in the result of this call (i.e. it
// will not be set to true if it had been already disabled before, use
// wxPMessageBoxEnabled() for this).
// ----------------------------------------------------------------------------

// parameters specifying how this message may be disabled
struct WXDLLMAYEXP wxPMessageBoxParams
{
    wxPMessageBoxParams()
        : disableMsg(_("Don't show this message again "))
    {
        indexDisable = -1;
        dontDisableOnNo = false;
    }

    wxString disableMsg;
    wxArrayString disableOptions;
    int indexDisable;
    bool dontDisableOnNo;
};

extern WXDLLMAYEXP int
wxPMessageBox(const wxString& configPath,
              const wxString& message,
              const wxString& caption,
              long style = wxYES_NO | wxICON_QUESTION,
              wxWindow *parent = NULL,
              wxConfigBase *config = NULL,
              wxPMessageBoxParams *params = NULL);

// Return a non null value if the message box was previously disabled.
//
// The function can return wxYES, wxNO or wxOK if the message was disabled and
// 0 otherwise.
extern WXDLLMAYEXP int wxPMessageBoxIsDisabled(const wxString& configPath,
                                               wxConfigBase *config = NULL);

// make sure that the next call to wxPMessageBox(configPath) will show the
// message box (by erasing the stored answer in it)
extern WXDLLMAYEXP void wxPMessageBoxEnable(const wxString& configPath,
                                            wxConfigBase *config = NULL);

// disable the given message box by storing the given value for it (i.e. it
// will be returned by wxPMessageBox() call, must be wxYES/NO/OK)
extern WXDLLMAYEXP void wxPMessageBoxDisable(const wxString& configPath,
                                             int value,
                                             wxConfigBase *config = NULL);

#endif // _WX_PWINDOW_H_

