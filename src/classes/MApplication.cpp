/*-*- c++ -*-********************************************************
 * MAppBase class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$         *
 *                                                                  *
 * $Log$
 * Revision 1.26  1998/07/20 12:23:06  KB
 * Adopted MApplication for new wxConfig.
 *
 * Revision 1.25  1998/07/12 15:05:25  KB
 * some fixes and ugly fix to work with std::string again
 *
 * Revision 1.24  1998/07/08 11:56:55  KB
 * M compiles and runs on Solaris 2.5/gcc 2.8/c-client gso
 *
 * Revision 1.23  1998/06/23 08:46:45  KB
 * fixed #pragma
 *
 * Revision 1.22  1998/06/22 22:36:57  VZ
 *
 * config file name fixed for Windows
 *
 * Revision 1.21  1998/06/19 08:05:15  KB
 * restructured FolderView, menu handling and added toolbars
 *
 * Revision 1.20  1998/06/14 12:24:18  KB
 * started to move wxFolderView to be a panel, Python improvements
 *
 * Revision 1.19  1998/06/12 16:06:57  KB
 * updated
 *
 * Revision 1.18  1998/06/08 08:19:12  KB
 * Fixed makefiles for wxtab/python. Made Python work with new MAppBase.
 *
 * Revision 1.17  1998/06/05 16:56:10  VZ
 *
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.16  1998/05/24 08:22:41  KB
 * changed the creation/destruction of MailFolders, now done through
 * MailFolder::Open/CloseFolder, made constructor/destructor private,
 * this allows multiple view on the same folder
 *
 * Revision 1.15  1998/05/18 17:48:29  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.14  1998/05/15 22:04:44  VZ
 *
 * small USE_WXCONFIG-only change
 *
 * Revision 1.13  1998/05/14 11:00:15  KB
 * Code cleanups and conflicts resolved.
 *
 * Revision 1.12  1998/05/14 09:48:50  KB
 * added IsEmpty() to strutil, minor changes
 *
 * Revision 1.11  1998/05/13 19:02:08  KB
 * added kbList, adapted MimeTypes for it, more python, new icons
 *
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
#endif   // USE_PCH

#include   <locale.h>
#include   <errno.h>

#include "Mdefaults.h"

#include "FolderView.h"
#include "Adb.h"
#include "Script.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"
#include "gui/wxMLogFrame.h"

#include "MDialogs.h"

#include "MApplication.h"

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
   logFrame = NULL;
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
   GLOBAL_DELETE logFrame;
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

      // FIXME @@@@ do something about recordDefaults
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

   // create log window
   logFrame = new wxMLogFrame;

   // create and show the main program window
   CreateTopLevelFrame();

   String   tmp;
   bool   found;
   String strRootDir = READ_APPCONFIG(MC_ROOTDIRNAME);
   PathFinder pf(READ_APPCONFIG(MC_PATHLIST));
   globalDir = pf.FindDir(strRootDir, &found);

   VerifySettings();
   
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
      //VAR((*i)->c_str());
      wxLogDebug("Opening folder '%s'...", (*i)->c_str());
      new wxFolderViewFrame((**i),topLevelFrame);
   }
   
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
      // avoid any more output being printed
      GLOBAL_DELETE logFrame;
      logFrame = NULL;

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

