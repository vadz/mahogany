/*-*- c++ -*-********************************************************
 * MailFolderCC class: handling of mail folders with c-client lib   *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.13  1998/06/22 22:42:34  VZ
 * kbList/CHECK/PY_CALLBACK small changes
 *
 * Revision 1.12  1998/06/14 21:33:54  KB
 * fixed the menu/callback problem, wxFolderView is now a panel
 *
 * Revision 1.11  1998/06/14 12:24:26  KB
 * started to move wxFolderView to be a panel, Python improvements
 *
 * Revision 1.10  1998/06/05 16:56:32  VZ
 *
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.9  1998/05/30 17:52:43  KB
 * addes some more classes to python interface
 *
 * Revision 1.8  1998/05/24 14:48:33  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.7  1998/05/24 08:23:30  KB
 * changed the creation/destruction of MailFolders, now done through
 * MailFolder::Open/CloseFolder, made constructor/destructor private,
 * this allows multiple view on the same folder
 *
 * Revision 1.6  1998/05/18 17:48:48  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.5  1998/05/15 22:00:46  VZ
 *
 * TEXT_DATA_CAST macro
 *
 * Revision 1.4  1998/05/11 20:57:36  VZ
 * compiles again under Windows + new compile option USE_WXCONFIG
 *
 * Revision 1.3  1998/04/22 19:57:00  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.2  1998/03/26 23:05:43  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:27  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "MailFolderCC.h"
#endif

#include  "Mpch.h"

#ifndef  USE_PCH
#  include <strutil.h>
#  include <MApplication.h>
#  include <MailFolderCC.h>
#  include <MessageCC.h>
#  include <MDialogs.h>

// includes for c-client library
extern "C"
{
#  include <osdep.h>
#  include <rfc822.h>
#  include <smtp.h>
#  include <nntp.h>
}

#endif // USE_PCH

#include  "Mdefaults.h"
#include   "MPython.h"

#include  "FolderView.h"
#include  "MailFolder.h"
#include  "MailFolderCC.h"
#include  "Message.h"
#include  "MessageCC.h"

String MailFolderCC::MF_user;
String MailFolderCC::MF_pwd;

//IMPLEMENT_CLASS2(MailFolderCC, MailFolder, CommonBase)

bool
MailFolderCC::Open(String const & filename)
{
   realName = filename;
   if(GetType() == MF_POP || GetType() == MF_IMAP)
   {
      MF_user = READ_CONFIG(profile, MP_POP_LOGIN);
      MF_pwd = READ_CONFIG(profile, MP_POP_PASSWORD);
   }

   // this will fail if file already exists, but it makes sure we can open it
   if(GetType() == MF_FILE)      
      mail_create(NIL, (char *)filename.c_str());
   
   SetDefaultObj();
   mailstream = mail_open(mailstream,(char *)realName.c_str(),
           debugFlag ? OP_DEBUG : NIL);
   SetDefaultObj(false);
   if(mailstream == NIL)
      return false;  // failed

   AddToMap(mailstream);

   mail_status(mailstream, (char *)realName.c_str(), SA_MESSAGES|SA_RECENT|SA_UNSEEN);
   String sequence = String("1:") + strutil_ultoa(numOfMessages);
   mail_fetchfast(mailstream, (char *)sequence.c_str());
   
   okFlag = true;
   if(okFlag)
      PY_CALLBACK(MCB_FOLDEROPEN, profile,0);
   return true;   // success
}

MailFolderCC *
MailFolderCC::OpenFolder(String const &name)
{
   StreamConnectionList::iterator i;
   for(i = streamList.begin(); i != streamList.end(); i++)
      if( (*i)->name == name )
      {
         (*i)->refcount++;
         return (*i)->folder;
      }
   // not found:
   return new MailFolderCC(name);
}

void
MailFolderCC::Close(void)
{
   StreamConnectionList::iterator i;
   for(i = streamList.begin(); i != streamList.end(); i++)
      if( (*i)->folder == this )
      {
         (*i)->refcount--;
         if((*i)->refcount == 0)
         {
            streamList.erase(i);
            delete this;
         }
      }
   //FIXME error!
}

void
MailFolderCC::Create(String const & iname)
{
   okFlag = false;
   mailstream = NIL;
   symbolicName = iname;

   profile = new Profile(iname, NULL);

   // make sure we can use the library
   if(! cclientInitialisedFlag)
      CClientInit();

   if(iname == String("INBOX"))
   {
      SetType(MF_INBOX);
      Open("INBOX");
   }
   else
   {
      const char *filename = READ_CONFIG(profile, MP_FOLDER_PATH);
      if(filename == NULL) // assume we are a file
      {
         SetType(MF_FILE);
         Open(iname);
      }
      else
      {
         SetType((FolderType)READ_CONFIG(profile, MP_FOLDER_TYPE));
         Open(filename);
      }
   }
}

void
MailFolderCC::Ping(void)
{
#if DEBUG
   String tmp = "MailFolderCC::Ping() on Folder " + realName;
   LOGMESSAGE((M_LOG_DEBUG, tmp));
#endif
   mail_ping(mailstream);
}


void
MailFolderCC::AppendMessage(const char *msg)
{
   STRING str;

   INIT(&str, mail_string, (void *) msg, strlen(msg));
   //mail_string_init(&str, (char *) msg, strlen(msg));
   mail_append(mailstream, (char *)realName.c_str(), &str);

}

MailFolderCC::MailFolderCC(String const & iname)
{
   Create(iname);
}

MailFolderCC::~MailFolderCC()
{
   mail_close(mailstream);
   RemoveFromMap(mailstream);
}

void
MailFolderCC::RegisterView(FolderViewBase *view, bool reg)
{
   FolderViewList::iterator
      i;
   if(reg)
      viewList.push_front(view);
   else  
      for(i = viewList.begin(); i != viewList.end(); i++)
      {
         if((*i) == view)
         {
            viewList.erase(i);
            return;
         }
      }
}


void
MailFolderCC::UpdateViews(void)
{
   FolderViewList::iterator
      i;
   for(i = viewList.begin(); i != viewList.end(); i++)
      (*i)->Update();
   PY_CALLBACK(MCB_FOLDERUPDATE, profile,0);
}

const String &
MailFolderCC::GetName(void) const
{
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
   MESSAGECACHE *mc = mail_elt(mailstream, msgno);

   if(size)    *size = mc->rfc822_size;
   if(day)  *day = mc->day;
   if(month)   *month = mc->month;
   if(year) *year = mc->year + BASEYEAR;
   status = 0;
   if(!mc->seen)  status += MSG_STAT_UNREAD;
   if(mc->answered)  status += MSG_STAT_REPLIED;
   if(mc->deleted)   status += MSG_STAT_DELETED;
   if(mc->searched)  status += MSG_STAT_SEARCHED;
   if(mc->recent) status += MSG_STAT_RECENT;
   return status;
}

Message *
MailFolderCC::GetMessage(unsigned long index)
{
   return new MessageCC(this,index,mail_uid(mailstream,index));
}


void
MailFolderCC::DeleteMessage(unsigned long index)
{
   String
      seq = strutil_ultoa(index);
   if(PY_CALLBACKVA((MCB_FOLDERDELMSG, this, this->GetClassName(), profile, "l", (signed long) index),1)  )
      mail_setflag(mailstream, (char *)seq.c_str(), "\\Deleted");
}

void
MailFolderCC::ExpungeMessages(void)
{
   if(PY_CALLBACK(MCB_FOLDEREXPUNGE,profile,1))
      mail_expunge (mailstream);
}

   
#ifndef NDEBUG
void
MailFolderCC::Debug(void) const
{
   char buffer[1024];

   CBDEBUG();
   VAR(numOfMessages);
   VAR(realName);
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
      if( (*i)->stream == stream )
      {
    streamList.erase(i);
    break;
      }
   if(streamListDefaultObj == this)
      SetDefaultObj(false);
}


//-------------------------------------------------------------------
// static class member functions:
//-------------------------------------------------------------------

StreamConnectionList MailFolderCC::streamList;

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
   conn->name = symbolicName;
   conn->refcount = 1;
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
    tmp = "MailFolderCC::mm_exists() for folder " + mf->realName
    + String(" n: ") + strutil_ultoa(number);
      LOGMESSAGE((M_LOG_DEBUG, tmp));
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
   LOGMESSAGE((M_LOG_DEFAULT, msg));
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
   String msg = "Flags changed for msg no. " + strutil_ltoa(number);
   LOGMESSAGE((M_LOG_DEFAULT, msg));
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
   String   tmp = "MailFolderCC::mm_status() for folder " +
      mf->realName;
   LOGMESSAGE((M_LOG_DEBUG, tmp));
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
   String   msg = (String) "c-client " + (String) str;
   LOGMESSAGE((M_LOG_INFO, msg));
}

/** log a debugging message
    @param str    message str
*/
void
MailFolderCC::mm_dlog(const char *str)
{
   String   msg = (String) "c-client debug: " + (String) str;
   //mApplication.Message(msg.c_str());
   LOGMESSAGE((M_LOG_DEBUG, msg));
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
   LOGMESSAGE((M_LOG_ERROR, msg));
}

//-------------------------------------------------------------------

MailFolderPopCC::MailFolderPopCC(String const &name)
   : MailFolderCC(name)
{
   SetType(MF_POP);
   Open();
}

void
MailFolderPopCC::Create(String const &name)
{
   MailFolderCC::Create(name);
   SetType(MF_POP);
   Open();
}

bool
MailFolderPopCC::Open(void)
{
   String mboxname = "{";
   mboxname += READ_CONFIG(profile, MP_POP_HOST);
   mboxname +="/pop3}";
   
   return MailFolderCC::Open(mboxname.c_str());
}


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

