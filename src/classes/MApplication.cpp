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
#   include  "MFolder.h"
#  include   <wx/dynarray.h>
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
#include  <wx/utils.h>
#define   SYSTEM(command) wxExecute(command, FALSE)

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(const wxMFrame *, ArrayFrames);


struct MailFolderEntry
{
   String      m_name;
   MailFolder *m_folder;
};

KBLIST_DEFINE(MailFolderList, MailFolderEntry);

/// only used by MailCollector to find incoming folders
class MAppFolderTraversal : public MFolderTraversal
{
public:
   MAppFolderTraversal(MailFolderList *list)
      : MFolderTraversal(*(m_folder = MFolder::Get("")))
      { m_list = list;}
   ~MAppFolderTraversal()
      { m_folder->DecRef(); }
   bool OnVisitFolder(const wxString& folderName)
      {
         MFolder *f = MFolder::Get(folderName);
         if(f && f->GetFlags() & MF_FLAGS_INCOMING)
         {
            wxLogDebug("Found incoming folder '%s'.",
                       folderName.c_str());
            MailFolder *mf = MailFolder::OpenFolder(MF_PROFILE,folderName);
            MailFolderEntry *e = new MailFolderEntry;
            e->m_name = folderName;
            e->m_folder = mf;
            m_list->push_back(e);
         }
         if(f)f->DecRef();
         return true;
      }

private:
   MFolder *m_folder;
   MailFolderList *m_list;
};



MailCollector::MailCollector(void)
{
   m_IsCollecting = false;
   m_list = new MailFolderList;
   MAppFolderTraversal t (m_list);
   if(! t.Traverse(true))
      wxLogError(_("Cannot build list of incoming mail folders."));
   // keep it open all the time to speed things up
   m_NewMailFolder = MailFolder::OpenFolder(MF_PROFILE,
                                            READ_APPCONFIG(MP_NEWMAIL_FOLDER));
}

MailCollector::~MailCollector(void)
{
   MailFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end(); i++)
      (**i).m_folder->DecRef();
   if(m_NewMailFolder)
      m_NewMailFolder->DecRef();
   delete m_list;
}

bool
MailCollector::IsIncoming(MailFolder *mf)
{
   MailFolderList::iterator i;
   for(i = m_list->begin();i != m_list->end();i++)
      if((**i).m_folder == mf)
         return true;
   return false;
}

bool
MailCollector::Collect(MailFolder *mf)
{
   bool rc = true;
   if(m_NewMailFolder)
   {
      m_NewMailFolder->EnableNewMailEvents(false,false);
      m_NewMailFolder->Ping();
   }

   MailFolderList::iterator i;
   if(mf == NULL)
   {
      for(i = m_list->begin();i != m_list->end();i++)
         rc &= CollectOneFolder((*i)->m_folder);
   }
   else
      rc = CollectOneFolder(mf);
   if(m_NewMailFolder)
   {
      m_NewMailFolder->EnableNewMailEvents(true,true);
      m_NewMailFolder->Ping();
   }
   return rc;
}

bool
MailCollector::CollectOneFolder(MailFolder *mf)
{
   ASSERT(mf);
   bool rc;
   
   m_IsCollecting = true;
   wxLogDebug(_("Auto-collecting mail from incoming folder '%s'."),
                mf->GetName().c_str());
   long oldcount = m_NewMailFolder->CountMessages();
   bool sendsEvents = mf->SendsNewMailEvents();
   mf->EnableNewMailEvents(false,true);
   mf->Ping(); //update it
   INTARRAY selections;

   const HeaderInfo *hi;
   for(hi = mf->GetFirstHeaderInfo();
       hi;
       hi = mf->GetNextHeaderInfo(hi))
      selections.Add(hi->GetUId());

   if(mf->SaveMessages(&selections,
                       READ_APPCONFIG(MP_NEWMAIL_FOLDER),
                       true))
   {
      mf->DeleteMessages(&selections);
      mf->ExpungeMessages();
      m_IsCollecting = false;
      rc = true;
   }
   else
      rc = false;
   long i = 0;
   String seq;
   for(hi = m_NewMailFolder->GetFirstHeaderInfo();
       hi;
       hi = m_NewMailFolder->GetNextHeaderInfo(hi))
   {
      if(i >= oldcount)
      {
         if(seq.Length()) seq << ',';
         seq << strutil_ultoa(hi->GetUId());
      }
      i++;
   }
   // mark new messages as new:
   m_NewMailFolder->SetSequenceFlag(seq,MailFolder::MSG_STAT_RECENT, true);
   m_NewMailFolder->SetSequenceFlag(seq,MailFolder::MSG_STAT_SEEN, false);
   mf->EnableNewMailEvents(sendsEvents);
   m_IsCollecting = false;
   return rc;
}

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
   m_framesOkToClose = NULL;
   m_MailCollector = NULL;
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
         wxLogError(_("Cannot find your Mahogany directory, please specify it "
                      "in the options dialog."));
      }
      else
      {
         wxSplitPath(szFileName, &m_localDir, NULL, NULL);
         m_profile->writeEntry(MP_USERDIR, m_localDir);
      }
   }
