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
const char *MDialog_FileRequester(char const *message,
				      MWindow *parent = NULL,
				      char const *path = NULL,
				      char const *filename = NULL,
				      char const *extension = NULL,
				      char const *wildcard = NULL,
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


AdbEntry *
MDialog_AdbLookupList(AdbExpandListType *adblist,
		      MWindow *parent = NULL);

/// simple AboutDialog to be displayed at startup
void
MDialog_AboutDialog( MWindow *parent);

#endif
