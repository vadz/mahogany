///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxOptionsDlg.cpp - M options dialog
// Purpose:     allows to easily change from one dialog all options
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"

#ifndef USE_PCH
# include "Mcommon.h"
#endif

#include <wx/log.h>
#include <wx/imaglist.h>
#include <wx/notebook.h>

#include "Mdefaults.h"

#include "MDialogs.h"

// first and last are shifted by -1, i.e. the range of fields for the page Foo
// is from ConfigField_FooFirst + 1 to ConfigField_FooLast inclusive.
//
// only the wxOptionsPage ctor knows about it, so if this is (for some reason)
// changed, it would be the only place to change.
enum ConfigFields
{
  // network
  ConfigField_NetworkFirst = -1,
  ConfigField_Username,
  ConfigField_Hostname,
  ConfigField_MailServer,
  ConfigField_NetworkLast = ConfigField_MailServer,

  // identity
  ConfigField_IdentFirst = ConfigField_NetworkLast,
  ConfigField_PersonalName,

  ConfigField_FromLabel,
  ConfigField_ToLabel,
  ConfigField_ShowCC,
  ConfigField_CCLabel,
  ConfigField_ShowBCC,
  ConfigField_BCCLabel,

  ConfigField_Signature,
  ConfigField_SignatureFile,
  ConfigField_SignatureSeparator,
  ConfigField_IdentLast = ConfigField_SignatureSeparator,

  // folders
  ConfigField_FoldersFirst = ConfigField_IdentLast,
  ConfigField_OpenFolders,
  ConfigField_MainFolder,
  ConfigField_FoldersLast = ConfigField_MainFolder,

  // python
  ConfigField_PythonFirst = ConfigField_FoldersLast,
  ConfigField_EnablePython,
  ConfigField_StartupScript,
  ConfigField_PythonLast = ConfigField_StartupScript,

  // other options
  ConfigField_OthersFirst = ConfigField_PythonLast,
  ConfigField_ShowLog,
  ConfigField_Splash,
  ConfigField_SplashDelay,
  ConfigField_DateFormat,
  ConfigField_OthersLast = ConfigField_DateFormat,

  // the end
  ConfigField_Max
};

// a structure giving the name of config key and it's default value
struct ConfigValueDefault
{
  ConfigValueDefault(const char *name_, long value)
    { bNumeric = TRUE; name = name_; lValue = value; }

  ConfigValueDefault(const char *name_, const char *value)
    { bNumeric = FALSE; name = name_; szValue = value; }

  long GetLong() const { wxASSERT( bNumeric ); return lValue; }
  const char *GetString() const { wxASSERT( !bNumeric ); return szValue; }

  const char *name;
  union
  {
    long        lValue;
    const char *szValue;
  };
  bool bNumeric;
};

// control ids
enum
{
  wxOptionsPage_BtnNew,
  wxOptionsPage_BtnModify,
  wxOptionsPage_BtnDelete
};

WX_DEFINE_ARRAY(wxControl *, ArrayControls);

// a button which is associated with a text control and which allows shows
// the file selection dialog and puts the filename chosen by the user into
// this text control
class wxBrowseButton : public wxButton
{
public:
  wxBrowseButton(wxTextCtrl *text, wxWindow *parent)
    : wxButton(parent, -1, ">>") { m_text = text; }

  void DoBrowse();

private:
  wxTextCtrl *m_text;
};

class wxOptionsDialog : public wxFrame
{
public:
  enum Icon
  {
    Icon_Network,
    Icon_Ident,
    Icon_Folders,
    Icon_Python,
    Icon_Others,
    Icon_Max
  };

  wxOptionsDialog(wxFrame *parent);

  // set dirty flag because our data somehow changed
  void SetDirty() { m_bDirty = TRUE; m_btnApply->Enable(TRUE); }

  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

  // callbacks
  void OnOK(wxCommandEvent& event);
  void OnApply(wxCommandEvent& event);
  void OnCancel(wxCommandEvent& event);

private:
  wxNotebook *m_notebook;
  wxButton   *m_btnOk,
             *m_btnApply;

  bool        m_bDirty;     // any changes?

  DECLARE_EVENT_TABLE()
};

class wxOptionsPage : public wxPanel
{
public:
  enum FieldType
  {
    Field_Text,   // one line text field
    Field_Number, // the same as text but accepts only digits
    Field_List,   // list of values - represented as a listbox
    Field_Bool,   // a checkbox
    Field_File,   // a text entry with a "Browse..." button
    Field_Max
  };