#else
#   error "Don't know how to find local dir under this OS"
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
         if(! SetupInitialConfig())
            ERRORMESSAGE((_("Failed to set up initial configuration.")));

         // next time won't be the first one any more
         m_profile->writeEntry(MP_FIRSTRUN, 0);
   }

   // do we need to upgrade something?
   String version = m_profile->readEntry(MP_VERSION, "");
   if ( version != M_VERSION )
   {
      if ( ! Upgrade(version) )
         return false;
      // write the new version
      m_profile->writeEntry(MP_VERSION, M_VERSION);
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
   if ( stat(strConfFile, &st) != 0 || !S_ISDIR(st.st_mode) )
   {
      if ( mkdir(strConfFile, 0700) != 0 )
      {
         wxLogError(_("Can't create the directory for configuration "
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

   m_profile = ProfileBase::CreateGlobalConfig(strConfFile);

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
      m_profile->writeEntry(MP_GLOBALDIR, m_globalDir);
   }

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

   /* Initialise mailcap/mime.types managing subsystem. */
   m_mimeManager = new wxMimeTypesManager();
   // attempt to load the extra information supplied with M:
   if(wxFileExists(GetGlobalDir()+"/mailcap"))
      m_mimeManager->ReadMailcap(GetGlobalDir()+"/mailcap");
   if(wxFileExists(GetGlobalDir()+"/mime.types"))
      m_mimeManager->ReadMimeTypes(GetGlobalDir()+"/mime.types");

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
      char *pathstring = strutil_strdup(tmp);  // this string must not be used again or freed
      putenv(pathstring);
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

   // initialise collector object for incoming mails
   // ----------------------------------------------
   m_MailCollector = new MailCollector();
   m_MailCollector->Collect(); // empty all at beginning
   
   // register with the event subsystem
   // ---------------------------------
   m_eventReg = MEventManager::Register(*this, MEventId_NewMail);

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
      MEventManager::Deregister(m_eventReg);
      m_eventReg = NULL;
   }
   if(m_MailCollector) delete m_MailCollector;
   // clean up
   AdbManager::Delete();
   ProfileBase::FlushAll();
   // The following little hack allows us to decref and delete the
   // global profile without triggering an assert, as this is not
   // normally allowed.
   ProfileBase *p = m_profile;
   m_profile = NULL;
   p->DecRef();
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

bool
MAppBase::CanClose() const
{
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
MAppBase::Exit()
{
   if ( m_topLevelFrame )
   {
      if ( m_topLevelFrame->Close() )
         m_topLevelFrame = NULL;
   }

   if ( m_framesOkToClose )
   {
      delete m_framesOkToClose;
      m_framesOkToClose = NULL;
   }
}

bool
MAppBase::OnMEvent(MEventData& event)
{
   // we're only registered for new mail events
   CHECK( event.GetId() == MEventId_NewMail, TRUE, "unexpected event" );

   // get the folder in which the new mail arrived
   MEventNewMailData& mailevent = (MEventNewMailData &)event;
   MailFolder *folder = mailevent.GetFolder();

   /* First, we need to check whether it is one of our incoming mail
      folders and if so, move it to the global new mail folder and
      ignore the event. */
   if(m_MailCollector->IsIncoming(folder))
   {
      if(m_MailCollector->IsCollecting())
         return false;
      if(!m_MailCollector->Collect(folder))
         wxLogError(_("Could not collect mail from incoming folder '%s'."),
                    folder->GetName().c_str());
      return false; 
   }

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

         unsigned long number = mailevent.GetNumber();
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
            message.Printf(_("You have received %lu new messages\nin folder '%s'."),
                           number, folder->GetName().c_str());
         }

         // TODO make it a wxPMessageBox to let the user shut if off from here?
         MDialog_Message(message, m_topLevelFrame, _("New Mail"));
      }
   }

   return TRUE;
}

