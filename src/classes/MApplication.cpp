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
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "gui/wxMDialogs.h"   // MDialog_YesNoDialog

#include "Mversion.h"
#include "Mupgrade.h"

#include <wx/confbase.h>      // wxExpandEnvVars
#include <wx/mimetype.h>      // wxMimeTypesManager

#include <wx/persctrl.h>      // for wxPControls::SetSettingsPath

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

/* The code in VerifySettings somehow overlaps with the purpose of
   upgrade.cpp where we set up the INBOX profile and possibly other
   things. I just can't be bothered now, but this should go there,
   too.
*/
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

#ifdef OS_UNIX
   if( strutil_isempty(READ_APPCONFIG(MC_USERDIR)) )
   {
      wxString strHome;
      //FIXME wxGetHomeDir(&strHome);
      strHome = getenv("HOME");
      strHome << DIR_SEPARATOR << READ_APPCONFIG(MC_USER_MDIR);
      m_profile->writeEntry(MC_USERDIR, strHome);
   }
#endif // Unix

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
         if ( ! Upgrade(version) )
            return false;

         if(! SetupInitialConfig())
            ERRORMESSAGE((_("Failed to set up initial configuration.")));

         // next time won't be the first one any more
         m_profile->writeEntry(MC_FIRSTRUN, 0);

         // write the new version
         m_profile->writeEntry(MC_VERSION, M_VERSION);
      }
   }

   if ( !VerifyInbox() )
      wxLogDebug("Evil user removed his /Profiles/INBOX - restored.");

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
   char *locale;
   wxString ldir = M_BASEDIR;
   ldir += "/locale";
   locale = setlocale (LC_ALL, "");
   locale = bindtextdomain (M_APPLICATIONNAME, ldir.c_str());
   locale = bindtextdomain (M_APPLICATIONNAME, NULL); //LOCALEDIR);
   textdomain (M_APPLICATIONNAME);
#  endif // USE_GETTEXT

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

   // NB: although this shouldn't normally be here (it's GUI-dependent code),
   //     it's really impossible to put it into wxMApp because some dialogs
   //     can be already shown from here and this initialization must be done
   //     before.

   // for the persistent controls to work (wx/persctrl.h) we must have
   // a global wxConfig object
   wxConfigBase::Set(m_profile->GetConfig());

   // also set the path for persistent controls to save their state to
   wxPControls::SetSettingsPath("/Settings/");

   // find our directories
   // --------------------
   String tmp;
#ifdef OS_UNIX
   bool   found;
   if(PathFinder::IsDir(M_BASEDIR))
      m_globalDir = M_BASEDIR;
   else
   {
      PathFinder pf(MC_ETCPATH_D);
      pf.AddPaths(M_PREFIX,false,true); 
      m_globalDir = pf.FindDir(MC_ROOTDIRNAME_D, &found);
      if(!found)
      {
         String msg;
         msg.Printf(_("Cannot find global directory \"%s\" in\n"
                      "\"%s\"\n"
                      "Would you like to specify its location now?"),
                     MC_ROOTDIRNAME_D, MC_ETCPATH_D);
         if ( MDialog_YesNoDialog(msg, NULL, MDIALOG_YESNOTITLE,
                                  TRUE /* yes default */, "AskSpecifyDir") )
         {
            wxDirDialog dlg(NULL, _("Specify global directory for M"));
            if ( dlg.ShowModal() )
            {
               m_globalDir = dlg.GetPath();
            }
         }
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

#ifdef OS_UNIX
   m_localDir = wxExpandEnvVars(READ_APPCONFIG(MC_USERDIR));
#endif

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
   if ( READ_CONFIG(m_profile, MC_USEPYTHON) && !InitPython() )
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
      tmp = READ_CONFIG(m_profile, MC_MBOXDIR);
      if( strutil_isempty(tmp) )
      {
         tmp = m_localDir;
         m_profile->writeEntry(MC_MBOXDIR, tmp);
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
