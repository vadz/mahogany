/*-*- c++ -*-********************************************************
 * PalmOS - a PalmOS connectivity module for Mahogany               *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mconfig.h"
#   include "Mcommon.h"
#   include "MDialogs.h"
#   include "strutil.h"
#   include "Mdefaults.h"
#   include "gui/wxMenuDefs.h"
#   include "MMainFrame.h"
#endif

#ifdef USE_PISOCK

#include "MModule.h"

#include "Mversion.h"
#include "MInterface.h"
#include "Message.h"

#include "SendMessageCC.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxMDialogs.h"

#include <wx/menu.h>

#include "adb/ProvPalm.h"

#define MODULE_NAME    "PalmOS"

#define DISPOSE_KEEP   0l
#define DISPOSE_DELETE 1l
#define DISPOSE_FILE   2l

#define MP_MOD_PALMOS_BOX        "PalmBox"
#define MP_MOD_PALMOS_BOX_D      "PalmBox"
#define MP_MOD_PALMOS_DISPOSE    "Dispose"
#define MP_MOD_PALMOS_DISPOSE_D  2  // 0=keep 1=delete 2=file
#define MP_MOD_PALMOS_SYNCMAIL   "SyncMail"
#define MP_MOD_PALMOS_SYNCMAIL_D 1l
#define MP_MOD_PALMOS_SYNCADDR   "SyncAddr"
#define MP_MOD_PALMOS_SYNCADDR_D 1l
#define MP_MOD_PALMOS_BACKUP     "Backup"
#define MP_MOD_PALMOS_BACKUP_D   0l
#define MP_MOD_PALMOS_BACKUPDIR              "BackupDir"
#define MP_MOD_PALMOS_BACKUPDIR_D            ""
#define MP_MOD_PALMOS_BACKUP_SYNC            "BackupSync"
#define MP_MOD_PALMOS_BACKUP_SYNC_D          0l
#define MP_MOD_PALMOS_BACKUP_INCREMENTAL     "BackupIncremental"
#define MP_MOD_PALMOS_BACKUP_INCREMENTAL_D   1l
#define MP_MOD_PALMOS_BACKUPALL              "BackupAll"
#define MP_MOD_PALMOS_BACKUPALL_D            1l
#define MP_MOD_PALMOS_BACKUP_EXCLUDELIST     "BackupExclude"
#define MP_MOD_PALMOS_BACKUP_EXCLUDELIST_D   ""
#define MP_MOD_PALMOS_PILOTDEV   "PilotDev"
#define MP_MOD_PALMOS_PILOTDEV_D "/dev/pilot"
#define MP_MOD_PALMOS_SPEED      "Speed"
#define MP_MOD_PALMOS_SPEED_D    3
#define MP_MOD_PALMOS_LOCK       "Lock"
#define MP_MOD_PALMOS_LOCK_D 0l
#define MP_MOD_PALMOS_SCRIPT1    "Script1"
#define MP_MOD_PALMOS_SCRIPT1_D  ""
#define MP_MOD_PALMOS_SCRIPT2    "Script2"
#define MP_MOD_PALMOS_SCRIPT2_D  ""

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

// to prevent a warning from linux headers
//#define __STRICT_ANSI__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <pi-address.h>     // for the addressbook
#include <pi-source.h>
#include <pi-socket.h>
#include <pi-file.h>
#include <pi-mail.h>        // for the mailbox
#include <pi-dlp.h>

#include "adb/AdbManager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <utime.h>

#include <wx/log.h>
#include <wx/textfile.h>

#ifdef OS_SOLARIS
extern "C" {
   extern int kill(pid_t, int);
};
#endif

// ---------------------------------------------------
// Implementation
// ---------------------------------------------------

class wxDeviceLock
{
public:
   wxDeviceLock(const wxString &dev)
   {
      m_Device = dev;
      m_Locked = FALSE;
   }

   ~wxDeviceLock()
   {
      if(m_Locked) Unlock();
   }

   bool Lock()
   {
      wxASSERT(! m_Locked);
      m_LockFile = "/var/lock/LCK..";
      m_LockFile << m_Device;
      int fd = open(m_LockFile,O_CREAT|O_EXCL,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

      if(fd == -1) // lock exists already
      {
         wxTextFile tf(m_LockFile);
         if(tf.Open())
         {
            pid_t pid = atoi(tf[0]);
            if(kill(pid,0) == ESRCH) // process no longer exists
            {
               if(remove(m_LockFile) == 0)
               {
                  // try again
                  fd = open(m_LockFile,
                            O_CREAT|O_EXCL,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
               }
            }
         }
      }

      if (fd != -1)
      {
         m_Locked = TRUE;
         wxString pidstr;
         pidstr.Printf("%lu", (unsigned long) getpid());
         write(fd, (char *)pidstr.c_str(),pidstr.Length());
         close(fd);
      }
      else
      {
         wxString msg;
         msg.Printf(_("Could not obtain lock for '/dev/%s'"),
                       m_Device.c_str());
         wxLogSysError(msg);
      }
      return m_Locked;
   }

   void Unlock()
   {
      wxASSERT(m_Locked);
      remove(m_LockFile);
   }

   bool IsLocked() const { return m_Locked; }

private:
   wxString m_Device;
   wxString m_LockFile;
   bool     m_Locked;
};


///------------------------------
/// MModule interface:
///------------------------------

class PalmOSModule : public MModule
{
   /** Override the Entry() function to allow Main() and Config()
       functions. */
   virtual int Entry(int arg, ...);
   void Synchronise(PalmBook *pBook);
   void Configure(void);
   MMODULE_DEFINE()

