/*-*- c++ -*-********************************************************
 * MailFolder class: handling of Unix mail folders                  *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
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
#   include <errno.h>
#endif

#include <wx/timer.h>
#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/dynarray.h>

#include "MDialogs.h" // for MDialog_FolderChoose

#include "Mdefaults.h"

#include "MFilter.h"

#include "Message.h"
#include "MailFolder.h"
#include "MailFolderCC.h"
#ifdef EXPERIMENTAL
#include "MMailFolder.h"
#endif

#include "miscutil.h"   // GetFullEmailAddress

#include "gui/wxComposeView.h"
#include "wx/persctrl.h"
#include "MApplication.h"
#include "modules/Filters.h"

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


MLogCircle
MailFolder::ms_LogCircle(MF_LOGCIRCLE_SIZE);

MLogCircle::MLogCircle(int n)
{
   m_N = n;
   m_Messages = new String[m_N];
   m_Next = 0;
}
MLogCircle::   ~MLogCircle()
{
   delete [] m_Messages;
}
void
MLogCircle:: Add(const String &txt)
{
   m_Messages[m_Next++] = txt;
   if(m_Next == m_N)
      m_Next = 0;
}
bool
MLogCircle:: Find(const String needle, String *store) const
{
   // searches backwards (most relevant first)
   // search from m_Next-1 to 0
   if(m_Next > 0)
      for(int i = m_Next-1; i >= 0 ; i--)
      {
         wxLogDebug("checking msg %d, %s", i, m_Messages[i].c_str());
         if(m_Messages[i].Contains(needle))
         {
            if(store) *store = m_Messages[i];
            return true;
         }
      }
   // search from m_N-1 down to m_Next:
   for(int i = m_N-1; i >= m_Next; i--)
   {
      wxLogDebug("checking msg %d, %s", i, m_Messages[i].c_str());
      if(m_Messages[i].Contains(needle))
      {
         if(store) *store = m_Messages[i];
         return true;
      }
   }
   // last attempt:
   String tmp = strerror(errno);
   if(tmp.Contains(needle))
   {
      if(store) *store = tmp;
      return true;
   }
   return false;
}
String
MLogCircle::GetLog(void) const
{
   String log;
   // search from m_Next to m_N
   int i;
   for(i = m_Next; i < m_N ; i++)
      log << m_Messages[i] << '\n';
         // search from 0 to m_Next-1:
   for(i = 0; i < m_Next; i++)
      log << m_Messages[i] << '\n';
   return log;
}

/* statuc */
String
MLogCircle::GuessError(void) const
{
   String guess, err;
   bool addLog = false;
   bool addErr = false;

   if(Find("No such host", &err))
   {
      guess = _("The server name could not be resolved.\n"
                "Maybe the network connection or the DNS servers are down?");
      addErr = true;
   }
   else if(Find("User unknown", &err))
   {
      guess = _("One or more email addresses were not recognised.");
      addErr = true;
      addLog = true;
   }
   else if(Find("INVALID_ADDRESS", &err)
           || Find(".SYNTAX-ERROR.", &err))
   {
      guess = _("The message contained an invalid address specification.");
      addErr = true;
      addLog = true;
   }
   if(addErr)
   {
      guess += _("\nThe exact error message was:\n");
      guess += err;
   }
   if(addLog) // we have an idea
   {
      guess += _("\nThe last few log messages were:\n");
      guess += GetLog();
   }
   return guess;
}

void
MLogCircle::Clear(void)
{
   for(int i = 0; i < m_N; i++)
      m_Messages[i] = "";
}

/*-------------------------------------------------------------------*
 * static member functions of MailFolder.h
 *-------------------------------------------------------------------*/

/// called to make sure everything is set up
static void InitStatic(void);
static void CleanStatic(void);

/* Flush all event queues for all MailFolder drivers. */

/* static */
void
MailFolder::ProcessEventQueue(void)
{
   InitStatic();
   MailFolderCC::ProcessEventQueue();
}

