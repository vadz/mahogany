/*-*- c++ -*-********************************************************
 * MailFolderCC class: handling of mail folders with c-client lib   *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#   pragma implementation "MailFolderCC.h"
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
#include "ASMailFolder.h"

// just to use wxFindFirstFile()/wxFindNextFile() for lockfile checking
#include <wx/filefn.h>

// including mh.h doesn't seem to work...
extern "C"
{
   int mh_isvalid(char *name, char *tmp, long synonly);

   char *mh_getpath(void);
}

#include <ctype.h>   // isspace()

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#define CHECK_DEAD(msg)   if(m_MailStream == NIL && ! PingReopen()) { ERRORMESSAGE((_(msg), GetName().c_str())); return; }
#define CHECK_DEAD_RC(msg, rc)   if(m_MailStream == NIL && ! PingReopen()) {   ERRORMESSAGE((_(msg), GetName().c_str())); return rc; }

#define ISO8859MARKER "=?" // "=?iso-8859-1?Q?"
#define QPRINT_MIDDLEMARKER_U "?Q?"
#define QPRINT_MIDDLEMARKER_L "?q?"

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

typedef int (*overview_x_t) (MAILSTREAM *stream,unsigned long uid,OVERVIEW *ov);



/** This is a list of all mailstreams which were left open because the
    dialup connection broke before we could close them.
    We remember them and try to close them at program exit. */
KBLIST_DEFINE(StreamList, MAILSTREAM);

class CCStreamCleaner
{
public:
   void Add(MAILSTREAM *stream) { if(stream) m_List.push_back(stream); }
   ~CCStreamCleaner();
private:
   StreamList  m_List;
};

CCStreamCleaner::~CCStreamCleaner()
{
#ifdef DEBUG
      LOGMESSAGE((M_LOG_DEBUG, "CCStreamCleaner: checking for left-over streams"));
#endif

   if(! mApplication->IsOnline())
   {
      // brutally free all memory without closing stream:
      MAILSTREAM *stream;
      StreamList::iterator i;
      for(i = m_List.begin(); i != m_List.end(); i++)
      {
#ifdef DEBUG
         LOGMESSAGE((M_LOG_DEBUG, "CCStreamCleaner: freeing stream offline"));
#endif
         stream = *i;
         // copied from c-client mail.c:
         if (stream->mailbox) fs_give ((void **) &stream->mailbox);
         stream->sequence++;		/* invalidate sequence */
				        /* flush user flags */
         for (int n = 0; n < NUSERFLAGS; n++)
            if (stream->user_flags[n]) fs_give ((void **) &stream->user_flags[n]);
         mail_free_cache (stream);	/* finally free the stream's storage */
         /*if (!stream->use)*/ fs_give ((void **) &stream);
      }
   }
   else
   {
      // we are online, so we can close it properly:
      StreamList::iterator i;
      for(i = m_List.begin(); i != m_List.end(); i++)
      {
#ifdef DEBUG
         LOGMESSAGE((M_LOG_DEBUG, "CCStreamCleaner: closing stream"));
#endif
         mail_close(*i);
      }
   }
}

/// exactly one object
static CCStreamCleaner *gs_CCStreamCleaner = NULL;



extern void CC_Cleanup(void)
{
   // as c-client lib doesn't seem to think that deallocating memory is
   // something good to do, do it at it's place...
   free(mail_parameters((MAILSTREAM *)NULL, GET_HOMEDIR, NULL));
   free(mail_parameters((MAILSTREAM *)NULL, GET_NEWSRC, NULL));

   if(gs_CCStreamCleaner)
   {
      delete gs_CCStreamCleaner;
      gs_CCStreamCleaner = NULL;
   }
}

// ============================================================================
// implementation
// ============================================================================


/*
 * Small function to translate c-client status flags into ours.
 */
static
MailFolder::MessageStatus
GetMsgStatus(MESSAGECACHE *elt)
{
   ASSERT(elt);
   int status = MailFolder::MSG_STAT_NONE;
   if(elt->recent)   status |= MailFolder::MSG_STAT_RECENT;
   if(elt->seen)     status |= MailFolder::MSG_STAT_SEEN;
   if(elt->flagged)  status |= MailFolder::MSG_STAT_FLAGGED;
   if(elt->answered) status |= MailFolder::MSG_STAT_ANSWERED;
   if(elt->deleted)  status |= MailFolder::MSG_STAT_DELETED;
   return (MailFolder::MessageStatus) status;
}

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
   // try upper and lowercase versions of middlemarker: "?q?"
   int pos2 = in.Find(QPRINT_MIDDLEMARKER_U);
   if(pos2 == -1) pos2 = in.Find(QPRINT_MIDDLEMARKER_L);
   if(pos2 == -1 || pos2 < pos)
      return in;

   String quoted;
   const char *cptr = in.c_str() + pos2 + strlen(QPRINT_MIDDLEMARKER_U);
   while(*cptr && !(*cptr == '?' && *(cptr+1) == '='))
      quoted << (char) *cptr++;
   if(*cptr)
   {
      cptr += 2; // "?="
      unsigned long unused_len;
      char *cptr2 = (char *)rfc822_qprint((unsigned char *)quoted.c_str(), quoted.length(),
                                          &unused_len);
      out +=  cptr2;
      fs_give((void **) &cptr2); // free memory allocated
   }
   out +=  cptr;

   // Check whether we need to decode even more:
   if(in.Find(ISO8859MARKER) != -1)
      return qprint(out);
   else
      return out;
}


class FolderListingEntryCC : public FolderListingEntry
{
public:
   /// The folder's name.
   virtual const String &GetName(void) const
      { return m_Name;}
   /// The folder's attribute.
   virtual long GetAttribute(void) const
      { return m_Attr; }
   FolderListingEntryCC(const String &name, long attr)
      : m_Name(name)
      { m_Attr = attr; }
private:
   const String &m_Name;
   long          m_Attr;
};


/** This class holds the listings of a server's folders. */
class FolderListingCC : public FolderListing
{
public:
   /// Return the delimiter character for entries in the hierarchy.
   virtual char GetDelimiter(void) const
      { return m_Del; }
   /// Returns the number of entries.
   virtual size_t CountEntries(void) const
      { return m_list.size(); }
   /// Returns the first entry.
   virtual const FolderListingEntry * GetFirstEntry(iterator &i) const
      {
         i = m_list.begin();
         if ( i != m_list.end() )
            return (*i);
         else
            return NULL;
      }
   /// Returns the next entry.
   virtual const FolderListingEntry * GetNextEntry(iterator &i) const
      {
         i++;
         if ( i != m_list.end())
            return (*i);
         else
            return NULL;
      }
   /** For our use only. */
   void SetDelimiter(char del)
      { m_Del = del; }
   void Add( FolderListingEntry *entry )
      {
         m_list.push_back(entry);
      }
private:
   char m_Del;
   FolderListingList m_list;
};


/** Builds an folder specification */
static
String BuildFolderSpec(const String &host,
                       FolderType protocol,
                       const String &mailbox = "")
{
   String spec;

   switch ( protocol )
   {
      case MF_INBOX:
         spec = "INBOX";
         break;

      case MF_IMAP:
         spec << '{' << host << "/imap}" << mailbox;
         break;

      case MF_NNTP:
         spec << '{' << host << "/nntp}" << mailbox;
         break;

      case MF_NEWS:
         spec << "#news." << mailbox;
         break;

      case MF_POP:
         spec << '{' << host << "/pop}" << mailbox;
         break;

      case MF_MH:
         spec << "#mh/" << mailbox;
         break;

      default:
         FAIL_MSG("unsupported protocol for folder listing");
   }

   return spec;
}




/*----------------------------------------------------------------------------------------
 * MailFolderCC code
 *---------------------------------------------------------------------------------------*/
String MailFolderCC::MF_user;
String MailFolderCC::MF_pwd;

// a variable telling c-client to shut up
static bool mm_ignore_errors = false;
/// a variable disabling all events
static bool mm_disable_callbacks = false;

