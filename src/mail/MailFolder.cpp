/*-*- c++ -*-********************************************************
 * MailFolder class: handling of Unix mail folders                  *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "MailFolder.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "guidef.h"    // only for high-level functions
#   include "strutil.h"
#   include "Profile.h"
#endif

#include "MDialogs.h" // for the password prompt...

#include "Mdefaults.h"

#include "Message.h"

#include "MailFolder.h"
#include "MailFolderCC.h"

#include <wx/file.h>

MailFolder *
MailFolder::OpenFolder(int typeAndFlags,
                       String const &i_name,
                       ProfileBase *parentProfile,
                       String const &i_server,
                       String const &i_login,
                       String const &i_passwd)
{
   // open a folder:
   ProfileBase *profile = NULL;
   String login, passwd, name, server;
   FolderType type = GetFolderType(typeAndFlags);
   int flags = GetFolderFlags(typeAndFlags);

   if ( type == MF_PROFILE || type == MF_PROFILE_OR_FILE )
   {
      profile = ProfileBase::CreateProfile(i_name, parentProfile);
      CHECK(profile, NULL, "can't create profile");   // return if it fails
      int typeflags = READ_CONFIG(profile, MP_FOLDER_TYPE);
      if(typeflags == MF_ILLEGAL)  // no such profile!
      {
         type = MF_FILE;
         flags = 0;
         name = i_name;
      }
      else
      {
         type = GetFolderType(typeflags);
         flags = GetFolderFlags(typeflags);
         login = READ_CONFIG(profile, MP_FOLDER_LOGIN);
         passwd = strutil_decrypt(READ_CONFIG(profile, MP_FOLDER_PASSWORD));
         name = READ_CONFIG(profile, MP_FOLDER_PATH);
         /* name can be empty, i.e. for imap */
      }
   }
   else // type != PROFILE
   {
      profile = ProfileBase::CreateEmptyProfile(parentProfile);
      CHECK(profile, NULL, "can't create profile");   // return if it fails

      server = i_server;
      name = i_name;
      passwd = i_passwd;
      login = i_login;
   }

   switch( type )
   {
      case MF_NNTP:
         if(strutil_isempty(i_server))
            server = READ_CONFIG(profile, MP_NNTPHOST);
         break;

      case MF_FILE:
      case MF_MH:
         if( strutil_isempty(name) )
            name = READ_CONFIG(profile, MP_FOLDER_PATH);
         if(name == "INBOX")
            type = MF_INBOX;
         name = strutil_expandfoldername(name);
         break;

      case MF_POP:
      case MF_IMAP:
         if(strutil_isempty(server))
            server = READ_CONFIG(profile, MP_FOLDER_HOST);
         if(strutil_isempty(login))
            login = READ_CONFIG(profile, MP_FOLDER_LOGIN);
         if(strutil_isempty(passwd))
            passwd = strutil_decrypt(READ_CONFIG(profile, MP_FOLDER_PASSWORD));
         if(strutil_isempty(name))
            name = READ_CONFIG(profile, MP_FOLDER_PATH);
         break;

      case MF_INBOX:
         // nothing special to do
         break;

   default:
      profile->DecRef();
      FAIL_MSG("unknown folder type");
   }

   if((type == MF_POP || type == MF_IMAP)
      && strutil_isempty(passwd))
   {
      String prompt;
      prompt.Printf( _("Password for '%s':"),
                     strutil_isempty(name) ? i_name.c_str() : name.c_str());
      if(! MInputBox(&passwd,
                     _("Password prompt"),
                     prompt,
                     NULL, NULL, NULL, // parent win, key, default,
                     true) // password mode
         || strutil_isempty(passwd))
      {
         String msg = _("Cannot access this folder without a password.");
         ERRORMESSAGE((msg));
         profile->DecRef();
         return NULL;
      }
   }

   // FIXME calling MailFolderCC::OpenFolder() explicitly here is "anti-OO"
   typeAndFlags = CombineFolderTypeAndFlags(type, flags);

   MailFolder *mf = MailFolderCC::OpenFolder(typeAndFlags, name, profile,
                                             server, login, passwd);
   if ( mf )
      mf->m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);

   profile->DecRef();

   return mf;
}

/* static */ String
MailFolder::ConvertMessageStatusToString(int status)
{
   String strstatus = "";

   if(status & MSG_STAT_RECENT)
   {
      if(status & MSG_STAT_SEEN)
         strstatus << 'R'; // seen but not read -->RECENT
      else
         strstatus << 'N'; // not seen yet  --> really new
   }
   else
      strstatus << ' ';
   strstatus << ((status & MSG_STAT_FLAGGED) ? 'F' : ' ');
   strstatus << ((status & MSG_STAT_ANSWERED) ? 'A' : ' ');
   strstatus << ((status & MSG_STAT_DELETED) ? 'D' : ' ');

   return strstatus;
}

/*-------------------------------------------------------------------*
 * Higher level functionality, nothing else depends on this.
 *-------------------------------------------------------------------*/


#include "wx/dynarray.h"
#include "gui/wxComposeView.h"
#include "wx/persctrl.h"
#include "MDialogs.h"
#include "MApplication.h"

