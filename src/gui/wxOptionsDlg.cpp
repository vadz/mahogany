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
#   include   "Mcommon.h"
#   include   "MApplication.h"
#   include   "Profile.h"
#   include   "guidef.h"
#endif

#include   <wx/log.h>
#include   <wx/imaglist.h>
#include   <wx/notebook.h>
#include   <wx/dynarray.h>
#include   <wx/resource.h>
#include   <wx/persctrl.h>

#include   "MDialogs.h"
#include   "Mdefaults.h"
#include   "Mupgrade.h"
#include   "gui/wxIconManager.h"
#include   "MailFolder.h"

#define MCB_FOLDEROPEN_D            ""
#define MCB_FOLDERUPDATE_D          ""
#define MCB_FOLDEREXPUNGE_D         ""
#define MCB_FOLDERSETMSGFLAG_D      ""
#define MCB_FOLDERCLEARMSGFLAG_D    ""

// first and last are shifted by -1, i.e. the range of fields for the page Foo
// is from ConfigField_FooFirst + 1 to ConfigField_FooLast inclusive.
//
// only the wxOptionsPage ctor knows about it, so if this is (for some reason)
// changed, it would be the only place to change.
//
// if you modify this enum, you must modify the data below too (search for
// DONT_FORGET_TO_MODIFY to find it)
enum ConfigFields
{
   // network & identity
   ConfigField_IdentFirst = -1,
   ConfigField_Username,
   ConfigField_Hostname,
   ConfigField_ReturnAddress,
   ConfigField_MailServer,
   ConfigField_NewsServer,
   ConfigField_PersonalName,
   ConfigField_IdentLast = ConfigField_PersonalName,

  // compose
   ConfigField_ComposeFirst = ConfigField_IdentLast,
   ConfigField_FromLabel,
   ConfigField_ToLabel,
   ConfigField_ShowCC,
   ConfigField_CCLabel,
   ConfigField_ShowBCC,
   ConfigField_BCCLabel,

   ConfigField_WrapMargin,
   ConfigField_ReplyCharacter,

   ConfigField_Signature,
   ConfigField_SignatureFile,
   ConfigField_SignatureSeparator,
   ConfigField_XFace,
   ConfigField_XFaceFile,
   ConfigField_ComposeLast = ConfigField_XFaceFile,

   // folders
   ConfigField_FoldersFirst = ConfigField_ComposeLast,
   ConfigField_OpenFolders,
   ConfigField_MainFolder,
   ConfigField_FoldersLast = ConfigField_MainFolder,

#ifdef USE_PYTHON
  // python
   ConfigField_PythonFirst = ConfigField_FoldersLast,
   ConfigField_EnablePython,
   ConfigField_PythonPath,
   ConfigField_StartupScript,
   ConfigField_CallbackFolderOpen,
   ConfigField_CallbackFolderUpdate,
   ConfigField_CallbackFolderExpunge,
   ConfigField_CallbackSetFlag,
   ConfigField_CallbackClearFlag,
   ConfigField_PythonLast = ConfigField_CallbackClearFlag,
#else  // !USE_PYTHON
   ConfigField_PythonLast = ConfigField_FoldersLast,
#endif // USE_PYTHON

  // other options
   ConfigField_OthersFirst = ConfigField_PythonLast,
   ConfigField_ShowLog,
   ConfigField_Splash,
   ConfigField_SplashDelay,
   ConfigField_ConfirmExit,
   ConfigField_Browser,
   ConfigField_DateFormat,
   ConfigField_OthersLast = ConfigField_DateFormat,

  // the end
   ConfigField_Max
};

// a structure giving the name of config key and it's default value which may
// be either numeric or a string (no special type for boolean values right
// now)
struct ConfigValueDefault
{
   ConfigValueDefault(const char *name_, long value)
      { bNumeric = TRUE; name = name_; lValue = value; }

   ConfigValueDefault(const char *name_, const char *value)
      { bNumeric = FALSE; name = name_; szValue = value; }

   long GetLong() const { wxASSERT( bNumeric ); return lValue; }
   const char *GetString() const { wxASSERT( !bNumeric ); return szValue; }

   bool IsNumeric() const { return bNumeric; }

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

class wxOptionsNotebookBase : public wxPNotebook
{
public:
   wxOptionsNotebookBase(const wxString& configPath, wxWindow *parent)
      : wxPNotebook(configPath, parent, -1) { }

   virtual ~wxOptionsNotebookBase();
};

class wxOptionsNotebook : public wxOptionsNotebookBase
{
public:
   enum Icon
   {
      Icon_Ident,
      Icon_Compose,
      Icon_Folders,
#ifdef USE_PYTHON
      Icon_Python,
#endif
      Icon_Others,
      Icon_Max
   };

   wxOptionsNotebook(wxWindow *parent);
};

class wxFolderCreateNotebook : public wxOptionsNotebookBase
{
public:
   enum Icon
   {
      Icon_Compose,
      Icon_Folder,
      Icon_Max
   };

   wxFolderCreateNotebook(wxWindow *parent);
};

class wxOptionsDialog : public wxDialog
{
public:
   /// Which type of notebook do we want, selects the pages.
   enum Types
   {
      Type_Preferences,
      Type_FolderCreate
   };

   wxOptionsDialog(wxFrame *parent,
                   Types type = Type_Preferences);

