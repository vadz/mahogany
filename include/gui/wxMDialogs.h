/*-*- c++ -*-********************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998-1999 by Karsten Ballüder (ballueder@bmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef WXMDIALOGS_H
#define WXMDIALOGS_H

#ifdef __GNUG__
#pragma interface "wxMDialogs.h"
#endif

#ifndef     USE_PCH
#   include "Mcommon.h"
#   include "Mdefaults.h"
#   include "guidef.h"
#   include "MFrame.h"
#   include "gui/wxIconManager.h"
#   include "MApplication.h"
#endif

#include <wx/icon.h>
#include <wx/frame.h>
#include <wx/progdlg.h>
#include <wx/dialog.h>
#include <wx/bmpbuttn.h>

// fwd decl
class ProfileBase;
class ArrayAdbElements;
class wxString;
class MFolder;
class wxButtonEvent;
class wxCloseEvent;

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
       @param disableParentOnly if true, only disable parent window's
       event handling
       @param abortButton if true, dialog will show an abort button
   */
   MProgressDialog(wxString const &title, wxString const &message,
                   int maximum = 100,
                   wxWindow *parent = NULL,
                   bool disableParentOnly = false,
                   bool abortButton = false)
   : wxProgressDialog(wxString("Mahogany : ") + title, message,
                      maximum, parent,
                      (disableParentOnly ? 0 : wxPD_APP_MODAL) |
                      (abortButton ? wxPD_CAN_ABORT : 0) |
                      wxPD_AUTO_HIDE
#ifdef wxPD_ESTIMATED_TIME
//FIXME WXCOMPATIBILITY
                      |wxPD_ESTIMATED_TIME|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME
#endif
      )
   {
   }
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
void   MDialog_ErrorMessage(char const *message,
                            const MWindow *parent = NULL,
                            char const *title = MDIALOG_ERRTITLE,
                            bool modal = false);

/** display system error message:
       @param message the text to display
       @param parent   the parent frame
       @param title   title for message box window
       @param modal   true to make messagebox modal
   */
void   MDialog_SystemErrorMessage(char const *message,
                                  const MWindow *parent = NULL,
                                  char const *title = MDIALOG_SYSERRTITLE,
                                  bool modal = false);

/** display error message and exit application
       @param message the text to display
       @param title   title for message box window
       @param parent   the parent frame
   */
void   MDialog_FatalErrorMessage(char const *message,
                                 const MWindow *parent = NULL,
                                 char const *title = MDIALOG_FATALERRTITLE);

/** display normal message and, if configPath != NULL, give a user a checkbox
    "don't show this message again" which allows not to show the same message
    again.
       @param message the text to display
       @param parent   the parent frame
       @param title   title for message box window
       @param configPath the profile path to use (doesn't use profile if NULL)
   */
void   MDialog_Message(char const *message,
                       const MWindow *parent = NULL,
                       char const *title = MDIALOG_MSGTITLE,
                       const char *configPath = NULL);

/** profile-aware Yes/No dialog: has a "don't show this message again" checkbox
    and remembers in the profile if it had been checked to avoid showing this
    dialog the next time.
       @param message the text to display
       @param parent   the parent frame
       @param title   title for message box window
       @param YesDefault true if Yes button is default, false for No as default
       @param configPath the profile path to use (doesn't use profile if NULL)

       @return true if Yes was selected
   */
bool   MDialog_YesNoDialog(char const *message,
                           const MWindow *parent = NULL,
                           char const *title = MDIALOG_YESNOTITLE,
                           bool YesDefault = true,
                           const char *configPath = NULL);

/** Filerequester
       @param message the text to display
       @param parent   the parent frame
       @param path   default path
       @param filename   default filename
       @param extension   default extension
       @param wildcard   pattern matching expression
       @param save   if true, saving a file
       @param profile   the profile to use
       @return pointer to a temporarily allocated buffer with he filename, or NULL
   */
const char *MDialog_FileRequester(String const &message,
                                  const MWindow *parent = NULL,
                                  String path = NULLstring,
                                  String filename = NULLstring,
                                  String extension = NULLstring,
                                  String wildcard = NULLstring,
                                  bool save = false,
                                  ProfileBase *profile = NULL
   );

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
               const MWindow *parent = NULL,
               const char *key = NULL,
               const char *def = NULL,
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

