/*-*- c++ -*-********************************************************
 * MAppBase class: all non GUI specific application stuff           *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
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
#  include "Mcommon.h"
#  include "strutil.h"
#  include "Profile.h"
#  include "kbList.h"
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "Mpers.h"

#  include  <wx/dynarray.h>
#endif   // USE_PCH

#include <errno.h>

#include "MPython.h"

#include "MFolder.h"
#include "FolderView.h"       // for OpenFolderViewFrame()
#include "MailFolder.h"
#include "HeaderInfo.h"
#include "MailCollector.h"

#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "MDialogs.h"         // MDialog_YesNoDialog
#include "gui/wxIconManager.h"
#include "adb/AdbManager.h"   // for AdbManager::Delete

#include "adb/AdbFrame.h"     // for ShowAdbFrame

#include "Mversion.h"
#include "Mupgrade.h"

#include "MFCache.h"          // for MfStatusCache::CleanUp

#include <wx/confbase.h>      // wxExpandEnvVars
#include <wx/mimetype.h>      // wxMimeTypesManager

#include <wx/dirdlg.h>        // wxDirDialog

#include "wx/persctrl.h"      // for wxPControls::SetSettingsPath

#ifdef OS_UNIX
#  include <unistd.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#endif //Unix

// instead of writing our own wrapper for wxExecute()
#include  <wx/utils.h>
#define   SYSTEM(command) wxExecute(command, FALSE)

// VZ: Karsten, if this is really not used any more, please remove all code
//     inside USE_ICON_SUBDIRS
#ifdef USE_ICON_SUBDIRS

// to map icon subdirs to numbers
static const char *gs_IconSubDirs[] =
{
   "default", "GNOME", "KDE", "small"
};

#endif // USE_ICON_SUBDIRS

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(const wxMFrame *, ArrayFrames);

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

/** Each entry of this class keeps one mail folder open all the time
    it exists. */
class MailFolderEntry
{
public:
   MailFolderEntry(const String name)
      {
         m_name = name;
         m_folder = MailFolder::OpenFolder(m_name);
      }
   ~MailFolderEntry()
      {
         if(m_folder) m_folder->DecRef();
      }
   MailFolder *GetMailFolder(void) const { return m_folder; }
   const String GetName(void) const { return m_name; }
private:
   String      m_name;
   MailFolder *m_folder;

};

KBLIST_DEFINE(MailFolderList, MailFolderEntry);

// ----------------------------------------------------------------------------
// MAppBase - the class which defines the "application object" interface
// ----------------------------------------------------------------------------

MAppBase::MAppBase()
{
   m_eventNewMailReg = NULL;
   m_eventOptChangeReg = NULL;
   m_eventFolderUpdateReg = NULL;

   m_topLevelFrame = NULL;
   m_framesOkToClose = NULL;
   m_MailCollector = NULL;
   m_profile = NULL;
   m_DialupSupport = FALSE;

   m_mimeManager = NULL;

   m_cycle = Initializing;

   m_isAway =
   m_autoAwayOn = FALSE;

   ResetLastError();
}

MAppBase::~MAppBase()
{
   delete m_framesOkToClose;
}

/* The code in VerifySettings somehow overlaps with the purpose of
   upgrade.cpp where we set up the INBOX profile and possibly other
   things. I just can't be bothered now, but this should go there,
   too.
*/
bool
MAppBase::VerifySettings(void)
{
   return CheckConfiguration();
}

