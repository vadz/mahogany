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

#   include  <wx/file.h>
#   include  <wx/textfile.h>
#   include  <wx/config.h>
#   include  <wx/fileconf.h>
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
#include "Script.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"

#include "MDialogs.h"
#include  "gui/wxOptionsDlg.h"


#include "Mversion.h"


#ifdef OS_UNIX
#  include  <unistd.h>
#  include  <sys/stat.h>
#endif

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

#ifdef  USE_PYTHON
  // only used here
  extern bool InitPython(void);
#endif //Python

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MAppBase - the class which defines the "application object" interface
// ----------------------------------------------------------------------------
MAppBase::MAppBase()
{
   m_topLevelFrame = NULL;
}

bool
MAppBase::VerifySettings(void)
{
   const size_t bufsize = 200;
   char  buffer[bufsize];
   
   if( strutil_isempty(READ_APPCONFIG(MP_USERNAME)) )
   {
      wxGetUserId(buffer,bufsize);
      Profile::GetAppConfig()->WRITE_ENTRY(MP_USERNAME, buffer);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_PERSONALNAME)) )
   {
      wxGetUserName(buffer,bufsize);
      Profile::GetAppConfig()->WRITE_ENTRY(MP_PERSONALNAME, buffer);
   }

#  ifdef OS_UNIX
      if( strutil_isempty(READ_APPCONFIG(MC_USERDIR)) )
      {
         wxString strHome;
//FIXME         wxGetHomeDir(&strHome);
         strHome = getenv("HOME");
         strHome << DIR_SEPARATOR << READ_APPCONFIG(MC_USER_MDIR);
         Profile::GetAppConfig()->WRITE_ENTRY(MC_USERDIR, strHome);
      }
#  endif // Unix
   
   if( strutil_isempty(READ_APPCONFIG(MP_HOSTNAME)) )
   {
      wxGetHostName(buffer,bufsize);
      Profile::GetAppConfig()->WRITE_ENTRY(MP_HOSTNAME, buffer);
   }

   bool bFirstRun = READ_APPCONFIG(MC_FIRSTRUN) != 0;
   if ( bFirstRun )
   {
      // next time won't be the first one any more
      Profile::GetAppConfig()->WRITE_ENTRY(MC_FIRSTRUN, 0L);

      // write the version
      Profile::GetAppConfig()->WRITE_ENTRY(MC_VERSION, M_VERSION);
   }

   return bFirstRun;
}

MAppBase::~MAppBase()
{
}