private:
   /** PalmOS constructor.
       As the class has no usable interface, this doesn´t do much, but
       it displays a small dialog to say hello.
       A real module would store the MInterface pointer for later
       reference and check if everything is set up properly.
   */
   PalmOSModule(MInterface *interface);

   bool ProcessMenuEvent(int id);
   bool Connect(void);
   void Disconnect(void);
   friend class PiConnection;

   bool IsConnected(void) const { return m_PiSocket >= 0; }

   void GetConfig(void);

   void SyncAddresses(PalmBook *pBook);
   void GetAddresses(PalmBook *pBook);

   void SyncMail(void);
   void SendEMails(void);
   void StoreEMails(void);

   void Backup(void);
   void Restore(void);
   void Install(void);

   void AutoInstall(void);
   void InstallFiles(char ** fnames, int files_total, bool delFiles);
   void InstallFromDir(wxString directory, bool delFiles);

   inline void ErrorMessage(const String &msg)
      { m_MInterface->Message(msg,NULL,"PalmOS module error!");wxYield(); }
   inline void Message(const String &msg)
      { m_MInterface->Message(msg,NULL,"PalmOS module"); wxYield(); }
   inline void StatusMessage(const String &msg)
      { m_MInterface->StatusMessage(msg);wxYield();}

   int createEntries(int db, struct AddressAppInfo * aai, PalmEntryGroup* p_Group);
   int CreateFileList(char *** list, DIR * dir);
   void DeleteFileList(char **list, int filecount);
   void RemoveFromList(char *name, char **list, int max);

   int m_PiSocket;
   int m_MailDB;
   int m_AddrDB;
   ProfileBase *m_Profile;

   int m_Dispose;
   int m_Speed;
   bool m_SyncMail, m_SyncAddr, m_Backup, m_LockPort;
   bool m_IncrBackup, m_BackupSync, m_BackupAll;
   String m_PilotDev, m_Script1, m_Script2, m_PalmBox;
   String m_BackupExludeList;
   String m_BackupDir;
   String m_AutoInstallDir;

   class wxDeviceLock *m_Lock;
};

/// small helper class
class PiConnection
{
public:
   PiConnection(class PalmOSModule *mi)
   {
      m=mi;
      if(! m->IsConnected())
      {
         m->Connect();
         cleanup = true;
      }
      else
         cleanup = false;
   }

   ~PiConnection() { if(cleanup) m->Disconnect(); }

private:
   PalmOSModule *m;
   bool cleanup;
};


bool
PalmOSModule::ProcessMenuEvent(int id)
{
   switch(id)
   {
      case WXMENU_MODULES_PALMOS_SYNC:
         Synchronise(NULL);
         return TRUE;
      case WXMENU_MODULES_PALMOS_BACKUP:
         Backup();
         return TRUE;
      case WXMENU_MODULES_PALMOS_RESTORE:
         Restore();
         return TRUE;
      case WXMENU_MODULES_PALMOS_INSTALL:
         Install();
         return TRUE;
      case WXMENU_MODULES_PALMOS_CONFIG:
         Configure();
         return TRUE;
      default:
         return FALSE;
   }
}

int
PalmOSModule::Entry(int arg, ...)
{
   switch(arg)
   {
      // GetFlags():
      case MMOD_FUNC_GETFLAGS:
         return MMOD_FLAG_HASMAIN|MMOD_FLAG_HASCONFIG;

      // Main():
      case MMOD_FUNC_MAIN:
         Synchronise(NULL);
         return 0;

      // Configure():
      case MMOD_FUNC_CONFIG:
         Configure();
         return 0;

      // Menu event
      case MMOD_FUNC_MENUEVENT:
      {
         va_list ap;
         va_start(ap, arg);
         int id = va_arg(ap, int);
         va_end(ap);
         return ProcessMenuEvent(id);
      }

      // module specific functions:
      // MMOD_FUNC_USER : Synchronise ADB
      case MMOD_FUNC_USER:
      {
         va_list ap;
         va_start(ap, arg);
         PalmBook *pbp = va_arg(ap, PalmBook *);
         va_end(ap);
         Synchronise(pbp);
         return 0;
      }

      default:
         return 0;
   }
}


void
PalmOSModule::GetConfig(void)
{
   ProfileBase * appConf = m_MInterface->GetGlobalProfile();

   // mail related values get read from the PALMBOX mailfolder profile:
   if(m_Profile == NULL)
   {
      m_Profile = m_MInterface->CreateProfile(
         appConf->readEntry(MP_MOD_PALMOS_BOX,MP_MOD_PALMOS_BOX_D));
   }

   // all other values get read from the module profile:
   ProfileBase * p= m_MInterface->CreateModuleProfile(MODULE_NAME);

   // must be in sync with the combobox values in config table
   //further down:
   static int speeds[] = { 9600,19200,38400,57600,115200 };

   m_Backup     = (READ_CONFIG(p, MP_MOD_PALMOS_BACKUP) != 0);
   m_BackupDir  = READ_CONFIG(p, MP_MOD_PALMOS_BACKUPDIR);
   m_SyncMail   = (READ_CONFIG(p, MP_MOD_PALMOS_SYNCMAIL) != 0);
   m_SyncAddr   = (READ_CONFIG(p, MP_MOD_PALMOS_SYNCADDR) != 0);
   m_LockPort   = (READ_CONFIG(p, MP_MOD_PALMOS_LOCK) != 0);
   m_PilotDev   = READ_CONFIG(p, MP_MOD_PALMOS_PILOTDEV);
   m_PalmBox    = READ_CONFIG(p, MP_MOD_PALMOS_BOX);
   m_Dispose    = READ_CONFIG(p, MP_MOD_PALMOS_DISPOSE);
   m_Speed      = READ_CONFIG(p, MP_MOD_PALMOS_SPEED);
   m_IncrBackup = READ_CONFIG(p, MP_MOD_PALMOS_BACKUP_INCREMENTAL);
   m_BackupSync = READ_CONFIG(p, MP_MOD_PALMOS_BACKUP_SYNC);
   m_BackupAll  = (READ_CONFIG(p, MP_MOD_PALMOS_BACKUPALL) != 0);
   m_BackupExcludeList = READ_CONFIG(p, MP_MOD_PALMOS_BACKUP_EXCLUDELIST;

   if(m_Speed < 0  || m_Speed > (signed) WXSIZEOF(speeds))
      m_Speed = speeds[0];
   else
      m_Speed = speeds[m_Speed];

   m_Script1 = READ_CONFIG(p, MP_MOD_PALMOS_SCRIPT1);
   m_Script2 = READ_CONFIG(p, MP_MOD_PALMOS_SCRIPT2);
   p->DecRef();

   if(m_BackupDir.Length()
      && m_BackupDir[m_BackupDir.Length()-1] != DIR_SEPARATOR)
      m_BackupDir << DIR_SEPARATOR;

   String dev;
   dev = m_PilotDev;
   if(strncmp(m_PilotDev,"/dev/",5)==0)
      dev = m_PilotDev.c_str()+5;
   if(m_Lock) delete m_Lock;
   m_Lock = new wxDeviceLock(dev);
}

MMODULE_IMPLEMENT(PalmOSModule,
                  "PalmOS",
                  "HandheldSynchronise",
                  "This module provides PalmOS connectivity.",
                  "0.00")

///------------------------------
/// Own functionality:
///------------------------------

/* static */

MModule *
PalmOSModule::Init(int version_major, int version_minor,
                   int version_release, MInterface *interface,
                   int *errorCode)
{
   if(! MMODULE_SAME_VERSION(version_major, version_minor,
                             version_release))
   {
      if(errorCode) *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }

   return new PalmOSModule(interface);
}


PalmOSModule::PalmOSModule(MInterface *minterface)
            : MModule(minterface)
{
   m_PiSocket = -1;
   m_Profile = NULL;
   m_Lock = NULL;

   wxMenu * palmOsMenu = new wxMenu("", wxMENU_TEAROFF);
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_SYNC, _("&Synchronise"));
   palmOsMenu->Break();
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_BACKUP, _("&Backup"));
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_RESTORE, _("&Restore"));
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_INSTALL, _("&Install"));
   palmOsMenu->Break();
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_CONFIG, _("&Configure"));

   MAppBase *mapp = minterface->GetMApplication();
   ((wxMainFrame *)mapp->TopLevelFrame())->AddModulesMenu(_("&PalmOS Module"),
                                        _("Functionality to interact with your PalmOS based palmtop."),
                                        palmOsMenu,
                                        -1);
}

