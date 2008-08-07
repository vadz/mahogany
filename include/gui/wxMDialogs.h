/*******************************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998-1999 by Karsten Ballüder (ballueder@bmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef WXMDIALOGS_H
#define WXMDIALOGS_H

#ifndef     USE_PCH
#   include "FolderType.h"
#   include "MApplication.h"

#   include <wx/bmpbuttn.h>
#endif // USE_PCH

#include "MDialogs.h"

#include <wx/progdlg.h>

// fwd decl
class Profile;
class MFolder;
class MPersMsgBox;

class WXDLLIMPEXP_FWD_CORE wxFrame;
class WXDLLIMPEXP_FWD_BASE wxStaticText;

/**
  Flags for MDialog_YesNoDialog()

  NB: M_DLG_DISABLE and M_DLG_NOT_ON_NO must be equal to the corresponding
      wxPMSBOX_XXX flags
*/
enum
{
   M_DLG_YES_DEFAULT = 0,        // default
   M_DLG_NO_DEFAULT  = 0x1000,
   M_DLG_DISABLE     = 0x2000,   // pre-check the "Don't ask again" checkbox
   M_DLG_NOT_ON_NO   = 0x4000,   // don't allow disabling on "No"
   M_DLG_ALLOW_CANCEL= 0x8000    // add "Cancel" button to MDialog_Message
};

/**
  Return value of MDialog_YesNoCancel
 */
enum MDlgResult
{
   MDlg_Cancel = -1,
   MDlg_No,
   MDlg_Yes
};

/**
   Dialog Boxes
*/


/** Progress dialog which shows a moving progress bar. */
class MProgressDialog : public wxProgressDialog
{
public:
   /** Creates and displays dialog, disables event handling for other
       frames or parent frame to avoid recursion problems.
       @param title title for window
       @param message message to display in window
       @param maximum maximum value for status bar, if <= 0, no bar is shown
       @param parent window or NULL
       @param flags combination of wxPD_XXX (default ones are always used)
   */
   MProgressDialog(wxString const &title, wxString const &message,
                   int maximum = 100,
                   wxWindow *parent = NULL,
                   int flags = wxPD_APP_MODAL | wxPD_CAN_ABORT)
      : wxProgressDialog
        (
            wxString(_T("Mahogany : ")) + title,
            message,
            maximum,
            parent,
            flags |
            wxPD_AUTO_HIDE |
            wxPD_ESTIMATED_TIME |
            wxPD_ELAPSED_TIME |
            wxPD_REMAINING_TIME
        )
   {
   }

private:
   // forbid background processing during the life time of the progress dialog
   // object: this can lead to nasty reentrancies if, e.g. a timer fires while
   // we're retrieving headers for the folder and results in a reentrant call
   // to c-client
   MAppCriticalSection m_appCS;

   DECLARE_NO_COPY_CLASS(MProgressDialog)
};

/// use this instead of MProgressDialog when maximum is unknown
class MProgressInfo
{
public:
   MProgressInfo(wxWindow *parent,
                 const String& label,
                 const String& title = wxEmptyString);
  ~MProgressInfo();

   /// change the label completely
   void SetLabel(const wxString& label);

   /// change just the number shown
   void SetValue(size_t numDone);

   /// get our frame
   wxFrame *GetFrame() const { return m_frame; }

private:
   wxFrame *m_frame;
   wxStaticText *m_labelText,
                *m_labelValue;
};