// be quiet
static inline void CCQuiet(bool disableCallbacks = false)
{
   MCclientLocker lock;
   mm_ignore_errors = true;
   if(disableCallbacks) mm_disable_callbacks = true;
}
// normal logging
static inline void CCVerbose(void)
{
   MCclientLocker lock;
   mm_ignore_errors = mm_disable_callbacks = false;
}

// loglevel for cclient error messages:
static int cc_loglevel = wxLOG_Error;

static int CC_GetLogLevel(void) { return cc_loglevel; }
static int CC_SetLogLevel(int arg)
{
   MCclientLocker lock;
   int ccl = cc_loglevel;
   cc_loglevel =  arg;
   return ccl;
}


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
   virtual time_t GetDate(void) const { return m_Date; }
   virtual String const &GetId(void) const { return m_Id; }
   virtual unsigned long GetUId(void) const { return m_Uid; }
   virtual String const &GetReferences(void) const { return m_References; }
   virtual int GetStatus(void) const { return m_Status; }
   virtual unsigned long const &GetSize(void) const { return m_Size; }
   virtual size_t SizeOf(void) const { return sizeof(HeaderInfoCC); }
   /// Return the indentation level for message threading.
   virtual unsigned GetIndentation() const { return m_Indentation; }
   /// Set the indentation level for message threading.
   virtual void SetIndentation(unsigned level) { m_Indentation = level; }
   /// Assignment operator.
   HeaderInfo & operator= (const HeaderInfo &);

   HeaderInfoCC();
protected:
   String m_Subject, m_From, m_Id, m_References;
   int m_Status;
   unsigned long m_Size;
   unsigned long m_Uid;
   time_t m_Date;
   unsigned m_Indentation;
   friend class MailFolderCC;
};

HeaderInfoCC::HeaderInfoCC()
{
   m_Indentation = 0;
   // all other fields are filled in by the MailFolderCC when creating
   // it
}

HeaderInfo &
HeaderInfoCC::operator= ( const HeaderInfo &old)
{
   m_Subject = old.GetSubject();
   m_From = old.GetFrom();
   m_Date = old.GetDate();
   m_Id = old.GetUId();
   m_References = old.GetReferences();
   m_Status = old.GetStatus();
   m_Size = old.GetSize();
   SetIndentation(old.GetIndentation());
   return *this;
}

/** This class holds a complete list of all messages in the folder. */
class HeaderInfoListCC : public HeaderInfoList
{
public:
   ///@name Interface
   //@{
   /// Count the number of messages in listing.
   virtual size_t Count(void) const
      { return m_NumEntries; }
   /// Returns the n-th entry.
   virtual const HeaderInfo * operator[](size_t n) const
      {
         // simply call non-const operator:
         return (*(HeaderInfoListCC *)this)[n];
      }
   //@}
   ///@name Implementation
   //@{
   /// Returns the n-th entry.
   virtual HeaderInfo * operator[](size_t n)
      {
         MOcheck();
         ASSERT(n < m_NumEntries);
         if(n >= m_NumEntries)
            return NULL;
         else
         {
            if(m_TranslationTable)
               return & m_Listing[ m_TranslationTable[n] ];
            else
               return & m_Listing[n];
         }
         return & m_Listing[n];
      }
   /// Returns pointer to array of data:
   virtual HeaderInfo *GetArray(void) { MOcheck(); return m_Listing; }

   /// For use by folder only: corrects size downwards:
   void SetCount(size_t newcount)
      { MOcheck(); ASSERT(newcount <= m_NumEntries);
      m_NumEntries = newcount; }
   //@}

   /// Returns an empty list of same size.
   virtual HeaderInfoList *DuplicateEmpty(void) const
      {
         return Create(m_NumEntries);
      }

   static HeaderInfoListCC * Create(size_t n)
      { return new HeaderInfoListCC(n); }
   /// Swaps two elements:
   virtual void Swap(size_t index1, size_t index2)
      {
         MOcheck();
         ASSERT(index1 < m_NumEntries);
         ASSERT(index2 < m_NumEntries);
         HeaderInfoCC hicc;
         hicc = m_Listing[index1];
         m_Listing[index1] = m_Listing[index2];
         m_Listing[index2] = hicc;
      }
   /** Sets a translation table re-mapping index values.
       Will be freed in destructor.
       @param array an array of indices or NULL to remove it.
   */
   virtual void SetTranslationTable(size_t array[] = NULL)
      {
         if(m_TranslationTable)
            delete [] m_TranslationTable;
         m_TranslationTable = array;
      }

protected:
   HeaderInfoListCC(size_t n)
      {
         m_Listing = n == 0 ? NULL : new HeaderInfoCC[n];
         m_NumEntries = n;
         m_TranslationTable = NULL;
      }
   ~HeaderInfoListCC()
      {
         MOcheck();
         if ( m_Listing ) delete [] m_Listing;
         if ( m_TranslationTable ) delete [] m_TranslationTable;
      }
   /// The current listing of the folder
   class HeaderInfoCC *m_Listing;
   /// number of entries
   size_t              m_NumEntries;
   /// translation of indices
   size_t *m_TranslationTable;

   MOBJECT_DEBUG(HeaderInfoListCC)
};


MailFolderCC::MailFolderCC(int typeAndFlags,
                           String const &path,
                           ProfileBase *profile,
                           String const &server,
                           String const &login,
                           String const &password)
   : MailFolderCmn(profile)
{
   if(! cclientInitialisedFlag)
      CClientInit();
   Create(typeAndFlags);
   if(GetType() == MF_FILE)
      m_MailboxPath = strutil_expandpath(path);
   else
         m_MailboxPath = path;
   m_Login = login;
   m_Password = password;
}

void
MailFolderCC::Create(int typeAndFlags)
{
   m_MailStream = NIL;

#ifdef DEBUG
   debugFlag = true;
#else // !DEBUG
   debugFlag = false;
#endif

   m_InCritical = false;

#define SET_TO(setting, var) var = READ_APPCONFIG(MP_TCP_##setting)

   SET_TO(OPENTIMEOUT, ms_TcpOpenTimeout);
   SET_TO(READTIMEOUT, ms_TcpReadTimeout);
   SET_TO(WRITETIMEOUT, ms_TcpWriteTimeout);
   SET_TO(CLOSETIMEOUT, ms_TcpCloseTimeout);
   SET_TO(RSHTIMEOUT, ms_TcpRshTimeout);

#undef SET_TO

   SetRetrievalLimit(0); // no limit
   m_NumOfMessages = 0;
   m_OldNumOfMessages = 0;
   m_Listing = NULL;
   m_FirstListing = true;
   m_UpdateNeeded = true;
   m_PingReopenSemaphore = false;
   m_BuildListingSemaphore = false;
   m_FolderListing = NULL;
   m_InCritical = false;
   m_ASMailFolder = NULL;
   FolderType type = GetFolderType(typeAndFlags);
   m_FolderFlags = GetFolderFlags(typeAndFlags);
   m_SearchMessagesFound = NULL;
   SetType(type);

   if( !FolderTypeHasUserName(type) )
      m_Login = ""; // empty login for these types
#ifdef USE_THREADS
   m_Mutex = new MMutex;
#else
   m_Mutex = false;
#endif
}

MailFolderCC::~MailFolderCC()
{
   PreClose();

   CCQuiet(true); // disable all callbacks!
   Close();

   ASSERT(m_SearchMessagesFound == NULL);

#ifdef USE_THREADS
   delete m_Mutex;
#endif

   if( m_Listing )
   {
      m_Listing->DecRef();
      m_Listing = NULL;
   }
   // note that RemoveFromMap already removed our node from streamList, so
   // do not do it here again!
}