/*
 * This function guesses: it checks if such a profile exists,
 * otherwise it tries a file with that name.
 */

/* static */
MailFolder *
MailFolder::OpenFolder(const String &name, Profile *parentProfile)
{
   InitStatic();
   if(Profile::ProfileExists(name))
   {
      MFolder *mf = MFolder::Get(name);
      if(mf )
      {
         MailFolder *m = OpenFolder(mf);
         mf->DecRef();
         return m;
      }
      else
         return NULL; // profile failed
   }
   else // attempt to open file
   {
      // profile entry does not exist
      Profile *profile = Profile::CreateEmptyProfile(parentProfile);
      MailFolder *m = OpenFolder( MF_FILE, name, profile);
      profile->DecRef();
      return m;
   }
}

/* static */
MailFolder *
MailFolder::OpenFolder(const MFolder *mfolder)
{
   InitStatic();
   CHECK( mfolder, NULL, "NULL MFolder in OpenFolder()" );

   // VZ: this doesn't do anything yet anyhow...
#if 0 //def EXPERIMENTAL
   if( mfolder->GetType() == MF_MFILE
       || mfolder->GetType() == MF_MDIR )
      return MMailFolder::OpenFolder(mfolder);
#endif

   int typeAndFlags = CombineFolderTypeAndFlags(mfolder->GetType(),
                                                mfolder->GetFlags());

   MailFolder *mf = OpenFolder( typeAndFlags,
                                mfolder->GetPath(),
                                NULL,
                                mfolder->GetServer(),
                                mfolder->GetLogin(),
                                mfolder->GetPassword(),
                                mfolder->GetFullName());
                                /*VS:was mfolder->GetName()*/
   return mf;
}

/* static */
MailFolder *
MailFolder::HalfOpenFolder(const MFolder *mfolder, Profile *profile)
{
   InitStatic();
   CHECK( mfolder, NULL, "NULL MFolder in OpenFolder()" );

   int typeAndFlags = CombineFolderTypeAndFlags(mfolder->GetType(),
                                                mfolder->GetFlags());

   return OpenFolder( typeAndFlags,
                      mfolder->GetPath(),
                      profile,
                      mfolder->GetServer(),
                      mfolder->GetLogin(),
                      mfolder->GetPassword(),
                      mfolder->GetName(),
                      true );
}

