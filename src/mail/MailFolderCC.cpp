/*-*- c++ -*-********************************************************
 * MailFolderCC class: handling of mail folders with c-client lib   *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
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
#   include "MDialogs.h"
#   include "Profile.h"
#endif // USE_PCH

#include "Mdefaults.h"
#include "MPython.h"
#include "FolderView.h"

#include "MailFolderCC.h"
#include "MessageCC.h"

String MailFolderCC::MF_user;
String MailFolderCC::MF_pwd;

// a variable telling c-client to shut up
static bool mm_ignore_errors = false;

// be quiet
static void CCQuiet(void) { mm_ignore_errors = true; }
// normal logging
static void CCVerbose(void) { mm_ignore_errors = false; }


MailFolderCC::MailFolderCC(MailFolder::Type type,
                           String const &path,
                           ProfileBase *profile,
                           String const &login,
                           String const &password)
{
   m_MailStream = NIL;

   m_Profile = profile;
   m_Profile->IncRef(); // we use it now
   m_MailboxPath = path;
   m_Login = login;
   m_Password = password;
   type = (MailFolder::Type) (type & MF_TYPEMASK);
   SetType(type);
   if(type == MF_INBOX || type == MF_FILE)
      m_Login = ""; // empty login for these types
      
}

MailFolderCC *
MailFolderCC::OpenFolder(MailFolder::Type type,
                         String const &name,
                         ProfileBase *profile,
                         String const &login,
                         String const &password)
{

   MailFolderCC *mf;
   String mboxpath;

   type = (MailFolder::Type)(type & MailFolder::MF_TYPEMASK);

   switch(type)
   {
   case MailFolder::MF_INBOX:
      mboxpath = "INBOX";
      break;
   case MailFolder::MF_FILE:
      if ( !IsAbsPath(name) )
         mboxpath << READ_APPCONFIG(MP_MBOXDIR) << '/';
      mboxpath << name;
      break;
   case MailFolder::MF_POP:
      mboxpath << '{' << name << "/pop3}";
      break;
   case MailFolder::MF_IMAP:
      mboxpath << '{' << name << "/imap}";
      break;
   case MailFolder::MF_NNTP:
      mboxpath << '{' << name << "/nntp}" << login;
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

   mf = new MailFolderCC(type,mboxpath,profile,login,password);
   if(mf->Open())
      return mf;
   else
      mf->DecRef();
   return NULL;
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
   CCQuiet(); // first try, don't log errors
   m_MailStream = mail_open(m_MailStream,(char *)m_MailboxPath.c_str(),
                          debugFlag ? OP_DEBUG : NIL);
   SetDefaultObj(false);
   CCVerbose();
   if(m_MailStream == NIL)
   {
      // this will fail if file already exists, but it makes sure we can open it
      // try again:
      // if this fails, we want to know it, so no CCQuiet()
      mail_create(NIL, (char *)m_MailboxPath.c_str());
      SetDefaultObj();
      m_MailStream = mail_open(m_MailStream,(char *)m_MailboxPath.c_str(),
                             debugFlag ? OP_DEBUG : NIL);
      SetDefaultObj(false);
   }
   if(m_MailStream == NIL)
      return false;

   AddToMap(m_MailStream); // now we are known

   mail_status(m_MailStream, (char *)m_MailboxPath.c_str(), SA_MESSAGES|SA_RECENT|SA_UNSEEN);
   String sequence = String("1:") + strutil_ultoa(numOfMessages);
   mail_fetchfast(m_MailStream, (char *)sequence.c_str());

   okFlag = true;
   if(okFlag)
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
}

MailFolderCC::~MailFolderCC()
{
   if ( m_MailStream )
      mail_close(m_MailStream);
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

   if(! mail_append(NIL,(char *)m_MailboxPath.c_str(),&str))
      ERRORMESSAGE(("cannot append message"));
}

void
MailFolderCC::AppendMessage(Message const &msg)
{
   String tmp;

   msg.WriteString(tmp);
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
   case MailFolder::MF_INBOX:
      symbolicName = "INBOX"; break;
   case MailFolder::MF_FILE:
      symbolicName = m_MailboxPath; break;
   case MailFolder::MF_POP:
      symbolicName << "pop_" << m_Login; break;
   case MailFolder::MF_IMAP:
      symbolicName << "imap_" << m_Login; break;
   case MailFolder::MF_NNTP:
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
   return numOfMessages;
}

int
MailFolderCC::GetMessageStatus(unsigned int msgno,
                unsigned long *size,
                unsigned int *day,
                unsigned int *month,
                unsigned int *year)
{
   int status;
   MESSAGECACHE *mc = mail_elt(m_MailStream, msgno);

   if(size)    *size = mc->rfc822_size;
   if(day)  *day = mc->day;
   if(month)   *month = mc->month;
   if(year) *year = mc->year + BASEYEAR;
   status = 0;
   if(!mc->seen)     status += MSG_STAT_UNREAD;
   if(mc->answered)  status += MSG_STAT_REPLIED;
   if(mc->deleted)   status += MSG_STAT_DELETED;
   if(mc->searched)  status += MSG_STAT_SEARCHED;
   if(mc->recent)    status += MSG_STAT_RECENT;
   return status;
}

Message *
MailFolderCC::GetMessage(unsigned long index)
{
   return new MessageCC(this,index,mail_uid(m_MailStream,index));
}


void
MailFolderCC::SetMessageFlag(unsigned long index, int flag, bool set)
{
   String
      seq;

   seq << index;

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

   const char *callback = set ? MCB_FOLDERSETMSGFLAG : MCB_FOLDERCLEARMSGFLAG;

   if(PY_CALLBACKVA((callback, 1, this, this->GetClassName(),
                     GetProfile(), "ls", (signed long) index, flagstr),1)  )
   {
      if(set)
         mail_setflag(m_MailStream, (char *)seq.c_str(), (char *)flagstr);
      else
         mail_clearflag(m_MailStream, (char *)seq.c_str(), (char *)flagstr);
   }
}

void
MailFolderCC::ExpungeMessages(void)
{
   if(PY_CALLBACK(MCB_FOLDEREXPUNGE,1,GetProfile()))
      mail_expunge (m_MailStream);
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
#endif

/// remove this object from Map
void
MailFolderCC::RemoveFromMap(MAILSTREAM const *stream)
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
MailFolderCC::LookupObject(MAILSTREAM const *stream)
{
   StreamConnectionList::iterator i;
   for(i = streamList.begin(); i != streamList.end(); i++)
      if( (*i)->stream == stream )
    return (*i)->folder;
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
MailFolderCC::mm_searched(MAILSTREAM *stream, unsigned long number)
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
#if DEBUG
      String
    tmp = "MailFolderCC::mm_exists() for folder " + mf->m_MailboxPath
    + String(" n: ") + strutil_ultoa(number);
      LOGMESSAGE((M_LOG_DEBUG, Str(tmp)));
#endif

      mf->numOfMessages = number;
      mf->UpdateViews();
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
      mf->numOfMessages--;
      mf->UpdateViews();
   }
}

/// flags have changed for a message
void
MailFolderCC::mm_flags(MAILSTREAM *stream, unsigned long number)
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
MailFolderCC::mm_notify(MAILSTREAM *stream, char *str, long
             errflg)
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
MailFolderCC::mm_list(MAILSTREAM *stream, char delim, char *name,
           long attrib)
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
MailFolderCC::mm_lsub(MAILSTREAM *stream, char delim, char *name,
           long attrib)
{
   //FIXME
}

/** status of mailbox has changed
    @param stream mailstream
    @param mailbox   mailbox name for this status
    @param status structure with new mailbox status
*/
void
MailFolderCC::mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS
             *status)
{
   MailFolderCC *mf = LookupObject(stream);
   if(mf == NULL)
      return;  // oops?!

#if DEBUG
   String   tmp = "MailFolderCC::mm_status() for folder " + mf->m_MailboxPath;
   LOGMESSAGE((M_LOG_DEBUG, Str(tmp)));
#endif

   if(status->flags & SA_MESSAGES)
      mf->numOfMessages  = status->messages;
   mf->UpdateViews();
}