   // set dirty flag because our data somehow changed
   void SetDirty() { m_bDirty = TRUE; m_btnApply->Enable(TRUE); }

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // callbacks
   void OnOK(wxCommandEvent& event);
   void OnApply(wxCommandEvent& event);
   void OnCancel(wxCommandEvent& event);

protected:
   /// ask for test
   void DoTest(void);

private:
   /// remember which type of dialog we are
   Types        m_type;
   wxPNotebook *m_notebook;
   wxButton    *m_btnOk,
               *m_btnApply;
   wxTextCtrl  *m_folderName; // only used for type Type_FolderCreate
   bool         m_bDirty;     // any changes?

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
      const char *label;   // which is shown in the dialog
      FieldType   type;    // see above
      int         enable;  // disable this field if "enable" field is unchecked
   };

   // array of all field descriptions
   static FieldInfo ms_aFields[];

   // ctor will add this page to the notebook
   wxOptionsPage(wxNotebook *parent,
                 const char *title,
                 ProfileBase *profile,
                 size_t nFirst,
                 size_t nLast);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // to change the profile associated with the page:
   void SetProfile(ProfileBase *profile) { m_Profile = profile; }

   // callbacks
   void OnBrowse(wxCommandEvent&);
   void OnChange(wxEvent&);
   void OnCheckboxChange(wxEvent& event);

   // enable/disable controls (better than OnUpdateUI here)
   void Refresh();

protected:
   // get the parent frame
   wxOptionsDialog *GetDialog() const;

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

   // we need a pointer to the profile to write to
   ProfileBase *m_Profile;

   // get the control with "right" index
   wxControl *GetControl(size_t /* ConfigFields */ n) const
      { return m_aControls[n - m_nFirst]; }

   // type safe access to the control text
   wxString GetControlText(size_t /* ConfigFields */ n) const
      {
         wxASSERT( GetControl(n)->IsKindOf(CLASSINFO(wxTextCtrl)) );

         return ((wxTextCtrl *)GetControl(n))->GetValue();
      }

private:
   // the controls themselves (indexes in this array are shifted by m_nFirst
   // with respect to ConfigFields enum!) - use GetControl()
   ArrayControls m_aControls;

   DECLARE_EVENT_TABLE()
      };

class wxOptionsPageCompose : public wxOptionsPage
{
public:
   wxOptionsPageCompose(wxNotebook *parent, ProfileBase *profile)
      : wxOptionsPage(parent, _("Compose"),
                      profile,
                      ConfigField_ComposeFirst, ConfigField_ComposeLast) { }

private:
};

class wxOptionsPageIdent : public wxOptionsPage
{
public:
   wxOptionsPageIdent(wxNotebook *parent, ProfileBase *profile)
      : wxOptionsPage(parent, _("Identity"),
                      profile,
                      ConfigField_IdentFirst, ConfigField_IdentLast) { }

private:
};

class wxOptionsPageFolders : public wxOptionsPage
{
public:
   wxOptionsPageFolders(wxNotebook *parent, ProfileBase *profile)
      : wxOptionsPage(parent, _("Mail boxes"),
                      profile,
                      ConfigField_FoldersFirst, ConfigField_FoldersLast) { }

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnIdle(wxIdleEvent&);

   void OnNewFolder(wxCommandEvent&);
   void OnModifyFolder(wxCommandEvent&);
   void OnDeleteFolder(wxCommandEvent&);

private:
   DECLARE_EVENT_TABLE()
      };

#ifdef USE_PYTHON
class wxOptionsPagePython : public wxOptionsPage
{
public:
   wxOptionsPagePython(wxNotebook *parent, ProfileBase *profile)
      : wxOptionsPage(parent, _("Python"),
                      profile,
                      ConfigField_PythonFirst, ConfigField_PythonLast) { }

private:
};
#endif // USE_PYTHON

class wxOptionsPageOthers : public wxOptionsPage
{
public:
   wxOptionsPageOthers(wxNotebook *parent, ProfileBase *profile)
      : wxOptionsPage(parent, _("Others"),
                      profile,
                      ConfigField_OthersFirst, ConfigField_OthersLast) { }

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(wxOptionsDialog, wxDialog)
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