/*
  This gets called with a folder path as its name, NOT with a symbolic
  folder/profile name.
*/
/* static */
MailFolderCC *
MailFolderCC::OpenFolder(int typeAndFlags,
                         String const &name,
                         ProfileBase *profile,
                         String const &server,
                         String const &login,
                         String const &password,
                         String const &symname)
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
      {
         if(login.Length())
            mboxpath << '{' << server << "/user=" << login << '}'<<
               name;
         else // we get asked later FIXME!!
            mboxpath << '{' << server << '}'<< name;
      }
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

   //FIXME: This should somehow be done in MailFolder.cc
   mf = FindFolder(mboxpath,login);
   if(mf)
   {
      mf->IncRef();
      mf->Ping(); // make sure it's updated
//      mf->SetName(symname);
      return mf;
   }


   String pword = password;
   // ask the password for the folders which need it but for which it hadn't been
   // specified during creation
   if ( FolderTypeHasUserName( (FolderType) GetFolderType(typeAndFlags))
        && (GetFolderFlags(typeAndFlags) & MF_FLAGS_ANON == 0)
        && pword.Length() == 0)
   {
      String prompt, fname;
      fname = name;
      if(! fname) fname = mboxpath;
      prompt.Printf(_("Please enter the password for folder '%s':"), fname.c_str());
      if(! MInputBox(&pword,
                     _("Password required"),
                     prompt, NULL,
                     NULL,NULL, true))
      {
         ERRORMESSAGE((_("Cannot access this folder without a password.")));

         mApplication->SetLastError(M_ERROR_AUTH);

         return NULL; // cannot open it
      }
   }
   mf = new
      MailFolderCC(typeAndFlags,mboxpath,profile,server,login,pword);
   mf->SetName(symname);
   if(mf && profile)
      mf->SetRetrievalLimit(READ_CONFIG(profile, MP_MAX_HEADERS_NUM));

   bool ok = TRUE;
   if ( mf->NeedsNetwork() && !mApplication->IsOnline() )
   {
      String msg;
      msg.Printf(_("To open the folder '%s', network access is required "
                   "but it is currently not available.\n"
                   "Would you like to connect to the network now?"),
                 mf->GetName().c_str());

      if ( MDialog_YesNoDialog(msg, NULL, MDIALOG_YESNOTITLE,
                               TRUE /* [Yes] default */,
                               mf->GetName()+"/DialUpOnOpenFolder"))
      {
         mApplication->GoOnline();
      }

      if ( !mApplication->IsOnline() ) // still not?
      {
         if ( !MDialog_YesNoDialog(_("Opening this folder will probably fail!\n"
                                     "Continue anyway?"),
                                   NULL, MDIALOG_YESNOTITLE,
                                   FALSE /* [No] default */,
                                   mf->GetName()+"/NetDownOpenAnyway") )
         {
             ok = FALSE;

             // remember that the user cancelled opening the folder
             mApplication->SetLastError(M_ERROR_CANCEL);
         }
      }
   }

   // try to really open it
   if ( ok )
   {
      ok = mf->Open();

      if ( !ok )
      {
         mApplication->SetLastError(M_ERROR_CCLIENT);
      }
   }

   if ( !ok )
   {
      mf->DecRef();
      mf = NULL;
   }

   return mf;
}



#define UPDATE_TO(name, var)    to = READ_CONFIG(p,MP_TCP_##name); \
                                if (to != var) \
                                { \
                                    var = to; \
                                    (void) mail_parameters(NIL, \
                                                           SET_##name, (void *) to); \
                                }

void
MailFolderCC::UpdateTimeoutValues(void)
{
   ProfileBase *p = GetProfile();
   int to;
   UPDATE_TO(OPENTIMEOUT, ms_TcpOpenTimeout);
   UPDATE_TO(READTIMEOUT, ms_TcpReadTimeout);
   UPDATE_TO(WRITETIMEOUT, ms_TcpWriteTimeout);
   UPDATE_TO(CLOSETIMEOUT, ms_TcpCloseTimeout);
   UPDATE_TO(RSHTIMEOUT, ms_TcpRshTimeout);
}

bool
MailFolderCC::Open(void)
{
   STATUSMESSAGE((_("Opening mailbox %s..."), GetName().c_str()));


   /** Now, we apply the very latest c-client timeout values, in case
       they have changed.
   */
   UpdateTimeoutValues();

   if(GetType() == MF_POP || GetType() == MF_IMAP)
   {
      MF_user = m_Login;
      MF_pwd = m_Password;
   }

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
            {
               MCclientLocker lock;
               lockfile = (char *) mail_parameters (NIL,GET_SYSINBOX,NULL);
               if(lockfile.IsEmpty()) // another c-client stupidity
                  lockfile = (char *) sysinbox();
               ProcessEventQueue();
            }
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
               STATUSMESSAGE((_("Could not open mailbox %s."), GetName().c_str()));
               return false;
            }
         }
         lockfile = wxFindNextFile();
      }
   }

#ifndef DEBUG
   CCQuiet(); // first try, don't log errors (except in debug mode)
#endif // DEBUG

   {
      MCclientLocker lock;
      SetDefaultObj();

      // create the mailbox if it doesn't exist yet
      bool exists = TRUE;
      if ( GetType() == MF_FILE )
      {
         exists = wxFileExists(m_MailboxPath);
      }
      else if ( GetType() == MF_MH )
      {
         // the real path for MH mailboxes is not just the mailbox name
         String path;
         path << InitializeMH() << m_MailboxPath.c_str() + 4; // strlen("#mh/")
         exists = wxFileExists(path);
      }

      /* Clever hack: if the folder is IMAP and the folder name ends
         in a slash '/' we only half-open it. The server does not like
         opening directories, but the half-open stream can still be
         used for retrieving subfolder listings. */
      if(GetType() == MF_IMAP
         && m_MailboxPath[m_MailboxPath.Length()-1] == '/')
      {
         String spec = m_MailboxPath.BeforeLast('}') + '}';
         CCVerbose();
         m_MailStream = mail_open(NIL,(char *)spec.c_str(),
                                  (debugFlag ? OP_DEBUG : NIL)|OP_HALFOPEN);
         ProcessEventQueue();
         SetDefaultObj(false);
         if(! m_MailStream)
            return false;
         else
            return true;
      }
      if ( !exists
           && (GetType() == MF_FILE || GetType() == MF_MH))
      {
         mail_create(NIL, (char *)m_MailboxPath.c_str());
      }

      // first try, don't log errors (except in debug mode)
      // If we don't have a mailstream yet, we half-open one:
      // this would give us a valid stream to use for looking up the
#if 0
      if(m_MailStream == NIL)
         m_MailStream = mail_open(NIL,(char *)m_MailboxPath.c_str(),
                                  (debugFlag ? OP_DEBUG : NIL)|OP_HALFOPEN);
#endif
      if(m_MailStream == NIL)
         m_MailStream = mail_open(m_MailStream,(char *)m_MailboxPath.c_str(),
                                  debugFlag ? OP_DEBUG : NIL);
      if(m_MailStream == NIL) // try to create it
      {
         CCVerbose();
         mail_create(NIL, (char *)m_MailboxPath.c_str());
         m_MailStream = mail_open(m_MailStream,(char *)m_MailboxPath.c_str(),
                                  debugFlag ? OP_DEBUG : NIL);
      }
      ProcessEventQueue();
      SetDefaultObj(false);
   }
   CCVerbose();
   if(m_MailStream == NIL)
   {
      // this will fail if file already exists, but it makes sure we can open it
      // try again:
      // if this fails, we want to know it, so no CCQuiet()
      MCclientLocker lock;
      SetDefaultObj();
      mail_create(NIL, (char *)m_MailboxPath.c_str());
      ProcessEventQueue();
      m_MailStream = mail_open(m_MailStream,(char *)m_MailboxPath.c_str(),
                               debugFlag ? OP_DEBUG : NIL);
      ProcessEventQueue();
      SetDefaultObj(false);
   }
   if(m_MailStream == NIL)
   {
         STATUSMESSAGE((_("Could not open mailbox %s."), GetName().c_str()));
         return false;
   }

   AddToMap(m_MailStream); // now we are known

   // listing already built

   STATUSMESSAGE((_("Mailbox %s opened."), GetName().c_str()));

   PY_CALLBACK(MCB_FOLDEROPEN, 0, GetProfile());
   return true;   // success
}

