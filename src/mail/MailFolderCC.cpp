/*-*- c++ -*-********************************************************
 * MailFolderCC class: handling of mail folders with c-client lib   *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$         *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "MailFolderCC.h"
#endif

#include  "Mpch.h"

#ifndef  USE_PCH
#   include "strutil.h"
#   include "MApplication.h"
#   include "Profile.h"
#endif // USE_PCH

#include "Mdefaults.h"
#include "MDialogs.h"
#include "MPython.h"
#include "FolderView.h"

#include "MailFolderCC.h"
#include "MessageCC.h"

#include "MEvent.h"

// just to use wxFindFirstFile()/wxFindNextFile() for lockfile checking
#include <wx/filefn.h>

#include <ctype.h>   // isspace()

#define CHECK_DEAD(msg)   if(m_MailStream == NIL) { ERRORMESSAGE((_(msg), GetName().c_str())); return; }
#define CHECK_DEAD_RC(msg, rc)   if(m_MailStream == NIL) {   ERRORMESSAGE((_(msg), GetName().c_str())); return rc; }

#define ISO8859MARKER "=?" // "=?iso-8859-1?Q?"
#define QPRINT_MIDDLEMARKER "?Q?"

/* Little helper function to convert iso8859 encoded header lines into 
   8 bit. This is a quick fix until wxWindows supports unicode.
   Modified it now to not look at the charset argument but always do a 
   deoding for "=?xxxxx?Q?.....?=" with arbitrary xxxx. If someone has a
   different character set, he will have different fonts, so it
   should be ok.
*/

/* static */
String MailFolderCC::qprint(const String &in)
{
   int pos = in.Find(ISO8859MARKER);
   if(pos == -1)
      return in;
   String out = in.substr(0,pos);
   int pos2 = in.Find(QPRINT_MIDDLEMARKER);
   if(pos2 == -1 || pos2 < pos)
      return in;
   
   String quoted;
   const char *cptr = in.c_str() + pos2 + strlen(QPRINT_MIDDLEMARKER);
   while(*cptr && !(*cptr == '?' && *(cptr+1) == '='))
      quoted << (char) *cptr++;
   cptr += 2; // "?="
   unsigned long unused_len;
   char *cptr2 = (char *)rfc822_qprint((unsigned char *)quoted.c_str(), in.Length(), &unused_len);
   out +=  cptr2;
   out +=  cptr;
   fs_give((void **) &cptr2); // free memory allocated
   return out;
}



/*----------------------------------------------------------------------------------------
 * MailFolderCC code
 *---------------------------------------------------------------------------------------*/
String MailFolderCC::MF_user;
String MailFolderCC::MF_pwd;

// a variable telling c-client to shut up
static bool mm_ignore_errors = false;

// be quiet
static inline void CCQuiet(void) { mm_ignore_errors = true; }
// normal logging
static inline void CCVerbose(void) { mm_ignore_errors = false; }

/** This class essentially maps to the c-client Overview structure,
    which holds information for showing lists of messages.
    The m_uid member is also used to map any access to the n-th message in 
    the folder to the correct message. I.e. when requesting
    GetMessage(5), it will use the message with the m_uid from the 6th 
    entry in the list.
*/
class HeaderInfoCC : public HeaderInfo
{
public:
   virtual String const &GetSubject(void) const { return m_Subject; }
   virtual String const &GetFrom(void) const { return m_From; }
   virtual String const &GetDate(void) const { return m_Date; }
   virtual String const &GetId(void) const { return m_Id; }
   virtual unsigned long GetUId(void) const { return m_Uid; }
   virtual String const &GetReferences(void) const { return m_References; }
   virtual int GetStatus(void) const { return m_Status; }
   virtual unsigned long const &GetSize(void) const { return m_Size; }
   
protected:
   String m_Subject, m_From, m_Date, m_Id, m_References;
   int m_Status;
   unsigned long m_Size;
   unsigned long m_Uid;
   friend class MailFolderCC;
};



MailFolderCC::MailFolderCC(int typeAndFlags,
                           String const &path,
                           ProfileBase *profile,
                           String const &server,
                           String const &login,
                           String const &password)
{
   m_MailStream = NIL;

   //FIXME: server is ignored for now
   SetRetrievalLimit(0); // no limit
   m_Profile = profile;
   m_Profile->IncRef(); // we use it now
   m_MailboxPath = path;
   m_Login = login;
   m_Password = password;
   m_NumOfMessages = 0;
   m_OldNumOfMessages = 0;
   m_Listing = NULL;
   m_GenerateNewMailEvents = false; // for now don't!
   m_UpdateNeeded = true;
   m_ProgressDialog = 0;
   FolderType type = GetFolderType(typeAndFlags);
   SetType(type);
   
   if( !FolderTypeHasUserName(type) )
      m_Login = ""; // empty login for these types

}

