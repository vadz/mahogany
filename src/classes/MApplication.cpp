/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$         *
 *                                                                  *
 * $Log$
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

#include     "Mpch.h"

#ifndef   USE_PCH
#   include   "Mcommon.h"
#   include   "strutil.h"
#   include   "Profile.h"
#   include   "MFrame.h"
#   include   "MLogFrame.h"
#   include   "MimeList.h"
#   include   "MimeTypes.h"
#   include   "Mdefaults.h"
#   include   "MApplication.h"
#   include   "kbList.h"
#endif


#include   <locale.h>
#include   <errno.h>

#include   "PathFinder.h"
#include   "FolderView.h"
#include   "Adb.h"
#include   "Script.h"
#include   "MailFolder.h"
#include   "MailFolderCC.h"
#include   "gui/wxFolderView.h"
#include   "gui/wxMainFrame.h"

#ifdef  USE_PYTHON
  // only used here
  extern void InitPython(void);
#endif //Python

// AppConf must be initialized with different args under Unix and Windows
// and if we're using wxWin2 we don't inherit from it at all
// @@@ all this looks quite horrible
#ifdef USE_WXCONFIG
#  include  <wx/file.h>
#  include  <wx/textfile.h>
#  include  <wx/fileconf.h>

#  define   CALL_APPCONF_CTOR
#else
#  ifdef OS_UNIX
#     define   CALL_APPCONF_CTOR : AppConfig(M_APPLICATIONNAME,\
                                             FALSE, FALSE, TRUE)
#  else
#     define   CALL_APPCONF_CTOR : AppConfig(M_APPLICATIONNAME)
#  endif
#endif

MApplication::MApplication(void) CALL_APPCONF_CTOR
{
   initialisedFlag = false;
   logFrame = NULL;
   
#  ifdef USE_WXCONFIG
      wxConfig::Set(GLOBAL_NEW wxFileConfig(
                    wxFileConfig::GetLocalFileName(M_APPLICATIONNAME)));
      // TODO: recordDefaults & expandVariables
#  else
      //  activate recording of configuration entries
      if(readEntry(MC_RECORDDEFAULTS,MC_RECORDDEFAULTS_D))
         recordDefaults(TRUE);

      // activate variable expansion as default
      expandVariables(TRUE);
#  endif

   // set the default path for configuration entries
   setCurrentPath(M_APPLICATIONNAME);
   VAR(readEntry("TestEntry","DefaultValue"));

   // initialise the profile
#ifdef   USE_WXCONFIG
      profile = GLOBAL_NEW ProfileAppConfig();
#else
      profile = GLOBAL_NEW ProfileAppConfig(this);
#endif

   adb = NULL;
   // do we have gettext() ?
#if   USE_GETTEXT
   setlocale (LC_ALL, "");
   //bindtextdomain (M_APPLICATIONNAME, LOCALEDIR);
   textdomain (M_APPLICATIONNAME);
#endif


}

void
MApplication::VerifySettings(void)
{
   const char *val;
   const size_t   bufsize = 200;
   char   buffer[bufsize];
   
   val = readEntry(MP_USERNAME,MP_USERNAME_D);
   if(! *val)
   {
      wxGetUserId(buffer,bufsize);
      writeEntry(MP_USERNAME,buffer);
   }

   val = readEntry(MP_PERSONALNAME,MP_PERSONALNAME_D);
   if(! *val)
   {
      wxGetUserName(buffer,bufsize);
      writeEntry(MP_PERSONALNAME,buffer);
   }

   val = readEntry(MC_USERDIR,MC_USERDIR_D);
   if(! *val)
   {
      wxGetHomeDir(buffer);
      strcat(buffer,"/");
      strcat(buffer,readEntry(MC_USER_MDIR,MC_USER_MDIR_D));
      writeEntry(MC_USERDIR,buffer);
   }
   
   val = readEntry(MP_HOSTNAME,MP_HOSTNAME_D);
   if(! *val)
   {
      wxGetHostName(buffer,bufsize);
      writeEntry(MP_HOSTNAME,buffer);
   }
  
}

MApplication::~MApplication()
{
   if(! initialisedFlag)
      return;
   GLOBAL_DELETE mimeList;
   GLOBAL_DELETE mimeTypes;
   GLOBAL_DELETE profile;
   if(adb)
      GLOBAL_DELETE adb;

   flush();

   #ifdef USE_WXCONFIG
      GLOBAL_DELETE wxConfig::Set(NULL);
   #endif
}

MFrame *
MApplication::OnInit(void)
{
   // this is being called from the GUI's initialisation function
   String   tmp;
   topLevelFrame = GLOBAL_NEW MainFrame();
   if(topLevelFrame)
   {
      initialisedFlag = true;
      topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);
      topLevelFrame->Show(true);
   }

   bool   found;
   String strRootDir = readEntry(MC_ROOTDIRNAME,MC_ROOTDIRNAME_D);
   PathFinder   pf(readEntry(MC_PATHLIST,MC_PATHLIST_D));
   globalDir = pf.FindDir(strRootDir, &found);

   VerifySettings();
   
   if(! found)
   {
      String msg = _("Cannot find global directory \"");
      msg += strRootDir;
      msg += _("\" in\n \"");
      msg += String(readEntry(MC_PATHLIST,MC_PATHLIST_D));
      ErrorMessage(msg,topLevelFrame,true);

      // we should either abort immediately or continue without returning
      // or vital initializations are skipped!
      //return NULL;
   }

   #ifdef   USE_WXCONFIG
      localDir = ExpandEnvVars(readEntry(MC_USERDIR,MC_USERDIR_D));
   #else
      char *cptr = ExpandEnvVars(readEntry(MC_USERDIR,MC_USERDIR_D));
      localDir = String(cptr);
      GLOBAL_DELETE [] cptr;
   #endif

   // extend path for commands, look in M's dirs first
   tmp="";
   tmp += GetLocalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp = GetGlobalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   if(getenv("PATH"))
      tmp += getenv("PATH");
   #  ifdef  __WINDOWS__
      // FIXME @@@@ what are setenv() params? can putenv be used instead?
   #else
      setenv("PATH", tmp.c_str(), 1);
   #endif

   // initialise python interpreter
