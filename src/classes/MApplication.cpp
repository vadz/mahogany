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
#  include "kbList.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "Mcclient.h"         // For env_parameters
#  include "Mdefaults.h"

#  include <wx/dynarray.h>      // for WX_DEFINE_ARRAY
#  include <wx/dirdlg.h>        // wxDirDialog
#endif   // USE_PCH

#include "MFolder.h"
#include "Message.h"
#include "FolderView.h"       // for OpenFolderViewFrame()
#include "MailFolder.h"
#include "HeaderInfo.h"
#include "FolderMonitor.h"
#include "PathFinder.h"       // for PathFinder
#include "Composer.h"         // for RestoreAll()
#include "SendMessage.h"
#include "ConfigSource.h"
#include "MAtExit.h"

#include "InitPython.h"

#include "gui/wxMainFrame.h"
#include "gui/wxMDialogs.h"   // MDialog_YesNoDialog
#include "adb/AdbManager.h"   // for AdbManager::Delete

#include "adb/AdbFrame.h"     // for ShowAdbFrame

#include "Mupgrade.h"

#include "MFCache.h"          // for MfStatusCache::CleanUp

#include "CmdLineOpts.h"

#include <wx/mimetype.h>      // wxMimeTypesManager
#include <wx/confbase.h>        // for wxConfigBase

#include "wx/persctrl.h"      // for wxPControls::SetSettingsPath

#ifdef OS_MAC
   #include <CoreServices/CoreServices.h>
#endif // OS_MAC

#ifdef OS_WIN
   #include <shlobj.h>
   #include <wx/msw/winundef.h>

   #include <wx/dynlib.h>
#endif // OS_WIN

class MOption;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_AWAY_AUTO_ENTER;
extern const MOption MP_AWAY_AUTO_EXIT;
extern const MOption MP_AWAY_REMEMBER;
extern const MOption MP_AWAY_STATUS;
extern const MOption MP_DONTOPENSTARTUP;
extern const MOption MP_EXPAND_ENV_VARS;
extern const MOption MP_FIRSTRUN;
extern const MOption MP_GLOBALDIR;
extern const MOption MP_ICONSTYLE;
extern const MOption MP_LOGFILE;
extern const MOption MP_MAINFOLDER;
extern const MOption MP_OPENFOLDERS;
extern const MOption MP_OUTBOX_NAME;
extern const MOption MP_REOPENLASTFOLDER;
extern const MOption MP_RUNONEONLY;
extern const MOption MP_SHOWADBEDITOR;
extern const MOption MP_SHOWLOG;
extern const MOption MP_SHOWSPLASH;
extern const MOption MP_TRASH_FOLDER;
extern const MOption MP_USEPYTHON;
extern const MOption MP_USE_OUTBOX;
extern const MOption MP_USE_TRASH_FOLDER;
extern const MOption MP_USERDIR;
extern const MOption MP_CREATE_INTERNAL_MESSAGE;

#ifdef OS_UNIX
extern const MOption MP_USE_SENDMAIL;
#endif // OS_UNIX

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_ABANDON_CRITICAL;
extern const MPersMsgBox *M_MSGBOX_ASK_RUNASROOT;
extern const MPersMsgBox *M_MSGBOX_ASK_SPECIFY_DIR;
extern const MPersMsgBox *M_MSGBOX_GO_ONLINE_TO_SEND_OUTBOX;
extern const MPersMsgBox *M_MSGBOX_EMPTY_TRASH_ON_EXIT;
extern const MPersMsgBox *M_MSGBOX_SEND_OUTBOX_ON_EXIT;

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(const wxMFrame *, ArrayFrames);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#if defined(OS_UNIX) && !defined(OS_MAC)
   static const wxChar *MAHOGANY_DATADIR = _T("share/mahogany");
#endif

// ============================================================================
// implementation
// ============================================================================

MAppBase *mApplication = NULL;

// there is no MAtExit.cpp (it wouldn't contain anything but this line) so do
// it here
MRunAtExit *MRunAtExit::ms_first = NULL;

// ----------------------------------------------------------------------------
// MAppBase - the class which defines the "application object" interface
// ----------------------------------------------------------------------------

MAppBase::MAppBase()
{
   m_eventOptChangeReg = NULL;
   m_eventFolderUpdateReg = NULL;

   m_topLevelFrame = NULL;
   m_framesOkToClose = NULL;
   m_FolderMonitor = NULL;
   m_profile = NULL;

#ifdef USE_DIALUP
   m_DialupSupport = FALSE;
#endif // USE_DIALUP

   m_mimeManager = NULL;
   m_statusPanes[0] = SF_ILLEGAL; // will be really initialized later

   m_cycle = Initializing;

   m_isAway =
   m_autoAwayOn = FALSE;

   m_cmdLineOptions = new CmdLineOptions;

   mApplication = this;

   // uninitialized yet
   m_debugMail = -1;

   ResetLastError();
}