/*
  This gets called with a folder path as its name, NOT with a symbolic 
  folder/profile name.
*/
MailFolderCC *
MailFolderCC::OpenFolder(int typeAndFlags,
                         String const &name,
                         ProfileBase *profile,
                         String const &server,
                         String const &login,
                         String const &password)
{

   MailFolderCC *mf;
   String mboxpath;

   ASSERT(profile);

   int flags = GetFolderFlags(typeAndFlags);
   int type = GetFolderType(typeAndFlags);

   switch( type )
   {
   case MF_INBOX:
      mboxpath = "INBOX";
      break;
   case MF_FILE:
      mboxpath = name;
      break;
   case MF_MH:
      mboxpath << "#mh/" << name;
      break;
   case MF_POP:
      mboxpath << '{' << server << "/pop3}";
      break;
   case MF_IMAP:  // do we need /imap flag?
      if(flags & MF_FLAGS_ANON)
         mboxpath << '{' << server << "/anonymous}" << name;
      else
         mboxpath << '{' << server << "/user=" << login << '}'<< name;
      break;
   case MF_NEWS:
      mboxpath << "#news." << name;
      break;
   case MF_NNTP:
      mboxpath << '{' << server << "/nntp}" << name;
      break;
   default:
      FAIL_MSG("Unsupported folder type.");
   }

   mf = FindFolder(mboxpath,login);
   if(mf)
   {
      mf->IncRef();
      return mf;
   }

   mf = new
      MailFolderCC(typeAndFlags,mboxpath,profile,server,login,password);
   if(mf && profile)
      mf->SetRetrievalLimit(READ_CONFIG(profile, MP_FOLDER_RETRIEVALLIMIT));
   if( mf->Open() )
      return mf;
   else
   {
      mf->DecRef();

      return NULL;
   }
}


bool
MailFolderCC::Open(void)
{
   // make sure we can use the library
   if(! cclientInitialisedFlag)
      CClientInit();

   if(GetType() == MF_POP || GetType() == MF_IMAP)
   {
      MF_user = m_Login;
      MF_pwd = m_Password;
   }

   SetDefaultObj();
   // for files, check whether mailbox is locked, c-client library is
   // to dumb to handle this properly
   if(GetType() == MF_FILE
#ifdef OS_UNIX
      || GetType() == MF_INBOX
#endif    
      )
   {
      String lockfile;
      if(GetType() == MF_FILE)
         lockfile = strutil_expandpath(m_MailboxPath);
#ifdef OS_UNIX
      else // INBOX
      {
         // get INBOX path name
         lockfile = (char *) mail_parameters (NIL,GET_SYSINBOX,NULL);
         if(lockfile.IsEmpty()) // another c-client stupidity
            lockfile = (char *) sysinbox();
         ProcessEventQueue();
      }
#endif
      lockfile << ".lock*"; //FIXME: is this fine for MS-Win?
      lockfile = wxFindFirstFile(lockfile, wxFILE);
      while ( !lockfile.IsEmpty() )
      {
         FILE *fp = fopen(lockfile,"r");
         if(fp) // outch, someone has a lock
         {
            fclose(fp);
            String msg;
            msg.Printf(_("Found lock-file:\n"
                       "'%s'\n"
                       "Some other process may be using the folder.\n"
                       "Shall I forcefully override the lock?"),
                       lockfile.c_str());
            if(MDialog_YesNoDialog(msg, NULL, MDIALOG_YESNOTITLE, true))
            {
               int success = remove(lockfile);
               if(success != 0) // error!
                  MDialog_Message(
                     _("Could not remove lock-file.\n"
                     "Other process may have terminated.\n"
                     "Will try to continue as normal."));
            }
            else
            {
               MDialog_Message(_("Cannot open the folder while "
                                 "lock-file exists."));
               return false;
            }
         }      
         lockfile = wxFindNextFile();
      }
   }

#ifndef DEBUG
   CCQuiet(); // first try, don't log errors (except in debug mode)
#endif // DEBUG

   if(GetType() == MF_FILE && ! wxFileExists(m_MailboxPath))
      mail_create(NIL, (char *)m_MailboxPath.c_str());
   
   m_MailStream = mail_open(m_MailStream,(char *)m_MailboxPath.c_str(),
                            debugFlag ? OP_DEBUG : NIL);
   ProcessEventQueue();
   SetDefaultObj(false);
   CCVerbose();
   if(m_MailStream == NIL)
   {
      // this will fail if file already exists, but it makes sure we can open it
      // try again:
      // if this fails, we want to know it, so no CCQuiet()
      mail_create(NIL, (char *)m_MailboxPath.c_str());
      ProcessEventQueue();
      SetDefaultObj();
      m_MailStream = mail_open(m_MailStream,(char *)m_MailboxPath.c_str(),
                               debugFlag ? OP_DEBUG : NIL);
      ProcessEventQueue();
      SetDefaultObj(false);
   }
   if(m_MailStream == NIL)
      return false;

   AddToMap(m_MailStream); // now we are known

   BuildListing();
   // from now on we want to know when new messages appear
   m_GenerateNewMailEvents = true;

   PY_CALLBACK(MCB_FOLDEROPEN, 0, GetProfile());
   return true;   // success
}

