///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxOptionsDlg.cpp - M options dialog
// Purpose:     allows to easily change from one dialog all program options
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
#include   <wx/confbase.h>

#include   "MDialogs.h"
#include   "Mdefaults.h"
#include   "Mupgrade.h"
#include   "Mcallbacks.h"

#include   "gui/wxIconManager.h"
#include   "gui/wxDialogLayout.h"
#include   "gui/wxOptionsDlg.h"
#include   "gui/wxOptionsPage.h"

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

// -----------------------------------------------------------------------------
// our notebook class
// -----------------------------------------------------------------------------

// notebook for the options
class wxOptionsNotebook : public wxNotebookWithImages
{
public:
   // icon names
   static const char *s_aszImages[];

   wxOptionsNotebook(wxWindow *parent);
};

// -----------------------------------------------------------------------------
// dialog classes
// -----------------------------------------------------------------------------

class wxOptionsDialog : public wxNotebookDialog
{
public:
   wxOptionsDialog(wxFrame *parent);

   ~wxOptionsDialog();

   // notifications from the notebook pages
      // something important change
   void SetDoTest() { SetDirty(); m_bTest = TRUE; }
      // some setting changed, but won't take effect until restart
   void SetGiveRestartWarning() { m_bRestartWarning = TRUE; }

   // override base class functions
   virtual void CreateNotebook(wxPanel *panel);
   virtual bool TransferDataToWindow();

   virtual bool OnSettingsChange();

   // unimplemented default ctor for DECLARE_DYNAMIC_CLASS
   wxOptionsDialog() { }

protected:
   // unset the dirty flag
   virtual void ResetDirty();

private:
   bool         m_bTest,            // test new settings?
                m_bRestartWarning;  // changes will take effect after restart

   DECLARE_DYNAMIC_CLASS(wxOptionsDialog)
};

// ----------------------------------------------------------------------------
// event tables and such
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxOptionsDialog, wxNotebookDialog)

BEGIN_EVENT_TABLE(wxOptionsPage, wxNotebookPageBase)
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
   { "&Hostname",                    Field_Text |
                                     Field_Vital,   -1,                        },
   { "&Return address",              Field_Text |
                                     Field_Vital,   -1,                        },
   { "SMTP (&mail) server",          Field_Text |
                                     Field_Vital,   -1,                        },
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
   { "&XFace file",                  Field_File,    ConfigField_XFace          },

   // folders
   { "Folders to open on &startup",  Field_List |
                                     Field_Restart, -1,                        },
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
   { "&Splash screen at startup",    Field_Bool |
                                     Field_Restart, -1,                        },
   { "Splash screen &delay",         Field_Number,  ConfigField_Splash         },
   { "Confirm &exit",                Field_Bool |
                                     Field_Restart, -1,                        },
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

// ----------------------------------------------------------------------------
// wxOptionsPage
// ----------------------------------------------------------------------------

wxOptionsPage::wxOptionsPage(wxNotebook *notebook,
                             const char *title,
                             ProfileBase *profile,
                             size_t nFirst,
                             size_t nLast)
             : wxNotebookPageBase(notebook)
{
   int image = notebook->GetPageCount();

   notebook->AddPage(this, title, FALSE /* don't select */, image);

   m_Profile = profile;

   // see enum ConfigFields for "+1"
   m_nFirst = nFirst + 1;
   m_nLast = nLast + 1;

   CreateControls();
}

