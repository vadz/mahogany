/*-*- c++ -*-********************************************************
 * PalmOS -  PalmOS connectivity module for Mahogany                *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)           *
 *                  Daniel Seifert (dseifert@gmx.net)               *
 *                                                                  *
 * $Id$              *
 *******************************************************************/

/**
   This module supports synchronisation, backup and install facilities
   for PalmOS based handheld devices.


   AvantGo / MAL server support:

   To get this enabled, "libmal" is needed, which is a small wrapper
   library around AvantGo's MAL source distribution.
   The same library is used by the jpilot-plugin for MAL
   synchronisation.

   That source must reside in the ../../extra/src/libmal directory.
   After placing it there, the config.cache file in the toplevel
   directory must be removed and ./configure run there again. It will
   then detect the existence of libmal and set everything up properly
   to compile and link it in. MAL support in this file will
   automatically be activated then.

**/

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "strutil.h"
#   include "Mdefaults.h"

#   include "wx/stattext.h"
#   include <wx/menu.h>
#   include <wx/log.h>
#   include <wx/filedlg.h>
#endif // USE_PCH

#include "MDialogs.h"
#include "gui/wxMenuDefs.h"
#include "MMainFrame.h"
#include "HeaderInfo.h"

// we can't compile an empty library as it was done before as then you get
// constant error messages when looking for a module telling that it is not a
// valid Mahogany module (as it doesn't then even have GetMModuleProperties()
// function), so instead just abort
#ifndef USE_PISOCK
   #error "This module shouldn't be compiled if you're not using Palm."
#endif

#if HAVE_PI_ACCEPT_TO
#   include <wx/minifram.h>
#endif

#include <wx/dir.h>

#include "adb/AdbManager.h"
#include "adb/ProvPalm.h"

#ifdef HAVE_LIBMAL
extern "C"
{
#   include "libmal.h"
}
#endif

//broken, causes PalmOS to crash:
//#define PALMOS_SYNCTIME
#undef PALMOS_SYNCTIME

// one of pisock headers defines struct Address and MInterface.h below
// includes our Address.h which defines class Address, so define this to
// prevent our Address from being defined now
#define M_DONT_DEFINE_ADDRESS

#include "MModule.h"
#include "Mversion.h"
#include "MInterface.h"
#include "Message.h"

#include "SendMessage.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxOptionsDlg.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxMDialogs.h"

#define MODULE_NAME    "PalmOS"

#define DISPOSE_KEEP   0l
#define DISPOSE_DELETE 1l
#define DISPOSE_FILE   2l

// Config file defines

#define MP_MOD_PALMOS_BOX                    "PalmBox"
#define MP_MOD_PALMOS_BOX_D                  "PalmBox"
#define MP_MOD_PALMOS_DISPOSE                "Dispose"
#define MP_MOD_PALMOS_DISPOSE_D              DISPOSE_FILE
#define MP_MOD_PALMOS_SYNCMAIL               "SyncMail"
#define MP_MOD_PALMOS_SYNCMAIL_D             1l
#define MP_MOD_PALMOS_SYNCADDR               "SyncAddr"
#define MP_MOD_PALMOS_SYNCADDR_D             1l
#define MP_MOD_PALMOS_BACKUP                 "Backup"
#define MP_MOD_PALMOS_BACKUP_D               0l
#define MP_MOD_PALMOS_BACKUPDIR              "BackupDir"
#define MP_MOD_PALMOS_BACKUPDIR_D            ""
#define MP_MOD_PALMOS_BACKUP_SYNC            "BackupSync"
#define MP_MOD_PALMOS_BACKUP_SYNC_D          0l
#define MP_MOD_PALMOS_BACKUP_INCREMENTAL     "BackupIncremental"
#define MP_MOD_PALMOS_BACKUP_INCREMENTAL_D   1l
#define MP_MOD_PALMOS_BACKUP_ALL             "BackupAll"
#define MP_MOD_PALMOS_BACKUP_ALL_D           0l
#define MP_MOD_PALMOS_BACKUP_EXCLUDELIST     "BackupExclude"
#define MP_MOD_PALMOS_BACKUP_EXCLUDELIST_D   ""
#define MP_MOD_PALMOS_AUTOINSTALL            "AutoInstall"
#define MP_MOD_PALMOS_AUTOINSTALL_D          0l
#define MP_MOD_PALMOS_AUTOINSTALLDIR         "AutoInstallDir"
#define MP_MOD_PALMOS_AUTOINSTALLDIR_D       "/tmp"
#define MP_MOD_PALMOS_PILOTDEV               "PilotDev"
#define MP_MOD_PALMOS_PILOTDEV_D             "/dev/pilot"
#define MP_MOD_PALMOS_SPEED                  "Speed"
#define MP_MOD_PALMOS_SPEED_D                3
#define MP_MOD_PALMOS_LOCK                   "Lock"
#define MP_MOD_PALMOS_LOCK_D                 0l
#define MP_MOD_PALMOS_SCRIPT1                "Script1"
#define MP_MOD_PALMOS_SCRIPT1_D              ""
#define MP_MOD_PALMOS_SCRIPT2                "Script2"
#define MP_MOD_PALMOS_SCRIPT2_D              ""

