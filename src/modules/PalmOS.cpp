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

class PalmBook;
#ifdef EXPERIMENTAL
#   include "adb/ProvPalm.h"
#endif

#define MODULE_NAME    "PalmOS"

#define DISPOSE_KEEP   0
#define DISPOSE_DELETE 1
#define DISPOSE_FILE   2

#define MP_MOD_PALMOS_BOX        "PalmBox"
#define MP_MOD_PALMOS_BOX_D      "PalmBox"
#define MP_MOD_PALMOS_DISPOSE    "Dispose"
#define MP_MOD_PALMOS_DISPOSE_D  2  // 0=keep 1=delete 2=file
#define MP_MOD_PALMOS_SYNCMAIL   "SyncMail"
#define MP_MOD_PALMOS_SYNCMAIL_D 1l
#define MP_MOD_PALMOS_SYNCADDR   "SyncAddr"
#define MP_MOD_PALMOS_SYNCADDR_D 1l
#define MP_MOD_PALMOS_PILOTDEV   "PilotDev"
#define MP_MOD_PALMOS_PILOTDEV_D "/dev/pilot"
#define MP_MOD_PALMOS_SPEED      "Speed"
#define MP_MOD_PALMOS_SPEED_D    "57600"
#define MP_MOD_PALMOS_LOCK       "Lock"
#define MP_MOD_PALMOS_LOCK_D 0l
#define MP_MOD_PALMOS_SCRIPT1    "Script1"
#define MP_MOD_PALMOS_SCRIPT1_D  ""
#define MP_MOD_PALMOS_SCRIPT2    "Script2"
#define MP_MOD_PALMOS_SCRIPT2_D  ""


// to prevent a warning from linux headers
#define __STRICT_ANSI__
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include <wx/log.h>
#include <wx/textfile.h>

#ifdef OS_SOLARIS
extern "C" {
   extern int kill(pid_t, int);
};
#endif

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
         int fd =
            open(m_LockFile,O_CREAT|O_EXCL,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
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
         if(fd != -1)
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
   void Synchronise(PalmBook *p_Book);
   void Configure(void);
   MMODULE_DEFINE(PalmOSModule)

private:
   /** PalmOS constructor.
       As the class has no usable interface, this doesn´t do much, but 
       it displays a small dialog to say hello.
       A real module would store the MInterface pointer for later
       reference and check if everything is set up properly.
   */
   PalmOSModule(MInterface *interface);
   ~PalmOSModule();
   
   bool Connect(void);
   void Disconnect(void);
   friend class PiConnection;

   bool IsConnected(void) const { return m_PiSocket != -1; }
   
   void GetConfig(void);

   void GetAddresses(PalmBook *p_Book);
 
   void SendEMails(void);
   void StoreEMails(void);

   inline void ErrorMessage(const String &msg)
      { m_MInterface->Message(msg,NULL,"PalmOS module error!");wxYield(); }
   inline void Message(const String &msg)
      { m_MInterface->Message(msg,NULL,"PalmOS module"); wxYield(); }
   inline void StatusMessage(const String &msg)
      { m_MInterface->StatusMessage(msg);wxYield();}
   MInterface * m_MInterface;

private:

#if EXPERIMENTAL
   int createEntries(int db, struct AddressAppInfo * aai, PalmEntryGroup* p_Group);
#endif

   int m_PiSocket;
   int m_MailDB;
   int m_AddrDB;
   ProfileBase *m_Profile;

   int m_Dispose;
   bool m_SyncMail, m_SyncAddr, m_LockPort;
   String m_PilotDev, m_Script1, m_Script2, m_PalmBox;
   int m_Speed;
   class wxDeviceLock *m_Lock;
};

/// small helper class
class PiConnection
{
public:
   PiConnection(class PalmOSModule *mi) { m=mi; m->Connect(); }
   ~PiConnection() { m->Disconnect(); }
private:
   PalmOSModule *m;
};