  struct FieldInfo
  {
    const char *label;
    FieldType   type;
    int         enable;  // disable this field if "enable" field is unchecked
  };

  // array of all field descriptions
  static FieldInfo ms_aFields[];

  // ctor will add this page to the notebook
  wxOptionsPage(wxNotebook *parent,
                const char *title,
                wxOptionsDialog::Icon image,
                size_t nFirst,
                size_t nLast);

  // these functions should never be called, but we can't make them pure
  // virtuals because they're defined in wxWindow already
  virtual bool TransferDataToWindow()
    { wxFAIL_MSG("pure virtual called."); return FALSE; }
  virtual bool TransferDataFromWindow()
    { wxFAIL_MSG("pure virtual called."); return FALSE; }

  // callbacks
  void OnBrowse(wxCommandEvent&);
  void OnChange(wxEvent&);
  void OnCheckboxChange(wxEvent& event);

  // enable/disable controls (better than OnUpdateUI here)
  void Refresh();

protected:
  void SetTopConstraint(wxLayoutConstraints *c, wxControl *last);
  
  wxListBox  *CreateListbox(const char *label, wxControl *last);
  wxCheckBox *CreateCheckBox(const char *label, long w, wxControl *last);
  wxTextCtrl *CreateTextWithLabel(const char *label, long w, wxControl *last,
                                  size_t nRightMargin = 0);
  wxTextCtrl *CreateFileEntry(const char *label, long w, wxControl *last);

  void        CreateControls();

  // range of our controls in ms_aFields
  size_t        m_nFirst,
                m_nLast;

  // the controls themselves (indexes in this array are shifted by m_nFirst
  // with respect to ConfigFields enum!)
  ArrayControls m_aControls;

  DECLARE_EVENT_TABLE()
};

class wxOptionsPageNetwork : public wxOptionsPage
{
public:
  wxOptionsPageNetwork(wxNotebook *parent)
    : wxOptionsPage(parent, _("Network"), wxOptionsDialog::Icon_Network,
                    ConfigField_NetworkFirst, ConfigField_NetworkLast) { }

  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
};

class wxOptionsPageIdent : public wxOptionsPage
{
public:
  wxOptionsPageIdent(wxNotebook *parent)
    : wxOptionsPage(parent, _("Identity"), wxOptionsDialog::Icon_Ident,
                    ConfigField_IdentFirst, ConfigField_IdentLast) { }

  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
};

class wxOptionsPageFolders : public wxOptionsPage
{
public:
  wxOptionsPageFolders(wxNotebook *parent)
    : wxOptionsPage(parent, _("Mail boxes"), wxOptionsDialog::Icon_Folders,
                    ConfigField_FoldersFirst, ConfigField_FoldersLast) { }

  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

  void OnNewFolder(wxCommandEvent&);
  void OnModifyFolder(wxCommandEvent&);
  void OnDeleteFolder(wxCommandEvent&);

private:
  DECLARE_EVENT_TABLE()
};

class wxOptionsPagePython : public wxOptionsPage
{
public:
  wxOptionsPagePython(wxNotebook *parent)
    : wxOptionsPage(parent, _("Python"), wxOptionsDialog::Icon_Python,
                    ConfigField_PythonFirst, ConfigField_PythonLast) { }

  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
};

class wxOptionsPageOthers : public wxOptionsPage
{
public:
  wxOptionsPageOthers(wxNotebook *parent)
    : wxOptionsPage(parent, _("Others"), wxOptionsDialog::Icon_Others,
                    ConfigField_OthersFirst, ConfigField_OthersLast) { }

  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(wxOptionsDialog, wxFrame)
  EVT_BUTTON(wxID_OK,     wxOptionsDialog::OnOK)
  EVT_BUTTON(wxID_APPLY,  wxOptionsDialog::OnApply)
  EVT_BUTTON(wxID_CANCEL, wxOptionsDialog::OnCancel)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPage, wxPanel)
  // NB: we assume that the only buttons we have are browse buttons
  EVT_BUTTON(-1, wxOptionsPage::OnBrowse)

  // any change should make us dirty
  EVT_CHECKBOX(-1, wxOptionsPage::OnCheckboxChange)
  EVT_TEXT(-1, wxOptionsPage::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxOptionsPageFolders, wxOptionsPage)
  EVT_BUTTON(wxOptionsPage_BtnNew,    wxOptionsPageFolders::OnNewFolder)
  EVT_BUTTON(wxOptionsPage_BtnModify, wxOptionsPageFolders::OnModifyFolder)
  EVT_BUTTON(wxOptionsPage_BtnDelete, wxOptionsPageFolders::OnDeleteFolder)