/* static */
MailFolder *
MailFolder::OpenFolder(int folderType,
                       String const &i_path,
                       Profile *parentProfile,
                       String const &i_server,
                       String const &i_login,
                       String const &i_passwd,
                       String const &i_name,
                       bool halfopen)
{
   InitStatic();
   FolderType type = GetFolderType(folderType);
#ifdef EXPERIMENTAL
   CHECK( type != MF_MFILE && type != MF_MDIR,
          NULL, "Obsolete MailFolder::OpenFolder() called." );
#endif

// open a folder:
   Profile *profile = NULL;
   String login, passwd, name, server;
   int flags = GetFolderFlags(folderType);

   String symbolicName = i_name;
   if(strutil_isempty(symbolicName))
      symbolicName = i_path;

   if ( type == MF_PROFILE )
   {
      if(type == MF_PROFILE)
         profile = Profile::CreateProfile(i_path, parentProfile);
      else
      {
         //String pname = (i_path[0] == '/') ? String(i_path.c_str()+1) : i_path;
	if(i_path[0] == '/')
          profile = Profile::CreateEmptyProfile(parentProfile);
        else
          profile = Profile::CreateProfile(i_path, parentProfile);
      }
      CHECK(profile, NULL, "can't create profile");   // return if it fails
      int typeflags = READ_CONFIG(profile, MP_FOLDER_TYPE);
      if(typeflags == MF_ILLEGAL)  // no such profile!
      {
         type = MF_FILE;
         flags = 0;
         name = i_path;
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
      //      String pname = (symbolicName[0] == '/') ? String(symbolicName.c_str()+1) : symbolicName;
      if(symbolicName[0] == '/')
        profile = Profile::CreateEmptyProfile(parentProfile);
      else
        profile = Profile::CreateProfile(symbolicName, parentProfile);

      CHECK(profile, NULL, "can't create profile");   // return if it fails

      server = i_server;
      name = i_path;
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

#ifdef EXPERIMENTAL
   case MF_MFILE:
   case MF_MDIR:
#endif
   case MF_INBOX:
      // nothing special to do
      break;

   default:
      profile->DecRef();
      FAIL_MSG("unknown folder type");
      return NULL;
   }

   MailFolder *mf;
   // all other types handles by c-client implementation:
   folderType = CombineFolderTypeAndFlags(type, flags);
   mf = MailFolderCC::OpenFolder(folderType, name, profile,
                                 server, login, passwd,
                                 symbolicName, halfopen);
   profile->DecRef();
   return mf;
}

/* static */
bool
MailFolder::DeleteFolder(const MFolder *mfolder)
{
   InitStatic();
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
   InitStatic();
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
#ifdef EXPERIMENTAL
      case MF_MDIR:
      case MF_MFILE:
#endif
         valid = true;
         break;
      default:
         valid = false; // all pseudo-types
      }
   if(! valid )
      return false;

   if( name.Length() == 0 )
      return false;

   ASSERT( (iflags & MF_FLAGSMASK) == iflags);
   FolderFlags flags = (FolderFlags) ( iflags & MF_FLAGSMASK ) ;
   Profile * p = Profile::CreateProfile(name);
   p->writeEntry(MP_PROFILE_TYPE, Profile::PT_FolderProfile);
   p->writeEntry(MP_FOLDER_TYPE, type|flags);
   if(path.Length() > 0)
      p->writeEntry(MP_FOLDER_PATH, path);
   if(comment.Length() > 0)
      p->writeEntry(MP_FOLDER_COMMENT, comment);
   p->DecRef();

   return true;
}


/* static */ String
MailFolder::ConvertMessageStatusToString(int status, MailFolder *mf)
{
   InitStatic();
   String strstatus = "";

   // We treat news differently:
   bool isNews = FALSE;
   if(mf && mf->GetType() == MF_NEWS  || mf->GetType() == MF_NNTP)
      isNews = TRUE;

   if(isNews)
   {
      if((status & MSG_STAT_SEEN) == 0)
      {
         if(status & MSG_STAT_RECENT)
            strstatus << 'U'; // unread or new!
         else
            strstatus << 'O'; // old messages, whatever that is
      }
      else
         strstatus << ' ';
   }
   else
   {
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
   }

   strstatus << ((status & MSG_STAT_FLAGGED) ? 'F' : ' ');
   strstatus << ((status & MSG_STAT_ANSWERED) ? 'A' : ' ');
   strstatus << ((status & MSG_STAT_DELETED) ? 'D' : ' ');

   return strstatus;
}


struct Dup_MsgInfo
{
   Dup_MsgInfo(const String &s, UIdType id, unsigned long size)
      { m_Id = s; m_UId = id; m_Size = size; }
   String m_Id;
   UIdType m_UId;
   unsigned long m_Size;
};

KBLIST_DEFINE(Dup_MsgInfoList, Dup_MsgInfo);

UIdType
MailFolderCmn::DeleteDuplicates()
{
   HeaderInfoList *hil = GetHeaders();
   if(! hil)
      return 0;

   Dup_MsgInfoList mlist;

   UIdArray toDelete;

   for(size_t idx = 0; idx < hil->Count(); idx++)
   {
      String   id = (*hil)[idx]->GetId();
      UIdType uid = (*hil)[idx]->GetUId();
      size_t size = (*hil)[idx]->GetSize();
      bool found = FALSE;
      for(Dup_MsgInfoList::iterator i = mlist.begin();
          i != mlist.end();
          i++)
         if( (**i).m_Id == id )
         {
            /// if new message is larger, keep it instead
            if( (**i).m_Size < size )
            {
               toDelete.Add((**i).m_UId);
               (**i).m_UId = uid;
               (**i).m_Size = size;
               found = FALSE;
            }
            else
               found = TRUE;
            break;
         }
      if(found)
         toDelete.Add(uid);
      else
      {
         Dup_MsgInfo *mi = new Dup_MsgInfo(id, uid, size);
         mlist.push_back(mi);
      }
   }
   hil->DecRef();

   if(toDelete.Count() == 0)
      return 0; // nothing to do

   if(DeleteMessages(&toDelete,FALSE))
      return toDelete.Count();
   // else - uncommented or compiler thinks there's return without value
   return UID_ILLEGAL; // an error happened
}


