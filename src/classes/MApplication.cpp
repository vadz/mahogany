/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$          *
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

// only used here
extern void InitPython(void);

#if USE_WXGTK
  //FIXME wxBell in wxGTK
  void wxBell(void) { }
  
  IMPLEMENT_WXWIN_MAIN
#endif

#ifdef OS_UNIX
MApplication::MApplication(void) : AppConfig(M_APPLICATIONNAME, FALSE,
                    FALSE, TRUE)
#else
MApplication::MApplication(void) : AppConfig(M_APPLICATIONNAME)
#endif
{
   initialisedFlag = false;
   logFrame = NULL;

   
   //  activate recording of configuration entries
   if(readEntry(MC_RECORDDEFAULTS,MC_RECORDDEFAULTS_D))
      recordDefaults(TRUE);

   // activate variable expansion as default
   expandVariables(TRUE);
   // set the default path for configuration entries
   setCurrentPath(M_APPLICATIONNAME);
   VAR(readEntry("TestEntry","DefaultValue"));
   // initialise the profile
   profile = GLOBAL_NEW ProfileAppConfig(this);

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
   PathFinder   pf(readEntry(MC_PATHLIST,MC_PATHLIST_D));
   globalDir = pf.FindDir(readEntry(MC_ROOTDIRNAME,MC_ROOTDIRNAME_D),
           &found);

   VerifySettings();
   
   if(! found)
   {
      String
    msg = _("Cannot find global directory \"")
    + String(readEntry(MC_ROOTDIRNAME,MC_ROOTDIRNAME_D));
      msg += _("\" in\n \"");
      msg += String(readEntry(MC_PATHLIST,MC_PATHLIST_D));
      ErrorMessage(msg,topLevelFrame,true);
      return NULL;
   }

   char *cptr = ExpandEnvVars(readEntry(MC_USERDIR,MC_USERDIR_D));
   localDir = String(cptr);
   GLOBAL_DELETE [] cptr;

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
   setenv("PATH", tmp.c_str(), 1);
 
   // initialise python interpreter
   InitPython();

   ExternalScript   echo("echo \"Hello World!\"", "", "");
   echo.Run("\"and so on\"");
   cout << echo.GetOutput() << endl;
   
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
   std::list<String>   openFoldersList;
   strutil_tokenise(folders,";",openFoldersList);
   GLOBAL_DELETE [] folders;
   std::list<String>::iterator i;
   for(i = openFoldersList.begin(); i != openFoldersList.end(); i++)
   {
      if((*i).length() == 0) // empty token
    continue;
      MailFolderCC *mf = GLOBAL_NEW MailFolderCC(*i);
      if(mf->IsInitialised())
    (GLOBAL_NEW wxFolderView(mf, *i, topLevelFrame))->Show();
      else
    GLOBAL_DELETE mf;
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
   else if(logFrame)
   {
      GLOBAL_DELETE logFrame;
      logFrame = NULL;
   }
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