#  ifdef  USE_PYTHON
      // only used here
      InitPython();

      ExternalScript   echo("echo \"Hello World!\"", "", "");
//      echo.Run("\"and so on\"");
      cout << echo.GetOutput() << endl;
#  endif //Python

   // now the icon is available, so do this again:
   topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);

   ShowConsole(readEntry(MC_SHOWCONSOLE,(long int)MC_SHOWCONSOLE_D) != 0);

   mimeList = GLOBAL_NEW MimeList();
   mimeTypes = GLOBAL_NEW MimeTypes();

   // load address database
   PathFinder lpf(localDir);
   String adbName = lpf.FindFile(readEntry(MC_ADBFILE,MC_ADBFILE_D), &found);
   if(! found)
      adbName = localDir + '/' + readEntry(MC_ADBFILE,MC_ADBFILE_D);
   adb = GLOBAL_NEW Adb(adbName);

   // Open all default mailboxes:
   char *folders = strutil_strdup(readEntry(MC_OPENFOLDERS,MC_OPENFOLDERS_D));
   kbStringList openFoldersList;
   strutil_tokenise(folders,";",openFoldersList);
   GLOBAL_DELETE [] folders;
   kbStringList::iterator i;
   for(i = openFoldersList.begin(); i != openFoldersList.end(); i++)
   {
      if((*i)->length() == 0) // empty token
         continue;
      
      wxLogDebug("Opening folder '%s'...", (*i)->c_str());
      MailFolderCC *mf = MailFolderCC::OpenFolder(**i);
      if(mf->IsInitialised())
         (GLOBAL_NEW wxFolderView(mf,(*i)->c_str(), topLevelFrame))->Show();
      else
         mf->CloseFolder();
   }
   
   return topLevelFrame;
}

const char *
MApplication::GetText(const char *in)
{
#if   USE_GETTEXT
   return   gettext(in);
#else
   return   in;
#endif
}

void
MApplication::Exit(bool force)
{
   if(force || YesNoDialog(_("Really exit M?"),topLevelFrame))
   {
      logFrame = NULL; // avoid any more output being printed
      if(topLevelFrame)
         GLOBAL_DELETE topLevelFrame;
      else
         exit(0);
   }
}

void
MApplication::ErrorMessage(String const &message, MFrame *parent, bool
            modal)
{
#ifdef   USE_WXWINDOWS
   wxMessageBox((char *)message.c_str(), _("Error"),
      wxOK|wxCENTRE|wxICON_EXCLAMATION, parent);
#else
#   error MApplication::ErrorMessage() not implemented
#endif 
}

void
MApplication::SystemErrorMessage(String const &message, MFrame *parent, bool
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
#   error MApplication::ErrorMessage() not implemented
#endif 
}

void
MApplication::FatalErrorMessage(String const &message,
           MFrame *parent)
{   
   String msg = message + _("\nExiting application...");
   ErrorMessage(message,parent,true);
   Exit(true);
}


void
MApplication::Message(String const &message, MFrame *parent, bool
            modal)
{
#ifdef   USE_WXWINDOWS
   wxMessageBox((char *)message.c_str(), _("Information"),
      wxOK|wxCENTRE|wxICON_INFORMATION, parent);
#else
#   error MApplication::Message() not implemented
#endif 
}

void
MApplication::ShowConsole(bool display)
{
   if(display)
   {
      if(logFrame == NULL)
    logFrame = GLOBAL_NEW MLogFrame();
      logFrame->Show(TRUE);
   }

   // wxWin uses the frame after calling OnClose() from which we're
   // called, so don't delete it!
   #ifndef  USE_WXWINDOWS2
      else if(logFrame)
      {
         GLOBAL_DELETE logFrame;
         logFrame = NULL;
      }
   #endif
}
   
void
MApplication::Log(int level, String const &message)
{
   if(! logFrame)
      return;
   if(level >= readEntry(MP_LOGLEVEL,(long int)MP_LOGLEVEL_D))
   {
      logFrame->Write(message);
      logFrame->Write("\n");
   }
}

bool
MApplication::YesNoDialog(String const &message,
           MFrame *parent,
           bool modal,
           bool YesDefault)
{
#ifdef   USE_WXWINDOWS
   return (bool) (wxYES == wxMessageBox(
      (char *)message.c_str(),
      _("Decision"),
      wxYES_NO|wxCENTRE|wxICON_QUESTION,
      parent));
#else
#   error MApplication::YesNoDialog() not implemented
#endif 
}


/// this creates the one and only application object
MApplication mApplication;
