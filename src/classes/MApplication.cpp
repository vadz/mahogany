/*-*- c++ -*-********************************************************
 * MAppBase class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$         *
 *                                                                  *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "MApplication.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"

#ifndef   USE_PCH
#  include   "Mcommon.h"
#  include   "strutil.h"
#  include   "Profile.h"
#  include   "MimeList.h"
#  include   "MimeTypes.h"
#  include   "kbList.h"
#  include   "Mdefaults.h"
#  include   "MApplication.h"
#endif   // USE_PCH

#include   <locale.h>
#include   <errno.h>

#include "FolderView.h"
#include "Adb.h"
#include "Script.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"

#include "MDialogs.h"

#ifdef USE_WXCONFIG
#  include  <wx/file.h>
#  include  <wx/textfile.h>
#  include  <wx/fileconf.h>
#endif

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

#ifdef  USE_PYTHON
  // only used here
  extern void InitPython(void);
#endif //Python

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MAppBase - the class which defines the "application object" interface
// ----------------------------------------------------------------------------
MAppBase::MAppBase()
{
   adb = NULL;
}

void
MAppBase::VerifySettings(void)
{
   const char *val;
   const size_t bufsize = 200;
   char  buffer[bufsize];
   
   val = READ_APPCONFIG(MP_USERNAME);
   if(! *val)
   {
      wxGetUserId(buffer,bufsize);
      Profile::GetAppConfig()->WRITE_ENTRY(MP_USERNAME, buffer);
   }

   val = READ_APPCONFIG(MP_PERSONALNAME);
   if(! *val)
   {
      wxGetUserName(buffer,bufsize);
      Profile::GetAppConfig()->WRITE_ENTRY(MP_PERSONALNAME, buffer);
   }

   val = READ_APPCONFIG(MC_USERDIR);
   if(! *val)
   {
      wxGetHomeDir(buffer);
      strcat(buffer,"/");
      strcat(buffer, READ_APPCONFIG(MC_USER_MDIR));
      Profile::GetAppConfig()->WRITE_ENTRY(MC_USERDIR, buffer);
   }
   
   val = READ_APPCONFIG(MP_HOSTNAME);
   if(! *val)
   {
      wxGetHostName(buffer,bufsize);
      Profile::GetAppConfig()->WRITE_ENTRY(MP_HOSTNAME, buffer);
   }
  
}

MAppBase::~MAppBase()
{
   GLOBAL_DELETE mimeList;
   GLOBAL_DELETE mimeTypes;
   GLOBAL_DELETE adb;
}

bool
MAppBase::OnStartup()
{
   // initialise the profile
#  if USE_WXCONFIG
      String strConfFile = Str(wxFileConfig::GetLocalFileName(M_APPLICATIONNAME));
#     if OS_UNIX
         strConfFile += "/config";
#     endif // Unix

      profile = GLOBAL_NEW Profile(strConfFile);
#  else
      // FIXME @@@@ config file location?
      profile = GLOBAL_NEW Profile(M_APPLICATIONNAME);

      //  activate recording of configuration entries
      if ( READ_APPCONFIG(MC_RECORDDEFAULTS) )
         Profile::GetAppConfig()->recordDefaults(TRUE);
#  endif

   // set the default path for configuration entries
   profile->GetConfig()->SET_PATH(M_APPLICATIONNAME);

   // do we have gettext() ?
#  ifdef  USE_GETTEXT
      setlocale (LC_ALL, "");
      //bindtextdomain (M_APPLICATIONNAME, LOCALEDIR);
      textdomain (M_APPLICATIONNAME);
#  endif // USE_GETTEXT


   String   tmp;
   bool   found;
   String strRootDir = READ_APPCONFIG(MC_ROOTDIRNAME);
   PathFinder pf(READ_APPCONFIG(MC_PATHLIST));

#  if OS_UNIX
      pf.AddPaths(M_DATADIR);
#  else
      // don't know how to get it from makeopts under Windows...
#  endif

   globalDir = pf.FindDir(strRootDir, &found);

   if(! found)
   {
      String msg = _("Cannot find global directory \"");
      msg += strRootDir;
      msg += _("\" in\n \"");
      msg += String(READ_APPCONFIG(MC_PATHLIST));
      ERRORMESSAGE((Str(msg)));
   }

#  ifdef   USE_WXCONFIG
   localDir = wxExpandEnvVars(READ_APPCONFIG(MC_USERDIR));
#  else
   char *cptr = ExpandEnvVars(READ_APPCONFIG(MC_USERDIR));
   localDir = String(cptr);
   GLOBAL_DELETE [] cptr;
#  endif

   // create and show the main program window
   CreateTopLevelFrame();

   VerifySettings();
   
   // extend path for commands, look in M's dirs first
   tmp = "";
   tmp += GetLocalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp = GetGlobalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   if(getenv("PATH"))
      tmp += getenv("PATH");
   tmp="PATH="+tmp;
   putenv(tmp.c_str());
   // initialise python interpreter
#  ifdef  USE_PYTHON
      InitPython();
#  endif //Python


   mimeList = GLOBAL_NEW MimeList();
   mimeTypes = GLOBAL_NEW MimeTypes();

   // load address database
   PathFinder lpf(localDir);
   String adbName = lpf.FindFile(READ_APPCONFIG(MC_ADBFILE), &found);
   if(! found)
      adbName = localDir + '/' + READ_APPCONFIG(MC_ADBFILE);
   adb = GLOBAL_NEW Adb(adbName);

   // open all default mailboxes:
   char *folders = strutil_strdup(READ_APPCONFIG(MC_OPENFOLDERS));
   kbStringList openFoldersList;
   strutil_tokenise(folders,";",openFoldersList);
   GLOBAL_DELETE [] folders;
   kbStringList::iterator i;
   for(i = openFoldersList.begin(); i != openFoldersList.end(); i++)
   {
      if((*i)->length() == 0) // empty token
         continue;
      wxLogDebug("Opening folder '%s'...", (*i)->c_str());
      (void) new wxFolderViewFrame((**i),topLevelFrame);
   }

   // now that we have the local dir, we can set up a default mail
   // folder dir
   profile->GetConfig()->SET_PATH(M_PROFILE_CONFIG_SECTION);
   tmp = READ_CONFIG(profile,MP_MBOXDIR);
   if(strutil_isempty(tmp))
   {
      tmp = localDir;
      profile->writeEntry(MP_MBOXDIR, tmp.c_str());
   }
   // set the default path for configuration entries
   profile->GetConfig()->SET_PATH(M_APPLICATIONNAME);
   
   return TRUE;
}

const char *
MAppBase::GetText(const char *in)
{
#  ifdef   USE_GETTEXT
      return   gettext(in);
#  else
      return   in;
#  endif
}

void
MAppBase::Exit(bool force)
{
   if(force || MDialog_YesNoDialog(_("Really exit M?")))
   {
      // FIXME @@@@ this doesn't terminate the application in wxWin2
      // (other frames are still left on screen)
      if ( topLevelFrame != NULL )
      {
         GLOBAL_DELETE topLevelFrame;
         topLevelFrame = NULL;
      }
      else
         exit(0);
   }
}

// ----------------------------------------------------------------------------
// !wxWindows2 part
// ----------------------------------------------------------------------------
#ifndef USE_WXWINDOWS2

void
MAppBase::ErrorMessage(String const &message, MFrame *parent, bool
            modal)
{
#ifdef   USE_WXWINDOWS
   wxMessageBox((char *)message.c_str(), _("Error"),
      wxOK|wxCENTRE|wxICON_EXCLAMATION, parent);
#else
#   error MAppBase::ErrorMessage() not implemented
#endif 
}

void
MAppBase::SystemErrorMessage(String const &message, MFrame *parent, bool
            modal)
{
   String
      msg;
   
   msg = message + String(("\nSystem error: "))
         + String(strerror(errno));
#ifdef   USE_WXWINDOWS
   wxMessageBox((char *)msg.c_str(), _("System Error"),
      wxOK|wxCENTRE|wxICON_EXCLAMATION, parent);
#else
#   error MAppBase::ErrorMessage() not implemented
#endif 
}

void
MAppBase::FatalErrorMessage(String const &message,
           MFrame *parent)
{   
   String msg = message + _("\nExiting application...");
   ErrorMessage(message,parent,true);
   Exit(true);
}


void
MAppBase::Message(String const &message, MFrame *parent, bool
            modal)
{
#ifdef   USE_WXWINDOWS
   wxMessageBox((char *)message.c_str(), _("Information"),
      wxOK|wxCENTRE|wxICON_INFORMATION, parent);
#else
#   error MAppBase::Message() not implemented
#endif 
}

void
MAppBase::Log(int level, String const &message)
{
   if(! logFrame)
      return;
   if(level >= readEntry(MP_LOGLEVEL,(long int)MP_LOGLEVEL_D))
   {
      logFrame->Write(message);
      logFrame->Write("\n");
   }
}

#endif