void wxOptionsPage::CreateControls()
{
   size_t n;

   // first determine the longest label
   wxArrayString aLabels;
   for ( n = m_nFirst; n < m_nLast; n++ ) {
      // do it only for text control labels
      switch ( GetFieldType(n) ) {
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

      aLabels.Add(_(ms_aFields[n].label));
   }

   long widthMax = GetMaxLabelWidth(aLabels, this);

   // now create the controls
   wxControl *last = NULL; // last control created
   for ( n = m_nFirst; n < m_nLast; n++ ) {
      switch ( GetFieldType(n) ) {
         case Field_File:
            last = CreateFileEntry(_(ms_aFields[n].label), widthMax, last);
            break;

         case Field_Number:
            // fall through -- for now they're the same as text

         case Field_Text:
            last = CreateTextWithLabel(_(ms_aFields[n].label), widthMax, last);
            break;

         case Field_List:
            last = CreateListbox(_(ms_aFields[n].label), last);
            break;

         case Field_Bool:
            last = CreateCheckBox(_(ms_aFields[n].label), widthMax, last);
            break;

         default:
            wxFAIL_MSG("unknown field type in CreateControls");
      }

      wxCHECK_RET( last, "control creation failed" );

      FieldFlags flags = GetFieldFlags(n);
      if ( flags & Field_Vital )
         m_aVitalControls.Add(last);
      if ( flags & Field_Restart )
         m_aRestartControls.Add(last);

      m_aControls.Add(last);
   }
}

void wxOptionsPage::OnChange(wxEvent& event)
{
   wxOptionsDialog *dialog = GET_PARENT_OF_CLASS(this, wxOptionsDialog);

   if ( !dialog )
   {
       // we don't put an assert here because this does happen when we're a
       // page in the folder properties dialog
       return;
   }

   wxControl *control = (wxControl *)event.GetEventObject();
   if ( m_aVitalControls.Index(control) != -1 )
      dialog->SetDoTest();
   else
      dialog->SetDirty();

   if ( m_aRestartControls.Index(control) != -1 )
      dialog->SetGiveRestartWarning();
}

void wxOptionsPage::OnCheckboxChange(wxEvent& event)
{
   OnChange(event);

   Refresh();
}

void wxOptionsPage::UpdateUI()
{
   for ( size_t n = m_nFirst; n < m_nLast; n++ ) {
      int nCheckField = ms_aFields[n].enable;
      if ( nCheckField != -1 ) {
         wxASSERT( nCheckField > 0 && nCheckField < ConfigField_Max );

         // avoid signed/unsigned mismatch in expressions
         size_t nCheck = (size_t)nCheckField;
         wxASSERT( GetFieldType(nCheck) == Field_Bool );
         wxASSERT( nCheck >= m_nFirst && nCheck < m_nLast );

         // enable only if the checkbox is checked
         wxCheckBox *checkbox = (wxCheckBox *)GetControl(nCheck);
         wxASSERT( checkbox->IsKindOf(CLASSINFO(wxCheckBox)) );

         wxControl *control = GetControl(n);
         bool bEnable = checkbox->GetValue();

         control->Enable(bEnable);

         switch ( GetFieldType(n) ) {
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
                                       (int)gs_aConfigDefaults[n].lValue);
         strValue.Printf("%ld", lValue);
      }
      else {
         // it's a string
         strValue = m_Profile->readEntry(gs_aConfigDefaults[n].name,
                                         gs_aConfigDefaults[n].szValue);
      }

      wxControl *control = GetControl(n);
      switch ( GetFieldType(n) ) {
         case Field_Text:
         case Field_File:
         case Field_Number:
            if ( GetFieldType(n) == Field_Number ) {
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
      switch ( GetFieldType(n) )
      {
         case Field_Text:
         case Field_File:
         case Field_Number:
            wxASSERT( control->IsKindOf(CLASSINFO(wxTextCtrl)) );

            strValue = ((wxTextCtrl *)control)->GetValue();

            if ( GetFieldType(n) == Field_Number ) {
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

// ----------------------------------------------------------------------------
// wxOptionsPageCompose
// ----------------------------------------------------------------------------

wxOptionsPageCompose::wxOptionsPageCompose(wxNotebook *parent,
                                           ProfileBase *profile)
                    : wxOptionsPage(parent,
                                    _("Compose"),
                                    profile,
                                    ConfigField_ComposeFirst,
                                    ConfigField_ComposeLast)
{
}

// ----------------------------------------------------------------------------
// wxOptionsPageIdent
// ----------------------------------------------------------------------------

wxOptionsPageIdent::wxOptionsPageIdent(wxNotebook *parent,
                                       ProfileBase *profile)
                  : wxOptionsPage(parent,
                                  _("Identity"),
                                  profile,
                                  ConfigField_IdentFirst,
                                  ConfigField_IdentLast)
{
}

// ----------------------------------------------------------------------------
// wxOptionsPagePython
// ----------------------------------------------------------------------------

#ifdef USE_PYTHON

wxOptionsPagePython::wxOptionsPagePython(wxNotebook *parent,
                                         ProfileBase *profile)
                   : wxOptionsPage(parent,
                                   _("Python"),
                                   profile,
                                   ConfigField_PythonFirst,
                                   ConfigField_PythonLast)
{
}

#endif // USE_PYTHON

// ----------------------------------------------------------------------------
// wxOptionsPageOthers
// ----------------------------------------------------------------------------

wxOptionsPageOthers::wxOptionsPageOthers(wxNotebook *parent,
                                         ProfileBase *profile)
                   : wxOptionsPage(parent,
                                   _("Miscellaneous"),
                                   profile,
                                   ConfigField_OthersFirst,
                                   ConfigField_OthersLast)
{
}

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
      // now if the user checked "confirm exit" checkbox we must reenable
      // the message box by erasing the stored answer to it
      if ( m_Profile->readEntry(MC_CONFIRMEXIT, false) )
         wxPMessageBoxEnable(MC_CONFIRMEXIT);
   }

   return rc;
}

// ----------------------------------------------------------------------------
// wxOptionsPageFolders
// ----------------------------------------------------------------------------

wxOptionsPageFolders::wxOptionsPageFolders(wxNotebook *parent,
                                           ProfileBase *profile)
                    : wxOptionsPage(parent,
                                    _("Mail boxes"),
                                    profile,
                                    ConfigField_FoldersFirst,
                                    ConfigField_FoldersLast)
{
}

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

void wxOptionsPageFolders::OnNewFolder(wxCommandEvent& event)
{
   wxString str;
   if ( !MInputBox(&str, _("Folders to open on startup"), _("Folder name"),
                   GET_PARENT_OF_CLASS(this, wxDialog), "LastStartupFolder") ) {
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

      wxOptionsPage::OnChange(event);
  }
}

void wxOptionsPageFolders::OnModifyFolder(wxCommandEvent&)
{
   wxListBox *l = (wxListBox *)GetControl(ConfigField_OpenFolders);
   int nSel = l->GetSelection();

   wxCHECK_RET( nSel != -1, "should be disabled" );

   ProfileBase *profile = ProfileBase::CreateProfile(l->GetString(nSel), NULL);

   MDialog_FolderProfile(GET_PARENT_OF_CLASS(this, wxDialog), profile);
}

void wxOptionsPageFolders::OnDeleteFolder(wxCommandEvent& event)
{
   wxListBox *l = (wxListBox *)GetControl(ConfigField_OpenFolders);
   int nSel = l->GetSelection();

   wxCHECK_RET( nSel != -1, "should be disabled" );

   l->Delete(nSel);
   wxOptionsPage::OnChange(event);
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

wxOptionsDialog::wxOptionsDialog(wxFrame *parent)
               : wxNotebookDialog(parent, _("Program options"))
{
}

bool
wxOptionsDialog::TransferDataToWindow()
{
   if ( !wxNotebookDialog::TransferDataToWindow() )
      return FALSE;

   int nPageCount = m_notebook->GetPageCount();
   for ( int nPage = 0; nPage < nPageCount; nPage++ ) {
      ((wxOptionsPage *)m_notebook->GetPage(nPage))->UpdateUI();
   }

   return TRUE;
}

bool
wxOptionsDialog::OnSettingsChange()
{
   if ( m_bTest )
   {
      if ( MDialog_YesNoDialog(_("Some important program settings were changed.\n"
                                 "\nWould you like to test the new setup "
                                 "(recommended)?"),
                               this,
                               _("Test setup?"),
                               true,
                               "OptTestAsk") )
      {
         if ( !VerifyMailConfig() )
         {
            return FALSE;
         }
      }
      else
      {
         // no test was done, assume it's ok...
         m_bTest = FALSE;
      }
   }

   if ( m_bRestartWarning )
   {
      MDialog_Message(_("Some of the changes to the program options will\n"
                        "only take effect when the progam will be run the\n"
                        "next time and not during this session."),
                        this, MDIALOG_MSGTITLE, "WarnRestartOpt");
      m_bRestartWarning = FALSE;
   }

   return TRUE;
}

void wxOptionsDialog::CreateNotebook(wxPanel *panel)
{
   m_notebook = new wxOptionsNotebook(panel);
}

// reset the dirty flag
void wxOptionsDialog::ResetDirty()
{
   wxNotebookDialog::ResetDirty();

   m_bTest =
   m_bRestartWarning = FALSE;
}

wxOptionsDialog::~wxOptionsDialog()
{
   // save settings
   wxConfigBase *config = mApplication->GetProfile()->GetConfig();
   CHECK_RET( config, "no config in ~wxOptionsDialog?" );
   (void)config->Flush();
}

// ----------------------------------------------------------------------------
// wxOptionsNotebookBase manages its own image list
// ----------------------------------------------------------------------------

// should be in sync with the enum OptionPage in wxOptionsDlg.h!
const char *wxOptionsNotebook::s_aszImages[] =
{
   "ident",
   "compose",
   "folders",
#ifdef USE_PYTHON
   "python",
#endif
   "miscopt",
   NULL
};

// create the control and add pages too
wxOptionsNotebook::wxOptionsNotebook(wxWindow *parent)
                 : wxNotebookWithImages("OptionsNotebook", parent, s_aszImages)
{
   // don't forget to update both the array above and the enum!
   wxASSERT( WXSIZEOF(s_aszImages) == OptionPage_Max + 1);

   ProfileBase *profile = mApplication->GetProfile();

   // create and add the pages
   (void)new wxOptionsPageIdent(this, profile);
   (void)new wxOptionsPageCompose(this, profile);
   (void)new wxOptionsPageFolders(this, profile);
#ifdef USE_PYTHON
   (void)new wxOptionsPagePython(this, profile);
#endif
   (void)new wxOptionsPageOthers(this, profile);
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

void ShowOptionsDialog(wxFrame *parent, OptionPage page)
{
   wxOptionsDialog dlg(parent);
   dlg.CreateAllControls();
   dlg.SetNotebookPage(page);
   (void)dlg.ShowModal();
}