/* Before actually closing a mailfolder, we keep it around for a
   little while. If someone reaccesses it, this speeds things up. So
   all we need, is a little helper class to keep a list of mailfolders
   open until a timeout occurs.
*/

class MfCloseEntry
{
public:
   MailFolderCmn * m_mf;
   wxDateTime m_dt;
   ~MfCloseEntry()
      {
         wxLogTrace("mailfolder", "Mailfolder '%s': close timed out.", m_mf->GetName().c_str());
         if(m_mf) m_mf->RealDecRef();
      }
   MfCloseEntry(MailFolderCmn *mf, int secs)
      {
         m_mf = mf;
         m_dt = wxDateTime::Now();
         m_dt.Add(wxTimeSpan::Seconds(secs));
      }
};

KBLIST_DEFINE(MfList, MfCloseEntry);

class MailFolderCloser : public MObject
{
public:
   MailFolderCloser()
      : m_MfList(true)
      {
      }
   ~MailFolderCloser()
      {
         ASSERT( m_MfList.empty() );
      }

   void Add(MailFolderCmn *mf, int delay)
      {
         CHECK_RET( mf, "NULL MailFolder in MailFolderCloser::Add()");
         m_MfList.push_back(new MfCloseEntry(mf,delay));
      }
   void OnTimer(void)
      {

         MfList::iterator i;
         for(i = m_MfList.begin(); i != m_MfList.end(); i++)
         {
            if( (**i).m_dt > wxDateTime::Now() )
               m_MfList.erase(i);
         }
      }
   void CleanUp(void)
      {
         MfList::iterator i = m_MfList.begin();
         while(i != m_MfList.end() )
            m_MfList.erase(i);
      }
private:
   MfList m_MfList;
};

static MailFolderCloser *gs_MailFolderCloser = NULL;

class CloseTimer : public wxTimer
{
public:
   void Notify(void) { gs_MailFolderCloser->OnTimer(); }
};

static CloseTimer *gs_CloseTimer = NULL;

bool
MailFolderCmn::DecRef()
{
   ASSERT_MSG(gs_MailFolderCloser, "DEBUG: this must not happen (harmless but should not be the case)");
   int delay = READ_CONFIG(GetProfile(),MP_FOLDER_CLOSE_DELAY);

   if(gs_MailFolderCloser
      && delay > 0
      && GetNRef() == 1  // only real closes get delayed
      && IsAlive() )     // and only if the folder was opened
                         // successfully and is still functional
   {
     wxLogTrace("mailfolder", "Mailfolder '%s': close delayed.", GetName().c_str());
     Checkpoint(); // flush data immediately
     gs_MailFolderCloser->Add(this, delay);
     return FALSE;
   }
   else
	return RealDecRef();
}