PalmOSModule::~PalmOSModule()
{
   if(IsConnected())
      Disconnect();

   if(m_Lock) delete m_Lock;
   SafeDecRef(m_Profile);
}

#if defined( wxUSE_THREADS ) && defined( OS_UNIX )
class PalmOSAcceptThread : public wxThread
{
public:
   PalmOSAcceptThread(int *piSocket)
   {
      m_PiSocketPtr = piSocket;
      m_NewSocket = *piSocket;
      *m_PiSocketPtr = -1; // -2: thread is running, -1: failure
   }

   // thread execution starts here
   virtual void *Entry()
   {
      *m_PiSocketPtr = -2;
      m_NewSocket = pi_accept(m_NewSocket, 0, 0);
      *m_PiSocketPtr = m_NewSocket;
      return NULL;
   }

private:
   int * m_PiSocketPtr;
   int m_NewSocket;
};
#endif

bool
PalmOSModule::Connect(void)
{
   if(m_PiSocket == -1)
   {
      GetConfig();

      struct pi_sockaddr addr;
      struct PilotUser pilotUser;
      int rc;

      if(m_Script1.Length())
      {
         int rc;
         if((rc = wxExecute(m_Script1, TRUE)) != 0)
         {
            String msg;
            msg.Printf(_("Executing command ´%s´ returned an error code (%d)."),
                       m_Script1.c_str(), rc);
            ErrorMessage(msg);
         }
      }

      if(m_LockPort)
         if(! m_Lock->Lock())
            return false;

      Message(_("Please press HotSync button and click on OK."));
      if (!(m_PiSocket = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP)))
      {
         ErrorMessage(_("Failed to connect to PalmOS device."));
         if(m_Lock->IsLocked()) m_Lock->Unlock();
         return false;
      }

      addr.pi_family = PI_AF_SLP;
      strncpy(addr.pi_device,
              m_PilotDev.c_str(),
              sizeof(addr.pi_device));

#ifdef HAVE_PI_SETMAXSPEED
      pi_setmaxspeed(m_PiSocket, m_Speed, 0 /* overclock */);
#endif

      rc = pi_bind(m_PiSocket, (struct sockaddr*)&addr, sizeof(addr));
      if(rc == -1)
      {
         ErrorMessage(_("Failed to connect to PalmOS device."));
         pi_close(m_PiSocket); m_PiSocket = -1;
         if (m_Lock->IsLocked())
            m_Lock->Unlock();

         return false;
      }

      rc = pi_listen(m_PiSocket,1);
      if(rc == -1)
      {
         ErrorMessage(_("Failed to connect to PalmOS device."));
         pi_close(m_PiSocket); m_PiSocket = -1;
         if (m_Lock->IsLocked())
            m_Lock->Unlock();

         return false;
      }

      // the following code is unsafe under Windows:
#if defined( wxUSE_THREADS ) && defined( OS_UNIX )
      // We interrupt the connection attempt after some seconds, to
      // provide some kind of timeout that the stupid pilot-link code
      // doesn't. Ugly hack, really. KB
      PalmOSAcceptThread *acceptThread = new PalmOSAcceptThread(&m_PiSocket);
      if ( acceptThread->Create() != wxTHREAD_NO_ERROR )
      {
         wxLogError(_("Cannot create thread!"));
         m_PiSocket = pi_accept(m_PiSocket, 0, 0);
      }
      else
      {
         int oldPiSocket = m_PiSocket;
         acceptThread->Run();
         time_t now = time(NULL);
         while(m_PiSocket < 0 && time(NULL)-now < 5)
            wxThread::Sleep(100);

         if(m_PiSocket < 0)
         {
            //acceptThread->Kill();
            pi_close(oldPiSocket);
         }
      }
#else
      m_PiSocket = pi_accept(m_PiSocket, 0, 0);
#endif
      if(m_PiSocket < 0)
      {
         ErrorMessage(_("Failed to connect to PalmOS device."));
         pi_close(m_PiSocket);
         if (m_Lock->IsLocked())
            m_Lock->Unlock();

         return false;
      }

      /* Ask the pilot who it is. */
      dlp_ReadUserInfo(m_PiSocket,&pilotUser);

      /* Tell user (via Pilot) that we are starting things up */
      dlp_OpenConduit(m_PiSocket);
   }

   if(m_PiSocket == -1)
      if(m_Lock->IsLocked()) m_Lock->Unlock();

   return m_PiSocket != -1;
}

