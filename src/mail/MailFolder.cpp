/*-*- c++ -*-********************************************************
 * MailFolder class: handling of Unix mail folders                  *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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
#   include "MEvent.h"
#   include "MModule.h"
#   include <stdlib.h>
#endif

#include "modules/Scoring.h"

#include "MDialogs.h" // for the password prompt...

#include "Mdefaults.h"

#include "Message.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "miscutil.h"   // GetFullEmailAddress

#include <wx/file.h>
#include <wx/dynarray.h>

#include "gui/wxComposeView.h"
#include "wx/persctrl.h"
#include "MDialogs.h"
#include "MApplication.h"
#include "modules/Filters.h"
#include <wx/timer.h>

/*-------------------------------------------------------------------*
 * local classes
 *-------------------------------------------------------------------*/

KBLIST_DEFINE(SizeTList, size_t);

/** a timer class to regularly ping the mailfolder. */
class MailFolderTimer : public wxTimer
{
public:
   /** constructor
       @param mf the mailfolder to query on timeout
   */
   MailFolderTimer(MailFolder *mf)
      : m_mf(mf)
   {

   }

   /// get called on timeout and pings the mailfolder
   void Notify(void) { if(! m_mf->IsLocked() ) m_mf->Ping(); }

protected:
   /// the mailfolder to update
   MailFolder  *m_mf;
};



/*-------------------------------------------------------------------*
 * static member functions of MailFolder.h
 *-------------------------------------------------------------------*/

/* static */
MailFolder *
MailFolder::OpenFolder(const MFolder *mfolder, ProfileBase *profile)
{
   ASSERT(mfolder);
   ASSERT_MSG(0,"OpenFolder(MFolder *) is incomplete and needs access to the server/login/passwd");
   int typeAndFlags = CombineFolderTypeAndFlags(mfolder->GetType(),
                                                mfolder->GetFlags());
   
   return OpenFolder( typeAndFlags,
                      mfolder->GetName(),
                      profile);
                      
}

/* static */
MailFolder *
MailFolder::OpenFolder(int folderType,
                       String const &i_name,
                       ProfileBase *parentProfile,
                       String const &i_server,
                       String const &i_login,
                       String const &i_passwd)
{
   // open a folder:
   ProfileBase *profile = NULL;
   String login, passwd, name, server;
   FolderType type = GetFolderType(folderType);
   int flags = GetFolderFlags(folderType);

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
         if( strutil_isempty(name) )
            name = READ_CONFIG(profile, MP_FOLDER_PATH);
         if(name == "INBOX")
            type = MF_INBOX;
         name = strutil_expandfoldername(name);
         break;

      case MF_MH:
         // initialize the MH driver now to get the MHPATH until cclient
         // has a chance to complain about it
         if ( !MailFolderCC::InitializeMH() )
         {
            profile->DecRef();
            return NULL;
         }
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

      case MF_NEWS:
         // initialize news spool
         if ( !MailFolderCC::InitializeNewsSpool() )
         {
            profile->DecRef();
            return NULL;
         }
         break;

      case MF_INBOX:
         // nothing special to do
         break;

      default:
         profile->DecRef();
         FAIL_MSG("unknown folder type");
         return NULL;
   }

#if 0
   // ask the password for the folders which need it but for which it hadn't been
   // specified during creation
   if ( FolderTypeHasUserName(type) && !(flags & MF_FLAGS_ANON) && !passwd )
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
#endif

   // FIXME calling MailFolderCC::OpenFolder() explicitly here is "anti-OO"
   folderType = CombineFolderTypeAndFlags(type, flags);

   MailFolder *mf = MailFolderCC::OpenFolder(folderType, name, profile,
                                             server, login, passwd, symbolicName);
   profile->DecRef();

   return mf;
}

/* static */
bool
MailFolder::DeleteFolder(const MFolder *mfolder)
{
   // for now there is only one implementation to call:
   return MailFolderCC::DeleteFolder(mfolder);
}