END_EVENT_TABLE()

// ============================================================================
// data: both of these arrays *must* be in sync with ConfigFields enum!
// ============================================================================

// the labels of all fields, their types and also the field they "depend on"
// (being dependent on another field only means that if that field is disabled
//  or unchecked, we're disabled too)
wxOptionsPage::FieldInfo wxOptionsPage::ms_aFields[] =
{
  // network config
  { "&Username",                    Field_Text,    -1,                        },
  { "&Hostname",                    Field_Text,    -1,                        },
  { "SMTP (&mail) server",          Field_Text,    -1,                        },

  // identity
  { "&Personal name",               Field_Text,    -1,                        },

  { "&From field label",            Field_Text,    -1,                        },
  { "&To field label",              Field_Text,    -1,                        },
  { "Show &CC field",               Field_Bool,    -1,                        },
  { "CC field &label",              Field_Text,    ConfigField_ShowCC         },
  { "Show &BCC field",              Field_Bool,    -1,                        },
  { "BCC field l&abel",             Field_Text,    ConfigField_ShowBCC        },

  { "&Use signature",               Field_Bool,    -1,                        },
  { "&Signature file",              Field_File,    ConfigField_Signature      },
  { "Use signature se&parator",     Field_Bool,    ConfigField_Signature      },

  // folders
  { "Folders to open on &startup",  Field_List,    -1,                        },
  { "Folder opened in &main frame", Field_Text,    -1,                        },


  // python
  { "&Enable Python",               Field_Bool,    -1,                        },
  { "&Startup script",              Field_File,    ConfigField_EnablePython   },

  // other options
  { "Show &log window",             Field_Bool,    -1,                        },
  { "&Splash screen at startup",    Field_Bool,    -1,                        },
  { "Splash screen &delay",         Field_Number,  ConfigField_Splash         },
  { "&Format for the date",         Field_Text,    -1,                        },
};

// @@@ ugly, ugly, ugly... config settings should be living in an array from
//     the beginning which would avoid us all these contorsions
#define CONFIG_ENTRY(name)  ConfigValueDefault(name, name##_D)

static const ConfigValueDefault gs_aConfigDefaults[] =
{
  // network
  CONFIG_ENTRY(MP_USERNAME),
  CONFIG_ENTRY(MP_HOSTNAME),
  CONFIG_ENTRY(MP_SMTPHOST),

  // identity
  CONFIG_ENTRY(MP_PERSONALNAME),

  CONFIG_ENTRY(MC_FROM_LABEL),
  CONFIG_ENTRY(MC_TO_LABEL),
  CONFIG_ENTRY(MP_SHOWCC),
  CONFIG_ENTRY(MC_CC_LABEL),
  CONFIG_ENTRY(MP_SHOWBCC),
  CONFIG_ENTRY(MC_BCC_LABEL),

  CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE),
  CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE_SEPARATOR),
  CONFIG_ENTRY(MP_COMPOSE_SIGNATURE),

  // folders
  CONFIG_ENTRY(MC_OPENFOLDERS),
  CONFIG_ENTRY(MC_MAINFOLDER),

  // python
  CONFIG_ENTRY(MC_USEPYTHON),
  CONFIG_ENTRY(MC_STARTUPSCRIPT),

  // other
  CONFIG_ENTRY(MC_SHOWLOG),
  CONFIG_ENTRY(MC_SHOWSPLASH),
  CONFIG_ENTRY(MC_SPLASHDELAY),
  CONFIG_ENTRY(MC_DATE_FMT),
};

// ============================================================================
// implementation
// ============================================================================
wxOptionsPage::wxOptionsPage(wxNotebook *notebook,
                             const char *title,
                             wxOptionsDialog::Icon image,
                             size_t nFirst,
                             size_t nLast)
             : wxPanel(notebook, -1)
{
  notebook->AddPage(this, title, FALSE /* don't select */, image);

  // see enum ConfigFields for "+1"
  m_nFirst = nFirst + 1;
  m_nLast = nLast + 1;

  CreateControls();

  SetAutoLayout(TRUE);
}