bool
MAppBase::OnStartup()
{
   mApplication = this;

#ifdef OS_UNIX
   // First, check our user ID: mahogany does not like being run as root.
   if(geteuid() == 0)
   {
      if(! MDialog_YesNoDialog(
         _("You have started Mahogany as the super-user (root).\n"
           "For security reasons this is not a good idea.\n\n"
           "Are you sure you want to continue?"),
         NULL,_("Run as root?"), FALSE,
         GetPersMsgBoxName(M_MSGBOX_ASK_RUNASROOT)))
         return false;
   }
#endif // Unix

   // initialise the profile(s)
   // -------------------------

   String strConfFile;
#ifdef OS_UNIX
   strConfFile = getenv("HOME");
   strConfFile << "/." << M_APPLICATIONNAME;
   // FIXME must create the directory ourselves!
   struct stat st;
   if ( stat(strConfFile, &st) != 0 || !S_ISDIR(st.st_mode) )
   {
      if ( mkdir(strConfFile, 0700) != 0 )
      {
         wxLogError(_("Cannot create the directory for configuration "
                      "files '%s'."), strConfFile.c_str());
         return FALSE;
      }
      else
         wxLogInfo(_("Created directory '%s' for configuration files."),
                   strConfFile.c_str());


      //  Also, create an empty config file with the right
      //  permissions:
      String realFile = strConfFile + "/config";
      int fd = creat(realFile, S_IRUSR|S_IWUSR);
      if(fd == -1)
      {
         wxLogError(_("Could not create initial empty configuration file\n"
                      "'%s'\n%s"),
                    realFile.c_str(), strerror(errno));
      }
      else
         close(fd);
   }
   /* Check whether other users can write our config dir. This must
      not be allowed! */
   if(stat(strConfFile, &st) == 0) // else shouldn't happen, we just created it
      if( (st.st_mode & (S_IWGRP | S_IWOTH)) != 0)
      {
         // No other user must have write access to the config dir.
         String
            msg;
         msg.Printf(_("Configuration directory '%s' was writable for other users.\n"
                      "Passwords may have been compromised, please consider changing them.\n"),
                    strConfFile.c_str());
         if(chmod(strConfFile, st.st_mode^(S_IWGRP|S_IWOTH)) == 0)
            msg += _("This has been fixed now, the directory is no longer writable for others.");
         else
         {
            String tmp;
            tmp.Printf(_("The attempt to make the directory unwritable for others has failed.\n(%s)"),
                       strerror(errno));
            msg << tmp;
         }
         wxLogError(msg);
      }

   strConfFile += "/config";
#else  // Windows
   strConfFile = M_APPLICATIONNAME;
#endif // Win/Unix

   m_profile = Profile::CreateGlobalConfig(strConfFile);

   // disable the use of environment variables if configured like this (this
   // speeds up things relatively significantly under Windows - and as few
   // people use evironment variables there, it is disabled for Windows by
   // default)
   m_profile->SetExpandEnvVars(READ_CONFIG(m_profile, MP_EXPAND_ENV_VARS) != 0);

#ifdef OS_UNIX
   /* Check whether other users can read our config file. This must
      not be allowed! */
   if(stat(strConfFile, &st) == 0)
      if( (st.st_mode & (S_IRGRP | S_IROTH)) != 0)
      {
         // No other user must have access to the config file.
         String
            msg;
         msg.Printf(_("Configuration file '%s' was readable for other users.\n"
                      "Passwords may have been compromised, please consider changing them.\n"),
                    strConfFile.c_str());
         if(chmod(strConfFile, S_IRUSR|S_IWUSR) == 0)
            msg += _("This has been fixed now, the file is no longer readable for others.");
         else
         {
            String tmp;
            tmp.Printf(_("The attempt to make the file unreadable for others has failed.\n(%s)"),
                       strerror(errno));
            msg << tmp;
         }
         wxLogError(msg);
      }
#endif // OS_UNIX

#ifdef DEBUG
   // enable tracing of the specified kinds of messages
   wxArrayString
      masks = strutil_restore_array(':', m_profile->readEntry("DebugTrace", ""));
   size_t nMasks = masks.GetCount();
   for ( size_t nMask = 0; nMask < nMasks; nMask++ )
   {
      wxLog::AddTraceMask(masks[nMask]);
   }

#endif // DEBUG

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

   InitGlobalDir();

   String tmp;
#ifdef USE_ICON_SUBDIRS
   // KB: we no longer use different subdirs:
   // We need to set this before wxWindows has a chance to process
   // idle events (splash screen), in case there was any problem with
   // the config file, or it will show the unknown icon.
   {
      unsigned long idx = (unsigned long) READ_APPCONFIG(MP_ICONSTYLE);
      if(idx < sizeof gs_IconSubDirs)
         GetIconManager()->SetSubDirectory(gs_IconSubDirs[idx]);
   }
#endif
   // cause it to re-read our global dir
   GetIconManager()->SetSubDirectory("");

   // do it first to avoid any interactive stuff from popping up if configured
   // to start up in the unattended mode
   SetAwayMode(READ_APPCONFIG(MP_AWAY_STATUS) != 0);

   // show the splash screen (do it as soon as we have profile to read
   // MP_SHOWSPLASH from) unless this is our first run in which case it will
   // disappear any how - not showing it avoids some ugly flicker on screen
   if ( !READ_APPCONFIG(MP_FIRSTRUN) && READ_APPCONFIG(MP_SHOWSPLASH) )
   {
      // no parent because no frames created yet
      MDialog_AboutDialog(NULL);
   }

   // verify (and upgrade if needed) our settings
   // -------------------------------------------
   if ( !VerifySettings() )
   {
      ERRORMESSAGE((_("Program execution aborted.")));

      return false;
   }

#ifdef OS_WIN
   // cclient extra initialization under Windows
   wxString strHome;
   mail_parameters((MAILSTREAM *)NULL, SET_HOMEDIR, (void *)wxGetHomeDir(&strHome));
#endif // OS_WIN

   // Initialise mailcap/mime.types managing subsystem.
   m_mimeManager = new wxMimeTypesManager();

#if 0 // we don't provide any mailcaps so far
   // attempt to load the extra information supplied with M:

   if(wxFileExists(GetGlobalDir()+"/mailcap"))
      m_mimeManager->ReadMailcap(GetGlobalDir()+"/mailcap");
   if(wxFileExists(GetGlobalDir()+"/mime.types"))
      m_mimeManager->ReadMimeTypes(GetGlobalDir()+"/mime.types");
#endif // 0

   // must be done before using the network
   SetupOnlineManager();

   // extend path for commands, look in M's dirs first
   tmp = "PATH=";
   tmp << GetLocalDir() << "/scripts" << PATH_SEPARATOR
       << GetDataDir() << "/scripts";

   const char *path = getenv("PATH");
   if ( path )
      tmp << PATH_SEPARATOR << path;

   // on some systems putenv() takes "char *", cast silents the warnings but
   // should be harmless otherwise
   putenv((char *)tmp.c_str());

   // initialise python interpreter
#  ifdef  USE_PYTHON
   // having the same error message each time M is started is annoying, so
   // give the user a possibility to disable it
   if ( READ_CONFIG(m_profile, MP_USEPYTHON) && ! InitPython() )
   {
      static const char *msg =
       "Detected a possible problem with your Python installation.\n"
       "A properly installed Python system is required for using\n"
       "M's scripting capabilities. Some minor functionality might\n"
       "be missing without it, however the core functions will be\n"
       "unaffected.\n"
       "Would you like to disable Python support for now?\n"
       "(You can re-enable it later from the options dialog)";
      if ( MDialog_YesNoDialog(_(msg)) )
      {
         // disable it
         m_profile->writeEntry(MP_USEPYTHON, FALSE);
      }
   }
#  endif //USE_PYTHON

   // load any modules requested: notice that this must be done as soon as
   // possible as filters module is already used by the folder opening code
   // below
   LoadModules();

   // create and show the main program window
   if ( !CreateTopLevelFrame() )
   {
      wxLogError(_("Failed to create the program window."));

      return false;
   }

   // now we can create the log window (the child of the main frame)
   if ( READ_APPCONFIG(MP_SHOWLOG) )
   {
      ShowLog();
   }

   // also start file logging if configured
   SetLogFile(READ_APPCONFIG(MP_LOGFILE));

   // now we have finished the vital initialization and so can assume
   // everything mostly works
   m_cycle = Running;

   // and as we have the main window, we can initialize the modules which
   // use it
   InitModules();

   // register with the event subsystem
   // ---------------------------------

   m_eventNewMailReg = MEventManager::Register(*this, MEventId_NewMail);

   // should never fail...
   CHECK( m_eventNewMailReg, FALSE,
          "failed to register event handler for new mail event " );
   m_eventOptChangeReg = MEventManager::Register(*this,
                                                 MEventId_OptionsChange);
   CHECK( m_eventOptChangeReg, FALSE,
          "failed to register event handler for options change event " );
   m_eventFolderUpdateReg = MEventManager::Register(*this,
                                                    MEventId_FolderUpdate);
   CHECK( m_eventFolderUpdateReg, FALSE,
          "failed to register event handler for folder status event " );

   // open all folders we open initially
   // ----------------------------------

   // open the remembered folder in the main frame unless disabled
   if ( !READ_APPCONFIG(MP_DONTOPENSTARTUP) )
   {
      String foldername = READ_APPCONFIG(MP_MAINFOLDER);
      if ( !foldername.IsEmpty() )
      {
         MFolder *folder = MFolder::Get(foldername);
         if ( folder )
         {
            // make sure it doesn't go away after OpenFolder()
            folder->IncRef();
            ((wxMainFrame *)m_topLevelFrame)->OpenFolder(folder);
            folder->DecRef();
         }
         else // huh?
         {
            wxLogWarning(_("Failed to reopen folder '%s', it doesn't seem "
                           "to exist any more."), foldername.c_str());
         }
      }
   }

   // update status of outbox once:
   UpdateOutboxStatus();

   // open all default mailboxes
   // --------------------------

   if ( !READ_APPCONFIG(MP_DONTOPENSTARTUP) )
   {
      char *folders = strutil_strdup(READ_APPCONFIG(MP_OPENFOLDERS));
      kbStringList openFoldersList;
      strutil_tokenise(folders,";",openFoldersList);
      delete [] folders;

      bool ok = true;

      kbStringList::iterator i;
      for(i = openFoldersList.begin(); i != openFoldersList.end(); i++)
      {
         String *name = *i;

         if ( name->empty() )
         {
            FAIL_MSG( "empty folder name in the list of folders to open?" );
            continue;
         }

         MFolder_obj folder(*name);
         if ( folder.IsOk() )
         {
            if ( !OpenFolderViewFrame(folder, m_topLevelFrame) )
            {
               // error message must be already given by OpenFolderViewFrame
               ok = false;
            }
         }
         else
         {
            wxLogWarning(_("Failed to reopen folder '%s', it doesn't seem "
                           "to exist any more."), name->c_str());

            ok = false;
         }
      }

      if ( !ok )
      {
         wxLogWarning(_("Not all folders could be reopened."));
      }
   }

   // initialise collector object for incoming mails
   // ----------------------------------------------

   // only do it if we are using the NewMail folder at all
   m_MailCollector = MailCollector::Create();
   if( READ_APPCONFIG(MP_COLLECTATSTARTUP) != 0)
      m_MailCollector->Collect();

   // show the ADB editor if it had been shown the last time when we ran
   // ------------------------------------------------------------------

   if ( READ_APPCONFIG(MP_SHOWADBEDITOR) )
   {
      ShowAdbFrame(TopLevelFrame());
   }

   // cache the auto away flag as it will be checked often in UpdateAwayMode
   m_autoAwayOn = READ_APPCONFIG(MP_AWAY_AUTO_ENTER) != 0;

   return TRUE;
}