void
PalmOSModule::Disconnect(void)
{
   if(m_PiSocket != -1)
   {
      pi_close(m_PiSocket);
      m_PiSocket = -1;

      if(m_Lock->IsLocked())
         m_Lock->Unlock();

      if(m_Script2.Length())
      {
         int rc;
         if((rc = wxExecute(m_Script2, TRUE)) != 0)
         {
            String msg;
            msg.Printf(_("Executing command ´%s´ returned an error code (%d)."),
                       m_Script2.c_str(), rc);
            ErrorMessage(msg);
         }
      }
   }
}

void PalmOSModule::SyncMail(void)
{
   PiConnection conn(this);
   if ( IsConnected())
   {
      SendEMails();
      StoreEMails();

      // here we close the opened database
      if (m_MailDB)
         dlp_CloseDB(m_PiSocket, m_MailDB);
   }
}

void PalmOSModule::SyncAddresses(PalmBook *pBook)
{
   PiConnection conn(this);
   if ( IsConnected() )
   {
      GetAddresses(pBook);

      // close the database again
      if (m_AddrDB)
         dlp_CloseDB(m_PiSocket, m_AddrDB);
   }
}

void PalmOSModule::Synchronise(PalmBook *pBook)
{
   if(m_MInterface->YesNoDialog(_("Do you want synchronise with your PalmOS device?")))
   {
      PiConnection conn(this);
      if(! IsConnected())
         return;

      if(m_SyncMail)
         SyncMail();

      if(m_SyncAddr)
         SyncAddresses(pBook);

      if(m_Backup)
         Backup();

      m_Profile->DecRef();
      m_Profile=NULL;
   }
}


/* Protect = and / in filenames */
static void protect_name(char *d, char *s)
{
   while(*s) {
      switch(*s) {
         case '/': *(d++) = '=';
            *(d++) = '2';
            *(d++) = 'F';
            break;
         case '=': *(d++) = '=';
            *(d++) = '3';
            *(d++) = 'D';
            break;
         case '\x0A':
            *(d++) = '=';
            *(d++) = '0';
            *(d++) = 'A';
            break;
         case '\x0D':
            *(d++) = '=';
            *(d++) = '0';
            *(d++) = 'D';
            break;
         default: *(d++) = *s;
      }
      s++;
   }
   *d = '\0';
}

int
PalmOSModule::CreateFileList(char ***list, DIR * dir)
{
   char **filelist = 0;
   struct dirent * dirent;

   int file_len;
   int filecount;
   filecount = 0;
   file_len = 0;

   while( (dirent = readdir(dir)) ) {
      char name[256];
      if (dirent->d_name[0] == '.')
         continue;

      if (!filelist) {
         file_len += 256;
         filelist = (char **) malloc(sizeof(char*) * file_len);
      } else if (filecount >= file_len) {
         file_len += 256;
         filelist = (char **) realloc(filelist, sizeof(char*) * file_len);
      }

      sprintf(name, "%s%s", m_BackupDir.c_str(), dirent->d_name);
      filelist[filecount++] = strutil_strdup(name);
   }

   *list = filelist;
   return filecount;
}


void
PalmOSModule::RemoveFromList(char *name, char **list, int max)
{
   for (int i = 0; i < max; i++)
   {
      if (list[i] != NULL && strcmp(name, list[i]) == 0)
      {
         free(list[i]);
         list[i] = NULL;
      }
   }
}


void
PalmOSModule::DeleteFileList(char **list, int filecount)
{
   if (list) {
      for (int i = 0; i < filecount; i++)
         if (list[i] != NULL)
            free(list[i]);

      if (list)
         free(list);
   }
}


void
PalmOSModule::Backup(void)
{
   // connect to the Palm
   PiConnection conn(this);
   if( ! IsConnected())
      return;

   // access backup directory
   DIR * dir;
   dir = opendir(m_BackupDir);

   if (dir <= 0)
   {
      String msg;
      msg.Printf(_("Could not access backup directory '%s'."),
                 m_BackupDir.c_str());
      ErrorMessage(msg);
      return;
   }

   // Read original list of files in the backup dir
   int     ofile_total;
   char ** orig_files = 0;

   ofile_total = CreateFileList(&orig_files, dir);
   closedir(dir);

   // count files on the palm
   int max = 0;
   int i = 0;

   while (true) {
      struct DBInfo info;
      if (dlp_ReadDBList(m_PiSocket, 0, 0x80, i, &info) < 0)
         break;
      max++;
      i = info.index + 1;
   }

   // open progress dialog
   MProgressDialog *pd;
   pd = new MProgressDialog(_("Palm Backup"), _("Backing up files"),
                            max, NULL, false, true);

   // resetting values
   max = 0;
   i = 0;

   // for each file on the palm do ...
   while (true) {
      struct DBInfo   info;
      struct pi_file *f;
      struct utimbuf  times;
      struct stat     statb;
      char            name[256];

      if (dlp_ReadDBList(m_PiSocket, 0, 0x80, i, &info) < 0)
         break;

      i = info.index + 1;

      if (dlp_OpenConduit(m_PiSocket) < 0) {
         ErrorMessage(_("Exiting on cancel, backup process incomplete!"));
         delete pd;
         return;
      }

      // don´t keep DB_open flag
      info.flags &= 0xff;

      // construct filename
      strncpy(name, m_BackupDir, 256);
      protect_name(strlen(name) + name, info.name);
      if (info.flags & dlpDBFlagResource)
         strcat(name, ".prc");
      else
         strcat(name, ".pdb");

      // update progress dialog, exit on "cancel"
      if( ! pd->Update(max++, name) )
      {
         DeleteFileList(orig_files, ofile_total);
         delete pd;
         return;
      }

      if (m_IncrBackup)
         if (stat(name, &statb) == 0)
            if (info.modifyDate == statb.st_mtime) {
               RemoveFromList(name, orig_files, ofile_total);
               continue;
            }

      // create file
      f = pi_file_create(name, &info);

      if (f == 0) {
         ErrorMessage(_("Unable to create file!"));
         break;
      }

      if (pi_file_retrieve(f, m_PiSocket, 0) < 0)
         ErrorMessage(_("Unable to back up database!"));

      pi_file_close(f);

      /* Note: This is no guarantee that the times on the host system
         actually match the GMT times on the Pilot. We only check to
         see whether they are the same or different, and do not treat
         them as real times. */
      times.actime = info.createDate;
      times.modtime = info.modifyDate;
      utime(name, &times);

      RemoveFromList(name, orig_files, ofile_total);
   }

   delete pd;

   // Remaining files are outdated
   if ( (m_BackupSync) && (orig_files) )
      for (i = 0; i < ofile_total; i++)
         if (orig_files[i] != NULL)
            unlink(orig_files[i]);  // delete

   // All files are backed up now.
   DeleteFileList(orig_files, ofile_total);
}