#define MP_MOD_PALMOS_SYNCTIME               "SyncTime"
#define MP_MOD_PALMOS_SYNCTIME_D             0l
#define MP_MOD_PALMOS_SYNCMAL                "Sync to MAL server"
#define MP_MOD_PALMOS_SYNCMAL_D              0l
#define MP_MOD_PALMOS_MAL_USE_PROXY          "MALUseProxy"
#define MP_MOD_PALMOS_MAL_USE_SOCKS          "MALUseSocks"
#define MP_MOD_PALMOS_MAL_PROXY_HOST         "MALProxy"
#define MP_MOD_PALMOS_MAL_PROXY_PORT         "MALProxyPort"
#define MP_MOD_PALMOS_MAL_PROXY_LOGIN        "MALProxyLogin"
#define MP_MOD_PALMOS_MAL_PROXY_PASSWORD     "MALProxyPw"
#define MP_MOD_PALMOS_MAL_SOCKS_HOST   "MALSocksHost"
#define MP_MOD_PALMOS_MAL_SOCKS_PORT   "MALSocksPort"
#define MP_MOD_PALMOS_MAL_USE_PROXY_D        0l
#define MP_MOD_PALMOS_MAL_PROXY_HOST_D       ""
#define MP_MOD_PALMOS_MAL_PROXY_PORT_D       80l
#define MP_MOD_PALMOS_MAL_PROXY_LOGIN_D      ""
#define MP_MOD_PALMOS_MAL_PROXY_PASSWORD_D   ""
#define MP_MOD_PALMOS_MAL_USE_SOCKS_D        0l
#define MP_MOD_PALMOS_MAL_SOCKS_HOST_D ""
#define MP_MOD_PALMOS_MAL_SOCKS_PORT_D 80l

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

#include <wx/textfile.h>

#ifdef OS_SOLARIS
extern "C" {
   extern int kill(pid_t, int);
};
#endif

// ----------------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------------


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



///----------------------------------------------------------------------------
/// MModule interface:
///----------------------------------------------------------------------------


class PalmOSModule : public MModule
{
   /** Override the Entry() function to allow Main() and Config() functions. */
   virtual int Entry(int arg, ...);
   void Synchronise(PalmBook *pBook);
   void Configure(void);
   MMODULE_DEFINE();

private:
   /** PalmOS constructor.
       As the class has no usable interface, this doesn't do much, but
       it displays a small dialog to say hello.
   */
   PalmOSModule(MInterface *mi);

   /// register with the main frame
   bool RegisterWithMainFrame();

   ~PalmOSModule();

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
#ifdef HAVE_LIBMAL
   void SyncMAL(void);
#endif

   void Backup(void);

   void Install(void);
   void AutoInstall(void)
      { InstallFromDir(m_AutoInstallDir, true); }
   void Restore(void)
      { InstallFromDir(m_BackupDir, false); }
   void InstallFiles(wxArrayString &fnames, bool delFiles);
   void InstallFromDir(wxString directory, bool delFiles);

   inline void ErrorMessage(const String &msg)
      { m_MInterface->MessageDialog(msg,NULL,"PalmOS module error!");wxYield(); }
   inline void Message(const String &msg)
      { m_MInterface->MessageDialog(msg,NULL,"PalmOS module"); wxYield(); }
   inline void StatusMessage(const String &msg)
      { m_MInterface->StatusMessage(msg);wxYield();}

   int GetPalmDBList(wxArrayString &dblist, bool getFilenames);

   int createEntries(int db, struct AddressAppInfo * aai, PalmEntryGroup* p_Group);
   int CreateFileList(wxArrayString &list, const wxString& directory);
   void RemoveFromList(wxArrayString &list, wxString &name)
      { if (list.Index(name) >= 0) list.Remove(list.Index(name)); }

   int m_PiSocket;
   int m_MailDB;
   int m_AddrDB;
   Profile *m_Profile;

   // variables to store configuration values
   int   m_Dispose;
   int   m_Speed;
   bool  m_SyncMail, m_SyncAddr, m_Backup, m_LockPort, m_SyncTime;
   bool  m_IncrBackup, m_BackupSync, m_BackupAll;
   bool  m_AutoInstall;
   String m_PilotDev, m_Script1, m_Script2, m_PalmBox;
   String m_BackupExcludeList;
   String m_BackupDir;
   String m_AutoInstallDir;
#ifdef HAVE_LIBMAL
   bool m_SyncMAL,
      m_MALUseProxy,
      m_MALUseSocks;
   String m_MALProxyHost,
      m_MALProxyLogin,
      m_MALProxyPassword,
      m_MALSocksHost;
   long m_MALProxyPort,
      m_MALSocksPort;
#endif

   class wxDeviceLock *m_Lock;
};

static MInterface *gs_MInterface = NULL;

#ifdef HAVE_LIBMAL

/* Function prototype missing in current libmal.h */
extern "C" { int register_printErrorHook (printErrorHook); }

/* Define function to print status messages for MAL synchronisation. */
static
int MAL_PrintFunc(bool errorflag, const char * format, va_list args)
{
   CHECK(gs_MInterface != NULL, 0, _T("no MInterface"));
   static wxString msg;
   int rc;
   if(! errorflag && format[0] == '.')
   {
      wxString tmp;
      rc = tmp.PrintfV(format, args);
      msg << tmp;
   }
   else
   {
      rc = msg.PrintfV(format, args);
      msg = "MAL sync: " + msg;
   }

   if(errorflag)
      gs_MInterface->MessageDialog(msg,NULL,"MAL synchronisation error!");
   else
   {

      gs_MInterface->StatusMessage(msg);
      wxYield();
   }
   return rc;
}

static
int MAL_PrintStatusFunc(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   return MAL_PrintFunc(FALSE, format, ap);
}

static
int MAL_PrintErrorFunc(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   return MAL_PrintFunc(TRUE, format, ap);
}
#endif