bool
MailFolderCmn::RealDecRef()
{
   return MObjectRC::DecRef();
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
                         const MailFolder::Params& params,
                         Profile *profile,
                         MWindow *parent)
{
   ASSERT_RET(msg);
   msg->IncRef();
   if(! profile) profile = mApplication->GetProfile();

   wxComposeView *cv = wxComposeView::CreateReplyMessage(params,
                                                         profile,
                                                         msg);

         // set the recipient address
   String name;
   String email = msg->Address(name, MAT_REPLYTO);
   email = GetFullEmailAddress(name, email);
   String cc;
   if(params.flags & REPLY_FOLLOWUP) // group reply
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

   // other header   s
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
                           const MailFolder::Params& params,
                           Profile *profile,
                           MWindow *parent)
{
   ASSERT_RET(msg);
   msg->IncRef();
   if(! profile) profile = mApplication->GetProfile();

   wxComposeView *cv = wxComposeView::CreateFwdMessage(params, profile);
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
            // do update - but only if this event is for us
            MEventOptionsChangeData& data = (MEventOptionsChangeData&)event;
            if ( data.GetProfile()->IsAncestor(m_Mf->GetProfile()) )
               m_Mf->OnOptionsChange(data.GetChangeKind());
            // Also update close timer:
            if(gs_CloseTimer)
            {
               gs_CloseTimer->Stop();
               gs_CloseTimer->Start(READ_APPCONFIG(MP_FOLDER_CLOSE_DELAY) *1000);
            }
         }
         return true;
      }

private:
   MailFolderCmn *m_Mf;
   void *m_MEventCookie, *m_MEventPingCookie;
};

MailFolderCmn::MailFolderCmn()
{
#ifdef DEBUG
   m_PreCloseCalled = false;
#endif

   // We need to know if we are building the first folder listing ever
   // or not, to suppress NewMail events.
   m_FirstListing = true;
   m_ProgressDialog = NULL;
   m_UpdateFlags = UF_Default;
   m_Timer = new MailFolderTimer(this);
   m_LastNewMsgUId = UID_ILLEGAL;

   m_MEventReceiver = new MFCmnEventReceiver(this);
}