   EVT_IDLE(wxOptionsPageFolders::OnIdle)
   END_EVENT_TABLE()

// ============================================================================
// data: both of these arrays *must* be in sync with ConfigFields enum!
// ============================================================================

// the labels of all fields, their types and also the field they "depend on"
// (being dependent on another field only means that if that field is disabled
//  or unchecked, we're disabled too)
//
// if you modify this array, search for DONT_FORGET_TO_MODIFY and modify data
// there too
   wxOptionsPage::FieldInfo wxOptionsPage::ms_aFields[] =
{
   // network config and identity
   { "&Username",                    Field_Text,    -1,                        },
   { "&Hostname",                    Field_Text,    -1,                        },
   { "&Return address",              Field_Text,    -1,                        },
   { "SMTP (&mail) server",          Field_Text,    -1,                        },
   { "NNTP (&news) server",          Field_Text,    -1,                        },
   { "&Personal name",               Field_Text,    -1,                        },

  // compose
   { "&From field label",            Field_Text,    -1,                        },
   { "&To field label",              Field_Text,    -1,                        },
   { "Show &CC field",               Field_Bool,    -1,                        },
   { "CC field &label",              Field_Text,    ConfigField_ShowCC         },
   { "Show &BCC field",              Field_Bool,    -1,                        },
   { "BCC field l&abel",             Field_Text,    ConfigField_ShowBCC        },

   { "&Wrap margin",                 Field_Number,  -1,                        },
   { "&Reply character",             Field_Text,    -1,                        },

   { "&Use signature",               Field_Bool,    -1,                        },
   { "&Signature file",              Field_File,    ConfigField_Signature      },
   { "Use signature se&parator",     Field_Bool,    ConfigField_Signature      },
   { "Us&e XFace",                   Field_Bool,    -1,                        },
   { "&XFace file",                  Field_File,    ConfigField_XFace},

   // folders
   { "Folders to open on &startup",  Field_List,    -1,                        },
   { "&Folder opened in main frame", Field_Text,    -1,                        },


#ifdef USE_PYTHON
  // python
   { "&Enable Python",               Field_Bool,    -1,                        },
   { "Python &Path",                 Field_Text,    ConfigField_EnablePython   },
   { "&Startup script",              Field_File,    ConfigField_EnablePython   },
   { "&Folder open callback",        Field_Text,    ConfigField_EnablePython   },
   { "Folder &update callback",      Field_Text,    ConfigField_EnablePython   },
   { "Folder e&xpunge callback",     Field_Text,    ConfigField_EnablePython   },
   { "Flag &set callback",           Field_Text,    ConfigField_EnablePython   },
   { "Flag &clear callback",         Field_Text,    ConfigField_EnablePython   },
#endif // USE_PYTHON

  // other options
   { "Show &log window",             Field_Bool,    -1,                        },
   { "&Splash screen at startup",    Field_Bool,    -1,                        },
   { "Splash screen &delay",         Field_Number,  ConfigField_Splash         },
   { "Confirm &exit",                Field_Bool,    -1,                        },
   { "&Browser program",             Field_File,    -1,                        },
   { "&Format for the date",         Field_Text,    -1,                        },
};

// @@@ ugly, ugly, ugly... config settings should be living in an array from
//     the beginning which would avoid us all these contorsions
#define CONFIG_ENTRY(name)  ConfigValueDefault(name, name##_D)

// if you modify this array, search for DONT_FORGET_TO_MODIFY and modify data
// there too
static const ConfigValueDefault gs_aConfigDefaults[] =
{
   // network and identity
   CONFIG_ENTRY(MP_USERNAME),
   CONFIG_ENTRY(MP_HOSTNAME),
   CONFIG_ENTRY(MP_RETURN_ADDRESS),
   CONFIG_ENTRY(MP_SMTPHOST),
   CONFIG_ENTRY(MP_NNTPHOST),
   CONFIG_ENTRY(MP_PERSONALNAME),

   CONFIG_ENTRY(MC_FROM_LABEL),
   CONFIG_ENTRY(MC_TO_LABEL),
   CONFIG_ENTRY(MP_SHOWCC),
   CONFIG_ENTRY(MC_CC_LABEL),
   CONFIG_ENTRY(MP_SHOWBCC),
   CONFIG_ENTRY(MC_BCC_LABEL),

   CONFIG_ENTRY(MP_COMPOSE_WRAPMARGIN),
   CONFIG_ENTRY(MP_REPLY_MSGPREFIX),

   CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE),
   CONFIG_ENTRY(MP_COMPOSE_SIGNATURE),
   CONFIG_ENTRY(MP_COMPOSE_USE_SIGNATURE_SEPARATOR),
   CONFIG_ENTRY(MP_COMPOSE_USE_XFACE),
   CONFIG_ENTRY(MP_COMPOSE_XFACE_FILE),

   // folders
   CONFIG_ENTRY(MC_OPENFOLDERS),
   CONFIG_ENTRY(MC_MAINFOLDER),

   // python
#ifdef USE_PYTHON
   CONFIG_ENTRY(MC_USEPYTHON),
   CONFIG_ENTRY(MC_PYTHONPATH),
   CONFIG_ENTRY(MC_STARTUPSCRIPT),
   CONFIG_ENTRY(MCB_FOLDEROPEN),
   CONFIG_ENTRY(MCB_FOLDERUPDATE),
   CONFIG_ENTRY(MCB_FOLDEREXPUNGE),
   CONFIG_ENTRY(MCB_FOLDERSETMSGFLAG),
   CONFIG_ENTRY(MCB_FOLDERCLEARMSGFLAG),
#endif // USE_PYTHON

   // other
   CONFIG_ENTRY(MC_SHOWLOG),
   CONFIG_ENTRY(MC_SHOWSPLASH),
   CONFIG_ENTRY(MC_SPLASHDELAY),
   CONFIG_ENTRY(MC_CONFIRMEXIT),
   CONFIG_ENTRY(MP_BROWSER),
   CONFIG_ENTRY(MC_DATE_FMT),

};

#undef CONFIG_ENTRY

// ============================================================================
// implementation
// ============================================================================

// OK, there is no working dialog editor and resizing doesn't work, so
// how about manually creating this panel? All others resize well.
#if 0
#include   "wxr/FOPanel.wxr"
//----------------------------------------------------------------------------
// a panel loading from a .wxr file
//----------------------------------------------------------------------------
class wxResourcePanel : public wxPanel
{
public:
   wxResourcePanel(wxString const &name, wxWindow *parent)
      {
         wxASSERT(LoadFromResource(parent, name));
      }
};

/** A panel displaying the core folder profile settings. This is just
    like the FolderOpen dialog in wxMDialogs which should probably be
    modified to use this.
*/
//FIXME: the renaming and de/activation of entries is still missing,
//copy the FODialog UpdateUI function here.
class wxFolderOpenPanel : public wxResourcePanel
{
public:
   virtual bool TransferDataToWindow(void);
   virtual bool TransferDataFromWindow(void);

   wxFolderOpenPanel(wxWindow *parent, ProfileBase *prof) :
      wxResourcePanel("FolderOpenPanel", parent)
      { m_profile = prof; }

private:
   ProfileBase *m_profile;
};