// ----------------------------------------------------------------------------
// PiConnection - small helper class to automate connection
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

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
      case MMOD_FUNC_INIT:
         return RegisterWithMainFrame() ? 0 : -1;

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
   Profile * appConf = m_MInterface->GetGlobalProfile();

   // mail related values get read from the PALMBOX mailfolder profile:
   if(m_Profile == NULL)
   {
      m_Profile = m_MInterface->CreateProfile(
         appConf->readEntry(MP_MOD_PALMOS_BOX,MP_MOD_PALMOS_BOX_D));
   }

   // all other values get read from the module profile:
   Profile * p= m_MInterface->CreateModuleProfile(MODULE_NAME);

   // must be in sync with the combobox values in config table
   //further down:
   static int speeds[] = { 9600,19200,38400,57600,115200 };

   m_Backup     = (READ_CONFIG_MOD(p, MP_MOD_PALMOS_BACKUP) != 0);
   m_BackupDir  = READ_CONFIG_MOD(p, MP_MOD_PALMOS_BACKUPDIR);
   m_SyncMail   = (READ_CONFIG_MOD(p, MP_MOD_PALMOS_SYNCMAIL) != 0);
   m_SyncTime   = (READ_CONFIG_MOD(p, MP_MOD_PALMOS_SYNCTIME) != 0);
   m_SyncAddr   = (READ_CONFIG_MOD(p, MP_MOD_PALMOS_SYNCADDR) != 0);
   m_LockPort   = (READ_CONFIG_MOD(p, MP_MOD_PALMOS_LOCK) != 0);
   m_PilotDev   = READ_CONFIG_MOD(p, MP_MOD_PALMOS_PILOTDEV);
   m_PalmBox    = READ_CONFIG_MOD(p, MP_MOD_PALMOS_BOX);
   m_Dispose    = READ_CONFIG_MOD(p, MP_MOD_PALMOS_DISPOSE);
   m_Speed      = READ_CONFIG_MOD(p, MP_MOD_PALMOS_SPEED);
   m_IncrBackup = READ_CONFIG_MOD(p, MP_MOD_PALMOS_BACKUP_INCREMENTAL);
   m_BackupSync = READ_CONFIG_MOD(p, MP_MOD_PALMOS_BACKUP_SYNC);
   m_BackupAll  = (READ_CONFIG_MOD(p, MP_MOD_PALMOS_BACKUP_ALL) != 0);
   m_BackupExcludeList = READ_CONFIG_MOD(p, MP_MOD_PALMOS_BACKUP_EXCLUDELIST);
   m_AutoInstall    = READ_CONFIG_MOD(p, MP_MOD_PALMOS_AUTOINSTALL);
   m_AutoInstallDir = READ_CONFIG_MOD(p, MP_MOD_PALMOS_AUTOINSTALLDIR);
#ifdef HAVE_LIBMAL
   m_SyncMAL = READ_CONFIG_MOD(p, MP_MOD_PALMOS_SYNCMAL);
   m_MALUseProxy = READ_CONFIG_MOD(p, MP_MOD_PALMOS_MAL_USE_PROXY);
   m_MALUseSocks = READ_CONFIG_MOD(p, MP_MOD_PALMOS_MAL_USE_SOCKS);
   m_MALProxyHost = READ_CONFIG_MOD(p, MP_MOD_PALMOS_MAL_PROXY_HOST);
   m_MALProxyPort = READ_CONFIG_MOD(p, MP_MOD_PALMOS_MAL_PROXY_PORT);
   m_MALProxyLogin = READ_CONFIG_MOD(p, MP_MOD_PALMOS_MAL_PROXY_LOGIN);
   m_MALProxyPassword = READ_CONFIG_MOD(p, MP_MOD_PALMOS_MAL_PROXY_PASSWORD);
   m_MALSocksHost = READ_CONFIG_MOD(p, MP_MOD_PALMOS_MAL_SOCKS_HOST);
   m_MALSocksPort = READ_CONFIG_MOD(p, MP_MOD_PALMOS_MAL_SOCKS_PORT);

   /* set up MAL status reporting callback */
   register_printStatusHook (MAL_PrintStatusFunc);
   register_printErrorHook (MAL_PrintErrorFunc);
#endif

   if(m_Speed < 0  || m_Speed > (signed) WXSIZEOF(speeds))
      m_Speed = speeds[0];
   else
      m_Speed = speeds[m_Speed];

   m_Script1 = READ_CONFIG_MOD(p, MP_MOD_PALMOS_SCRIPT1);
   m_Script2 = READ_CONFIG_MOD(p, MP_MOD_PALMOS_SCRIPT2);
   p->DecRef();

   if(m_BackupDir.Length()
      && m_BackupDir[m_BackupDir.Length()-1] != DIR_SEPARATOR)
      m_BackupDir << DIR_SEPARATOR;

   if(m_AutoInstallDir.Length()
      && m_AutoInstallDir[m_AutoInstallDir.Length()-1] != DIR_SEPARATOR)
      m_AutoInstallDir << DIR_SEPARATOR;

   String dev;
   dev = m_PilotDev;
   if(strncmp(m_PilotDev,"/dev/",5)==0)
      dev = m_PilotDev.c_str()+5;
   if(m_Lock) delete m_Lock;
   m_Lock = new wxDeviceLock(dev);
}

MMODULE_BEGIN_IMPLEMENT(PalmOSModule,
                        "PalmOS",
                        "HandheldSynchronise",
                        "PalmOS module",
                        "1.00")
   MMODULE_PROP("description",
                _("This module provides PalmOS connectivity"))
   MMODULE_PROP("author", "Karsten Ballüder <karsten@phy.hw.ac.uk>")
MMODULE_END_IMPLEMENT(PalmOSModule)


///----------------------------------------------------------------------------
/// Own functionality:
///----------------------------------------------------------------------------

/* static */
MModule *
PalmOSModule::Init(int version_major, int version_minor,
                   int version_release, MInterface *minterface,
                   int *errorCode)
{
   if(! MMODULE_SAME_VERSION(version_major, version_minor,
                             version_release))
   {
      if(errorCode)
         *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }

   return new PalmOSModule(minterface);
}