MailFolderCmn::~MailFolderCmn()
{
#ifdef DEBUG
   ASSERT(m_PreCloseCalled == true);
#endif
   delete m_Timer;
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
MailFolderCmn::SaveMessagesToFile(const UIdArray *selections,
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
MailFolderCmn::SaveMessages(const UIdArray *selections,
                            MFolder *folder,
                            bool updateCount)
{
   CHECK( folder, false, "SaveMessages() needs a valid folder pointer" );

   int n = selections->Count();

   CHECK( n, true, "SaveMessages(): nothing to save" );

   /* It could be that we are trying to open the very folder we are
      getting our messages from, so we need to temporarily unlock this
      folder. */
   bool locked = IsLocked();
   if(locked)
      UnLock(); // release our own lock

   MailFolder *mf = MailFolder::OpenFolder(folder);
   if ( !mf )
   {
      String msg;
      msg.Printf(_("Cannot save messages to folder '%s'."),
                 folder->GetFullName().c_str());
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
   for ( int i = 0; i < n; i++ )
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
   mf->Ping(); // with our flags
   mf->SetUpdateFlags(updateFlags); // restore old flags
   mf->DecRef();
   if(pd) delete pd;
   if(locked)
      Lock();
   return rc;
}

bool
MailFolderCmn::SaveMessages(const UIdArray *selections,
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
   mf->Ping(); // with our flags
   mf->SetUpdateFlags(updateFlags); // restore old flags
   mf->DecRef();
   if(pd) delete pd;
   if(locked)
      Lock();
   return rc;
}

bool
MailFolderCmn::UnDeleteMessages(const UIdArray *selections)
{
   int n = selections->Count();
   int i;
   bool rc = true;
   for(i = 0; i < n; i++)
      rc &= UnDeleteMessage((*selections)[i]);
   return rc;
}

bool
MailFolderCmn::SaveMessagesToFolder(const UIdArray *selections,
                                    MWindow *parent,
                                    MFolder *folder)
{
   bool rc = false;
   if ( !folder )
      folder = MDialog_FolderChoose(parent);
   else
      folder->IncRef(); // to match DecRef() below

   if ( folder )
   {
      rc = SaveMessages(selections, folder, true);
      folder->DecRef();
   }

   return rc;
}

bool
MailFolderCmn::SaveMessagesToFile(const UIdArray *selections, MWindow *parent)
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
MailFolderCmn::ReplyMessages(const UIdArray *selections,
                             const MailFolder::Params& params,
                             MWindow *parent)
{
   Message *msg;

   int n = selections->Count();
   for( int i = 0; i < n; i++ )
   {
      msg = GetMessage((*selections)[i]);
      ReplyMessage(msg, params, GetProfile(), parent);
      msg->DecRef();
      //now done by composeView SetMessageFlag((*selections)[i], MailFolder::MSG_STAT_ANSWERED, true);
   }
}


void
MailFolderCmn::ForwardMessages(const UIdArray *selections,
                               const MailFolder::Params& params,
                               MWindow *parent)
{
   int i;
   Message *msg;

   int n = selections->Count();
   for(i = 0; i < n; i++)
   {
      msg = GetMessage((*selections)[i]);
      ForwardMessage(msg, params, GetProfile(), parent);
      msg->DecRef();
   }
}

static MMutex gs_SortListingMutex;

static long gs_SortOrder = 0;
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
         case MSO_NONE_REV:
            result = 1; // reverse the order
            break;
         case MSO_DATE:
         case MSO_DATE_REV:
            flag = criterium == MSO_DATE ? 1 : -1;
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
            result = criterium == MSO_AUTHOR ?
               -Stricmp(i1->GetFrom(), i2->GetFrom())
               : Stricmp(i1->GetFrom(), i2->GetFrom());
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
               int score1 = i1->GetScore(),
                  score2 = i2->GetScore();
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

   // no need to incref/decref, done in UpdateListing()
   if(hil->Count() > 1)
      qsort(hil->GetArray(),
            hil->Count(),
            (*hil)[0]->SizeOf(),
            ComparisonFunction);

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
   if(hilp)
   {
      hilp->IncRef();
      SortListing(this, hilp, m_Config.m_ListingSortOrder);
      if(m_Config.m_UseThreading)
         ThreadMessages(this, hilp);
      hilp->DecRef();
   }
}

void
MailFolderCmn::UpdateMessageStatus(UIdType uid)
{
   /* I thought we could be more clever here, but it seems that
      c-client/imapd create *lots* of unnecessary status change
      entries even when only one message status changes. So we just
      tell the folder to send a single event and invalidate the
      listing. That is more economical. */
   RequestUpdate();
}

void
MailFolderCmn::OnOptionsChange(MEventOptionsChangeData::ChangeKind kind)
{
   /*
      We want to avoid rebuilding the listing unnecessary (it is an expensive
      operation) so we only do it if something really changed for us.
   */

   MFCmnOptions config;
   ReadConfig(config);
   if ( config != m_Config )
   {
      m_Config = config;

      DoUpdate();
   }
}

void
MailFolderCmn::UpdateConfig(void)
{
   ReadConfig(m_Config);

   DoUpdate();
}

void
MailFolderCmn::DoUpdate()
{
   int interval = m_Config.m_UpdateInterval * 1000;
   if ( interval != m_Timer->GetInterval() )
   {
      m_Timer->Stop();
      m_Timer->Start(interval);
   }

   RequestUpdate(true);
   MailFolder::ProcessEventQueue();
}

void
MailFolderCmn::ReadConfig(MailFolderCmn::MFCmnOptions& config)
{
   Profile *profile = GetProfile();
   config.m_ListingSortOrder = READ_CONFIG(profile, MP_MSGS_SORTBY);
   config.m_ReSortOnChange = READ_CONFIG(profile, MP_MSGS_RESORT_ON_CHANGE) != 0;
   config.m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);
   config.m_UseThreading = READ_CONFIG(profile, MP_MSGS_USE_THREADING) != 0;

   MailFolderCC::UpdateCClientConfig();
}


/** Delete a message.
    @param uid mesage uid
    @return always true UNSUPPORTED!
    */
bool
MailFolderCmn::DeleteMessage(unsigned long uid)
{
   UIdArray *ia = new UIdArray;
   ia->Add(uid);
   bool rc = DeleteMessages(ia);
   delete ia;
   return rc;
}


bool
MailFolderCmn::DeleteOrTrashMessages(const UIdArray *selections)
{
   bool reallyDelete = ! READ_CONFIG(GetProfile(), MP_USE_TRASH_FOLDER);
   // If we are the trash folder, we *really* delete.
   if(!reallyDelete && GetName() == READ_CONFIG(GetProfile(),
                                                MP_TRASH_FOLDER))
      reallyDelete = true;

   // newsgroups really delete:
   if(GetType() == MF_NNTP || GetType() == MF_NEWS)
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
//      RequestUpdate();
      Ping();
      return rc;
   }
   else
      return DeleteMessages(selections);
}