/// This allows us to call them from modules.
extern "C"
{

/** display error message:
       @param message the text to display
       @param parent   the parent frame
       @param title   title for message box window
       @param modal   true to make messagebox modal
   */
void   MDialog_ErrorMessage(const wxString& message,
                            const wxWindow *parent = NULL,
                            const wxString& title = MDIALOG_ERRTITLE,
                            bool modal = false);

/** display system error message:
       @param message the text to display
       @param parent   the parent frame
       @param title   title for message box window
       @param modal   true to make messagebox modal
   */
void   MDialog_SystemErrorMessage(const wxString& message,
                                  const wxWindow *parent = NULL,
                                  const wxString& title = MDIALOG_SYSERRTITLE,
                                  bool modal = false);

/** display error message and exit application
       @param message the text to display
       @param title   title for message box window
       @param parent   the parent frame
   */
void   MDialog_FatalErrorMessage(const wxString& message,
                                 const wxWindow *parent = NULL,
                                 const wxString& title = MDIALOG_FATALERRTITLE);

/** display normal message and, if configPath != NULL, give a user a checkbox
    "don't show this message again" which allows not to show the same message
    again.
       @param message the text to display
       @param parent   the parent frame
       @param title   title for message box window
       @param configPath the profile path to use (doesn't use profile if NULL)

       @return TRUE if Ok was pressed, FALSE if Cancel
   */
bool   MDialog_Message(const wxString& message,
                       const wxWindow *parent = NULL,
                       const wxString& title = MDIALOG_MSGTITLE,
                       const char *configPath = NULL,
                       int flags = 0);

/** profile-aware Yes/No dialog: if persMsg is not NULL, it has a "don't show
    this message again" checkbox and remembers in the profile if it had been
    checked to avoid showing this dialog the next time.

    @param message the text to display
    @param parent the parent frame
    @param title title for message box window
    @param flags combination of M_DLG_XXX values
    @param persMsg the profile path to use (doesn't use profile if NULL)
    @param folderName the name of the folder, global setting if NULL

    @return true if yes was selected
*/
bool   MDialog_YesNoDialog(const wxString& message,
                           const wxWindow *parent = NULL,
                           const wxString& title = MDIALOG_YESNOTITLE,
                           int flags = M_DLG_YES_DEFAULT,
                           const MPersMsgBox *persMsg = NULL,
                           const wxChar *folderName = NULL);

/**
  This is a 3 choice dialog: it has Yes, No and Cancel buttons. Unlile
  MDialog_YesNoDialog it is not persistent as remembering "Cancel" as answer
  would probably be very confusing.

  @param message the text to display
  @param parent the parent frame
  @param title title for message box window
  @param flags combination of M_DLG_XXX values

  @return M_DLG_ if yes was chosen, 0 if no and -1 if cancel
 */
MDlgResult MDialog_YesNoCancel(const wxString& message,
                               const wxWindow *parent = NULL,
                               const wxString& title = MDIALOG_YESNOTITLE,
                               int flags = M_DLG_YES_DEFAULT,
                               const MPersMsgBox *persMsg = NULL);

/** show a (modal) dialog with the given text

    @param title is the title of the dialog
    @param text is the text to show in it
    @param configPath is the path in the config used to save dialog position
           and size and may be NULL
*/
void MDialog_ShowText(wxWindow *parent,
                      const wxString& title,
                      const wxString& text,
                      const char *configPath = NULL);

} // extern "C"

/**
  Another, new, version of MDialog_Message which uses MPersMsgBox and has a
  more useful parameter order.

  @param message the text to display
  @param parent the parent frame
  @param persMsg the profile path to use (doesn't use profile if NULL)
  @param title title for message box window

  @return TRUE if Ok was pressed, FALSE if Cancel
 */
bool MDialog_Message(const wxString& message,
                     const wxWindow *parent,
                     const MPersMsgBox *persMsg,
                     int flags = 0,
                     const wxString& title = MDIALOG_MSGTITLE);

/** File requester dialog: asks user for a file name
       @param message the text to display
       @param parent   the parent frame
       @param path   default path
       @param filename   default filename
       @param extension   default extension
       @param wildcard   pattern matching expression
       @param save   if true, saving a file
       @param profile   the profile to use
       @return string with the filename or empty if cancelled
   */
String MDialog_FileRequester(const String &message,
                             wxWindow *parent = NULL,
                             String path = NULLstring,
                             String filename = NULLstring,
                             String extension = NULLstring,
                             String wildcard = NULLstring,
                             bool save = false,
                             Profile *profile = NULL);

/** Ask user for a directory

  @param message the explanatory message
  @param path the initially selected directory (if not empty)
  @param parent the parent frame
  @param configPath the profile path to use (may be NULL)
  @return the path of the selected directory or an empty string
 */
String MDialog_DirRequester(const String& message,
                            const String& path = NULLstring,
                            wxWindow *parent = NULL,
                            const char *configPath = NULL);