MailFolderCC *
MailFolderCC::FindFolder(String const &path, String const &login)
{
   StreamConnectionList::iterator i;
   DBGMESSAGE(("Looking for folder to re-use: '%s',login '%s'",
               path.c_str(), login.c_str()));

   for(i = streamList.begin(); i != streamList.end(); i++)
   {
      DBGMESSAGE(("  Comparing to entry: '%s',login '%s'",
                  (**i).name.c_str(), (**i).login.c_str()));
      if( (*i)->name == path && (*i)->login == login)
      {
         DBGMESSAGE(("  Re-using entry: '%s',login '%s'",
                     (**i).name.c_str(), (**i).login.c_str()));
         return (*i)->folder;
      }
   }
   DBGMESSAGE(("  No matching entry found."));
   return NULL;
}

bool
MailFolderCC::PingReopen(void) const
{
   DBGMESSAGE(("MailFolderCC::PingReopen() on Folder %s.", GetName().c_str()));
   if(m_PingReopenSemaphore)
   {
      DBGMESSAGE(("MailFolderCC::PingReopen() avoiding recursion!"));
      return false;  // or would true be better?
   }

   // just to circumvene the "const":
   MailFolderCC *t = (MailFolderCC *)this;

   t->m_PingReopenSemaphore = true;
   bool rc = true;

   String msg;
   msg.Printf(_("Dial-Up network is down.\n"
                "Do you want to try and check folder '%s' anyway?"), GetName().c_str());
   if(NeedsNetwork()
      && ! mApplication->IsOnline()
      && ! MDialog_YesNoDialog(msg,
                               NULL, MDIALOG_YESNOTITLE,
                               false, GetName()+"/NoNetPingAnyway"))
   {
      ((MailFolderCC *)this)->Close(); // now folder is dead
      return false;
   }


   if(! m_MailStream || ! mail_ping(m_MailStream))
   {
      if(m_MailStream)
         ProcessEventQueue(); // flush queue
      RemoveFromMap(); // will be added again by Open()
      LOGMESSAGE((M_LOG_WINONLY, _("Mailstream for folder '%s' has been closed, trying to reopen it."),
                  GetName().c_str()));
      rc = t->Open();
      if(rc == false)
      {
         ERRORMESSAGE((_("Re-opening closed folder '%s' failed."),GetName().c_str()));
         t->m_MailStream = NIL;
      }
   }
   if(m_MailStream)
   {
      ProcessEventQueue();
      rc = true;
      LOGMESSAGE((M_LOG_WINONLY, _("Folder '%s' is alive."),
                  GetName().c_str()));
   }
   else
      rc = false;
   t->m_PingReopenSemaphore = false;

   return rc;
}

/* static */ bool
MailFolderCC::PingReopenAll(void)
{
   // try to reopen all streams
   bool rc = true;
   StreamConnectionList::iterator i;
   const MailFolderCC *mf;
   for(mf = MailFolderCC::GetFirstMapEntry(i);
       mf;
       mf = MailFolderCC::GetNextMapEntry(i))
      rc &= mf->PingReopen();
   return rc;
}

void
MailFolderCC::Ping(void)
{
   if(NeedsNetwork() && ! mApplication->IsOnline())
   {
      ERRORMESSAGE((_("System is offline, cannot access mailbox ´%s´"), GetName().c_str()));
      return;
   }
   if(Lock())
   {
      DBGMESSAGE(("MailFolderCC::Ping() on Folder %s.",
                  GetName().c_str()));

      int ccl = CC_SetLogLevel(M_LOG_WINONLY);

      ProcessEventQueue();

      // This is terribly inefficient to do, but needed for some sick
      // POP3 servers.
      if(m_FolderFlags & MF_FLAGS_REOPENONPING)
      {
         DBGMESSAGE(("MailFolderCC::Ping() forcing close on folder %s.",
                     GetName().c_str()));
         Close();
      }

      if(PingReopen())
      {
         mail_check(m_MailStream); // update flags, etc, .newsrc
         ProcessEventQueue();
      }
      CC_SetLogLevel(ccl);
      if(m_NumOfMessages > m_OldMessageCount)
         RequestUpdate();
      ProcessEventQueue();
      UnLock();
   }
}

void
MailFolderCC::Close(void)
{
   // can cause references to this folder, cannot be allowd:
   //ProcessEventQueue();
   CCQuiet(true); // disable all callbacks!
   // We cannot run ProcessEventQueue() here as we must not allow any
   // Message to be created from this stream. If we miss an event -
   // that's a pity.
   if( m_MailStream )
   {
      if(!NeedsNetwork() || mApplication->IsOnline())
      {
         mail_check(m_MailStream); // update flags, etc, .newsrc
         mail_close(m_MailStream);
      }
      else
         gs_CCStreamCleaner->Add(m_MailStream); // delay closing
      m_MailStream = NIL;
   }
   CCVerbose();
   RemoveFromMap();
}

bool
MailFolderCC::Lock(void) const
{
#ifdef USE_THREADS
   ((MailFolderCC *)this)->m_Mutex->Lock();
   return true;
#else
   if(m_Mutex == false)
   {
      ((MailFolderCC *)this)->m_Mutex = true;
      return true;
   }
   else
      return false;
#endif
}

void
MailFolderCC::UnLock(void) const
{
#ifdef USE_THREADS
   ((MailFolderCC *)this)->m_Mutex->Unlock();
#else
   ((MailFolderCC *)this)->m_Mutex = false;
#endif
}




bool
MailFolderCC::AppendMessage(String const &msg)
{
   CHECK_DEAD_RC("Appending to closed folder '%s' failed.", false);

   STRING str;

   INIT(&str, mail_string, (void *) msg.c_str(), msg.Length());
   ProcessEventQueue();

//??   PingReopen();
#if 0
   char *flags = "\\NEW\\RECENT";
   bool rc = ( mail_append_full(
      m_MailStream,
      (char *)m_MailboxPath.c_str(),
      flags, NIL, &str)
               != 0);
#else
   bool rc = ( mail_append(
      m_MailStream, (char *)m_MailboxPath.c_str(), &str)
               != 0);
#endif
   if(! rc)
      ERRORMESSAGE(("cannot append message"));
   ProcessEventQueue();
   return rc;
}

bool
MailFolderCC::AppendMessage(Message const &msg)
{
   CHECK_DEAD_RC("Appending to closed folder '%s' failed.", false);
   String tmp;

   msg.WriteToString(tmp);
   return AppendMessage(tmp);
}


