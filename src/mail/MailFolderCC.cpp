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

String MailFolderCC::MF_user;
String MailFolderCC::MF_pwd;

// a variable telling c-client to shut up
static bool mm_ignore_errors = false;

// be quiet
static inline void CCQuiet(void) { mm_ignore_errors = true; }
// normal logging
static inline void CCVerbose(void) { mm_ignore_errors = false; }

/** This class essentially maps to the c-client Overview structure,
    which holds information for showing lists of messages. */
class HeaderInfoCC : public HeaderInfo
{
public:
   virtual String const &GetSubject(void) const { return m_Subject; }
   virtual String const &GetFrom(void) const { return m_From; }
   virtual String const &GetDate(void) const { return m_Date; }
   virtual String const &GetId(void) const { return m_Id; }
   virtual String const &GetReferences(void) const { return m_References; }
   virtual String const &GetStatus(void) const { return m_Status; }
   virtual unsigned long const &GetSize(void) const { return m_Size; }
protected:
   String m_Subject, m_From, m_Date, m_Id, m_References, m_Status;
   unsigned long m_Size;
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
   m_Profile = profile;
   m_Profile->IncRef(); // we use it now
   m_MailboxPath = path;
   m_Login = login;
   m_Password = password;
   m_numOfMessages = 0;
   m_Listing = NULL;
   m_GenerateNewMailEvents = false; // for now don't!
   FolderType type = GetFolderType(typeAndFlags);
   SetType(type);

   if( !FolderTypeHasUserName(type) )
      m_Login = ""; // empty login for these types

}

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

   mf = new MailFolderCC(typeAndFlags,mboxpath,profile,server,login,password);
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
         lockfile = m_MailboxPath;
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

   // TESTING
   HeaderInfo const *hi;
   for(hi = GetFirstHeaderInfo(); hi != NULL; hi = GetNextHeaderInfo(hi))
   {
      cout << hi->GetSubject() << ' ' << hi->GetFrom() << ' '
           << hi->GetDate() << ' ' << hi->GetId() << endl
           << "    " << hi->GetReferences() << ' ' << hi->GetStatus()
           << hi->GetSize() << endl;
   }
   
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
   DBGMESSAGE(("MailFolderCC::Ping() on Folder %s.", m_MailboxPath.c_str()));

   mail_ping(m_MailStream);
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
      symbolicName = m_MailboxPath; break;
   case MF_POP:
      symbolicName << "pop_" << m_Login; break;
   case MF_IMAP:
      symbolicName << "imap_" << m_Login; break;
   case MF_NNTP:
      symbolicName << "news_" << m_Login;
   default:
      // just to keep the compiler happy
      ;
   }
   return symbolicName;
}

long
MailFolderCC::CountMessages(void) const
{
   return m_numOfMessages;
}

Message *
MailFolderCC::GetMessage(unsigned long index)
{
   MessageCC *m = MessageCC::CreateMessageCC(this,mail_uid(m_MailStream,index),index);
   ProcessEventQueue();
   return m;
}

void
MailFolderCC::SetSequenceFlag(String const &sequence,
                              int flag,
                              bool set)
{
   const char *flagstr;

   switch(flag)
   {
   case MSG_STAT_UNREAD:
      flagstr = "\\SEEN"; set = set ? false : true;
      break;
   case MSG_STAT_REPLIED:
      flagstr = "\\ANSWERED";
      break;
   case MSG_STAT_DELETED:
      flagstr = "\\DELETED";
      break;
   default:
      return;
   }

   if(PY_CALLBACKVA((set ? MCB_FOLDERSETMSGFLAG : MCB_FOLDERCLEARMSGFLAG,
                     1, this, this->GetClassName(),
                     GetProfile(), "ss", sequence.c_str(), flagstr),1)  )
   {
      if(set)
         mail_setflag(m_MailStream, (char *)sequence.c_str(), (char *)flagstr);
      else
         mail_clearflag(m_MailStream, (char *)sequence.c_str(), (char *)flagstr);
      ProcessEventQueue();
   }
}