MailFolderCC *
MailFolderCC::FindFolder(String const &path, String const &login)
{
   StreamConnectionList::iterator i;
   for(i = streamList.begin(); i != streamList.end(); i++)
   {
      if( (*i)->name == path && (*i)->login == login)
         return (*i)->folder;
   }
   return NULL;
}

void
MailFolderCC::Ping(void)
{
   if(! m_MailStream)
      return;
   DBGMESSAGE(("MailFolderCC::Ping() on Folder %s.", GetName().c_str()));

   if(! mail_ping(m_MailStream))
   {
      LOGMESSAGE((M_LOG_DEBUG, _("Mailstream for folder '%s' has been closed, trying to reopen it."),
                  GetName().c_str()));
      bool rc = Open();
      if(rc == false)
      {
         ERRORMESSAGE((_("Re-opening folder '%s' failed."),GetName().c_str()));
      }
      else
      {
         RemoveFromMap(m_MailStream);
         m_MailStream = NIL;
      }
   }
   if(! m_MailStream)
      return;

   ProcessEventQueue();
   CCQuiet();
   mail_check(m_MailStream); // update flags, etc, .newsrc
   CCVerbose();
   ProcessEventQueue();
}

MailFolderCC::~MailFolderCC()
{
   ProcessEventQueue();
   if( m_MailStream ) mail_close(m_MailStream);
   if( m_Listing ) delete [] m_Listing;

   RemoveFromMap(m_MailStream);

   // note that RemoveFromMap already removed our node from streamList, so
   // do not do it here again!

   GetProfile()->DecRef();
}

void
MailFolderCC::RegisterView(FolderView *view, bool reg)
{
   FolderViewList::iterator
      i;
   if(reg)
      m_viewList.push_front(view);
   else
      for(i = m_viewList.begin(); i != m_viewList.end(); i++)
      {
         if((*i) == view)
         {
            // do _not_ erase() the entry, just remove it from the list
            m_viewList.remove(i);
            return;
         }
      }
}


void
MailFolderCC::AppendMessage(String const &msg)
{
   CHECK_DEAD("Appending to closed folder '%s' failed.");

   STRING str;

   INIT(&str, mail_string, (void *) msg.c_str(), msg.Length());
   ProcessEventQueue();
   if(! mail_append(NIL,(char *)m_MailboxPath.c_str(),&str))
      ERRORMESSAGE(("cannot append message"));
   ProcessEventQueue();
}

void
MailFolderCC::AppendMessage(Message const &msg)
{
   CHECK_DEAD("Appending to closed folder '%s' failed.");
   String tmp;

   msg.WriteToString(tmp);
   AppendMessage(tmp);
}

void
MailFolderCC::UpdateViews(void)
{
   FolderViewList::iterator
      i;
   for(i = m_viewList.begin(); i != m_viewList.end(); i++)
      (*i)->Update();
   PY_CALLBACK(MCB_FOLDERUPDATE, 0, GetProfile());
}

String
MailFolderCC::GetName(void) const
{
   String symbolicName;
   switch(GetType())
   {
   case MF_INBOX:
      symbolicName = "INBOX"; break;
   case MF_FILE:
   case MF_POP:
   case MF_IMAP:
   case MF_NNTP:
      symbolicName = m_MailboxPath; break;
#if 0
   case MF_POP:
      symbolicName << "pop_" << m_Login; break;
   case MF_IMAP:
      symbolicName << "imap_" << m_Login; break;
   case MF_NNTP:
      symbolicName << "news_" << m_MailboxPath; break;
#endif
   default:
      ASSERT(0);
   }
   return symbolicName;
}

long
MailFolderCC::CountMessages(void) const
{
   return m_NumOfMessages;
}

Message *
MailFolderCC::GetMessage(unsigned long uid)
{
   CHECK_DEAD_RC("Cannot access closed folder\n'%s'.", NULL);
//FIXME: add some test whether uid is valid   ASSERT(index < m_NumOfMessages && index >= 0);
   MessageCC *m = MessageCC::CreateMessageCC(this,uid);
   ProcessEventQueue();
   return m;
}

const class HeaderInfo *
MailFolderCC::GetHeaderInfo(unsigned long msgno)
{
   CHECK_DEAD_RC("Cannot access closed folder\n'%s'.", NULL);
   ASSERT(m_Listing);
   ASSERT(msgno >= 0 && msgno < m_NumOfMessages);
   return m_Listing + msgno;
}