unsigned long
MailFolderCC::CountMessages(int mask, int value) const
{
   unsigned long numOfMessages = m_NumOfMessages;

   if ( mask )
   {
      HeaderInfoList *hil = GetHeaders();
      if ( !hil )
         return 0;

      // FIXME there should probably be a much more efficient way (using
      //       cclient functions?) to do it
      for ( unsigned long msgno = 0; msgno < m_NumOfMessages; msgno++ )
      {
         if ( ((*hil)[msgno]->GetStatus() & mask) != value)
            numOfMessages--;
      }
      hil->DecRef();
   }

   return numOfMessages;
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



class HeaderInfoList *
MailFolderCC::GetHeaders(void) const
{
   if(! m_Listing && m_NumOfMessages > 0)
   {
      ((MailFolderCC *)this)->UpdateListing();
   }
   if(m_Listing) m_Listing->IncRef();
   return m_Listing;
}

bool
MailFolderCC::SetSequenceFlag(String const &sequence,
                              int flag,
                              bool set)
{
   CHECK_DEAD_RC("Cannot access closed folder\n'%s'.", false);
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
   return true; /* not supported by c-client */
}


bool
MailFolderCC::SetFlag(const INTARRAY *selections, int flag, bool set)
{
   int n = selections->Count();
   int i;
   String sequence;
   for(i = 0; i < n-1; i++)
      sequence << strutil_ultoa((*selections)[i]) << ',';
   if(n)
      sequence << strutil_ultoa((*selections)[n-1]);
   return SetSequenceFlag(sequence, flag, set);
}

void
MailFolderCC::ExpungeMessages(void)
{
   CHECK_DEAD("Cannot access closed folder\n'%s'.");
   if(PY_CALLBACK(MCB_FOLDEREXPUNGE,1,GetProfile()))
      mail_expunge (m_MailStream);
   ProcessEventQueue();
}

/*
  TODO: this code relies on the header info list being sorted the same
  way as the listing in the folderview. Better would be to return UIds
  rather than msgnumbers and have the folderview select individual
  messages, or simply mark them as selected somehow and rebuild the
  listing in the views. But that seems to be an O(N^2) operation. :-(
*/

INTARRAY *
MailFolderCC::SearchMessages(const class SearchCriterium *crit)
{
   ASSERT(m_SearchMessagesFound == NULL);
   ASSERT(crit);

   HeaderInfoList *hil = GetHeaders();
   if ( !hil )
      return 0;

   unsigned long lastcount = 0;
   MProgressDialog *progDlg = NULL;
   if ( m_NumOfMessages > (unsigned)READ_CONFIG(m_Profile,
                                                MP_FOLDERPROGRESS_THRESHOLD) )
   {
      String msg;
      msg.Printf(_("Searching in %lu messages..."),
                 (unsigned long) m_NumOfMessages);
      progDlg = new MProgressDialog(GetName(),
                                    msg,
                                    m_NumOfMessages,
                                    NULL,
                                    false, true);// open a status window:
   }

   m_SearchMessagesFound = new INTARRAY;

   String what;

   bool error = false;

   for ( unsigned long msgno = 0; msgno < m_NumOfMessages; msgno++ )
   {
      if(crit->m_What == SearchCriterium::SC_SUBJECT)
         what = (*hil)[msgno]->GetSubject();
      else
         if(crit->m_What == SearchCriterium::SC_FROM)
            what = (*hil)[msgno]->GetFrom();
         else
         {
            Message *msg = GetMessage((*hil)[msgno]->GetUId());
            ASSERT(hil);
            switch(crit->m_What)
            {
            case SearchCriterium::SC_FULL:
            case SearchCriterium::SC_BODY:
               // wrong for body as it checks the whole message
               // including header
               what = msg->FetchText();
               break;
            case SearchCriterium::SC_HEADER:
               what = msg->GetHeader();
               break;
            case SearchCriterium::SC_TO:
               msg->GetHeaderLine("To", what);
               break;
            case SearchCriterium::SC_CC:
               msg->GetHeaderLine("CC", what);
               break;
            default:
               LOGMESSAGE((M_LOG_ERROR,"Unknown search criterium!"));
               error = true;
            }
            msg->DecRef();
         }
      bool found =  what.Contains(crit->m_Key);
      if(crit->m_Invert)
         found = ! found;
      if(found)
         m_SearchMessagesFound->Add(msgno); // sequence number, not uid!!!

      if(progDlg)
      {
         String msg;
         msg.Printf(_("Searching in %lu messages..."),
                    (unsigned long) m_NumOfMessages);
         String msg2;
         unsigned long cnt = m_SearchMessagesFound->Count();
         if(cnt != lastcount)
         {
            msg2.Printf(_(" - %lu matches found."), cnt);
            msg = msg + msg2;
            if(! progDlg->Update( msgno, msg ))
               break;  // abort searching
            lastcount = cnt;
         }
         else if(! progDlg->Update( msgno ))
            break;  // abort searching
      }
   }
   hil->DecRef();

#if 0
   /// TODO: Use this code for IMAP!
   // c-client does not implement this for all drivers, so we do it ourselves
   SEARCHPGM *pgm = mail_newsearchpgm();

   STRINGLIST *slist = mail_newstringlist();


   slist->text.data = (unsigned char *)strutil_strdup(crit->m_Key);
   slist->text.size = crit->m_Key.length();

   bool error = false;

   switch(crit->m_What)
   {
   case SearchCriterium::SC_FULL:
      pgm->subject = slist; break;
   case SearchCriterium::SC_BODY:
      pgm->subject = slist; break;
   case SearchCriterium::SC_SUBJECT:
      pgm->subject = slist; break;
   case SearchCriterium::SC_TO:
      pgm->subject = slist; break;
   case SearchCriterium::SC_FROM:
      pgm->subject = slist; break;
   case SearchCriterium::SC_CC:
      pgm->subject = slist; break;
   default:
      LOGMESSAGE((M_LOG_ERROR,"Unknown search criterium!"));
      error = true;
   }

   if(! error)
      mail_search_full (m_MailStream,
                        /* charset */ NIL,
                        pgm,
                        SE_UID|SE_FREE);
   else
      mail_free_searchpgm(&pgm);
#endif

   INTARRAY *rc = m_SearchMessagesFound; // will get freed by caller!
   m_SearchMessagesFound = NULL;

   if(progDlg)
      delete progDlg;
   return rc;
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
   str << "mailbox '" << m_MailboxPath << "' of type " << (int) m_folderType;

   return str;
}

#endif // DEBUG

/// remove this object from Map
void
MailFolderCC::RemoveFromMap(void) const
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
   static int mm_overview_header (MAILSTREAM *stream,unsigned long uid, OVERVIEW *ov);
   void mail_fetch_overview_nonuid (MAILSTREAM *stream,char *sequence,overview_t ofn);
   void mail_fetch_overview_x (MAILSTREAM *stream,char *sequence,overview_x_t ofn);
};


bool
MailFolderCC::IsNewMessage(const HeaderInfo *hi)
{
   UIdType msgId = hi->GetUId();
   bool isNew = false;
   int status = hi->GetStatus();

   if( (status & MSG_STAT_SEEN) == 0
       && ( status & MSG_STAT_RECENT))
      isNew = true;
   if(m_LastNewMsgUId != UID_ILLEGAL
      && m_LastNewMsgUId >= msgId)
      isNew = false;

   return isNew;
}

/* This is called by the UpdateListing method of the common code. */
HeaderInfoList *
MailFolderCC::BuildListing(void)
{
   m_BuildListingSemaphore = true;
   m_UpdateNeeded = false;

   CHECK_DEAD_RC("Cannot access closed folder\n'%s'.", NULL);

   if ( m_FirstListing )
   {
      // Hopefully this will only count really existing messages for
      // NNTP connections.
      if(GetType() == MF_NNTP)
         m_NumOfMessages = m_MailStream->recent;
      else
         m_NumOfMessages = m_MailStream->nmsgs;
   }

   if(m_Listing && m_NumOfMessages > m_OldNumOfMessages)
   {
      m_Listing->DecRef();
      m_Listing = NULL;
   }

   if(! m_Listing )
      m_Listing =  HeaderInfoListCC::Create(m_NumOfMessages);

   // by default, we retrieve all messages
   unsigned long numMessages = m_NumOfMessages;
   // the first one to retrieve
   unsigned long firstMessage = 1; //m_NumOfMessages - numMessages + 1;

   // if the number of the messages in the folder is bigger than the
   // configured limit, ask the user whether he really wants to retrieve them
   // all. The value of 0 disables the limit. Ask only once and never for file
   // folders (loading headers from them is quick)
   if ( !IsLocalQuickFolder(GetType()) &&
        (m_RetrievalLimit > 0) && m_FirstListing &&
        (m_NumOfMessages > m_RetrievalLimit) )
   {
      // too many messages - ask the user how many of them he really wants
      String msg, prompt, title;
      title.Printf(_("How many messages to retrieve from folder '%s'?"),
            GetName().c_str());
      msg.Printf(_("This folder contains %lu messages, which is greater than\n"
               "the current threshold of %lu (set it to 0 to avoid this "
               "question)."),
            m_NumOfMessages, m_RetrievalLimit);
      prompt = _("How many of them do you want to retrieve?");

      long nRetrieve = MGetNumberFromUser(msg, prompt, title,
                                          m_RetrievalLimit,
                                          1, m_NumOfMessages);
      if ( nRetrieve != -1 )
      {
         numMessages = nRetrieve;
         if(numMessages > m_NumOfMessages)
            numMessages = m_NumOfMessages;
      }
      //else: cancelled, retrieve all
   }

   m_BuildNextEntry = 0;

   if ( m_ProgressDialog == NULL &&
        numMessages > (unsigned)READ_CONFIG(m_Profile,
                                            MP_FOLDERPROGRESS_THRESHOLD) )
   {
      String msg;
      msg.Printf(_("Reading %lu message headers..."), numMessages);
      m_ProgressDialog = new MProgressDialog(GetName(),
                                             msg,
                                             numMessages,
                                             NULL,
                                             false, true);// open a status window:
   }

   // mail_fetch_overview() will now fill the m_Listing array with
   // info on the messages
   /* stream, sequence, header structure to fill */
   String sequence;
   if ( numMessages != m_NumOfMessages )
   {
      sequence = strutil_ultoa(mail_uid(m_MailStream, m_NumOfMessages-numMessages+1));
      sequence += ":*";
   }
   else
   {
      sequence.Printf("%lu", firstMessage);
      if( GetType() == MF_NNTP )
         // FIXME: no idea why this works for NNTP but not for the other types
         sequence << '-';
      else
         sequence << ":*";
   }

   mail_fetch_overview_x(m_MailStream, (char *)sequence.c_str(), mm_overview_header);

   if ( m_ProgressDialog != (MProgressDialog *)1 )
      delete m_ProgressDialog;

   // We set it to an illegal address here to suppress further updating. This
   // value is checked against in OverviewHeader(). The reason is that we only
   // want it the first time that the folder is being opened.
   m_ProgressDialog = (MProgressDialog *)1;

   // for NNTP, it will not show all messages
   //ASSERT(m_BuildNextEntry == m_NumOfMessages || m_folderType == MF_NNTP);
   m_NumOfMessages = m_BuildNextEntry;
   m_Listing->SetCount(m_NumOfMessages);


   m_FirstListing = false;
   m_BuildListingSemaphore = false;

   m_Listing->IncRef();
   return m_Listing;
}

