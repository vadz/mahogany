/*-*- c++ -*-********************************************************
 * MAppBase class: all non GUI specific application stuff           *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
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

#include <locale.h>
#include <errno.h>

#include "FolderView.h"
#include "Script.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "gui/wxMDialogs.h"   // MDialog_YesNoDialog

#include "Mversion.h"

#include <wx/confbase.h>      // wxExpandEnvVars
#include <wx/mimetype.h>      // wxMimeTypesManager

#include "MFolder.h"          // DeleteRootFolder

#ifdef OS_UNIX
#  include  <unistd.h>
#  include  <sys/stat.h>
#endif //Unix

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

MAppBase *mApplication = NULL;

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

   String
      str;

   str = MP_USERNAME;
   str = m_profile->readEntry(str, MP_USERNAME_D);

   if( strutil_isempty(READ_APPCONFIG(MP_USERNAME)) )
   {
      wxGetUserId(buffer,bufsize);
      m_profile->writeEntry(MP_USERNAME, buffer);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_PERSONALNAME)) )
   {
      wxGetUserName(buffer,bufsize);
      m_profile->writeEntry(MP_PERSONALNAME, buffer);
   }

#  ifdef OS_UNIX
   if( strutil_isempty(READ_APPCONFIG(MC_USERDIR)) )
   {
      wxString strHome;
      //FIXME wxGetHomeDir(&strHome);
      strHome = getenv("HOME");
      strHome << DIR_SEPARATOR << READ_APPCONFIG(MC_USER_MDIR);
      m_profile->writeEntry(MC_USERDIR, strHome);
   }
#  endif // Unix

   if( strutil_isempty(READ_APPCONFIG(MP_HOSTNAME)) )
   {
      wxGetHostName(buffer,bufsize);
      m_profile->writeEntry(MP_HOSTNAME, buffer);
   }

   bool bFirstRun = READ_APPCONFIG(MC_FIRSTRUN) != 0;
   if ( bFirstRun )
   {
      // do we need to upgrade something?
      String version = m_profile->readEntry(MC_VERSION, "");
      if ( version != M_VERSION ) {
         if ( !Upgrade(version) )
            return false;

         // next time won't be the first one any more
         m_profile->writeEntry(MC_FIRSTRUN, 0);

         // write the new version
         m_profile->writeEntry(MC_VERSION, M_VERSION);
      }
   }

   return true;
}

MAppBase::~MAppBase()
{
}

bool
MAppBase::OnStartup()
{
   mApplication = this;

   // do we have gettext()?
   // ---------------------
#  ifdef  USE_GETTEXT
      setlocale (LC_ALL, "");
      //bindtextdomain (M_APPLICATIONNAME, LOCALEDIR);
      textdomain (M_APPLICATIONNAME);
#  endif // USE_GETTEXT


   // find our directories (has to be done first!)
   String tmp;
#ifdef OS_UNIX
   bool   found;
   if(PathFinder::IsDir(M_BASEDIR))
      m_globalDir = M_BASEDIR;
   else
   {
      PathFinder pf(MC_ETCPATH_D);
      pf.AddPaths(M_PREFIX,true,true); 
      m_globalDir = pf.FindDir(MC_ROOTDIRNAME_D, &found);
      if(!found)
      {
         // TODO instead of insulting the user it would be better to propose
         //      to find it right now or even create one
         String msg = _("Cannot find global directory \"");
         msg << MC_ROOTDIRNAME_D << _("\" in\n \"") << MC_ETCPATH_D << ':' << M_BASEDIR; 
         ERRORMESSAGE((Str(msg)));
      }
   }
#else  //Windows
      // under Windows our directory is always the one where the executable is
      // located. At least we're sure that it exists this way...
      wxString strPath;
      ::GetModuleFileName(::GetModuleHandle(NULL),
                          strPath.GetWriteBuf(MAX_PATH), MAX_PATH);
      strPath.UngetWriteBuf();

      // extract the dir name
      wxSplitPath(strPath, &m_globalDir, NULL, NULL);
#endif //Unix

      // initialise the profile(s)
   // -------------------------

   String strConfFile;
#ifdef OS_UNIX
   strConfFile = getenv("HOME");
   strConfFile << "/." << M_APPLICATIONNAME;
   // FIXME must create the directory ourselves!
   struct stat st;
   if ( stat(strConfFile, &st) != 0 || !S_ISDIR(st.st_mode) ) {
      if ( mkdir(strConfFile, 0777) != 0 )
      {
         wxLogError(_("Can't create the directory for configuration"
                      "files '%s'."), strConfFile.c_str());
         return FALSE;
      }
      
      wxLogInfo(_("Created directory '%s' for configuration files."),
                strConfFile.c_str());
   }
   
   strConfFile += "/config";
#else  // Windows
   strConfFile = M_APPLICATIONNAME;
#endif // Win/Unix

   m_profile = ProfileBase::CreateGlobalConfig(strConfFile);
   m_localDir = wxExpandEnvVars(READ_APPCONFIG(MC_USERDIR));

   // show the splash screen (do it as soon as we have profile to read
   // MC_SHOWSPLASH from)
   if ( READ_APPCONFIG(MC_SHOWSPLASH) )
   {
      // no parent because no frames created yet
      MDialog_AboutDialog(NULL);
   }

   // verify (and upgrade if needed) our settings
   // -------------------------------------------
   if ( !VerifySettings() )
   {
      ERRORMESSAGE((_("Program execution aborted due to "
                      "installation problems.")));

      return false;
   }

   m_mimeManager = new wxMimeTypesManager();


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
   if ( ! InitPython() )
   {
      // otherwise it would hide our message box
      CloseSplash();

      static const char *msg =
         "It's possible that you have problems with Python\n"
         "installation. Would you like to disable Python\n"
         "support for now (set " MC_USEPYTHON " to 1 to reenable it later)?";
      if ( MDialog_YesNoDialog(_(msg)) )
      {
         // disable it
         m_profile->writeEntry(MC_USEPYTHON, FALSE);
      }
   }
#  endif //USE_PYTHON

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
      (void)wxFolderViewFrame::Create((**i), m_topLevelFrame);
   }

   // now that we have the local dir, we can set up a default mail
   // folder dir
   {
      ProfilePathChanger ppc(m_profile, M_PROFILE_CONFIG_SECTION);
      tmp = READ_CONFIG(m_profile, MP_MBOXDIR);
      if( strutil_isempty(tmp) )
      {
         tmp = m_localDir;
         m_profile->writeEntry(MP_MBOXDIR, tmp);
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
   DeleteRootFolder();  // do it _before_ deleting the profile

   m_profile->DecRef();
   delete m_mimeManager;
}

const char *
MAppBase::GetText(const char *in) const
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
   m_topLevelFrame->Close(force);
}
