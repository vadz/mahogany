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

#include "miscutil.h"   // GetFullEmailAddress

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

   String symbolicName = i_name;

   if ( type == MF_PROFILE || type == MF_PROFILE_OR_FILE )
   {
      if(type == MF_PROFILE)
         profile = ProfileBase::CreateProfile(i_name, parentProfile);
      else
      {
         String pname = (i_name[0] == '/') ? String(i_name.c_str()+1) : i_name;
         profile = ProfileBase::CreateProfile(pname, parentProfile);
      }
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
         if(FolderTypeHasUserName(type) && strutil_isempty(login)) // fall back to user name
            login = READ_CONFIG(profile, MP_USERNAME);
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
         if(strutil_isempty(server))
         {
            if ( type == MF_POP )
               server = READ_CONFIG(profile, MP_POPHOST);
            else
               server = READ_CONFIG(profile, MP_IMAPHOST);
         }
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
                                             server, login, passwd, symbolicName);
   if ( mf )
      mf->SetUpdateInterval(READ_CONFIG(profile, MP_UPDATEINTERVAL));

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
      if(! (status & MSG_STAT_SEEN))
         strstatus << 'U';
      else
         strstatus << ' ';
      
   strstatus << ((status & MSG_STAT_FLAGGED) ? 'F' : ' ');
   strstatus << ((status & MSG_STAT_ANSWERED) ? 'A' : ' ');
   strstatus << ((status & MSG_STAT_DELETED) ? 'D' : ' ');

   return strstatus;
}

/* static */
bool MailFolder::CanExit(String *which)
{
   return MailFolderCC::CanExit(which);
}

/*-------------------------------------------------------------------*
 * Higher level functionality, nothing else depends on this.
 *-------------------------------------------------------------------*/


#include <wx/dynarray.h>
#include "gui/wxComposeView.h"
#include "wx/persctrl.h"
#include "MDialogs.h"
#include "MApplication.h"

bool
MailFolder::SaveMessagesToFile(const INTARRAY *selections,
                               String const & fileName0)
{
   int
      n = selections->Count(),
      i;
   String fileName = strutil_expandpath(fileName0);
   if(strutil_isempty(fileName))
      return false;

   wxFile file;
   if ( !file.Create(fileName, TRUE /* overwrite */) )
   {
      wxLogError(_("Could not truncate the existing file."));
      return false;
   }
   Message *msg;
   MProgressDialog *pd = NULL;
   int threshold = GetProfile() ?
      READ_CONFIG(GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : MP_FOLDERPROGRESS_THRESHOLD_D;
   if(n > threshold)
   {
      String msg;
      msg.Printf(_("Saving %d messages..."), n);
      pd = new MProgressDialog(GetName(),
                               msg,
                               2*n, NULL);// open a status window:
   }
   int t,n2,j;
   size_t size;
   bool rc = true;
   const char *cptr;
   for(i = 0; i < n; i++)
   {
      msg = GetMessage((*selections)[i]);
      if(msg)
      {
         if(pd) pd->Update( 2*i + 1 );

            // iterate over all parts
         n2 = msg->CountParts();
         for(j = 0; j < n; j++)
         {
            t = msg->GetPartType(j);
            if( ( size = msg->GetPartSize(j)) == 0)
               continue; //    ignore empty parts
            if ( (t == Message::MSG_TYPETEXT) ||
                 (t == Message::MSG_TYPEMESSAGE ))
            {
               cptr = msg->GetPartContent(i);
               if(cptr == NULL)
                  continue; // error ?
               rc &= (file.Write(cptr, size) == size);
            }
         }
         if(pd) pd->Update( 2*i + 2);
         msg->DecRef();
      }
   }
   if(pd) delete pd;
   return rc;
   
}

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
      msg.Printf(_("Cannot open folder '%s'."), folderName.c_str());
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
   bool rc = true;
   for(i = 0; i < n; i++)
   {
      msg = GetMessage((*selections)[i]);
      if(msg)
      {
         if(pd) pd->Update( 2*i + 1 );
         rc &= mf->AppendMessage(*msg);
         if(pd) pd->Update( 2*i + 2);
         msg->DecRef();
      }
   }
   mf->Ping(); // update any views
   mf->EnableNewMailEvents(events, true);
   mf->DecRef();
   if(pd) delete pd;
   return rc;
}


bool
MailFolder::DeleteMessages(const INTARRAY *selections)
{
   int n = selections->Count();
   int i;
   String sequence;
   for(i = 0; i < n-1; i++)
      sequence << strutil_ultoa((*selections)[i]) << ',';
   if(n)
      sequence << strutil_ultoa((*selections)[n-1]);
   return SetSequenceFlag(sequence, MailFolder::MSG_STAT_DELETED);
}

bool
MailFolder::UnDeleteMessages(const INTARRAY *selections)
{
   int n = selections->Count();
   int i;
   bool rc = true;
   for(i = 0; i < n; i++)
      rc &= UnDeleteMessage((*selections)[i]);
   return rc;
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
   if(! filename.IsEmpty())
   {
      {
         // truncate the file
         wxFile fd;
         if ( !fd.Create(filename, TRUE /* overwrite */) )
            wxLogError(_("Could not truncate the existing file."));
      }
      return SaveMessagesToFile(selections,filename);
   }
   else
      return false;
}