//FIXME: doesn't compile because "not in class declaration" ????
#if 0
bool
wxFolderOpenPanel::TranferDataToWindow(void)
{
   return true; // nothing for now, should check for existence
}

bool
wxFolderOpenPanel::TranferDataFromWindow(void)
{
   int type;
   wxRadioBox *ctrl = (wxRadioBox*)wxFindWindowByName("FolderType",this);
   ASSERT(ctrl);
   m_profile->writeEntry(MP_FOLDER_TYPE,type = ctrl->GetSelection());

   if(type == 0)
      m_profile->writeEntry(MP_FOLDER_PATH, "INBOX");
   else if(type == 1) // file
   {
      // interpret MailHost setting as pathname
      wxTextCtrl *tctrl = (wxTextCtrl *)wxFindWindowByName("MailHost",this);
      ASSERT(tctrl);
      m_profile->writeEntry(MP_FOLDER_PATH,tctrl->GetValue());
   }
   else
   {
      wxTextCtrl *tctrl = (wxTextCtrl *)wxFindWindowByName("UserID",this);
      ASSERT(tctrl);
      m_profile->writeEntry(MP_POP_LOGIN,tctrl->GetValue());

      tctrl = (wxTextCtrl *)wxFindWindowByName("Password",this);
      ASSERT(tctrl); 
      m_profile->writeEntry(MP_POP_PASSWORD,tctrl->GetValue());
   
      tctrl = (wxTextCtrl *)wxFindWindowByName("MailHost",this);
      ASSERT(tctrl);
      m_profile->writeEntry(MP_POP_HOST,tctrl->GetValue());
   }
   return true;
}
#endif

#endif

//----------------------------------------------------------------------------
// A panel for defining a new Folder
//----------------------------------------------------------------------------
class wxFolderPropertiesPage : public wxPanel
{
public:
   wxFolderPropertiesPage(wxNotebook *notebook, ProfileBase *profile);

   virtual bool TransferDataToWindow(void);
   virtual bool TransferDataFromWindow(void);
   /// update controls
   void UpdateUI(void);
   void OnEvent(wxCommandEvent& event);

protected:
   wxNotebook *  m_notebook;
   ProfileBase * m_profile;
   /// type of the folder
   int m_type;
   /// folder type
   wxRadioBox * m_radio;
   /// user name
   wxTextCtrl * m_login;
   /// password
   wxTextCtrl * m_password;
   /// folder path
   wxTextCtrl * m_path;
   /// and a browse button for it
   wxBrowseButton * m_browsePath;
   /// server name
   wxTextCtrl * m_server;
   /// newsgroup name
   wxTextCtrl * m_newsgroup;

   wxTextCtrl *MakeTextCtrl(wxString const &label, wxWindow *last = NULL);

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxFolderPropertiesPage, wxPanel)
   EVT_BUTTON(-1, wxFolderPropertiesPage::OnCommand)
END_EVENT_TABLE()