// the top item is positioned near the top of the page, the others are
// positioned from top to bottom, i.e. under the last one
void wxOptionsPage::SetTopConstraint(wxLayoutConstraints *c, wxControl *last)
{
  if ( last == NULL )
    c->top.SameAs(this, wxTop, 2*LAYOUT_Y_MARGIN);
  else {
    size_t margin = LAYOUT_Y_MARGIN;
    if ( last->IsKindOf(CLASSINFO(wxListBox)) ) {
      // listbox has a surrounding box, so leave more space
      margin *= 2;
    }

    c->top.Below(last, margin);
  }
}

wxTextCtrl *wxOptionsPage::CreateFileEntry(const char *label,
                                           long widthMax,
                                           wxControl *last)
{
  static size_t widthBtn = 0;
  if ( widthBtn == 0 ) {
    // calculate it only once, it's almost a constant
    widthBtn = 2*GetCharWidth();
  }

  // create the label and text zone, as usually
  wxTextCtrl *text = CreateTextWithLabel(label, widthMax, last,
                                         widthBtn + 2*LAYOUT_X_MARGIN);

  // and also create a button for browsing for file
  wxButton *btn = new wxBrowseButton(text, this);
  wxLayoutConstraints *c = new wxLayoutConstraints;
  SetTopConstraint(c, last);
  c->left.RightOf(text, LAYOUT_X_MARGIN);
  c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
  c->height.SameAs(text, wxHeight);
  btn->SetConstraints(c);

  return text;
}

// create a single-line text control with a label
wxTextCtrl *wxOptionsPage::CreateTextWithLabel(const char *label,
                                               long widthMax,
                                               wxControl *last,
                                               size_t nRightMargin)
{
  wxLayoutConstraints *c;

  // for the label
  c = new wxLayoutConstraints;
  c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
  SetTopConstraint(c, last);
  c->width.Absolute(widthMax);
  c->height.AsIs();
  wxStaticText *pLabel = new wxStaticText(this, -1, label,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxALIGN_RIGHT);
  pLabel->SetConstraints(c);

  // for the text control
  c = new wxLayoutConstraints;
  SetTopConstraint(c, last);
  c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
  c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN + nRightMargin);
  c->height.AsIs();
  wxTextCtrl *pText = new wxTextCtrl(this, -1, "");
  pText->SetConstraints(c);

  return pText;
}

// create a checkbox
wxCheckBox *wxOptionsPage::CreateCheckBox(const char *label,
                                          long widthMax,
                                          wxControl *last)
{
  static size_t widthCheck = 0;
  if ( widthCheck == 0 ) {
    // calculate it only once, it's almost a constant
    widthCheck = GetCharHeight() - 2; // @@ 2 is purely empiric...
  }

  wxCheckBox *checkbox = new wxCheckBox(this, -1, label,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxALIGN_RIGHT);

  wxLayoutConstraints *c = new wxLayoutConstraints;
  SetTopConstraint(c, last);
  c->width.AsIs();
  c->right.SameAs(this, wxLeft, -(int)(2*LAYOUT_X_MARGIN + widthMax
                                                         + widthCheck));
  c->height.AsIs();

  checkbox->SetConstraints(c);

  return checkbox;
}

// create a listbox and the buttons to work with it
// NB: we consider that there is only one listbox (at most) per page, so
//     the button ids are always the same
wxListBox *wxOptionsPage::CreateListbox(const char *label, wxControl *last)
{
  // a box around all this stuff
  wxStaticBox *box = new wxStaticBox(this, -1, label);

  wxLayoutConstraints *c;
  c = new wxLayoutConstraints;
  SetTopConstraint(c, last);
  c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
  c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
  c->height.PercentOf(this, wxHeight, 50);
  box->SetConstraints(c);

  // the buttons vertically on the right of listbox
  wxButton *button;
  static const char *aszLabels[] =
  {
    "&Add",
    "&Modify",
    "&Delete",
  };

  // determine the longest button label
  size_t nBtn;
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  long width, widthMax = 0;
  for ( nBtn = 0; nBtn < WXSIZEOF(aszLabels); nBtn++ ) {
    dc.GetTextExtent(aszLabels[nBtn], &width, NULL);
    if ( width > widthMax )
      widthMax = width;
  }

  widthMax += 15; // @@ loks better like this
  for ( nBtn = 0; nBtn < WXSIZEOF(aszLabels); nBtn++ ) {
    c = new wxLayoutConstraints;
    if ( nBtn == 0 )
      c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
    else
      c->top.Below(button, LAYOUT_Y_MARGIN);
    c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
    c->width.Absolute(widthMax);
    c->height.AsIs();
    button = new wxButton(this, wxOptionsPage_BtnNew + nBtn, aszLabels[nBtn]);
    button->SetConstraints(c);
  }

  // and the listbox itself
  wxListBox *listbox = new wxListBox(this, -1);
  c = new wxLayoutConstraints;
  c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
  c->left.SameAs(box, wxLeft, LAYOUT_X_MARGIN);
  c->right.LeftOf(button, LAYOUT_X_MARGIN);;
  c->bottom.SameAs(box, wxBottom, LAYOUT_Y_MARGIN);
  listbox->SetConstraints(c);

  return listbox;
}

