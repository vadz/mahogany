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
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "kbList.h"
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "MPython.h"
#  include "Mpers.h"
#  include   <wx/dynarray.h>
#endif   // USE_PCH

#include <errno.h>

#include "MFolder.h"
#include "FolderView.h"
#include "MailFolder.h"
#include "MailCollector.h"

#include "gui/wxFolderView.h"
#include "gui/wxMainFrame.h"
#include "gui/wxMApp.h"
#include "MDialogs.h"   // MDialog_YesNoDialog
#include "gui/wxIconManager.h"
#include "adb/AdbManager.h"   // for AdbManager::Delete

#include "adb/AdbFrame.h"     // for ShowAdbFrame

#include "Mversion.h"
#include "Mupgrade.h"

#include <wx/confbase.h>      // wxExpandEnvVars
#include <wx/mimetype.h>      // wxMimeTypesManager

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
   m_KeepOpenFolders = new MailFolderList;
   m_profile = NULL;
   m_DialupSupport = FALSE;

   m_mimeManager = NULL;

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

/// only used to find list of folders to keep open at all times
class KeepOpenFolderTraversal : public MFolderTraversal
{
public:
   KeepOpenFolderTraversal(MailFolderList *list)
      : MFolderTraversal(*(m_folder = MFolder::Get("")))
      { m_list = list; }
   ~KeepOpenFolderTraversal()
      { m_folder->DecRef(); }
   bool OnVisitFolder(const wxString& folderName)
      {
         MFolder *f = MFolder::Get(folderName);
         if(f && f->GetFlags() & MF_FLAGS_KEEPOPEN)
         {
            wxLogDebug("Found folder to keep open'%s'.", folderName.c_str());
            m_list->push_back( new MailFolderEntry(folderName) );
         }
         if(f)f->DecRef();
         return true;
      }
private:
   MailFolderList *m_list;
   MFolder *m_folder;
};


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
#endif
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
   m_globalDir = READ_APPCONFIG(MP_GLOBALDIR);
   if(strutil_isempty(m_globalDir) || ! PathFinder::IsDir(m_globalDir))
   {
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
               wxDirDialog dlg(NULL, _("Specify global directory for Mahogany"));
               if ( dlg.ShowModal() )
               {
                  m_globalDir = dlg.GetPath();
               }
            }
         }
      }
#else  //Windows
      InitGlobalDir();
#endif //Unix

      m_profile->writeEntry(MP_GLOBALDIR, m_globalDir);
   }
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

   /* Initialise mailcap/mime.types managing subsystem. */
   m_mimeManager = new wxMimeTypesManager();
   // attempt to load the extra information supplied with M:
   if(wxFileExists(GetGlobalDir()+"/mailcap"))
      m_mimeManager->ReadMailcap(GetGlobalDir()+"/mailcap");
   if(wxFileExists(GetGlobalDir()+"/mime.types"))
      m_mimeManager->ReadMimeTypes(GetGlobalDir()+"/mime.types");

   // must be done before using the network
   SetupOnlineManager();

   // create and show the main program window
   CreateTopLevelFrame();

   // update status of outbox once:
   UpdateOutboxStatus();

   // now we can create the log window (the child of the main frame)
   if ( READ_APPCONFIG(MP_SHOWLOG) )
   {
      ShowLog();
   }

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
   char *pathstring = strutil_strdup(tmp);  // this string must not be used again or freed
   putenv(pathstring);
#  endif //OS_WIN

   // initialise python interpreter
#  ifdef  USE_PYTHON
   // having the same error message each time M is started is annoying, so
   // give the user a possibility to disable it
   if ( READ_CONFIG(m_profile, MP_USEPYTHON) && ! InitPython() )
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

   KeepOpenFolderTraversal t(m_KeepOpenFolders);

   if(! t.Traverse(true))
   {
      ERRORMESSAGE((_("Cannot build list of folders to keep open.")));
   }

   // Load any modules requested: notice that this must be done before
   // restoring the ADB editor as it uses the modules
   // must also be done before calling mailcollector
   LoadModules();

   // initialise collector object for incoming mails
   // ----------------------------------------------
   // only do it if we are using the NewMail folder at all
   m_MailCollector = MailCollector::Create();
   if( READ_CONFIG(m_profile, MP_COLLECTATSTARTUP) != 0)
      m_MailCollector->Collect();

   // show the ADB editor if it had been shown the last time when we ran
   // ------------------------------------------------------------------

   if ( READ_CONFIG(m_profile, MP_SHOWADBEDITOR) )
   {
      ShowAdbFrame(TopLevelFrame());
   }

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

   return TRUE;
}

