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
#  include   "MPython.h"
#endif   // USE_PCH

#include <errno.h>

#include "FolderView.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "gui/wxMDialogs.h"   // MDialog_YesNoDialog

#include "adb/AdbManager.h"   // for AdbManager::Delete

#include "Mversion.h"
#include "Mupgrade.h"

#include <wx/confbase.h>      // wxExpandEnvVars
#include <wx/mimetype.h>      // wxMimeTypesManager

#include <wx/persctrl.h>      // for wxPControls::SetSettingsPath

#ifdef OS_UNIX
#  include  <unistd.h>
#  include  <sys/stat.h>
#endif //Unix

// instead of writing our own wrapper for wxExecute()
#include   <wx/utils.h>
#define   SYSTEM(command) wxExecute(command, FALSE)

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
   m_eventReg = NULL;
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
      wxGetUserId(buffer,bufsize); // contains , delimited fields of info
      str = strutil_before(str,','); 
      m_profile->writeEntry(MP_USERNAME, buffer);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_PERSONALNAME)) )
   {
      wxGetUserName(buffer,bufsize);
      m_profile->writeEntry(MP_PERSONALNAME, buffer);
   }

#if defined(OS_UNIX)
   if( strutil_isempty(READ_APPCONFIG(MP_USERDIR)) )
   {
      wxString strHome;
      //FIXME wxGetHomeDir(&strHome);
      strHome = getenv("HOME");
      strHome << DIR_SEPARATOR << READ_APPCONFIG(MP_USER_MDIR);
      m_profile->writeEntry(MP_USERDIR, strHome);
   }
   m_localDir = wxExpandEnvVars(READ_APPCONFIG(MP_USERDIR));
#elif defined(OS_WIN)
   m_localDir = wxExpandEnvVars(READ_APPCONFIG(MP_USERDIR));
   if ( m_localDir.IsEmpty() )
   {
      // take the directory of the program
      char szFileName[MAX_PATH];
      if ( !GetModuleFileName(NULL, szFileName, WXSIZEOF(szFileName)) )
      {
         wxLogError(_("Can't find your M directory, please specify it "
                      "in the options dialog."));
      }
      else
      {
         wxSplitPath(szFileName, &m_localDir, NULL, NULL);
         m_profile->writeEntry(MP_USERDIR, m_localDir);
      }
   }
#else
   #error "Don't know how to find local dir under this OS"