int
PalmOSModule::Entry(int arg, ...)
{
   switch(arg)
   {
      // GetFlags():
   case 0:
      return MMOD_FLAG_HASMAIN|MMOD_FLAG_HASCONFIG;
      // Main():
   case 1:
      Synchronise(NULL);
      return 0;
      // Configure():
   case 2:
      Configure();
      return 0;
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
   ASSERT(m_Profile == NULL);
   ProfileBase * appConf = m_MInterface->GetGlobalProfile();

   // mail related values get read from the PALMBOX mailfolder profile:
   m_Profile = m_MInterface->CreateProfile(
      appConf->readEntry(MP_MOD_PALMOS_BOX,MP_MOD_PALMOS_BOX_D));

   // all other values get read from the module profile:
   ProfileBase * p= m_MInterface->CreateModuleProfile(MODULE_NAME);
   
   m_SyncMail = (READ_CONFIG(p, MP_MOD_PALMOS_SYNCMAIL) != 0);
   m_SyncAddr = (READ_CONFIG(p, MP_MOD_PALMOS_SYNCADDR) != 0);
   m_LockPort = (READ_CONFIG(p, MP_MOD_PALMOS_LOCK) != 0);
   m_PilotDev = READ_CONFIG(p, MP_MOD_PALMOS_PILOTDEV);
   m_PalmBox = READ_CONFIG(p, MP_MOD_PALMOS_BOX);
   m_Dispose = READ_CONFIG(p,MP_MOD_PALMOS_DISPOSE);
   m_Speed = atoi(READ_CONFIG(p,MP_MOD_PALMOS_SPEED));
   m_Script1 = READ_CONFIG(p, MP_MOD_PALMOS_SCRIPT1);
   m_Script2 = READ_CONFIG(p, MP_MOD_PALMOS_SCRIPT2);
   p->DecRef();
   
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
{
   m_MInterface = minterface;
   m_PiSocket = -1;
   m_Profile = NULL;
   m_Lock = NULL;
}

PalmOSModule::~PalmOSModule()
{
   if(IsConnected())
      Disconnect();
   if(m_Lock) delete m_Lock;
   SafeDecRef(m_Profile);
}

bool
PalmOSModule::Connect(void)
{
   if(m_PiSocket == -1)
   {
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
      {
         if(! m_Lock->Lock())
            return false;
      }

      Message(_("Please press HotSync button and click on OK."));
      if (!(m_PiSocket = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP)))
      {
         ErrorMessage(_("Cannot get connection."));
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
         ErrorMessage(_("pi_bind() failed"));
         pi_close(m_PiSocket); m_PiSocket = -1;
         if(m_Lock->IsLocked()) m_Lock->Unlock();
         return false;
      }
      
      rc = pi_listen(m_PiSocket,1);
      if(rc == -1)
      {
         ErrorMessage(_("pi_listen() failed"));
         pi_close(m_PiSocket); m_PiSocket = -1;
         if(m_Lock->IsLocked()) m_Lock->Unlock();
         return false;
      }
      
      m_PiSocket = pi_accept(m_PiSocket, 0, 0);
      if(m_PiSocket == -1)
      {
         ErrorMessage(_("pi_accept() failed"));
         pi_close(m_PiSocket); m_PiSocket = -1;
         if(m_Lock->IsLocked()) m_Lock->Unlock();
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
      if(m_Lock->IsLocked()) m_Lock->Unlock();
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

void PalmOSModule::Synchronise(PalmBook *p_Book)
{
  if(m_MInterface->YesNoDialog(_("Do you want synchronise with your PalmOS device?")))
  {
     GetConfig();
     PiConnection conn(this);
     if(! IsConnected())
     {
        m_Profile->DecRef();
        m_Profile=NULL;
        return;
     }

     if(m_SyncMail)
     {
        SendEMails();
        StoreEMails();
        
        // here we close the opened database
        if (m_MailDB) {
            dlp_CloseDB(m_PiSocket, m_MailDB);
        }
     }

     if(m_SyncAddr)
     {
        // this does read the PalmADB and stores
        // the entries to the PalmBook

        GetAddresses(p_Book);
        
        // close the database again
        if (m_AddrDB) {
            dlp_CloseDB(m_PiSocket, m_AddrDB);
        }
     }
     
     Disconnect();
     m_Profile->DecRef();
     m_Profile=NULL;
  }
}

void
PalmOSModule::GetAddresses(PalmBook *palmbook)
{
#ifdef EXPERIMENTAL
   int    l;
   char   buf[0xffff];
   struct AddressAppInfo aai;
   PalmEntryGroup *rootGroup;
   
   if (!palmbook) {
      ErrorMessage(_("No PalmBook specified."));
      // TODO: Check for already existing PalmADB and update it, or
      // create one ourselves!
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
#endif
}

#if EXPERIMENTAL
int 
PalmOSModule::createEntries(int db, struct AddressAppInfo * aai, PalmEntryGroup* p_TopGroup)
{
  struct Address a;
  char buf[0xffff];
  int category, attribute;

  // the categories of the Palm addressbook
  struct CategoryAppInfo cats = aai->category;

  // we create our own PalmEntryGroup for each used category  
  PalmEntryGroup** catGroups = new PalmEntryGroup*[16];

  // check which category to create and do so
  for (int i = 0; i<16; i++)
    if (cats.name[i][0] != 0x0)
      catGroups[i] = (PalmEntryGroup*)p_TopGroup->CreateGroup(cats.name[i]);

  // read every single address
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
#endif

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

#include <wx/statbox.h>
#include <wx/layout.h>
#include "gui/wxDialogLayout.h"
#include "MHelp.h"

class wxPalmOSDialog : public wxOptionsPageSubdialog
{
public:
   wxPalmOSDialog(ProfileBase *profile, wxWindow *parent = NULL);

   // reset the selected options to their default values
   virtual bool TransferDataFromWindow();
   virtual bool TransferDataToWindow();
   bool WasChanged(void)
      {
         return
            m_OldSyncAddr != m_SyncAddr->GetValue()
            || m_OldSyncMail != m_SyncMail->GetValue()
            || m_OldPilotDev != m_PilotDev->GetValue()
            || m_OldSpeed != m_Speed->GetStringSelection()
            || m_OldDispose != m_Dispose->GetSelection()
            || m_OldLockPort != m_LockPort->GetValue()
            || m_OldScript1 != m_Script1->GetValue()
            || m_OldScript2 != m_Script2->GetValue()
            || m_OldPalmBox != m_PalmBox->GetValue();
      };
protected:
   wxCheckBox *m_SyncMail, *m_SyncAddr, *m_LockPort;
   wxChoice *m_Dispose, *m_Speed;
   wxTextCtrl *m_PilotDev, *m_Script1, *m_Script2, *m_PalmBox;
   
   bool m_OldSyncMail, m_OldSyncAddr, m_OldLockPort;
   wxString m_OldPilotDev, m_OldScript1, m_OldScript2, m_OldSpeed,
      m_OldPalmBox; 
   int m_OldDispose;
   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxPalmOSDialog, wxOptionsPageSubdialog)
//   EVT_BUTTON(-1, wxPalmOSDialog::OnButton)
//   EVT_CHECKBOX(-1, wxPalmOSDialog::OnButton)
END_EVENT_TABLE()


wxPalmOSDialog::wxPalmOSDialog(ProfileBase *profile,
                               wxWindow *parent)
   : wxOptionsPageSubdialog(profile,parent,
                            _("PalmOS module"),
                            "Modules/PalmOS/ConfDialog")
{
   wxStaticBox *box = CreateStdButtonsAndBox(_("PalmOS"), FALSE,
                                             MH_MODULES_PALMOS_CONFIG);
   wxLayoutConstraints *c;
   
   wxStaticText *stattext = new wxStaticText(this, -1,
                                             _("This module can synchronise your e-Mail and Addressbook\n"
                                               "contents with a PalmOS based device.")); 
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(box, wxTop, 6*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   stattext->SetConstraints(c);

   m_SyncMail = new wxCheckBox(this, -1, _("Synchronise Mail"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(stattext, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_SyncMail->SetConstraints(c);

   static wxString dispValues[] =
   {
      gettext_noop("kept in Outbox"),
      gettext_noop("deleted"),
      gettext_noop("filed")
   };
   static const size_t NUM_DISPVALUES  = WXSIZEOF(dispValues);


   wxStaticText *dispLabel = new wxStaticText(this, -1,_("Sent messages will be")); 
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.Below(m_SyncMail, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   dispLabel->SetConstraints(c);

   m_Dispose = new wxChoice(this, -1, wxDefaultPosition,
                            wxDefaultSize, NUM_DISPVALUES,dispValues);
   c = new wxLayoutConstraints;
   c->left.RightOf(dispLabel, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_SyncMail, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_Dispose->SetConstraints(c);


   m_SyncAddr = new wxCheckBox(this, -1, _("Synchronise Addressbooks"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_Dispose, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_SyncAddr->SetConstraints(c);

   wxStaticText *mboxLabel = new wxStaticText(this, -1,_("Mahogany Palm-mailbox"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.AsIs();
   c->top.Below(m_SyncAddr, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   mboxLabel->SetConstraints(c);

   m_PalmBox = new wxTextCtrl(this, -1);
   c = new wxLayoutConstraints;
   c->left.RightOf(mboxLabel, 4*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(mboxLabel, wxTop, 0);
   c->height.AsIs();
   m_PalmBox->SetConstraints(c);

   wxStaticText *devLabel = new wxStaticText(this, -1,_("Serial device"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 4*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.Below(m_PalmBox, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   devLabel->SetConstraints(c);

   m_PilotDev = new wxTextCtrl(this, -1);
   c = new wxLayoutConstraints;
   c->left.RightOf(devLabel, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(devLabel, wxTop, 0);
   c->height.AsIs();
   m_PilotDev->SetConstraints(c);
   
   wxStaticText *spdLabel = new wxStaticText(this, -1,_("Serial speed"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.AsIs();
   c->top.Below(m_PilotDev, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   spdLabel->SetConstraints(c);

   static wxString spdValues[] =
   {
      "9600","19200","38400","57600","115200","230400","460800"
   };
   static const size_t NUM_SPDVALUES  = WXSIZEOF(spdValues);

   m_Speed = new wxChoice(this, -1, wxDefaultPosition,
                            wxDefaultSize, NUM_SPDVALUES,spdValues);
   c = new wxLayoutConstraints;
   c->left.RightOf(devLabel, 2*LAYOUT_X_MARGIN);
   c->width.AsIs();
   c->top.SameAs(spdLabel, wxTop, 0);
   c->height.AsIs();
   m_Speed->SetConstraints(c);
   
   m_LockPort = new wxCheckBox(this, -1, _("Attempt to lock device"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->top.Below(m_Speed, 2*LAYOUT_Y_MARGIN);
   c->width.AsIs();
   c->height.AsIs();
   m_LockPort->SetConstraints(c);

   wxStaticText *scr1Label = new wxStaticText(this, -1,_("Optional script to run before opening device"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.AsIs();
   c->top.Below(m_LockPort, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   scr1Label->SetConstraints(c);
   
   m_Script1 = new wxTextCtrl(this, -1);
   c = new wxLayoutConstraints;
   c->left.RightOf(devLabel, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(scr1Label, wxTop, 0);
   c->height.AsIs();
   m_Script1->SetConstraints(c);
   
   wxStaticText *scr2Label = new wxStaticText(this, -1,_("Optional script to run after closing device"));
   c = new wxLayoutConstraints;
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.AsIs();
   c->top.Below(m_Script1, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   scr2Label->SetConstraints(c);
   
   m_Script2 = new wxTextCtrl(this, -1);
   c = new wxLayoutConstraints;
   c->left.RightOf(devLabel, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(scr2Label, wxTop, 0);
   c->height.AsIs();
   m_Script2->SetConstraints(c);

   SetDefaultSize(280, 220, FALSE /* not minimal */);
   TransferDataToWindow();
   // Register old values:
   m_OldSyncMail = m_SyncMail->GetValue();
   m_OldSyncAddr = m_SyncAddr->GetValue();
   m_OldPilotDev = m_PilotDev->GetValue();
   m_OldLockPort = m_LockPort->GetValue();
   m_OldDispose = m_Dispose->GetSelection();
   m_OldSpeed = m_Speed->GetStringSelection();
   m_OldPilotDev = m_PilotDev->GetValue();
   m_OldScript1 = m_Script1->GetValue();
   m_OldScript2 = m_Script2->GetValue();
   m_OldPalmBox = m_PalmBox->GetValue();
}

bool
wxPalmOSDialog::TransferDataToWindow()
{
   m_SyncMail->SetValue( READ_CONFIG(GetProfile(), MP_MOD_PALMOS_SYNCMAIL) != 0);
   m_SyncAddr->SetValue( READ_CONFIG(GetProfile(), MP_MOD_PALMOS_SYNCADDR) != 0);
   m_LockPort->SetValue( READ_CONFIG(GetProfile(), MP_MOD_PALMOS_LOCK) != 0);
   m_PilotDev->SetValue( READ_CONFIG(GetProfile(), MP_MOD_PALMOS_PILOTDEV));
   m_PalmBox->SetValue( READ_CONFIG(GetProfile(), MP_MOD_PALMOS_BOX));
   m_Dispose->SetSelection(READ_CONFIG(GetProfile(),MP_MOD_PALMOS_DISPOSE));
   m_Speed->SetStringSelection(READ_CONFIG(GetProfile(),MP_MOD_PALMOS_SPEED));
   m_Script1->SetValue(READ_CONFIG(GetProfile(), MP_MOD_PALMOS_SCRIPT1));
   m_Script2->SetValue(READ_CONFIG(GetProfile(), MP_MOD_PALMOS_SCRIPT2));
   return TRUE;
}

bool
wxPalmOSDialog::TransferDataFromWindow()
{
   GetProfile()->writeEntry(MP_MOD_PALMOS_SYNCMAIL, m_SyncMail->GetValue());
   GetProfile()->writeEntry(MP_MOD_PALMOS_SYNCADDR, m_SyncAddr->GetValue());
   GetProfile()->writeEntry(MP_MOD_PALMOS_LOCK, m_LockPort->GetValue());
   GetProfile()->writeEntry(MP_MOD_PALMOS_PILOTDEV,m_PilotDev->GetValue());
   GetProfile()->writeEntry(MP_MOD_PALMOS_BOX, m_PalmBox->GetValue());
   GetProfile()->writeEntry(MP_MOD_PALMOS_DISPOSE,m_Dispose->GetSelection());
   GetProfile()->writeEntry(MP_MOD_PALMOS_SPEED,m_Speed->GetStringSelection());
   GetProfile()->writeEntry(MP_MOD_PALMOS_SCRIPT1,m_Script1->GetValue());
   GetProfile()->writeEntry(MP_MOD_PALMOS_SCRIPT1, m_Script2->GetValue());
   return TRUE;
}



static ConfigValueDefault gs_ConfigValues [] =
{
   ConfigValueDefault(MP_MOD_PALMOS_SYNCMAIL, MP_MOD_PALMOS_SYNCMAIL_D)
};

static wxOptionsPage::FieldInfo gs_FieldInfos[] =
{
   { gettext_noop("Synchronise Mail"), wxOptionsPage::Field_Bool,    -1 }
};

static
struct wxOptionsPageDesc  gs_OptionsPageDesc = 
{
   gettext_noop("PalmOS module preferences"),
      "",// image
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

//   ShowCustomOptionsDialog(gs_OptionsPageDesc, NULL);


   wxPalmOSDialog dlg(p);
   (void) dlg.ShowModal();
   p->DecRef();
   
}


#endif