void
MAppBase::OnAbnormalTermination()
{
   // no more event processing as it may lead to unexpected results in the
   // state we are in
   MEventManager::Suspend();

   m_cycle = ShuttingDown;

   // maybe we could try calling OnShutDown()?
}

void
MAppBase::OnShutDown()
{
   m_cycle = ShuttingDown;

   // do we need to save the away mode state?
   if ( READ_APPCONFIG(MP_AWAY_REMEMBER) )
   {
      m_profile->writeEntry(MP_AWAY_STATUS, IsInAwayMode());
   }

   // Try to store our remotely synchronised configuration settings
   if(! SaveRemoteConfigSettings() )
      wxLogError(_("Synchronised configuration information could not "
                   "be stored remotely."));

   // don't want events any more
   if ( m_eventNewMailReg )
   {
      MEventManager::Deregister(m_eventNewMailReg);
      m_eventNewMailReg = NULL;
   }
   if ( m_eventOptChangeReg )
   {
      MEventManager::Deregister(m_eventOptChangeReg);
      m_eventOptChangeReg = NULL;
   }
   if ( m_eventFolderUpdateReg )
   {
      MEventManager::Deregister(m_eventFolderUpdateReg);
      m_eventFolderUpdateReg = NULL;
   }

   if (m_MailCollector)
   {
      m_MailCollector->DecRef();
      m_MailCollector = NULL;
   }

   // clean up
   MEventManager::DispatchPending();
   AdbManager::Delete();
   Profile::FlushAll();

   // The following little hack allows us to decref and delete the
   // global profile without triggering an assert, as this is not
   // normally allowed.
   Profile *p = m_profile;
   m_profile = NULL;
   p->DecRef();

   // misc cleanup
   delete m_mimeManager;

   MailFolder::CleanUp();
   MfStatusCache::CleanUp();

   // there might have been events queued, get rid of them
   //
   // FIXME: there should be no events by now, is this really needed? (VZ)
   MEventManager::DispatchPending();

   // suppress memory leak reports in debug mode - it is not a real memory
   // leak as memory is allocated only once but still
#if defined(OS_WIN) && defined(DEBUG)
   // clean up after cclient (order is important as sysinbox() uses the
   // username)
   free(sysinbox());
   free(myusername_full(NULL));
#endif // OS_WIN

#ifdef USE_PYTHON_DYNAMIC
   // free python DLL: this is ok to call even if it wasn't loaded
   FreePythonDll();
#endif // USE_PYTHON_DYNAMIC
}

