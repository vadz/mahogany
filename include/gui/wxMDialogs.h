/*-*- c++ -*-********************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef WXMDIALOGS_H
#define WXMDIALOGS_H

#ifdef __GNUG__
#pragma interface "wxMDialogs.h"
#endif

#include	<MApplication.h>
#include	<MFrame.h>
#include	<MDialogs.h>
#include	<Adb.h>

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
		     MFrame *parent = NULL,
		     char const *title = MDIALOG_ERRTITLE,
		     bool modal = false);

/** display system error message:
       @param message the text to display
       @param parent	the parent frame
       @param title	title for message box window
       @param modal	true to make messagebox modal
   */
void	MDialog_SystemErrorMessage(char const *message,
			   MFrame *parent = NULL,
			   char const *title = MDIALOG_SYSERRTITLE,
			   bool modal = false);
   
/** display error message and exit application
       @param message the text to display
       @param title	title for message box window
       @param parent	the parent frame
   */
void	MDialog_FatalErrorMessage(char const *message,
			  MFrame *parent = NULL,
			  char const *title = MDIALOG_FATALERRTITLE
   );
   
/** display normal message:
       @param message the text to display
       @param parent	the parent frame
       @param title	title for message box window
       @param modal	true to make messagebox modal
   */
void	MDialog_Message(char const *message,
		MFrame *parent = NULL,
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
		    MFrame *parent = NULL,
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
const char *	MDialog_FileRequester(char const *message,
				      MFrame *parent = NULL,
				      char const *path = NULL,
				      char const *filename = NULL,
				      char const *extension = NULL,
				      char const *wildcard = NULL,
				      bool save = false,
				      ProfileBase *profile = NULL
   );

AdbEntry *
MDialog_AdbLookupList(AdbExpandListType *adblist,
		      MFrame *parent = NULL);
		      
#endif