/* static */
bool
MailFolder::CreateFolder(const String &name,
                         FolderType type,
                         int iflags,
                         const String &path,
                         const String &comment)
{
   bool valid;
   
   switch(type)
      {
      case MF_INBOX:
      case MF_FILE:
      case MF_POP:
      case MF_IMAP:
      case MF_NNTP:
      case MF_NEWS:
      case MF_MH:
         valid = true;
         break;
      default:
         valid = false; // all pseudo-types
      }
   if(! valid )
      return false;

   if( name.Length() == 0 )
      return false;

   ASSERT( (iflags & MF_FLAGSMASK) != iflags);
   FolderFlags flags = (FolderFlags) ( iflags & MF_FLAGSMASK ) ;
   ProfileBase * p = ProfileBase::CreateProfile(name);
   p->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);
   p->writeEntry(MP_FOLDER_TYPE, type|flags);
   if(path.Length() > 0)
      p->writeEntry(MP_FOLDER_PATH, path);
   if(comment.Length() > 0)
      p->writeEntry(MP_FOLDER_COMMENT, comment);
   p->DecRef();

   return true;
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

/* static */
bool
MailFolder::Subscribe(const String &host, FolderType protocol,
                      const String &mailboxname, bool subscribe)
{
   return MailFolderCC::Subscribe(host, protocol, mailboxname, subscribe);
}

void
MailFolder::ReplyMessage(class Message *msg,
                         int flags,
                         ProfileBase *profile,
                         MWindow *parent)
{
   ASSERT_RET(msg);
   msg->IncRef();
   if(! profile) profile = mApplication->GetProfile();

   wxComposeView *cv = wxComposeView::CreateReplyMessage(parent, profile);

         // set the recipient address
   String name;
   String email = msg->Address(name, MAT_REPLYTO);
   email = GetFullEmailAddress(name, email);
   String cc;
   if(flags & REPLY_FOLLOWUP) // group reply
   {
      msg->GetHeaderLine("CC", cc);
      String addr = msg->Address(name, MAT_FROM);
      if(! email.Contains(addr))
         email << ", " << addr;
      addr = msg->Address(name, MAT_TO);
      if(! email.Contains(addr))
         email << ", " << addr;
   }
   cv->SetAddresses(email,cc);

         // construct the new subject
   String newSubject;
   String replyPrefix = READ_CONFIG(profile, MP_REPLY_PREFIX);
   String subject = msg->Subject();

   // we may collapse "Re:"s in the subject if asked for it
   enum CRP // this is an abbreviation of "collapse reply prefix"
   {
      NoCollapse,
      DumbCollapse,
      SmartCollapse
   } collapse = (CRP)READ_CONFIG(profile, MP_REPLY_COLLAPSE_PREFIX);

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

   cv->InitText(msg);
   cv->Show(TRUE);

   SafeDecRef(msg);
}

/* static */
void
MailFolder::ForwardMessage(class Message *msg,
                           ProfileBase *profile,
                           MWindow *parent)
{
   ASSERT_RET(msg);
   msg->IncRef();
   if(! profile) profile = mApplication->GetProfile();

   wxComposeView *cv = wxComposeView::CreateFwdMessage(parent, profile);
   cv->SetSubject(READ_CONFIG(profile, MP_FORWARD_PREFIX)+ msg->Subject());
   cv->InitText(msg);
   cv->Show(TRUE);
   SafeDecRef(msg);
}

/*-------------------------------------------------------------------*
 * Higher level functionality, collected in MailFolderCmn class.
 *-------------------------------------------------------------------*/

class MFCmnEventReceiver : public MEventReceiver
{
public:
   inline MFCmnEventReceiver(MailFolderCmn *mf)
      {
         m_Mf = mf;
         m_MEventCookie = MEventManager::Register(*this, MEventId_OptionsChange);
         ASSERT_MSG( m_MEventCookie, "can't reg folder with event manager");
         m_MEventPingCookie = MEventManager::Register(*this, MEventId_Ping);
         ASSERT_MSG( m_MEventPingCookie, "can't reg folder with event manager");
      }
   virtual inline ~MFCmnEventReceiver()
      {
         MEventManager::Deregister(m_MEventCookie);
         MEventManager::Deregister(m_MEventPingCookie);
      }
   virtual inline bool OnMEvent(MEventData& event)
      {
         if(event.GetId() == MEventId_Ping)
            m_Mf->Ping();
         else // options change
         {
            m_Mf->UpdateConfig();
            m_Mf->RequestUpdate();
         }
         return true;
      }
private:
   MailFolderCmn *m_Mf;
   void *m_MEventCookie, *m_MEventPingCookie;
};


