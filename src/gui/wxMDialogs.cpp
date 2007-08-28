/*-*- c++ -*-********************************************************
 * wxMDialogs.cpp : wxWindows version of dialog boxes               *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "Mdefaults.h"
#  include "guidef.h"
#  include "strutil.h"
#  include "MFrame.h"
#  include "Profile.h"
#  include "gui/wxMApp.h"
#  include "gui/wxMIds.h"
#  include "gui/wxIconManager.h"
#  include "sysutil.h"    // for sysutil_compare_filenames
#  include "MHelp.h"
#  include "gui/wxMFrame.h"

#  include <wx/layout.h>
#  include <wx/sizer.h>
#  include <wx/menu.h>
#  include <wx/checklst.h>
#  include <wx/statbox.h>
#  include <wx/msgdlg.h>
#  include <wx/choicdlg.h>
#  include <wx/filedlg.h>
#  include <wx/dcclient.h>  // for wxClientDC
#  include <wx/settings.h>  // for wxSystemSettings
#  include <wx/timer.h>     // for wxTimer
#ifdef __WINE__
#  include <wx/stattext.h>  // for wxStaticText
#endif // __WINE__
#endif // USE_PCH

#include "Mpers.h"

#include "XFace.h"

#include "MFolder.h"

#include "ConfigSource.h"
#include "ConfigSourcesAll.h"

#include <wx/tipdlg.h>
#include <wx/numdlg.h>
#include <wx/statline.h>
#include <wx/minifram.h>
#include <wx/fs_mem.h>
#include <wx/html/htmlwin.h>

#include "MFolderDialogs.h"
#include "FolderView.h"
#include "ASMailFolder.h"

#include "gui/wxFolderTree.h"
#include "gui/wxSelectionDlg.h"
#include "gui/wxIdentityCombo.h"

#include <errno.h>

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_COMPOSE_USE_XFACE;
extern const MOption MP_COMPOSE_XFACE_FILE;
extern const MOption MP_CRYPTALGO;
extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_DATE_FMT;
extern const MOption MP_DATE_GMT;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_FOLDER_PATH;
extern const MOption MP_HEIGHT;
extern const MOption MP_LASTTIP;
extern const MOption MP_NNTPHOST_LOGIN;
extern const MOption MP_SHOWTIPS;
extern const MOption MP_SMTPHOST_PASSWORD;
extern const MOption MP_WIDTH;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_AUTOEXPUNGE;
extern const MPersMsgBox *M_MSGBOX_REENABLE_HINT;

// ----------------------------------------------------------------------------
// other constants
// ----------------------------------------------------------------------------

#define MESSAGE_BOXES_ROOT _T("/") M_SETTINGS_CONFIG_SECTION _T("/MessageBox")

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// define this to use semi-modal dialogs which allow to view help while they
// are shown - note that this doesn't work at all well under Windows
//#define USE_SEMIMODAL

int
wxMDialog::ShowModal()
{
   // hide the splash if it is still shown or we wouldn't see the dialog behind
   // it
   CloseSplash();

#ifdef USE_SEMIMODAL
   m_modalShowing = TRUE;

#if wxUSE_HELP && wxUSE_HTML
   /* Disable all other windows apart from the help frame and this
      one. */

   wxFrame *hf = NULL;
   wxHelpController *hc = ((wxMApp *)mApplication)->GetHelpController();
   if(hc && hc->IsKindOf(CLASSINFO(wxHelpControllerHtml)))
      hf = ((wxHelpControllerHtml *)hc)->GetFrameParameters();
#else
   wxWindow *hf = NULL;
#endif

   wxWindowList::Node *node;
   for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
   {
      if(node->GetData() != hf && node->GetData() != this)
         node->GetData()->Enable(FALSE);
   }

   Show( TRUE );

   while(IsModal())
      wxTheApp->Dispatch();

   wxEnableTopLevelWindows(TRUE);
   return GetReturnCode();
#else // !USE_SEMIMODAL
   return wxDialog::ShowModal();
#endif // USE_SEMIMODAL/!USE_SEMIMODAL
}

void wxMDialog::EndModal( int retCode )
{
#ifdef USE_SEMIMODAL
    SetReturnCode( retCode );

    if (!IsModal())
    {
        wxFAIL_MSG( _T("wxMDialog:EndModal called twice") );
        return;
    }
    m_modalShowing = FALSE;
    Show( FALSE );
#else // !USE_SEMIMODAL
    wxDialog::EndModal(retCode);
#endif // USE_SEMIMODAL/!USE_SEMIMODAL
}

// better looking and wxConfig-aware wxTextEntryDialog
class MTextInputDialog : public wxDialog
{
public:
   MTextInputDialog(wxWindow *parent,
                    const wxString& strText,
                    const wxString& strCaption,
                    const wxString& strPrompt,
                    const wxString& strConfigPath,
                    const wxString& strDefault,
                    bool passwordflag);

   // accessors
   const wxString& GetText() const { return m_strText; }

   // base class virtuals implemented
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // if using a textctrl and not a combobox, this will process the
   // ENTER key
   void OnEnter(wxCommandEvent&)
   {
      TransferDataFromWindow();
      EndModal(wxID_OK);
   }

private:
   wxString      m_strText;
   wxPTextEntry *m_text;
   wxTextCtrl   *m_passwd; // used if we ask for a password, NULL otherwise

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MTextInputDialog)
};

BEGIN_EVENT_TABLE(MTextInputDialog, wxDialog)
    EVT_TEXT_ENTER(-1, MTextInputDialog::OnEnter)
END_EVENT_TABLE()

// a dialog showing all folders
class MFolderDialog : public wxManuallyLaidOutDialog
{
public:
   MFolderDialog(wxWindow *parent, MFolder *folder, int flags);
   virtual ~MFolderDialog();

   // accessors
   MFolder *GetFolder() const { SafeIncRef(m_folder); return m_folder; }
   bool HasUserChosenFolder() const { return m_userChoseFolder; }

   // base class virtuals implemented
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnButton(wxCommandEvent &ev);

private:
   // get the config path used for storing the last folders name
   static wxString GetConfigPath();

   wxString      m_FileName;
   MFolder      *m_folder;
   wxFolderTree *m_tree;
   wxWindow     *m_winTree;

   // has the user chosen a folder?
   bool m_userChoseFolder;

   // the combination of MDlg_Folder_XXX flags
   int m_flags;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MFolderDialog)
};

BEGIN_EVENT_TABLE(MFolderDialog, wxMDialog)
    EVT_BUTTON(-1, MFolderDialog::OnButton)
END_EVENT_TABLE()

// helper class for MFolderDialog: this folder tree selects the folders on
// double click instead of opening them
class MFolderDialogTree : public wxFolderTree
{
public:
   MFolderDialogTree(MFolderDialog *dlg) : wxFolderTree(dlg) { m_dlg = dlg; }

   virtual bool OnDoubleClick()
   {
      // pretend the dialog was closed via ok button
      wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK);
      event.SetEventObject(m_dlg);
      m_dlg->GetEventHandler()->ProcessEvent(event);

      return m_dlg->HasUserChosenFolder();
   }

private:
   MFolderDialog *m_dlg;
};

// ----------------------------------------------------------------------------
// NoBusyCursor ensures that the cursor is not "busy" (hourglass) while this
// object is in scope and is supposed to be used when creating a modal dialog
// (see MDialog_ErrorMessage &c for the examples)
// ----------------------------------------------------------------------------

class NoBusyCursor
{
public:
   NoBusyCursor()
   {
      m_countBusy = 0;
      while ( wxIsBusy() )
      {
         m_countBusy++;

         MEndBusyCursor();
      }
   }

   ~NoBusyCursor()
   {
      while ( m_countBusy-- > 0 )
      {
         MBeginBusyCursor();
      }
   }

private:
   int m_countBusy;
};

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

// under Windows we don't use wxCENTRE style which uses the generic message box
// instead of the native one (and thus it doesn't have icons, for example)
static inline long GetMsgBoxStyle(long style)
{
# ifdef OS_WIN
    return style;
# else //OS_WIN
    return style | wxCENTRE;
# endif
}

// get the msg box style for a question dialog box
static inline long GetYesNoMsgBoxStyle(int flags)
{
   int style = GetMsgBoxStyle(wxYES_NO | wxICON_QUESTION);
   if ( flags & M_DLG_NO_DEFAULT )
      style |= wxNO_DEFAULT;

   return GetMsgBoxStyle(style);
}

// needed to fix a bug/misfeature of wxWin 2.2.x: an already deleted window may
// be used as parent for the newly created dialogs, don't let this happen
extern void ReallyCloseTopLevelWindow(wxFrame *frame)
{
   frame->Hide(); // immediately
   frame->Close(true /* force */); // will happen later
}