void
MailFolder::ReplyMessages(const INTARRAY *selections,
                          MWindow *parent,
                          ProfileBase *profile,
                          int flags)
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
      msg = GetMessage((*selections)[i]);
      if(msg)
      {
         cv = wxComposeView::CreateNewMessage(parent, profile);
         str = "";
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
               // the inserted text comes from the program, not from the user
               cv->ResetDirty();
            }
         }
         cv->Show(TRUE);

      // set the recipient address
         String name;
         String email = msg->Address(name, MAT_REPLYTO);
         email = GetFullEmailAddress(name, email);
         String cc;
         if(flags & REPLY_FOLLOWUP) // group reply
         {
            String from = msg->Address(name, MAT_FROM);
            msg->GetHeaderLine("CC", cc);
            if(from != email)
               email << ", " << from;
         }
         cv->SetAddresses(email,cc);

         // construct    the new subject
         String newSubject;
         String replyPrefix = READ_CONFIG(GetProfile(), MP_REPLY_PREFIX);
         String subject = msg->Subject();

         // we may collapse "Re:"s in the subject if asked for it
         enum CRP // this is an abbreviation of "collapse reply prefix"
         {
            NoCollapse,
            DumbCollapse,
            SmartCollapse
         } collapse = (CRP)READ_CONFIG(GetProfile(), MP_REPLY_COLLAPSE_PREFIX);

         if ( collapse != NoCollapse )
         {
            // the default value of the reply prefix is "Re: ", yet we want to
            // match something like "Re[2]: " in replies, so hack it a little
            String replyPrefixWithoutColon(replyPrefix);
            replyPrefixWithoutColon.Trim();
            if ( replyPrefixWithoutColon.Last() == ':' )
            {
               // throw away last character
               replyPrefixWithoutColon.Truncate(replyPrefixWithoutColon.Len() - 1);
            }

            // determine the reply level (level is 0 for first reply, 1 for the
            // reply to the reply &c)
            size_t replyLevel = 0;

            // the search is case insensitive
            wxString subjectLower(subject.Lower()),
               replyPrefixLower(replyPrefixWithoutColon.Lower());
            const char *pStart = subjectLower.c_str();
            for ( ;; )
            {
               // search for the standard string, for the configured string, and
               // for the translation of the standard string (notice that the
               // standard string should be in lower case because we transform
               // everything to lower case)
               static const char *replyPrefixStandard = gettext_noop("re");

               size_t matchLen = 0;
               const char *pMatch = strstr(pStart, replyPrefixLower);
               if ( !pMatch )
                  pMatch = strstr(pStart, replyPrefixStandard);
               else if ( !matchLen )
                  matchLen = replyPrefixLower.length();
               if ( !pMatch )
                  pMatch = strstr(pStart, _(replyPrefixStandard));
               else if ( !matchLen )
                  matchLen = strlen(replyPrefixStandard);
               if ( !pMatch
                    || (*(pMatch+matchLen) != '[' &&
                        *(pMatch+matchLen) != ':'
                        && *(pMatch+matchLen) != '(')
                  )
                  break;
               else if ( !matchLen )
                  matchLen = strlen(_(replyPrefixStandard));
               pStart = pMatch + matchLen;
               replyLevel++;
            }

//            if ( replyLevel == 1 )
//            {
               // try to see if we don't have "Re[N]" string already
               int replyLevelOld;
               if ( sscanf(pStart, "[%d]", &replyLevelOld) == 1 ||
                    sscanf(pStart, "(%d)", &replyLevelOld) == 1 )
               {
                  replyLevel += replyLevelOld;
                  replyLevel --; // one was already counted
                  pStart++; // opening [ or (
                  while( isdigit(*pStart) )
                     pStart ++;
                  pStart++; // closing ] or )
               }
//            }

            // skip spaces
            while ( isspace(*pStart) )
               pStart++;

         // skip also the ":" after "Re" is there was one
            if ( replyLevel > 0 && *pStart == ':' )
            {
               pStart++;

               // ... and the spaces after ':' if any too
               while ( isspace(*pStart) )
                  pStart++;
            }

            // this is the start of real subject
            subject = subject.Mid(pStart - subjectLower.c_str());

            if ( collapse == SmartCollapse && replyLevel > 0 )
            {
               // TODO not configurable enough, allow the user to specify the
               //      format string himself and also decide whether we use powers
               //      of 2, just multiply by 2 or nothing at all
               // for now we just increment the replyLevel by one,
               // everything else is funny as it doesn't maintain
               // powers of two :-) KB
               replyLevel ++;
               newSubject.Printf("%s[%d]: %s",
                                 replyPrefixWithoutColon.c_str(),
                                 replyLevel,
                                 subject.c_str());
            }
         }

         // in cases of {No|Dumb}Collapse we fall here
         if ( !newSubject )
         {
            newSubject = replyPrefix + subject;
         }

         cv->SetSubject(newSubject);

      // other headers
         String messageid;
         msg->GetHeaderLine("Message-Id", messageid);
         String references;
         msg->GetHeaderLine("References", references);
         strutil_delwhitespace(references);
         if(references.Length() > 0)
            references << ",\015\012 ";
         references << messageid;
         cv->AddHeaderEntry("In-Reply-To", messageid);
         cv->AddHeaderEntry("References", references);
         if( READ_CONFIG(profile, MP_SET_REPLY_FROM_TO) )
         {
            String rt;
            msg->GetHeaderLine("To", rt);
            cv->AddHeaderEntry("Reply-To", rt);
         }
         SetMessageFlag((*selections)[i], MailFolder::MSG_STAT_ANSWERED, true);
         SafeDecRef(msg);
      }//if(msg)
   }//for()
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
      if(msg)
      {
         cv = wxComposeView::CreateNewMessage(parent,GetProfile());
         cv->SetSubject(READ_CONFIG(GetProfile(), MP_FORWARD_PREFIX)
                        + msg->Subject());
         msg->WriteToString(str);
         cv->InsertData(strutil_strdup(str), str.Length(),
                        "MESSAGE/RFC822");
         SafeDecRef(msg);
      }
   }
}