int
MailFolderCC::OverviewHeaderEntry (unsigned long uid, OVERVIEW *ov)
{
   ASSERT(m_Listing);

   /* Ignore all entries that have arrived in the meantime to avoid
      overflowing the array.
   */
   if(m_BuildNextEntry >= m_NumOfMessages)
      return 0;

   // This is 1 if we don't want any further updates.
   if(m_ProgressDialog && m_ProgressDialog != (MProgressDialog *)1)
   {
      if(! m_ProgressDialog->Update( m_BuildNextEntry ))
         return 0;
   }

   // This seems to occasionally happen. Some race condition?
   if(m_BuildNextEntry >= m_Listing->Count())
   {
      m_NumOfMessages = m_Listing->Count();
      return 0;
   }

   HeaderInfoCC & entry = *(HeaderInfoCC *)(*m_Listing)[m_BuildNextEntry];

   ADDRESS *adr;

   unsigned long msgno = mail_msgno (m_MailStream,uid);
   MESSAGECACHE *elt = mail_elt (m_MailStream,msgno);
   MESSAGECACHE selt;

   // STATUS:
   entry.m_Status = GetMsgStatus(elt);

   /* For NNTP, do not show deleted messages: */
   if(m_folderType == MF_NNTP && elt->deleted)
      return 1; // ignore but continue

   // DATE
   mail_parse_date (&selt,ov->date);
   entry.m_Date = (time_t) mail_longdate( &selt);

   // FROM
   /* get first from address from envelope */
   for (adr = ov->from; adr && !adr->host; adr = adr->next);
   if(adr)
   {
      entry.m_From = "";
      if (adr->personal) // a personal name is given
         entry.m_From << adr->personal;
      if(adr->mailbox)
      {
         if(adr->personal)
            entry.m_From << " <";
         entry.m_From << adr->mailbox;
         if(adr->host && strlen(adr->host)
            && (strcmp(adr->host,BADHOST) != 0))
            entry.m_From << '@' << adr->host;
         if(adr->personal)
            entry.m_From << '>';
      }
   }
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
   if(m_ProgressDialog && m_ProgressDialog != (MProgressDialog *)1)
      m_ProgressDialog->Update( m_BuildNextEntry - 1 );
   return 1;
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
   if(cclientInitialisedFlag == true)
      return;
   // do further initialisation
#include <linkage.c>
   cclientInitialisedFlag = true;
   ASSERT(gs_CCStreamCleaner == NULL);
   gs_CCStreamCleaner = new CCStreamCleaner();
}

String MailFolderCC::ms_MHpath;

const String&
MailFolderCC::InitializeMH()
{
   if ( !ms_MHpath )
   {
      // first, init cclient
      if ( !cclientInitialisedFlag )
      {
         CClientInit();
      }

      // normally, the MH path is read by MH cclient driver from the MHPROFIle
      // file (~/.mh_profile under Unix), but we can't rely on this because if
      // this file is not found, it results in an error and the MH driver is
      // disabled - though it's perfectly ok for this file to be empty... So
      // we can use cclient MH logic if this file exists - but not if it
      // doesn't (and never under Windows)

#ifdef OS_UNIX
      String home = getenv("HOME");
      String filenameMHProfile = home + "/.mh_profile";
      FILE *fp = fopen(filenameMHProfile, "r");
      if ( fp )
      {
         fclose(fp);
      }
      else
#endif // OS_UNIX
      {
         // need to find MH path ourself
#ifdef OS_UNIX
         // the standard location under Unix
         String pathMH = home + "/Mail";
#else // !Unix
         // use the user directory by default
         String pathMH = READ_APPCONFIG(MP_USERDIR);
#endif // Unix/!Unix

         // const_cast is harmless
         mail_parameters(NULL, SET_MHPATH, (char *)pathMH.c_str());
      }

      // force cclient to init the MH driver
      char tmp[MAILTMPLEN];
      if ( !mh_isvalid("#MHINBOX", tmp, TRUE /* syn only check */) )
      {
         wxLogError(_("Sorry, support for MH folders is disabled."));
      }
      else
      {
         // retrieve the MH path (notice that we don't always find it ourself -
         // sometimes it's found only by the call to mh_isvalid)

         // calling mail_parameters doesn't work because of a compiler bug: gcc
         // mangles mh_parameters function completely and it never returns
         // anything for GET_MHPATH - I didn't find another workaround
#if 0
         (void)mail_parameters(NULL, GET_MHPATH, &tmp);

         ms_MHpath = tmp;
#else // 1
         ms_MHpath = mh_getpath();
#endif // 0/1

         // the path should have a trailing [back]slash
         if ( !!ms_MHpath && !wxIsPathSeparator(ms_MHpath.Last()) )
         {
            ms_MHpath << wxFILE_SEP_PATH;
         }
      }
   }

   return ms_MHpath;
}

bool
MailFolderCC::GetMHFolderName(String *path)
{
   String& name = *path;

   if ( wxIsAbsolutePath(name) )
   {
      if ( !InitializeMH() ) // it's harmless to call it more than once
      {
         // no MH support
         return FALSE;
      }

      wxString pathFolder(name, ms_MHpath.length());
      if ( strutil_compare_filenames(pathFolder, ms_MHpath) )
      {
         // skip MH path (and trailing slash which follows it)
         name = name.c_str() + ms_MHpath.length();
      }
      else
      {
         wxLogError(_("Invalid MH folder name '%s' - all MH folders should "
                      "be under '%s' directory."),
                    name.c_str(),
                    ms_MHpath.c_str());

         return FALSE;
      }
   }
   //else: relative path - leave as is

   return TRUE;
}