void
MailFolderCC::SetSequenceFlag(String const &sequence,
                              int flag,
                              bool set)
{
   CHECK_DEAD("Cannot access closed folder\n'%s'.");
   String flags;

   if(flag & MSG_STAT_SEEN)
      flags <<"\\SEEN ";
   if(flag & MSG_STAT_RECENT)
      flags <<"\\RECENT ";
   if(flag & MSG_STAT_ANSWERED)
      flags <<"\\ANSWERED ";
   if(flag & MSG_STAT_DELETED)
      flags <<"\\DELETED ";
   if(flag & MSG_STAT_FLAGGED)
      flags <<"\\FLAGGED ";

   if(PY_CALLBACKVA((set ? MCB_FOLDERSETMSGFLAG : MCB_FOLDERCLEARMSGFLAG,
                     1, this, this->GetClassName(),
                     GetProfile(), "ss", sequence.c_str(), flags.c_str()),1)  )
   {
      if(set)
         mail_setflag_full(m_MailStream,
                           (char *)sequence.c_str(),
                           (char *)flags.c_str(),
                           ST_UID);
      else
         mail_clearflag_full(m_MailStream,
                        (char *)sequence.c_str(),
                        (char *)flags.c_str(),
                        ST_UID);
      ProcessEventQueue();
   }
}

void
MailFolderCC::SetMessageFlag(unsigned long uid,
                             int flag,
                             bool set)
{
   CHECK_DEAD("Cannot access closed folder\n'%s'.");
   ASSERT(m_Listing);
//FIXME uid check   ASSERT(msgno >= 0 && msgno < m_NumOfMessages);
   String sequence = strutil_ultoa(uid);
   SetSequenceFlag(sequence,flag,set);
}

void
MailFolderCC::ExpungeMessages(void)
{
   CHECK_DEAD("Cannot access closed folder\n'%s'.");
   if(PY_CALLBACK(MCB_FOLDEREXPUNGE,1,GetProfile()))
      mail_expunge (m_MailStream);
   ProcessEventQueue();
}


#ifdef DEBUG
void
MailFolderCC::Debug(void) const
{
   char buffer[1024];

   DBGLOG("--list of streams and objects--");

   StreamConnectionList::iterator
      i;

   for(i = streamList.begin(); i != streamList.end(); i++)
   {
      sprintf(buffer,"\t%p -> %p \"%s\"",
              (*i)->stream, (*i)->folder, (*i)->folder->GetName().c_str());
      DBGLOG(buffer);
   }
   DBGLOG("--end of list--");
}

String
MailFolderCC::DebugDump() const
{
   String str = MObjectRC::DebugDump();
   str << "mailbox '" << m_MailboxPath << "' of type " << m_folderType;

   return str;
}

#endif // DEBUG

/// remove this object from Map
void
MailFolderCC::RemoveFromMap(MAILSTREAM const * /* stream */)
{
   StreamConnectionList::iterator i;
   for(i = streamList.begin(); i != streamList.end(); i++)
   {
      if( (*i)->folder == this )
      {
         StreamConnection *conn = *i;
         delete conn;
         streamList.erase(i);

         break;
      }
   }

   if(streamListDefaultObj == this)
      SetDefaultObj(false);
}


/// Gets first mailfolder in map.
/* static */
MailFolderCC *
MailFolderCC::GetFirstMapEntry(StreamConnectionList::iterator &i)
{
   i = streamList.begin();
   if( i != streamList.end())
      return (**i).folder;
   else
      return NULL;
}

/// Gets next mailfolder in map.
/* static */
MailFolderCC *
MailFolderCC::GetNextMapEntry(StreamConnectionList::iterator &i)
{
   i++;
   if( i != streamList.end())
      return (**i).folder;
   else
      return NULL;
}


extern "C"
{
   static void mm_overview_header (MAILSTREAM *stream,unsigned long uid, OVERVIEW *ov);
   void mail_fetch_overview_nonuid (MAILSTREAM *stream,char *sequence,overview_t ofn);
};