PalmOSModule::PalmOSModule(MInterface *minterface)
   : MModule()
{
   SetMInterface(minterface);

   m_PiSocket = -1;
   m_Profile = NULL;
   m_Lock = NULL;

   ASSERT(gs_MInterface == NULL);
   gs_MInterface = m_MInterface;
}

bool
PalmOSModule::RegisterWithMainFrame()
{
   wxMenu * palmOsMenu = new wxMenu("", wxMENU_TEAROFF);
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_SYNC, _("&Synchronise"));
   palmOsMenu->Break();
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_BACKUP, _("&Backup"));
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_RESTORE, _("&Restore"));
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_INSTALL, _("&Install"));
   palmOsMenu->Break();
   palmOsMenu->Append(WXMENU_MODULES_PALMOS_CONFIG, _("&Configure"));

   MAppBase *mapp = m_MInterface->GetMApplication();

   wxMFrame *mframe = mapp->TopLevelFrame();
   CHECK( mframe, false, _T("can't init PalmOS module - no main window") );

   ((wxMainFrame *)mapp->TopLevelFrame())->AddModulesMenu(_("&PalmOS Module"),
                                                          _("Functionality to interact with your PalmOS based palmtop."),
                                                          palmOsMenu,
                                                          -1);
   return true;
}

PalmOSModule::~PalmOSModule()
{
   if(IsConnected())
      Disconnect();

   if(m_Lock) delete m_Lock;
   if(m_Profile) m_Profile->DecRef();
   ASSERT(gs_MInterface != NULL);
   gs_MInterface = NULL;
}

#if 0
#if wxUSE_THREADS && defined( OS_UNIX )
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
#endif //0

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
            msg.Printf(_("Executing command '%s' returned an error code (%d)."),
                       m_Script1.c_str(), rc);
            ErrorMessage(msg);
         }
      }

      if(m_LockPort)
         if(! m_Lock->Lock())
            return false;

      if (!(m_PiSocket = pi_socket(PI_AF_PILOT, PI_SOCK_STREAM, PI_PF_PADP)))
      {
         ErrorMessage(_("Failed to connect to PalmOS device:\nCould not open socket."));
         if(m_Lock->IsLocked()) m_Lock->Unlock();
         return false;
      }

      addr.pi_family = PI_AF_PILOT;
      strncpy(addr.pi_device,
              m_PilotDev.c_str(),
              sizeof(addr.pi_device));

#ifdef HAVE_PI_SETMAXSPEED
      pi_setmaxspeed(m_PiSocket, m_Speed, 0 /* overclock */);
#endif

      StatusMessage(_("Connecting to Palm ..."));
      rc = pi_bind(m_PiSocket, (struct sockaddr*)&addr, sizeof(addr));
      if(rc == -1)
      {
         ErrorMessage(_("Failed to connect to PalmOS device:\nCould not bind to socket."));
         pi_close(m_PiSocket); m_PiSocket = -1;
         if (m_Lock->IsLocked())
            m_Lock->Unlock();

         return false;
      }

      rc = pi_listen(m_PiSocket,1);
      if(rc == -1)
      {
         ErrorMessage(_("Failed to connect to PalmOS device:\nListening on socket failed."));
         pi_close(m_PiSocket); m_PiSocket = -1;
         if (m_Lock->IsLocked())
            m_Lock->Unlock();

         return false;
      }

      StatusMessage(_("Please press HotSync button and click on OK!"));
#if HAVE_PI_ACCEPT_TO
      wxMiniFrame *mini = new wxMiniFrame(NULL,-1, "Mahogany");
      wxPanel *p = new wxPanel(mini, -1);
      (void) new wxStaticText(p,-1,
                              _("Please press the HotSync button..."));
      p->Fit(); mini->Fit();
      mini->Show(TRUE);
      wxSafeYield(mini);
      m_PiSocket = pi_accept_to(m_PiSocket, 0, 0, 5000);
      delete mini;
#else
      Message(_("Please press HotSync button and click on OK."));
      // horrible old interface, hangs if fails:
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

#ifdef PALMOS_SYNCTIME
      /* set Palm's time */
      if (m_SyncTime)
      {
         time_t   localTime;
         time(&localTime);
         dlp_SetSysDateTime(m_PiSocket, localTime);
      }
#endif
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
            msg.Printf(_("Executing command '%s' returned an error code (%d)."),
                       m_Script2.c_str(), rc);
            ErrorMessage(msg);
         }
      }
   }

   StatusMessage(_T(""));
}

// ----------------------------------------------------------------------------
// General Synchronisation calls
// ----------------------------------------------------------------------------

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
   /* If we are called with pBook == NULL, then the user triggered the
      synchronisation via the menu and we don't ask for
      confirmation. If pBook != NULL, we check before trying to sync
      as we are called from adb code. */
   if(pBook == NULL ||
      m_MInterface->YesNoDialog(
         _("Do you want synchronise with your PalmOS device?")))
   {
      PiConnection conn(this);
      if(! IsConnected())
         return;

      if(pBook != NULL)
         // only asked to synchronise the addressbook
         SyncAddresses(pBook);
      else
      {
         if(m_SyncMail)
            SyncMail();
         if(m_SyncAddr)
            SyncAddresses(pBook);
         if(m_Backup)
            Backup();
         if(m_AutoInstall)
            AutoInstall();
#ifdef HAVE_LIBMAL
      // we do this last as there is most potential for something to
      // go wrong and the error handling in libmal is _very_
      // dodgy... (KB)
         if(m_SyncMAL)
            SyncMAL();
#endif
      }
      m_Profile->DecRef();
      m_Profile=NULL;
   }
}


/* Protect = and / in filenames */
static void protect_name(wxString &s)
{
   s.Replace("=", "=3D");
   s.Replace("/", "=2F");
   s.Replace(",", "=2C");
   s.Replace("\x0A", "=0A");
   s.Replace("\x0D", "=0D");
}