wxTextCtrl *
wxFolderPropertiesPage::MakeTextCtrl(wxString const &label, wxWindow *last)
{
   int widthMax = 100;
   wxLayoutConstraints *c;
   
   wxStaticText *l = new wxStaticText(this, -1, label, wxDefaultPosition,
                                      wxDefaultSize, wxALIGN_RIGHT);
   c = new wxLayoutConstraints;
   if(last)
      c->top.SameAs(last, wxBottom, LAYOUT_Y_MARGIN);
   else
      c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->width.Absolute(widthMax);
   c->height.AsIs();
   l->SetConstraints(c);

   wxTextCtrl *t = new wxTextCtrl(this, -1, "");
   c = new wxLayoutConstraints;
   c->top.SameAs(l, wxTop);
   c->left.RightOf(l, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();
   t->SetConstraints(c);
   return t;
}


wxFolderPropertiesPage::wxFolderPropertiesPage(wxNotebook *nb,
                                               ProfileBase *p)
   : wxPanel(nb, -1)
{
   m_notebook = nb;
   m_profile = p;

   wxLayoutConstraints *c;
   
   // radiobox of folder type
   wxString radioChoices[5];
   radioChoices[0] = _("INBOX");
   radioChoices[1] = _("File");
   radioChoices[2] = _("POP3");
   radioChoices[3] = _("IMAP");
   radioChoices[4] = _("Newsgroup");
   m_radio = new wxRadioBox(this,-1,_("Folder Type"),
                            wxDefaultPosition,wxDefaultSize,
                            5, radioChoices,3,wxRA_HORIZONTAL);

   c = new wxLayoutConstraints();
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(this, wxTop, LAYOUT_Y_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->bottom.AsIs();
   m_radio->SetConstraints(c);

   m_login = MakeTextCtrl(_("User name"), m_radio);
   m_password = MakeTextCtrl(_("Passsword"),m_login);
   m_path = MakeTextCtrl(_("Filename"), m_password);
   m_server = MakeTextCtrl(_("Server"), m_path);
   m_newsgroup = MakeTextCtrl(_("Newsgroup"), m_server);


   wxStaticText *l = new wxStaticText(this, -1, _("Filename"),
                                      wxDefaultPosition,
                                      wxDefaultSize, wxALIGN_RIGHT);
   c = new wxLayoutConstraints;
   c->top.SameAs(m_newsgroup, wxBottom, LAYOUT_Y_MARGIN);
   c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
   c->width.Absolute(100);
   c->height.AsIs();
   l->SetConstraints(c);

   wxTextCtrl *t = new wxTextCtrl(this, -1, "");
   c = new wxLayoutConstraints;
   c->top.SameAs(l, wxTop);
   c->left.RightOf(l, LAYOUT_X_MARGIN);
   c->right.AsIs();
   c->height.AsIs();
   t->SetConstraints(c);

   m_browsePath = new wxBrowseButton(t,this);
   c = new wxLayoutConstraints;
   c->top.SameAs(l, wxTop);
   c->left.RightOf(t, LAYOUT_X_MARGIN);
   c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
   c->height.AsIs();
   m_browsePath->SetConstraints(c);

   SetAutoLayout(TRUE);
}

void
wxFolderPropertiesPage::OnEvent(wxCommandEvent& event)
{
   UpdateUI();
}

void
wxFolderPropertiesPage::UpdateUI(void)
{
   m_profile->writeEntry(MP_FOLDER_TYPE,m_radio->GetSelection());

   switch(m_radio->GetSelection())
   {
   case MailFolder::MF_POP:
   case MailFolder::MF_IMAP:
      m_login->Enable(TRUE);
      m_password->Enable(TRUE);
      m_server->Enable(TRUE);
      m_path->Enable(FALSE);
      m_newsgroup->Enable(FALSE);
      m_browsePath->Enable(FALSE);
      break;
   case MailFolder::MF_NNTP:
      m_profile->writeEntry(MP_FOLDER_PATH,m_path->GetValue());
      m_login->Enable(FALSE);
      m_password->Enable(FALSE);
      m_server->Enable(TRUE);
      m_path->Enable(FALSE);
      m_newsgroup->Enable(TRUE);
      m_browsePath->Enable(FALSE);
      break;
   case MailFolder::MF_FILE:
      m_profile->writeEntry(MP_FOLDER_PATH,m_path->GetValue());
      break;
   case MailFolder::MF_INBOX:
      break;
   default:
      wxFAIL_MSG("Unexpected folder type.");
   }
}


bool
wxFolderPropertiesPage::TransferDataToWindow(void)
{
   m_login->SetValue(READ_CONFIG(m_profile,MP_POP_LOGIN));
   m_password->SetValue(READ_CONFIG(m_profile,MP_POP_PASSWORD));
   m_server->SetValue(READ_CONFIG(m_profile,MP_POP_HOST));
   m_path->SetValue(READ_CONFIG(m_profile,MP_FOLDER_PATH));
   m_newsgroup->SetValue(READ_CONFIG(m_profile,MP_FOLDER_PATH));
   return true;
}

bool
wxFolderPropertiesPage::TransferDataFromWindow(void)
{
   switch(m_radio->GetSelection())
   {
   case MailFolder::MF_POP:
   case MailFolder::MF_IMAP:
      m_profile->writeEntry(MP_POP_LOGIN,m_login->GetValue());
      m_profile->writeEntry(MP_POP_PASSWORD,m_password->GetValue());
      m_profile->writeEntry(MP_POP_HOST,m_server->GetValue());
      break;
   case MailFolder::MF_NNTP:
      m_profile->writeEntry(MP_NNTPHOST,m_server->GetValue());
      m_profile->writeEntry(MP_FOLDER_PATH,m_newsgroup->GetValue());
      break;
   case MailFolder::MF_FILE:
      m_profile->writeEntry(MP_FOLDER_PATH,m_newsgroup->GetValue());
      break;
   case MailFolder::MF_INBOX:
      m_path->SetValue("INBOX");
      break;
   default:
      wxFAIL_MSG("Unexpected folder type.");
   }
   return true;
}


wxOptionsPage::wxOptionsPage(wxNotebook *notebook,
                             const char *title,
                             ProfileBase *profile,
                             size_t nFirst,
                             size_t nLast)
             : wxPanel(notebook, -1)
{
   int image = notebook->GetPageCount();

   notebook->AddPage(this, title, FALSE /* don't select */, image);

   m_Profile = profile;

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
      widthCheck = AdjustCharHeight(GetCharHeight()) + 1;
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

wxOptionsDialog *wxOptionsPage::GetDialog() const
{
   // find the frame we're in
   wxWindow *win = GetParent();
   while ( win && !win->IsKindOf(CLASSINFO(wxDialog)) ) {
      win = win->GetParent();
   }

   wxASSERT( win != NULL );  // we must have a parent frame!

   return (wxOptionsDialog *)win;
}

void wxOptionsPage::OnBrowse(wxCommandEvent& event)
{
   wxBrowseButton *btn = (wxBrowseButton *)event.GetEventObject();
   btn->DoBrowse();
}

void wxOptionsPage::OnChange(wxEvent&)
{
  GetDialog()->SetDirty();
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
         wxCheckBox *checkbox = (wxCheckBox *)GetControl(nCheck);
         wxASSERT( checkbox->IsKindOf(CLASSINFO(wxCheckBox)) );

         wxControl *control = GetControl(n);
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
         default:
            ;
         }
      }
      // this field is always enabled
   }
}