void
MAppBase::AddKeepOpenFolder(const String name)
{
   bool alreadyThere = false;
   MailFolderList::iterator i;
   for(i = m_KeepOpenFolders->begin();
       i != m_KeepOpenFolders->end();
       i++)
      if(name == (**i).GetName())
      {
         alreadyThere = true;
         break;
      }
   if(! alreadyThere)
      m_KeepOpenFolders->push_back( new MailFolderEntry(name) );
}

bool
MAppBase::RemoveKeepOpenFolder(const String name)
{
   MailFolderList::iterator i;
   for(i = m_KeepOpenFolders->begin();
       i != m_KeepOpenFolders->end();
       i++)
      if(name == (**i).GetName())
      {
         m_KeepOpenFolders->erase(i);
         return true;
      }
   return false;
}



void
MAppBase::OnAbnormalTermination()
{
}

void
MAppBase::OnShutDown()
{
   // Try to store our remotely synchronised configuration settings
   extern bool SaveRemoteConfigSettings();
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
      m_MailCollector->DecRef();
   delete m_KeepOpenFolders;

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
   delete m_mimeManager;
   MailFolder::CleanUp();
   // there might have been events queued, get rid of them:
   MEventManager::DispatchPending();
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

   if(READ_APPCONFIG(MP_USE_TRASH_FOLDER)
      // don´t ask twice:
      && READ_APPCONFIG(MP_TRASH_FOLDER) != ((wxMainFrame *)m_topLevelFrame)->GetFolderName())
   {
      ASMailFolder *mf = ASMailFolder::OpenFolder(
         READ_APPCONFIG(MP_TRASH_FOLDER));
      if(mf)
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
   else
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

bool
MAppBase::OnMEvent(MEventData& event)
{
   if(event.GetId() == MEventId_NewMail)
   {
      // we're only registered for new mail events
      CHECK( event.GetId() == MEventId_NewMail, TRUE, "unexpected event" );

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
               unsigned i;
               unsigned found = 0;
               if ( number <= (unsigned long) READ_CONFIG(profile,
                                                          MP_SHOW_NEWMAILINFO))
               {
                  for(i = 0; i < number; i++)
                  {
                     Message *msg =
                        folder->GetMessage(mailevent.GetNewMessageIndex(i));
                     if ( msg )
                     {
                        message << _("Subject: ") << msg->Subject() << ' '
                                << _("From: ") << msg->From()
                                << '\n'
                                << _("in folder '") << folder->GetName() << "'\n\n";
                        msg->DecRef();
                        found++;
                     }
                     else
                        FAIL_MSG("new mail received but no new message?");
                  }
               }
               else
               {
                  // it seems like a better idea to give this brief message in case
                  // of several messages
                  message.Printf(_("You have received %lu new messages\nin folder '%s'."),
                                 number, folder->GetName().c_str());
                  found = number;
               }
               // This test is there as the messages might have gone
               // in the meantime, as happens when filtering.
               if(found > 0) 
                  MDialog_Message(message, m_topLevelFrame, _("New Mail"));
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
      // else
      ASSERT(0); // unexpected
   return TRUE;
}

#ifdef OS_WIN
void
MAppBase::InitGlobalDir()
{
   if ( !m_globalDir )
   {
      // under Windows our directory is always the one where the executable is
      // located. At least we're sure that it exists this way...
      wxString strPath;
      ::GetModuleFileName(::GetModuleHandle(NULL),
                          strPath.GetWriteBuf(MAX_PATH), MAX_PATH);
      strPath.UngetWriteBuf();

      // extract the dir name
      wxSplitPath(strPath, &m_globalDir, NULL, NULL);
   }
   //else: already done once
}
#endif

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