int
PalmOSModule::CreateFileList(wxArrayString &list, const wxString& directory)
{
   wxDir dir;
   if ( !dir.Open(directory) )
      return -1;

   int filecount = 0;

   list.Empty();

   StatusMessage(_("Reading local directory ..."));
   wxString name;
   for ( bool cont = dir.GetFirst(&name); cont; cont = dir.GetNext(&name) )
   {
      // TODO: check that we got a regular file
      wxString extension = name.AfterLast('.');
      if(extension != "pdb" && extension != "prc" &&
         extension != "prc" && extension != "PRC")
      {
         wxString msg;
         msg.Printf(_("Ignoring file '%s' with unknown extension."),
                    name.c_str());
         StatusMessage(_(msg));
         continue;
      }

      // now we need the full pathname:
      name = directory + '/' + name;

      // now we open the file and see whether it is really a file for
      // the Palm. If yes, then we remember the filename.
      struct pi_file *f = pi_file_open((char*)name.c_str());
      if (f > 0)
      {
         pi_file_close(f);
         list.Add(name);
         ++filecount;
      }
   }

   return filecount;
}


int
PalmOSModule::GetPalmDBList(wxArrayString &dblist, bool getFilenames)
{
   dblist.Empty();

   // connect to the Palm
   PiConnection conn(this);
   if( ! IsConnected())
      return 0;

   int i = 0;
   int max = 0;

   while (true) {
      struct DBInfo info;
      if (dlp_ReadDBList(m_PiSocket, 0, 0x80, i, &info) < 0)
         break;

      StatusMessage(_("Reading list of databases ..."));

      wxString dbname;
      dbname.Printf("%s", info.name);

      if (getFilenames) {
         // construct filename
         protect_name(dbname);
         if (info.flags & dlpDBFlagResource)
            dbname.append(".prc");
         else
            dbname.append(".pdb");
      }

      dblist.Add(dbname);

      ++max;
      i = info.index + 1;
   }

   return max;
}


// ----------------------------------------------------------------------------
// Backup
// ----------------------------------------------------------------------------


void
PalmOSModule::Backup(void)
{
   // connect to the Palm
   PiConnection conn(this);
   if( ! IsConnected())
      return;

   // Read original list of files in the backup dir
   wxArrayString orig_files;
   if ( CreateFileList(orig_files, m_BackupDir) == -1 )
   {
      String msg;
      msg.Printf(_("Could not access backup directory '%s'."),
                 m_BackupDir.c_str());
      ErrorMessage(msg);
   }

   // count files on the palm
   int max = 0;
   int i = 0;

   StatusMessage(_("Reading list of databases ..."));
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

   StatusMessage(_("Backing up..."));
   // for each file on the palm do ...
   while (true) {
      struct DBInfo   info;
      struct pi_file *f;
      struct utimbuf  times;
      struct stat     statb;
      wxString name, fname;

      if (dlp_ReadDBList(m_PiSocket, 0, 0x80, i, &info) < 0)
         break;

      i = info.index + 1;

      if (dlp_OpenConduit(m_PiSocket) < 0) {
         ErrorMessage(_("Exiting on cancel, backup process incomplete!"));
         delete pd;
         return;
      }

      // don't keep DB_open flag
      info.flags &= 0xff;

      // construct filename
      fname.Printf("%s", info.name);
      protect_name(fname);

      if (info.flags & dlpDBFlagResource)
         fname.Append(".prc");
      else
         fname.Append(".pdb");

      name.Printf("%s%s", m_BackupDir.c_str(), fname.c_str());

      // update progress dialog, exit on "cancel"
      if( ! pd->Update(max++, name) )
      {
         delete pd;
         return;
      }

      // check whether this might be a database we have to ignore
      if (m_IncrBackup)
         if (stat(name.c_str(), &statb) == 0)
            if (info.modifyDate == statb.st_mtime) {
               RemoveFromList(orig_files, name);
               continue;
            }

      if (!m_BackupAll && !(info.flags & dlpDBFlagBackup)) {
         RemoveFromList(orig_files, name);
         continue;
      }

      // check exclude list
      int pos;

      pos = m_BackupExcludeList.find(fname.c_str(), 0);
      if (pos >= 0) {
         // the found string is only valid, if it is either surrounded by commata or
         // with string start or end
         bool valid = false;

         if (pos == 0)
            valid = true;  // first entry in ExcludeList
         else
            if ((m_BackupExcludeList.Mid(pos-1, 1)).Cmp(",") == 0)
               if (pos == 1)
                  valid = true;  // should never happen (string starts with kommata)
               else
                  if ((m_BackupExcludeList.Mid(pos-2, 1)).Cmp("\\") != 0)
                     valid = true; // before entry is a kommata that is not escaped

         // now we've made sure that the entry in the exclude list started correctly,
         // so let's check whether it ends correctly, too
         if (valid)
            if (m_BackupExcludeList.Len() != fname.Len())
               if ((m_BackupExcludeList.Mid(pos + fname.Len(),1)).Cmp(",") != 0)
                  valid = false; // it was neither the last entry nor did it end with komma

         if (valid) {
            if (orig_files.Index(name) >= 0)
               orig_files.Remove(orig_files.Index(name));
            continue;
         }
      }

      // create file
      f = pi_file_create((char*)name.c_str(), &info);

      if (f == 0) {
         wxString msg;
         msg.Printf(_("Unable to create file %s!"),
                    (char*)name.c_str());
         ErrorMessage(_(msg));
         continue;
      }

      if (pi_file_retrieve(f, m_PiSocket, 0) < 0) {
         wxString msg;
         msg.Printf(_("Unable to backup database %s!"),
                    name.c_str());
         ErrorMessage(_(msg));
      }

      pi_file_close(f);

      /* Note: This is no guarantee that the times on the host system
         actually match the GMT times on the Pilot. We only check to
         see whether they are the same or different, and do not treat
         them as real times. */
      times.actime = info.createDate;
      times.modtime = info.modifyDate;
      utime(name, &times);

      RemoveFromList(orig_files, name);
   }

   delete pd;

   // Remaining files are outdated
   if (m_BackupSync) {
      for (unsigned int j = 0; j < orig_files.GetCount(); j++)
         unlink(orig_files.Item(j).c_str());  // delete
   }

   // All files are backed up now.
   orig_files.Empty();
}