// read the data from config
bool wxOptionsPage::TransferDataToWindow()
{
   // disable environment variable expansion here because we want the user to
   // edit the real value stored in the config
   ProfileEnvVarSuspend suspend(m_Profile);

   // check that we didn't forget to update one of the arrays...
   wxASSERT( WXSIZEOF(gs_aConfigDefaults) == ConfigField_Max );
   wxASSERT( WXSIZEOF(wxOptionsPage::ms_aFields) == ConfigField_Max );

   String strValue;
   long lValue;
   for ( size_t n = m_nFirst; n < m_nLast; n++ )
   {
      if ( gs_aConfigDefaults[n].IsNumeric() )
      {
         lValue = m_Profile->readEntry(gs_aConfigDefaults[n].name,
                                       (int) gs_aConfigDefaults[n].lValue);
         strValue.Printf("%ld", lValue);
      }
      else {
         // it's a string
         strValue = m_Profile->readEntry(gs_aConfigDefaults[n].name,
                                         gs_aConfigDefaults[n].szValue);
      }

      wxControl *control = GetControl(n);
      switch ( ms_aFields[n].type ) {
      case Field_Text:
      case Field_File:
      case Field_Number:
         if ( ms_aFields[n].type == Field_Number ) {
            wxASSERT( gs_aConfigDefaults[n].IsNumeric() );

            strValue.Printf("%ld", lValue);
         }
         else {
            wxASSERT( !gs_aConfigDefaults[n].IsNumeric() );
         }
         wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

         ((wxTextCtrl *)control)->SetValue(strValue);
         break;

      case Field_Bool:
         wxASSERT( gs_aConfigDefaults[n].IsNumeric() );
         wxASSERT( control->IsKindOf(CLASSINFO(wxCheckBox)) );

         ((wxCheckBox *)control)->SetValue(lValue != 0);
         break;

      case Field_List:
         wxASSERT( !gs_aConfigDefaults[n].IsNumeric() );
         wxASSERT( control->IsKindOf(CLASSINFO(wxListBox)) );

         // split it (FIXME @@@ what if it contains ';'?)
         {
            String str;
            for ( size_t m = 0; m < strValue.Len(); m++ ) {
               if ( strValue[m] == ';' ) {
                  if ( !str.IsEmpty() ) {
                     ((wxListBox *)control)->Append(str);
                     str.Empty();
                  }
                  //else: nothing to do, two ';' one after another
               }
               else {
                  str << strValue[m];
               }
            }

            if ( !str.IsEmpty() ) {
               ((wxListBox *)control)->Append(str);
            }
         }
         break;

      default:
         wxFAIL_MSG("unexpected field type");
      }
   }

   return TRUE;
}

// write the data to config
bool wxOptionsPage::TransferDataFromWindow()
{
   // @@@ should only write the entries which really changed

   String strValue;
   long lValue;
   for ( size_t n = m_nFirst; n < m_nLast; n++ )
   {
      wxControl *control = GetControl(n);
      switch ( ms_aFields[n].type )
      {
      case Field_Text:
      case Field_File:
      case Field_Number:
         wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

         strValue = ((wxTextCtrl *)control)->GetValue();

         if ( ms_aFields[n].type == Field_Number ) {
            wxASSERT( gs_aConfigDefaults[n].IsNumeric() );

            lValue = atol(strValue);
         }
         else {
            wxASSERT( !gs_aConfigDefaults[n].IsNumeric() );
         }
         break;

      case Field_Bool:
         wxASSERT( gs_aConfigDefaults[n].IsNumeric() );
         wxASSERT( control->IsKindOf(CLASSINFO(wxCheckBox)) );

         lValue = ((wxCheckBox *)control)->GetValue();
         break;

      case Field_List:
         wxASSERT( !gs_aConfigDefaults[n].IsNumeric() );
         wxASSERT( control->IsKindOf(CLASSINFO(wxListBox)) );

         // join it (FIXME @@@ what if it contains ';'?)
         {
            wxListBox *listbox = (wxListBox *)control;
            for ( size_t m = 0; m < (size_t)listbox->Number(); m++ ) {
               if ( !strValue.IsEmpty() ) {
                  strValue << ';';
               }

               strValue << listbox->GetString(m);
            }
         }
         break;

      default:
         wxFAIL_MSG("unexpected field type");
      }

      if ( gs_aConfigDefaults[n].IsNumeric() )
      {
         m_Profile->writeEntry(gs_aConfigDefaults[n].name, (int)lValue);
      }
      else
      {
         // it's a string
         m_Profile->writeEntry(gs_aConfigDefaults[n].name, strValue);
      }
   }

   // @@@@ life is easy if we don't check for errors...
   return TRUE;
}

#if 0 // unused for now
// ----------------------------------------------------------------------------
// wxOptionsPageCompose
// ----------------------------------------------------------------------------
bool wxOptionsPageCompose::TransferDataToWindow()
{
   return TRUE;
}

bool wxOptionsPageCompose::TransferDataFromWindow()
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

#ifdef USE_PYTHON

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

#endif // USE_PYTHON
#endif // 0 (not yet implemented)

// ----------------------------------------------------------------------------
// wxOptionsPageOthers
// ----------------------------------------------------------------------------
bool wxOptionsPageOthers::TransferDataToWindow()
{
   // if the user checked "don't ask me again" checkbox in the message box
   // these setting might be out of date - synchronize

   // TODO this should be table based too probably...
   if ( !wxPMessageBoxEnabled(MC_CONFIRMEXIT) )
      m_Profile->writeEntry(MC_CONFIRMEXIT, false);

   return wxOptionsPage::TransferDataToWindow();
}

bool wxOptionsPageOthers::TransferDataFromWindow()
{
   bool rc = wxOptionsPage::TransferDataFromWindow();
   if ( rc )
   {
      // no if the user check "confirm exit" checkbox we must reenable the
      // message box by erasing the stored answer to it
      if ( m_Profile->readEntry(MC_CONFIRMEXIT, false) )
         wxPMessageBoxEnable(MC_CONFIRMEXIT);
   }

   return rc;
}

// ----------------------------------------------------------------------------
// wxOptionsPageFolders
// ----------------------------------------------------------------------------

