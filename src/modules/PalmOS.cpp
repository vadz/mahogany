/*-*- c++ -*-********************************************************
 * PalmOS - a PalmOS connectivity module for Mahogany               *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@gmx.net)                 *
 *                                                                  *
 *
 * Due to being based and linked against the pisock library, this
 * module is (C) under the GNU GENERIC PUBLIC LICENSE, Version 2.
 * This does not affect the copyright of any other parts of the
 * Mahogany source code and relates only to the PalmOS connectivity
 * module.
 *
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
#include "modules/PalmOS.h"

#include "Mversion.h"
#include "MInterface.h"
#include "Message.h"

#include "SendMessageCC.h"


#define DISPOSE_KEEP   0
#define DISPOSE_DELETE 1
#define DISPOSE_FILE   2

#define MP_MOD_PALM_BOX "PalmBox"
#define MP_MOD_PALM_BOX_D "PalmBox"
#define MP_MOD_PALM_DISPOSE "PalmDispose"
#define MP_MOD_PALM_DISPOSE_D DISPOSE_FILE

///------------------------------
/// MModule interface:
///------------------------------

class PalmOSModule : public MModule_PalmOS
{
   MMODULE_DEFINE(PalmOSModule)

   void Synchronise(void);
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
   
   void SendEMails(void);
   void StoreEMails(void);
   
   inline void ErrorMessage(const String &msg)
      { m_MInterface->Message(msg,NULL,"PalmOS module error!");wxYield(); }
   inline void Message(const String &msg)
      { m_MInterface->Message(msg,NULL,"PalmOS module"); wxYield(); }
   inline void StatusMessage(const String &msg)
      { m_MInterface->StatusMessage(msg);wxYield();}
   struct PalmOSModuleConfig
   {
      /// The device
      String m_PilotPort;

   } m_Config;
   MInterface * m_MInterface;

private:
   int m_PiSocket;
   int m_MailDB;
   ProfileBase *m_Profile;
   int m_Dispose;
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


void
PalmOSModule::GetConfig(void)
{
   m_Config.m_PilotPort = "/dev/pilot";

   ASSERT(m_Profile == NULL);
   ProfileBase * appConf = m_MInterface->GetGlobalProfile();
   m_Profile = m_MInterface->CreateProfile(
      appConf->readEntry(MP_MOD_PALM_BOX,MP_MOD_PALM_BOX_D));
   m_Dispose = READ_CONFIG(m_Profile, MP_MOD_PALM_DISPOSE);
}

MMODULE_IMPLEMENT(PalmOSModule,
                  "PalmOS",
                  MMODULE_INTERFACE_PALMOS,
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
}

PalmOSModule::~PalmOSModule()
{
   if(IsConnected())
      Disconnect();
   SafeDecRef(m_Profile);
}

// to prevent a warning from linux headers
#define __STRICT_ANSI__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pi-source.h>
#include <pi-socket.h>
#include <pi-file.h>
#include <pi-mail.h>
#include <pi-dlp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

bool
PalmOSModule::Connect(void)
{
   if(m_PiSocket == -1)
   {
      struct pi_sockaddr addr;
      struct PilotUser pilotUser;
      int rc;

      Message(_("Please press HotSync button and click on OK."));
      if (!(m_PiSocket = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP)))
      {
         ErrorMessage(_("Cannot get connection."));
         return false;
      }
      
      addr.pi_family = PI_AF_SLP;
      strncpy(addr.pi_device,
              m_Config.m_PilotPort.c_str(),
              sizeof(addr.pi_device)); 
      
      
      rc = pi_bind(m_PiSocket, (struct sockaddr*)&addr, sizeof(addr));
      if(rc == -1)
      {
         ErrorMessage(_("pi_bind() failed"));
         pi_close(m_PiSocket); m_PiSocket = -1;
         return false;
      }
      
      rc = pi_listen(m_PiSocket,1);
      if(rc == -1)
      {
         ErrorMessage(_("pi_listen() failed"));
         pi_close(m_PiSocket); m_PiSocket = -1;
         return false;
      }
      
      m_PiSocket = pi_accept(m_PiSocket, 0, 0);
      if(m_PiSocket == -1)
      {
         ErrorMessage(_("pi_accept() failed"));
         pi_close(m_PiSocket); m_PiSocket = -1;
         return false;
      }
      
      /* Ask the pilot who it is. */
      dlp_ReadUserInfo(m_PiSocket,&pilotUser);

      /* Tell user (via Pilot) that we are starting things up */
      dlp_OpenConduit(m_PiSocket);
   }
   return m_PiSocket != -1;
}

void
PalmOSModule::Disconnect(void)
{
   if(m_PiSocket != -1)
   {
      pi_close(m_PiSocket);
      m_PiSocket = -1;
   }
}

void PalmOSModule::Synchronise(void)
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
     SendEMails();
     StoreEMails();
     Disconnect();
     m_Profile->DecRef();
     m_Profile=NULL;
  }
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

  /* Check outbox of pilot: */
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
     Message(_("No messages found in PalmOS-Outbox."));
  }
}


void
PalmOSModule::StoreEMails(void)
{
   String palmbox = READ_CONFIG(m_Profile,MP_MOD_PALM_BOX);
   if(! palmbox)
      return; // nothing to do

   unsigned char buffer[0xffff];

   UIdType count = 0;
   MailFolder *mf = m_MInterface->OpenFolder(MF_PROFILE_OR_FILE,palmbox);
   if(! mf)
   {
      String tmpstr;
      tmpstr.Printf(_("Cannot open PalmOS synchronisation mailbox ´%s´"), palmbox.c_str());
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
            msg->WriteToString(content, true);
            strncpy(buffer, content, 0xffff);
            buffer[0xfffe] = '\0';
            t.body = (char *) malloc( strlen(buffer) + 1 );
            strcpy(t.body, buffer);
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
#endif