struct db {
   char           name[256];
   int            flags;
   unsigned long  creator;
   unsigned long  type;
   int            maxblock;
};

int
pdbCompare(struct db* d1, struct db* d2)
{
   /* types of 'appl' sort later than other types */
   if (d1->creator == d2->creator)
      if (d1->type != d2->type) {
         if(d1->type == pi_mktag('a','p','p','l'))
            return 1;
         if(d2->type == pi_mktag('a','p','p','l'))
            return -1;
      }

   return d1->maxblock < d2->maxblock;
}


// ----------------------------------------------------------------------------
// Install routines
// ----------------------------------------------------------------------------


void
PalmOSModule::InstallFiles(wxArrayString &fnames, bool delFile)
{
   struct DBInfo info;
   struct db * db[256];
   int    dbcount = 0;
   int    size;
   int    i, max;
   struct pi_file * f;
   MProgressDialog *pd;

   StatusMessage(_("Checking files to install ..."));
   for (unsigned int j = 0; j < fnames.GetCount(); j++) {
      db[dbcount] = (struct db*)malloc(sizeof(struct db));

      // remember filename
      sprintf(db[dbcount]->name, "%s", fnames.Item(j).c_str());

      f = pi_file_open(db[dbcount]->name);

      if (f==0) {
         wxString msg;
         msg.Printf(_("Could not open file %s or file invalid."),
                    db[dbcount]->name);
         ErrorMessage(_(msg));
         continue;
      }

      pi_file_get_info(f, &info);

      db[dbcount]->creator  = info.creator;
      db[dbcount]->type     = info.type;
      db[dbcount]->flags    = info.flags;
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
   }

   // sort list in alphabetical order
   for (i=0; i < dbcount; i++)
      for (int j = i+1; j<dbcount; j++)
         if (pdbCompare(db[i], db[j]) > 0)
         {
            struct db * temp = db[i];
            db[i] = db[j];
            db[j] = temp;
         }

   pd = new MProgressDialog(_("Palm Install"), _("Installing files"),
                            dbcount, NULL, false, true);

   // Install files
   StatusMessage(_("Installing Files ..."));
   for (i=0; i < dbcount; i++) {

      if (!pd->Update(i, db[i]->name)) {
         delete pd;
         return;
      }

      if ( dlp_OpenConduit(m_PiSocket) < 0) {
         ErrorMessage(_("Exiting on cancel, all data not restored/installed"));
         delete pd;
         return;
      }

      f = pi_file_open(db[i]->name);
      if (f == 0) {
         wxString msg;
         msg.Printf(_("Could not open file %s or file invalid!"),
                    db[dbcount]->name);
         ErrorMessage(_(msg));
         continue;
      }

      fflush(stdout);
      wxString msg;
      msg.Printf(_("Installing file '%s'..."), db[i]->name);
      StatusMessage(_(msg));
      //FIXME: check returncode
      pi_file_install(f, m_PiSocket, 0);
      pi_file_close(f);

      // shall we delete the file after deletion?
      if (delFile) {
         unlink(db[i]->name);
      }
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
PalmOSModule::InstallFromDir(wxString directory, bool delFiles)
{
   PiConnection conn(this);
   if( ! IsConnected())
      return;

   wxArrayString fnames;

   int ofile_total = CreateFileList(fnames, directory);
   if ( ofile_total == -1 )
   {
      wxString msg;
      msg.Printf(_("Could not access directory %s!"),
                 directory.c_str());
      ErrorMessage(_(msg));
      return;
   }

   // install files
   if (ofile_total > 0)
      InstallFiles(fnames, delFiles);

   fnames.Empty();
}

void
PalmOSModule::Install()
{
   MAppBase *mapp = m_MInterface->GetMApplication();

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

   wxArrayString fnames;
   fileDialog.GetPaths(fnames);

   if (fnames.GetCount() == 0)
      return;

   // install files
   PiConnection conn(this);
   if( IsConnected())
      InstallFiles(fnames, false);
   fnames.Empty();
}


// ----------------------------------------------------------------------------
// Synchronise Addresses
// ----------------------------------------------------------------------------


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
//      ErrorMessage(_("Couldn't create new palmbook"));
      return;
   }

   /* Open the Address database, store access handle in db */
   if(dlp_OpenDB(m_PiSocket, 0, 0x80|0x40, "AddressDB", &m_AddrDB) < 0) {
      ErrorMessage(_("Could not open AddressDB."));
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



// ----------------------------------------------------------------------------
// Synchronise EMails
// ----------------------------------------------------------------------------


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

      SendMessage *smsg = m_MInterface->CreateSendMessage(m_Profile,Prot_SMTP);
      smsg->SetSubject(mail.subject);
      smsg->SetAddresses(mail.to, mail.cc, mail.bcc);
      if(mail.replyTo)
         smsg->AddHeaderEntry("Reply-To",mail.replyTo);
      if(mail.sentTo)
         smsg->AddHeaderEntry("Sent-To",mail.sentTo);
      smsg->AddPart(MimeType::TEXT,
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
   MailFolder *mf = m_MInterface->OpenMailFolder(m_PalmBox);
   if(! mf)
   {
      String tmpstr;
      tmpstr.Printf(_("Cannot open PalmOS synchronisation mailbox '%s'"), m_PalmBox.c_str());
      ErrorMessage((tmpstr));
      return;
   }

   if(mf->IsEmpty())
   {  // nothing to do
      mf->DecRef();
      return;
   }
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
         content = msg->GetAddressesString(MAT_REPLYTO);
         t.replyTo = strutil_strdup(content);
         t.dated = 1;
         time_t tt;
         time(&tt);
         t.date = *localtime(&tt);

         content = "";
         for(int partNo = 0; partNo < msg->CountParts(); partNo++)
         {
            String mimeType = msg->GetPartMimeType(partNo);

            if ( msg->GetPartType(partNo) == MimeType::TEXT &&
                  mimeType.Upper() == "TEXT/PLAIN" )
               content << (char *)msg->GetPartContent(partNo);
            else
               content << '[' << mimeType << ']' << '\n';
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
   SafeDecRef(mf);
}

#ifdef HAVE_LIBMAL
void
PalmOSModule::SyncMAL(void)
{
   PiConnection conn(this);
   if ( ! IsConnected())
      return;

   PalmSyncInfo * pInfo = syncInfoNew();
   if (NULL == pInfo)
      return;

   /* set up MAL status reporting callback */
   register_printStatusHook (MAL_PrintStatusFunc);
   register_printErrorHook (MAL_PrintErrorFunc);
   /* are we using a proxy? */
   if(m_MALUseProxy)
   {
      StatusMessage(_("Setting up MAL proxy..."));
      setHttpProxy ((char *) m_MALProxyHost.c_str());
      setHttpProxyPort ( m_MALProxyPort);
      setProxyUsername ((char *) m_MALProxyLogin.c_str());
      setProxyPassword ((char *) m_MALProxyPassword.c_str());
   }

   /* are we using a SOCKS proxy? */
   if(m_MALUseSocks)
   {
      StatusMessage(_("Setting up SOCKS proxy..."));
      setSocksProxy ((char *) m_MALSocksHost.c_str());
      setSocksProxyPort ( m_MALSocksPort );
   }
   StatusMessage(_("Synchronising MAL server/AvantGo..."));
   malsync (m_PiSocket, pInfo);
   syncInfoFree(pInfo);
   StatusMessage("");
}
#endif



//---------------------------------------------------------
// The configuration dialog
//---------------------------------------------------------

#include "gui/wxDialogLayout.h"
#include "MHelp.h"


static ConfigValueDefault gs_ConfigValues1 [] =
{
   ConfigValueDefault(MP_MOD_PALMOS_SYNCMAIL, MP_MOD_PALMOS_SYNCMAIL_D),
#ifdef PALMOS_SYNCTIME
   ConfigValueDefault(MP_MOD_PALMOS_SYNCTIME, MP_MOD_PALMOS_SYNCTIME_D),
#endif
//   ConfigValueDefault(MP_MOD_PALMOS_SYNCADDR, MP_MOD_PALMOS_SYNCADDR_D),
      ConfigValueDefault(MP_MOD_PALMOS_BACKUP, MP_MOD_PALMOS_BACKUP_D),
#ifdef HAVE_LIBMAL
      ConfigValueDefault(MP_MOD_PALMOS_SYNCMAL, MP_MOD_PALMOS_SYNCMAL_D),
#endif
      ConfigValueNone(),
      ConfigValueDefault(MP_MOD_PALMOS_AUTOINSTALL, MP_MOD_PALMOS_AUTOINSTALL_D),
      ConfigValueDefault(MP_MOD_PALMOS_AUTOINSTALLDIR, MP_MOD_PALMOS_AUTOINSTALLDIR_D),
      ConfigValueDefault(MP_MOD_PALMOS_PILOTDEV, MP_MOD_PALMOS_PILOTDEV_D),
      ConfigValueDefault(MP_MOD_PALMOS_SPEED, MP_MOD_PALMOS_SPEED_D),
      ConfigValueDefault(MP_MOD_PALMOS_BOX, MP_MOD_PALMOS_BOX_D),
      ConfigValueDefault(MP_MOD_PALMOS_DISPOSE, MP_MOD_PALMOS_DISPOSE_D),
      ConfigValueDefault(MP_MOD_PALMOS_BACKUPDIR, MP_MOD_PALMOS_BACKUPDIR_D),
      ConfigValueDefault(MP_MOD_PALMOS_BACKUP_SYNC, MP_MOD_PALMOS_BACKUP_SYNC_D),
      ConfigValueDefault(MP_MOD_PALMOS_BACKUP_INCREMENTAL, MP_MOD_PALMOS_BACKUP_INCREMENTAL_D),
      ConfigValueDefault(MP_MOD_PALMOS_BACKUP_ALL, MP_MOD_PALMOS_BACKUP_ALL_D),
      ConfigValueDefault(MP_MOD_PALMOS_BACKUP_EXCLUDELIST, MP_MOD_PALMOS_BACKUP_EXCLUDELIST_D),
      };

static wxOptionsPage::FieldInfo gs_FieldInfos1[] =
{
   { gettext_noop("Synchronise Mail"), wxOptionsPage::Field_Bool,    -1 },
#ifdef PALMOS_SYNCTIME
   { gettext_noop("Synchronise Time"), wxOptionsPage::Field_Bool, -1 },
#endif
   //   { gettext_noop("Synchronise Addressbook"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Always do Backup on sync"), wxOptionsPage::Field_Bool,    -1 },
#ifdef HAVE_LIBMAL
   { gettext_noop("Sync to MAL server (e.g. AvantGo)"), wxOptionsPage::Field_Bool,    -1 },
#endif
   { gettext_noop("The Auto-Install function will automatically\n"
         "install all databases from a given directory\n"
         "on the PalmPilot during synchronisation."), wxOptionsPage::Field_Message, -1},
   { gettext_noop("Do Auto-Install"), wxOptionsPage::Field_Bool,  -1 },
   { gettext_noop("Directory for Auto-Install"), wxOptionsPage::Field_Dir, -1 },
   { gettext_noop("Pilot device"), wxOptionsPage::Field_Text,    -1 },
   // the speed values must be in sync with the ones in the speeds[]
   // array in GetConfig() further up:
   { gettext_noop("Connection speed:9600:19200:38400:57600:115200"), wxOptionsPage::Field_Combo,    -1 },
   { gettext_noop("Mailbox for exchange"), wxOptionsPage::Field_Folder, -1},
   { gettext_noop("Mail disposal mode:keep:delete:file"), wxOptionsPage::Field_Combo,   -1},
   { gettext_noop("Directory for backup files"), wxOptionsPage::Field_Dir,    -1 },
   { gettext_noop("Delete backups of no longer existing databases"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Backup only modified databases"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Force total backup of all databases"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("Exclude these databases"), wxOptionsPage::Field_Text,    -1 },
};


static ConfigValueDefault gs_ConfigValues2 [] =
{
   ConfigValueDefault(MP_MOD_PALMOS_LOCK, MP_MOD_PALMOS_LOCK_D),
   ConfigValueNone(),
   ConfigValueDefault(MP_MOD_PALMOS_SCRIPT1, MP_MOD_PALMOS_SCRIPT1_D),
   ConfigValueDefault(MP_MOD_PALMOS_SCRIPT2, MP_MOD_PALMOS_SCRIPT2_D),
#ifdef HAVE_LIBMAL
   ConfigValueNone(),
   ConfigValueDefault(MP_MOD_PALMOS_MAL_USE_PROXY, MP_MOD_PALMOS_MAL_USE_PROXY_D),
   ConfigValueDefault(MP_MOD_PALMOS_MAL_PROXY_HOST, MP_MOD_PALMOS_MAL_PROXY_HOST_D),
   ConfigValueDefault(MP_MOD_PALMOS_MAL_PROXY_PORT, MP_MOD_PALMOS_MAL_PROXY_PORT_D),
   ConfigValueDefault(MP_MOD_PALMOS_MAL_PROXY_LOGIN, MP_MOD_PALMOS_MAL_PROXY_LOGIN_D),
   ConfigValueDefault(MP_MOD_PALMOS_MAL_PROXY_PASSWORD, MP_MOD_PALMOS_MAL_PROXY_PASSWORD_D),
   ConfigValueDefault(MP_MOD_PALMOS_MAL_USE_SOCKS, MP_MOD_PALMOS_MAL_USE_SOCKS_D),
   ConfigValueDefault(MP_MOD_PALMOS_MAL_SOCKS_HOST, MP_MOD_PALMOS_MAL_SOCKS_HOST_D),
   ConfigValueDefault(MP_MOD_PALMOS_MAL_SOCKS_PORT, MP_MOD_PALMOS_MAL_SOCKS_PORT_D),
#endif
};

#define MALPROXY 3
static wxOptionsPage::FieldInfo gs_FieldInfos2[] =
{
   { gettext_noop("Try to lock device"), wxOptionsPage::Field_Bool,    -1 },
   { gettext_noop("The following two settings can run other\n"
         "programs or scripts just before and after\n"
         "the synchronisation, e.g. to start/stop other\n"
         "services using the serial port. Leave them\n"
         "empty unless you know what you are doing."), wxOptionsPage::Field_Message, -1},
   { gettext_noop("Script to run before"), wxOptionsPage::Field_File,    -1 },
   { gettext_noop("Script to run after"), wxOptionsPage::Field_File,    -1 },
#ifdef HAVE_LIBMAL
   { gettext_noop("The following options affect only the\n"
         "MAL/AvantGo synchronisation:"), wxOptionsPage::Field_Message, -1},
   { gettext_noop("Use Proxy host for MAL"), wxOptionsPage::Field_Bool,  -1 },
   { gettext_noop("  Proxy host"), wxOptionsPage::Field_Text, MALPROXY },
   { gettext_noop("  Proxy port number"), wxOptionsPage::Field_Number,MALPROXY},
   { gettext_noop("  Proxy login"), wxOptionsPage::Field_Text,  MALPROXY },
   { gettext_noop("  Proxy password"), wxOptionsPage::Field_Passwd, MALPROXY },
   { gettext_noop("Use SOCKS server for MAL access"), wxOptionsPage::Field_Bool,  -1},
   { gettext_noop("  SOCKS host"), wxOptionsPage::Field_Text,  MALPROXY+5 },
   { gettext_noop("  SOCKS port number"), wxOptionsPage::Field_Number,  MALPROXY+5 },
#endif
};


static
struct wxOptionsPageDesc  gs_OptionsPageDesc1 =
wxOptionsPageDesc(
   gettext_noop("PalmOS module preferences"),
   "palmpilot",// image
   MH_MODULES_PALMOS_CONFIG,
   // the fields description
   gs_FieldInfos1,
   gs_ConfigValues1,
   WXSIZEOF(gs_FieldInfos1)
   );

static
struct wxOptionsPageDesc  gs_OptionsPageDesc2 =
wxOptionsPageDesc(
   gettext_noop("Advanced settings"),
   "palmpilot",// image
   MH_MODULES_PALMOS_CONFIG,
   // the fields description
   gs_FieldInfos2,
   gs_ConfigValues2,
   WXSIZEOF(gs_FieldInfos2)
   );

static
struct wxOptionsPageDesc g_Pages[] =
{
   gs_OptionsPageDesc1, gs_OptionsPageDesc2
};

void
PalmOSModule::Configure(void)
{
   Profile * p= m_MInterface->CreateModuleProfile(MODULE_NAME);
   ShowCustomOptionsDialog(2,g_Pages, p, NULL);
   p->DecRef();
}