bool
MAppBase::CanClose() const
{
   // Try to send outgoing messages:
   if( CheckOutbox() )
   {
      if(MDialog_YesNoDialog(
         _("You still have messages queued to be sent.\n"
           "Do you want to send them before exiting the application?"),
         NULL, MDIALOG_YESNOTITLE,
                             TRUE, "SendOutboxOnExit"))
         SendOutbox();
   }

   // Check Trash folder if it contains any messages (only the global
   // Trash can be checked here!):
   if ( READ_APPCONFIG(MP_USE_TRASH_FOLDER) )
   {
      String trashName = READ_APPCONFIG(MP_TRASH_FOLDER);

      // don´t ask twice
      if ( trashName != ((wxMainFrame *)m_topLevelFrame)->GetFolderName() )
      {
         ASMailFolder *mf = ASMailFolder::OpenFolder(trashName);
         if( mf )
         {
            // make sure they are all marked as deleted:
            UIdArray * ia = GetAllMessagesSequence(mf);
            mf->SetSequenceFlag(ia, MailFolder::MSG_STAT_DELETED);
            delete ia;
            // And now, we can ask if we want to expunge them:
            CheckExpungeDialog(mf);
            mf->DecRef();
         }
      }
   }

   String folders;
   if(! MailFolder::CanExit(&folders) )
   {
      String msg = _("Some folders are in critical sections and should be allowed\n"
                     "to finish whatever operation is pending on them before exiting\n"
                     "the application.\n"
                     "These folders are:\n");
      msg += folders;
      msg += _("Do you want to exit anyway?");
      return MDialog_YesNoDialog(msg, NULL, MDIALOG_YESNOTITLE,
                                 FALSE /* no=default */,
                                 "AbandonCriticalFolders");
   }

   return true;
}