void wxOptionsPage::CreateControls()
{
  size_t n;

  // first determine the longest label
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  long width, widthMax = 0;
  for ( n = m_nFirst; n < m_nLast; n++ ) {
    // do it only for text control labels
    switch ( ms_aFields[n].type ) {
      case Field_Number:
      case Field_File:
      case Field_Bool:
        // fall through: for this purpose (finding the longest label)
        // they're the same as text

      case Field_Text:
        break;

      default:
        // don't take into account the other types
        continue;
    }

    dc.GetTextExtent(ms_aFields[n].label, &width, NULL);
    if ( width > widthMax )
      widthMax = width;
  }

  // now create the controls
  wxControl *last = NULL; // last control created
  for ( n = m_nFirst; n < m_nLast; n++ ) {
    switch ( ms_aFields[n].type ) {
      case Field_File:
        last = CreateFileEntry(ms_aFields[n].label, widthMax, last);
        break;

      case Field_Number:
        // fall through -- for now they're the same as text

      case Field_Text:
        last = CreateTextWithLabel(ms_aFields[n].label, widthMax, last);
        break;

      case Field_List:
        last = CreateListbox(ms_aFields[n].label, last);
        break;

      case Field_Bool:
        last = CreateCheckBox(ms_aFields[n].label, widthMax, last);
        break;

      default:
        wxFAIL_MSG("unknown field type in CreateControls");
    }

    wxCHECK_RET( last, "control creation failed" );

    m_aControls.Add(last);
  }
}

void wxOptionsPage::OnBrowse(wxCommandEvent& event)
{
  wxBrowseButton *btn = (wxBrowseButton *)event.GetEventObject();
  btn->DoBrowse();
}

void wxOptionsPage::OnChange(wxEvent&)
{
  // find the frame we're in
  wxWindow *win = GetParent();
  while ( win && !win->IsKindOf(CLASSINFO(wxFrame)) ) {
    win = win->GetParent();
  }

  if ( win )
    ((wxOptionsDialog *)win)->SetDirty();
}

void wxOptionsPage::OnCheckboxChange(wxEvent& event)
{
  OnChange(event);

  Refresh();
}

void wxOptionsPage::Refresh()
{
  for ( size_t n = m_nFirst; n < m_nLast; n++ ) {
    int nCheckField = ms_aFields[n].enable;
    if ( nCheckField != -1 ) {
      wxASSERT( nCheckField > 0 && nCheckField < ConfigField_Max );

      // avoid signed/unsigned mismatch in expressions
      size_t nCheck = (size_t)nCheckField;
      wxASSERT( ms_aFields[nCheck].type == Field_Bool );
      wxASSERT( nCheck >= m_nFirst && nCheck < m_nLast );

      // enable only if the checkbox is checked
      wxCheckBox *checkbox = (wxCheckBox *)m_aControls[nCheck - m_nFirst];
      wxASSERT( checkbox->IsKindOf(CLASSINFO(wxCheckBox)) );

      wxControl *control = m_aControls[n - m_nFirst];
      bool bEnable = checkbox->GetValue();

      control->Enable(bEnable);

      switch ( ms_aFields[n].type ) {
        case Field_File:
          // for file entries, also disable the browse button
          {
            // @@ we assume that the control ids are consecutif
            long id = control->GetId() + 1;
            wxWindow *win = FindWindow(id);
        
            if ( win == NULL ) {
              wxFAIL_MSG("can't find browse button for the file entry zone");
            }
            else {
              win->Enable(bEnable);
            }
          }
          // fall through

        case Field_Text:
          // not only enable/disable it, but also make (un)editable because
          // it gives visual feedback
          wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );
          ((wxTextCtrl *)control)->SetEditable(bEnable);
          break;

        case Field_List:
          // also disable the buttons
          {
            long i;
            for ( i = wxOptionsPage_BtnNew; i <= wxOptionsPage_BtnNew; i++ ) {
              wxWindow *win = FindWindow(i);
              if ( win ) {
                win->Enable(bEnable);
              }
              else {
                wxFAIL_MSG("can't find listbox buttons by id");
              }
            }
          }
          break;
      }
    }
    // this field is always enabled
  }
}