wxWindow *GetParentOfClass(const wxWindow *win, wxClassInfo *classinfo)
{
   // find the frame we're in
   while ( win && !win->IsKindOf(classinfo) ) {
      win = win->GetParent();
   }

   // may be NULL!
   return (wxWindow *)win; // const_cast
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// simple wrappers around wxWin functions
// ----------------------------------------------------------------------------

long
MGetNumberFromUser(const wxString& message,
                   const wxString& prompt,
                   const wxString& caption,
                   long value,
                   long min,
                   long max,
                   wxWindow *parent,
                   const wxPoint& pos)
{
   CloseSplash();

   NoBusyCursor no;

   return wxGetNumberFromUser(message, prompt, caption,
                              value, min, max,
                              parent, pos);
}

// ----------------------------------------------------------------------------
// MTextInputDialog dialog and MInputBox (which uses it)
// ----------------------------------------------------------------------------

MTextInputDialog::MTextInputDialog(wxWindow *parent,
                                   const wxString& strText,
                                   const wxString& strCaption,
                                   const wxString& strPrompt,
                                   const wxString& strConfigPath,
                                   const wxString& strDefault,
                                   bool passwordflag)
   : wxDialog(parent, -1, wxString(M_TITLE_PREFIX) + strCaption)
{
  // text is the default value normally read from config and it may be
  // overriden by strDefault parameter if it is not empty
  if ( strDefault.empty() )
  {
     m_strText = strText;
  }
  else // the program provided default value, use it
  {
     m_strText = strDefault;
  }

  // layout
  wxCoord widthLabel, heightLabel;
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
  dc.GetTextExtent(strPrompt, &widthLabel, &heightLabel);

  long heightBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);
  long widthText = 3*widthBtn,
       heightText = TEXT_HEIGHT_FROM_LABEL(heightLabel);
  if ( widthText < widthLabel ) {
     // too short text zone looks ugly
     widthText = widthLabel;
  }

  long widthDlg = widthLabel + widthText + 3*LAYOUT_X_MARGIN,
       heightDlg = heightText + heightBtn + 3*LAYOUT_Y_MARGIN;

  long x = LAYOUT_X_MARGIN,
       y = LAYOUT_Y_MARGIN,
       dy = (heightText - heightLabel) / 2;

  // label and the text
  (void)new wxStaticText(this, -1, strPrompt, wxPoint(x, y + dy),
                         wxSize(widthLabel, heightLabel));
  if(passwordflag)
  {
     m_passwd = new wxTextCtrl(this, -1, wxEmptyString,
                               wxPoint(x + widthLabel + LAYOUT_X_MARGIN, y),
                               wxSize(widthText, heightText),
                               wxTE_PASSWORD|wxTE_PROCESS_ENTER);
     m_passwd->SetFocus();
  }
  else
  {
     m_text = new wxPTextEntry(strConfigPath, this, -1, wxEmptyString,
                               wxPoint(x + widthLabel + LAYOUT_X_MARGIN, y),
                               wxSize(widthText, heightText));
     m_text->SetFocus();
     m_passwd = NULL; // signal that it's not used
  }

  // buttons
  wxButton *btnOk = new
    wxButton(this, wxID_OK, _("OK"),
             wxPoint(widthDlg - 2*LAYOUT_X_MARGIN - 2*widthBtn,
                     heightDlg - LAYOUT_Y_MARGIN - heightBtn),
             wxSize(widthBtn, heightBtn));
  (void)new wxButton(this, wxID_CANCEL, _("Cancel"),
                     wxPoint(widthDlg - LAYOUT_X_MARGIN - widthBtn,
                             heightDlg - LAYOUT_Y_MARGIN - heightBtn),
                     wxSize(widthBtn, heightBtn));
  btnOk->SetDefault();

  // set position and size
  SetClientSize(widthDlg, heightDlg);
  Centre(wxCENTER_FRAME | wxBOTH);
}

bool MTextInputDialog::TransferDataToWindow()
{
   if ( m_passwd == NULL )
   {
      if ( m_strText.empty() )
      {
         // use the last previously entered value instead
         if ( m_text->GetCount() )
         {
            m_strText = m_text->GetString(0);
         }
      }

      m_text->SetValue(m_strText);

      // select everything so that it's enough to type a single letter to erase
      // the old value (this way it's as unobtrusive as you may get)
      m_text->SetSelection(-1, -1);
   }

   return true;
}

bool MTextInputDialog::TransferDataFromWindow()
{
   wxString strText;
   if(m_passwd)
      strText = m_passwd->GetValue();
   else
      strText = m_text->GetValue();
  if ( strText.empty() )
  {
    // imitate [Cancel] button
    EndModal(wxID_CANCEL);
    return FALSE;
  }

  m_strText = strText;
  return TRUE;
}

// a wxConfig-aware function which asks user for a string
bool MInputBox(wxString *pstr,
               const wxString& strCaption,
               const wxString& strPrompt,
               const wxWindow *parent,
               const wxChar *szKey,
               const wxChar *def,
               bool passwordflag)
{
  wxString strConfigPath;
  strConfigPath << _T("/Prompts/") << szKey;

  MTextInputDialog dlg(GetDialogParent(parent), *pstr,
                       strCaption, strPrompt, strConfigPath, def, passwordflag);

  // do not allow attempts to store the password:
  wxASSERT_MSG( !passwordflag || (szKey==NULL && def == NULL),
                _T("passwords can't be stored!") );

  if ( dlg.ShowModal() != wxID_OK )
     return false;

  *pstr = dlg.GetText();

  return true;
}

// ----------------------------------------------------------------------------
// other functions
// ----------------------------------------------------------------------------
void
MDialog_ErrorMessage(const wxChar *msg,
                     const wxWindow *parent,
                     const wxChar *title,
                     bool /* modal */)
{
   //MGuiLocker lock;
   CloseSplash();
   NoBusyCursor no;
   wxMessageBox(msg, wxString(M_TITLE_PREFIX) + title,
                GetMsgBoxStyle(wxOK|wxICON_EXCLAMATION),
                GetDialogParent(parent));
}


/** display system error message:
    @param message the text to display
    @param parent the parent frame
    @param title  title for message box window
    @param modal  true to make messagebox modal
   */
void
MDialog_SystemErrorMessage(const wxChar *message,
               const wxWindow *parent,
               const wxChar *title,
               bool modal)
{
   String
      msg;

   msg = String(message) + _("\nSystem error: ")
      + wxSafeConvertMB2WX(strerror(errno));

   MDialog_ErrorMessage(msg.c_str(), parent, wxString(M_TITLE_PREFIX)+title, modal);
}


/** display error message and exit application
       @param message the text to display
       @param title  title for message box window
       @param parent the parent frame
   */
void
MDialog_FatalErrorMessage(const wxChar *message,
              const wxWindow *parent,
              const wxChar *title)
{
   String msg = String(message) + _("\nExiting application...");

   MDialog_ErrorMessage(message,parent, wxString(M_TITLE_PREFIX)+title,true);
   mApplication->Exit();
}


/** display normal message:
       @param message the text to display
       @param parent the parent frame
       @param title  title for message box window
       @param modal  true to make messagebox modal
   */
bool
MDialog_Message(const wxChar *message,
                const wxWindow *parent,
                const wxChar *title,
                const wxChar *configPath,
                int flags)
{
   // if the msg box is disabled, don't make the splash disappear, return
   // immediately
   if ( wxPMessageBoxIsDisabled(configPath) )
      return true;

   CloseSplash();
   NoBusyCursor noBC;

   long style = GetMsgBoxStyle(wxOK | wxICON_INFORMATION);
   if ( flags & M_DLG_ALLOW_CANCEL )
      style |= wxCANCEL;

   wxPMessageBoxParams params;
   if ( flags & M_DLG_DISABLE )
      params.indexDisable = 0;

   return wxPMessageBox
          (
            configPath,
            message,
            String(M_TITLE_PREFIX) + title,
            style,
            GetDialogParent(parent),
            NULL,
            &params
          ) != wxCANCEL;
}

bool MDialog_Message(wxChar const *message,
                     const wxWindow *parent,
                     const MPersMsgBox *persMsg,
                     int flags,
                     wxChar const *title)
{
   String configPath;
   if ( persMsg )
      configPath = GetPersMsgBoxName(persMsg);

   return MDialog_Message(message, parent, title,
                          persMsg ? configPath.c_str()
                                  : (const wxChar *)NULL, flags);
}

MDlgResult MDialog_YesNoCancel(wxChar const *message,
                               const wxWindow *parent,
                               wxChar const *title,
                               int flags,
                               const MPersMsgBox *persMsg)
{
   CloseSplash();
   NoBusyCursor noBC;

   String configPath;
   if ( persMsg )
   {
      configPath = GetPersMsgBoxName(persMsg);
   }

   switch ( wxPMessageBox
            (
               configPath,
               message,
               String(M_TITLE_PREFIX) + title,
               GetYesNoMsgBoxStyle(flags) | wxCANCEL,
               GetDialogParent(parent)
            ) )
   {
      case wxNO:
         return MDlg_No;

      case wxYES:
         return MDlg_Yes;

      default:
         FAIL_MSG( _T("unexpected wxMessageBox return value") );
         // fall through

      case wxCANCEL:
         ;
   }

   // have to put it outside the switch to avoid compilation warnings from some
   // brain-dead compilers
   return MDlg_Cancel;
}

/** simple Yes/No dialog
    @param message the text to display
    @param parent the parent frame
    @param title  title for message box window
    @param modal  true to make messagebox modal
    @param YesDefault true if Yes button is default, false for No as default
    @return true if Yes was selected
*/
bool
MDialog_YesNoDialog(const wxChar *message,
                    const wxWindow *parent,
                    const wxChar *title,
                    int flags,
                    const MPersMsgBox *msgBox,
                    const wxChar *folderName)
{
   String pathGlobal,
          pathLocal;

   if ( msgBox )
   {
      pathGlobal = GetPersMsgBoxName(msgBox);

      // first check the global value
      int storedValue = wxPMessageBoxIsDisabled(pathGlobal);

      // if this is a folder-specific message box, then look if it's not
      // disabled just for this folder
      if ( !storedValue && folderName )
      {
         pathLocal << Profile::FilterProfileName(folderName)
                   << '/'
                   << pathGlobal;

         storedValue = wxPMessageBoxIsDisabled(pathLocal);
      }

      if ( storedValue )
      {
         // don't execute the code below at all, it has a nice side effect of
         // _not_ closing the splash unless the message box is really shown
         return storedValue == wxYES;
      }

      if ( pathLocal.empty() )
         pathLocal = pathGlobal;
   }

   NoBusyCursor noBC;
   CloseSplash();

   wxPMessageBoxParams params;
   params.dontDisableOnNo = (flags & M_DLG_NOT_ON_NO) != 0;

   if ( folderName )
   {
      params.disableOptions.Add(_("just for this &folder"));
      params.disableOptions.Add(_("for &all folders"));
   }

   if ( flags & M_DLG_DISABLE )
   {
      params.indexDisable = 0;
   }

   int rc = wxPMessageBox
            (
               pathLocal,
               message,
               String(M_TITLE_PREFIX) + title,
               GetYesNoMsgBoxStyle(flags),
               GetDialogParent(parent),
               NULL,
               &params
            );

   if ( params.indexDisable != -1 )
   {
      // a folder-specific message box can be disabled just for this folder or
      // for all of them globally
      if ( folderName && params.indexDisable == 1 )
      {
         // we don't need the local key
         wxPMessageBoxEnable(pathLocal);

         // as we disable it globally anyhow
         wxPMessageBoxDisable(pathGlobal, rc);
      }
      //else: it's already disabled for this folder by wxPMessageBox()

      // show a message about how to enable this dialog back
      MDialog_Message
      (
         _("Remember that you can reenable this and other dialogs\n"
           "by using the button in the last page of the \"Preferences\"\n"
           "dialog.\n"),
         parent,
         MDIALOG_MSGTITLE,
         GetPersMsgBoxName(M_MSGBOX_REENABLE_HINT),
         M_DLG_DISABLE
      );
   }

   return rc == wxYES;
}


/** Filerequester
       @param message the text to display
       @param parent the parent frame
       @param path   default path
       @param filename  default filename
       @param extension default extension
       @param wildcard  pattern matching expression
       @param save   if true, for saving a file
       @return pointer to a temporarily allocated buffer with he filename, or NULL
   */