/** log a message
    @param str message str
    @param errflg error level
*/
void
MailFolderCC::mm_log(const char *str, long errflg)
{
   if(mm_ignore_errors)
      return;
   String   msg = (String) "c-client " + (String) str;
   LOGMESSAGE((M_LOG_INFO, Str(msg)));
}

/** log a debugging message
    @param str    message str
*/
void
MailFolderCC::mm_dlog(const char *str)
{
   String   msg = (String) "c-client debug: " + (String) str;
   //mApplication.Message(msg.c_str());
   LOGMESSAGE((M_LOG_DEBUG, Str(msg)));
}

/** get user name and password
       @param  mb parsed mailbox specification
       @param  user    pointer where to return username
       @param  pwd   pointer where to return password
       @param  trial number of prior login attempts
       */
void
MailFolderCC::mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
   strcpy(user,MF_user.c_str());
   strcpy(pwd,MF_pwd.c_str());
}

/** alert that c-client will run critical code
       @param  stream   mailstream
   */
void
MailFolderCC::mm_critical(MAILSTREAM *stream)
{
   // ignore
}

/**   no longer running critical code
   @param   stream mailstream
     */
void
MailFolderCC::mm_nocritical(MAILSTREAM *stream)
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
MailFolderCC::mm_diskerror(MAILSTREAM *stream, long errcode, long serious)
{
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
      mf->okFlag = false;  // signal error
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

#if 0
//-------------------------------------------------------------------

MailFolderPopCC::MailFolderPopCC(String const &name)
   : MailFolderCC(name)
{
   SetType(MFolder::POP);
   Open();
}

void
MailFolderPopCC::Create(String const &name)
{
   MailFolderCC::Create(name);
   SetType(MFolder::POP);
   Open();
}

bool
MailFolderPopCC::Open(void)
{
   String mboxname = "{";
   mboxname += READ_CONFIG(GetProfile(), MP_POP_HOST);
   mboxname +="/pop3}";

   return MailFolderCC::Open(mboxname.c_str());
}
#endif


// the callbacks:
extern "C"
{

void
mm_searched(MAILSTREAM *stream, unsigned long number)
{
   MailFolderCC::mm_searched(stream,  number);
}

void
mm_exists(MAILSTREAM *stream, unsigned long number)
{
   MailFolderCC::mm_exists(stream, number);
}

void
mm_expunged(MAILSTREAM *stream, unsigned long number)
{
   MailFolderCC::mm_expunged(stream,  number);
}

void
mm_flags(MAILSTREAM *stream, unsigned long number)
{
   MailFolderCC::mm_flags(stream,  number);
}

void
mm_notify(MAILSTREAM *stream, char *str, long
             errflg)
{
   MailFolderCC::mm_notify(stream, str,  errflg);
}

void
mm_list(MAILSTREAM *stream, int delim, char *name, long attrib)
{
   MailFolderCC::mm_list(stream, delim,name, attrib);
}

void
mm_lsub(MAILSTREAM *stream, int delim, char *name, long attrib)
{
   MailFolderCC::mm_lsub(stream, delim, name, attrib);
}

void
mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS
             *status)
{
   MailFolderCC::mm_status(stream, mailbox, status);
}

void
mm_log(char *str, long errflg)
{
   String   tmp = String(_("log: ")) + str;
   MailFolderCC::mm_log(tmp.c_str(),errflg);
}

void
mm_dlog(char *str)
{
   String   tmp = String(_("debug: ")) + str;
   MailFolderCC::mm_dlog(tmp.c_str());
}

void
mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
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

} // extern "C"

//#define TESTCLASS

#ifdef   TESTCLASS

int main(void)
{
   // testing the MailFolderCC class:

   MailFolderCC   mf("DemoBox");
   if(! mf.Open("/home/karsten/src/Projects/M/test/testbox"))
      cerr << "Open() returned error." << endl;

   mf.DoDebug();  // enable debugging

   if(mf.IsOk())
      cerr << "Object initialised correctly." << endl;
   else
      cerr << "Object initialisation failed." << endl;
   mf.Debug();    // show debug info
   cout << "----------------------------------------------------------" << endl;

   MailFolder &m = mf;

   cout
      << "Name: " << m.GetName()
      << "# of Messages: " << m.CountMessages()
      << endl;

#if USE_CLASSINFO
   cout << "m is of class " << m.GetClassName() << endl;
#endif

   int  i;
   Message  *msg;
   for(i = 1; i <= m.CountMessages(); i++)
   {
      msg = m[i];
      cout << i << '\t' << msg->From() << msg->Subject() << endl;
      delete msg;
   }

   return 0;
}

#endif

