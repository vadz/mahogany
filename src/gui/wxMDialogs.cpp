/*-*- c++ -*-********************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.5  1998/05/24 14:48:15  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.4  1998/05/18 17:48:43  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.3  1998/04/22 19:56:32  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.2  1998/03/26 23:05:41  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxMDialogs.h"
#endif

#include	"Mpch.h"
#include  "Mcommon.h"

#include	<errno.h>

#if       !USE_PCH
  #include	<guidef.h>
  #include	<strutil.h>
#endif

#include	"MFrame.h"
#include	"MLogFrame.h"

#include	"Mdefaults.h"

#include	"PathFinder.h"
#include	"MimeList.h"
#include	"MimeTypes.h"
#include	"Profile.h"

#include  "MApplication.h"

#include  "Adb.h"
#include  "MDialogs.h"

void	
MDialog_ErrorMessage(const char *message,
			 MFrame *parent,
			 const char *title,
			 bool modal)
{
   wxMessageBox((char *)message, title,
		wxOK|wxCENTRE|wxICON_EXCLAMATION, parent);
}


/** display system error message:
    @param message the text to display
    @param parent	the parent frame
    @param title	title for message box window
    @param modal	true to make messagebox modal
   */
void
MDialog_SystemErrorMessage(const char *message,
			      MFrame *parent,
			      const char *title,
			      bool modal)
{
   String
      msg;
   
   msg = String(message) + String(("\nSystem error: "))
      + String(strerror(errno));

   MDialog_ErrorMessage(msg.c_str(), parent, title, modal);
}

   
/** display error message and exit application
       @param message the text to display
       @param title	title for message box window
       @param parent	the parent frame
   */
void
MDialog_FatalErrorMessage(const char *message,
			     MFrame *parent,
			     const char *title)
{
   String msg = String(message) + _("\nExiting application...");
   MDialog_ErrorMessage(message,parent,title,true);
   mApplication.Exit(true);
}

   
/** display normal message:
       @param message the text to display
       @param parent	the parent frame
       @param title	title for message box window
       @param modal	true to make messagebox modal
   */
void
MDialog_Message(const char *message,
		   MFrame *parent,
		   const char *title,
		   bool modal)
{
   wxMessageBox((char *)message, title,
		wxOK|wxCENTRE|wxICON_INFORMATION, parent);
}


/** simple Yes/No dialog
       @param message the text to display
       @param parent	the parent frame
       @param title	title for message box window
       @param modal	true to make messagebox modal
       @param YesDefault true if Yes button is default, false for No as default
       @return true if Yes was selected
   */
bool
MDialog_YesNoDialog(const char *message,
		       MFrame *parent,
		       bool modal,
		       const char *title,
		       bool YesDefault)
{
   return (bool) (wxYES == wxMessageBox(
      (char *)message,title,
      wxYES_NO|wxCENTRE|wxICON_QUESTION,
      parent));
}


/** Filerequester
       @param message the text to display
       @param parent	the parent frame
       @param path	default path
       @param filename	default filename
       @param extension	default extension
       @param wildcard	pattern matching expression
       @param save	if true, for saving a file
       @return pointer to a temporarily allocated buffer with he filename, or NULL
   */
const char *
MDialog_FileRequester(const char *message,
			 MFrame *parent,
			 const char *path,
			 const char *filename,
			 const char *extension,
		      const char *wildcard,
		      bool save,
		      ProfileBase *profile)
{
   if(! profile)
      profile = mApplication.GetProfile();
   
   if(! path)
      path = save ?
	 profile->readEntry(MP_DEFAULT_SAVE_PATH,MP_DEFAULT_SAVE_PATH_D)
	 : profile->readEntry(MP_DEFAULT_LOAD_PATH,MP_DEFAULT_LOAD_PATH_D);
   if(! filename)
      filename = save ?
	 profile->readEntry(MP_DEFAULT_SAVE_FILENAME,MP_DEFAULT_SAVE_FILENAME_D)
	 : profile->readEntry(MP_DEFAULT_LOAD_FILENAME,MP_DEFAULT_LOAD_FILENAME_D);
   if(! extension)
      extension = save ?
	 profile->readEntry(MP_DEFAULT_SAVE_EXTENSION,MP_DEFAULT_SAVE_EXTENSION_D)
	 : profile->readEntry(MP_DEFAULT_LOAD_EXTENSION,MP_DEFAULT_LOAD_EXTENSION_D);
      if(! wildcard)
	 wildcard = save ?
	    profile->readEntry(MP_DEFAULT_SAVE_WILDCARD,MP_DEFAULT_SAVE_WILDCARD_D)
	    : profile->readEntry(MP_DEFAULT_LOAD_WILDCARD,MP_DEFAULT_LOAD_WILDCARD_D);
   return wxFileSelector(
      message, path, filename, extension, wildcard, 0, parent);
}

AdbEntry *
MDialog_AdbLookupList(AdbExpandListType *adblist, MFrame *parent)
{
   AdbExpandListType::iterator i;

   int
      idx = 0,
      val,
      n = adblist->size();
   String
      tmp;
   char
      ** choices = new char *[n];
   AdbEntry
      ** entries = new AdbEntry *[n],
      *result;
   
   for(i = adblist->begin(); i != adblist->end(); i++, idx++)
   {
      tmp =(*i)->formattedName + String(" <")
	 + (*i)->email.preferred.c_str() + String(">");
      choices[idx] = strutil_strdup(tmp);
      entries[idx] = (*i);
   }
   int w,h;
   if(parent)
   {
      parent->GetClientSize(&w,&h);
      w = (int) (w*0.8);
      h = (int) (h*0.8);
   }
   else
   {
      w = 200;
      h = 250;
   }	
   val = wxGetSingleChoiceIndex(
      _("Please choose an entry:"),
      _("Expansion options"),
      n, (char **)choices, parent,
      -1,-1, // x,y
      TRUE,//centre
      w,h);

   if(val == -1)
      result = NULL;
   else
      result = entries[val];

   for(idx = 0; idx < n; idx++)
      delete [] choices[idx];
   
   delete [] choices;
   delete [] entries;
  
   return result;
}