void
MailFolderCC::BuildListing(void)
{
   CHECK_DEAD("Cannot access closed folder\n'%s'.");
   m_NumOfMessages = m_MailStream->nmsgs;
   
   if(m_Listing && m_NumOfMessages > m_OldNumOfMessages)
   {
      delete [] m_Listing;
      m_Listing = NULL;
   }
      
   if(! m_Listing && m_NumOfMessages > 0)
      m_Listing = new HeaderInfoCC[m_NumOfMessages];

   m_BuildNextEntry = 0;

   if(m_ProgressDialog == NULL)
   {
      String msg;
      msg.Printf(_("Reading %lu message headers..."), (unsigned long) m_NumOfMessages);
      m_ProgressDialog = new MProgressDialog(GetName(),
                                             msg,
                                             100, NULL);// open a status window:
   }
   // mail_fetch_overview() will now fill the m_Listing array with
   // info on the messages
   /* stream, sequence, header structure to fill */
   if(m_RetrievalLimit && m_NumOfMessages > m_RetrievalLimit)
   {
      String sequence =
         strutil_ultoa(mail_uid(m_MailStream, m_NumOfMessages-m_RetrievalLimit));
      sequence << ":*";
      mail_fetch_overview (m_MailStream, (char *)sequence.c_str(), mm_overview_header);
   }
   else
   {
      if(GetType() == MF_NNTP)
         // FIMXE: no idea why this works for NNTP 
         // but not for the other types
         mail_fetch_overview (m_MailStream, (char *)"1-", mm_overview_header);
      else
         mail_fetch_overview (m_MailStream, (char *)"1:*", mm_overview_header);
   }
   
   if(m_ProgressDialog != (MProgressDialog *)1)
      delete m_ProgressDialog;
   // We set it to an illegal address here to suppress further
   // updating. This value is checked against in OverviewHeader().
   // The reason is that we only want it the first time that the
   // folder is being opened.   
   m_ProgressDialog = (MProgressDialog *)1;

   // for NNTP, it will not show all messages
   //ASSERT(m_BuildNextEntry == m_NumOfMessages || m_folderType == MF_NNTP);
   m_NumOfMessages = m_BuildNextEntry;
   
   // now we sent an update event to update folderviews etc
   MEventFolderUpdateData data(this);
   MEventManager::Send(data);

   /* Now check whether we need to send new mail notifications: */
   if(m_GenerateNewMailEvents && m_NumOfMessages  > m_OldNumOfMessages) // new mail has arrived
   {
      unsigned long n = m_NumOfMessages - m_OldNumOfMessages;
      unsigned long *messageIDs = new unsigned long[n];

      // actually these are no IDs, but indices into the listing
      for ( unsigned long i = 0; i < n; i++ )
         messageIDs[i] = m_Listing[m_OldNumOfMessages + i].GetUId();

      MEventNewMailData data(this, n, messageIDs);
      MEventManager::Send(data);

      delete [] messageIDs;
   }
   m_OldNumOfMessages = m_NumOfMessages;
   m_UpdateNeeded = false;
}

void
MailFolderCC::OverviewHeaderEntry (unsigned long uid, OVERVIEW *ov)
{

   ASSERT(m_Listing);

   HeaderInfoCC & entry = m_Listing[m_BuildNextEntry];

   char tmp[MAILTMPLEN];
   ADDRESS *adr;

   unsigned long msgno = mail_msgno (m_MailStream,uid);
   MESSAGECACHE *elt = mail_elt (m_MailStream,msgno);
   MESSAGECACHE selt;

   // STATUS:
   int status = MSG_STAT_NONE;
   if(elt->recent) status |= MSG_STAT_RECENT;
   if(elt->seen) status |= MSG_STAT_SEEN;
   if(elt->flagged) status |= MSG_STAT_FLAGGED;
   if(elt->answered) status |= MSG_STAT_ANSWERED;
   if(elt->deleted) status |= MSG_STAT_DELETED;
   entry.m_Status = (MailFolder::MessageStatus) status;

   /* For NNTP, do not show deleted messages: */
   if(m_folderType == MF_NNTP && elt->deleted)
      return;
   
   // DATE
   mail_parse_date (&selt,ov->date);
   mail_date (tmp,&selt);  //FIXME: is this ok? Use our date format!
   entry.m_Date = tmp;

   // FROM
   /* get first from address from envelope */
   for (adr = ov->from; adr && !adr->host; adr = adr->next);
   if (adr) /* if a personal name exists use it */
      if (adr->personal) // a personal name is given
         entry.m_From.Printf("%s <%s@%s>", adr->personal,adr->mailbox,adr->host);
      else
         entry.m_From.Printf("%s@%s",adr->mailbox,adr->host);
   else
      entry.m_From = _("<address missing>");
   entry.m_From = qprint(entry.m_From);
#if 0
   //FIXME: what on earth are user flags?
   if (i = elt->user_flags)
   {
      strcat (tmp,"{");
      while (i)
      {
         strcat (tmp,stream->user_flags[find_rightmost_bit (&i)]);
         if (i) strcat (tmp," ");
      }
      strcat (tmp,"} ");
   }
#endif

   entry.m_Subject = qprint(ov->subject);
   entry.m_Size = ov->optional.octets;
   entry.m_Id = ov->message_id;
   entry.m_References = ov->references;
   entry.m_Uid = uid;
   m_BuildNextEntry++;

   // This is 1 if we don't want any further updates.
   if(m_ProgressDialog != (MProgressDialog *)1)
      m_ProgressDialog->Update( (100 * m_BuildNextEntry)/m_NumOfMessages);
}


/// Return a pointer to the first message's header info.
HeaderInfo const *
MailFolderCC::GetFirstHeaderInfo(void) const
{
   return (m_NumOfMessages > 0 ) ? m_Listing : NULL;
}

/// Return a pointer to the next message's header info.
HeaderInfo const *
MailFolderCC::GetNextHeaderInfo(HeaderInfo const* last) const
{
   ASSERT(m_Listing);
   HeaderInfoCC const *lastCC = (HeaderInfoCC *)last;
   
   if(lastCC >= m_Listing+m_NumOfMessages-1) return NULL;
   return (lastCC+1);
}


//-------------------------------------------------------------------
// static class member functions:
//-------------------------------------------------------------------

StreamConnectionList MailFolderCC::streamList(FALSE);