bool
MailFolder::SaveMessages(const INTARRAY *selections,
                         String const & folderName,
                         bool isProfile,
                         bool updateCount)
{
   int
      n = selections->Count(),
      i;
   MailFolder
      *mf;

   if(strutil_isempty(folderName))
      return false;

   mf = MailFolder::OpenFolder(isProfile ? MF_PROFILE : MF_FILE, folderName);
   if(! mf)
   {
      String msg;
      msg << _("Cannot open folder '") << folderName << "'.";
      ERRORMESSAGE((msg));
      return false;
   }
   Message *msg;
   bool events = mf->SendsNewMailEvents();
   mf->EnableNewMailEvents(false, updateCount);

   MProgressDialog *pd = NULL;
   int threshold = mf->GetProfile() ?
      READ_CONFIG(mf->GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : MP_FOLDERPROGRESS_THRESHOLD_D;
   if(n > threshold)
   {
      String msg;
      msg.Printf(_("Saving %d messages..."), n);
      pd = new MProgressDialog(mf->GetName(),
                                             msg,
                                             2*n, NULL);// open a status window:
   }
   for(i = 0; i < n; i++)
   {
      msg = GetMessage((*selections)[i]);
      if(pd) pd->Update( 2*i + 1 );
      mf->AppendMessage(*msg);
      if(pd) pd->Update( 2*i + 2);
      msg->DecRef();
   }
   mf->Ping(); // update any views
   mf->EnableNewMailEvents(events, true);
   mf->DecRef();
   if(pd) delete pd;
   return true;
}


void
MailFolder::DeleteMessages(const INTARRAY *selections)
{
   int n = selections->Count();
   int i;
   String sequence;
   for(i = 0; i < n-1; i++)
      sequence << strutil_ultoa((*selections)[i]) << ',';
   if(n)
      sequence << strutil_ultoa((*selections)[n-1]);
   SetSequenceFlag(sequence, MailFolder::MSG_STAT_DELETED);
}

void
MailFolder::UnDeleteMessages(const INTARRAY *selections)
{
   int n = selections->Count();
   int i;
   for(i = 0; i < n; i++)
      UnDeleteMessage((*selections)[i]);
}

bool
MailFolder::SaveMessagesToFolder(const INTARRAY *selections, MWindow *parent)
{
   bool rc = false;
   MFolder *folder = MDialog_FolderChoose(parent);
   if ( folder )
   {
      rc = SaveMessages(selections, folder->GetFullName(), true);
      folder->DecRef();
   }
   return rc;
}

bool
MailFolder::SaveMessagesToFile(const INTARRAY *selections, MWindow *parent)
{

   String filename = wxPFileSelector("MsgSave",
                                     _("Choose file to save message to"),
                                     NULL, NULL, NULL,
                                     _("All files (*.*)|*.*"),
                                     wxSAVE | wxOVERWRITE_PROMPT,
                                     parent);
   if(filename)
   {
      // truncate the file
      wxFile fd;
      if ( !fd.Create(filename, TRUE /* overwrite */) )
          wxLogError(_("Couldn't truncate the existing file."));

      return SaveMessages(selections,filename, false);
   }
   else
      return false;
}

void
MailFolder::ReplyMessages(const INTARRAY *selections,
                          MWindow *parent,
                          ProfileBase *profile)
{
   int i,np,p;
   String str;
   String str2, prefix;
   const char *cptr;
   wxComposeView *cv;
   Message *msg;

   if(! profile) profile = mApplication->GetProfile();
   int n = selections->Count();
   prefix = READ_CONFIG(profile, MP_REPLY_MSGPREFIX);
   for(i = 0; i < n; i++)
   {
      cv = wxComposeView::CreateNewMessage(parent, profile);
      str = "";
      msg = GetMessage((*selections)[i]);
      np = msg->CountParts();
      for(p = 0; p < np; p++)
      {
         if(msg->GetPartType(p) == Message::MSG_TYPETEXT)
         {
            str = msg->GetPartContent(p);
            cptr = str.c_str();
            str2 = prefix;
            while(*cptr)
            {
               if(*cptr == '\r')
               {
                  cptr++;
                  continue;
               }
               str2 += *cptr;
               if(*cptr == '\n' && *(cptr+1))
               {
                  str2 += prefix;
               }
               cptr++;
            }
            cv->InsertText(str2);
         }
      }
      cv->Show(TRUE);
      String
         name, email;
      email = msg->Address(name, MAT_REPLYTO);
      if(name.length() > 0)
         email = name + String(" <") + email + String(">");
      cv->SetAddresses(email);
      cv->SetSubject(READ_CONFIG(GetProfile(), MP_REPLY_PREFIX)
                     + msg->Subject());
      String messageid;
      msg->GetHeaderLine("Message-Id", messageid);
      String references;
      msg->GetHeaderLine("References", references);
      strutil_delwhitespace(references);
      if(references.Length() > 0)
         references << ",\015\012 ";
      references << messageid;
      cv->AddHeaderEntry("In-Reply-To",messageid);
      cv->AddHeaderEntry("References",references);

      SetMessageFlag((*selections)[i], MailFolder::MSG_STAT_ANSWERED, true);
      SafeDecRef(msg);
   }
}


void
MailFolder::ForwardMessages(const INTARRAY *selections,
                            MWindow *parent,
                            ProfileBase *profile)
{
   int i;
   String str;
   String str2, prefix;
   wxComposeView *cv;
   Message *msg;

   if(! profile) profile = mApplication->GetProfile();
   int n = selections->Count();
   prefix = READ_CONFIG(GetProfile(), MP_REPLY_MSGPREFIX);
   for(i = 0; i < n; i++)
   {
      str = "";
      msg = GetMessage((*selections)[i]);
      cv = wxComposeView::CreateNewMessage(parent,GetProfile());
      cv->SetSubject(READ_CONFIG(GetProfile(), MP_FORWARD_PREFIX)
                                 + msg->Subject());

      msg->WriteToString(str);
      cv->InsertData(strutil_strdup(str), str.Length(),
                     "MESSAGE/RFC822");
      SafeDecRef(msg);
   }
}