/**
 Let the user choose an element from aEntries. If there is exactly one element
 in the array (and we shouldn't be called with 0), 0 is always returned and
 user is not asked at all. If the dialog is shown, the user may cancel it and
 in this case -1 is returned. Otherwise, it's the index of selected item.
*/
int MDialog_AdbLookupList(ArrayAdbElements& aEntries,
                          const MWindow *parent = NULL);

/// simple AboutDialog to be displayed at startup
void
MDialog_AboutDialog( const MWindow *parent, bool bCloseOnTimeout = true);

/// a tip dialog which will show some (random) tip
void
MDialog_ShowTip(const MWindow *parent);

// the global pointer to the splash screen (NULL if there is no flash screen)
extern class MFrame *g_pSplashScreen;

/// function which will close the splash screen if it's (still) opened
extern void CloseSplash();

/// edit an existing folder profile (wrapper around ShowFolderPropertiesDialog)
void
MDialog_FolderProfile(const MWindow *parent, const String& folderName);

/// choose a folder from the list of all folders, returns NULL if cancelled
// TODO store the last folder in config
MFolder *
MDialog_FolderChoose(const MWindow *parent);

/// test
void MDialog_FolderOpen(class wxMFrame *parent);

/** show a (modal) dialog with the given text

    @param title is the title of the dialog
    @param text is the text to show in it
    @param configPath is the path in the config used to save dialog position
           and size and may be NULL
*/
void MDialog_ShowText(MWindow *parent,
                      const char *title,
                      const char *text,
                      const char *configPath = NULL);

} // extern "C"

/// Configure modules, in wxModulesDialog.cpp:
extern
void ShowModulesDialog(wxFrame *parent);
/// Configure message/folder sorting
extern
bool ConfigureSorting(ProfileBase *profile, wxWindow *parent);
/* Configuration dialog for searching for messages. */
extern
bool
ConfigureSearchMessages(class SearchCriterium *crit,
                        ProfileBase *profile,
                        wxWindow *parent);

extern
bool ConfigureDateFormat(ProfileBase *profile, wxWindow *parent);
/** Pick an icon file: for XFace */
extern
bool PickXFaceDialog(ProfileBase *profile, wxWindow *parent);

/** Asks the user if he wants to expunge the deleted messages. */
extern
void CheckExpungeDialog(class ASMailFolder *mf, wxWindow *parent = NULL);

/** Set up filters rules. In wxMFilterDialog.cpp */
extern
bool ConfigureFilterRules(ProfileBase *profile, wxWindow *parent);
/** Read a filter program from the data.
 This takes the sub-groups under "Filters/" and assembles a filter
 program from them. */
extern
String GetFilterProgram(ProfileBase *profile);

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
   int AcceptCertificateDialog(const char *subject, const char *issuer,
                               const char *fingerprint);
}
#endif

                            
#ifdef OS_WIN
#  undef USE_SEMIMODAL

#  define wxSMDialog wxDialog
#else // !Win
#  define USE_SEMIMODAL

/** Semi-modal dialog to allow pop-up help to work.
    (Yes, I know it's a funny name, but so is the whole Help/Modal
    problem... :-)

    Behaves identical to wxDialog, but ShowModal() is only a
    fake-modal mode to allow one to browse the help window.
*/
class wxSMDialog : public wxDialog
{
public:
   wxSMDialog( wxWindow *parent, wxWindowID id,
               const wxString &title,
               const wxPoint &pos = wxDefaultPosition,
               const wxSize &size = wxDefaultSize,
               long style = wxDEFAULT_DIALOG_STYLE,
               const wxString &name = wxDialogNameStr )
      : wxDialog ( parent, id, title, pos, size, style, name)
      {
      }

   wxSMDialog() : wxDialog() {}
   virtual int ShowModal();
   virtual void EndModal(int rc);
};
#endif // Win/!Win


class wxXFaceButton : public wxBitmapButton
{
public:
   wxXFaceButton(wxWindow *parent, int id,  wxString filename)
      {
         m_Parent = parent;
         wxBitmap bmp = mApplication->GetIconManager()->GetBitmap("noxface");
         wxBitmapButton::Create(parent,id, bmp, wxDefaultPosition,
                                wxSize(64,64));
         SetFile(filename);
      }
   void SetFile(const wxString &filename);
   wxString GetFile(void) const { return m_XFace; }
private:
   wxString m_XFace;
   wxWindow *m_Parent;
};

#endif  //WXMDIALOGS_H