bool
MAppBase::OnStartup()
{
   // initialise the profile(s)
   // -------------------------
   m_cfManager = new ConfigFileManager;

      String strConfFile;
#     ifdef OS_UNIX
         strConfFile = wxFileConfig::GetLocalFileName(M_APPLICATIONNAME);
         // FIXME must create the directory ourselves!
         struct stat st;
         if ( stat(strConfFile, &st) != 0 || !S_ISDIR(st.st_mode) ) {
           if ( mkdir(strConfFile, 0777) != 0 ) {
             wxLogError(_("Can't create the directory for configuration"
                          "files '%s'."), strConfFile.c_str());
             
             return FALSE;
           }
           
           wxLogInfo(_("Created directory '%s' for configuration files."),
                     strConfFile.c_str());
         }

         strConfFile += "/config";
#     else  // Windows
         strConfFile << "wxWindows\\" << M_APPLICATIONNAME;
#     endif // Unix

      m_profile = GLOBAL_NEW Profile(strConfFile);

#  ifndef OS_WIN
      // set the default path for configuration entries
      m_profile->GetConfig()->SET_PATH(M_APPLICATIONNAME);
#  endif

   // do we have gettext()?
   // ---------------------
#  ifdef  USE_GETTEXT
      setlocale (LC_ALL, "");
      //bindtextdomain (M_APPLICATIONNAME, LOCALEDIR);
      textdomain (M_APPLICATIONNAME);
#  endif // USE_GETTEXT


   // do any first-time specific initializations
   // ------------------------------------------
   bool bFirstRun = VerifySettings();

   if ( bFirstRun ) {
      wxLog *log = wxLog::GetActiveTarget();
      if ( log ) {
        wxLogMessage(_("As it seems that you're running M for the first\n"
                       "time, you should probably set up some of the options\n"
                       "needed by the program (especially network "
                       "parameters)."));
        log->Flush();
      }

      ShowOptionsDialog();
   }

   // find our directories
   // --------------------
   String tmp;
#  ifdef OS_UNIX
      bool   found;
      String strRootDir = READ_APPCONFIG(MC_ROOTDIRNAME);
      PathFinder pf(READ_APPCONFIG(MC_PATHLIST));

      pf.AddPaths(M_DATADIR);

      m_globalDir = pf.FindDir(strRootDir, &found);

      if(! found)
      {
         String msg = _("Cannot find global directory \"");
         msg += strRootDir;
         msg += _("\" in\n \"");
         msg += String(READ_APPCONFIG(MC_PATHLIST));
         ERRORMESSAGE((Str(msg)));
      }

      m_localDir = wxExpandEnvVars(READ_APPCONFIG(MC_USERDIR));
#  else  //Windows
      // under Windows our directory is always the one where the executable is
      // located. At least we're sure that it exists this way...
      wxString strPath;
      ::GetModuleFileName(::GetModuleHandle(NULL),
                          strPath.GetWriteBuf(MAX_PATH), MAX_PATH);
      strPath.UngetWriteBuf();

      // extract the dir name
      wxSplitPath(strPath, &m_globalDir, NULL, NULL);
      m_localDir = m_globalDir;
#  endif //Unix

   // create and show the main program window
   CreateTopLevelFrame();

   // it doesn't seem to do anything under Windows (though it should...)
#  ifndef OS_WIN
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
#  endif //OS_WIN

   // initialise python interpreter
#  ifdef  USE_PYTHON
      // having the same error message each time M is started is annoying, so
      // give the user a possibility to disable it
      if ( READ_APPCONFIG(MC_USEPYTHON) && !InitPython() ) {
         const char *msg = "It's possible that you have problems with Python\n"
                           "installation. Would you like to disable Python\n"
                           "support for now (set " MC_USEPYTHON " to 1 to"
                           "reenable it later)?";
         if ( MDialog_YesNoDialog(_(msg)) ) {
            // disable it
            Profile::GetAppConfig()->WRITE_ENTRY(MC_USEPYTHON, (long)FALSE);
         }
      }
#  endif //USE_PYTHON


   m_mimeList = GLOBAL_NEW MimeList();
   m_mimeTypes = GLOBAL_NEW MimeTypes();

   // open all default mailboxes
   // --------------------------
   char *folders = strutil_strdup(READ_APPCONFIG(MC_OPENFOLDERS));
   kbStringList openFoldersList;
   strutil_tokenise(folders,";",openFoldersList);
   GLOBAL_DELETE [] folders;
   kbStringList::iterator i;
   for(i = openFoldersList.begin(); i != openFoldersList.end(); i++)
   {
      if((*i)->length() == 0) // empty token
         continue;
      DBGMESSAGE(("Opening folder '%s'...", (*i)->c_str()));
      (void) new wxFolderViewFrame((**i), m_topLevelFrame);
   }

   // now that we have the local dir, we can set up a default mail
   // folder dir
   {
      ProfilePathChanger ppc(m_profile->GetConfig(), M_PROFILE_CONFIG_SECTION);
      tmp = READ_CONFIG(m_profile, MP_MBOXDIR);
      if( strutil_isempty(tmp) )
      {
         tmp = m_localDir;
         m_profile->writeEntry(MP_MBOXDIR, tmp.c_str());
      }
   }
   
   return TRUE;
}

void
MAppBase::OnAbnormalTermination()
{
}

void
MAppBase::OnShutDown()
{
   GLOBAL_DELETE m_mimeList;
   GLOBAL_DELETE m_mimeTypes;
   GLOBAL_DELETE m_cfManager;
   GLOBAL_DELETE m_profile;
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
   if ( force || MDialog_YesNoDialog(_("Really exit M?")) )
   {
      m_topLevelFrame->Close(TRUE);
      m_topLevelFrame = NULL;
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