// ----------------------------------------------------------------------------
// wxOptionsPageNetwork
// ----------------------------------------------------------------------------
bool wxOptionsPageNetwork::TransferDataToWindow()
{
  return TRUE;
}

bool wxOptionsPageNetwork::TransferDataFromWindow()
{
  return TRUE;
}

// ----------------------------------------------------------------------------
// wxOptionsPageIdent
// ----------------------------------------------------------------------------
bool wxOptionsPageIdent::TransferDataToWindow()
{
  return TRUE;
}

bool wxOptionsPageIdent::TransferDataFromWindow()
{
  return TRUE;
}

// ----------------------------------------------------------------------------
// wxOptionsPageFolders
// ----------------------------------------------------------------------------
bool wxOptionsPageFolders::TransferDataToWindow()
{
  return TRUE;
}

bool wxOptionsPageFolders::TransferDataFromWindow()
{
  return TRUE;
}

void wxOptionsPageFolders::OnNewFolder(wxCommandEvent&)
{
}

void wxOptionsPageFolders::OnModifyFolder(wxCommandEvent&)
{
}

void wxOptionsPageFolders::OnDeleteFolder(wxCommandEvent&)
{
}

// ----------------------------------------------------------------------------
// wxOptionsPagePython
// ----------------------------------------------------------------------------
bool wxOptionsPagePython::TransferDataToWindow()
{
  return TRUE;
}

bool wxOptionsPagePython::TransferDataFromWindow()
{
  return TRUE;
}

// ----------------------------------------------------------------------------
// wxOptionsPageOthers
// ----------------------------------------------------------------------------
bool wxOptionsPageOthers::TransferDataToWindow()
{
  return TRUE;
}

bool wxOptionsPageOthers::TransferDataFromWindow()
{
  return TRUE;
}