MailFolderCmn::MailFolderCmn(ProfileBase *profile)
{
#ifdef DEBUG
   m_PreCloseCalled = false;
#endif

   // We need to know if we are building the first folder listing ever
   // or not, to suppress NewMail events.
   m_FirstListing = true;
   m_ProgressDialog = NULL;
   m_UpdateFlags = UF_Default;
   ASSERT(profile);
   m_Profile = profile;
   if(m_Profile) m_Profile->IncRef();
   m_Timer = new MailFolderTimer(this);
   m_LastNewMsgUId = UID_ILLEGAL;

   m_MEventReceiver = new MFCmnEventReceiver(this);

   UpdateConfig(); // read profile settings
}

MailFolderCmn::~MailFolderCmn()
{
#ifdef DEBUG
   ASSERT(m_PreCloseCalled == true);
#endif
   delete m_Timer;
   if(m_Profile)
      m_Profile->DecRef();
   delete m_MEventReceiver;
}

void
MailFolderCmn::PreClose(void)
{
   if ( m_Timer )
      m_Timer->Stop();
#ifdef DEBUG
   m_PreCloseCalled = true;
#endif
}

bool
MailFolderCmn::SaveMessagesToFile(const INTARRAY *selections,
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
   String tmpstr;
   for(i = 0; i < n; i++)
   {
      msg = GetMessage((*selections)[i]);
      if(msg)
      {
         if(pd) pd->Update( 2*i + 1 );

            // iterate over all parts
         n2 = msg->CountParts();
         for(j = 0; j < n2; j++)
         {
            t = msg->GetPartType(j);
            if( ( size = msg->GetPartSize(j)) == 0)
               continue; //    ignore empty parts
            if ( (t == Message::MSG_TYPETEXT) ||
                 (t == Message::MSG_TYPEMESSAGE ))
            {
               cptr = msg->GetPartContent(j);
               if(cptr == NULL)
                  continue; // error ?
               tmpstr = strutil_enforceNativeCRLF(cptr);
               rc &= (file.Write(tmpstr, tmpstr.length()) == size);
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
MailFolderCmn::SaveMessages(const INTARRAY *selections,
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

   /* It could be that we are trying to open the very folder we are
      getting our messages from, so we need to temporarily unlock this 
      folder. */
   bool locked = IsLocked();
   if(locked)
      UnLock(); // release our own lock


   mf = MailFolder::OpenFolder(isProfile ? MF_PROFILE : MF_FILE,
                               folderName);
   if(! mf)
   {
      String msg;
      msg.Printf(_("Cannot open folder '%s'."), folderName.c_str());
      ERRORMESSAGE((msg));
      return false;
   }
   Message *msg;

   int updateFlags = mf->GetUpdateFlags();
   mf->SetUpdateFlags( updateCount ? UF_UpdateCount : 0 );

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
   if(Lock()) // we don't want anything to happen in here
   {
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
      UnLock();
      mf->Ping(); // with our flags
      mf->SetUpdateFlags(updateFlags); // restore old flags
   }
   else
      rc = false;
   
   mf->DecRef();
   if(pd) delete pd;
   if(locked)
      Lock();
   return rc;
}

bool
MailFolderCmn::UnDeleteMessages(const INTARRAY *selections)
{
   int n = selections->Count();
   int i;
   bool rc = true;
   for(i = 0; i < n; i++)
      rc &= UnDeleteMessage((*selections)[i]);
   return rc;
}

bool
MailFolderCmn::SaveMessagesToFolder(const INTARRAY *selections, MWindow *parent)
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
MailFolderCmn::SaveMessagesToFile(const INTARRAY *selections, MWindow *parent)
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
MailFolderCmn::ReplyMessages(const INTARRAY *selections,
                          MWindow *parent,
                          int flags)
{
   Message *msg;

   int n = selections->Count();
   for( int i = 0; i < n; i++ )
   {
      msg = GetMessage((*selections)[i]);
      ReplyMessage(msg, flags, GetProfile(), parent);
      msg->DecRef();
      SetMessageFlag((*selections)[i], MailFolder::MSG_STAT_ANSWERED, true);
   }
}


void
MailFolderCmn::ForwardMessages(const INTARRAY *selections, MWindow *parent)
{
   int i;
   Message *msg;

   int n = selections->Count();
   for(i = 0; i < n; i++)
   {
      msg = GetMessage((*selections)[i]);
      ForwardMessage(msg, GetProfile(), parent);
      msg->DecRef();
   }
}


static MMutex gs_SortListingMutex;

static long gs_SortOrder = 0;
static MModule_Scoring *gs_ScoreModule = NULL;
static MailFolder * gs_MailFolder = NULL;

static int CompareStatus(int stat1, int stat2, int flag)
{
   int
      score1 = 0,
      score2 = 0;

   if( stat1 & MailFolder::MSG_STAT_DELETED )
   {
      if( ! (stat2 & MailFolder::MSG_STAT_DELETED ))
         return flag;
   }
   else if( stat2 & MailFolder::MSG_STAT_DELETED )
      return -flag;

   /* We use a scoring system:
      recent = +1
      unseen   = +1
      answered = -1
   */

   if(stat1 & MailFolder::MSG_STAT_RECENT)
      score1 += 1;
   if( !(stat1 & MailFolder::MSG_STAT_SEEN) )
      score1 += 1;
   if( stat1 & MailFolder::MSG_STAT_ANSWERED )
      score1 -= 1;

   if(stat2 & MailFolder::MSG_STAT_RECENT)
      score2 += 1;
   if( !(stat2 & MailFolder::MSG_STAT_SEEN) )
      score2 += 1;
   if( stat2 & MailFolder::MSG_STAT_ANSWERED )
      score2 -= 1;

   return (score1 > score2) ? -flag : (score1 < score2) ? flag : 0;
}

extern "C"
{
   static int ComparisonFunction(const void *a, const void *b)
   {
      HeaderInfo
         *i1 = (HeaderInfo *)a,
         *i2 = (HeaderInfo *)b;

      long sortOrder = gs_SortOrder, criterium;

      int result = 0;
      int flag;

      while(result == 0 && sortOrder != 0 )
      {
         criterium = sortOrder & 0xF;
         sortOrder >>= 4;

         switch(criterium)
         {
         case MSO_NONE:
            break;
         case MSO_DATE:
         case MSO_DATE_REV:
            flag = criterium == MSO_DATE ? -1 : 1;
            if(i1->GetDate() > i2->GetDate())
               result = flag;
            else
               if(i1->GetDate() < i2->GetDate())
                  result = -flag;
               else
                  result = 0;
         break;
         case MSO_SUBJECT:
         case MSO_SUBJECT_REV:
         {
            String
               subj1 = strutil_removeReplyPrefix(i1->GetSubject()),
               subj2 = strutil_removeReplyPrefix(i2->GetSubject());

            result = criterium == MSO_SUBJECT ?
               Stricmp(subj1, subj2) : -Stricmp(subj1, subj2);
         }
         break;
         case MSO_AUTHOR:
         case MSO_AUTHOR_REV:
            result = criterium == MSO_SUBJECT ?
               Stricmp(i1->GetFrom(), i2->GetFrom())
               : -Stricmp(i1->GetFrom(), i2->GetFrom());
            break;
         case MSO_STATUS_REV:
         case MSO_STATUS:
             flag = criterium == MSO_STATUS_REV ? -1 : 1;
             {
                int
                   stat1 = i1->GetStatus(),
                   stat2 = i2->GetStatus();
                // ms   g1 new?
                result = CompareStatus(stat1, stat2, flag);
             }
             break;
         case MSO_SCORE:
         case MSO_SCORE_REV:
            flag = criterium == MSO_SCORE_REV ? -1 : 1;
            {
               long score1 = 0, score2 = 0;
               if(gs_ScoreModule)
               {
                  score1 = gs_ScoreModule->ScoreMessage(gs_MailFolder, i1);
                  score2 = gs_ScoreModule->ScoreMessage(gs_MailFolder, i2);
               }
               result = score1 > score2 ?
                  flag : score2 > score1 ?
                  -flag : 0;
            }
         break;
         case MSO_THREAD:
         case MSO_THREAD_REV:
            ASSERT_MSG(0,"reverse threading not implemented yet");
            break;
         default:
            ASSERT_MSG(0,"unknown sorting criterium");
            break;
         }
      }
      return result;
     }
}

#define MFCMN_INDENT1_MARKER   1000
#define MFCMN_INDENT2_MARKER   1001

static void AddDependents(size_t &idx, int level,
                          int i ,
                          size_t indices[],
                          unsigned indents[],
                          HeaderInfoList *hilp,
                          SizeTList dependents[])
{
   if(level >= MFCMN_INDENT1_MARKER)
      level = MFCMN_INDENT1_MARKER - 1;

   SizeTList::iterator it;

   for(it = dependents[i].begin(); it != dependents[i].end(); it++)
   {
      size_t idxToAdd = **it;
      // a non-zero index means, this one has been added already
      if((*hilp)[idxToAdd]->GetIndentation() != MFCMN_INDENT1_MARKER)
      {
         ASSERT(idx < (*hilp).Count());
         indices[idx] = idxToAdd;
         indents[idx] = level;
         (*hilp)[idxToAdd]->SetIndentation(MFCMN_INDENT1_MARKER);
         idx++;
         // after adding an element, we need to check if we have to add
         // any of its own dependents:
         AddDependents(idx, level+1, idxToAdd, indices, indents, hilp, dependents);
      }
   }
}


static void ThreadMessages(MailFolder *mf, HeaderInfoList *hilp)
{
   // no need to incref/decref, done in UpdateListing()
   if((*hilp).Count() <= 1)
      return; // nothing to be done

   STATUSMESSAGE((_("Threading %lu messages..."), (unsigned long) hilp->Count()));

   size_t i;
   for(i = 0; i < hilp->Count(); i++)
      (*hilp)[i]->SetIndentation(0);

   /* We need a list of dependent messages for each entry. */
   SizeTList  * dependents = new SizeTList[ (*hilp).Count() ];

   for(i = 0; i < (*hilp).Count(); i++)
   {
      String id = (*hilp)[i]->GetId();
      if(id.Length()) // no Id lines in Outbox!
      {
         for(size_t j = 0; j < (*hilp).Count(); j++)
         {
            String references = (*hilp)[j]->GetReferences();
            if(references.Find(id) != wxNOT_FOUND)
            {
               dependents[i].push_front(new size_t(j));
               (*hilp)[j]->SetIndentation(MFCMN_INDENT2_MARKER);
            }
         }
      }
   }
   /* Now we have all the dependents set up properly and can re-build
      the list. We now build an array of indices for the new list.
   */

   size_t * indices = new size_t [(*hilp).Count()];
   unsigned * indents = new unsigned [(*hilp).Count()];
   for(i = 0; i < hilp->Count(); i++)
      indents[i] = 0;

   size_t idx = 0; // where to store next entry
   for(i = 0; i < hilp->Count(); i++)
   {
      // we mark used indices with a non-0 indentation:
      if((*hilp)[i]->GetIndentation() == 0)
      {
         ASSERT(idx < (*hilp).Count());
         indices[idx++] = i;
         (*hilp)[i]->SetIndentation(MFCMN_INDENT1_MARKER);
         AddDependents(idx, 1, i, indices, indents, hilp, dependents);
      }
   }
   ASSERT(idx == hilp->Count());

   hilp->SetTranslationTable(indices);


   for(i = 0; i < hilp->Count(); i++)
      (*hilp)[i]->SetIndentation(indents[i]);


   //delete [] indices; // freed by ~HeaderInfoList()
   delete [] indents;
   delete [] dependents;
   STATUSMESSAGE((_("Threading %lu messages...done."), (unsigned long) hilp->Count()));
}

static void SortListing(MailFolder *mf, HeaderInfoList *hil, long SortOrder)
{
   gs_SortListingMutex.Lock();
   gs_MailFolder = mf;
   gs_SortOrder = SortOrder;
   gs_ScoreModule = (MModule_Scoring *)MModule::GetProvider(MMODULE_INTERFACE_SCORING);

   // no need to incref/decref, done in UpdateListing()
   if(hil->Count() > 1)
      qsort(hil->GetArray(),
            hil->Count(),
            (*hil)[0]->SizeOf(),
            ComparisonFunction);

   if(gs_ScoreModule) gs_ScoreModule->DecRef();
   gs_MailFolder = NULL;
   gs_SortListingMutex.Unlock();
}

/*
  This function is called by GetHeaders() immediately after building a 
  new folder listing. It checks for new mails and, if required, sends
  out new mail events.
*/
void
MailFolderCmn::CheckForNewMail(HeaderInfoList *hilp)
{
   /* Now check whether we need to send new mail notifications: */
   UIdType n = (*hilp).Count();
   UIdType *messageIDs = new UIdType[n];

   DBGMESSAGE(("CheckForNewMail(): folder: %s highest seen uid: %lu.",
               GetName().c_str(), (unsigned long) m_LastNewMsgUId));

   // Find the new messages:
   UIdType nextIdx = 0;
   UIdType highestId = UID_ILLEGAL;
   for ( UIdType i = 0; i < n; i++ )
   {
      if( ( m_LastNewMsgUId == UID_ILLEGAL
            || (*hilp)[i]->GetUId() > m_LastNewMsgUId ))
      {
         UIdType uid = (*hilp)[i]->GetUId();
         if(IsNewMessage( (*hilp)[i] ) )
         {
            messageIDs[nextIdx++] = uid;
         }
         if(highestId == UID_ILLEGAL || uid > highestId)
            highestId = uid;
      }
   }
   ASSERT(nextIdx <= n);

   if( (m_UpdateFlags & UF_DetectNewMail) // do we want new mail events?
       && m_LastNewMsgUId != UID_ILLEGAL) // is it not the first time
      // that we look at this folder?
   {
      if( nextIdx != 0)
         MEventManager::Send( new MEventNewMailData (this, nextIdx, messageIDs) );
   }

   if(highestId != UID_ILLEGAL && (m_UpdateFlags & UF_UpdateCount) )
      m_LastNewMsgUId = highestId;
   
   DBGMESSAGE(("CheckForNewMail() after test: folder: %s highest seen uid: %lu.",
               GetName().c_str(), (unsigned long) highestId));


   delete [] messageIDs;
}


/* This will do the work in the new mechanism:

   - For now it only sorts or threads the headerinfo list
 */
void
MailFolderCmn::ProcessHeaderListing(HeaderInfoList *hilp)
{
   ASSERT(hilp);
   hilp->IncRef();

   SortListing(this, hilp, m_Config.m_ListingSortOrder);
   if(m_Config.m_UseThreading)
      ThreadMessages(this, hilp);
   
   hilp->DecRef();
}

#if 0
/* This will disappear: */
void
MailFolderCmn::UpdateListing(void)
{
   ASSERT_MSG(0,"obsolete UpdateListing() called");

   // We must make sure that we have called BuildListing() at least
   // once, or ApplyFilterRules() will get into an endless recursion
   // when it tries to obtain a listing and then gets called from
   // here.
   HeaderInfoList * hilp = BuildListing();
   if(CountNewMessages() > 0)
   {
      m_FiltersCausedChange = false;
      // before updating the listing, we need to filter any new messages:
      ApplyFilterRules(true);
      if(m_FiltersCausedChange)
      {
        // need to re-build it as it might have changed:
         SafeDecRef(hilp);
         hilp = BuildListing();
      }
   }
   if(hilp)
   {
      ProcessHeaderListing(hilp);
      // now we sent an update event to update folderviews etc
      MEventManager::Send( new MEventFolderUpdateData (this) );

      CheckForNewMail(hilp);

      hilp->DecRef();
      m_FirstListing = false;
   }
}
#endif

void
MailFolderCmn::UpdateMessageStatus(UIdType uid)
{
   /* I thought we could be more clever here, but it seems that
      c-client/imapd create *lots* of unnecessary status change
      entries even when only one message status changes. So we just
      tell the folder to send a single event and invalidate the
      listing. That is more economical. */
   RequestUpdate();
#if 0
   if(m_Config.m_ReSortOnChange)
      RequestUpdate(); // we need a complete new listing
   else
   {
      /// just tell them that we have an updated listing:
      MEventManager::Send( new MEventFolderUpdateData (this) );
   }
#endif
}

void
MailFolderCmn::UpdateConfig(void)
{
   m_Config.m_ListingSortOrder = READ_CONFIG(GetProfile(), MP_MSGS_SORTBY);
   m_Config.m_ReSortOnChange = READ_CONFIG(GetProfile(),
                                           MP_MSGS_RESORT_ON_CHANGE) != 0;
   m_Config.m_UpdateInterval = READ_CONFIG(GetProfile(),
                                           MP_UPDATEINTERVAL);
   m_Config.m_UseThreading = READ_CONFIG(GetProfile(),
                                         MP_MSGS_USE_THREADING) != 0;

   m_Timer->Stop();
   m_Timer->Start(m_Config.m_UpdateInterval * 1000);
}


/** Delete a message.
    @param uid mesage uid
    @return always true UNSUPPORTED!
    */
bool
MailFolderCmn::DeleteMessage(unsigned long uid)
{
   INTARRAY *ia = new INTARRAY;
   ia->Add(uid);
   bool rc = DeleteMessages(ia);
   delete ia;
   return rc;
}


bool
MailFolderCmn::DeleteOrTrashMessages(const INTARRAY *selections)
{
   bool reallyDelete = ! READ_CONFIG(GetProfile(), MP_USE_TRASH_FOLDER);
   // If we are the trash folder, we *really* delete.
   if(!reallyDelete && GetName() == READ_CONFIG(GetProfile(),
                                                MP_TRASH_FOLDER))
      reallyDelete = true;

   if(!reallyDelete)
   {
      bool rc = SaveMessages(selections,
                             READ_CONFIG(GetProfile(), MP_TRASH_FOLDER),
                             true /* is profile */,
                             false /* don´t update */);
      if(rc)
         SetSequenceFlag(GetSequenceString(selections),MSG_STAT_DELETED);
      ExpungeMessages();
      RequestUpdate();
      Ping();
      return rc;
   }
   else
      return DeleteMessages(selections);
}

bool
MailFolderCmn::DeleteMessages(const INTARRAY *selections, bool expunge)
{
   bool rc = SetSequenceFlag(GetSequenceString(selections),
                             MailFolder::MSG_STAT_DELETED);
   if(rc && expunge)
      ExpungeMessages();
   return rc;
}

int
MailFolderCmn::ApplyFilterRules(bool newOnly)
{
   // Maybe we are lucky and have nothing to do?
   if(newOnly && CountNewMessages() == 0)
         return 0;

   // Obtain pointer to the filtering module:
   MModule_Filters *filterModule = MModule_Filters::GetModule();

   /* Has the folder got any filter rules set?
      If so, apply the rules before collecting.
   */
   if(filterModule)
   {
      int rc = 0;
      String filterString;
      // apply the filter from the original folder:
      filterString = READ_CONFIG(GetProfile(), MP_FILTER_RULE);
      if(filterString.Length())
      {
         FilterRule * filterRule = filterModule->GetFilter(filterString);
         if(filterRule)
         {
            // This might change the folder contents,
            // so we must set this flag:
            m_FiltersCausedChange = true;
            rc = filterRule->Apply(this, newOnly);
            filterRule->DecRef();
         }
      }
      filterModule->DecRef();
      return rc;
   }
   else
      return 0; // no filter module
}