bool MailFolderCC::cclientInitialisedFlag = false;

MailFolderCC *MailFolderCC::streamListDefaultObj = NULL;

void
MailFolderCC::CClientInit(void)
{
#include <linkage.c>
   cclientInitialisedFlag = true;
}

/// adds this object to Map
void
MailFolderCC::AddToMap(MAILSTREAM const *stream)
{
   StreamConnection  *conn = new StreamConnection;
   conn->folder = this;
   conn->stream = stream;
   conn->name = m_MailboxPath;
   conn->login = m_Login;
   streamList.push_front(conn);
}


/// lookup object in Map
MailFolderCC *
MailFolderCC::LookupObject(MAILSTREAM const *stream, const char *name)
{
   StreamConnectionList::iterator i;
   for(i = streamList.begin(); i != streamList.end(); i++)
      if( (*i)->stream == stream )
         return (*i)->folder;
   /* Sometimes the IMAP code (imap4r1.c) allocates a temporary stream 
      for e.g. mail_status(), that is difficult to handle here, we
      must compare the name parameter which might not be 100%
      identical, but we can check the hostname for identity and the
      path. However, that seems a bit far-fetched for now, so I just
      make sure that streamListDefaultObj gets set before any call to
      mail_status(). If that doesn't work, we need to parse the name
      string for hostname, portnumber and path and compare these.
      //FIXME: This might be an issue for MT code.
   */
   if(name)
   {
      for(i = streamList.begin(); i != streamList.end(); i++)
      {
         if( (*i)->name == name )  // (*i)->name is of type String,  so we can
            return (*i)->folder;
      }
   }
   if(streamListDefaultObj)
   {
      LOGMESSAGE((M_LOG_DEBUG, "Routing call to default mailfolder."));
      return streamListDefaultObj;
   }

   LOGMESSAGE((M_LOG_ERROR,"Cannot find mailbox for callback!"));
   return NULL;
}

void
MailFolderCC::SetDefaultObj(bool setit)
{
   if(setit)
      streamListDefaultObj = this;
   else
      streamListDefaultObj = NULL;
}



/// this message matches a search
void
MailFolderCC::mm_searched(MAILSTREAM * /* stream */,
                          unsigned long /* number */)
{
   //FIXME
}

/// tells program that there are this many messages in the mailbox
void
MailFolderCC::mm_exists(MAILSTREAM *stream, unsigned long number)
{
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
   {
#ifdef DEBUG
      String tmp = "MailFolderCC::mm_exists() for folder " + mf->m_MailboxPath
                   + String(" n: ") + strutil_ultoa(number);
      LOGMESSAGE((M_LOG_DEBUG, Str(tmp)));
#endif
      mf->m_NumOfMessages = number;
      mf->RequestUpdate();
   }
   else
   {
      FAIL_MSG("new mail in unknown mail folder?");
   }
}


/** deliver stream message event
       @param stream mailstream
       @param str message str
       @param errflg error level
       */
void
MailFolderCC::mm_notify(MAILSTREAM * /* stream */, String str, long errflg)
{
   mm_log(str,errflg);
}


/** this mailbox name matches a listing request
       @param stream mailstream
       @param delim  character that separates hierarchies
       @param name   mailbox name
       @param attrib mailbox attributes
       */
void
MailFolderCC::mm_list(MAILSTREAM * /* stream */,
                      char /* delim */,
                      String /* name */,
                      long /* attrib */)
{
   //FIXME
}


/** matches a subscribed mailbox listing request
       @param stream mailstream
       @param delim  character that separates hierarchies
       @param name   mailbox name
       @param attrib mailbox attributes
       */
void
MailFolderCC::mm_lsub(MAILSTREAM * /* stream */,
                      char /* delim */,
                      String /* name */,
                      long /* attrib */)
{
   //FIXME
}

/** status of mailbox has changed
    @param stream mailstream
    @param mailbox   mailbox name for this status
    @param status structure with new mailbox status
*/
void
MailFolderCC::mm_status(MAILSTREAM *stream,
                        String  mailbox ,
                        MAILSTATUS *status)
{
   MailFolderCC *mf = LookupObject(stream, mailbox);
   if(mf == NULL)
      return;  // oops?!

   wxLogDebug("mm_status: folder '%s', %lu messages",
              mf->m_MailboxPath.c_str(), status->messages);

   if(status->flags & SA_MESSAGES)
      mf->m_NumOfMessages  = status->messages;
   mf->UpdateViews();
}


/** log a message
    @param str message str
    @param errflg error level
*/
void
MailFolderCC::mm_log(String str, long /* errflg */)
{
   String  msg = _("c-client log: ");
   msg += str;
   LOGMESSAGE((M_LOG_INFO, Str(msg)));
   const char *unexpected = "Unexpected change";
   if(strncmp(str,unexpected, strlen(unexpected)) == 0)
   {
      // try to reopen the stream
      // Problem: we don't know the stream, so we need to ping them
      // all
      StreamConnectionList::iterator i;
      MailFolderCC *mf;
      for(mf = MailFolderCC::GetFirstMapEntry(i);
          mf;
          mf = MailFolderCC::GetNextMapEntry(i))
         mf->Ping();
   }
}