void
MailFolderCC::SetMessageFlag(unsigned long msgno,
                             int flag,
                             bool set)
{
   String sequence = strutil_ultoa(msgno);
   SetSequenceFlag(sequence,flag,set);
}

void
MailFolderCC::ExpungeMessages(void)
{
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

void
MailFolderCC::UpdateCount(void)
{
   unsigned long oldnum = m_numOfMessages;
   m_numOfMessages = n;

   UpdateViews();

   if(m_GenerateNewMailEvents && n  > oldnum) // new mail has arrived
   {
      n = m_numOfMessages - oldnum;
      unsigned long *messageIDs = new unsigned long[n];

      // actually these are no IDs, but sequence numbers, which is fine.
      // KB: I think it should be "New" AND "Unseen"
      for ( unsigned long i = 0; i < (unsigned long)n; i++ )
         messageIDs[i] = oldnum + i + 1;

      MEventNewMailData data(this, messageIDs);
      MEventManager::Send(data);

      delete [] messageIDs;
   }
   oldnum = numOfMessages;
}

extern "C"
{
   static void mm_overview_header (MAILSTREAM *stream,unsigned long uid, OVERVIEW *ov);
   void mail_fetch_overview_nonuid (MAILSTREAM *stream,char *sequence,overview_t ofn);
};

void
MailFolderCC::BuildListing(void)
{
   //FIXME:MT any use of SetDefaultObj() needs to be protected by a semaphore
      
   SetDefaultObj();  // imap4r1 does temporary strings
   // this will set the number ofmessages
   mail_status(m_MailStream, (char *)m_MailboxPath.c_str(), SA_MESSAGES|SA_RECENT|SA_UNSEEN);
   ProcessEventQueue();
   SetDefaultObj(false);
   // now we know how many messages there are

   if(m_Listing) delete [] m_Listing;
   m_Listing = new HeaderInfoCC[m_numOfMessages];

   m_BuildNextEntry = 0;
   String sequence = "1:";
   sequence << strutil_ultoa(m_numOfMessages);
   // mail_fetch_overview() will now fill the m_Listing array with
   // info on the messages
   /* stream, sequence, header structure to fill */
   mail_fetch_overview_nonuid (m_MailStream, (char *)sequence.c_str(), mm_overview_header);
   ASSERT(m_BuildNextEntry == m_numOfMessages);
}

void
MailFolderCC::OverviewHeaderEntry (unsigned long uid, OVERVIEW *ov)
{

   ASSERT(m_Listing);

   HeaderInfoCC & entry = m_Listing[m_BuildNextEntry++];

   char tmp[MAILTMPLEN];
   ADDRESS *adr;

   unsigned long msgno = mail_msgno (m_MailStream,uid);
   MESSAGECACHE *elt = mail_elt (m_MailStream,msgno);
   MESSAGECACHE selt;

   // STATUS:
   entry.m_Status = "";
   entry.m_Status << (elt->recent ? (elt->seen ? 'R': 'N') : ' ');
   entry.m_Status << ((elt->recent | elt->seen) ? ' ' : 'U');
   entry.m_Status << (elt->flagged ? 'F' : ' ');
   entry.m_Status << (elt->answered ? 'A' : ' ');
   entry.m_Status << (elt->deleted ? 'D' : ' ');

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

   entry.m_Subject = ov->subject;
   entry.m_Size = ov->optional.octets;
   entry.m_Id = ov->message_id;
}


/// Return a pointer to the first message's header info.
HeaderInfo const *
MailFolderCC::GetFirstHeaderInfo(void) const
{
   ASSERT(m_Listing);
   return m_Listing;
}

/// Return a pointer to the next message's header info.
HeaderInfo const *
MailFolderCC::GetNextHeaderInfo(HeaderInfo const* last) const
{
   ASSERT(m_Listing);
   HeaderInfoCC const *lastCC = (HeaderInfoCC *)last;
   
   if(lastCC >= m_Listing+m_numOfMessages-1) return NULL;
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
      mf->numOfMessages = number;
   }
   else
   {
      FAIL_MSG("new mail in unknown mail folder?");
   }
}