struct db {
   char name[256];
   int flags;
   unsigned long creator;
   unsigned long type;
   int maxblock;
};

int
pdbCompare(struct db * d1, struct db * d2)
{
   /* types of 'appl' sort later then other types */
   if(d1->creator == d2->creator)
      if(d1->type != d2->type) {
         if(d1->type == pi_mktag('a','p','p','l'))
            return 1;
         if(d2->type == pi_mktag('a','p','p','l'))
            return -1;
      }

   return d1->maxblock < d2->maxblock;
}

void
PalmOSModule::InstallFiles(char **fnames, int files_total, bool delFile)
{
   struct DBInfo info;
   struct db * db[256];
   int    dbcount = 0;
   int    size;
   int i, j, max;
   struct pi_file * f;
   MProgressDialog *pd;

   for ( j = 0; j < files_total; j++) {
      db[dbcount] = (struct db*)malloc(sizeof(struct db));

      // remember filename
      sprintf(db[dbcount]->name, "%s", fnames[j]);

      f = pi_file_open(db[dbcount]->name);

      if (f==0) {
         // TODO: show filename
         ErrorMessage(_("Unable to open file!"));
         break;
      }

      pi_file_get_info(f, &info);

      db[dbcount]->creator = info.creator;
      db[dbcount]->type = info.type;
      db[dbcount]->flags = info.flags;
      db[dbcount]->maxblock = 0;

      pi_file_get_entries(f, &max);

      for (i=0; i<max; i++) {
         if (info.flags & dlpDBFlagResource)
            pi_file_read_resource(f, i, 0, &size, 0, 0);
         else
            pi_file_read_record(f, i, 0, &size, 0, 0,0 );

         if (size > db[dbcount]->maxblock)
            db[dbcount]->maxblock = size;
      }

      pi_file_close(f);
      dbcount++;

      // shall we delete the file after deletion?
      if (delFile) {
         unlink(fnames[j]);
      }
   }

   // sort list in alphabetical order
   for (i=0; i < dbcount; i++)
      for (j = i+1; j<dbcount; j++)
         if (pdbCompare(db[i], db[j]) > 0)
         {
            struct db * temp = db[i];
            db[i] = db[j];
            db[j] = temp;
         }

   pd = new MProgressDialog(_("Palm Install"), _("Installing files"),
                            dbcount, NULL, false, true);

   // Install files
   for (i=0; i < dbcount; i++)
   {
      if (!pd->Update(i, db[i]->name))
      {
         delete pd;
         return;
      }

      if ( dlp_OpenConduit(m_PiSocket) < 0)
      {
         ErrorMessage(_("Exiting on cancel, all data not restored"));
         delete pd;
         return;
      }

      f = pi_file_open(db[i]->name);
      if (f == 0)
      {
         // TODO: display filename
         ErrorMessage(_("Unable to open file."));
         break;
      }

      fflush(stdout);
      pi_file_install(f, m_PiSocket, 0);
      pi_file_close(f);
   }

   delete pd;

   // save time of last hotsync
   struct PilotUser U;
   if (dlp_ReadUserInfo(m_PiSocket, &U)>=0) {
      U.lastSyncPC = 0x00000000;  /* Hopefully unique constant, to tell
                                    any Desktop software that databases
                                    have been altered, and that a slow
                                    sync is necessary */
      U.lastSyncDate = U.successfulSyncDate = time(0);
      dlp_WriteUserInfo(m_PiSocket, &U);
   }
}


void
PalmOSModule::Restore()
{
   InstallFromDir(m_BackupDir, false);
}

void
PalmOSModule::AutoInstall()
{
   InstallFromDir(m_AutoInstallDir, true);
}

void
PalmOSModule::InstallFromDir(wxString directory, bool delFiles)
{
   PiConnection conn(this);
   if( ! IsConnected())
      return;

   DIR*   dir;
   int    ofile_total;
   char ** fnames = 0;

   dir = opendir(directory);
   if (dir <= 0) {
      ErrorMessage(_("Couldn´t access directory.")); //TODO : add directory name
      return;
   }

   ofile_total = CreateFileList(&fnames, dir);

   // we´ve finished reading the filelist
   closedir(dir);

   // install files
   InstallFiles(fnames, ofile_total, delFiles);
   DeleteFileList(fnames, ofile_total);
}

void
PalmOSModule::Install()
{
   int    ofile_total;
   char ** fnames = 0;

   MAppBase *mapp = m_MInterface->GetMApplication();


   // TODO: extend to "real" install routine, e.g.
   // let choose different files in different directories

   /*
   wxString wxPFileSelector("PalmOS/InstallFilesDlg",
                            _("Please pick databases to install"),
                            "","","","*.p*|*",wxOPEN|wxMULTIPLE,
                            (wxMainFrame *)mapp->TopLevelFrame());
   */

   wxFileDialog fileDialog((wxMainFrame *)mapp->TopLevelFrame() ,
                           _("Please pick databases to install"),
                           "","", "*.p*|*", wxOPEN|wxMULTIPLE);
   if ( fileDialog.ShowModal() != wxID_OK )
      return;

   wxArrayString fileNames;
   fileDialog.GetPaths(fileNames);

   ofile_total = (int) fileNames.Count();
   fnames = (char **) malloc( sizeof(char *) * ofile_total );
   for(int i = 0; i < ofile_total; i++)
      fnames[i] = strutil_strdup( fileNames[i] );

   // install files
   PiConnection conn(this);
   if( IsConnected())
      InstallFiles(fnames, ofile_total, false);
   DeleteFileList(fnames, ofile_total);
}