String
MDialog_FileRequester(String const & message,
                      wxWindow *parent,
                      String path,
                      String filename,
                      String extension,
                      String wildcard,
                      bool save,
                      Profile * /* profile */)
{
   //MGuiLocker lock;
   CloseSplash();


   if(parent == NULL)
      parent = mApplication->TopLevelFrame();

   // TODO we save only one file name for all "open file" dialogs and one for
   //      all "save file" dialogs - may be should be more specific (add
   //      configPath parameter to MDialog_FileRequester?)
   return wxPFileSelector(save ? _T("save") : _T("load"),
                          message, path, filename, extension,
                          wildcard, 0, (wxWindow *)parent);
}

String MDialog_DirRequester(const String& message,
                            const String& pathOrig,
                            wxWindow *parent,
                            const wxChar *confpath)
{
   return wxPDirSelector(confpath, message, pathOrig, parent);
}

void
MDialog_ShowTip(const wxWindow *parent)
{
   String dir, filename;

   // Tips files are in $prefix/share/mahogany/doc/Tips/
   dir = mApplication->GetDataDir();
   if ( !dir )
   {
      // like this, it will work in an uninstalled copy of M too
      dir = _T("..");
   }

   dir << DIR_SEPARATOR << _T("doc") << DIR_SEPARATOR
#ifndef OS_WIN
       << _T("Tips") << DIR_SEPARATOR
#endif // !Windows
       ;

   // Tips files are either Tips_LOCALENAME.txt, e.g. Tips_de.txt or
   // simply Tips.txt

   filename = _T("Tips");

#ifdef USE_I18N
   wxLocale * locale = wxGetLocale();
   if(locale)
      filename << '_' << locale->GetLocale();
#endif // USE_I18N

   filename << _T(".txt");

   if(! wxFileExists(dir+filename))
      filename = _T("Tips.txt");

   wxTipProvider *tipProvider =
      wxCreateFileTipProvider(dir+filename, READ_APPCONFIG(MP_LASTTIP));

   bool showOnStarup = wxShowTip((wxWindow *)parent, tipProvider);

   Profile *profile = mApplication->GetProfile();
   if ( showOnStarup != READ_APPCONFIG_BOOL(MP_SHOWTIPS) )
   {
      profile->writeEntry(MP_SHOWTIPS, showOnStarup);
   }

   profile->writeEntry(MP_LASTTIP, tipProvider->GetCurrentTip());

   delete tipProvider;
}

void
MDialog_FolderProfile(const wxWindow *parent, const String& folderName)
{
   MFolder_obj folder(MFolder::Get(folderName));

   if ( folder )
   {
      ShowFolderPropertiesDialog(folder, (wxWindow *)parent);
   }
}

void
MDialog_FolderOpen(const wxWindow *parent)
{
   MFolder *folder = MDialog_FolderChoose(parent, NULL, true /* open */);
   if ( folder != NULL )
   {
      // open a view on this folder
      OpenFolderViewFrame(folder, (wxWindow *)parent);
      folder->DecRef();
   }
   //else: cancelled
}

// ----------------------------------------------------------------------------
// folder dialog stuff
// ----------------------------------------------------------------------------

MFolderDialog::MFolderDialog(wxWindow *parent, MFolder *folder, int flags)
             : wxManuallyLaidOutDialog(parent,
                                       _("Choose folder"),
                                       _T("FolderSelDlg"))
{
   m_flags = flags;
   m_userChoseFolder = false;

   m_folder = folder;
   SafeIncRef(m_folder);

   wxLayoutConstraints *c;

   // create box and buttons
   // ----------------------

   wxStaticBox *box = CreateStdButtonsAndBox(_("Please select a folder"));

   wxWindow *btnOk = FindWindow(wxID_OK);

   // File button
   if ( !(m_flags & MDlg_Folder_NoFiles) )
   {
      wxButton *btnFile = new wxButton(this, wxID_OPEN, _("&File..."));
      c = new wxLayoutConstraints;
      c->height.SameAs(btnOk, wxHeight);
      c->width.SameAs(btnOk, wxWidth);
      c->right.SameAs(btnOk, wxLeft, 2*LAYOUT_X_MARGIN);
      c->bottom.SameAs(btnOk, wxBottom);
      btnFile->SetConstraints(c);
   }

   // Help button
   wxButton *btnHelp = new wxButton(this, wxID_HELP, _("Help"));
   c = new wxLayoutConstraints;
   c->height.SameAs(btnOk, wxHeight);
   c->width.SameAs(btnOk, wxWidth);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(btnOk, wxBottom);
   btnHelp->SetConstraints(c);

   // create the folder tree control
   // ------------------------------

   m_tree = new MFolderDialogTree(this);

   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);

   m_winTree = m_tree->GetWindow();
   m_winTree->SetConstraints(c);

   // position the dialog
   SetDefaultSize(6*wBtn, 20*hBtn);
}

MFolderDialog::~MFolderDialog()
{
   SafeDecRef(m_folder);

   delete m_tree;
}

void
MFolderDialog::OnButton(wxCommandEvent &ev)
{
   switch ( ev.GetId() )
   {
      case wxID_OPEN:
         m_FileName = wxPFileSelector
                      (
                        "FolderDialogFile",
                        _("Mahogany: Please choose a folder file"),
                        NULL, NULL, NULL, NULL,
                        m_flags & MDlg_Folder_Open
                           ? wxFD_OPEN | wxFD_FILE_MUST_EXIST
                           : wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
                        this
                      );
         if ( !m_FileName.empty() )
         {
            // folder (file) chosen
            if ( TransferDataFromWindow() )
               EndModal(wxID_OK);
         }
         break;

      case wxID_HELP:
         mApplication->Help(MH_DIALOG_FOLDERDLG);
         break;

      default:
         ev.Skip();
   }
}

wxString MFolderDialog::GetConfigPath()
{
   wxString path;
   path << _T('/') << M_SETTINGS_CONFIG_SECTION << _T("/LastPickedFolder");
   return path;
}

bool MFolderDialog::TransferDataToWindow()
{
   // restore last folder from config (not Profile as we put it in
   // M_SETTINGS_CONFIG_SECTION, i.e. outside M_PROFILE_CONFIG_SECTION)
   wxConfigBase *config = wxConfigBase::Get();
   if ( config )
   {
      wxString folderName = config->Read(GetConfigPath(), wxEmptyString);
      if ( !folderName.empty() )
      {
         // select folder in the tree
         MFolder_obj folder(folderName);
         if ( folder )
         {
            if ( !m_tree->SelectFolder(folder) )
            {
               wxLogDebug(_T("Couldn't restore the last selected folder in the tree."));
            }
         }
         //else: the folder was probably destroyed since the last time
      }
   }

   // we can only do it now as wxGTK doesn't allow us to set the focus from
   // ctor
   m_winTree->SetFocus();

   return true;
}

bool MFolderDialog::TransferDataFromWindow()
{
   if ( m_FileName.Length() == 0 )
   {
      SafeDecRef(m_folder);
      m_folder = m_tree->GetSelection();
      if ( m_folder != NULL )
      {
         // save the folder name to config
         wxConfigBase *config = wxConfigBase::Get();
         if ( config )
         {
            config->Write(GetConfigPath(), m_folder->GetFullName());
         }
      }
   }
   else // file has been chosen
   {
      // the name of the folder can't contain '/' and such, so take just the
      // name, not the full name as the folder name
      wxString name;
      wxSplitPath(m_FileName, NULL, &name, NULL);

      SafeDecRef(m_folder);
      m_folder = MFolder::CreateTempFile(name, m_FileName, 0);
   }

   m_userChoseFolder = true;

   return true;
}

MFolder *
MDialog_FolderChoose(const wxWindow *parent, MFolder *folder, int flags)
{
   // TODO store the last folder in config
   MFolderDialog dlg((wxWindow *)parent, folder, flags);

   return dlg.ShowModal() == wxID_OK ? dlg.GetFolder() : NULL;
}


//-----------------------------------------------------------------------------


static wxString DateFormatsLabels[] =
{
   gettext_noop("31 (Day)"),
   gettext_noop("Mon"),
   gettext_noop("Monday"),
   gettext_noop("12 (Month)"),
   gettext_noop("Nov"),
   gettext_noop("November"),
   gettext_noop("99 (year)"),
   gettext_noop("1999 (year)"),
   gettext_noop("24 (hour)"),
   gettext_noop("12 (hour)"),
   gettext_noop("am/pm"),
   gettext_noop("59 (minutes)"),
   gettext_noop("29 (seconds)"),
   gettext_noop("EST (timezone)"),
   gettext_noop("Default format including time"),
   gettext_noop("Default format without time")
};

static wxString DateFormats[] =
{
   _T("%d"),
   _T("%a"),
   _T("%A"),
   _T("%m"),
   _T("%b"),
   _T("%B"),
   _T("%y"),
   _T("%Y"),
   _T("%H"),
   _T("%I"),
   _T("%p"),
   _T("%M"),
   _T("%S"),
   _T("%Z"),
   _T("%c"),
   _T("%x")
};

static const int NUM_DATE_FMTS  = WXSIZEOF(DateFormats);
static const int NUM_DATE_FMTS_LABELS  = WXSIZEOF(DateFormatsLabels);

static const int NUM_DATE_FIELDS = WXSIZEOF(DateFormats);;

class wxDateTextCtrl : public wxTextCtrl
{
public:
   wxDateTextCtrl(wxWindow *parent) : wxTextCtrl(parent,-1)
      {
         m_menu = new wxMenu(wxEmptyString, wxMENU_TEAROFF);
         m_menu->SetTitle(_("Format Specifiers:"));
         for ( int n = 0; n < NUM_DATE_FMTS;n++ )
            m_menu->Append(n, wxGetTranslation(DateFormatsLabels[n]));
      }
   ~wxDateTextCtrl()
      { delete m_menu; }
   void OnRClick(wxMouseEvent& event)
      { (void)PopupMenu(m_menu, event.GetPosition()); }
   void OnMenu(wxCommandEvent &event)
      {
         ASSERT(event.GetId() >= 0 && event.GetId() < NUM_DATE_FMTS);
         WriteText(DateFormats[event.GetId()]);
      }

protected:
   wxMenu *m_menu;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxDateTextCtrl)
};

BEGIN_EVENT_TABLE(wxDateTextCtrl, wxTextCtrl)
   EVT_MENU(-1, wxDateTextCtrl::OnMenu)
   EVT_RIGHT_DOWN(wxDateTextCtrl::OnRClick)