void
MAppBase::AddToFramesOkToClose(const wxMFrame *frame)
{
   if ( !m_framesOkToClose )
      m_framesOkToClose = new ArrayFrames;

   m_framesOkToClose->Add(frame);
}

bool
MAppBase::IsOkToClose(const wxMFrame *frame) const
{
   return m_framesOkToClose && m_framesOkToClose->Index(frame) != wxNOT_FOUND;
}

void
MAppBase::Exit(bool ask)
{
   // in case it's still opened...
   CloseSplash();

   if ( !ask || CanClose() )
   {
      if ( READ_APPCONFIG(MP_REOPENLASTFOLDER) )
      {
         // reset the list of folders to be reopened, it will be recreated in
         // MEventId_AppExit handlers
         GetProfile()->writeEntry(MP_OPENFOLDERS, "");
      }

      // send an event telling everybody we're closing: note that this can't
      // be blocked, it is just a notification
      MEventManager::Send(new MEventData(MEventId_AppExit));
      MEventManager::DispatchPending();

      // this will close all our window and thus terminate the application
      DoExit();
   }
   else
   {
      // when we will try to close the next time, we shouldn't assume that
      // these frames still don't mind being closed - may be the user will
      // modify the compose view contents or something else changes
      m_framesOkToClose->Empty();
   }
}

void MAppBase::UpdateAwayMode()
{
   if ( IsInAwayMode() )
   {
      // in away mode - should we exit from it?
      if ( READ_APPCONFIG(MP_AWAY_AUTO_EXIT) )
      {
         SetAwayMode(false);
      }
   }
   else // not in away mode - reset the timer if necessary
   {
      if ( m_autoAwayOn )
      {
         RestartTimer(Timer_Away);
      }
   }
}

