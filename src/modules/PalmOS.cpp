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
#endif

#ifdef USE_PISOCK
#include "MModule.h"

#include "Mversion.h"
#include "MInterface.h"

///------------------------------
/// MModule interface:
///------------------------------

class PalmOSModule : public MModule
{
   MMODULE_DEFINE(PalmOSModule)
private:
   /** PalmOS constructor.
       As the class has no usable interface, this doesn´t do much, but 
       it displays a small dialog to say hello.
       A real module would store the MInterface pointer for later
       reference and check if everything is set up properly.
   */
   PalmOSModule(MInterface *interface);

   void GetConfig(void);
   
   void ListEMails(void);

   inline void ErrorMessage(const String &msg)
      { m_MInterface->Message(msg,NULL,"PalmOS module error!"); }
   inline void Message(const String &msg)
      { m_MInterface->Message(msg,NULL,"PalmOS module"); }
   struct PalmOSModuleConfig
   {
      /// The device
      String m_PilotPort;

   } m_Config;
   MInterface * m_MInterface;
};

void
PalmOSModule::GetConfig(void)
{
   m_Config.m_PilotPort = "/dev/pilot";
}

MMODULE_IMPLEMENT(PalmOSModule,
                  "PalmOS",
                  "none",
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
   GetConfig();
   ListEMails();
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

void
PalmOSModule::ListEMails(void)
{
  struct pi_sockaddr addr;
  struct PilotUser pilotUser;
  unsigned char buffer[0xffff];
  struct MailAppInfo tai;
  struct MailSyncPref mSPrefs;
  struct MailSignaturePref sigPrefs;
  
  int sd, db, rc;

  Message(_("Please press HotSync button and click on OK."));
  if (!(sd = pi_socket(PI_AF_SLP, PI_SOCK_STREAM, PI_PF_PADP)))
  {
    ErrorMessage(_("Cannot get connection."));
    return;
  }

  addr.pi_family = PI_AF_SLP;
  strncpy(addr.pi_device,
          m_Config.m_PilotPort.c_str(),
          sizeof(addr.pi_device)); 

  
  rc = pi_bind(sd, (struct sockaddr*)&addr, sizeof(addr));
  if(rc == -1)
  {
     ErrorMessage(_("pi_bind() failed"));
     pi_close(sd);
    return;
  }

  rc = pi_listen(sd,1);
  if(rc == -1)
  {
    ErrorMessage(_("pi_listen() failed"));
    pi_close(sd);
    return;
  }

  sd = pi_accept(sd, 0, 0);
  if(sd == -1)
  {
     ErrorMessage(_("pi_accept() failed"));
     pi_close(sd);
     return;
  }
  
  memset(&tai, '\0', sizeof(struct MailAppInfo));
  memset(&mSPrefs, '\0', sizeof(struct MailSyncPref));

  /* Ask the pilot who it is. */
  dlp_ReadUserInfo(sd,&pilotUser);
  
  /* Tell user (via Pilot) that we are starting things up */
  dlp_OpenConduit(sd);
  
  /* Open the Mail database, store access handle in db */
  if(dlp_OpenDB(sd, 0, 0x80|0x40, "MailDB", &db) < 0)
  {
     ErrorMessage(_("Unable to open MailDB"));
     dlp_AddSyncLogEntry(sd, (char *)_("Unable to open MailDB.\n"));
     pi_close(sd);
     return;
  }
  
  dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
  unpack_MailAppInfo(&tai, buffer, 0xffff);
  
  mSPrefs.syncType = 0;
  mSPrefs.getHigh = 0;
  mSPrefs.getContaining = 0;
  mSPrefs.truncate = 8*1024;
  mSPrefs.filterTo = 0;
  mSPrefs.filterFrom = 0;
  mSPrefs.filterSubject = 0;
  
  if (pi_version(sd) > 0x0100)
  {
     if (dlp_ReadAppPreference(sd, makelong("mail"), 1, 1, 0xffff,
                               buffer, 0, 0)>=0)
     {
        Message("Got local backup mail preferences\n"); /* 2 for remote prefs */
        unpack_MailSyncPref(&mSPrefs, buffer, 0xffff);
     }
     else
    {
       Message("Unable to get mail preferences, trying current\n");
       if (dlp_ReadAppPreference(sd, makelong("mail"), 1, 1, 0xffff,
                                 buffer, 0, 0)>=0)
       {
          Message("Got local current mail preferences\n"); /* 2 for remote prefs */
          unpack_MailSyncPref(&mSPrefs, buffer, 0xffff);
       }
       else
          Message("Couldn't get any mail preferences.\n");
    }
    
     if (dlp_ReadAppPreference(sd, makelong("mail"), 3, 1, 0xffff,
                               buffer, 0, 0)>0)
     {
        unpack_MailSignaturePref(&sigPrefs, buffer, 0xffff);
     }

  }

  String msg;
  msg.Printf(
     "Local Prefs: Sync=%d, High=%d, getc=%d, trunc=%d, to=|%s|, from=|%s|, subj=|%s|\n",
     mSPrefs.syncType, mSPrefs.getHigh, mSPrefs.getContaining,
     mSPrefs.truncate, mSPrefs.filterTo ? mSPrefs.filterTo : "<none>", 
     mSPrefs.filterFrom ? mSPrefs.filterFrom : "<none>",
     mSPrefs.filterSubject ? mSPrefs.filterSubject : "<none>");
  Message(msg);
  
  msg.Printf("AppInfo SigPrefsnature: |%s|\n\n", sigPrefs.signature ?
             sigPrefs.signature : "<None>");
  Message(msg);
  

  /* Check outbox of pilot: */
  struct Mail mail;
  int attr;
  int size, len;
  recordid_t id;
  int numMessages = 0;
  
  for(int i = 0; ; i++, numMessages++)
  {
     len = dlp_ReadNextRecInCategory(sd, db, 1, buffer, &id, 0, &size, 
                                     &attr);

     if(len < 0 ) break; // No more messages, we are done!

     if(   ( attr & dlpRecAttrDeleted)
           || ( attr & dlpRecAttrArchived) )
        continue; // skip deleted records
     unpack_Mail(&mail, buffer, len);

     msg.Printf("Found message to send:\n"
                "Subject: %s\n"
                "From: %s\n"
                "To: %s\n"
                "CC: %s\n"
                "BCC: %s\n"
                "Reply-To: %s\n"
                "SentTo: %s\n"
                "Contents:\n%s",
                mail.subject,
                mail.from,
                mail.to,
                mail.cc,
                mail.bcc,
                mail.replyTo,
                mail.sentTo,
                mail.body,
                mail.subject);
     Message(msg);
     free_Mail(&mail);
  }
  if(! numMessages)
     Message(_("No messages found in PalmOS-Outbox."));
  pi_close(sd);
}

#endif