END_EVENT_TABLE()

class wxDateFmtDialog : public wxOptionsPageSubdialog
{
public:
   // ctor & dtor
   wxDateFmtDialog(Profile *profile, wxWindow *parent);
   virtual ~wxDateFmtDialog() { m_timer->Stop(); delete m_timer; }

   // transfer data to/from dialog
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();

   // returns TRUE if the format string was changed
   bool WasChanged(void) { return m_DateFmt != m_OldDateFmt;}

   // update the example control to show the current type
   void UpdateExample();

   // event handlers
   void OnUpdate(wxCommandEvent& /* event */) { UpdateExample(); }

protected:
   // update timer
   class ExampleUpdateTimer : public wxTimer
   {
   public:
      ExampleUpdateTimer(wxDateFmtDialog *dialog)
      {
         m_dialog = dialog;
      }

      virtual void Notify() { m_dialog->UpdateExample(); }

   private:
      wxDateFmtDialog *m_dialog;

      DECLARE_NO_COPY_CLASS(ExampleUpdateTimer)
   } *m_timer;

   // data
   wxString  m_DateFmt,
             m_OldDateFmt;

   // GUI controls
   wxCheckBox   *m_UseGMT;       // checked => use GMT for time display
   wxStaticText *m_statExample;  // shows the example of current format
   wxTextCtrl   *m_textctrl;     // the time/date format to use

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxDateFmtDialog)
};

BEGIN_EVENT_TABLE(wxDateFmtDialog, wxOptionsPageSubdialog)
   EVT_TEXT(-1, wxDateFmtDialog::OnUpdate)
   EVT_CHECKBOX(-1, wxDateFmtDialog::OnUpdate)
END_EVENT_TABLE()

#ifdef _MSC_VER
   // 'this' : used in base member initializer list (so what??)
#   pragma warning(disable:4355)
#endif

wxDateFmtDialog::wxDateFmtDialog(Profile *profile, wxWindow *parent)
               : wxOptionsPageSubdialog(profile,
                                        parent,
                                        _("Date Format"),
                                        _T("DateFormatDialog"))
{
   wxASSERT(NUM_DATE_FMTS == NUM_DATE_FMTS_LABELS);

   wxString foldername = profile->GetFolderName();
   wxString labelBox;
   if ( !foldername.empty() )
      labelBox.Printf(_("&Date format for folder '%s'"), foldername.c_str());
   else
      labelBox.Printf(_("&Default date format"));

   wxStaticBox *box = CreateStdButtonsAndBox(labelBox, FALSE, MH_DIALOG_DATEFMT);

   wxLayoutConstraints *c;

   wxStaticText *stattext = new wxStaticText
                                (
                                 this,
                                 -1,
                                 _("Press the right mouse button over the\n"
                                   "input field to insert format specifiers.")
                                );
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);

   m_textctrl = new wxDateTextCtrl(this);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(stattext, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_textctrl->SetConstraints(c);

   stattext = new wxStaticText(this, -1, _("Here is how it will look like: "));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_textctrl, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);

   m_statExample = new wxStaticText(this, -1, wxEmptyString);
   c = new wxLayoutConstraints;
   c->left.RightOf(stattext);
   c->top.Below(m_textctrl, 2*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_statExample->SetConstraints(c);

   m_UseGMT = new wxCheckBox(this, -1, _("Display time in GMT/UST."));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_statExample, 4*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_UseGMT->SetConstraints(c);

   SetDefaultSize(5*wBtn, 8*hBtn, TRUE /* minimal */);
   TransferDataToWindow();
   m_OldDateFmt = m_DateFmt;

   m_timer = new ExampleUpdateTimer(this);

   // update each second
   m_timer->Start(1000);
}

#ifdef _MSC_VER
#   pragma warning(default:4355)
#endif

void
wxDateFmtDialog::UpdateExample()
{
   time_t ltime;
   (void)time(&ltime);

   m_statExample->SetLabel(strutil_ftime(ltime,
                                         m_textctrl->GetValue(),
                                         m_UseGMT->GetValue()));
}

bool
wxDateFmtDialog::TransferDataFromWindow()
{
   m_DateFmt = m_textctrl->GetValue();
   GetProfile()->writeEntry(MP_DATE_FMT, m_DateFmt);
   GetProfile()->writeEntry(MP_DATE_GMT, m_UseGMT->GetValue());

   return TRUE;
}

bool
wxDateFmtDialog::TransferDataToWindow()
{
#ifdef OS_WIN
   // MP_DATE_FMT contains '%' which are being (mis)interpreted as env var
   // expansion characters under Windows
   ProfileEnvVarSave noEnvVars(GetProfile());
#endif // OS_WIN

   m_DateFmt = READ_CONFIG_TEXT(GetProfile(), MP_DATE_FMT);
   m_UseGMT->SetValue( READ_CONFIG_BOOL(GetProfile(), MP_DATE_GMT));
   m_textctrl->SetValue(m_DateFmt);

   return TRUE;
}


/* Configuration dialog for showing the date. */
extern
bool ConfigureDateFormat(Profile *profile, wxWindow *parent)
{
   wxDateFmtDialog dlg(profile, parent);

   return dlg.ShowModal() == wxID_OK && dlg.WasChanged();
}

wxXFaceButton::wxXFaceButton(wxWindow *parent, int id,  wxString filename)
{
   m_Parent = parent;
   wxBitmap bmp = mApplication->GetIconManager()->GetBitmap(_T("noxface"));
   wxBitmapButton::Create(parent,id, bmp, wxDefaultPosition,
                          wxSize(64,64));
   SetFile(filename);
}

void
wxXFaceButton::SetFile(const wxString &filename)
{
   wxBitmap bmp;
   if(filename.Length() != 0)
   {
      bool success = FALSE;
      if(wxFileExists(filename))
         bmp = wxBitmap(XFace::GetXFaceImg(filename, &success, m_Parent));
      if(! success)
      {
         bmp = wxBitmap(((wxMApp *)mApplication)->GetStdIcon(wxICON_HAND));
         m_XFace = wxEmptyString;
      }
      else
         m_XFace = filename;
   }
   else
      bmp = mApplication->GetIconManager()->wxIconManager::GetBitmap(_T("noxface"));
   SetBitmapLabel(bmp);
   SetBitmapFocus(bmp);
   SetBitmapSelected(bmp);
}


class wxXFaceDialog : public wxOptionsPageSubdialog
{
public:
   wxXFaceDialog(Profile *profile, wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();
   bool WasChanged(void)
      {
         return
            m_Button->GetFile() != m_OldXFace
            || m_Checkbox->GetValue() != m_OldUseXFace;
      };

   void OnButton(wxCommandEvent & event );
protected:
   wxString     m_OldXFace;
   bool          m_OldUseXFace;

   wxXFaceButton *m_Button;
   wxCheckBox    *m_Checkbox;
   bool m_Changed;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxXFaceDialog)
};

BEGIN_EVENT_TABLE(wxXFaceDialog, wxOptionsPageSubdialog)
   EVT_BUTTON(-1, wxXFaceDialog::OnButton)
   EVT_CHECKBOX(-1, wxXFaceDialog::OnButton)
END_EVENT_TABLE()