// ----------------------------------------------------------------------------
// wxOptionsDialog
// ----------------------------------------------------------------------------
wxOptionsDialog::wxOptionsDialog(wxFrame *parent)
               : wxFrame(parent, -1, wxString(_("M Options")),
                         wxDefaultPosition, wxSize(450, 350),
                         wxDEFAULT_FRAME_STYLE | wxDIALOG_MODAL)
{
  wxLayoutConstraints *c;  
  SetAutoLayout(TRUE);

  // calculate the controls size
  // ---------------------------

  // basic unit is the height of a char, from this we fix the sizes of all 
  // other controls
  int hBtn = TEXT_HEIGHT_FROM_LABEL(GetCharHeight() - 3), // @@ -3 is empiric
      wBtn = BUTTON_WIDTH_FROM_HEIGHT(hBtn);

  // create the panel
  // ----------------
  wxPanel *panel = new wxPanel(this, -1);
  panel->SetAutoLayout(TRUE);
  c = new wxLayoutConstraints;
  c->left.SameAs(this, wxLeft);
  c->right.SameAs(this, wxRight);
  c->top.SameAs(this, wxTop);
  c->bottom.SameAs(this, wxBottom);
  panel->SetConstraints(c);

  // create the notebook
  // -------------------

  // create the control itself
  m_notebook = new wxNotebook(panel, -1);
  c = new wxLayoutConstraints;
  c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
  c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
  c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
  c->bottom.SameAs(panel, wxBottom, 2*LAYOUT_Y_MARGIN + hBtn);
  m_notebook->SetConstraints(c);

  // create and fill the imagelist
  static const char *aszImages[] =
  { 
    // should be in sync with the corresponding enum
    "network", "ident", "folders", "python", "miscopt"
  };

  wxASSERT( WXSIZEOF(aszImages) == Icon_Max );  // don't forget to update both!

  wxImageList *imageList = new wxImageList(32, 32, FALSE, WXSIZEOF(aszImages));
  size_t n;
  for ( n = 0; n < Icon_Max; n++ ) {
    imageList->Add(wxBitmap(aszImages[n]));
  }

  m_notebook->SetImageList(imageList);

  // create and add the pages
  wxASSERT( WXSIZEOF(wxOptionsPage::ms_aFields) == ConfigField_Max );

  (void)new wxOptionsPageNetwork(m_notebook);
  (void)new wxOptionsPageIdent(m_notebook);
  (void)new wxOptionsPageFolders(m_notebook);
  (void)new wxOptionsPagePython(m_notebook);
  (void)new wxOptionsPageOthers(m_notebook);

  // create the buttons
  // ------------------

  // we need to create them from left to right to have the correct tab order
  // (although it would have been easier to do it from right to left)
  m_btnOk = new wxButton(panel, wxID_OK, _("OK"));
  m_btnOk->SetDefault();
  c = new wxLayoutConstraints;
  c->left.SameAs(panel, wxRight, -3*(LAYOUT_X_MARGIN + wBtn));
  c->width.Absolute(wBtn);
  c->height.Absolute(hBtn);
  c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
  m_btnOk->SetConstraints(c);

  wxButton *btn = new wxButton(panel, wxID_CANCEL, _("Cancel"));
  c = new wxLayoutConstraints;
  c->left.SameAs(panel, wxRight, -2*(LAYOUT_X_MARGIN + wBtn));
  c->width.Absolute(wBtn);
  c->height.Absolute(hBtn);
  c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
  btn->SetConstraints(c);

  m_btnApply = new wxButton(panel, wxID_APPLY, _("&Apply"));
  c = new wxLayoutConstraints;
  c->left.SameAs(panel, wxRight, -(LAYOUT_X_MARGIN + wBtn));
  c->width.Absolute(wBtn);
  c->height.Absolute(hBtn);
  c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
  m_btnApply->SetConstraints(c);
  m_btnApply->Enable(FALSE);  // initially, there is nothing to apply

  // set position
  // ------------
  SetSizeHints(6*wBtn, 15*hBtn);
  Centre(wxCENTER_FRAME | wxBOTH);

  TransferDataToWindow();

  m_bDirty = FALSE;
  for ( size_t nPage = 0; nPage < Icon_Max; nPage++ ) {
    ((wxOptionsPage *)m_notebook->GetPage(nPage))->Refresh();
  }
}

bool wxOptionsDialog::TransferDataToWindow()
{
  for ( size_t nPage = 0; nPage < Icon_Max; nPage++ ) {
    if ( !m_notebook->GetPage(nPage)->TransferDataToWindow() ) {
      return FALSE;
    }
  }

  return TRUE;
}

bool wxOptionsDialog::TransferDataFromWindow()
{
  for ( size_t nPage = 0; nPage < Icon_Max; nPage++ ) {
    if ( !m_notebook->GetPage(nPage)->TransferDataFromWindow() ) {
      return FALSE;
    }
  }

  return TRUE;
}

void wxOptionsDialog::OnOK(wxCommandEvent& event)
{
  if ( !m_bDirty || TransferDataFromWindow() ) {
    Close();
  }
}

void wxOptionsDialog::OnApply(wxCommandEvent& event)
{
  TransferDataFromWindow();
  m_bDirty = FALSE;
  m_btnApply->Enable(FALSE);
}

void wxOptionsDialog::OnCancel(wxCommandEvent& event)
{
  Close();
}

// ----------------------------------------------------------------------------
// wxBrowseButton
// ----------------------------------------------------------------------------

void wxBrowseButton::DoBrowse()
{
  // get the last position
  wxString strLastDir, strLastFile, strLastExt, strPath = m_text->GetValue();
  wxSplitPath(strPath, &strLastDir, &strLastFile, &strLastExt);

  wxFileDialog dialog(this, "",
                      strLastDir, strLastFile,
                      _("All files (*.*)|*.*"),
                      wxHIDE_READONLY | wxFILE_MUST_EXIST);

  if ( dialog.ShowModal() == wxID_OK ) {
    m_text->SetValue(dialog.GetPath());
  }
}

// ----------------------------------------------------------------------------
// our public interface is just this simple function
// ----------------------------------------------------------------------------
wxFrame *ShowOptionsDialog(wxFrame *parent)
{
  wxOptionsDialog *dlg = new wxOptionsDialog(parent);
  dlg->Show(TRUE);

  return dlg;
}
