/*-*- c++ -*-********************************************************
 * wxMDialogs.h : wxWindows version of dialog boxes                 *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$            *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "wxMDialogs.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"
#include "Mcommon.h"

#include <errno.h>

#ifndef USE_PCH
#  include <guidef.h>
#  include <strutil.h>
#  include "MFrame.h"
#  include "MLogFrame.h"
#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#endif

#include "Mdefaults.h"

#include "MApplication.h"
#include "gui/wxMApp.h"

#include "Adb.h"
#include "MDialogs.h"
#include "gui/wxlwindow.h"

// ============================================================================
// implementation
// ============================================================================

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
    @param parent the parent frame
    @param title  title for message box window
    @param modal  true to make messagebox modal
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

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   
   MDialog_ErrorMessage(msg.c_str(), parent, title, modal);
}

   
/** display error message and exit application
       @param message the text to display
       @param title  title for message box window
       @param parent the parent frame
   */
void
MDialog_FatalErrorMessage(const char *message,
              MFrame *parent,
              const char *title)
{
   String msg = String(message) + _("\nExiting application...");

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   
   MDialog_ErrorMessage(message,parent,title,true);
   mApplication.Exit(true);
}

   
/** display normal message:
       @param message the text to display
       @param parent the parent frame
       @param title  title for message box window
       @param modal  true to make messagebox modal
   */
void
MDialog_Message(const char *message,
         MFrame *parent,
         const char *title,
         bool modal)
{

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   
   wxMessageBox((char *)message, title,
      wxOK|wxCENTRE|wxICON_INFORMATION, parent);
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
MDialog_YesNoDialog(const char *message,
             MFrame *parent,
             bool modal,
             const char *title,
             bool YesDefault)
{

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   
   return (bool) (wxYES == wxMessageBox(
      (char *)message,title,
      wxYES_NO|wxCENTRE|wxICON_QUESTION,
      parent));
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

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   
   return wxFileSelector(
      message, path, filename, extension, wildcard, 0, parent);
}

// simple "Yes/No" dialog (@@ we'd surely need one with [Cancel] also)
bool
MDialog_YesNoDialog(String const &message,
                    MFrame *parent,
                    bool modal,
                    bool YesDefault)
{
#  ifdef  OS_WIN
      const long styleMsgBox = wxYES_NO|wxICON_QUESTION;
#  else
      const long styleMsgBox = wxYES_NO|wxCENTRE|wxICON_QUESTION;
#  endif

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   

   return wxMessageBox((char *)message.c_str(), _("Decision"),
                       styleMsgBox,
                       parent == NULL ? mApplication.TopLevelFrame() 
                                      : parent) == wxYES;
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

   if(parent == NULL)
      parent = mApplication.TopLevelFrame();
   
   
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

// simple AboutDialog to be displayed at startup
void
MDialog_AboutDialog( MFrame *parent)
{

   wxFrame *frame = new wxFrame();

   frame->Create(parent,-1, _("Welcome"));

   wxLayoutWindow *lw = new wxLayoutWindow(frame);

   wxLayoutList &ll = lw->GetLayoutList();
   
   frame->Show(FALSE);

   ll.SetEditable(true);
   ll.Insert("Welcome to M!");
   ll.LineBreak();
   
   ll.SetEditable(false);

   frame->Show(TRUE);
   frame->Fit();
}