bool MailFolderCC::SpecToFolderName(const String& specification,
                                    FolderType folderType,
                                    String *pName)
{
   CHECK( pName, FALSE, "NULL name in MailFolderCC::SpecToFolderName" );

   String& name = *pName;
   switch ( folderType )
   {
      case MF_MH:
         {
            static const int lenPrefix = 4;  // strlen("#mh/")
            if ( strncmp(specification, "#mh/", lenPrefix) != 0 )
            {
               FAIL_MSG("invalid MH folder specification - no #mh/ prefix");

               return FALSE;
            }

            // make sure that the folder name does not start with s;ash
            const char *start = specification.c_str() + lenPrefix;
            while ( wxIsPathSeparator(*start) )
            {
               start++;
            }

            name = start;
         }
         break;

      case MF_NNTP:
      case MF_NEWS:
         {
            int startIndex = wxNOT_FOUND;
            if ( specification[0u] == '{' )
            {
               wxString protocol(specification.c_str() + 1, 4);
               protocol.MakeLower();
               if ( protocol == "nntp" || protocol == "news" )
               {
                  startIndex = specification.Find('}');
               }
               //else: leave it to be wxNOT_FOUND
            }

            if ( startIndex == wxNOT_FOUND )
            {
               FAIL_MSG("invalid NNTP folder specification - no {nntp/...}");

               return FALSE;
            }

            name = specification.c_str() + (size_t)startIndex + 1;
         }
         break;

      default:
         FAIL_MSG("not done yet");
         return FALSE;
   }

   return TRUE;
}

/// adds this object to Map
void
MailFolderCC::AddToMap(MAILSTREAM const *stream) const
{
   StreamConnection  *conn = new StreamConnection;
   conn->folder = (MailFolderCC *) this;
   conn->stream = stream;
   conn->name = m_MailboxPath;
   conn->login = m_Login;
   streamList.push_front(conn);
}

/* static */
bool
MailFolderCC::CanExit(String *which)
{
   bool canExit = true;
   *which = "";
   StreamConnectionList::iterator i;
   for(i = streamList.begin(); i != streamList.end(); i++)
      if( (*i)->folder->InCritical() )
      {
         canExit = false;
         *which << (*i)->folder->GetName() << '\n';
      }
   return canExit;
}

/// lookup object in Map
/* static */ MailFolderCC *
MailFolderCC::LookupObject(MAILSTREAM const *stream, const char *name)
{
   StreamConnectionList::iterator i;
   for(i = streamList.begin(); i != streamList.end(); i++)
   {
      if( (*i)->stream == stream )
         return (*i)->folder;
   }

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
MailFolderCC::SetDefaultObj(bool setit) const
{
   if(setit)
   {
      CHECK_RET(streamListDefaultObj == NULL,
                "conflicting mail folder default object access (trying to override non-NULL)!");
      streamListDefaultObj = (MailFolderCC *)this;
   }
   else
   {
      CHECK_RET(streamListDefaultObj == this,
                "conflicting mail folder default object access (trying to re-set to NULL)!");
      streamListDefaultObj = NULL;
   }
}



/// this message matches a search
void
MailFolderCC::mm_searched(MAILSTREAM * stream,
                          unsigned long number)
{
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
   {
      mf->m_SearchMessagesFound->Add(number);
   }
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

      // this test seems necessary for MH folders, otherwise we're going into
      // an infinite loop (and it shouldn't (?) break anything for other
      // folders)
      if ( (mf->m_NumOfMessages == 0 && number != 0) ||
           (mf->m_NumOfMessages != number) )
      {
         mf->m_NumOfMessages = number;
         mf->RequestUpdate();
      }
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
MailFolderCC::mm_notify(MAILSTREAM * stream, String str, long errflg)
{
   mm_log(str,errflg, MailFolderCC::LookupObject(stream));
}


/** this mailbox name matches a listing request
       @param stream mailstream
       @param delim  character that separates hierarchies
       @param name   mailbox name
       @param attrib mailbox attributes
       */
void
MailFolderCC::mm_list(MAILSTREAM * stream,
                      char delim ,
                      String  name,
                      long  attrib)
{
   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET(mf,"NULL mailfolder");

   // VZ: is this really needed?
#if 0
   CHECK_RET(mf->m_FolderListing,"NULL mailfolder listing");
#endif

   // create the event corresponding to the folder
   ASMailFolder::ResultFolderExists *result =
      ASMailFolder::ResultFolderExists::Create
      (
         mf->m_ASMailFolder,
         mf->m_Ticket,
         name,
         delim,
         mf->m_UserData
      );

   // and send it
   MEventManager::Send(new MEventASFolderResultData (result) );

   // don't forget to free the result - MEventASFolderResultData makes a copy
   // (i.e. calls IncRef) of it
   result->DecRef();
}


/** matches a subscribed mailbox listing request
       @param stream mailstream
       @param delim  character that separates hierarchies
       @param name   mailbox name
       @param attrib mailbox attributes
       */
void
MailFolderCC::mm_lsub(MAILSTREAM * stream,
                      char delim ,
                      String  name,
                      long  attrib)
{
   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET(mf,"NULL mailfolder");
   CHECK_RET(mf->m_FolderListing,"NULL mailfolder listing");
   mf->m_FolderListing->SetDelimiter(delim);
   mf->m_FolderListing->Add(new FolderListingEntryCC(name, attrib));
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
   MEventManager::Send( new MEventFolderUpdateData (mf) );
}

/** log a message
    @param str message str
    @param errflg error level
*/
void
MailFolderCC::mm_log(String str, long errflg, MailFolderCC *mf )
{
   String  msg;
   if(mf)
      msg.Printf(_("Folder '%s' : "), mf->GetName().c_str());
   else
      msg = _("Folder Log: ");
   msg += str;
#ifdef DEBUG
   msg << _(", error level: ") << strutil_ultoa(errflg);
#endif
   if(errflg > 1)
      LOGMESSAGE((CC_GetLogLevel(), msg));
   else
      LOGMESSAGE((M_LOG_WINONLY, Str(msg)));
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

   LOGMESSAGE((M_LOG_WINONLY, Str(msg)));
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
MailFolderCC::mm_critical(MAILSTREAM * stream)
{
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
   {
#ifdef DEBUG
      String tmp = "MailFolderCC::mm_critical() for folder " + mf->m_MailboxPath;
      LOGMESSAGE((M_LOG_DEBUG, Str(tmp)));
#endif
      mf->m_InCritical = true;
   }
}

/**   no longer running critical code
   @param   stream mailstream
     */
void
MailFolderCC::mm_nocritical(MAILSTREAM *  stream )
{
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
   {
#ifdef DEBUG
      String tmp = "MailFolderCC::mm_nocritical() for folder " + mf->m_MailboxPath;
      LOGMESSAGE((M_LOG_DEBUG, Str(tmp)));
#endif
      mf->m_InCritical = false;
   }
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
                              evptr->m_args[1].m_long,
                              NULL);
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
      {
         /* We just need to update the information for exactly one
            message.
         */
         MailFolderCC *mf = LookupObject(evptr->m_stream);
         CHECK_RET(mf,"NULL mailfolder");
         // Find the listing entry for this message:
         UIdType uid = mail_uid(evptr->m_stream,
                                evptr->m_args[0].m_ulong);
         HeaderInfoList *hil = mf->GetHeaders();
         ASSERT(hil);
         if(hil)
         {
            UIdType i;
            for(i = 0; i < hil->Count() && (*hil)[i]->GetUId() != uid; i++)
               ;
            if(i < hil->Count())
            {
               ASSERT(((*hil)[i])->GetUId() == uid);
               MESSAGECACHE *elt = mail_elt (evptr->m_stream,evptr->m_args[0].m_ulong);
               ((HeaderInfoCC *)((*hil)[i]))->m_Status = GetMsgStatus(elt);
               // now we sent an update event to update folderviews etc
               mf->UpdateMessageStatus(uid);
            }
            hil->DecRef();
         }
         break;
      }
      case Expunged:
      {
         MailFolderCC *mf = MailFolderCC::LookupObject(evptr->m_stream);
         ASSERT(mf);
         if(mf) mf->RequestUpdate();  // Queues an Update event.
         break;
      }
      /* The Update event is not caused by c-client callbacks but
         by this very event handling mechanism itself. It causes
         BuildListing() to fetch a new listing. */
      case Update:
      {
         MailFolderCC *mf = MailFolderCC::LookupObject(evptr->m_stream);
         ASSERT(mf);
         if(mf && mf->UpdateNeeded())  // only call it once
            mf->UpdateListing();
         break;
      }
      }// switch
      delete evptr;
   }
}