/**
  Ask the user to enter some text and remember the last value in the "Prompt"
  section of the global config object in the key named "key" if it's !NULL:
  it may be NULL in which case this function won't use wxConfig.

  If FALSE is returned (meaning the user chose <Cancel>), the string pstr is
  not modified, otherwise it receives the value entered (the old contents is
  the default value shown in the dialog unless it's NULL and key is not: then
  the default value is read from the config using the "def" argument for the
  default value)

  If parent parameter is NULL, the top level application frame is used.

  @param pstr is the string which will receive the entered string
  @param caption is the title of the dialog
  @param prompt is the message displayed in the dialog itself
  @param parent is the parent window for the modal dialog box
  @param key contains the name of config entry to use if !NULL
  @param def contains the default value (only if pstr->IsEmpty())
  @param passwordflag if true, hide the input

  @return FALSE if cancelled, TRUE otherwise
*/
bool MInputBox(wxString *pstr,
               const wxString& caption,
               const wxString& prompt,
               const wxWindow *parent = NULL,
               const char *key = NULL,
               const wxString& def = wxEmptyString,
               bool passwordflag = false);

/**
  Gets a number in the given range from user. Both message andprompt may be
  empty (but probably not simultaneously unless you only work with really
  clever users)

  @param message a possibly multiline explanation message
  @param prompt a singleline prompt shown just before the number entry zone
  @param caption the dialog title
  @param value initial value
  @param min the minimum possible value
  @param max the maximum possible value
 */
extern long
MGetNumberFromUser(const wxString& message,
                   const wxString& prompt,
                   const wxString& caption,
                   long value = 0,
                   long min = 0,
                   long max = 100,
                   wxWindow *parent = (wxWindow *)NULL,
                   const wxPoint& pos = wxDefaultPosition);

/// simple AboutDialog to be displayed at startup
void
MDialog_AboutDialog( const wxWindow *parent, bool bCloseOnTimeout = true);

/// a tip dialog which will show some (random) tip
void
MDialog_ShowTip(const wxWindow *parent);

// the global pointer to the splash screen (NULL if there is no flash screen)
extern class wxFrame *g_pSplashScreen;

/// edit an existing folder profile (wrapper around ShowFolderPropertiesDialog)
void
MDialog_FolderProfile(const wxWindow *parent, const String& folderName);

/// the flags for MDialog_FolderChoose
enum
{
   /// propose to choose file to save to
   MDlg_Folder_Save = 0,

   /// propose to choose file to open
   MDlg_Folder_Open = 1,

   /// don't propose choosing a file at all
   MDlg_Folder_NoFiles = 2
};

/**
  Choose a folder from the list of all folders (also allows to choose a file to
  use as a folder), returns NULL if cancelled

  @param parent the parent window for the dialog
  @param folder the default folder to use
  @param flags the dialog option flags, combination of MDlg_Folder_XXX values
  @return the folder selected by user (must be DecRef()'d) or NULL if cancelled
 */
MFolder *MDialog_FolderChoose(const wxWindow *parent,
                              MFolder *folder = NULL,
                              int flags = MDlg_Folder_Save);

/// choose a folder and open a view on it
void MDialog_FolderOpen(const wxWindow *parent);

/// Configure modules, in wxModulesDialog.cpp:
extern
void ShowModulesDialog(wxFrame *parent);

extern
bool ConfigureDateFormat(Profile *profile, wxWindow *parent);
/** Pick an icon file: for XFace */
extern
bool PickXFaceDialog(Profile *profile, wxWindow *parent);
/// set new global passwd
extern bool PickGlobalPasswdDialog(Profile *profile, wxWindow *parent);

/** Asks the user if he wants to expunge the deleted messages. */
extern
void CheckExpungeDialog(class ASMailFolder *mf, wxWindow *parent = NULL);

/** Read a filter program from the data.
 This takes the sub-groups under "Filters/" and assembles a filter
 program from them. */
extern
String GetFilterProgram(Profile *profile);

/// Shows the dialog allowing to reenable disabled wxPMesageBox()es
extern
bool ReenablePersistentMessageBoxes(wxWindow *parent = NULL);

/// Returns true if the license was accepted:
extern bool ShowLicenseDialog(wxWindow *parent = NULL);

/// Run a wizard for folder creation:
extern
class MFolder *
RunCreateFolderWizard(bool *cancelled, MFolder *parent, wxWindow *parWin);