void
PalmOSModule::GetAddresses(PalmBook *palmbook)
{
   int    l;
   char   buf[0xffff];
   struct AddressAppInfo aai;
   PalmEntryGroup *rootGroup;

   if (!palmbook) {
/*
      AdbManager_obj adbManager;
      adbManager->LoadAll();

      String adbName;

      sprintf(adbName, "%s", "PalmOS-ADB (ReadOnly)");
      // There is no PalmBook, create a new one if possible
      palmbook = (PalmBook *)adbManager->CreateBook((String)"PalmOS Addressbook", NULL , &adbName);
*/
   }

   if (!palmbook) {
//      ErrorMessage(_("Couldn´t create new palmbook"));
      return;
   }

   /* Open the Address database, store access handle in db */
   if(dlp_OpenDB(m_PiSocket, 0, 0x80|0x40, "AddressDB", &m_AddrDB) < 0) {
      ErrorMessage(_("Unable to open AddressDB"));
      dlp_AddSyncLogEntry(m_PiSocket, "Unable to open AddressDB.\n");
      exit(1);
   }

   l = dlp_ReadAppBlock(m_PiSocket, m_AddrDB, 0, (unsigned char *)buf, 0xffff);
   unpack_AddressAppInfo(&aai, (unsigned char *)buf, l);

   rootGroup = (PalmEntryGroup*)(palmbook->GetGroup());
   if (!rootGroup) {
      ErrorMessage(_("ADB not correctly initialized!"));
      return;
   }

   createEntries(m_AddrDB, &aai, rootGroup);
}

int
PalmOSModule::createEntries(int db, struct AddressAppInfo * aai, PalmEntryGroup* p_TopGroup)
{
   struct Address a;
   char buf[0xffff];
   int category, attribute;
   int addrCount;

   // the categories of the Palm addressbook
   struct CategoryAppInfo cats = aai->category;

   // we create our own PalmEntryGroup for each used category
   PalmEntryGroup** catGroups = new PalmEntryGroup*[16];

   // check which category to create and do so
   for (int i = 0; i<16; i++)
      if (cats.name[i][0] != 0x0)
         catGroups[i] = (PalmEntryGroup*)p_TopGroup->CreateGroup(cats.name[i]);

   // read every single address
   dlp_ReadOpenDBInfo(m_PiSocket, db, &addrCount);

   /* TODO
   ** addrCount does now contain the number of addresses we are going
   ** to read. This should make it possible to display a statusbar
   ** so the user can see the progress of the db import.
   */

   int l, j;
   for(int i = 0;
       (j = dlp_ReadRecordByIndex(m_PiSocket, db, i, (unsigned char *)buf, 0, &l, &attribute, &category)) >= 0;
       i++)
   {
      // to which category/EntryGroup does this entry belong?
      PalmEntryGroup* p_Group = catGroups[category];

      // ignore deleted addresses
      if (attribute & dlpRecAttrDeleted)
         continue;

      unpack_Address(&a, (unsigned char *)buf, l);

      // create Name for entry
      String e_name = a.entry[0];                 // familyname

      if (a.entry[1] != NULL) {
         if (e_name == "")
            e_name = a.entry[1];
         else
            e_name = (String)a.entry[1] + ' ' + (String)a.entry[0];
      }

      if (e_name == "")
         e_name = a.entry[2];                    // company

      // create new entry ...
      PalmEntry *p_Entry = new PalmEntry(p_Group, e_name);

      if (!p_Entry)
         return -1;

      // ... and fill it
      p_Entry->Load(a);

      // add entry
      p_Group->AddEntry(p_Entry);
   }

   // everything worked fine
   return 0;
}