bool
MailFolderCmn::DeleteMessages(const UIdArray *selections, bool expunge)
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
   return ApplyFilterRulesCommonCode(NULL, newOnly);
}

int
MailFolderCmn::ApplyFilterRules(UIdArray msgs)
{
   return ApplyFilterRulesCommonCode(&msgs);
}

/// common code for ApplyFilterRules:
int
MailFolderCmn::ApplyFilterRulesCommonCode(UIdArray *msgs,
                                          bool newOnly)
{
   // Maybe we are lucky and have nothing to do?
   if(newOnly && CountRecentMessages() == 0)
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

      MFolder_obj folder(GetName());
      wxArrayString filters = folder->GetFilters();
      size_t count = filters.GetCount();

      for ( size_t n = 0; n < count; n++ )
      {
         MFilter_obj filter(filters[n]);
         MFilterDesc fd = filter->GetDesc();
//FIXME why?         wxLogVerbose("Using filter rule '%s'", fd.GetName().c_str());
         filterString += fd.GetRule();
      }
      if(filterString[0]) // not empty
      {
         FilterRule * filterRule = filterModule->GetFilter(filterString);
         if ( filterRule )
         {
            // This might change the folder contents, so we must set this
            // flag:
            m_FiltersCausedChange = true;
            if(msgs)
               rc = filterRule->Apply(this, *msgs);
            else
               rc = filterRule->Apply(this, newOnly);
            filterRule->DecRef();
         }
      }
      filterModule->DecRef();

      return rc;
   }
   else
   {
      wxLogVerbose("Filter module couldn't be loaded.");

      return 0; // no filter module
   }
}

// ----------------------------------------------------------------------------
// sort order conversions
// ----------------------------------------------------------------------------

// split a long value (as read from profile) into (several) sort orders
wxArrayInt SplitSortOrder(long sortOrder)
{
   wxArrayInt sortOrders;
   while ( sortOrder )
   {
      sortOrders.Add(sortOrder & 0x00000F);
      sortOrder >>= 4;
   }

   return sortOrders;
}

// combine several (max 8) sort orders into one value
long BuildSortOrder(const wxArrayInt& sortOrders)
{
   long sortOrder = 0l;

   size_t count = sortOrders.GetCount();
   if ( count > 8 )
      count = 8;
   for ( size_t n = count + 1; n > 1; n-- )
   {
      sortOrder <<= 4;
      sortOrder |= sortOrders[n - 2];
   }

   return sortOrder;
}




static void InitStatic()
{
   if(gs_CloseTimer)
      return; // nothing to do
   gs_MailFolderCloser = new MailFolderCloser;
   gs_CloseTimer = new CloseTimer();
   gs_CloseTimer->Start( READ_APPCONFIG(MP_FOLDER_CLOSE_DELAY) *1000);
}

static void CleanStatic()
{
   if(! gs_CloseTimer)
      return;
   delete gs_CloseTimer;
   gs_CloseTimer = NULL;
   gs_MailFolderCloser->CleanUp();
   delete gs_MailFolderCloser;
   gs_MailFolderCloser = NULL;
}

/* static */
void
MailFolder::CleanUp(void)
{
   CleanStatic();

   // clean up CClient driver memory
   extern void CC_Cleanup(void);
   CC_Cleanup();
}