/// Run the wizard for folder import
extern void RunImportFoldersWizard(void);

#ifdef USE_SSL
/// Accept or reject certificate
extern "C"
{
   int AcceptCertificateDialog(const wxString& subject,
                               const wxString& issuer,
                               const wxString& fingerprint);
}
#endif

/**
  Propose the user to choose one of the strings in the choices array, returns
  the index of the string chosen or -1 if the dialog was cancelled.

  @param choices is the array of strings to choose from
  @param parent is the parent window
  @return the index of the selected item or -1
*/
int MDialog_GetSelection(const wxString& message,
                         const wxString& caption,
                         const wxArrayString& choices,
                         wxWindow *parent = NULL);

/**
  Propose the user to choose one or several of the strings in the choices
  array, returns the number of strings chosen.

  @param choices is the array of strings to choose from
  @param selections is the array filled by function with selections
  @param parent is the parent window
  @param confpath the path in config for storing the window position
  @param sizeDef the default size for this dialog
  @return the number of selected items
*/
size_t MDialog_GetSelections(const wxString& message,
                             const wxString& caption,
                             const wxArrayString& choices,
                             wxArrayInt *selections,
                             wxWindow *parent = NULL,
                             const wxString& confpath = wxEmptyString,
                             const wxSize& sizeDef = wxDefaultSize);

/**
  Show a dialog allowing the user to show a subset of a set of strings and the
  order of them. The choices array contains all strings and status array
  contains their on/off status. If the function returns TRUE, the arrays will
  contain the new configuration as selected by user.

  @param message a single line explanation message
  @param caption is the dialog title
  @param choices
  @param status
  @param profileKey is the key to be used to stor dialog position &c
  @param parent is the parent window
  @return TRUE if Ok
*/
bool MDialog_GetSelectionsInOrder(const wxString& message,
                                  const wxString& caption,
                                  wxArrayString* choices,
                                  wxArrayInt* status,
                                  const wxString& profileKey,
                                  wxWindow *parent = NULL);

/**
  Show a dialog asking for two text values
*/

bool MDialog_GetText2FromUser(const wxString& message,
                              const wxString& caption,
                              const wxString& prompt1,
                              String *value1,
                              const wxString& prompt2,
                              String *value2,
                              wxWindow *parent = NULL);

/**
  Show a dialog asking the user for password and also, optionally, the
  username for the folder

  @return TRUE if Ok was pressed
*/
bool MDialog_GetPassword(const wxString& folderName,
                         wxString *password,
                         wxString *username,
                         wxWindow *parent = NULL);

/**
  Show a dialog asking the user for the password and username needed to
  send an SMTP/NNTP message

  @return TRUE if Ok was pressed
*/
bool MDialog_GetPassword(Protocol protocol,
                         const wxString& server,
                         wxString *password,
                         wxString *username,
                         wxWindow *parent = NULL);

/**
  The base class for all Mahogany dialogs.

  It takes care of hiding the splash screen when it is shown and implements
  Karsten's unique semi-modality concept ;-) under !Windows.
*/
class wxMDialog : public wxDialog
{
public:
   wxMDialog(wxWindow *parent, wxWindowID id,
             const wxString &title,
             const wxPoint &pos = wxDefaultPosition,
             const wxSize &size = wxDefaultSize,
             long style = wxDEFAULT_DIALOG_STYLE,
             const wxString &name = wxDialogNameStr)
      : wxDialog ( parent, id, title, pos, size, style, name)
      {
      }

   wxMDialog()
   {
   }

   virtual int ShowModal();
   virtual void EndModal(int rc);

   DECLARE_NO_COPY_CLASS(wxMDialog)
};

class wxXFaceButton : public wxBitmapButton
{
public:
   wxXFaceButton(wxWindow *parent, int id,  wxString filename);
   void SetFile(const wxString &filename);
   wxString GetFile(void) const { return m_XFace; }
private:
   wxString m_XFace;
   wxWindow *m_Parent;

   DECLARE_NO_COPY_CLASS(wxXFaceButton)
};

/// returns the argument if it's !NULL of the top-level application frame
extern wxWindow *GetDialogParent(const wxWindow *parent);

#endif  //WXMDIALOGS_H