MAppBase::~MAppBase()
{
   // if the program startup failed it could be not deleted before
   if ( m_profile )
   {
      m_profile->DecRef();
      m_profile = NULL;
   }

   Profile::DeleteGlobalConfig();

   delete m_framesOkToClose;

   // execute MRunAtExit callbacks
   for ( MRunAtExit *p = MRunAtExit::GetFirst(); p; p = p->GetNext() )
   {
      p->Do();
   }

   MObjectRC::CheckLeaks();
   MObject::CheckLeaks();

   mApplication = NULL;
}

bool
MAppBase::ProcessSendCmdLineOptions(const CmdLineOptions& cmdLineOpts)
{
   Composer *composer;
   if ( !cmdLineOpts.composer.to.empty() )
   {
      composer = Composer::CreateNewMessage();
   }
   else if ( !cmdLineOpts.composer.newsgroups.empty() )
   {
      composer = Composer::CreateNewArticle();
   }
   else
   {
      composer = NULL;
   }

   if ( composer )
   {
      composer->AddRecipients(cmdLineOpts.composer.bcc,
                              Composer::Recipient_Bcc);
      composer->AddRecipients(cmdLineOpts.composer.cc,
                              Composer::Recipient_Cc);
      composer->AddRecipients(cmdLineOpts.composer.newsgroups,
                              Composer::Recipient_Newsgroup);

      // the "to" parameter may be a mailto: URL, pass it through our expansion
      // function first
      String to = cmdLineOpts.composer.to;
      composer->ExpandRecipient(&to);

      composer->AddRecipients(to, Composer::Recipient_To);

      composer->SetSubject(cmdLineOpts.composer.subject);

      composer->InsertText(cmdLineOpts.composer.body);
      composer->ResetDirty();

      // the composer should be in front of everything
      composer->GetFrame()->Raise();
   }

   return composer != NULL;
}