bool wxOptionsPageFolders::TransferDataToWindow()
{
   bool bRc = wxOptionsPage::TransferDataToWindow();
   if ( bRc ) {
      // we add the folder opened in the main frame to the list of folders
      // opened on startup if it's not yet among them
      wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
      wxString strMain = GetControlText(ConfigField_MainFolder);
      int n = listbox->FindString(strMain);
      if ( n == -1 ) {
         listbox->Append(strMain);
      }
   }

   return bRc;
}

bool wxOptionsPageFolders::TransferDataFromWindow()
{
   // undo what we did in TransferDataToWindow: remove the main folder from
   // the list of folders to be opened on startup
   wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
   wxString strMain = GetControlText(ConfigField_MainFolder);
   int n = listbox->FindString(strMain);
   if ( n != -1 ) {
      listbox->Delete(n);
   }

   return wxOptionsPage::TransferDataFromWindow();
}

void wxOptionsPageFolders::OnNewFolder(wxCommandEvent&)
{
   wxString str;
   if ( !MInputBox(&str, _("Folders to open on startup"), _("Folder name"),
                   GetDialog(), "LastStartupFolder") ) {
      return;
   }

   // check that it's not already there
   wxListBox *listbox = (wxListBox *)GetControl(ConfigField_OpenFolders);
   if ( listbox->FindString(str) != -1 ) {
      wxLogError(_("Folder '%s' is already present in the list, not added."),
                 str.c_str());
   }
   else {
      // ok, do add it
      listbox->Append(str);

    GetDialog()->SetDirty();
  }
}

void wxOptionsPageFolders::OnModifyFolder(wxCommandEvent&)
{
   wxListBox *l = (wxListBox *)GetControl(ConfigField_OpenFolders);
   int nSel = l->GetSelection();

   wxCHECK_RET( nSel != -1, "should be disabled" );

   ProfileBase *profile = ProfileBase::CreateProfile(l->GetString(nSel), NULL);

  MDialog_FolderProfile(GetDialog(), profile);
}

void wxOptionsPageFolders::OnDeleteFolder(wxCommandEvent&)
{
   wxListBox *l = (wxListBox *)GetControl(ConfigField_OpenFolders);
   int nSel = l->GetSelection();

   wxCHECK_RET( nSel != -1, "should be disabled" );

   l->Delete(nSel);
   GetDialog()->SetDirty();
}

void wxOptionsPageFolders::OnIdle(wxIdleEvent&)
{
   bool bEnable = ((wxListBox *)GetControl(ConfigField_OpenFolders))
      ->GetSelection() != -1;

   long id;
   for ( id = wxOptionsPage_BtnModify; id <= wxOptionsPage_BtnDelete; id++ ) {
      wxWindow *win = FindWindow(id);
      if ( win ) {
         win->Enable(bEnable);
      }
      else {
         wxFAIL_MSG("can't find listbox buttons by id");
      }
   }
}

// ----------------------------------------------------------------------------
// wxOptionsDialog
// ----------------------------------------------------------------------------
wxOptionsDialog::wxOptionsDialog(wxFrame *parent,
                                 Types type)
   : wxDialog(parent, -1, wxString(_("M: Options")))
{
   wxLayoutConstraints *c;
   SetAutoLayout(TRUE);
   m_type = type;

   // calculate the controls size
   // ---------------------------

   // basic unit is the height of a char, from this we fix the sizes of all
   // other controls
   size_t heightLabel = AdjustCharHeight(GetCharHeight());
   int hBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       wBtn = BUTTON_WIDTH_FROM_HEIGHT(hBtn);

   // these are more or less arbitrary numbers
   const int wDlg = 6*wBtn;
   const int hDlg = 19*hBtn;
   wxWindow::SetSize(wDlg, hDlg);

   wxWindow *last;

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


   // for a dialog of type FolderCreate we need an extra box
   // with the folder name
   // -------------------
   if(m_type == Type_FolderCreate)
   {
      // for the label
      c = new wxLayoutConstraints;
      c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
      c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
      c->width.Absolute(100); //FIXME
      c->height.AsIs();
      wxStaticText *pLabel = new wxStaticText(panel, -1, _("Folder Name"),
                                              wxDefaultPosition, wxDefaultSize,
                                              wxALIGN_RIGHT);
      pLabel->SetConstraints(c);

      m_folderName = new wxTextCtrl(panel,-1,"");
      c = new wxLayoutConstraints;
      c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
      c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
      c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
      c->height.AsIs();
      m_folderName->SetConstraints(c);
      last = pLabel;
   }
   else
      last = panel;


   // create the notebook
   // -------------------

   switch(type)
   {
   case Type_Preferences:
      m_notebook = new wxOptionsNotebook(panel);
      break;
   case Type_FolderCreate:
      m_notebook = new wxFolderCreateNotebook(panel);
      break;
   }
   c = new wxLayoutConstraints;
   c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
   if(last == panel)
      c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
   else
      c->top.SameAs(last, wxBottom, LAYOUT_Y_MARGIN);
   c->bottom.SameAs(panel, wxBottom, 2*LAYOUT_Y_MARGIN + hBtn);
   m_notebook->SetConstraints(c);

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

   Layout();

   // set position
   // ------------
   SetSizeHints(wDlg, hDlg);
   Centre(wxCENTER_FRAME | wxBOTH);

   TransferDataToWindow();

   m_bDirty = FALSE;
   for ( int nPage = 0; nPage < m_notebook->GetPageCount(); nPage++ ) {
      ((wxOptionsPage *)m_notebook->GetPage(nPage))->Refresh();
   }
}