/** log a debugging message
    @param str    message str
*/
void
MailFolderCC::mm_dlog(String str)
{
   String msg = _("c-client debug: ");
   // check for PASS
   if ( str[0u] == 'P' && str[1u] == 'A' && str[2u] == 'S' && str[3u] == 'S' )
   {
      msg += "PASS";

      size_t n = 4;
      while ( isspace(str[n]) )
      {
         msg += str[n++];
      }

      // hide the password
      while ( n < str.length() )
      {
         msg += '*';
         n++;
      }
   }
   else
   {
      msg += str;
   }
   
   LOGMESSAGE((M_LOG_DEBUG, Str(msg)));
}

/** get user name and password
       @param  mb parsed mailbox specification
       @param  user    pointer where to return username
       @param  pwd   pointer where to return password
       @param  trial number of prior login attempts
       */
void
MailFolderCC::mm_login(NETMBX * /* mb */,
                       char *user,
                       char *pwd,
                       long /* trial */)
{
   strcpy(user,MF_user.c_str());
   strcpy(pwd,MF_pwd.c_str());
}

/** alert that c-client will run critical code
       @param  stream   mailstream
   */
void
MailFolderCC::mm_critical(MAILSTREAM * /* stream */)
{
   // ignore
}

/**   no longer running critical code
   @param   stream mailstream
     */
void
MailFolderCC::mm_nocritical(MAILSTREAM * /* stream */)
{
   // ignore
}

/** unrecoverable write error on mail file
       @param  stream   mailstream
       @param  errcode  OS error code
       @param  serious  non-zero: c-client cannot undo error
       @return abort flag: if serious error and abort non-zero: abort, else retry
       */
long
MailFolderCC::mm_diskerror(MAILSTREAM *stream,
                           long /* errcode */,
                           long /* serious */)
{
//   MailFolderCC *mf = LookupObject(stream);
   return 1;   // abort!
}

/** program is about to crash!
    @param  str   message str
*/
void
MailFolderCC::mm_fatal(char *str)
{
   String   msg = (String) "Fatal Error:" + (String) str;
   LOGMESSAGE((M_LOG_ERROR, Str(msg)));
}


/* static */
MailFolderCC::EventQueue MailFolderCC::ms_EventQueue;

/* static */
void
MailFolderCC::ProcessEventQueue(void)
{
   Event *evptr;

   while(! ms_EventQueue.empty())
   {
      evptr = ms_EventQueue.pop_front();
      switch(evptr->m_type)
      {
         /* The following events don't have much meaning yet. */
      case Searched:
         MailFolderCC::mm_searched(evptr->m_stream,  evptr->m_args[0].m_ulong);
         break;
      case Notify:
         MailFolderCC::mm_notify(evptr->m_stream,
                                 *(evptr->m_args[0].m_str),
                                 evptr->m_args[1].m_long);
         delete evptr->m_args[0].m_str;
         break;
      case List:
         MailFolderCC::mm_list(evptr->m_stream,
                               evptr->m_args[0].m_int,
                               *(evptr->m_args[1].m_str),
                               evptr->m_args[2].m_long);
         delete evptr->m_args[1].m_str;
         break;
      case LSub:
         MailFolderCC::mm_lsub(evptr->m_stream, 
                               evptr->m_args[0].m_int,
                               *(evptr->m_args[1].m_str),
                               evptr->m_args[2].m_long);
         delete evptr->m_args[1].m_str;
         break;
      case Log:
         MailFolderCC::mm_log(*(evptr->m_args[0].m_str),
                              evptr->m_args[1].m_long);
         delete evptr->m_args[0].m_str;
         break;
      case DLog:
         MailFolderCC::mm_dlog(*(evptr->m_args[0].m_str));
         delete evptr->m_args[0].m_str;
         break;
      case Exists:
         /* The Exists event is ignored here.
            When the mm_exists() callback is called, the
            m_NumOfMessages counter is updated immediately,
            circumvening the event queue. It should never appear.
            The mm_exists() callback calls RequestUpdate() though.*/
         ASSERT(0);
         break;
            /* These three events all notify us of changes to the folder
            which we need to incorporate into our listing of the
            folder. So we request an update which marks the folder to
            be updated and generates an Update event which will be
            processed later in this loop. */
      case Status:
      case Flags:
      case Expunged:
      {
         MailFolderCC *mf = MailFolderCC::LookupObject(evptr->m_stream);
         ASSERT(mf);
         mf->RequestUpdate();  // Queues an Update event.
         break;
      }
      /* The Update event is not caused by c-client callbacks but 
         by this very event handling mechanism itself. It causes
         BuildListing() to fetch a new listing. */
      case Update:
      {
         MailFolderCC *mf = MailFolderCC::LookupObject(evptr->m_stream);
         ASSERT(mf);
         if(mf->UpdateNeeded())  // only call it once
            mf->BuildListing();
         break;
      }
      }// switch
      delete evptr;
   }
}