wxXFaceDialog::wxXFaceDialog(Profile *profile,
                             wxWindow *parent)
   : wxOptionsPageSubdialog(profile,parent,
                            _("Choose a XFace"),
                            _T("XFaceChooser"))
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("XFace"), FALSE,
                                             MH_DIALOG_XFACE);
   wxLayoutConstraints *c;

   wxStaticText *stattext = new wxStaticText(this, -1,
                                             _("XFaces are small images that can be included\n"
                                               "in your mail message to help recognise you.\n"
                                               "They are limited to 48*48 pixels black and white,\n"
                                               "but Mahogany will automatically convert any file\n"
                                               "to this specification automatically.\n"
                                               "\n"
                                               "Click on the button to change it."));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 6*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);


   m_Button = new wxXFaceButton(this, -1, wxEmptyString);
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(stattext, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_Button->SetConstraints(c);

   m_Checkbox = new wxCheckBox(this, -1, _("Use XFace."));
   c = new wxLayoutConstraints;
   c->left.RightOf(m_Button, 4*LAYOUT_X_MARGIN);
   c->top.Below(stattext, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_Checkbox->SetConstraints(c);

   m_Changed = FALSE;

   SetDefaultSize(325, 348, TRUE /* minimal */);
   TransferDataToWindow();
   m_OldXFace = m_Button->GetFile();
   m_OldUseXFace = m_Checkbox->GetValue();
}


void
wxXFaceDialog::OnButton(wxCommandEvent & event )
{
   wxObject *obj = event.GetEventObject();
   if ( obj != m_Button && obj != m_Checkbox)
   {
      event.Skip();
      return;
   }

   if(obj == m_Button)
   {
      wxString path, file, xface;
      String newface;
      xface = m_Button->GetFile();
      path = xface.BeforeLast('/');
      file = xface.AfterLast('/');
      newface = wxPLoadExistingFileSelector
                (
                  this,
                  GetProfile()->GetName() + _T("/xfacefilerequester"),
                  _("Please pick an image file"),
                  path, file, NULL
                );
      m_Button->SetFile(newface);
   }
   m_Button->Enable(m_Checkbox->GetValue() != 0);
}

bool
wxXFaceDialog::TransferDataToWindow()
{
   m_Button->SetFile(READ_CONFIG(GetProfile(), MP_COMPOSE_XFACE_FILE));
   m_Checkbox->SetValue(READ_CONFIG_BOOL(GetProfile(), MP_COMPOSE_USE_XFACE));
   m_Button->Enable(m_Checkbox->GetValue() != 0);
   return TRUE;
}

bool
wxXFaceDialog::TransferDataFromWindow()
{
   GetProfile()->writeEntry(MP_COMPOSE_XFACE_FILE, m_Button->GetFile());
   GetProfile()->writeEntry(MP_COMPOSE_USE_XFACE, m_Checkbox->GetValue());
   return TRUE;
}

extern
bool PickXFaceDialog(Profile *profile, wxWindow *parent)
{
   wxXFaceDialog dlg(profile, parent);
   return ( dlg.ShowModal() == wxID_OK && dlg.WasChanged() );
}

// ----------------------------------------------------------------------------
// global password UI support: this password is used for TwoFish encryption
// ----------------------------------------------------------------------------

class ChangePasswdTraversal : public MFolderTraversal
{
public:
   ChangePasswdTraversal(MFolder* folder,
                         bool oldUseCrypt,
                         const wxString &oldPw,
                         bool newUseCrypt,
                         const wxString &newPw) : MFolderTraversal(*folder)
      {
         m_NewPw = newPw; m_OldPw = oldPw;
         m_OldUC = oldUseCrypt;
         m_NewUC = newUseCrypt;
      }

   virtual bool OnVisitFolder(const wxString& folderName)
      {
         static const wxChar * keys[] =
         {
            MP_FOLDER_PASSWORD,
            MP_SMTPHOST_PASSWORD,
            MP_NNTPHOST_LOGIN,
            NULL
         };
         for(int idx = 0; keys[idx]; idx++)
         {
            Profile *p=mApplication->GetProfile();
            Profile_obj profile(folderName);
            if(profile->HasEntry(keys[idx])) // don't work on inherited ones
            {
               wxString val = profile->readEntry(keys[idx], wxEmptyString);
               if(val != wxEmptyString)
               {
                  p->writeEntry(MP_CRYPTALGO, m_OldUC);
                  strutil_setpasswd(m_OldPw);
                  wxString data = strutil_decrypt(val);
                  p->writeEntry(MP_CRYPTALGO, m_NewUC);
                  strutil_setpasswd(m_NewPw);
                  profile->writeEntry(keys[idx], strutil_encrypt(data));
               }
            }
         }
         return TRUE;
      }
   ~ChangePasswdTraversal()
      {
         m_OldPw = _T("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
         m_NewPw = _T("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      }
private:
   wxString m_OldPw, m_NewPw;
   bool m_OldUC, m_NewUC;
};


class wxGlobalPasswdDialog : public wxOptionsPageSubdialog
{
public:
   wxGlobalPasswdDialog(Profile *profile, wxWindow *parent);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();
   bool WasChanged(void) const
      {
         // difficult to say, be cautious:
         return TRUE;
      };

   void OnButton(wxCommandEvent & event );
   void OnUpdateUI(wxUpdateUIEvent& event);
   void DoUpdateUI();

protected:
   wxString m_NewPassword;

   wxCheckBox *m_UseGlobalPassword;
   wxTextCtrl *m_oPassword,
              *m_nPassword,
              *m_nPassword2;

   wxStaticText *m_text1,
                *m_text2;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxGlobalPasswdDialog)
};

BEGIN_EVENT_TABLE(wxGlobalPasswdDialog, wxOptionsPageSubdialog)
   EVT_UPDATE_UI(-1, wxGlobalPasswdDialog::OnUpdateUI)
END_EVENT_TABLE()

void wxGlobalPasswdDialog::OnUpdateUI(wxUpdateUIEvent& /* event */)
{
   DoUpdateUI();
}

void wxGlobalPasswdDialog::DoUpdateUI()
{
   bool enable = m_UseGlobalPassword->GetValue();
   m_nPassword->Enable(enable);
   m_nPassword2->Enable(enable);
   m_text1->Enable(enable);
   m_text2->Enable(enable);
}

wxGlobalPasswdDialog::wxGlobalPasswdDialog(Profile *profile,
                                           wxWindow *parent)
   : wxOptionsPageSubdialog(profile,parent,
                            _("Choose a global password"),
                            _T("GlobalPasswdChooser"))
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("Global Password Settings"), FALSE,
                                             MH_DIALOG_GLOBALPASSWD);
   wxLayoutConstraints *c;
   wxStaticText *stattext = new wxStaticText
                                (
                                 this, -1,
                                 _("Mahogany can use a global password setting\n"
                                   "to protect all sensitive information, like\n"
                                   "server passwords, in your configuration files.\n"
                                   "\n"
                                   "If you do not want to use a global password,\n"
                                   "leave this dialog and disable the use of it\n"
                                   "in the preferences dialog."
                                  )
                                );

   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 6*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);


   wxStaticText *text = new wxStaticText(this, -1,
                                         _("Use global password:"));
   c = new wxLayoutConstraints;
   c->top.Below(stattext, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(stattext, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   text->SetConstraints(c);

   m_UseGlobalPassword = new wxCheckBox
                             (
                              this, -1,
                              _("(this implies using stronger encryption)")
                             );
   c = new wxLayoutConstraints;
   c->left.RightOf(text, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(text, wxTop);
   c->width.AsIs();
   c->height.AsIs();
   m_UseGlobalPassword->SetConstraints(c);

   wxWindow *last = m_UseGlobalPassword;

   if ( READ_CONFIG(profile, MP_CRYPTALGO) )
   {
      wxStaticText *label = new wxStaticText(this, -1, _("Old password:"));
      c = new wxLayoutConstraints;
      c->top.Below(last, 4*LAYOUT_Y_MARGIN);
      c->left.SameAs(stattext, wxLeft, 2*LAYOUT_X_MARGIN);
      c->width.AsIs();
      c->height.AsIs();
      label->SetConstraints(c);

      m_oPassword = new wxTextCtrl(this, -1, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_PASSWORD);
      c = new wxLayoutConstraints;
      c->left.RightOf(label, 2*LAYOUT_X_MARGIN);
      c->centreY.SameAs(label, wxCentreY);
      c->width.AsIs();
      c->height.AsIs();
      m_oPassword->SetConstraints(c);

      last = m_oPassword;
   }
   else
   {
      m_oPassword = NULL;
   }

   m_text1 = new wxStaticText(this, -1, _("New password:"));
   c = new wxLayoutConstraints;
   c->top.Below(last, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(stattext, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_text1->SetConstraints(c);

   m_nPassword = new wxTextCtrl(this, -1, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_PASSWORD);
   c = new wxLayoutConstraints;
   c->left.RightOf(m_text1, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(m_text1, wxCentreY);
   c->width.AsIs();
   c->height.AsIs();
   m_nPassword->SetConstraints(c);

   m_text2 = new wxStaticText(this, -1, _("Retype password:"));
   c = new wxLayoutConstraints;
   c->top.Below(m_nPassword, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(stattext, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_text2->SetConstraints(c);

   m_nPassword2 = new wxTextCtrl(this, -1, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_PASSWORD);
   c = new wxLayoutConstraints;
   c->left.RightOf(m_text2, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(m_text2, wxCentreY);
   c->width.AsIs();
   c->height.AsIs();
   m_nPassword2->SetConstraints(c);

   SetDefaultSize(360, 400, TRUE /* minimal */);
   TransferDataToWindow();
}

bool
wxGlobalPasswdDialog::TransferDataToWindow()
{
   m_UseGlobalPassword->SetValue(READ_APPCONFIG_BOOL(MP_CRYPTALGO));
   DoUpdateUI();

   return TRUE;
}

bool
wxGlobalPasswdDialog::TransferDataFromWindow()
{
   Profile * p = mApplication->GetProfile();

   wxString oldPw = strutil_getpasswd();
   bool oldUseCrypt = READ_CONFIG_BOOL(p, MP_CRYPTALGO);

   if ( m_oPassword )
   {
      // check the old password was given correctly first
      if ( !strutil_checkpasswd(m_oPassword->GetValue()) )
      {
         wxLogError(_("Incorrect old password value!"));
         return FALSE;
      }
   }

   bool newUseCrypt = m_UseGlobalPassword->GetValue();
   wxString newPw;
   if ( newUseCrypt )
   {
      newPw = m_nPassword->GetValue();

      if ( newPw.empty() )
      {
         wxLogError(_("Password can't be empty."));
         return FALSE;
      }

      if ( newPw != m_nPassword2->GetValue() )
      {
         wxLogError(_("The two values for the password do not match!"));
         return FALSE;
      }
   }

   // write test data encrypted with new password
   p->writeEntry(MP_CRYPTALGO, newUseCrypt);
   if ( !newPw.empty() )
   {
      strutil_setpasswd(newPw);
   }

   // reencrypt all existing passwords with the new one
   if(oldPw != newPw || oldUseCrypt != newUseCrypt)
   {
      MFolder_obj folderRoot(wxEmptyString);
      ChangePasswdTraversal traverse(folderRoot,
                                     oldUseCrypt, oldPw,
                                     newUseCrypt, newPw);
      traverse.Traverse();
   }

   return TRUE;
}

extern
bool PickGlobalPasswdDialog(Profile *profile, wxWindow *parent)
{
   wxGlobalPasswdDialog dlg(profile, parent);

   return dlg.ShowModal() == wxID_OK && dlg.WasChanged();
}

// ----------------------------------------------------------------------------
// CheckExpungeDialog
// ----------------------------------------------------------------------------

extern
void CheckExpungeDialog(ASMailFolder *asmf, wxWindow *parent)
{
   // For all non-NNTP folders, check if the user wants to auto-expunge the
   // messages?
   if ( CanDeleteMessagesInFolder(asmf->GetType()) )
   {
      // are there any messages to expunge?
      MailFolder *mf = asmf->GetMailFolder();
      if ( !mf )
         return;

      if ( mf->IsOpened() && mf->CountDeletedMessages() )
      {
         String msg;
         msg.Printf(_("Do you want to expunge all deleted messages\n"
                      "in folder '%s'?"),
                    mf->GetName().c_str());

         if ( MDialog_YesNoDialog(msg, parent, MDIALOG_YESNOTITLE,
                                  M_DLG_NO_DEFAULT,
                                  M_MSGBOX_AUTOEXPUNGE,
                                  mf->GetName()) )
         {
            (void) mf->ExpungeMessages();
         }
      }

      mf->DecRef();
   }
}

// ----------------------------------------------------------------------------
// reenable previously disabled persistent message boxes
// ----------------------------------------------------------------------------

class ReenableDialog : public wxManuallyLaidOutDialog
{
public:
   ReenableDialog(wxWindow *parent);

   // adds all disabled message boxes for the given folder to the listctrl and
   // remember their paths in entries parameter so that they could be deleted
   // later
   void AddAllEntries(const ConfigSource *config,
                      const String& folder,
                      wxArrayString& entries);

   // get the selected items
   const wxArrayInt& GetSelections() const { return m_selections; }

   // return TRUE if there are any msg boxes to reenable
   bool ShouldShow() const { return m_listctrl->GetItemCount() != 0; }

   // transfer data (selections) from control
   virtual bool TransferDataFromWindow();

private:
   wxArrayInt  m_selections;
   wxListCtrl *m_listctrl;

   DECLARE_NO_COPY_CLASS(ReenableDialog)
};

ReenableDialog::ReenableDialog(wxWindow *parent)
              : wxManuallyLaidOutDialog(parent,
                                        _("Mahogany"),
                                        _T("ReenableDialog"))
{
   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // Ok and Cancel buttons and a static box around everything else
   wxStaticBox *box = CreateStdButtonsAndBox(_("&Settings"));

   // an explanatory text
   wxStaticText *text = new wxStaticText(this, -1,
                                         _("Select the settings to reset"));
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   text->SetConstraints(c);

   // and a listctrl
   m_listctrl = new wxPListCtrl(_T("PMsgBoxList"), this, -1,
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT);
   c = new wxLayoutConstraints;
   c->top.Below(text, 2*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_listctrl->SetConstraints(c);

   // setup the listctrl columns
   m_listctrl->InsertColumn(0, _("Setting"));
   m_listctrl->InsertColumn(1, _("Value"));
   m_listctrl->InsertColumn(2, _("Folder"));

   // finishing initialization
   // ------------------------

   m_listctrl->SetFocus();

   // set the minimal and initial window size
   SetDefaultSize(6*wBtn, 8*hBtn);
}

void ReenableDialog::AddAllEntries(const ConfigSource *config,
                                   const String& folder,
                                   wxArrayString& entries)
{
   wxString key(MESSAGE_BOXES_ROOT);
   if ( !folder.empty() )
      key << '/' << folder;

   ConfigSource::EnumData dummy;
   String name;
   for ( bool cont = config->GetFirstEntry(key, name, dummy);
         cont;
         cont = config->GetNextEntry(name, dummy) )
   {
      // decode the remembered value
      String value;
      long val;
      if ( !config->Read(name, &val) )
         continue;

      switch ( val )
      {
         case wxYES:
         case 0x0020:   // old value of wxYES
            value = _("yes");
            break;

         case wxNO:
         case 0x0040:   // old value of wxNO
            value = _("no");
            break;

         case -1:
            // this is a special hack used by persistent msg boxes, ignore it,
            // this one is not really disabled
            continue;

         default:
            FAIL_MSG( _T("unknown message box value") );
            // fall through

         case wxOK:
            // must be a simple msg box
            value = _("off");
      }

      int index = m_listctrl->GetItemCount();
      m_listctrl->InsertItem(index, GetPersMsgBoxHelp(name));
      m_listctrl->SetItem(index, 1, value);

      String folderName;
      if ( !folder )
      {
         folderName = _("all folders");
      }
      else
      {
         // previous versions of the program prefixed the folder names with
         // "M_Profiles_", the current version of Profile::FilterProfileName()
         // doesn't do it any more but we still have to deal with the old
         // config files
         if ( !folder.StartsWith(_T("M_Profiles_"), &folderName) )
         {
            folderName = folder;
         }

         // restore the folder separator (i.e. undo what FilterProfileName()
         // did)
         //
         // FIXME: this won't work correctly for the folder names with '_'s in
         //        them - not critical but annoying
         folderName.Replace(_T("_"), _T("/"));
      }

      m_listctrl->SetItem(index, 2, folderName);

      // and remember the config path in case we'll delete it later
      if ( !!folder )
      {
         name.Prepend(folder + _T('/'));
      }
      entries.Add(name);
   }
}

bool ReenableDialog::TransferDataFromWindow()
{
   ASSERT_MSG( m_selections.IsEmpty(),
               _T("TransferDataFromWindow() called twice?") );

   int index = -1;
   for ( ;; )
   {
      index = m_listctrl->GetNextItem(index,
                                      wxLIST_NEXT_ALL,
                                      wxLIST_STATE_SELECTED);
      if ( index == -1 )
         break;

      m_selections.Add(index);
   }

   return TRUE;
}

// TODO: all this should be implemented in wx/persctrl.cpp, not here!

extern
bool ReenablePersistentMessageBoxes(wxWindow *parent)
{
   // NB: working with global config in M is tricky - here, we need to create
   //     the dialog before setting the config path because the dlg ctor will
   //     change the current path in the global config...
   ReenableDialog dlg(parent);

   // get all entries under Settings/MessageBox in all config sources
   wxArrayString entries;

   const AllConfigSources::List& sources = AllConfigSources::Get().GetSources();
   for ( AllConfigSources::List::iterator config = sources.begin(),
                                             end = sources.end();
         config != end;
         ++config )
   {
      dlg.AddAllEntries(config.operator->(), wxEmptyString, entries);

      ConfigSource::EnumData dummy;
      String name;
      bool cont = config->GetFirstGroup(MESSAGE_BOXES_ROOT, name, dummy);
      while ( cont )
      {
         dlg.AddAllEntries(config.operator->(), name, entries);

         cont = config->GetNextGroup(name, dummy);
      }
   }

   if ( dlg.ShouldShow() )
   {
      // now show the dialog allowing to choose those entries whih the user
      // wants to reenable
      if ( dlg.ShowModal() == wxID_OK )
      {
         const wxArrayInt& selections = dlg.GetSelections();
         size_t count = selections.GetCount();
         if ( count )
         {
            for ( size_t n = 0; n < count; n++ )
            {
               wxString key;
               key << MESSAGE_BOXES_ROOT << '/' << entries[selections[n]];

               // we don't know in which config source this message box was
               // disabled but it doesn't matter: if we want to reenable it, we
               // must do it in all of them anyhow
               for ( AllConfigSources::List::iterator config = sources.begin(),
                                                         end = sources.end();
                     config != end;
                     ++config )
               {
                  config->DeleteEntry(key);
               }
            }

            wxLogStatus(GetFrame(parent),
                        _("Reenabled %d previously disabled message boxes."),
                        count);

            return true;
         }
         else
         {
            wxLogStatus(GetFrame(parent), _("No message boxes were reenabled."));
         }
      }
   }
   else
   {
      MDialog_Message(_("No message boxes have been disabled yet."), parent);
   }

   return false;
}

#ifdef USE_SSL
/// Accept or reject certificate
extern "C"
{
   int AcceptCertificateDialog(const wxChar *subject, const wxChar *issuer,
                               const wxChar *fingerprint)
   {
      wxString info;
      info << _("The server presents the following certificate:\n")
           << '\n'
           << _("Name : ") << subject << '\n'
           << _("Issuer : ") << issuer << '\n'
           << _("Fingerprint : ") << fingerprint << '\n'
           << '\n'
           << _("Do you accept this certificate?");
      return (int) MDialog_YesNoDialog(info,
                                       NULL, _("SSL certificate verification"),
                                       M_DLG_YES_DEFAULT);
   }
}


#endif // USE_SSL



class wxLicenseDialog : public wxManuallyLaidOutDialog
{
public:
   wxLicenseDialog(wxWindow *parent);

private:
   DECLARE_NO_COPY_CLASS(wxLicenseDialog)
};

wxLicenseDialog::wxLicenseDialog(wxWindow *parent)
               : wxManuallyLaidOutDialog(parent,
                                         _("Mahogany Licensing Conditions"))
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("Licensing Conditions"), FALSE,
                                             MH_DIALOG_LICENSE);
   wxHtmlWindow *license = new wxHtmlWindow(this);

   wxBitmap bmp(mApplication->GetIconManager()->GetBitmap(_T("Msplash")));
   int w = bmp.Ok() ? bmp.GetWidth() : 0;
   if ( w < 400 )
      w = 400;

   wxMemoryFSHandler::AddFile(_T("splash.png"), bmp, wxBITMAP_TYPE_PNG);

   license->SetPage("<body text=#000000 bgcolor=#ffffff>"
                    "<center><img src=\"memory:splash.png\"><br>"
                    "</center>"
#include "license.html"
                   );
   wxMemoryFSHandler::RemoveFile(_T("splash.png"));


   wxLayoutConstraints *c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->right.SameAs(box, wxRight, LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   license->SetConstraints(c);

   wxButton *button = (wxButton *) FindWindow(wxID_OK);
   button->SetLabel(_("&Accept"));

   button = (wxButton *) FindWindow(wxID_CANCEL);
   button->SetLabel(_("&Reject"));

   SetAutoLayout(TRUE);
   SetDefaultSize(w + 12*LAYOUT_X_MARGIN, (3*w)/2);
}


extern
bool ShowLicenseDialog(wxWindow *parent)
{
   wxLicenseDialog dlg(parent);

   return dlg.ShowModal() == wxID_OK;
}

// ----------------------------------------------------------------------------
// MDialog_GetSelection
// ----------------------------------------------------------------------------

int MDialog_GetSelection(const wxString& message,
                         const wxString& caption,
                         const wxArrayString& choices,
                         wxWindow *parent)
{
   size_t count = choices.GetCount();
   wxString *aChoices = new wxString[count];
   for ( size_t n = 0; n < count; n++ )
   {
      aChoices[n] = choices[n];
   }

   int rc = wxGetSingleChoiceIndex(message, caption, count, aChoices, parent);

   delete [] aChoices;

   return rc;
}

// ----------------------------------------------------------------------------
// MDialog_GetSelections() stuff
// ----------------------------------------------------------------------------

class wxMultipleChoiceDialog : public wxDialog
{
public:
   wxMultipleChoiceDialog(wxWindow *parent,
                          const wxString& message,
                          const wxString& caption,
                          const wxArrayString& choices,
                          wxArrayInt *selections);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   wxArrayInt *m_selections;

   wxCheckListBox *m_checklstbox;

   DECLARE_NO_COPY_CLASS(wxMultipleChoiceDialog)
};

wxMultipleChoiceDialog::wxMultipleChoiceDialog(wxWindow *parent,
                                               const wxString& message,
                                               const wxString& caption,
                                               const wxArrayString& choices,
                                               wxArrayInt *selections)
                      : wxDialog(parent, -1, caption,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_selections = selections;

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

    topsizer->Add( CreateTextSizer( message ), 0, wxALL, 10 );

    size_t count = choices.GetCount();
    wxASSERT_MSG( count, _T("shouldn't be used without choices") );
    wxString *aChoices = new wxString[count];
    for ( size_t n = 0; n < count; n++ )
    {
       aChoices[n] = choices[n];
    }

    m_checklstbox = new wxCheckListBox(
                                       this,
                                       -1,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       count, aChoices,
                                       wxLB_EXTENDED
                                      );

    delete [] aChoices;

    topsizer->Add(m_checklstbox, 1,
                  wxEXPAND | wxLEFT | wxRIGHT, 3*LAYOUT_X_MARGIN);

#if wxUSE_STATLINE
    topsizer->Add(new wxStaticLine( this, -1 ), 0,
                  wxEXPAND | wxLEFT|wxRIGHT|wxTOP, 2*LAYOUT_X_MARGIN);
#endif

    topsizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0,
                  wxCENTRE | wxALL, 2*LAYOUT_X_MARGIN);

    SetAutoLayout(TRUE);
    SetSizer(topsizer);

    topsizer->SetSizeHints(this);
    topsizer->Fit(this);

    Centre(wxBOTH);
}

bool wxMultipleChoiceDialog::TransferDataToWindow()
{
   size_t count = m_selections->GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      m_checklstbox->Check(m_selections->Item(n));
   }

   m_checklstbox->Select(0);
   m_checklstbox->SetFocus();

   return TRUE;
}

bool wxMultipleChoiceDialog::TransferDataFromWindow()
{
   m_selections->Empty();

   size_t count = m_checklstbox->GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( m_checklstbox->IsChecked(n) )
         m_selections->Add(n);
   }

   return TRUE;
}

size_t MDialog_GetSelections(const wxString& message,
                             const wxString& caption,
                             const wxArrayString& choices,
                             wxArrayInt *selections,
                             wxWindow *parent,
                             const wxString& confpathOrig,
                             const wxSize& sizeDef)
{
   wxCHECK_MSG( selections, 0, _T("selections pointer can't be NULL") );

   if ( choices.IsEmpty() )
   {
      // nothing to choose from
      return 0;
   }

   wxString confpath = confpathOrig;
   if ( confpath.empty() )
   {
      // use default
      confpath = _T("MultiSelect");
   }

   int x, y, w, h;
   if ( !wxMFrame::RestorePosition(confpath, &x, &y, &w, &h) )
   {
      // we didn't find anything in the config, use the provided default size
      if ( sizeDef.x != -1 )
      {
         w = sizeDef.x;
         h = sizeDef.y;
      }
      else // no def size specified, use defaults for default size (sic)
      {
         w = 150;
         h = 250;
      }
   }

   wxMultipleChoiceDialog dlg(parent, message, caption, choices, selections);

   dlg.Move(x, y);
   dlg.SetSize(w, h);

   if ( dlg.ShowModal() != wxID_OK )
   {
      // cancelled, don't save the position and size
      return 0;
   }

   // save the position and size
   wxMFrame::SavePosition(confpath, &dlg);

   return selections->GetCount();
}

// ----------------------------------------------------------------------------
// wxSelectionsOrderDialog
// ----------------------------------------------------------------------------

// control ids for wxSelectionsOrderDialog
enum
{
   Button_Up = 1000,
   Button_Down
};

BEGIN_EVENT_TABLE(wxSelectionsOrderDialog, wxManuallyLaidOutDialog)
   EVT_BUTTON(Button_Up, wxSelectionsOrderDialog::OnButtonUp)
   EVT_BUTTON(Button_Down, wxSelectionsOrderDialog::OnButtonDown)

   EVT_CHECKLISTBOX(-1, wxSelectionsOrderDialog::OnCheckLstBoxToggle)

   EVT_LISTBOX(-1, wxSelectionsOrderDialog::OnCheckLstBoxSelChanged)
END_EVENT_TABLE()

wxSelectionsOrderDialog::wxSelectionsOrderDialog(wxWindow *parent,
                                                 const wxString& message,
                                                 const wxString& caption,
                                                 const wxString& profileKey)
                       : wxManuallyLaidOutDialog(parent, caption, profileKey)
{
   m_hasChanges = false;

   // layout the controls
   // -------------------

   wxLayoutConstraints *c;

   // Ok and Cancel buttons and a static box around everything else
   m_box = CreateStdButtonsAndBox(message);

   // buttons to move items up/down
   m_btnDown = new wxButton(this, Button_Down, _("&Down"));
   c = new wxLayoutConstraints();
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(m_box, wxCentreY, LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_btnDown->SetConstraints(c);

   // FIXME: we also assume that "Down" is longer than "Up" - which may, of
   //        course, be false after translation
   m_btnUp = new wxButton(this, Button_Up, _("&Up"));
   c = new wxLayoutConstraints();
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_box, wxCentreY, LAYOUT_Y_MARGIN);
   c->width.SameAs(m_btnDown, wxWidth);
   c->height.AsIs();
   m_btnUp->SetConstraints(c);

   // a checklistbox with headers on the space which is left
   m_checklstBox = new wxCheckListBox(this, -1);
   c = new wxLayoutConstraints();
   c->left.SameAs(m_box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.LeftOf(m_btnDown, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(m_box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->bottom.SameAs(m_box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_checklstBox->SetConstraints(c);

   UpdateButtons(m_checklstBox->GetSelection());

   // set the minimal window size
   SetDefaultSize(3*wBtn, 7*hBtn);
}

void wxSelectionsOrderDialog::UpdateButtons(int sel)
{
   // only enable buttons if there is something selected and also if pressing
   // them would do something
   m_btnDown->Enable(sel != -1 && (size_t)sel < m_checklstBox->GetCount() - 1);
   m_btnUp->Enable(sel != -1 && sel > 0);
}

void wxSelectionsOrderDialog::OnCheckLstBoxSelChanged(wxCommandEvent& event)
{
   UpdateButtons(event.GetSelection());
}

void wxSelectionsOrderDialog::OnButtonMove(bool up)
{
    int selection = m_checklstBox->GetSelection();
    if ( selection != -1 )
    {
        wxString label = m_checklstBox->GetString(selection);

        int count = m_checklstBox->GetCount();
        int positionNew = up ? selection - 1 : selection + 2;
        if ( positionNew >= 0 && positionNew <= count )
        {
            bool wasChecked = m_checklstBox->IsChecked(selection);

            int positionOld = up ? selection + 1 : selection;

            // insert the item
            m_checklstBox->InsertItems(1, &label, positionNew);

            // and delete the old one
            m_checklstBox->Delete(positionOld);

            int selectionNew = up ? positionNew : positionNew - 1;
            m_checklstBox->Check(selectionNew, wasChecked);
            m_checklstBox->SetSelection(selectionNew);

            // the item could have gone out of the visible part of the listbox
            m_checklstBox->SetFirstItem(selectionNew);

            // notify the derived class about the change
            if ( up )
                positionOld--;
            OnItemSwap(positionOld, selectionNew);

            UpdateButtons(m_checklstBox->GetSelection());

            // something changed, remember it
            m_hasChanges = true;
        }
        //else: out of range, silently ignore
    }
}

// ----------------------------------------------------------------------------
// MDialog_GetSelectionsInOrder()
// ----------------------------------------------------------------------------

bool wxSelectionsOrderDialogSimple::TransferDataToWindow()
{
   size_t count = m_choices->GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      m_checklstBox->Append(m_choices->Item(n));
      if ( m_status->Item(n) )
         m_checklstBox->Check(n);
   }

   return TRUE;
}

bool wxSelectionsOrderDialogSimple::TransferDataFromWindow()
{
   // we're a bit dumb here as we assume that if the user changed something, it
   // should matter (this is not always the case: clicking on the check list
   // box twice doesn't cange anything...) -- we could instead compare the old
   // state with the new one
   if ( m_hasChanges )
   {
      // discard old values
      m_choices->Empty();
      m_status->Empty();

      // go through the check list box and fill the arrays
      size_t count = m_checklstBox->GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         m_choices->Add(m_checklstBox->GetString(n));
         m_status->Add(m_checklstBox->IsChecked(n));
      }
   }

   return TRUE;
}

bool MDialog_GetSelectionsInOrder(const wxString& message,
                                  const wxString& caption,
                                  wxArrayString* choices,
                                  wxArrayInt* status,
                                  const wxString& profileKey,
                                  wxWindow *parent)
{
   wxSelectionsOrderDialogSimple dlg(message, caption,
                                     choices, status,
                                     profileKey, parent);

   return dlg.ShowModal() == wxID_OK && dlg.HasChanges();
}

// ----------------------------------------------------------------------------
// ident combo stuff
// ----------------------------------------------------------------------------

class wxIdentCombo;
WX_DEFINE_ARRAY(wxIdentCombo *, wxIdentComboArray);

class wxIdentCombo : public wxChoice
{
public:
   wxIdentCombo(wxWindow *parent, size_t count, const wxString choices[])
      : wxChoice(parent, IDC_IDENT_COMBO,
                 wxDefaultPosition, wxDefaultSize,
                 count, choices)
   {
      ms_allIdentCombos.Add(this);
   }

   virtual ~wxIdentCombo()
   {
      ms_allIdentCombos.Remove(this);
   }

#if wxCHECK_VERSION(2, 9, 0)
   virtual void DoDeleteOneItem(unsigned int index)
#else
   virtual void Delete(unsigned int index)
#endif
   {
      // sync all other combos with this one
      size_t count = ms_allIdentCombos.GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         if ( ms_allIdentCombos[n] != this )
         {
            ms_allIdentCombos[n]->Delete(index);
         }
      }

#if wxCHECK_VERSION(2, 9, 0)
      wxChoice::DoDeleteOneItem(index);
#else
      wxChoice::Delete(index);
#endif
   }

#if wxCHECK_VERSION(2, 9, 0)
   virtual int DoAppendItems(const wxArrayStringsAdapter& items,
                             void **clientData,
                             wxClientDataType type)
#else
   virtual int DoAppend(const wxString& item)
#endif
   {
      // sync all other comboboxes with the one on which Append() had been
      // called: be careful to avoid reentrancy which would result in the
      // infinite recursion
      if ( ms_indexOfAppend == wxNOT_FOUND )
      {
         ms_indexOfAppend = ms_allIdentCombos.Index(this);

         CHECK( ms_indexOfAppend != wxNOT_FOUND, -1,
                _T("all wxIdentCombos should be in the array!") );

         size_t count = ms_allIdentCombos.GetCount();
         for ( size_t n = 0; n < count; n++ )
         {
            if ( ms_allIdentCombos[n] != this )
            {
               ms_allIdentCombos[n]->
#if wxCHECK_VERSION(2, 9, 0)
               DoAppendItems(items, clientData, type);
#else
               Append(item);
#endif
            }
         }

         ms_indexOfAppend = wxNOT_FOUND;
      }

#if wxCHECK_VERSION(2, 9, 0)
      return wxChoice::DoAppendItems(items, clientData, type);
#else
      return wxChoice::DoAppend(item);
#endif
   }

private:
   // the array of all existing identity comboboxes
   static wxIdentComboArray ms_allIdentCombos;

   // the index of the control on which Append() was called in the array above
   static int ms_indexOfAppend;

   DECLARE_NO_COPY_CLASS(wxIdentCombo)
};

wxIdentComboArray wxIdentCombo::ms_allIdentCombos;
int wxIdentCombo::ms_indexOfAppend = wxNOT_FOUND;

extern wxChoice *CreateIdentCombo(wxWindow *parent)
{
   wxArrayString identities = Profile::GetAllIdentities();
   size_t count = identities.GetCount();
   if ( !count )
      return (wxChoice *)NULL;

   // first one is always the default identity, i.e. no identity at all
   wxString *choices = new wxString[count + 1];
   choices[0] = _("Default");
   for ( size_t n = 0; n < count; n++ )
   {
      choices[n + 1] = identities[n];
   }

   wxChoice *combo = new wxIdentCombo(parent, count + 1, choices);
   delete [] choices;

   wxString identity = READ_APPCONFIG(MP_CURRENT_IDENTITY);
   if ( !identity.empty() )
      combo->SetStringSelection(identity);

#if wxUSE_TOOLTIPS
   combo->SetToolTip(_("Change the identity"));
#endif // wxUSE_TOOLTIPS

   return combo;
}

// ----------------------------------------------------------------------------
// MProgressInfo
// ----------------------------------------------------------------------------

MProgressInfo::MProgressInfo(wxWindow *parent,
                             const String& text,
                             const String& title)
{
   wxString caption = title;
   if ( !caption )
      caption = _("Mahogany: please wait");

   // create the frame which stays either on top of its parent or, if there is
   // no parent, on top of all the windows
   if ( !parent )
      parent = mApplication->TopLevelFrame();

   m_frame = new
#ifdef __WXCOCOA__
                 // FIXME: no wxMiniFrame in wxCocoa yet but wxUSE_MINIFRAME
                 //        is still set to 1
                 wxFrame
#else
                 wxMiniFrame
#endif
                 (
                     parent,
                     -1,
                     caption,
                     wxDefaultPosition,
                     wxDefaultSize,
                     wxDEFAULT_FRAME_STYLE |
                     wxTINY_CAPTION_HORIZ |
                     (parent ? wxFRAME_FLOAT_ON_PARENT : wxSTAY_ON_TOP)
                 );

#ifdef __WXMSW__
   m_frame->SetBackgroundColour(wxSystemSettings::
                                GetColour(wxSYS_COLOUR_BTNFACE));

   m_frame->EnableCloseButton(FALSE);
#endif // __WXMSW__

   wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
   m_labelText = new wxStaticText(m_frame, -1, text);
   sizer->Add(m_labelText, 0, wxALIGN_CENTER_VERTICAL | (wxALL & ~wxRIGHT), 10);

   // hack: use a long label for sizer calculations
   m_labelValue = new wxStaticText(m_frame, -1, _("XXXXXX done"));
   sizer->Add(m_labelValue, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 10);

   m_frame->SetAutoLayout(TRUE);
   m_frame->SetSizer(sizer);
   sizer->Fit(m_frame);
   sizer->SetSizeHints(m_frame);

   // and then remove it
   m_labelValue->SetLabel(wxEmptyString);

   m_frame->CentreOnParent();
   m_frame->Show();

   m_frame->Update();
}

void MProgressInfo::SetLabel(const wxString& label)
{
   m_labelText->SetLabel(label);

   // update the frame
   wxYieldIfNeeded();
}

void MProgressInfo::SetValue(size_t numDone)
{
   m_labelValue->SetLabel(wxString::Format(_("%u done"), numDone));

   // update the frame
   wxYieldIfNeeded();
}

MProgressInfo::~MProgressInfo()
{
   ReallyCloseTopLevelWindow(m_frame);
}

// ----------------------------------------------------------------------------
// MText2Dialog: dialog with 2 text entries
// ----------------------------------------------------------------------------

class MText2Dialog : public wxManuallyLaidOutDialog
{
public:
   MText2Dialog(wxWindow *parent,
                const wxString& perspath,
                const wxString& message,
                const wxString& caption,
                const wxString& labelBox,
                const wxString& prompt1,
                String *value1,
                const wxString& prompt2,
                String *value2,
                long textStyle = 0);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   wxTextCtrl *m_text1,
              *m_text2;

   wxString *m_value1,
            *m_value2;

   DECLARE_NO_COPY_CLASS(MText2Dialog)
};

MText2Dialog::MText2Dialog(wxWindow *parent,
                           const wxString& perspath,
                           const wxString& message,
                           const wxString& caption,
                           const wxString& labelBox,
                           const wxString& prompt1,
                           String *value1,
                           const wxString& prompt2,
                           String *value2,
                           long textStyle)
               : wxManuallyLaidOutDialog(parent,
                                         caption,
                                         perspath)
{
   // init members
   m_value1 = value1;
   m_value2 = value2;

   // create controls
   wxLayoutConstraints *c;
   wxStaticText *label;

   wxStaticBox *box = CreateStdButtonsAndBox(labelBox);

   label = new wxStaticText(this, -1, message);
   c = new wxLayoutConstraints;
   c->width.AsIs();
   c->height.AsIs();
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, (labelBox.empty() ? 2 : 5)*LAYOUT_Y_MARGIN);
   label->SetConstraints(c);

   wxArrayString labels;
   labels.Add(prompt1);
   labels.Add(prompt2);

   long widthMax = GetMaxLabelWidth(labels, this);

   c = new wxLayoutConstraints;
   c->width.Absolute(widthMax);
   c->height.AsIs();
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(label, 3*LAYOUT_Y_MARGIN);
   label = new wxStaticText(this, -1, labels[0]);
   label->SetConstraints(c);

   m_text1 = new wxTextCtrl(this, -1, wxEmptyString);
   c = new wxLayoutConstraints;
   c->left.RightOf(label, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(label, wxCentreY);
   c->height.AsIs();
   m_text1->SetConstraints(c);

   label = new wxStaticText(this, -1, labels[1]);
   c = new wxLayoutConstraints;
   c->width.Absolute(widthMax);
   c->height.AsIs();
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_text1, 2*LAYOUT_Y_MARGIN);
   label->SetConstraints(c);

   m_text2 = new wxTextCtrl(this, -1, wxEmptyString,
                            wxDefaultPosition, wxDefaultSize,
                            textStyle);
   c = new wxLayoutConstraints;
   c->left.RightOf(label, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(label, wxCentreY);
   c->height.AsIs();
   m_text2->SetConstraints(c);

   SetDefaultSize(5*wBtn, 10*hBtn);
}

bool MText2Dialog::TransferDataToWindow()
{
   m_text1->SetValue(*m_value1);
   m_text2->SetValue(*m_value2);

   return TRUE;
}

bool MText2Dialog::TransferDataFromWindow()
{
   *m_value1 = m_text1->GetValue();
   *m_value2 = m_text2->GetValue();

   return TRUE;
}

bool MDialog_GetText2FromUser(const wxString& message,
                              const wxString& caption,
                              const wxString& prompt1,
                              String *value1,
                              const wxString& prompt2,
                              String *value2,
                              wxWindow *parent)
{
   MText2Dialog dlg(parent, _T("Text2Dialog"), message, caption, wxEmptyString,
                    prompt1, value1, prompt2, value2);

   return dlg.ShowModal() == wxID_OK;
}

// ----------------------------------------------------------------------------
// MPasswordDialog &c
// ----------------------------------------------------------------------------

class MPasswordDialog : public MText2Dialog
{
public:
   MPasswordDialog(wxWindow *parent,
                   const wxString& message,
                   const wxString& label,
                   wxString *username,
                   wxString *password)
      : MText2Dialog(parent,
                     _T("PasswordDialog"),
                     message,
                     _("Password required"), // caption
                     label,
                     _("&Username: "), username,
                     _("&Password: "), password,
                     wxTE_PASSWORD
                    )
   {
   }

   virtual bool TransferDataToWindow();

protected:
   void OnUpdateOk(wxUpdateUIEvent& event);

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(MPasswordDialog)
};

BEGIN_EVENT_TABLE(MPasswordDialog, wxMDialog)
   EVT_UPDATE_UI(wxID_OK, MPasswordDialog::OnUpdateOk)
END_EVENT_TABLE()

bool MPasswordDialog::TransferDataToWindow()
{
   if ( !MText2Dialog::TransferDataToWindow() )
      return FALSE;

   m_text2->SetFocus();

   return TRUE;
}

void MPasswordDialog::OnUpdateOk(wxUpdateUIEvent& event)
{
   event.Enable( !(m_text1->GetValue().empty() &&
                   m_text2->GetValue().empty()) );
}

// ----------------------------------------------------------------------------
// MFolderPasswordDialog
// ----------------------------------------------------------------------------

class MFolderPasswordDialog : public MPasswordDialog
{
public:
   MFolderPasswordDialog(wxWindow *parent,
                         const wxString& folderName,
                         wxString *username,
                         wxString *password)
      : MPasswordDialog(
                        parent,
                        _("Please enter login/password to access this folder"),
                        wxString::Format(_("Folder '%s':"), folderName.c_str()),
                        username,
                        password
                       )
   {
   }

private:
   DECLARE_NO_COPY_CLASS(MFolderPasswordDialog)
};

bool MDialog_GetPassword(const wxString& folderName,
                         wxString *username,
                         wxString *password,
                         wxWindow *parent)
{
   MFolderPasswordDialog dlg(parent, folderName, username, password);

   return dlg.ShowModal() == wxID_OK;
}

// ----------------------------------------------------------------------------
// MSendPasswordDialog
// ----------------------------------------------------------------------------

class MSendPasswordDialog : public MPasswordDialog
{
public:
   MSendPasswordDialog(wxWindow *parent,
                       const wxString& server,
                       Protocol protocol,
                       wxString *username,
                       wxString *password)
      : MPasswordDialog(
                        parent,
                        wxString::Format(
                           _("Please enter login/password to %s this message"),
                           protocol == Prot_SMTP ? _("send") : _("post")
                        ),
                        wxString::Format(_("Server '%s':"), server.c_str()),
                        username,
                        password
                       )
   {
   }

private:
   DECLARE_NO_COPY_CLASS(MSendPasswordDialog)
};

bool MDialog_GetPassword(Protocol protocol,
                         const wxString& server,
                         wxString *password,
                         wxString *username,
                         wxWindow *parent)
{
   MSendPasswordDialog dlg(parent, server, protocol, username, password);

   return dlg.ShowModal() == wxID_OK;
}

extern wxWindow *GetDialogParent(const wxWindow *parent)
{
  return parent == NULL ? mApplication->TopLevelFrame()
                        : GetFrame(parent);
}