bool
MAppBase::OnMEvent(MEventData& event)
{
   if(event.GetId() == MEventId_NewMail)
   {
      // first of all, do nothing at all in the away mode
      if ( IsInAwayMode() )
      {
         // avoid any interaction with the user
         return TRUE;
      }

      // get the folder in which the new mail arrived
      MEventNewMailData& mailevent = (MEventNewMailData &)event;
      MailFolder *folder = mailevent.GetFolder();

      // step 1: execute external command if it's configured
      Profile *profile = folder->GetProfile();
      if ( READ_CONFIG(profile, MP_USE_NEWMAILCOMMAND) )
      {
         String command = READ_CONFIG(profile, MP_NEWMAILCOMMAND);
         if(! command.IsEmpty())
         {
            if ( ! SYSTEM(command) )
            {
               // TODO ask whether the user wants to disable it
               wxLogError(_("Command '%s' (to execute on new mail reception)"
                            " failed."), command.c_str());
            }
         }
      }

#ifdef   USE_PYTHON
      // step 2: folder specific Python callback
      if(! PythonCallback(MCB_FOLDER_NEWMAIL, 0, folder, folder->GetClassName(),
                          profile))

         // step 3: global python callback
         if(! PythonCallback(MCB_MAPPLICATION_NEWMAIL, 0, this, "MApplication",
                             GetProfile()))
#endif //USE_PYTHON
         {
            if(READ_CONFIG(profile, MP_SHOW_NEWMAILMSG))
            {
               String message;

               unsigned long number = mailevent.GetNumber();
               unsigned long found = 0;
               if ( number <= (unsigned long) READ_CONFIG(profile,
                                                          MP_SHOW_NEWMAILINFO))
               {
                  message = _("You have received new mail:");
                  for( unsigned long i = 0; i < number; i++)
                  {
                     Message *msg =
                        folder->GetMessage(mailevent.GetNewMessageIndex(i));
                     if ( msg )
                     {
                        message << '\n'
                                << _("\tIn folder '") << folder->GetName() << "'\n"
                                << _("\tFrom: '") << msg->From()
                                << _("' with subject: ") << msg->Subject();

                        msg->DecRef();
                        found++;
                     }
                     else
                     {
                        FAIL_MSG("new mail received but no new message?");
                     }
                  }
               }
               else // too many new messages
               {
                  // it seems like a better idea to give this brief message in case
                  // of several messages
                  message.Printf(_("You have received %lu new messages "
                                   "in folder '%s'."),
                                 number, folder->GetName().c_str());
                  found = number;
               }

               // This test is there as the messages might have gone in the
               // meantime, as happens when filtering.
               if ( found > 0 )
                  LOGMESSAGE((M_LOG_WINONLY, message));
            }
         }
   }
   else if (event.GetId() == MEventId_OptionsChange)
   {
      SetupOnlineManager(); // make options change effective
#ifdef USE_ICON_SUBDIRS
     {
         unsigned long idx = (unsigned long)  READ_APPCONFIG(MP_ICONSTYLE);
         if(idx < sizeof gs_IconSubDirs)
            GetIconManager()->SetSubDirectory(gs_IconSubDirs[idx]);
     }
#endif
     // re-generate the mailcollector object
     m_MailCollector->RequestReInit();
   }
   else if (event.GetId() == MEventId_FolderUpdate)
   {
      MEventFolderUpdateData& ev = (MEventFolderUpdateData &)event;
      MailFolder *folder = ev.GetFolder();
      String folderName = folder->GetName();
      String outbox = READ_APPCONFIG(MP_OUTBOX_NAME);
      if(folderName.CmpNoCase(outbox) == 0
         && ! folder->IsLocked()) // simple recursion check
      {
         UpdateOutboxStatus(folder);
      }
   }
   else
   {
      FAIL_MSG("unexpected event in MAppBase");
   }

   return TRUE;
}