void
PalmOSModule::SendEMails(void)
{
   unsigned char buffer[0xffff];
   struct MailAppInfo tai;
   struct MailSyncPref mSPrefs;
   struct MailSignaturePref sigPrefs;

   memset(&tai, '\0', sizeof(struct MailAppInfo));
   memset(&mSPrefs, '\0', sizeof(struct MailSyncPref));


   /* Open the Mail database, store access handle in db */
   if(dlp_OpenDB(m_PiSocket, 0, 0x80|0x40, "MailDB", &m_MailDB) < 0)
   {
      ErrorMessage(_("Unable to open MailDB"));
      dlp_AddSyncLogEntry(m_PiSocket, (char *)_("Unable to open MailDB.\n"));
      return;
   }

   dlp_ReadAppBlock(m_PiSocket, m_MailDB, 0, buffer, 0xffff);
   unpack_MailAppInfo(&tai, buffer, 0xffff);

   mSPrefs.syncType = 0;
   mSPrefs.getHigh = 0;
   mSPrefs.getContaining = 0;
   mSPrefs.truncate = 8*1024;
   mSPrefs.filterTo = 0;
   mSPrefs.filterFrom = 0;
   mSPrefs.filterSubject = 0;

   if (pi_version(m_PiSocket) > 0x0100)
   {
      if (dlp_ReadAppPreference(m_PiSocket, makelong("mail"), 1, 1, 0xffff,
                                buffer, 0, 0)>=0)
      {
         StatusMessage("Got local backup mail preferences\n"); /* 2 for remote prefs */
         unpack_MailSyncPref(&mSPrefs, buffer, 0xffff);
      }
      else
      {
         Message("Unable to get mail preferences, trying current\n");
         if (dlp_ReadAppPreference(m_PiSocket, makelong("mail"), 1, 1, 0xffff,
                                   buffer, 0, 0)>=0)
         {
            Message("Got local current mail preferences\n"); /* 2 for remote prefs */
            unpack_MailSyncPref(&mSPrefs, buffer, 0xffff);
         }
         else
            Message("Couldn't get any mail preferences.\n");
      }

      if (dlp_ReadAppPreference(m_PiSocket, makelong("mail"), 3, 1, 0xffff,
                                buffer, 0, 0)>0)
      {
         unpack_MailSignaturePref(&sigPrefs, buffer, 0xffff);
      }

   }

#if 0
   String msg;
   msg.Printf(
      "Local Prefs: Sync=%d, High=%d, getc=%d, trunc=%d, to=|%s|, from=|%s|, subj=|%s|\n",
      mSPrefs.syncType, mSPrefs.getHigh, mSPrefs.getContaining,
      mSPrefs.truncate, mSPrefs.filterTo ? mSPrefs.filterTo : "<none>",
      mSPrefs.filterFrom ? mSPrefs.filterFrom : "<none>",
      mSPrefs.filterSubject ? mSPrefs.filterSubject : "<none>");
   Message(msg);

   msg.Printf("Signature: |%s|\n\n", sigPrefs.signature ? sigPrefs.signature : "<None>");
   Message(msg);
#endif

   /**
    **
    ** Check outbox of pilot and send messages out:
    **
    **/
   struct Mail mail;
   int attr;
   int size, len;
   recordid_t id;
   int numMessages = 0;
   int numMessagesTransferred = 0;

   for(int i = 0; ; i++, numMessages++)
   {
      len = dlp_ReadNextRecInCategory(m_PiSocket, m_MailDB, 1, buffer, &id, 0, &size,
                                      &attr);

      if(len < 0 ) break; // No more messages, we are done!

      if(   ( attr & dlpRecAttrDeleted)
            || ( attr & dlpRecAttrArchived) )
         continue; // skip deleted records
      unpack_Mail(&mail, buffer, len);

      SendMessageCC *smsg = m_MInterface->CreateSendMessageCC(m_Profile,Prot_SMTP);
      smsg->SetSubject(mail.subject);
      smsg->SetAddresses(mail.to, mail.cc, mail.bcc);
      if(mail.replyTo)
         smsg->AddHeaderEntry("Reply-To",mail.replyTo);
      if(mail.sentTo)
         smsg->AddHeaderEntry("Sent-To",mail.sentTo);
      smsg->AddPart(Message::MSG_TYPETEXT,
                    mail.body, strlen(mail.body));
      if(smsg->SendOrQueue())
      {
         String msg;
         msg.Printf(_("Transferred message %d: \"%s\" for \"%s\"."),
                    ++numMessagesTransferred,
                    mail.subject, mail.to);
         StatusMessage(msg);
         switch(m_Dispose)
         {
         case DISPOSE_KEEP:
            // do nothing
            break;
         case DISPOSE_DELETE:
            dlp_DeleteRecord(m_PiSocket, m_MailDB, 0, id);
            break;
         case DISPOSE_FILE:
         default:
            /* Rewrite into Filed category */
            dlp_WriteRecord(m_PiSocket, m_MailDB, attr, id, 3, buffer, size, 0);
         }
      }
      else
      {
         String tmpstr;
         tmpstr.Printf(_("Failed to transfer message\n\"%s\"\nfor \"%s\"."),
                       mail.subject, mail.to);
         ErrorMessage(tmpstr);
      }
      free_Mail(&mail);
      delete smsg;
   }
   if(numMessages)
   {
      String tmpstr;
      tmpstr.Printf(_("Transferred %d/%d messages."),
                    numMessagesTransferred, numMessages);
      dlp_AddSyncLogEntry(m_PiSocket, (char *)tmpstr.c_str());
      StatusMessage(tmpstr);
   }
   if(! numMessages)
   {
      StatusMessage(_("No messages found in PalmOS-Outbox."));
   }
}


/**
 **
 ** Copy messages from palmbox folder to handheld:
 **
 **/
void
PalmOSModule::StoreEMails(void)
{
   if(! m_PalmBox)
      return; // nothing to do

   unsigned char buffer[0xffff];

   UIdType count = 0;
   MailFolder *mf = m_MInterface->OpenFolder(MF_PROFILE_OR_FILE,m_PalmBox);
   if(! mf)
   {
      String tmpstr;
      tmpstr.Printf(_("Cannot open PalmOS synchronisation mailbox ´%s´"), m_PalmBox.c_str());
      ErrorMessage((tmpstr));
      return;
   }

   if(mf->CountMessages() == 0)
   {  // nothing to do
      mf->DecRef();
      return;
   }
   if(mf->Lock())
   {
      HeaderInfoList *hil = mf->GetHeaders();
      if(! hil)
      {
         mf->DecRef();
         return; // nothing to do
      }

      const HeaderInfo *hi;
      class Message *msg;
      for(UIdType i = 0; i < hil->Count(); i++)
      {
         hi = (*hil)[i];
         ASSERT(hi);
         if((hi->GetStatus() & MailFolder::MSG_STAT_DELETED) != 0)
         {
            String tmpstr;
            tmpstr.Printf(_("Skipping deleted message %lu/%lu"),
                          (unsigned long)(i+1),
                          (unsigned long)(hil->Count()));
            StatusMessage(tmpstr);
         }
         else
         {
            struct Mail t;
            t.to = 0;
            t.from = 0;
            t.cc = 0;
            t.bcc = 0;
            t.subject = 0;
            t.replyTo = 0;
            t.sentTo = 0;
            t.body = 0;
            t.dated = 0;

            msg = mf->GetMessage(hi->GetUId());
            ASSERT(msg);
            String tmpstr;
            tmpstr.Printf( _("Storing message %lu/%lu: %s"),
                           (unsigned long)(i+1),
                           (unsigned long)(hil->Count()),
                           msg->Subject().c_str());
            StatusMessage(tmpstr);
            String content;
            msg->GetHeaderLine("From",content);
            t.from = strutil_strdup(content);
            t.subject = strutil_strdup(msg->Subject());
            msg->GetHeaderLine("To",content);
            t.to = strutil_strdup(content);
            msg->Address(content, MAT_REPLYTO);
            t.replyTo = strutil_strdup(content);
            t.dated = 1;
            time_t tt;
            time(&tt);
            t.date = *localtime(&tt);

            content = "";
            for(int partNo = 0; partNo < msg->CountParts(); partNo++)
            {
               if(msg->GetPartType(partNo) == Message::MSG_TYPETEXT
                  && ( msg->GetPartMimeType(partNo) == "TEXT/PLAIN"
                       || msg->GetPartMimeType(partNo) == "TEXT/plain"
                     )
                  )
                  content << msg->GetPartContent(partNo);
               else
                  content << '[' << msg->GetPartMimeType(partNo) <<
                     ']' << '\n';
            }
            // msg->WriteToString(content, false /* headers */);
            String content2;
            const char *cptr = content.c_str();
            while(*cptr)
            {
               if(*cptr != '\r')
                  content2 << *cptr;
               cptr++;
            }
            strncpy((char *)buffer, content2, 0xffff);
            buffer[0xfffe] = '\0';
            t.body = (char *) malloc( strlen((char *)buffer) + 1 );
            strcpy(t.body, (char *)buffer);
            int len = pack_Mail(&t,  buffer, 0xffff);
            if(dlp_WriteRecord(m_PiSocket, m_MailDB, 0, 0, 0, buffer, len, 0) <= 0)
            {
               String tmpstr;
               tmpstr.Printf( _("Could not store message %lu/%lu: %s"),
                              (unsigned long)(i+1),
                              (unsigned long)(hil->Count()),
                              msg->Subject().c_str());
               ErrorMessage(tmpstr);
               count++;
            }
            else
               mf->DeleteMessage(msg->GetUId());
            free_Mail(&t);
            SafeDecRef(msg);
         }
      }
      if(count > 0)
      {
         String tmpstr;
         tmpstr.Printf(_("Stored %lu/%lu messages on PalmOS device."),
                       (unsigned long) count,
                       (unsigned long) hil->Count());
         StatusMessage((tmpstr));
      }
      SafeDecRef(hil);
   }
   else
   {
      ErrorMessage((_("Could not obtain lock for mailbox '%s'."),
                    mf->GetName().c_str()));
   }
   SafeDecRef(mf);
}