void
MailFolderCC::RequestUpdate(void)   
{
   if(! m_UpdateNeeded)
   {
      // we want only one update event
      MailFolderCC::Event *evptr = new MailFolderCC::Event(m_MailStream,Update);   
      MailFolderCC::QueueEvent(evptr);
      m_UpdateNeeded = true;
   }
}

/* Handles the mm_overview_header callback on a per folder basis. */
void
MailFolderCC::OverviewHeader (MAILSTREAM *stream, unsigned long uid, OVERVIEW *ov)
{
   MailFolderCC *mf = MailFolderCC::LookupObject(stream);
   ASSERT(mf);
   mf->OverviewHeaderEntry(uid, ov);
}

extern "C"
{
/* Mail fetch message overview using sequence numbers instead of UIDs!
 * Accepts: mail stream
 *	    sequence to fetch (no-UIDs but sequence numbers)
 *	    pointer to overview return function
 */

void mail_fetch_overview_nonuid (MAILSTREAM *stream,char *sequence,overview_t ofn)
{
   if (stream->dtb &&
       !(stream->dtb->overview && (*stream->dtb->overview)(stream,sequence,ofn)) &&
       mail_sequence (stream,sequence) &&
       mail_ping (stream))
   {
      MESSAGECACHE *elt;
      ENVELOPE *env;
      OVERVIEW ov;
      unsigned long i;
      ov.optional.lines = 0;
      ov.optional.xref = NIL;
      for (i = 1; i <= stream->nmsgs; i++)
         if (((elt = mail_elt (stream,i))->sequence) &&
             (env = mail_fetch_structure (stream,i,NIL,NIL)) && ofn)
         {
            ov.subject = env->subject;
            ov.from = env->from;
            ov.date = env->date;
            ov.message_id = env->message_id;
            ov.references = env->references;
            ov.optional.octets = elt->rfc822_size;
            (*ofn) (stream,mail_uid (stream,i),&ov);
         }
   }
}
} // extern "C"

// the callbacks:
extern "C"
{

void
mm_searched(MAILSTREAM *stream, unsigned long number)
{
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Searched);
   evptr->m_args[0].m_ulong = number;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_expunged(MAILSTREAM *stream, unsigned long number)
{
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Expunged);
   evptr->m_args[0].m_ulong = number;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_flags(MAILSTREAM *stream, unsigned long number)
{
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Flags);
   evptr->m_args[0].m_ulong = number;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_notify(MAILSTREAM *stream, char *str, long
             errflg)
{
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Notify);
   evptr->m_args[0].m_str = new String(str);
   evptr->m_args[1].m_long = errflg;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_list(MAILSTREAM *stream, int delim, char *name, long attrib)
{
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::List);
   evptr->m_args[0].m_int = delim;
   evptr->m_args[1].m_str = new String(name);
   evptr->m_args[2].m_long = attrib;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_lsub(MAILSTREAM *stream, int delim, char *name, long attrib)
{
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::LSub);
   evptr->m_args[0].m_int = delim;
   evptr->m_args[1].m_str = new String(name);
   evptr->m_args[2].m_long = attrib;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_exists(MAILSTREAM *stream, unsigned long number)
{
   // update count immediately to reflect change:
   MailFolderCC::mm_exists(stream, number);
}

void
mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS *status)
{
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Status);
   MailFolderCC::QueueEvent(evptr);
}

void
mm_log(char *str, long errflg)
{
   if(mm_ignore_errors)
      return;
   
   MailFolderCC::Event *evptr = new MailFolderCC::Event(NULL,MailFolderCC::Log);
   evptr->m_args[0].m_str = new String(str);
   evptr->m_args[1].m_long = errflg;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_dlog(char *str)
{
   MailFolderCC::Event *evptr = new MailFolderCC::Event(NULL,MailFolderCC::DLog);
   evptr->m_args[0].m_str = new String(str);
   MailFolderCC::QueueEvent(evptr);
}

void
mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
   // Cannot use event system, has to be called while c-client is working.
   MailFolderCC::mm_login(mb, user, pwd, trial);
}

void
mm_critical(MAILSTREAM *stream)
{
   MailFolderCC::mm_critical(stream);
}

void
mm_nocritical(MAILSTREAM *stream)
{
   MailFolderCC::mm_nocritical(stream);
}

long
mm_diskerror(MAILSTREAM *stream, long int errorcode, long int serious)
{
   return MailFolderCC::mm_diskerror(stream,  errorcode, serious);
}

void
mm_fatal(char *str)
{
   MailFolderCC::mm_fatal(str);
}

/* This callback needs to be processed immediately or ov will become
   invalid. */
static void
mm_overview_header (MAILSTREAM *stream,unsigned long uid, OVERVIEW *ov)
{
   MailFolderCC::OverviewHeader(stream, uid, ov);
}

} // extern "C"