void
MAppBase::InitGlobalDir()
{
   m_globalDir = READ_APPCONFIG(MP_GLOBALDIR);
   if ( m_globalDir.empty() || !PathFinder::IsDir(m_globalDir) )
   {
      // under Unix we try to find our directory in some standard locations
#ifdef OS_UNIX
      bool found;
      if ( PathFinder::IsDir(M_BASEDIR) )
      {
         m_globalDir = M_BASEDIR;

         found = true;
      }
      else
      {
         PathFinder pf(MP_PREFIXPATH_D);
         pf.AddPaths(M_PREFIX,false,true);
         m_globalDir = pf.FindDir(MP_ROOTDIRNAME_D, &found);
      }

      if ( !found )
      {
         String msg;
         msg.Printf(_("Cannot find global directory \"%s\" in\n"
                      "\"%s\"\n"
                      "Would you like to specify its location now?"),
                    MP_ROOTDIRNAME_D, MP_ETCPATH_D);
#else
         String msg = _("Cannot find global program directory.\n"
                        "\n"
                        "Would you like to specify its location now?");
#endif // OS_UNIX
         if ( MDialog_YesNoDialog(msg, NULL, MDIALOG_YESNOTITLE,
                                  TRUE /* yes default */, "AskSpecifyDir") )
         {
            wxDirDialog dlg(NULL, _("Specify global directory for Mahogany"));
            if ( dlg.ShowModal() )
            {
               m_globalDir = dlg.GetPath();
            }
         }
#ifdef OS_UNIX
      }
#endif // OS_UNIX

      if ( !m_globalDir.empty() )
      {
         m_profile->writeEntry(MP_GLOBALDIR, m_globalDir);
      }
   }
}

/// Send all messages from the outbox
void
MAppBase::SendOutbox(void) const
{
   if ( READ_APPCONFIG(MP_USE_OUTBOX) )
   {
      STATUSMESSAGE((_("Checking for queued messages...")));
      // get name of SMTP outbox:
      String outbox = READ_APPCONFIG(MP_OUTBOX_NAME);
      SendOutbox(outbox, true);
      UpdateOutboxStatus();
   }
}

// return true if there are messages to be sent
bool MAppBase::CheckOutbox(UIdType *nSMTP, UIdType *nNNTP, MailFolder *mfi) const
{
   if ( !READ_APPCONFIG(MP_USE_OUTBOX) )
      return false;

   String outbox = READ_APPCONFIG(MP_OUTBOX_NAME);

   UIdType
      smtp = 0,
      nntp = 0;

   if(nSMTP) *nSMTP = 0;
   if(nNNTP) *nNNTP = 0;

   MailFolder *mf = NULL;
   if(mfi)
   {
      mf = mfi;
      mf->IncRef();
   }
   else
   {
      mf = MailFolder::OpenFolder(outbox);
      if(mf == NULL)
      {
         String msg;
         msg.Printf(_("Cannot open outbox ´%s´"), outbox.c_str());
         ERRORMESSAGE((msg));
         return FALSE;
      }
   }

   if(mf->CountMessages() > 0)
   {
      HeaderInfoList *hil = mf->GetHeaders();
      if( hil )
      {
         const HeaderInfo *hi;
         Message *msg;
         for(UIdType i = 0; i < hil->Count(); i++)
         {
            hi = (*hil)[i];
            ASSERT(hi);
            msg = mf->GetMessage(hi->GetUId());
            ASSERT(msg);
            String newsgroups;
            msg->GetHeaderLine("Newsgroups", newsgroups);
            if(newsgroups.Length() > 0)
               nntp++;
            else
               smtp++;
            SafeDecRef(msg);
         }
         SafeDecRef(hil);
      }
   }
   mf->DecRef();

   if(nSMTP) *nSMTP = smtp;
   if(nNNTP) *nNNTP = nntp;
   return smtp != 0 || nntp != 0;
}