//---------------------------------------------------------
// The configuration dialog
//---------------------------------------------------------

#include "gui/wxDialogLayout.h"
#include "MHelp.h"


static ConfigValueDefault gs_ConfigValues [] =
{
   ConfigValueDefault(MP_MOD_PALMOS_SYNCMAIL, MP_MOD_PALMOS_SYNCMAIL_D),
   ConfigValueDefault(MP_MOD_PALMOS_BOX, MP_MOD_PALMOS_BOX_D),
   ConfigValueDefault(MP_MOD_PALMOS_DISPOSE, MP_MOD_PALMOS_DISPOSE_D),
   ConfigValueDefault(MP_MOD_PALMOS_SYNCADDR, MP_MOD_PALMOS_SYNCADDR_D),
   ConfigValueDefault(MP_MOD_PALMOS_BACKUP, MP_MOD_PALMOS_BACKUP_D),
   ConfigValueDefault(MP_MOD_PALMOS_BACKUPDIR, MP_MOD_PALMOS_BACKUPDIR_D),
   ConfigValueDefault(MP_MOD_PALMOS_BACKUP_SYNC, MP_MOD_PALMOS_BACKUP_SYNC_D),
   ConfigValueDefault(MP_MOD_PALMOS_BACKUP_INCREMENTAL, MP_MOD_PALMOS_BACKUP_INCREMENTAL_D),
   ConfigValueDefault(MP_MOD_PALMOS_BACKUP_ALL, MP_MOD_PALMOS_BACKUP_ALL_D),
   ConfigValueDefault(MP_MOD_PALMOS_BACKUP_EXCLUDELIST, MP_MOD_PALMOS_BACKUP_EXCLUDELIST_D),
   ConfigValueDefault(MP_MOD_PALMOS_PILOTDEV, MP_MOD_PALMOS_PILOTDEV_D),
   ConfigValueDefault(MP_MOD_PALMOS_SPEED, MP_MOD_PALMOS_SPEED_D),
   ConfigValueDefault(MP_MOD_PALMOS_LOCK, MP_MOD_PALMOS_LOCK_D),
   ConfigValueDefault(MP_MOD_PALMOS_SCRIPT1, MP_MOD_PALMOS_SCRIPT1_D),
   ConfigValueDefault(MP_MOD_PALMOS_SCRIPT2, MP_MOD_PALMOS_SCRIPT2_D)
};

static wxOptionsPage::FieldInfo gs_FieldInfos[] =
{
   { gettext_noop("Synchronise Mail"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Mailbox for exchange"), wxOptionsPage::Field_Text, 0},
   { gettext_noop("Mail disposal mode:keep:delete:file"), wxOptionsPage::Field_Combo,   0},
   { gettext_noop("Synchronise Addressbook"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Always do Backup"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Directory for backup files"), wxOptionsPage::Field_Text,    -1 },
   { gettext_noop("Delete no longer existing backup files"),
     wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Make incremental backup only"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Backup all databases"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Exclude these databases"), wxOptionsPage::Field_Text,    -1 },
   { gettext_noop("Pilot device"), wxOptionsPage::Field_Text,    -1 },
   // the speed values must be in sync with the ones in the speeds[]
   // array in GetConfig() further up:
   { gettext_noop("Connection speed:9600:19200:38400:57600:115200"), wxOptionsPage::Field_Combo,    -1 },
   { gettext_noop("Try to lock device"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Script to run before"), wxOptionsPage::Field_File,    -1 },
   { gettext_noop("Script to run after"), wxOptionsPage::Field_File,    -1 }
};

static
struct wxOptionsPageDesc  gs_OptionsPageDesc =
{
   gettext_noop("PalmOS module preferences"),
   "palmpilot",// image
   MH_MODULES_PALMOS_CONFIG,
   // the fields description
   gs_FieldInfos,
   gs_ConfigValues,
   WXSIZEOF(gs_FieldInfos)
};

void
PalmOSModule::Configure(void)
{
   ProfileBase * p= m_MInterface->CreateModuleProfile(MODULE_NAME);
   ShowCustomOptionsDialog(gs_OptionsPageDesc, p, NULL);
   p->DecRef();
}


#endif
