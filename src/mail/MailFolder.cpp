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

// define this for some additional checks of folder closing logic
#undef DEBUG_FOLDER_CLOSE

#include <wx/timer.h>
#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/dynarray.h>

#include "MDialogs.h" // for MDialog_FolderChoose

#include "Mdefaults.h"

#include "MFilter.h"
#include "MThread.h"

#include "Message.h"
#include "MailFolder.h"
#include "HeaderInfo.h"
#include "MailFolderCC.h" // CC_Cleanup (FIXME: shouldn't be there!)

#ifdef EXPERIMENTAL
#include "MMailFolder.h"
#endif

#include "miscutil.h"   // GetFullEmailAddress

#include "gui/wxComposeView.h"
#include "wx/persctrl.h"
#include "MApplication.h"
#include "modules/Filters.h"

// ----------------------------------------------------------------------------
// trace masks
// ----------------------------------------------------------------------------

// trace new mail processing
#define TRACE_NEWMAIL "newmail"

// trace mail folder closing
#define TRACE_MF_CLOSE "mfclose"

// trace mail folder ref counting
#define TRACE_MF_REF "mfref"

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
         wxLogTrace("logcircle", "checking msg %d, %s", i, m_Messages[i].c_str());
         if(m_Messages[i].Contains(needle))
         {
            if(store) *store = m_Messages[i];
            return true;
         }
      }
   // search from m_N-1 down to m_Next:
   for(int i = m_N-1; i >= m_Next; i--)
   {
      wxLogTrace("logcircle", "checking msg %d, %s", i, m_Messages[i].c_str());
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

/* static */
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
   // check for various POP3 server messages telling us that another session
   // is active
   else if ( Find("mailbox locked", &err) ||
             Find("lock busy", &err) ||
             Find("[IN-USE]", &err) )
   {
       guess = _("The mailbox is already opened from another session,\n"
                 "you should close it there first.");
       addErr = true;
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

// ----------------------------------------------------------------------------
// MailFolder opening
// ----------------------------------------------------------------------------

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

   /* So far the drivers do an auto-create when we open a folder, so
      now we attempt to open the folder to see what happens: */
   MFolder *mf = MFolder::Get(name);
   MailFolder *mf2 = MailFolder::OpenFolder(mf);
   SafeDecRef(mf2);
   SafeDecRef(mf);

   p->DecRef();

   return true;
}

// ----------------------------------------------------------------------------
// message flags
// ----------------------------------------------------------------------------

/* static */ String
MailFolder::ConvertMessageStatusToString(int status)
{
   InitStatic();

   String strstatus;

   // in IMAP/cclient the NEW == RECENT && !SEEN while for most people it is
   // just NEW == !SEEN
   if ( status & MSG_STAT_RECENT )
   {
      // 'R' == RECENT && SEEN (doesn't make much sense if you ask me)
      strstatus << ((status & MSG_STAT_SEEN) ? 'R' : 'N');
   }
   else // !recent
   {
      // we're interested in showing the UNSEEN messages
      strstatus << ((status & MSG_STAT_SEEN) ? ' ' : 'U');
   }

   strstatus << ((status & MSG_STAT_FLAGGED) ? '*' : ' ')
             << ((status & MSG_STAT_ANSWERED) ? 'A' : ' ')
             << ((status & MSG_STAT_DELETED) ? 'D' : ' ');

   return strstatus;
}

String FormatFolderStatusString(const String& format,
                                const String& name,
                                MailFolderStatus *status,
                                const MailFolder *mf)
{
   // this is a wrapper class which catches accesses to MailFolderStatus and
   // fills it with the info from the real folder if it is not yet initialized
   class StatusInit
   {
   public:
      StatusInit(MailFolderStatus *status,
                 const String& name,
                 const MailFolder *mf)
         : m_name(name)
      {
         m_status = status;
         m_mf = (MailFolder *)mf; // cast away const for IncRef()
         SafeIncRef(m_mf);

         m_init = false;
      }

      MailFolderStatus *operator->() const
      {
         if ( !m_init )
         {
            ((StatusInit *)this)->m_init = true;

            // query the mail folder for info
            MailFolder *mf;
            if ( !m_mf )
            {
               MFolder_obj mfolder(m_name);
               mf = MailFolder::OpenFolder(mfolder);
            }
            else // already have the folder
            {
               mf = m_mf;
               mf->IncRef();
            }

            if ( mf )
            {
               mf->CountInterestingMessages(m_status);
               mf->DecRef();
            }
         }

         return m_status;
      }

      ~StatusInit()
      {
         SafeDecRef(m_mf);
      }

   private:
      MailFolderStatus *m_status;
      MailFolder *m_mf;
      String m_name;
      bool m_init;
   } stat(status, name, mf);

   String result;
   const char *start = format.c_str();
   for ( const char *p = start; *p; p++ )
   {
      if ( *p == '%' )
      {
         switch ( *++p )
         {
            case '\0':
               wxLogWarning(_("Unexpected end of string in the status format "
                              "string '%s'."), start);
               p--; // avoid going beyond the end of string
               break;

            case 'f':
               result += name;
               break;

            case 'i':               // flagged == important
               result += strutil_ultoa(stat->flagged);
               break;


            case 't':               // total
               result += strutil_ultoa(stat->total);
               break;

            case 'r':               // recent
               result += strutil_ultoa(stat->recent);
               break;

            case 'n':               // new
               result += strutil_ultoa(stat->newmsgs);
               break;

            case 's':               // search result
               result += strutil_ultoa(stat->searched);
               break;

            case 'u':               // unseen
               result += strutil_ultoa(stat->unseen);
               break;

            case '%':
               result += '%';
               break;

            default:
               wxLogWarning(_("Unknown macro '%c%c' in the status format "
                              "string '%s'."), *(p-1), *p, start);
         }
      }
      else // not a format spec
      {
         result += *p;
      }
   }

   return result;
}

// VZ: disabling this stuff - it is a workaround for the bug which doesn't exist
//     any more
#if 0
// ----------------------------------------------------------------------------
// eliminating duplicate messages code
// ----------------------------------------------------------------------------

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
#endif // 0

// ----------------------------------------------------------------------------
// ping
// ----------------------------------------------------------------------------

/* static */
bool MailFolder::PingAllOpened(void)
{
   // FIXME this should be fixed by moving the list of all opened folders into
   //       MailFolder itself from MailFolderCC
   return MailFolderCC::PingAllOpened();
}

// ----------------------------------------------------------------------------
// folder closing
// ----------------------------------------------------------------------------

/* Before actually closing a mailfolder, we keep it around for a
   little while. If someone reaccesses it, this speeds things up. So
   all we need, is a little helper class to keep a list of mailfolders
   open until a timeout occurs.
*/

class MfCloseEntry
{
public:
   enum
   {
      // special value for MfCloseEntry() ctor delay parameter
      NEVER_EXPIRES = INT_MAX
   };

   MfCloseEntry(MailFolderCmn *mf, int secs)
      {
         m_mf = mf;

         // keep it for now
         m_mf->IncRef();

         m_expires = secs != NEVER_EXPIRES;
         m_timeout = secs;

         ResetTimeout();

#ifdef DEBUG_FOLDER_CLOSE
         m_deleting = false;
#endif // DEBUG_FOLDER_CLOSE
      }

   ~MfCloseEntry()
      {
         if ( m_mf )
         {
            wxLogTrace(TRACE_MF_CLOSE, "Really closing mailfolder '%s'",
                       m_mf->GetName().c_str());

#ifdef DEBUG_FOLDER_CLOSE
            m_deleting = true;
#endif // DEBUG_FOLDER_CLOSE

            m_mf->RealDecRef();
         }
      }

   // restart the expire timer
   void ResetTimeout()
   {
      if ( m_expires )
      {
         m_dt = wxDateTime::Now() + wxTimeSpan::Seconds(m_timeout);
      }
      //else: no need to (re)set timeout which never expires, leave m_dt as is
   }

   // the folder we're going to close
   MailFolderCmn *m_mf;

   // the time when we will close it
   wxDateTime m_dt;

   // the timeout in seconds we're waiting before closing the folder
   int m_timeout;

   // if false, timeout is infinite
   bool m_expires;

#ifdef DEBUG_FOLDER_CLOSE
   // set to true just before deleting the folder
   bool m_deleting;
#endif // DEBUG_FOLDER_CLOSE
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
#ifdef DEBUG_FOLDER_CLOSE
         // this shouldn't happen as we only add the folder to the list when
         // it is about to be deleted and it can't be deleted if we're holding
         // to it (MfCloseEntry has a lock), so this would be an error in ref
         // counting (extra DecRef()) elsewhere
         ASSERT_MSG( !GetCloseEntry(mf),
                     "adding a folder to MailFolderCloser twice??" );
#endif // DEBUG_FOLDER_CLOSE

         CHECK_RET( mf, "NULL MailFolder in MailFolderCloser::Add()");
         m_MfList.push_back(new MfCloseEntry(mf,delay));
      }

   void OnTimer(void)
      {
         MfList::iterator i;
         for( i = m_MfList.begin(); i != m_MfList.end();)
         {
            MfCloseEntry *entry = *i;
            if ( entry->m_expires && (entry->m_dt > wxDateTime::Now()) )
               i = m_MfList.erase(i);
            else
               ++i;
         }
      }
   void CleanUp(void)
      {
         MfList::iterator i = m_MfList.begin();
         while(i != m_MfList.end() )
            i = m_MfList.erase(i);
      }

   MfCloseEntry *GetCloseEntry(MailFolderCmn *mf) const
   {
      for ( MfList::iterator i = m_MfList.begin();
            i != m_MfList.end();
            i++ )
      {
         MfCloseEntry *entry = *i;

         // if the deleting flag is set, we're removing the folder from the
         // list right now, so the check for !GetCloseEntry() in RealDecRef()
         // below shouldn't succeed
         if ( entry->m_mf == mf
#ifdef DEBUG_FOLDER_CLOSE
               && !entry->m_deleting
#endif // DEBUG_FOLDER_CLOSE
            )
            return entry;
      }

      return NULL;
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
   // don't keep folders alive artificially if we're going to terminate soon
   // anyhow
   if ( gs_MailFolderCloser && !mApplication->IsShuttingDown() )
   {
      switch ( GetNRef() )
      {
         case 1:
            // the folder is going to be really closed if this is the last
            // DecRef(), delay closing of the folder after checking that it
            // was opened successfully and is atill functional
            if ( IsAlive() )
            {
               int delay;

               if ( GetFlags() & MF_FLAGS_KEEPOPEN )
               {
                  // never close it at all
                  delay = MfCloseEntry::NEVER_EXPIRES;
               }
               else
               {
                  // don't close it only if the linger delay is set
                  delay = READ_CONFIG(GetProfile(), MP_FOLDER_CLOSE_DELAY);
               }

               if ( delay > 0 )
               {
                  wxLogTrace(TRACE_MF_CLOSE,
                             "Mailfolder '%s': close delayed for %d seconds.",
                             GetName().c_str(), delay);

                  Checkpoint(); // flush data immediately

                  gs_MailFolderCloser->Add(this, delay);
               }
            }
            break;

         case 2:
            // if the folder is already in gs_MailFolderCloser list the
            // remaining lock is hold by it and will be removed when the
            // timeout expires, but we want to reset the timeout as the folder
            // was just accessed and so we want to keep it around longer
            {
               MfCloseEntry *entry = gs_MailFolderCloser->GetCloseEntry(this);
               if ( entry )
               {
                  // restart the expire timer
                  entry->ResetTimeout();
               }
            }
            break;
      }
   }

   return RealDecRef();
}

// temp hack: so I can put a breakpoint on all allotments of this class
#ifdef DEBUG_FOLDER_CLOSE
void
MailFolderCmn::IncRef()
{
   wxLogTrace(TRACE_MF_REF, "MF(%s)::IncRef(): %u -> %u",
              GetName().c_str(), m_nRef, m_nRef + 1);

   MObjectRC::IncRef();
}
#endif // DEBUG_FOLDER_CLOSE

bool
MailFolderCmn::RealDecRef()
{
#ifdef DEBUG_FOLDER_CLOSE
   wxLogTrace(TRACE_MF_REF, "MF(%s)::DecRef(): %u -> %u",
              GetName().c_str(), m_nRef, m_nRef - 1);

   // check that the folder is not in the mail folder closer list any more if
   // we're going to delete it
   if ( m_nRef == 1 )
   {
      if ( gs_MailFolderCloser && gs_MailFolderCloser->GetCloseEntry(this) )
      {
         // we will crash later when removing it from the list!
         FAIL_MSG("deleting folder still in MailFolderCloser list!");
      }
   }
#endif // DEBUG_FOLDER_CLOSE

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

// ----------------------------------------------------------------------------
// message operations
// ----------------------------------------------------------------------------

void
MailFolder::ReplyMessage(class Message *msg,
                         const MailFolder::Params& params,
                         Profile *profile,
                         MWindow *parent)
{
   CHECK_RET(msg, "no message to reply to");
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
                           const MailFolder::Params& params,
                           Profile *profile,
                           MWindow *parent)
{
   CHECK_RET(msg, "no message to forward");
   msg->IncRef();
   if(! profile) profile = mApplication->GetProfile();

   wxComposeView *cv = wxComposeView::CreateFwdMessage(params, profile);
   cv->SetSubject(READ_CONFIG(profile, MP_FORWARD_PREFIX)+ msg->Subject());
   cv->InitText(msg);
   cv->Show(TRUE);
   SafeDecRef(msg);
}

char MailFolder::GetFolderDelimiter() const
{
   switch ( GetType() )
   {
      default:
         FAIL_MSG( "Don't call GetFolderDelimiter() for this type" );
         // fall through nevertheless

      case MF_FILE:
      case MF_MFILE:
      case MF_POP:
         // the folders of this type don't have subfolders at all
         return '\0';

      case MF_MH:
      case MF_MDIR:
         // the filenames use slash as separator
         return '/';

      case MF_NNTP:
      case MF_NEWS:
         // newsgroups components are separated by periods
         return '.';

      case MF_IMAP:
         // for IMAP this depends on server!
         FAIL_MSG( "shouldn't be called for IMAP, unknown delimiter" );

         // guess :-(
         return '/';
   }
}

// ----------------------------------------------------------------------------
// MFCmnEventReceiver
// ----------------------------------------------------------------------------

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

// ============================================================================
// MailFolderCmn: higher level functionality
// ============================================================================

// ----------------------------------------------------------------------------
// MailFolderCmn creation/destruction
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// MailFolderCmn message saving
// ----------------------------------------------------------------------------

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
   if ( threshold > 0 && n > threshold )
   {
      wxString msg;
      msg.Printf(_("Saving %d messages to the file '%s'..."),
                 n, fileName0.c_str());

      pd = new MProgressDialog(GetName(), msg, 2*n, NULL);
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

   MailFolder *mf = MailFolder::OpenFolder(folder);
   if ( !mf )
   {
      String msg;
      msg.Printf(_("Cannot save messages to folder '%s'."),
                 folder->GetFullName().c_str());
      ERRORMESSAGE((msg));
      return false;
   }

   if ( mf->IsLocked() )
   {
      FAIL_MSG( "Can't SaveMessages() to locked folder" );

      mf->DecRef();

      return false;
   }

   int updateFlags = mf->GetUpdateFlags();
   mf->SetUpdateFlags( updateCount ? UF_UpdateCount : 0 );

   MProgressDialog *pd = NULL;
   int threshold = mf->GetProfile() ?
      READ_CONFIG(mf->GetProfile(), MP_FOLDERPROGRESS_THRESHOLD)
      : MP_FOLDERPROGRESS_THRESHOLD_D;

   if ( threshold > 0 && n > threshold )
   {
      // open a progress window:
      wxString msg;
      msg.Printf(_("Saving %d messages to the folder '%s'..."),
                 n, folder->GetName().c_str());

      pd = new MProgressDialog(mf->GetName(), msg, 2*n, NULL);
   }

   bool rc = true;
   for ( int i = 0; i < n; i++ )
   {
      Message *msg = GetMessage((*selections)[i]);
      if(msg)
      {
         if(pd)
            pd->Update( 2*i + 1 );
         rc &= mf->AppendMessage(*msg, FALSE);
         if(pd)
            pd->Update( 2*i + 2);

         msg->DecRef();
      }
   }

   mf->Ping(); // with our flags
   mf->SetUpdateFlags(updateFlags); // restore old flags
   mf->DecRef();

   delete pd;

   return rc;
}

bool
MailFolderCmn::SaveMessages(const UIdArray *selections,
                            String const & folderName,
                            bool isProfile,
                            bool updateCount)
{
   // this shouldn't happen any more, use SaveMessages(MFolder)
   CHECK( isProfile, false, "obsolete version of SaveMessages called" );

   MFolder_obj folder(folderName);
   if ( !folder.IsOk() )
   {
      wxLogError(_("Impossible to save messages to not existing folder '%s'."),
                 folderName.c_str());

      return false;
   }

   return SaveMessages(selections, folder, updateCount);
}

bool
MailFolderCmn::SaveMessagesToFolder(const UIdArray *selections,
                                    MWindow *parent,
                                    MFolder *folder)
{
   bool rc;
   if ( !folder )
      folder = MDialog_FolderChoose(parent);
   else
      folder->IncRef(); // to match DecRef() below

   if ( folder )
   {
      if ( CanCreateMessagesInFolder(folder->GetType()) )
      {
         rc = SaveMessages(selections, folder, true);
      }
      else // we can't copy/move the messages there
      {
         wxLogError(_("Impossible to copy messages in the folder '%s'.\n"
                      "You can't create messages in the folders of this type."),
                    folder->GetFullName().c_str());
         rc = false;
      }

      folder->DecRef();
   }
   else // no folder to save to
   {
      rc = false;
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

// ----------------------------------------------------------------------------
// MailFolderCmn message replying/forwarding
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// MailFolderCmn sorting
// ----------------------------------------------------------------------------

// we can only sort one listing at a time currently as we use global data to
// pass information to the C callback used by qsort() - if this proves to be a
// significant limitation, we should reimplement sorting ourselves
static struct SortData
{
   // the struct must be locked while in use
   MMutex mutex;

   // the sort order to use
   long order;

   // the listing which we're sorting
   HeaderInfoList *hil;
} gs_SortData;

// return negative number if a < b, 0 if a == b and positive number if a > b
#define CmpNumeric(a, b) ((a)-(b))

static int CompareStatus(int stat1, int stat2)
{
   // deleted messages are considered to be less important than undeleted ones
   if ( stat1 & MailFolder::MSG_STAT_DELETED )
   {
      if ( ! (stat2 & MailFolder::MSG_STAT_DELETED ) )
         return -1;
   }
   else if ( stat2 & MailFolder::MSG_STAT_DELETED )
   {
      return 1;
   }

   /*
      We use a scoring system:

      recent = +1
      unseen   = +1
      answered = -1

      (by now, messages are either both deleted or neither one is)
   */

   int
      score1 = 0,
      score2 = 0;

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

   return CmpNumeric(score1, score2);
}

extern "C"
{
   static int ComparisonFunction(const void *p1, const void *p2)
   {
      // check that the caller didn't forget to acquire the global data lock
      ASSERT_MSG( gs_SortData.mutex.IsLocked(), "using unlocked gs_SortData" );

      size_t n1 = *(size_t *)p1,
             n2 = *(size_t *)p2;

      HeaderInfo *i1 = gs_SortData.hil->GetItemByIndex(n1),
                 *i2 = gs_SortData.hil->GetItemByIndex(n2);

      // copy it as we're going to modify it while processing
      long sortOrder = gs_SortData.order;

      int result = 0;
      while ( result == 0 && sortOrder != 0 )
      {
         long criterium = sortOrder & 0xF;
         sortOrder >>= 4;

         // we rely on MessageSortOrder values being what they are: as _REV
         // version immediately follows the normal order constant, we should
         // reverse the comparison result for odd values of MSO_XXX
         int reverse = criterium % 2;
         switch ( criterium - reverse )
         {
            case MSO_NONE:
               break;

            case MSO_DATE:
               result = CmpNumeric(i1->GetDate(), i2->GetDate());
               break;

            case MSO_SUBJECT:
               {
                  String
                     subj1 = strutil_removeReplyPrefix(i1->GetSubject()),
                     subj2 = strutil_removeReplyPrefix(i2->GetSubject());

                  result = Stricmp(subj1, subj2);
               }
               break;

            case MSO_AUTHOR:
               result = Stricmp(i1->GetFrom(), i2->GetFrom());
               break;

            case MSO_STATUS:
               result = CompareStatus(i1->GetStatus(), i2->GetStatus());
               break;

            case MSO_SIZE:
               result = CmpNumeric(i1->GetSize(), i2->GetSize());
               break;

            case MSO_SCORE:
               result = CmpNumeric(i1->GetScore(), i2->GetScore());
               break;

            default:
               FAIL_MSG("unknown sorting criterium");
         }

         if ( reverse )
         {
            if ( criterium == MSO_NONE )
            {
               // special case: result is 0 for MSO_NONE but -1 (reversed order)
               // for MSO_NONE_REV
               result = -1;
            }
            else // for all other cases just revert the result value
            {
               result = -result;
            }
         }
      }

      return result;
   }
}

// ----------------------------------------------------------------------------
// threading
// ----------------------------------------------------------------------------

/*
   FIXME:

   1. we should use server side threading when possible
   2. this algorithm is O(N^2) and awfully inefficient in its using of
      Set/GetIndentation() for its own temporary data
 */

#define MFCMN_INDENT1_MARKER   ((size_t)-1)
#define MFCMN_INDENT2_MARKER   ((size_t)-2)

static void AddDependents(size_t &idx,
                          int level,
                          int i,
                          size_t indices[],
                          size_t indents[],
                          HeaderInfoList *hilp,
                          SizeTList dependents[])
{
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
   CHECK_RET( hilp, "no listing to thread" );

   HeaderInfoList_obj hil(hilp);

   size_t count = hil->Count();

   if ( count <= 1 )
   {
      // nothing to be done: one message is never threaded
      return;
   }

   MFrame *frame = mf->GetInteractiveFrame();
   if ( frame )
   {
      wxLogStatus(frame, _("Threading %u messages..."), count);
   }

   // reset indentation first
   size_t i;
   for ( i = 0; i < count; i++ )
   {
      hil[i]->SetIndentation(0);
   }

   // We need a list of dependent messages for each entry.
   SizeTList *dependents = new SizeTList[count];

   for ( i = 0; i < count; i++ )
   {
      String id = hil[i]->GetId();

      if ( !id.empty() ) // no Id lines in Outbox!
      {
         for ( size_t j = 0; j < count; j++ )
         {
            String references = hil[j]->GetReferences();
            if ( references.Find(id) != wxNOT_FOUND )
            {
               dependents[i].push_back(new size_t(j));
               hil[j]->SetIndentation(MFCMN_INDENT2_MARKER);
            }
         }
      }
   }

   /* Now we have all the dependents set up properly and can re-build
      the list. We now build an array of indices for the new list.
   */

   size_t *indices = new size_t[count];
   size_t *indents = new size_t[count];
   memset(indents, 0, count*sizeof(size_t));

   size_t idx = 0; // where to store next entry
   for (i = 0; i < count; i++ )
   {
      // we mark used indices with a non-0 indentation, so only the top level
      // messages still have 0 indentation
      if ( hil[i]->GetIndentation() == 0 )
      {
         ASSERT(idx < (*hilp).Count());

         indices[idx++] = i;
         hil[i]->SetIndentation(MFCMN_INDENT1_MARKER);
         AddDependents(idx, 1, i, indices, indents, hilp, dependents);
      }
   }

   // we should have found all the messages
   ASSERT_MSG( idx == hilp->Count(), "logic error in threading code" );

   // we use AddTranslationTable() instead of SetTranslationTable() as this one
   // should be combined with the existing table from sorting
   hilp->AddTranslationTable(indices);

   for ( i = 0; i < hilp->Count(); i++ )
   {
      hil[i]->SetIndentation(indents[i]);
   }

   delete [] indices;
   delete [] indents;
   delete [] dependents;

   if ( frame )
   {
      wxLogStatus(frame, _("Threading %lu messages... done."), count);
   }
}

static void SortListing(MailFolder *mf, HeaderInfoList *hil, long sortOrder)
{
   CHECK_RET( hil, "no listing to sort" );

   // don't sort the listing if we don't have any sort criterium (so sorting
   // "by arrival order" will be much faster!)
   if ( sortOrder != 0 )
   {
      size_t count = hil->Count();
      if ( count >= 1 )
      {
         MFrame *frame = mf->GetInteractiveFrame();
         if ( frame )
         {
            wxLogStatus(frame, _("Sorting %u messages..."), count);
         }

         MLocker lock(gs_SortData.mutex);
         gs_SortData.order = sortOrder;
         gs_SortData.hil = hil;

         // start with unsorted listing
         size_t *transTable = new size_t[count];
         for ( size_t n = 0; n < count; n++ )
         {
            transTable[n] = n;
         }

         // now sort it
         qsort(transTable, count, sizeof(size_t), ComparisonFunction);

         // and tell the listing to use it (it will delete it)
         hil->SetTranslationTable(transTable);

         // just in case
         gs_SortData.hil = NULL;

         if ( frame )
         {
            wxLogStatus(frame, _("Sorting %u messages... done."), count);
         }
      }
      //else: avoid sorting empty listing or listing of 1 element
   }
   //else: configured not to do any sorting

   hil->DecRef();
}

// ----------------------------------------------------------------------------
// MailFolderCmn new mail check
// ----------------------------------------------------------------------------

/*
  This function is called by GetHeaders() immediately after building a
  new folder listing. It checks for new mails and, if required, sends
  out new mail events.
*/
void
MailFolderCmn::CheckForNewMail(HeaderInfoList *hilp)
{
   UIdType n = hilp->Count();
   if ( !n )
      return;

   UIdType *messageIDs = new UIdType[n];

   wxLogTrace(TRACE_NEWMAIL,
              "CheckForNewMail(): folder: %s highest seen uid: %lu.",
              GetName().c_str(), (unsigned long) m_LastNewMsgUId);

   // new messages are supposed to have UIDs greater than the last new message
   // seen, but not all messages with greater UID are new, so we have to first
   // find all messages with such UIDs and then check if they're really new

   // when we check for the new mail the first time, m_LastNewMsgUId is still
   // invalid and so we must reset it before comparing with it
   bool firstTime = m_LastNewMsgUId == UID_ILLEGAL;
   if ( firstTime )
      m_LastNewMsgUId = 0;

   // Find the new messages:
   UIdType nextIdx = 0;
   UIdType highestId = m_LastNewMsgUId;
   for ( UIdType i = 0; i < n; i++ )
   {
      HeaderInfo *hi = hilp->GetItemByIndex(i);
      UIdType uid = hi->GetUId();
      if ( uid > m_LastNewMsgUId )
      {
         if ( IsNewMessage(hi) )
         {
            messageIDs[nextIdx++] = uid;
         }

         if ( uid > highestId )
            highestId = uid;
      }
   }

   ASSERT_MSG( nextIdx <= n, "more new messages than total?" );

   // do we want new mail events?
   if ( m_UpdateFlags & UF_DetectNewMail )
   {
      if ( nextIdx != 0)
         MEventManager::Send(new MEventNewMailData(this, nextIdx, messageIDs));
      //else: no new messages found
   }

   if ( m_UpdateFlags & UF_UpdateCount )
      m_LastNewMsgUId = highestId;

   wxLogTrace(TRACE_NEWMAIL,
              "CheckForNewMail() after test: folder: %s highest seen uid: %lu.",
              GetName().c_str(), (unsigned long) highestId);


   delete [] messageIDs;
}


/* This will do the work in the new mechanism:

   - For now it only sorts or threads the headerinfo list
 */
void
MailFolderCmn::ProcessHeaderListing(HeaderInfoList *hilp)
{
   CHECK_RET( hilp, "no listing to process" );

   hilp->IncRef();
   SortListing(this, hilp, m_Config.m_ListingSortOrder);

   if ( m_Config.m_UseThreading )
   {
      hilp->IncRef();
      ThreadMessages(this, hilp);
   }
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

      // listing has been resorted/rethreaded
      RequestUpdate();
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

      if ( interval > 0 ) // interval of zero == disable ping timer
         m_Timer->Start(interval);
   }

   if ( HasHeaders() )
   {
      HeaderInfoList *hil = GetHeaders();
      if ( hil )
      {
         ProcessHeaderListing(hil);
         hil->DecRef();
      }
   }
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

// ----------------------------------------------------------------------------
// MailFolderCmn message deletion
// ----------------------------------------------------------------------------

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


/** Delete a message.
    @param uid mesage uid
    @return true if ok
    */
bool
MailFolderCmn::DeleteMessage(unsigned long uid)
{
   UIdArray a;
   a.Add(uid);

   // delete without expunging
   return DeleteMessages(&a);
}


bool
MailFolderCmn::DeleteOrTrashMessages(const UIdArray *selections)
{
   CHECK( CanDeleteMessagesInFolder(GetType()), FALSE,
          "can't delete messages in this folder" );

   // we can either "really delete" the messages (in fact, just mark them as
   // deleted) or move them to trash which implies deleting and expunging them
   bool reallyDelete = !READ_CONFIG(GetProfile(), MP_USE_TRASH_FOLDER);

   // If we are the trash folder, we *really* delete
   wxString trashFolderName = READ_CONFIG(GetProfile(), MP_TRASH_FOLDER);
   if ( !reallyDelete && GetName() == trashFolderName )
      reallyDelete = true;

   bool rc;
   if ( reallyDelete )
   {
      // delete without expunging
      rc = DeleteMessages(selections, FALSE);
   }
   else // move to trash
   {
      rc = SaveMessages(selections,
                        trashFolderName,
                        true /* is profile */,
                        false /* don´t update */);
      if ( rc )
      {
         // delete and expunge
         rc = DeleteMessages(selections, TRUE);
      }
   }

   return rc;
}

bool
MailFolderCmn::DeleteMessages(const UIdArray *selections, bool expunge)
{
   String seq = GetSequenceString(selections);
   if ( seq.empty() )
   {
      // nothing to do
      return true;
   }

   bool rc = SetSequenceFlag(seq, MailFolder::MSG_STAT_DELETED);
   if ( rc && expunge )
      ExpungeMessages();

   return rc;
}

// ----------------------------------------------------------------------------
// MailFolderCmn filtering
// ----------------------------------------------------------------------------

/// apply the filters to the selected messages
int
MailFolderCmn::ApplyFilterRules(UIdArray msgs)
{
   // Has the folder got any filter rules set? Concat all its filters together
   String filterString;

   MFolder_obj folder(GetName());
   wxArrayString filters = folder->GetFilters();
   size_t count = filters.GetCount();

   for ( size_t n = 0; n < count; n++ )
   {
      MFilter_obj filter(filters[n]);
      MFilterDesc fd = filter->GetDesc();
      filterString += fd.GetRule();
   }

   int rc = 0;
   if ( !filterString.empty() )
   {
      // Obtain pointer to the filtering module:
      MModule_Filters *filterModule = MModule_Filters::GetModule();

      if ( filterModule )
      {
         FilterRule *filterRule = filterModule->GetFilter(filterString);
         if ( filterRule )
         {
            wxBusyCursor busyCursor;

            // don't ignore deleted messages here (hence "false")
            rc = filterRule->Apply(this, msgs, false);

            filterRule->DecRef();
         }
         else
         {
            wxLogWarning(_("Error in filter code for folder '%s', "
                           "filters not applied"),
                         filterString.c_str());

            rc = FilterRule::Error;
         }

         filterModule->DecRef();
      }
      else // no filter module
      {
         wxLogWarning(_("Filter module couldn't be loaded, "
                        "filters not applied"));

         rc = FilterRule::Error;
      }
   }
   else // no filters to apply
   {
      wxLogVerbose(_("No filters configured for this folder."));
   }

   return rc;
}

/** Checks for new mail and filters if necessary.
    Returns FilterRule flag combination
*/
int
MailFolderCmn::FilterNewMail(HeaderInfoList *hil)
{
   CHECK(hil, 0, "FilterNewMail() needs header info list");

   int rc = 0;

   // Maybe we are lucky and have nothing to do?
   size_t count = hil->Count();
   if ( count > 0 )
   {
      UIdType nRecent = CountRecentMessages();
      if ( nRecent > 0 )
      {
         // Obtain pointer to the filtering module:
         MModule_Filters *filterModule = MModule_Filters::GetModule();
         if(filterModule)
         {
            // Build an array of NEW message UIds to apply the filters to:
            UIdArray messages;
            for ( size_t idx = 0; idx < count; idx++ )
            {
               HeaderInfo *hi = hil->GetItemByIndex(idx);
               int status = hi->GetStatus();

               // only take NEW (== RECENT && !SEEN) and ignore DELETED
               if ( (status & 
                     (MSG_STAT_RECENT | MSG_STAT_SEEN | MSG_STAT_DELETED))
                     == MSG_STAT_RECENT )
               {
                  messages.Add(hi->GetUId());
               }
            }

            // build a single program from all filter rules:
            String filterString;
            MFolder_obj folder(GetName());
            wxArrayString filters;
            if ( folder )
               filters = folder->GetFilters();
            size_t countFilters = filters.GetCount();
            for ( size_t n = 0; n < countFilters; n++ )
            {
               MFilter_obj filter(filters[n]);
               MFilterDesc fd = filter->GetDesc();
               filterString += fd.GetRule();
            }
            if(filterString[0]) // not empty
            {
               // translate filter program:
               FilterRule * filterRule = filterModule->GetFilter(filterString);
               // apply it:
               if ( filterRule )
               {
                  // true here means "ignore deleted"
                  rc |= filterRule->Apply(this, messages, true);

                  filterRule->DecRef();
               }
            }
            filterModule->DecRef();
         }
         else
         {
            wxLogVerbose("Filter module couldn't be loaded.");

            rc = FilterRule::Error;
         }
      }

      // do we have any messages to move?
      if ( (GetFlags() & MF_FLAGS_INCOMING) != 0)
      {
         // where to we move the mails?
         String newMailFolder = READ_CONFIG(GetProfile(), MP_NEWMAIL_FOLDER);
         if ( newMailFolder == GetName() )
         {
            ERRORMESSAGE((_("Cannot collect mail from folder '%s' into itself."),
                         GetName().c_str()));
         }
         else // ok, move to another folder
         {
            UIdArray messages;
            for ( size_t idx = 0; idx < count; idx++ )
            {
               // check that the message isn't deleted - avoids getting 2 (or
               // more!) copies of the same "new" message (first normal,
               // subsequent deleted) if, for whatever reason, we failed to
               // expunge the messages the last time we collected mail from here
               HeaderInfo *hi = hil->GetItemByIndex(idx);
               if ( !(hi->GetStatus() & MSG_STAT_DELETED) )
               {
                  messages.Add(hi->GetUId());
               }
            }

            if ( SaveMessages(&messages,
                              newMailFolder,
                              true /* isProfile */,
                              false /* update count */))
            {
               // delete and expunge
               DeleteMessages(&messages, true);

               rc |= FilterRule::Expunged;
            }
            else // don't delete them if we failed to move
            {
               ERRORMESSAGE((_("Cannot move newly arrived messages.")));
            }
         }
      }
   }

   hil->DecRef();

   return rc;
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

// ----------------------------------------------------------------------------
// static functions used by MailFolder
// ----------------------------------------------------------------------------

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

   // any MailFolderCmn::DecRef() shouldn't add folders to gs_MailFolderCloser
   // from now on, so NULL it immediately
   MailFolderCloser *mfCloser = gs_MailFolderCloser;
   gs_MailFolderCloser = NULL;

   mfCloser->CleanUp();
   delete mfCloser;
}

/* static */
bool
MailFolder::Init()
{
   InitStatic();

   return CC_Init();
}

/* static */
void
MailFolder::CleanUp(void)
{
   CleanStatic();

   // clean up CClient driver memory
   CC_Cleanup();
}

// ----------------------------------------------------------------------------
// interactive frame stuff
// ----------------------------------------------------------------------------

String MailFolder::ms_interactiveFolder;
MFrame *MailFolder::ms_interactiveFrame = NULL;

/* static */
void MailFolder::SetInteractive(MFrame *frame, const String& foldername)
{
   ms_interactiveFolder = foldername;
   ms_interactiveFrame = frame;
}

/* static */
void MailFolder::ResetInteractive()
{
   SetInteractive(NULL, "");
}

MFrame *MailFolder::SetInteractiveFrame(MFrame *frame)
{
   MFrame *frameOld = m_frame;
   m_frame = frame;
   return frameOld;
}

MFrame *MailFolder::GetInteractiveFrame() const
{
   if ( m_frame )
      return m_frame;

   if ( GetName() == ms_interactiveFolder )
      return ms_interactiveFrame;

   return NULL;
}