void
MailFolderCC::RequestUpdate(void)
{
   // we want only one update event
   if(! m_UpdateNeeded)
   {
      MailFolderCC::Event *evptr = new MailFolderCC::Event(m_MailStream,Update);
      MailFolderCC::QueueEvent(evptr);
      m_UpdateNeeded = true;
   }
}

/* Handles the mm_overview_header callback on a per folder basis. */
int
MailFolderCC::OverviewHeader (MAILSTREAM *stream, unsigned long uid, OVERVIEW *ov)
{
   MailFolderCC *mf = MailFolderCC::LookupObject(stream);
   ASSERT(mf);
   return mf->OverviewHeaderEntry(uid, ov);
}


/* static */
bool
MailFolderCC::Subscribe(const String &host,
                        FolderType protocol,
                        const String &mailboxname,
                        bool subscribe)
{
   String spec = BuildFolderSpec(host, protocol, mailboxname);
   return (subscribe ? mail_subscribe (NIL, (char *)spec.c_str())
           : mail_unsubscribe (NIL, (char *)spec.c_str()) ) != NIL;
}

void
MailFolderCC::ListFolders(ASMailFolder *asmf,
                          const String &pattern,
                          bool subscribedOnly,
                          const String &reference,
                          UserData ud,
                          Ticket ticket)
{
   String spec = m_MailboxPath;

   /* Make sure that IMAP specifications are either
      {...}pattern or {....}path/pattern
      A spec like {....}pathpattern is useless.
   */
   if(GetType() == MF_IMAP
      && spec.Length() > 0
      && spec[spec.Length()-1] != '/'
      && spec[spec.Length()-1] != '}')
      spec += '/';

   spec += pattern;

   ASSERT(asmf);

   // set user data (retrieved by mm_list)
   m_UserData = ud;
   m_Ticket = ticket;
   m_ASMailFolder = asmf;
   m_ASMailFolder->IncRef();

   char *ref = reference.length() == 0 ? NULL : (char *)reference.c_str();

   if ( subscribedOnly )
   {
      mail_lsub (m_MailStream, ref, (char *) spec.c_str());
   }
   else
   {
      mail_list (m_MailStream, ref, (char *) spec.c_str());
   }

   // Send event telling about end of listing:
   ASMailFolder::ResultFolderExists *result =
      ASMailFolder::ResultFolderExists::Create
      (
         m_ASMailFolder,
         m_Ticket,
         "",  // empty name == no more entries
         0,   // delim == 0 ==> no more entries
         m_UserData
      );

   // and send it
   MEventManager::Send(new MEventASFolderResultData (result) );

   result->DecRef();
   m_ASMailFolder->DecRef();
   m_ASMailFolder = NULL;
}



extern "C"
{

/* Mail fetch message overview
 * Accepts: mail stream
 *            UID sequence to fetch
 *            pointer to overview return function

 -- Modified to evaluate "continue" return code of callback to allow
    user to abort it.
 -- Also modified to fetch messages backwards, useful if aborted.
 */

void mail_fetch_overview_x (MAILSTREAM *stream,char *sequence,overview_x_t ofn)
{
  if (stream->dtb && !(stream->dtb->overview &&
                       (*stream->dtb->overview) (stream,sequence,(overview_t)ofn)) &&
      mail_uid_sequence (stream,sequence) && mail_ping (stream)) {
    MESSAGECACHE *elt;
    ENVELOPE *env = NULL;  // initialisation not needed but keeps compiler happy
    OVERVIEW ov;
    unsigned long i;
    ov.optional.lines = 0;
    ov.optional.xref = NIL;
    for (i = stream->nmsgs; i>= 1; i--)
      if (((elt = mail_elt (stream,i))->sequence) &&
          (env = mail_fetch_structure (stream,i,NIL,NIL)) && ofn) {
        ov.subject = env->subject;
        ov.from = env->from;
        ov.date = env->date;
        ov.message_id = env->message_id;
        ov.references = env->references;
        ov.optional.octets = elt->rfc822_size;
        if(! (*ofn) (stream,mail_uid (stream,i),&ov))
           break;
      }
  }
}


/* Mail fetch message overview using sequence numbers instead of UIDs!
 * Accepts: mail stream
 *    sequence to fetch (no-UIDs but sequence numbers)
 *    pointer to overview return function
 */

void mail_fetch_overview_nonuid (MAILSTREAM *stream,char *sequence,overview_t ofn)
{
   if (stream->dtb &&
       !(stream->dtb->overview && (*stream->dtb->overview)(stream,sequence,ofn)) &&
       mail_sequence (stream,sequence) &&
       mail_ping (stream))
   {
      MESSAGECACHE *elt;
      ENVELOPE *env = NULL;  // keep compiler happy
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
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Searched);
   evptr->m_args[0].m_ulong = number;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_expunged(MAILSTREAM *stream, unsigned long number)
{
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Expunged);
   evptr->m_args[0].m_ulong = number;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_flags(MAILSTREAM *stream, unsigned long number)
{
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Flags);
   evptr->m_args[0].m_ulong = number;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_notify(MAILSTREAM *stream, char *str, long errflg)
{
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Notify);
   evptr->m_args[0].m_str = new String(str);
   evptr->m_args[1].m_long = errflg;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_list(MAILSTREAM *stream, int delim, char *name, long attrib)
{
   if(mm_disable_callbacks)
      return;

   MailFolderCC::mm_list(stream, delim, name, attrib);
/*
  MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::List);
   evptr->m_args[0].m_int = delim;
   evptr->m_args[1].m_str = new String(name);
   evptr->m_args[2].m_long = attrib;
   MailFolderCC::QueueEvent(evptr);
*/
}

void
mm_lsub(MAILSTREAM *stream, int delim, char *name, long attrib)
{
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::LSub);
   evptr->m_args[0].m_int = delim;
   evptr->m_args[1].m_str = new String(name);
   evptr->m_args[2].m_long = attrib;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_exists(MAILSTREAM *stream, unsigned long number)
{
   if(mm_disable_callbacks)
      return;
   // update count immediately to reflect change:
   MailFolderCC::mm_exists(stream, number);
}

void
mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS *status)
{
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Status);
   MailFolderCC::QueueEvent(evptr);
}

void
mm_log(char *str, long errflg)
{
   if(mm_disable_callbacks || mm_ignore_errors)
      return;

   String *msg = new String(str);
   if(errflg >= 4) // fatal imap error, reopen-mailbox
   {
      if(!MailFolderCC::PingReopenAll())
         *msg << _("\nAttempt to re-open all folders failed.");
   }

   MailFolderCC::Event *evptr = new MailFolderCC::Event(NULL,MailFolderCC::Log);
   evptr->m_args[0].m_str = msg;
   evptr->m_args[1].m_long = errflg;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_dlog(char *str)
{
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(NULL,MailFolderCC::DLog);
   evptr->m_args[0].m_str = new String(str);
   MailFolderCC::QueueEvent(evptr);
}

void
mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
   if(mm_disable_callbacks)
      return;
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
   if(mm_disable_callbacks)
      return 1;
   return MailFolderCC::mm_diskerror(stream,  errorcode, serious);
}

void
mm_fatal(char *str)
{
   if(mm_disable_callbacks)
      return;
   MailFolderCC::mm_fatal(str);
}

/* This callback needs to be processed immediately or ov will become
   invalid. */
static int
mm_overview_header (MAILSTREAM *stream,unsigned long uid, OVERVIEW *ov)
{
   return MailFolderCC::OverviewHeader(stream, uid, ov);
}

} // extern "C"

// ----------------------------------------------------------------------------
// debugging support
// ----------------------------------------------------------------------------

#ifdef DEBUG

String HeaderInfoListCC::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("%u entries", Count());

   return s1 + s2;
}

#endif // DEBUG
