/*-*- c++ -*-********************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$             *
 *******************************************************************/

#ifndef WXMDIALOGS_H
#define WXMDIALOGS_H

#ifdef __GNUG__
#pragma interface "wxMDialogs.h"
#endif

#ifndef     USE_PCH
#   include	<MApplication.h>
#   include	<MFrame.h>
#   include	<MDialogs.h>
#   include	<Adb.h>
#else
  // fwd decl
  class ProfileBase;
  class AdbEntry;

  // don't fwd decl typedef if it's already defined
#   ifndef   ADB_H
    class AdbExpandListType;
#   endif
#endif

class wxString;
class wxWindow;

/**
   Dialog Boxes 
*/

/** display error message:
       @param message the text to display
       @param parent	the parent frame
       @param title	title for message box window
       @param modal	true to make messagebox modal
   */
void	MDialog_ErrorMessage(char const *message,
		     MWindow *parent = NULL,
			     char const *title = MDIALOG_ERRTITLE,
		     bool modal = false);

/** display system error message:
       @param message the text to display
       @param parent	the parent frame
       @param title	title for message box window
       @param modal	true to make messagebox modal
   */
void	MDialog_SystemErrorMessage(char const *message,
			   MWindow *parent = NULL,
			   char const *title = MDIALOG_SYSERRTITLE,
			   bool modal = false);
   
/** display error message and exit application
       @param message the text to display
       @param title	title for message box window
       @param parent	the parent frame
   */
void	MDialog_FatalErrorMessage(char const *message,
			  MWindow *parent = NULL,
			  char const *title = MDIALOG_FATALERRTITLE
   );
   
/** display normal message:
       @param message the text to display
       @param parent	the parent frame
       @param title	title for message box window
       @param modal	true to make messagebox modal
   */
void	MDialog_Message(char const *message,
		MWindow *parent = NULL,
		char const *title = MDIALOG_MSGTITLE,
		bool modal = false);

/** simple Yes/No dialog
       @param message the text to display
       @param parent	the parent frame
       @param title	title for message box window
       @param modal	true to make messagebox modal
       @param YesDefault true if Yes button is default, false for No as default
       @return true if Yes was selected
   */
bool	MDialog_YesNoDialog(char const *message,
		    MWindow *parent = NULL,
		    bool modal = false,
		    char const *title = MDIALOG_YESNOTITLE,
		    bool YesDefault = true);

/** Filerequester
       @param message the text to display
       @param parent	the parent frame
       @param path	default path
       @param filename	default filename
       @param extension	default extension
       @param wildcard	pattern matching expression
       @param save	if true, saving a file
       @param profile	the profile to use
       @return pointer to a temporarily allocated buffer with he filename, or NULL
   */
#if 0
const char *MDialog_FileRequester(char const *message,
				      MWindow *parent = NULL,
				      char const *path = NULL,
				      char const *filename = NULL,
				      char const *extension = NULL,
				      char const *wildcard = NULL,
				      bool save = false,
				      ProfileBase *profile = NULL
   );
#endif

const char *MDialog_FileRequester(String const &message,
                                  MWindow *parent = NULL,
                                  String path = NULLstring,
                                  String filename = NULLstring,
                                  String extension = NULLstring,
                                  String wildcard = NULLstring,
                                  bool save = false,
                                  ProfileBase *profile = NULL
   );

/** simple Yes/No dialog
    @param message the text to display
    @param parent the parent frame (NULL => main app window)
    @param modal  true to make messagebox modal
    @param YesDefault true if Yes button is default, false for No as default
    @return true if Yes was selected
  */
bool MDialog_YesNoDialog(String const &message,
                         MWindow *parent = NULL,
                         bool modal = false,
                         bool YesDefault = true);


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

  @return FALSE if cancelled, TRUE otherwise
*/
bool MInputBox(wxString *pstr,
               const wxString& caption,
               const wxString& prompt,
               wxWindow *parent = NULL,
               const char *key = NULL,
               const char *def = NULL);

AdbEntry *
MDialog_AdbLookupList(AdbExpandListType *adblist,
		      MWindow *parent = NULL);

/// simple AboutDialog to be displayed at startup
void
MDialog_AboutDialog( MWindow *parent);

// @@@ the coeffecients are purely empirical...
#define TEXT_HEIGHT_FROM_LABEL(h)   (23*(h)/13)
#define BUTTON_WIDTH_FROM_HEIGHT(w) (75*(w)/23)

#endif  //WXMDIALOGS_H