#endif // OS

   // now that we have the local dir, we can set up a default mail
   // folder dir
   str = READ_APPCONFIG(MP_MBOXDIR);
   if( strutil_isempty(str) )
   {
      str = m_localDir;
      m_profile->writeEntry(MP_MBOXDIR, str);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_HOSTNAME)) )
   {
      wxGetHostName(buffer,bufsize);
      m_profile->writeEntry(MP_HOSTNAME, buffer);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_NNTPHOST)) )
   {
      char *cptr = getenv("NNTPSERVER");
      if(!cptr || !*cptr)
	cptr = MP_NNTPHOST_FB;
      m_profile->writeEntry(MP_NNTPHOST, cptr);
   }

   if( strutil_isempty(READ_APPCONFIG(MP_SMTPHOST)) )
   {
      char *cptr = getenv("SMTPSERVER");
      if(!cptr || !*cptr)
	cptr = MP_SMTPHOST_FB;
      m_profile->writeEntry(MP_SMTPHOST, cptr);
   }

   bool bFirstRun = READ_APPCONFIG(MP_FIRSTRUN) != 0;
   if ( bFirstRun )
   {
      // do we need to upgrade something?
      String version = m_profile->readEntry(MP_VERSION, "");
      if ( version != M_VERSION ) {
         if ( ! Upgrade(version) )
            return false;

         if(! SetupInitialConfig())
            ERRORMESSAGE((_("Failed to set up initial configuration.")));

         // next time won't be the first one any more
         m_profile->writeEntry(MP_FIRSTRUN, 0);

         // write the new version
         m_profile->writeEntry(MP_VERSION, M_VERSION);
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
         wxLogError(_("Can't create the directory for configuration "
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
      PathFinder pf(MP_PREFIXPATH_D);
      pf.AddPaths(M_PREFIX,false,true);
      m_globalDir = pf.FindDir(MP_ROOTDIRNAME_D, &found);
      if(!found)
      {
         String msg;
         msg.Printf(_("Cannot find global directory \"%s\" in\n"
                      "\"%s\"\n"
                      "Would you like to specify its location now?"),
                      MP_ROOTDIRNAME_D, MP_ETCPATH_D);
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


   // show the splash screen (do it as soon as we have profile to read
   // MP_SHOWSPLASH from)
   if ( READ_APPCONFIG(MP_SHOWSPLASH) )
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
   if ( READ_CONFIG(m_profile, MP_USEPYTHON) && !InitPython() )
   {
      // otherwise it would hide our message box
      CloseSplash();

      static const char *msg =
       "Detected a possible problem with your Python installation.\n"
       "A properly installed Python system is required for using\n"
       "M's scripting capabilities. Some minor functionality might\n"
       "be missing without it, however the core functions will be\n"
       "unaffected.\n"
       "Would you like to disable Python support for now?\n"
       " (You can set " MP_USEPYTHON " to 1 to re-enable it later)";
      if ( MDialog_YesNoDialog(_(msg)) )
      {
         // disable it
         m_profile->writeEntry(MP_USEPYTHON, FALSE);
      }
   }
#  endif //USE_PYTHON

   // open all default mailboxes
   // --------------------------
   char *folders = strutil_strdup(READ_APPCONFIG(MP_OPENFOLDERS));
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

   // register with the event subsystem
   // ---------------------------------
   m_eventReg = EventManager::Register(*this, EventId_NewMail);

   // should never fail...
   CHECK( m_eventReg, FALSE,
          "failed to register event handler for new mail event " );

   return TRUE;
}

void
MAppBase::OnAbnormalTermination()
{
}

void
MAppBase::OnShutDown()
{
   // don't want events any more
   if ( m_eventReg )
   {
      EventManager::Unregister(m_eventReg);
      m_eventReg = NULL;
   }

   // clean up
   AdbManager::Delete();
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
   if(m_topLevelFrame)
      m_topLevelFrame->Close(force);
   m_topLevelFrame = NULL;
}

bool
MAppBase::OnEvent(EventData& event)
{
   // we're only registered for new mail events
   CHECK( event.GetId() == EventId_NewMail, TRUE, "unexpected event" );

   // get the folder in which the new mail arrived
   EventMailData& mailevent = (EventMailData &)event;
   MailFolder *folder = mailevent.GetFolder();

   // step 1: execute external command if it's configured
   String command = READ_CONFIG(folder->GetProfile(), MP_NEWMAILCOMMAND);
   if(! command.IsEmpty())
   {
      if ( ! SYSTEM(command) )
      {
         // TODO ask whether the user wants to disable it
         wxLogError(_("Command '%s' (to execute on new mail reception)"
                      " failed."), command.c_str());
      }
   }

#ifdef   USE_PYTHON
   // step 2: folder specific Python callback
   if(! PythonCallback(MCB_FOLDER_NEWMAIL, 0, folder, folder->GetClassName(),
                       folder->GetProfile()))

   // step 3: global python callback
   if(! PythonCallback(MCB_MAPPLICATION_NEWMAIL, 0, this, "MApplication",
                       GetProfile()))
#endif //USE_PYTHON
   {
      if(READ_CONFIG(GetProfile(), MP_SHOW_NEWMAILMSG))
      {
         String message;

         unsigned long number = mailevent.GetNumberOfMessages();
         unsigned i;
         if ( number <= (unsigned long) READ_CONFIG(GetProfile(),
                                                    MP_SHOW_NEWMAILINFO)) 
         {
            for(i = 0; i < number; i++)
            {
               Message *msg =
                  folder->GetMessage(mailevent.GetNewMessageIndex(i));
               if ( msg )
               {
                  message << _("Subject: ") << msg->Subject() << '\n'
                          << _("From: ") << msg->From() << '\n'
                          << _("in folder '") << folder->GetName() << "'\n\n";
                  msg->DecRef();
               }
               else
                  FAIL_MSG("new mail received but no new message?");
            }
         }
         else
         {
            // it seems like a better idea to give this brief message in case
            // of several messages
            message.Printf(_("You have received %lu new messages."), number);
         }

         // TODO make it a wxPMessageBox to let the user shut if off from here?
         MDialog_Message(message, m_topLevelFrame, _("New Mail"));
      }
   }

   return TRUE;
}