void
MAppBase::ContinueStartup()
{
   // open all windows we open initially
   // ----------------------------------

   // open any interrupted composer windows we may have
   Composer::RestoreAll();

   // open the remembered folder in the main frame unless disabled: note that
   // specifying a folder on the command line still overrides this
   String foldername = m_cmdLineOptions->folder;

   // if an empty folder was explicitly given, don't open anything
   if ( foldername.empty() && !m_cmdLineOptions->useFolder )
   {
      if ( !READ_APPCONFIG(MP_DONTOPENSTARTUP) )
      {
         foldername = READ_APPCONFIG_TEXT(MP_MAINFOLDER);
      }
   }

   if ( !foldername.empty() )
   {
      MFolder *folder = MFolder::Get(foldername);
      if ( folder )
      {
         // make sure it doesn't go away after OpenFolder()
         folder->IncRef();
         ((wxMainFrame *)m_topLevelFrame)->OpenFolder(folder);
         folder->DecRef();
      }
      else // invalid folder name
      {
         wxLogWarning(_("Failed to open folder '%s' in the main window."),
                      foldername.c_str());
      }
   }

   // update status of outbox once:
   UpdateOutboxStatus();

   // open all default mailboxes
   // --------------------------

#ifdef USE_DIALUP
   // must be done before using the network
   SetupOnlineManager();
#endif // USE_DIALUP

   if ( !READ_APPCONFIG(MP_DONTOPENSTARTUP) )
   {
      String foldersToReopen = READ_APPCONFIG(MP_OPENFOLDERS);
      wxChar *folders = strutil_strdup(foldersToReopen);
      kbStringList openFoldersList;
      strutil_tokenise(folders, _T(";"), openFoldersList);
      delete [] folders;

      bool ok = true;

      kbStringList::iterator i;
      for(i = openFoldersList.begin(); i != openFoldersList.end(); i++)
      {
         String *name = *i;

         if ( name->empty() )
         {
            FAIL_MSG( _T("empty folder name in the list of folders to open?") );
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

   // TODO: only do it if we are using the NewMail folder at all?
   m_FolderMonitor = FolderMonitor::Create();

   // also start the mail auto collection timer
   StartTimer(MAppBase::Timer_PollIncoming);

   // show the ADB editor if it had been shown the last time when we ran
   // ------------------------------------------------------------------

   if ( READ_APPCONFIG(MP_SHOWADBEDITOR) )
   {
      ShowAdbFrame(TopLevelFrame());
   }

   // open the composer if requested on command line
   // ----------------------------------------------

   ProcessSendCmdLineOptions(*m_cmdLineOptions);
}

bool
MAppBase::OnStartup()
{
   // initialise the profile(s)
   // -------------------------

   m_profile = Profile::CreateGlobalConfig(m_cmdLineOptions->configFile);

   if ( !m_profile )
   {
      // not much we can do
      return false;
   }

   // import the settings from the given location
   if ( !m_cmdLineOptions->configImport.empty() )
   {
      ConfigSource_obj
         configSrc(ConfigSourceLocal::CreateFile(m_cmdLineOptions->configImport)),
         configDst(ConfigSourceLocal::CreateDefault());

      if ( !ConfigSource::Copy(*configDst, *configSrc) )
      {
         wxLogError(_("Failed to import Mahogany settings from \"%s\"."),
                    m_cmdLineOptions->configImport.c_str());

         // continue nevertheless
      }
      else // imported ok
      {
         // give a message as otherwise the user has no way to know that we did
         // anything at all (this shouldn't be annoying as import is not
         // supposed to be used often)
         wxLogMessage(_("Successfully imported Mahogany settings from \"%s\"."),
                      m_cmdLineOptions->configImport.c_str());
      }
   }

   // disable the use of environment variables if configured like this (this
   // speeds up things relatively significantly under Windows - and as few
   // people use evironment variables there, it is disabled for Windows by
   // default)
   m_profile->SetExpandEnvVars(READ_CONFIG_BOOL(m_profile, MP_EXPAND_ENV_VARS));

#ifdef DEBUG
   // enable tracing of the specified kinds of messages
   wxArrayString
      masks = strutil_restore_array(m_profile->readEntry(_T("DebugTrace"), _T("")));
   size_t nMasks = masks.GetCount();
   for ( size_t nMask = 0; nMask < nMasks; nMask++ )
   {
      wxLog::AddTraceMask(masks[nMask]);
   }
#endif // DEBUG

   // set the mail debugging flag: as it soon will be possible to open mail
   // folders and it should be set before IsMailDebuggingEnabled() can be
   // called
   m_debugMail = m_cmdLineOptions->debugMail;

   // safe mode disables remote calls
   if ( !m_cmdLineOptions->safe )
   {
      // before doing anything else, if the corresponding option is set, check
      // if another program instance is not already running
      if ( READ_APPCONFIG_BOOL(MP_RUNONEONLY) )
      {
         if ( IsAnotherRunning() )
         {
            // try to defer to it
            if ( CallAnother() )
            {
               // succeeded, nothing more to do in this process

               // set the error to indicate that our false return code wasn't, in
               // fact, an error
               SetLastError(M_ERROR_CANCEL);

               return false;
            }
         }

         // set up a server to listen for the remote calls
         if ( !SetupRemoteCallServer() )
         {
            wxLogError(_("Communication between different Mahogany processes "
                         "won't work, please disable the \"Always run only "
                         "one instance\" option if you don't use it to avoid "
                         "this error message in the future."));
         }
      }
   }

   // NB: although this shouldn't normally be here (it's GUI-dependent code),
   //     it's really impossible to put it into wxMApp because some dialogs
   //     can be already shown from here and this initialization must be done
   //     before.

   // for the persistent controls to work (wx/persctrl.h) we must have
   // a global wxConfig object
   wxConfigBase::Set(m_profile->GetConfig());

   // also set the path for persistent controls to save their state to
   wxPControls::SetSettingsPath(_T("/Settings/"));

#ifdef OS_UNIX
   // now check our user ID: mahogany does not like being run as root
   //
   // NB: this can't be done before initializing the persistent controls path
   //     above or it would be impossible to suppress this dialog (might be a
   //     good thing to do, too, but the users risk to be annoyed by this)
   if ( geteuid() == 0 )
   {
      if( !MDialog_YesNoDialog
           (
            _("You have started Mahogany as the super-user (root).\n"
              "For security reasons we strongly recommend that you\n"
              "exit the program now and run it as an ordinary user\n"
              "instead.\n\n"
              "Are you sure you want to continue?"),
            NULL,
            _("Run as root?"),
            M_DLG_NO_DEFAULT,
            M_MSGBOX_ASK_RUNASROOT) )
      {
         mApplication->SetLastError(M_ERROR_CANCEL);

         return false;
      }
   }
#endif // Unix

   // find our directories
   InitDirectories();

   // safe mode implies interactive
   if ( !m_cmdLineOptions->safe )
   {
      // do it first to avoid any interactive stuff from popping up if
      // configured to start up in the unattended mode
      SetAwayMode(READ_APPCONFIG_BOOL(MP_AWAY_STATUS));
   }

   // show the splash screen (do it as soon as we have profile to read
   // MP_SHOWSPLASH from) unless this is our first run in which case it will
   // disappear anyhow - not showing it avoids some ugly flicker on screen
   if ( !READ_APPCONFIG(MP_FIRSTRUN) && READ_APPCONFIG(MP_SHOWSPLASH) )
   {
      // don't show splash in safe mode as it might be a source of the problems
      // as well
      if ( !m_cmdLineOptions->safe )
      {
         // no parent because no frames created yet
         MDialog_AboutDialog(NULL);
      }
   }

   // verify (and upgrade if needed) our settings
   // -------------------------------------------

   if ( !CheckConfiguration() )
   {
      ERRORMESSAGE((_("Program execution aborted.")));

      return false;
   }

   // Turn off "folder internal data" message. This must be done after
   // profile is initialized and before any window is shown or folder
   // manipulated. Macro name was contributed by c-client maintainer.
   if(!READ_APPCONFIG(MP_CREATE_INTERNAL_MESSAGE))
      env_parameters(SET_USERHASNOLIFE,(void *)1);

#ifndef __CYGWIN__ // FIXME otherwise PATH becomes
                   // D:\Mahogany/scripts:D:\Mahogany/scripts:/cygdrive/c/WINNT/system32:...
   // extend path for commands, look in M's dirs first
   String pathEnv;
   pathEnv << GetLocalDir() << DIR_SEPARATOR << _T("scripts") << PATH_SEPARATOR
           << GetDataDir() << DIR_SEPARATOR << _T("scripts");

   const wxChar *path = wxGetenv(_T("PATH"));
   if ( path )
      pathEnv << PATH_SEPARATOR << path;

   wxSetEnv(_T("PATH"), pathEnv);
#endif //!CYGWIN

   // initialise python interpreter
#ifdef  USE_PYTHON
   // having the same error message each time M is started is annoying, so
   // give the user a possibility to disable it
   if ( !m_cmdLineOptions->noPython &&
            READ_CONFIG(m_profile, MP_USEPYTHON) &&
                  !InitPython() )
   {
      // show the error messages generated before first
      wxLog::FlushActive();

      static const wxChar *msg =
       _T("Detected a possible problem with your Python installation.\n"
       "A properly installed Python system is required for using\n"
       "M's scripting capabilities. Some minor functionality might\n"
       "be missing without it, however the core functions will be\n"
       "unaffected.\n"
       "Would you like to disable Python support for now?\n"
       "(You can re-enable it later from the options dialog)");
      if ( MDialog_YesNoDialog(_(msg)) )
      {
         // disable it
         m_profile->writeEntry(MP_USEPYTHON, FALSE);
      }
   }
#endif //USE_PYTHON

   // the modules can contain bugs, don't load in safe mode
   if ( !m_cmdLineOptions->safe )
   {
      // load any modules requested: notice that this must be done as soon as
      // possible as filters module is already used by the folder opening code
      // below
      LoadModules();
   }

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

      // we want the main window to be above the log frame
      m_topLevelFrame->Raise();
   }

   // also start file logging if configured
   SetLogFile(READ_APPCONFIG(MP_LOGFILE));

   // now we have finished the vital initialization and so can assume
   // everything mostly works
   m_cycle = Running;

   // register with the event subsystem
   // ---------------------------------

   // should never fail...
   m_eventOptChangeReg = MEventManager::Register(*this,
                                                 MEventId_OptionsChange);
   CHECK( m_eventOptChangeReg, FALSE,
          _T("failed to register event handler for options change event") );
   m_eventFolderUpdateReg = MEventManager::Register(*this,
                                                    MEventId_FolderUpdate);
   CHECK( m_eventFolderUpdateReg, FALSE,
          _T("failed to register event handler for folder status event") );

   // finish non critical initialization
   // ----------------------------------

   if ( !m_cmdLineOptions->safe )
   {
      ContinueStartup();

      // as we now have the main window, we can initialize the modules which
      // use it
      InitModules();

      // cache the auto away flag as it will be checked often in UpdateAwayMode
      m_autoAwayOn = READ_APPCONFIG_BOOL(MP_AWAY_AUTO_ENTER);
   }
   else // safe mode
   {
      m_autoAwayOn = false;
   }

   // we won't need the command line options any more
   delete m_cmdLineOptions;
   m_cmdLineOptions = NULL;

   return TRUE;
}

void
MAppBase::OnAbnormalTermination(const char *)
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
   bool initialized = m_cycle != Initializing;

   m_cycle = ShuttingDown;

   if ( initialized )
   {
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

      if (m_FolderMonitor)
      {
         delete m_FolderMonitor;
         m_FolderMonitor = NULL;
      }

      // clean up
      MEventManager::DispatchPending();
      AdbManager::Delete();
      Profile::FlushAll();
   }

   if ( m_profile )
   {
      m_profile->DecRef();
      m_profile = NULL;
   }

   if ( initialized )
   {
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
#if defined(OS_WIN) && defined(DEBUG) && !defined(__CYGWIN__)
      // clean up after cclient (order is important as sysinbox() uses the
      // username)
      free(sysinbox());
      free(myusername_full(NULL));
#endif // OS_WIN

#ifdef USE_PYTHON
      // shut down Python interpreter
      FreePython();
#endif // USE_PYTHON
   }

   // normally this is done in OnStartup() but if we hadn't done it there
   // (because the app couldn't start up) do it now
   if ( m_cmdLineOptions )
   {
      delete m_cmdLineOptions;
      m_cmdLineOptions = NULL;
   }
}

bool
MAppBase::CanClose() const
{
   // Try to send outgoing messages:
   if( CheckOutbox() )
   {
      if ( MDialog_YesNoDialog
           (
            _("You still have messages queued to be sent.\n"
              "Do you want to send them before exiting the application?"),
            NULL,
            MDIALOG_YESNOTITLE,
            M_DLG_YES_DEFAULT,
            M_MSGBOX_SEND_OUTBOX_ON_EXIT)
         )
      {
         SendOutbox();
      }
   }

   // Check Trash folder if it contains any messages (only the global
   // Trash can be checked here!):
   if ( READ_APPCONFIG(MP_USE_TRASH_FOLDER) )
   {
      String trashName = READ_APPCONFIG(MP_TRASH_FOLDER);
      if ( !trashName.empty() )
      {
         MFolder_obj folderTrash(trashName);
         if ( folderTrash.IsOk() )
         {
            MailFolder_obj mf(MailFolder::OpenFolder(folderTrash));
            if ( !!mf && mf->GetMessageCount() )
            {
               if ( MDialog_YesNoDialog
                    (
                     String::Format
                     (
                        _("Would you like to purge all messages from "
                          "the trash mailbox (%s)?"),
                        trashName.c_str()
                     ),
                     NULL,
                     _("Empty trash?"),
                     M_DLG_NO_DEFAULT,
                     M_MSGBOX_EMPTY_TRASH_ON_EXIT
                    ) )
               {
                  if ( MailFolder::ClearFolder(folderTrash) < 0 )
                  {
                     ERRORMESSAGE((_("Failed to empty the trash folder.")));
                  }
               }
            }
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

      if ( !MDialog_YesNoDialog(msg, NULL, MDIALOG_YESNOTITLE,
                                M_DLG_NO_DEFAULT,
                                M_MSGBOX_ABANDON_CRITICAL) )
         return false;
   }

   // ok, we're going to shut down
   return true;
}

void
MAppBase::OnClose()
{
#if 0
   if ( READ_APPCONFIG(MP_REOPENLASTFOLDER) )
   {
      // reset the list of folders to be reopened, it will be recreated in
      // MEventId_AppExit handlers
      GetProfile()->writeEntry(MP_OPENFOLDERS, "");
   }
#endif // 0

   // send an event telling everybody we're closing: note that this can't
   // be blocked, it is just a notification
   MEventManager::Send(new MEventData(MEventId_AppExit));
   MEventManager::DispatchPending();
}

void
MAppBase::AddToFramesOkToClose(const wxMFrame *frame)
{
   if ( !m_framesOkToClose )
      m_framesOkToClose = new ArrayFrames;

   m_framesOkToClose->Add(frame);
}

void
MAppBase::ResetFramesOkToClose()
{
   if ( m_framesOkToClose )
      m_framesOkToClose->Empty();
}

bool
MAppBase::IsOkToClose(const wxMFrame *frame) const
{
   return m_framesOkToClose && m_framesOkToClose->Index(frame) != wxNOT_FOUND;
}

void
MAppBase::Exit(bool ask)
{
   CHECK_RET( m_topLevelFrame, _T("can't close main window - there is none") );

   // if we don't ask, force closing the frame by passing TRUE to Close()
   m_topLevelFrame->Close(!ask);
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
   if (event.GetId() == MEventId_OptionsChange)
   {
#ifdef USE_DIALUP
      SetupOnlineManager(); // make options change effective
#endif // USE_DIALUP
      OnChangeCreateInternalMessage(event);
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
      FAIL_MSG(_T("unexpected event in MAppBase"));
   }

   return TRUE;
}

void
MAppBase::OnChangeCreateInternalMessage(MEventData& event)
{
   MEventOptionsChangeData *optionsChange
      = (MEventOptionsChangeData *)&event;

   if( optionsChange->GetChangeKind() == MEventOptionsChangeData::Ok
         || optionsChange->GetChangeKind() == MEventOptionsChangeData::Apply )
   {
      bool original = !env_parameters(GET_USERHASNOLIFE, NULL);
      bool current = READ_APPCONFIG_BOOL(MP_CREATE_INTERNAL_MESSAGE);
      if ( original != current )
      {
         env_parameters(SET_USERHASNOLIFE, (void *)!current);
         if ( !current )
         {
            MDialog_Message(_(
               "Creating hidden \"folder internal data\" message is\n"
               "disabled, but old such messages still appear in your\n"
               "folders. Delete them using text editor. Keep in mind\n"
               "that not all such messages were created by Mahogany."));
         }
      }
   }
}

void
MAppBase::InitDirectories()
{
   // first the local one: this one contains user-specific files
   // ----------------------------------------------------------

   // the cmd line value overrides everything
   m_localDir = m_cmdLineOptions->userDir;

   // if not given, use the one from the config file
   m_localDir = READ_APPCONFIG_TEXT(MP_USERDIR);

   // if still not given, try to find a good default one ourselves
   if ( m_localDir.empty() )
   {
#ifdef OS_WIN
      // normally we store the user files under APPDATA directory which is
      // something like "C:\Documents and Settings\username\Application Data"

      // suppress errors if shell32.dll can't be loaded or if it doesn't have
      // SHGetSpecialFolderPath (as is the case on Windows 95)
      {
         wxLogNull noLog;

         wxDynamicLibrary dllShell32(_T("shell32.dll"));
         if ( dllShell32.IsLoaded() )
         {
            typedef BOOL
               (WINAPI *SHGetSpecialFolderPathA_t)(HWND, LPTSTR, int, BOOL);

            wxDYNLIB_FUNCTION(SHGetSpecialFolderPathA_t,
                              SHGetSpecialFolderPathA,
                              dllShell32);

            if ( pfnSHGetSpecialFolderPathA )
            {
               String pathAppData;
               if ( pfnSHGetSpecialFolderPathA
                       (
                        NULL,                                  // owner hwnd
                        wxStringBuffer(pathAppData, MAX_PATH), // [out] buffer
                        CSIDL_APPDATA,                         // which to get
                        FALSE                                  // don't create
                       ) )
               {
                  m_localDir = pathAppData + _T("\\Mahogany");
               }
            }
         }
      }

      // but if we couldn't determine APPDATA directory...
      if ( m_localDir.empty() )
      {
         // ... store the files in the program directory itself and in this
         // case use the users name in the subdirectory part to still try to
         // make it possible for several users to use the same installation
         m_localDir = wxGetHomeDir() + _T('\\') + wxGetUserName();
      }

#elif defined(OS_UNIX)
      m_localDir = wxGetHomeDir() + _T("/.M");
#else
      #error "Don't know where to put per-user Mahogany files on this system"
#endif // OS_WIN

      // save it for the next runs
      m_profile->writeEntry(MP_USERDIR, m_localDir);
   }

   // and now the global directory: this one is shared by all users
   // -------------------------------------------------------------

   m_globalDir = READ_APPCONFIG_TEXT(MP_GLOBALDIR);
   if ( m_globalDir.empty() || !PathFinder::IsDir(m_globalDir) )
   {
      // [try to] find the global directory

#if defined(OS_MAC)
      // use the bundle directory under Mac OS X
      CFBundleRef bundle = CFBundleGetMainBundle();
      if ( bundle )
      {
         CFURLRef url = CFBundleCopyBundleURL(bundle);
         if ( url )
         {
            CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
            if ( path )
            {
               // hmm, do we need to use CFStringGetCString() here instead?
               m_globalDir = CFStringGetCStringPtr(path,
                                                   CFStringGetSystemEncoding());

               CFRelease(path);
            }

            CFRelease(url);
         }
      }
#elif defined(OS_UNIX)
      // under Unix we try to find our directory first in compiled-in location
      // and then in some standard ones

      static const wxChar *
         stdPrefixes = _T("/usr:/usr/local:/opt:/usr/local/opt:/tmp");
      PathFinder pf(stdPrefixes);
      pf.AddPaths(M_PREFIX,false,true /* prepend */);

      bool found;
      m_globalDir = pf.FindDir(MAHOGANY_DATADIR, &found);

      // if failed, give up and ask the user
      if ( !found )
      {
         String msg;
         msg.Printf(_("Cannot find global directory \"%s\" in\n"
                      "\"%s\"\n"
                      "Would you like to specify its location now?"),
                    MAHOGANY_DATADIR,
                    stdPrefixes);
         if ( MDialog_YesNoDialog(msg, NULL, MDIALOG_YESNOTITLE,
                                  M_DLG_YES_DEFAULT,
                                  M_MSGBOX_ASK_SPECIFY_DIR) )
         {
            wxDirDialog dlg(NULL, _("Specify global directory for Mahogany"));
            if ( dlg.ShowModal() == wxID_OK )
            {
               m_globalDir = dlg.GetPath();
            }
         }
      }
      else // found in a std location
      {
         // we want just the DESTDIR, not DESTDIR/share/mahogany, so
         // truncate (-1 for slash)
         m_globalDir.resize(m_globalDir.length() - wxStrlen(MAHOGANY_DATADIR) - 1);
      }
#elif defined(OS_WIN)
      // use the program installation directory under Windows
      wxString path;
      if ( ::GetModuleFileName(NULL, wxStringBuffer(path, MAX_PATH), MAX_PATH) )
      {
         // get just the directory
         wxSplitPath(path, &m_globalDir, NULL, NULL);
      }
#else
      #error "Don't know how to find program directory on this platform!"
#endif // OS_MAC/OS_UNIX/OS_WIN

      if ( m_globalDir.empty() )
      {
         // use the user directory by default -- what else can we do?
         m_globalDir = m_localDir;
      }
      else // got valid global directory, save it for subequent runs
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
      MFolder_obj folderOutbox(outbox);
      if ( folderOutbox )
      {
         mf = MailFolder::OpenFolder(folderOutbox);
         if(mf == NULL)
         {
            String msg;
            msg.Printf(_("Cannot open outbox '%s'"), outbox.c_str());
            ERRORMESSAGE((msg));
            return FALSE;
         }
      }
      else
      {
         ERRORMESSAGE((_("Outbox folder '%s' doesn't exist"), outbox.c_str()));
         return FALSE;
      }
   }

   if( !mf->IsEmpty() )
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
            msg->GetHeaderLine(_T("Newsgroups"), newsgroups);
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
MAppBase::SendOutbox(const String & outbox, bool 
#ifdef USE_DIALUP
                     checkOnline 
#endif
                     ) const
{
   CHECK_RET( outbox.length(), _T("missing outbox folder name") );

   UIdType count = 0;

   MFolder_obj folderOutbox(outbox);
   if ( !folderOutbox )
   {
      ERRORMESSAGE((_("Outbox folder '%s' doesn't exist"), outbox.c_str()));
      return;
   }

   MailFolder_obj mf(MailFolder::OpenFolder(folderOutbox));
   if(! mf)
   {
      String msg;
      msg.Printf(_("Cannot open outbox '%s'"), outbox.c_str());
      ERRORMESSAGE((msg));
      return;
   }

   if( mf->IsEmpty() )
   {
      // nothing to do
      return;
   }

#ifdef USE_DIALUP
   if(checkOnline && ! IsOnline())
   {
      if ( !MDialog_YesNoDialog
            (
             _("Cannot send queued messages while dialup network is down.\n"
               "Do you want to go online now?"),
             NULL,
             MDIALOG_YESNOTITLE,
             M_DLG_YES_DEFAULT,
             M_MSGBOX_GO_ONLINE_TO_SEND_OUTBOX
            ) )
      {
         return;
      }

      STATUSMESSAGE((_("Going online...")));
      GoOnline();
   }
#endif // USE_DIALUP

   HeaderInfoList_obj hil(mf->GetHeaders());
   if(! hil)
   {
      // hmm, folder is not empty but has no headers (maybe no more?)?
      return;
   }

   // We can't have a for loop over the HeaderInfoList as messages
   // will be deleted from it just after sending (that is, inside
   // the loop)
   const size_t totalNb = hil->Count();
   size_t nbOfMsgTried = 0;
   UIdType i = 0;

   // FIXME: rewrite this loop as a for loop and do not try
   // to delete messages inside the body of the loop ?
   while ( i < hil->Count() )
   {
      nbOfMsgTried++;

      const HeaderInfo *hi = hil[i];
      if ( !hi )
      {
         ERRORMESSAGE(( _("Failed to access message #%lu in the outbox."),
                        (unsigned long)nbOfMsgTried ));
         ++i;

         continue;
      }

      Message_obj msg(mf->GetMessage(hi->GetUId()));
      if ( !msg )
      {
         ERRORMESSAGE(( _("Failed to recreate message #%lu from the outbox."),
                        (unsigned long)nbOfMsgTried ));
         ++i;

         continue;
      }

      const String subject = msg->Subject();
      STATUSMESSAGE(( _("Sending message %lu/%lu: %s"),
                      (unsigned long)nbOfMsgTried,
                      (unsigned long)totalNb,
                      subject.c_str()));
      wxYield();
      SendMessage_obj
         sendMsg(SendMessage::CreateFromMsg(mf->GetProfile(), msg.Get()));

      if ( sendMsg && sendMsg->SendOrQueue(SendMessage::NeverQueue) )
      {
         count++;
         mf->DeleteMessage(hi->GetUId());
      }
      else
      {
         ERRORMESSAGE((_("Cannot send message '%s'."), subject.c_str()));
         ++i;
      }

      mf->ExpungeMessages();
   }

   if(count > 0)
   {
      String msg;
      msg.Printf(_("Sent %lu messages from outbox \"%s\"."),
                 (unsigned long) count, mf->GetName().c_str());
      STATUSMESSAGE((msg));
   }
}

// ----------------------------------------------------------------------------
// status bar support
// ----------------------------------------------------------------------------

int
MAppBase::GetStatusField(StatusFields field)
{
   // status bar can't be empty except right after program startup, so if it is
   // initialize it with the default values
   if ( m_statusPanes[0] == SF_ILLEGAL )
   {
      m_statusPanes[0] = SF_STANDARD;
      for ( size_t n = 1; n < WXSIZEOF(m_statusPanes); n++ )
         m_statusPanes[n] = SF_ILLEGAL;
   }

   // look for the field in the sorted array using linear search (for an array
   // of 4 elements this isn't wasteful)
   for ( size_t n = 0; n < WXSIZEOF(m_statusPanes); n++ )
   {
      if ( m_statusPanes[n] == field )
      {
         // found the position at which this field appears
         return n;
      }

      if ( m_statusPanes[n] > field || m_statusPanes[n] == SF_ILLEGAL )
      {
         // we insert the status field here to keep the array sorted
         for ( size_t m = WXSIZEOF(m_statusPanes) - 1; m > n; m-- )
         {
            m_statusPanes[m] = m_statusPanes[m - 1];
         }

         m_statusPanes[n] = field;

         RecreateStatusBar();

         return n;
      }
   }

   FAIL_MSG( _T("logic error in GetStatusField") );

   return -1;
}

void MAppBase::RemoveStatusField(StatusFields field)
{
   for ( size_t n = 0; n < WXSIZEOF(m_statusPanes); n++ )
   {
      if ( m_statusPanes[n] == SF_ILLEGAL )
      {
         // no more (initialized) fields
         break;
      }

      if ( m_statusPanes[n] == field )
      {
         // remove this field and shift the remaining ones (don't use memmove()
         // to avoid reading uninitialized memory and provoking Purify ire...)
         for ( size_t m = n + 1; m <= WXSIZEOF(m_statusPanes); m++ )
         {
            if ( m == WXSIZEOF(m_statusPanes) )
            {
               m_statusPanes[m - 1] = SF_ILLEGAL;
            }
            else
            {
               if ( (m_statusPanes[m - 1] = m_statusPanes[m]) == SF_ILLEGAL )
               {
                  // no more fields
                  break;
               }
            }
         }

         RecreateStatusBar();

         break;
      }
   }
}

static bool FatalErrorSemaphore = false;
/// Report a fatal error:
extern "C"
{
   void FatalError(const wxChar *message)
   {
      if(FatalErrorSemaphore)
         abort();
      else
      {
         FatalErrorSemaphore = true;
         mApplication->FatalError(message);
      }
   }
}

// ----------------------------------------------------------------------------
// MAppBase various accessors
// ----------------------------------------------------------------------------

String MAppBase::GetDataDir() const
{
   String dir = GetGlobalDir();

   // do we have a real global dir or was it just set to local dir as
   // fallback?
   if ( dir != GetLocalDir() )
   {
#if defined(OS_MAC)
      // global dir is the bundle directory, our files are under it
      dir << _T("/Contents/Resources");
#elif defined(OS_UNIX)
      // if we used local directory as fall back for the global one because we
      // couldn't find the latter, there is no sense to append share/mahogany
      // to it
      dir << _T('/') << MAHOGANY_DATADIR;
#elif !defined(OS_WIN)
      // under Windows the data directory is the same as the program one
      #error "Don't know how to find data directory on this platform!"
#endif // OS_XXX
   }

   return dir;
}

wxMimeTypesManager& MAppBase::GetMimeManager(void) const
{
    if ( !m_mimeManager )
    {
        // const_cast
        ((MAppBase *)this)->m_mimeManager = new wxMimeTypesManager;

#if 0 // we don't provide any mailcaps so far
        // attempt to load the extra information supplied with M:

        if(wxFileExists(GetDataDir()+_T("/mailcap")))
            m_mimeManager->ReadMailcap(GetDataDir()+_T("/mailcap"));
        if(wxFileExists(GetDataDir()+_T("/mime.types")))
            m_mimeManager->ReadMimeTypes(GetDataDir()+_T("/mime.types"));
#endif // 0

    }

    return *m_mimeManager;
}

#ifdef DEBUG

// only used from wxMainFrame.cpp to toggle debug on/off
bool g_debugMailForceOn = false;

#endif // DEBUG

bool MAppBase::IsMailDebuggingEnabled() const
{
   ASSERT_MSG( m_debugMail != -1, _T("command line not parsed yet!") );

   return
#ifdef DEBUG
         g_debugMailForceOn ||
#endif // DEBUG
         m_debugMail == 1; // don't use TRUE to avoid VC++ warning
}