bool wxOptionsDialog::TransferDataToWindow()
{
   ProfilePathChanger(mApplication->GetProfile(),M_PROFILE_CONFIG_SECTION);

   for ( int nPage = 0; nPage < m_notebook->GetPageCount(); nPage++ ) {
      if ( !m_notebook->GetPage(nPage)->TransferDataToWindow() ) {
         return FALSE;
      }
   }

   return TRUE;
}

bool wxOptionsDialog::TransferDataFromWindow()
{
   ProfilePathChanger(mApplication->GetProfile(),M_PROFILE_CONFIG_SECTION);
   ProfileBase *profile;

   // for the creation of a folder we don't use the toplevel config
   // section but create a profile of that name first
   if(m_type == Type_FolderCreate)
   {
      // first, create such a profile:
      profile = ProfileBase::CreateProfile(m_folderName->GetValue(),
                                           mApplication->GetProfile());
      profile->writeEntry(MP_PROFILE_TYPE,ProfileBase::PT_FolderProfile);
      // tell the pages to use this profile instead of the global one:
      for (int nPage = 0; nPage < m_notebook->GetPageCount(); nPage++)
         ((wxOptionsPage *)m_notebook->GetPage(nPage))->SetProfile(profile);
   }

   for ( int nPage = 0; nPage < m_notebook->GetPageCount(); nPage++ )
      if ( !m_notebook->GetPage(nPage)->TransferDataFromWindow() )
         return FALSE;

   if(m_type == Type_FolderCreate)
      profile->DecRef();

   return TRUE;
}

void
wxOptionsDialog::DoTest(void)
{
   if(m_type == Type_Preferences)
      if(MDialog_YesNoDialog(_("Test new setup?"),
                             this, _("Test setup?"),true))
         VerifyMailConfig();
}

void wxOptionsDialog::OnOK(wxCommandEvent& /* event */)
{
   if(m_type == Type_FolderCreate && m_folderName->GetValue().length())
      m_bDirty = true;
   if ( !m_bDirty || TransferDataFromWindow() )
   {
      DoTest();
   }

   EndModal(0);
}

void wxOptionsDialog::OnApply(wxCommandEvent& /* event */)
{
   TransferDataFromWindow();
   m_bDirty = FALSE;
   m_btnApply->Enable(FALSE);
}

void wxOptionsDialog::OnCancel(wxCommandEvent& /* event */)
{
  EndModal(0);
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
// wxOptionsNotebookBase manages its own image list
// ----------------------------------------------------------------------------

// create the control and add pages too
wxOptionsNotebook::wxOptionsNotebook(wxWindow *parent)
                 : wxOptionsNotebookBase("OptionsNotebook", parent)
{
   // create and fill the imagelist
   static const char *aszImages[] =
   {
      // should be in sync with the corresponding enum
      "ident",
      "compose",
      "folders",
#ifdef USE_PYTHON
      "python",
#endif
      "miscopt"
   };

   wxASSERT( WXSIZEOF(aszImages) == Icon_Max );  // don't forget to update both!

   wxImageList *imageList = new wxImageList(32, 32, FALSE, WXSIZEOF(aszImages));
   size_t n;
   for ( n = 0; n < Icon_Max; n++ ) {
      imageList->Add(mApplication->GetIconManager()->GetBitmap(String(aszImages[n])));
   }

   SetImageList(imageList);

   ProfileBase *profile = mApplication->GetProfile();
   // create and add the pages
   (void)new wxOptionsPageIdent(this,profile);
   (void)new wxOptionsPageCompose(this,profile);
   (void)new wxOptionsPageFolders(this,profile);
#ifdef USE_PYTHON
   (void)new wxOptionsPagePython(this,profile);
#endif
   (void)new wxOptionsPageOthers(this,profile);
}

// create the control and add pages too
wxFolderCreateNotebook::wxFolderCreateNotebook(wxWindow *parent)
                      : wxOptionsNotebookBase("FolderCreateNotebook", parent)
{
   // create and fill the imagelist
   static const char *aszImages[] =
   {
      // should be in sync with the corresponding enum
      "compose","access"
   };

   wxASSERT( WXSIZEOF(aszImages) == Icon_Max );  // don't forget to update both!

   wxImageList *imageList = new wxImageList(32, 32, FALSE, WXSIZEOF(aszImages));
   size_t n;
   for ( n = 0; n < Icon_Max; n++ ) {
      imageList->Add(mApplication->GetIconManager()->GetBitmap(String(aszImages[n])));
   }

   SetImageList(imageList);

   ProfileBase *profile = mApplication->GetProfile();
   // create and add the pages
   (void)new wxOptionsPageCompose(this,profile);
//   wxResourceParseData(FolderOpenPanel);
//   AddPage(new wxResourcePanel("FolderOpenPanel",this),
   //   _("Folder"),FALSE,1);
   AddPage(new wxFolderPropertiesPage(this,profile), _("Access"),FALSE,1);
}


wxOptionsNotebookBase::~wxOptionsNotebookBase()
{
   delete GetImageList();
}


//----------------------------------------------------------------------------
// our public interface is just this simple function
//----------------------------------------------------------------------------
void ShowOptionsDialog(wxFrame *parent)
{
   wxOptionsDialog dlg(parent);
   (void)dlg.ShowModal();
}

void ShowFolderCreateDialog(wxFrame *parent)
{
  wxOptionsDialog dlg(parent, wxOptionsDialog::Type_FolderCreate);
  (void)dlg.ShowModal();
}