/// this message has been expunged, subsequent numbers changed
void
MailFolderCC::mm_expunged(MAILSTREAM *stream, unsigned long number)
{
   String msg = "Expunged message no. " + strutil_ltoa(number);
   LOGMESSAGE((M_LOG_DEFAULT, Str(msg)));
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
   {
      mf->m_numOfMessages--;
      mf->UpdateViews();
   }
}

/// flags have changed for a message
void
MailFolderCC::mm_flags(MAILSTREAM *stream, unsigned long /* number */)
{
   //String msg = "Flags changed for msg no. " + strutil_ltoa(number);
   //LOGMESSAGE((M_LOG_DEFAULT, Str(msg)));
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
      mf->UpdateViews();
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
      mf->m_numOfMessages  = status->messages;
   mf->UpdateViews();
}


/** log a message
    @param str message str
    @param errflg error level
*/
void
MailFolderCC::mm_log(String str, long /* errflg */)
{
   if(mm_ignore_errors)
      return;

   String  msg = _("c-client log: ");
   msg += str;

   LOGMESSAGE((M_LOG_INFO, Str(msg)));
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


MailFolderCC::EventQueue MailFolderCC::ms_EventQueue;

void
MailFolderCC::ProcessEventQueue(void)
{
   Event *evptr;
   MailFolderCC *mf;
   
   while(! ms_EventQueue.empty())
   {
      evptr = ms_EventQueue.pop_front();
      switch(evptr->m_type)
      {
      case Searched:
         MailFolderCC::mm_searched(evptr->m_stream,  evptr->m_args[0].m_ulong);
         break;
      case Exists:
         // the msg count has already been updated, but may need to generate a 
         // new mail event
         if((mf = LookupObject(evptr->m_stream)) != NULL)
            mf->UpdateCount();
         break;
      case Expunged:
         MailFolderCC::mm_expunged(evptr->m_stream,  evptr->m_args[0].m_ulong);
         break;
      case Flags:
         MailFolderCC::mm_flags(evptr->m_stream,  evptr->m_args[0].m_ulong);
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
      case Status:
         ASSERT(0); // we should never reach here
#if 0
         MailFolderCC::mm_status(evptr->m_stream,
                                 *(evptr->m_args[0].m_str),
                                 evptr->m_args[1].m_status);
         delete evptr->m_args[0].m_str;
         delete evptr->m_args[1].m_status;
#endif
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
      }
      delete evptr;
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
  if (stream->dtb && !(stream->dtb->overview &&
		       (*stream->dtb->overview) (stream,sequence,ofn)) &&
      mail_sequence (stream,sequence) && mail_ping (stream)) {
    MESSAGECACHE *elt;
    ENVELOPE *env;
    OVERVIEW ov;
    unsigned long i;
    ov.optional.lines = 0;
    ov.optional.xref = NIL;
    for (i = 1; i <= stream->nmsgs; i++)
      if (((elt = mail_elt (stream,i))->sequence) &&
	  (env = mail_fetch_structure (stream,i,NIL,NIL)) && ofn) {
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
}

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
mm_exists(MAILSTREAM *stream, unsigned long number)
{
   // update count immediately to reflect change:
   MailFolderCC::mm_exists(stream, number);
   // generate an event to trigger checks for new mail after processing this
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Exists);
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
   MailFolderCC::mm_lsub(stream, delim, name, attrib);
}

void
mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS *status)
{
/* All we are using so far is the message about the change in the
   number of messages, so we just call mm_exists() here which does the 
   job for us. */
   if(status->flags & SA_MESSAGES)
      mm_exists(stream,status->messages);
/* Handling status changes is non-trivial as the number of messages
   can have changed when the event is processed. */
#if 0
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Status);
   evptr->m_args[0].m_str = new String(mailbox);
   evptr->m_args[1].m_status = new MAILSTATUS;
   memcpy(evptr->m_args[1].m_status, status, sizeof (MAILSTATUS));
   MailFolderCC::QueueEvent(evptr);
//   MailFolderCC::mm_status(stream, mailbox, status);
#endif
}

void
mm_log(char *str, long errflg)
{
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