void
MAppBase::SendOutbox(const String & outbox, bool checkOnline ) const
{
   CHECK_RET( outbox.length(), "missing outbox folder name" );

   Protocol protocol;
   UIdType count = 0;
   MailFolder *mf = MailFolder::OpenFolder(outbox);
   if(! mf)
   {
      String msg;
      msg.Printf(_("Cannot open outbox ´%s´"), outbox.c_str());
      ERRORMESSAGE((msg));
      return;
   }

   if(mf->CountMessages() == 0)
   {  // nothing to do
      mf->DecRef();
      return;
   }

   if(checkOnline && ! IsOnline())
   {
      if ( MDialog_YesNoDialog(
         _("Cannot send queued messages while dialup network is down.\n"
           "Do you want to go online now?"),
         NULL,
         MDIALOG_YESNOTITLE,
         TRUE /* yes default */, "GoOnlineToSendOutbox") )
      {
         STATUSMESSAGE((_("Going online...")));
         GoOnline();
      }
      else
      {
         mf->DecRef();
         return;
      }
   }

   HeaderInfoList *hil = mf->GetHeaders();
   if(! hil)
   {
      mf->DecRef();
      return; // nothing to do
   }

   const HeaderInfo *hi;
   Message *msg;
   for(UIdType i = 0; i < hil->Count(); i++)
   {
      hi = (*hil)[i];
      ASSERT(hi);
      msg = mf->GetMessage(hi->GetUId());
      ASSERT(msg);
      if(msg)
      {
         String msgText;
         String target;
         bool alreadyCounted = false;
         msg->GetHeaderLine("To", target);
         protocol = Prot_Illegal;
         if(target.Length() > 0)
         {
            protocol = READ_APPCONFIG(MP_USE_SENDMAIL) ?
               Prot_Sendmail : Prot_SMTP;
            STATUSMESSAGE(( _("Sending message %lu/%lu: %s"),
                            (unsigned long)(i+1),
                            (unsigned long)(hil->Count()),
                            msg->Subject().c_str()));
            wxYield();
            if(msg->SendOrQueue(protocol, TRUE))
            {
               count++; alreadyCounted = true;
               mf->DeleteMessage(hi->GetUId());
            }
            else
            {
               String msg;
               msg.Printf(_("Cannot send message ´%s´."),
                          hi->GetSubject().c_str());
               ERRORMESSAGE((msg));
            }
         }
         msg->GetHeaderLine("Newsgroups", target);
         protocol = Prot_NNTP; // default
         if(target.Length() > 0)
         {
            protocol = Prot_NNTP;
            STATUSMESSAGE(( _("Posting article %lu/%lu: %s"),
                            (unsigned long)(i+1),
                            (unsigned long)(hil->Count()),
                            msg->Subject().c_str()));
            wxYield();
            if(msg->SendOrQueue(protocol, TRUE))
            {
               if(! alreadyCounted) count++;
               mf->DeleteMessage(hi->GetUId());
            }
            else
            {
               String msg;
               msg.Printf(_("Cannot post article ´%s´."),
                          hi->GetSubject().c_str());
               ERRORMESSAGE((msg));
            }
         }
      }
      //ASSERT(0); //TODO: static SendMessageCC::Send(String)!!!
      SafeDecRef(msg);
      mf->ExpungeMessages();
   }
   SafeDecRef(hil);
   if(count > 0)
   {
      String msg;
      msg.Printf(_("Sent %lu messages from outbox ´%s´."),
                 (unsigned long) count, mf->GetName().c_str());
      STATUSMESSAGE((msg));
   }
   SafeDecRef(mf);
}


int
MAppBase::GetStatusField(enum StatusFields function) const
{
   // re-setting the statusbar causes crashes on wxGTK, keep it with
   // three fields always for now:
   switch(function)
   {
   case SF_STANDARD:
      return 0;
   case SF_ONLINE:
      return 1;
   case SF_OUTBOX:
      return 2;
   default:
      ASSERT(0);
      return 0;
   }
#if 0
   int field = 0;
   if( function > SF_STANDARD)
      field++;
   if(function > SF_ONLINE && m_DialupSupport)
      field++;
   return field;
#endif
}

static bool FatalErrorSemaphore = false;
/// Report a fatal error:
extern "C"
{
   void FatalError(const char *message)
   {
      if(FatalErrorSemaphore)
         abort();
      else
      {
         FatalErrorSemaphore = true;
         mApplication->FatalError(message);
      }
   }
};

String MAppBase::GetDataDir() const
{
   String dir = GetGlobalDir();
   if ( dir.empty() )
   {
      // M_TOP_SOURCEDIR is defined by configure
#ifdef M_TOP_SOURCEDIR
      dir = M_TOP_SOURCEDIR;
#endif

      if ( !dir.empty() )
         dir += '/';

      dir += "src";
   }

   return dir;
}

