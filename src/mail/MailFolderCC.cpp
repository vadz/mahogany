//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MailFolderCC.cpp: implements MailFolderCC class
// Purpose:     handling of mail folders with c-client lib
// Author:      Karsten Ballüder
// Modified by: Vadim Zeitlin at 24.01.01: complete rewrite of update logic
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

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

#include "MPython.h"

#include "Mdefaults.h"
#include "MDialogs.h"
#include "FolderView.h"

#include "AddressCC.h"
#include "MailFolderCC.h"
#include "MessageCC.h"

#include "MEvent.h"
#include "MThread.h"
#include "ASMailFolder.h"
#include "Mpers.h"

// we need "Impl" for ArrayHeaderInfo declaration
#include "HeaderInfoImpl.h"

#include "modules/Filters.h"

#include "MFCache.h"
#include "Sequence.h"

// just to use wxFindFirstFile()/wxFindNextFile() for lockfile checking and
// wxFile::Exists() too
#include <wx/filefn.h>
#include <wx/file.h>

// DecodeHeader() uses CharsetToEncoding()
#include <wx/fontmap.h>

#include <ctype.h>   // isspace()

#ifdef OS_UNIX
   #include <signal.h>

   static void sigpipe_handler(int)
   {
      // do nothing
   }
#endif // OS_UNIX

#ifdef OS_WIN
    #include "wx/msw/private.h"     // for ::UpdateWindow()
#endif // OS_WIN

extern "C"
{
   #undef LOCAL         // previously defined in other cclient headers
   #include <pop3.h>    // for pop3_xxx() in GetMessageUID()
   #undef LOCAL         // now defined by pop3.h

   #define namespace cc__namespace
   #include <imap4r1.h> // for LEVELSORT/THREAD in CanSort()/Thread()
   #undef namespace
}

// this is #define'd in windows.h included by one of cclient headers
#undef CreateDialog

#define USE_READ_PROGRESS
//#define USE_BLOCK_NOTIFY

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// check if the folder is still available trying to reopen it if not
#define CHECK_DEAD(msg) \
   if ( m_MailStream == NIL && !PingReopen() ) \
   { \
      ERRORMESSAGE((_(msg), GetName().c_str()));\
      return;\
   }

#define CHECK_DEAD_RC(msg, rc) \
   if ( m_MailStream == NIL && !PingReopen() )\
   { \
      ERRORMESSAGE((_(msg), GetName().c_str()));\
      return rc; \
   }

// enable this as needed as it produces *lots* of outputrun
#ifdef DEBUG_STREAMS
#   define CHECK_STREAM_LIST() MailFolderCC::DebugStreams()
#else // !DEBUG
#   define CHECK_STREAM_LIST()
#endif // DEBUG/!DEBUG

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_DEBUG_CCLIENT;
extern const MOption MP_FOLDERPROGRESS_THRESHOLD;
extern const MOption MP_FOLDER_FILE_DRIVER;
extern const MOption MP_FOLDER_LOGIN;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_FOLDER_TRY_CREATE;
extern const MOption MP_IMAP_LOOKAHEAD;
extern const MOption MP_MESSAGEPROGRESS_THRESHOLD_SIZE;
extern const MOption MP_MESSAGEPROGRESS_THRESHOLD_TIME;
extern const MOption MP_MSGS_SERVER_SORT;
extern const MOption MP_MSGS_SERVER_THREAD;
extern const MOption MP_MSGS_SERVER_THREAD_REF_ONLY;
extern const MOption MP_NEWS_SPOOL_DIR;
extern const MOption MP_RSH_PATH;
extern const MOption MP_SSH_PATH;
extern const MOption MP_TCP_CLOSETIMEOUT;
extern const MOption MP_TCP_OPENTIMEOUT;
extern const MOption MP_TCP_READTIMEOUT;
extern const MOption MP_TCP_WRITETIMEOUT;
extern const MOption MP_TCP_RSHTIMEOUT;
extern const MOption MP_TCP_SSHTIMEOUT;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// invalid value for MailFolderCC::m_chDelimiter
#define ILLEGAL_DELIMITER ((char)-1)

// ----------------------------------------------------------------------------
// trace masks used (you have to wxLog::AddTraceMask() to enable the
// correpsonding kind of messages)
// ----------------------------------------------------------------------------

// turn on to get messages about using the mail folder cache (ms_StreamList)
#define TRACE_MF_CACHE  "mfcache"

// turn on to log [almost] all cclient callbacks
#define TRACE_MF_CALLBACK "cccback"

// turn on logging of MailFolderCC operations
#define TRACE_MF_CALLS "mfcall"

// turn on logging of events sent by MailFolderCC
#define TRACE_MF_EVENTS "mfevent"

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

extern "C"
{
#ifdef USE_READ_PROGRESS
   void mahogany_read_progress(GETS_DATA *md, unsigned long count);
#endif

#ifdef USE_BLOCK_NOTIFY
   void *mahogany_block_notify(int reason, void *data);
#endif
};

typedef void (*mm_list_handler)(MAILSTREAM *stream,
                                char delim,
                                String name,
                                long  attrib);

typedef void (*mm_status_handler)(MAILSTREAM *stream,
                                  const char *mailbox,
                                  MAILSTATUS *status);

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

/// object used to reflect some events back to MailFolderCC
static class CCEventReflector *gs_CCEventReflector = NULL;

/// object used to close the streams if it can't be done when closing folder
static class CCStreamCleaner *gs_CCStreamCleaner = NULL;

/// handler for temporarily redirected mm_list calls
static mm_list_handler gs_mmListRedirect = NULL;

/// handler for temporarily redirected mm_status calls
static mm_status_handler gs_mmStatusRedirect = NULL;

// a variable telling c-client to shut up
static bool mm_ignore_errors = false;

/// a variable disabling most events (not mm_list or mm_lsub though)
static bool mm_disable_callbacks = false;

/// show cclient debug output
#ifdef DEBUG
   static bool mm_show_debug = true;
#else
   static bool mm_show_debug = false;
#endif

// loglevel for cclient error messages:
static int cc_loglevel = wxLOG_Error;

// ============================================================================
// private classes
// ============================================================================

/**
   The idea behind CCEventReflector is to allow postponing some actions in
   MailFolderCC code, i.e. instead of doing something immediately after getting
   c-client notification, we generate and send a helper event which is
   processed by gs_CCEventReflector later (== during the next idle handler
   iteration) and which calls back the corresponding MailFolderCC method on the
   object which got the c-client notification in the first place.

   This is necessary in some cases and very convenient in the others: necessary
   for new mail handling as we want to filter it which can (and will) result in
   more c-client calls but this can't be done from inside c-client handler
   (such as mm_exists()) because c-client is not reentrant. Convenient for
   batching together several notifications: for example, we often receive a lot
   of mm_flags at once (when the status of a selection consisting of many
   messages is changed) and forwarding all of them to the GUI immediately
   would result in horrible flicker - so, instead, we just accumulate them in
   internal arrays and generate a single MsgStatus event for them when
   OnMsgStatusChanged() is called.

   The same technique should be used to batch process other notifications as
   well if it's worth it.
 */

// ----------------------------------------------------------------------------
// private event classes used together with CCEventReflector to reflect
// some events back to MailFolderCC
// ----------------------------------------------------------------------------

/// MEventFolderOnNewMailData - used by MailFolderCC to update itself
class MEventFolderOnNewMailData : public MEventWithFolderData
{
public:
   MEventFolderOnNewMailData(MailFolder *folder)
      : MEventWithFolderData(MEventId_MailFolder_OnNewMail, folder) { }
};

/// MEventFolderOnMsgStatusData - used by MailFolderCC to send MsgStatus events
class MEventFolderOnMsgStatusData : public MEventWithFolderData
{
public:
   MEventFolderOnMsgStatusData(MailFolder *folder)
      : MEventWithFolderData(MEventId_MailFolder_OnMsgStatus, folder) { }
};

// ----------------------------------------------------------------------------
// CCEventReflector
// ----------------------------------------------------------------------------

// this class processes MEventFolderOnNewMail events by just calling
// MailFolderCC::OnNewMail()
class CCEventReflector : public MEventReceiver
{
public:
   CCEventReflector()
   {
      MEventManager::RegisterAll
      (
         this,
         MEventId_MailFolder_OnNewMail,   &m_cookieNewMail,
         MEventId_MailFolder_OnMsgStatus, &m_cookieMsgStatus,
         MEventId_Null
      );
   }

   virtual bool OnMEvent(MEventData& ev)
   {
      MEventWithFolderData& event = (MEventWithFolderData &)ev;
      MailFolderCC *mfCC = (MailFolderCC *)event.GetFolder();

      switch ( ev.GetId() )
      {
         case MEventId_MailFolder_OnNewMail:
            mfCC->OnNewMail();
            break;

         case MEventId_MailFolder_OnMsgStatus:
            mfCC->OnMsgStatusChanged();
            break;

         default:
            FAIL_MSG( "unexpected event in CCEventReflector!" );
            return true;
      }

      // no need to search further, only we process these events
      return false;
   }

   virtual ~CCEventReflector()
   {
      MEventManager::DeregisterAll
      (
         &m_cookieNewMail,
         &m_cookieMsgStatus,
         NULL
      );
   }

private:
   void *m_cookieNewMail,
        *m_cookieMsgStatus;
};

// ----------------------------------------------------------------------------
// CCStreamCleaner
// ----------------------------------------------------------------------------

/** This is a list of all mailstreams which were left open because the
    dialup connection broke before we could close them.
    We remember them and try to close them at program exit. */
KBLIST_DEFINE(StreamList, MAILSTREAM);

class CCStreamCleaner
{
public:
   void Add(MAILSTREAM *stream)
   {
      CHECK_RET( stream, "NULL stream in CCStreamCleaner::Add()" );

      m_List.push_back(stream);
   }

   ~CCStreamCleaner();

private:
   StreamList m_List;
};

// ----------------------------------------------------------------------------
// OverviewData: this class is just used to share parameters between
// GetHeaderInfo() and OverviewHeaderEntry() which is called from it
// indirectly
// ----------------------------------------------------------------------------

class OverviewData
{
public:
   // ctor takes the pointer to the start of HeaderInfo buffer and the number
   // of slots in it and the msgno we are going to start retrieving headers
   // from
   OverviewData(const Sequence& seq, ArrayHeaderInfo& headers, size_t nTotal)
      : m_seq(seq), m_headers(headers)
   {
      m_nTotal = nTotal;
      m_nRetrieved = 0;
      m_progdlg = NULL;

      m_current = m_seq.GetFirst(m_seqCookie);
   }

   // dtor deletes the progress dialog if it had been shown
   ~OverviewData()
   {
      if ( m_progdlg )
      {
         MGuiLocker locker;
         delete m_progdlg;

         // ok, it's really ugly to put it here, but it's the only way to do it
         // for now, unfortunately (FIXME)
         //
         // we don't want to leave a big "hole" on the parent window but this
         // may happen as we are not returning to the main loop immediately -
         // instead we risk to spend some (long) time applying filters and the
         // window won't be repainted before we finish, so force window redraw
         // right now
#ifdef OS_WIN
         wxFrame *frame = mApplication->TopLevelFrame();
         if ( frame )
         {
            ::UpdateWindow(GetHwndOf(frame));
         }
#endif // OS_WIN
      }
   }

   // get the current HeaderInfo object
   HeaderInfo *GetCurrent() const { return m_headers[m_current - 1]; }

   // get the number of messages retrieved so far
   size_t GetRetrievedCount() const { return m_nRetrieved; }

   // go to the next message
   void Next()
   {
      m_nRetrieved++;

      m_current = m_seq.GetNext(m_current, m_seqCookie);
   }

   // return true if we retrieved all the messages
   bool Done() const
   {
      ASSERT_MSG( m_nRetrieved <= m_nTotal, "too many messages" );

      return m_nRetrieved == m_nTotal;
   }

   // tell us to use the progress dialog
   void SetProgressDialog(MProgressDialog *progdlg)
   {
      ASSERT_MSG( !m_progdlg, "SetProgressDialog() can only be called once" );

      m_progdlg = progdlg;
   }

   // get the progress dialog we use (may be NULL)
   MProgressDialog *GetProgressDialog() const { return m_progdlg; }

   // update the progress dialog and return false if it was cancelled
   bool UpdateProgress() const
   {
      if ( m_progdlg )
      {
         MGuiLocker locker;
         if ( !m_progdlg->Update(m_nRetrieved) )
         {
            // cancelled by user
            return false;
         }
      }

      return true;
   }

private:
   // the total number of messages we are going to retrieve
   size_t m_nTotal;

   // the number of headers retrieved so far, the final value of this is the
   // GetHeaderInfo() return value
   size_t m_nRetrieved;

   // the current msgno in the sequence
   UIdType m_current;

   // the sequence cookie used for iterating over it
   size_t m_seqCookie;

   // the sequence we're overviewing
   const Sequence& m_seq;

   // the array where we read the info
   ArrayHeaderInfo& m_headers;

   // the progress dialog if we use one, NULL otherwise
   MProgressDialog *m_progdlg;
};

// ----------------------------------------------------------------------------
// FolderListing stuff (TODO: move elsewhere, i.e. in MailFolderCmn)
// ----------------------------------------------------------------------------

class FolderListingEntryCC : public FolderListingEntry
{
public:
   /// The folder's name.
   virtual const String &GetName(void) const
      { return m_folderName;}
   /// The folder's attribute.
   virtual long GetAttribute(void) const
      { return m_Attr; }
   FolderListingEntryCC(const String &name, long attr)
      : m_folderName(name)
      { m_Attr = attr; }
private:
   const String &m_folderName;
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

// ----------------------------------------------------------------------------
// Various locker classes
// ----------------------------------------------------------------------------

// lock the folder in ctor, unlock in dtor
class MailFolderLocker
{
public:
   MailFolderLocker(MailFolderCC *mf) : m_mf(mf) { m_mf->Lock(); }
   ~MailFolderLocker() { m_mf->UnLock(); }

   operator bool() const { return m_mf->IsLocked(); }

private:
   MailFolderCC *m_mf;
};

// temporarily disable (most) cclient callbacks
class CCCallbackDisabler
{
public:
   CCCallbackDisabler()
   {
      m_old = mm_disable_callbacks;
      mm_disable_callbacks = true;
   }

   ~CCCallbackDisabler() { mm_disable_callbacks = m_old; }

private:
   bool m_old;
};

// temporarily disable error messages from cclient
class CCErrorDisabler
{
public:
   CCErrorDisabler()
   {
      m_old = mm_ignore_errors;
      mm_ignore_errors = true;
   }

   ~CCErrorDisabler() { mm_ignore_errors = m_old; }

private:
   bool m_old;
};

// temporarily disable callbacks and errors from cclient
class CCAllDisabler : public CCCallbackDisabler, public CCErrorDisabler
{
};

// temporarily change cclient log level for all messages
class CCLogLevelSetter
{
public:
   CCLogLevelSetter(int level)
   {
      m_level = cc_loglevel;
      cc_loglevel = level;
   }

   ~CCLogLevelSetter() { cc_loglevel = m_level; }

private:
   int m_level;
};

// temporarily set the given as the default folder to use for cclient
// callbacks
class CCDefaultFolder
{
public:
   CCDefaultFolder(MailFolderCC *mf)
   {
      m_mfOld = MailFolderCC::ms_StreamListDefaultObj;
      MailFolderCC::ms_StreamListDefaultObj = mf;
   }

   ~CCDefaultFolder() { MailFolderCC::ms_StreamListDefaultObj = m_mfOld; }

private:
   MailFolderCC *m_mfOld;
};

// temporarily pause a global timer
class MTimerSuspender
{
public:
   MTimerSuspender(MAppBase::Timer timer)
   {
      m_restart = mApplication->StopTimer(m_timer = timer);
   }

   ~MTimerSuspender()
   {
      if ( m_restart )
      {
         if ( !mApplication->StartTimer(m_timer) )
         {
            wxFAIL_MSG("Failed to restart suspended timer");
         }
      }
   }

private:
   MAppBase::Timer m_timer;
   bool m_restart;
};

// ----------------------------------------------------------------------------
// redirector classes: temporarily redirect c-client callbacks
// ----------------------------------------------------------------------------

// temporarily redirect mm_list callbacks to the given function
class MMListRedirector
{
public:
   MMListRedirector(mm_list_handler handler)
   {
      // this would be a mess and mm_list() calls would surely go to a wrong
      // handler!
      ASSERT_MSG( !gs_mmListRedirect, "not supposed to be used recursively" );

      gs_mmListRedirect = handler;
   }

   ~MMListRedirector()
   {
      gs_mmListRedirect = NULL;
   }
};

// temporarily redirect mm_status callbacks and only remember the info they
// provide in the given MAILSTATUS object instead of performing the usual
// processing
class MMStatusRedirector
{
public:
   MMStatusRedirector(const String& mailbox, MAILSTATUS *mailstatus)
   {
      ms_mailstatus = mailstatus;
      memset(ms_mailstatus, 0, sizeof(*ms_mailstatus));
      ms_mailbox = mailbox;

      gs_mmStatusRedirect = MMStatusRedirectorHandler;
   }

   ~MMStatusRedirector()
   {
      gs_mmStatusRedirect = NULL;

      ms_mailstatus = NULL;
      // ms_mailbox.clear(); -- unneeded
   }

private:
   static String ms_mailbox;
   static MAILSTATUS *ms_mailstatus;

   static void MMStatusRedirectorHandler(MAILSTREAM * /* stream */,
                                         const char *name,
                                         MAILSTATUS *mailstatus)
   {
      // we can get the status events from other mailboxes, ignore them
      if ( strcmp(ms_mailbox, name) == 0 )
      {
         memcpy(ms_mailstatus, mailstatus, sizeof(*ms_mailstatus));
      }
   }
};

String MMStatusRedirector::ms_mailbox;
MAILSTATUS *MMStatusRedirector::ms_mailstatus = NULL;

// ----------------------------------------------------------------------------
// ReadProgressInfo: create an instance of this object to start showing progress
// info, delete it to make it disappear
// ----------------------------------------------------------------------------

#ifdef USE_READ_PROGRESS

class ReadProgressInfo
{
public:
   // totalToRead is the amount of data to be read, in byts, while secondsToWait
   // is the time we wait before popping up the dialog at all
   ReadProgressInfo(unsigned long totalToRead, int secondsToWait)
   {
      // never set m_readTotal to 0
      m_readTotal = totalToRead ? totalToRead : 1;
      m_readSoFar = 0ul;

      if ( secondsToWait )
      {
         // don't show the dialog immediately, wait a little
         m_dlgProgress = NULL;

         m_timeStart = time(NULL) + secondsToWait;
      }
      else // show the dialog from the beginning
      {
         CreateDialog();
      }
   }

   ~ReadProgressInfo()
   {
      delete m_dlgProgress;
   }

   void OnProgress(unsigned long count)
   {
      // CreateDialog() and Update() may call wxYield() and result in
      // generation of the idle events and calls to MEventManager::
      // DispatchPending() which may lead to a disaster if any event handler
      // decides to call back to c-client which is currently locked by our
      // caller
      //
      // So disable the event processing temporarily to avoid all these
      // complications
      MEventManagerSuspender suspendEvent;

      m_readSoFar = count;

      if ( !m_dlgProgress )
      {
         if ( time(NULL) > m_timeStart )
         {
            CreateDialog();
         }
         else // too early to show it yet
         {
            // skip Update() below (which would dereference NULL pointer)
            return;
         }
      }

      int percentDone = (m_readSoFar * 100) / m_readTotal;
      if ( percentDone > 100 )
      {
         // I don't know if this can happen, but if, by chance, it does, don't
         // look silly - cheat the user instead
         percentDone = 99;
      }

      m_dlgProgress->Update(percentDone);
   }

private:
   void CreateDialog()
   {
      String msg;
      msg.Printf(_("Reading %s..."),
                 MailFolder::SizeToString(m_readTotal, 0,
                                          MessageSize_AutoBytes,
                                          true /* verbose */).c_str()
                );

      m_dlgProgress = new MProgressDialog
                          (
                           _("Retrieving data from server"),
                           msg
                          );
   }

   MProgressDialog *m_dlgProgress;

   unsigned long m_readSoFar,
                 m_readTotal;

   // the moment when we started reading
   time_t m_timeStart;
} *gs_readProgressInfo = NULL;

#endif // USE_READ_PROGRESS

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// various MailFolderCC statics
// ----------------------------------------------------------------------------

String MailFolderCC::ms_LastCriticalFolder = "";

// ----------------------------------------------------------------------------
// functions working with IMAP specs and flags
// ----------------------------------------------------------------------------

static String GetImapFlags(int flag)
{
   String flags;
   if(flag & MailFolder::MSG_STAT_SEEN)
      flags <<"\\SEEN ";

   // RECENT flag can't be set, it is handled entirely by server in IMAP

   if(flag & MailFolder::MSG_STAT_ANSWERED)
      flags <<"\\ANSWERED ";
   if(flag & MailFolder::MSG_STAT_DELETED)
      flags <<"\\DELETED ";
   if(flag & MailFolder::MSG_STAT_FLAGGED)
      flags <<"\\FLAGGED ";

   if(flags.Length() > 0) // chop off trailing space
     flags.RemoveLast();

   return flags;
}

// small GetImapSpec() helper
static String GetSSLOptions(int flags)
{
   String specSsl;

#ifdef USE_SSL
   if ( flags & MF_FLAGS_SSLAUTH )
   {
      specSsl << "/ssl";
      if ( flags & MF_FLAGS_SSLUNSIGNED )
         specSsl << "/novalidate-cert";
   }
#endif // USE_SSL

   return specSsl;
}

/* static */
String MailFolder::GetImapSpec(int typeOrig,
                               int flags,
                               const String &name,
                               const String &iserver,
                               const String &login)
{
   String mboxpath;

   String server = iserver;
   strutil_tolower(server);

   FolderType type = (FolderType)typeOrig;

#ifdef USE_SSL
   extern bool InitSSL(void);

   // SSL only for NNTP/IMAP/POP:
   if( (flags & MF_FLAGS_SSLAUTH) && !FolderTypeSupportsSSL(type) )
   {
      flags ^= MF_FLAGS_SSLAUTH;
      wxLogWarning(_("Ignoring SSL authentication for folder '%s'"),
                   name.c_str());
   }

   if( (flags & MF_FLAGS_SSLAUTH) != 0 && ! InitSSL() )
#else // !USE_SSL
   if( (flags & MF_FLAGS_SSLAUTH) != 0 )
#endif // USE_SSL/!USE_SSL
   {
      ERRORMESSAGE((_("SSL authentication is not available.")));
      flags ^= MF_FLAGS_SSLAUTH;
   }

   switch( type )
   {
      case MF_INBOX:
         mboxpath = "INBOX";
         break;

      case MF_FILE:
         mboxpath = strutil_expandfoldername(name, type);
         break;

      case MF_MH:
         {
            // accept either absolute or relative filenames here, but the absolute
            // ones should start with MHROOT
            wxString mhRoot = MailFolderCC::InitializeMH();

            const char *p;
            wxString nameReal;
            if ( name.StartsWith(mhRoot, &nameReal) )
            {
               p = nameReal.c_str();
            }
            else
            {
               p = name.c_str();

               if ( *p == '/' )
               {
                  wxLogError(_("Invalid MH folder name '%s' not under the "
                               "root MH directory '%s'."),
                            p, mhRoot.c_str());

                  return "";
               }
            }

            // newly created MH folders won't have leading slash, but we may still
            // have to deal with the old entries in config, so eat both here, but
            // be sure to remove the leading backslash: we want #mh/inbox, not
            // #mh//inbox
            while ( *p == '/' )
               p++;

            mboxpath << "#mh/" << p;
         }
         break;

      case MF_POP:
         mboxpath << '{' << server << "/pop3" << GetSSLOptions(flags) << '}';
         break;

      case MF_IMAP:
         mboxpath << '{' << server;
         if ( flags & MF_FLAGS_ANON )
         {
            mboxpath << "/anonymous";
         }
         else
         {
            if( !login.empty() )
               mboxpath << "/user=" << login ;
            //else: we get asked  later
         }

         mboxpath << GetSSLOptions(flags) << '}' << name;
         break;

      case MF_NEWS:
         mboxpath << "#news." << name;
         break;

      case MF_NNTP:
         mboxpath << '{' << server << "/nntp"
                  << GetSSLOptions(flags) << '}' << name;
         break;

      default:
         FAIL_MSG("Unsupported folder type.");
   }

   return mboxpath;
}

static
wxString GetImapSpec(const MFolder *folder)
{
   return MailFolder::GetImapSpec(folder->GetType(),
                                  folder->GetFlags(),
                                  folder->GetPath(),
                                  folder->GetServer(),
                                  folder->GetLogin());
}

bool MailFolder::SpecToFolderName(const String& specification,
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

      // Why this code? The spec is {hostname/nntp} not
      // {nntp/hostname}, I leave it in for now, but add correct(?)
      // code underneath:
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
         wxString lowercase = specification;
         lowercase.MakeLower();
         if( lowercase.Contains("/nntp") ||
             lowercase.Contains("/news") )
         {
            startIndex = specification.Find('}');
         }
      }

      if ( startIndex == wxNOT_FOUND )
      {
         FAIL_MSG("invalid folder specification - no {nntp/...}");

         return FALSE;
      }

      name = specification.c_str() + (size_t)startIndex + 1;
   }
   break;
   case MF_IMAP:
      name = specification.AfterFirst('}');
      return TRUE;
   default:
      FAIL_MSG("not done yet");
      return FALSE;
   }

   return TRUE;
}

// This does not return driver prefixes such as #mh/ which are
// considered part of the pathname for now.
static
String GetFirstPartFromImapSpec(const String &imap)
{
   if(imap[0] != '{') return "";
   String first;
   const char *cptr = imap.c_str()+1;
   while(*cptr && *cptr != '}')
      first << *cptr++;
   first << *cptr;
   return first;
}

static
String GetPathFromImapSpec(const String &imap)
{
   if(imap[0] != '{') return imap;

   const char *cptr = imap.c_str()+1;
   while(*cptr && *cptr != '}')
      cptr++;
   if(*cptr == '}')
      return cptr+1;
   else
      return ""; // error
}

// ----------------------------------------------------------------------------
// misc helpers
// ----------------------------------------------------------------------------

// Small function to translate c-client status flags into ours.
static int GetMsgStatus(const MESSAGECACHE *elt)
{
   int status = 0;
   if (elt->recent)
      status |= MailFolder::MSG_STAT_RECENT;
   if (elt->seen)
      status |= MailFolder::MSG_STAT_SEEN;
   if (elt->flagged)
      status |= MailFolder::MSG_STAT_FLAGGED;
   if (elt->answered)
      status |= MailFolder::MSG_STAT_ANSWERED;
   if (elt->deleted)
      status |= MailFolder::MSG_STAT_DELETED;

   // TODO: what about custom/user flags?

   return status;
}

// msg status info
//
// NB: name MessageStatus is already taken by MailFolder
struct MsgStatus
{
   bool recent;
   bool unread;
   bool newmsgs;
   bool flagged;
   bool searched;
};

// is this message recent/new/unread/...?
static MsgStatus AnalyzeStatus(int stat)
{
   MsgStatus status;

   // deal with recent and new (== recent and !seen)
   status.recent = (stat & MailFolder::MSG_STAT_RECENT) != 0;
   status.unread = !(stat & MailFolder::MSG_STAT_SEEN);
   status.newmsgs = status.recent && status.unread;

   // and also count flagged and searched messages
   status.flagged = (stat & MailFolder::MSG_STAT_FLAGGED) != 0;
   status.searched = (stat & MailFolder::MSG_STAT_SEARCHED ) != 0;

   return status;
}

// ----------------------------------------------------------------------------
// header decoding
// ----------------------------------------------------------------------------

/*
   See RFC 2047 for the description of the encodings used in the mail headers.
   Briefly, "encoded words" can be inserted which have the form of

      encoded-word = "=?" charset "?" encoding "?" encoded-text "?="

   where charset and encoding can't contain space, control chars or "specials"
   and encoded-text can't contain spaces nor "?".

   NB: don't be confused by 2 meanings of encoding here: it is both the
       charset encoding for us and also QP/Base64 encoding for RFC 2047
 */

/* static */
String MailFolder::DecodeHeader(const String &in, wxFontEncoding *pEncoding)
{
   // we don't enforce the sanity checks on charset and encoding - should we?
   // const char *specials = "()<>@,;:\\\"[].?=";

   wxFontEncoding encoding = wxFONTENCODING_SYSTEM;

   String out;
   out.reserve(in.length());
   for ( const char *p = in.c_str(); *p; p++ )
   {
      if ( *p == '=' && *(p + 1) == '?' )
      {
         // found encoded word

         // save the start of it
         const char *pEncWordStart = p++;

         // get the charset
         String csName;
         for ( p++; *p && *p != '?'; p++ ) // initial "++" to skip '?'
         {
            csName += *p;
         }

         if ( !*p )
         {
            wxLogDebug("Invalid encoded word syntax in '%s': missing charset.",
                       pEncWordStart);
            out += pEncWordStart;

            break;
         }

         if ( encoding == wxFONTENCODING_SYSTEM )
         {
            encoding = wxTheFontMapper->CharsetToEncoding(csName);
         }

         // get the encoding in RFC 2047 sense
         enum
         {
            Encoding_Unknown,
            Encoding_Base64,
            Encoding_QuotedPrintable
         } enc2047 = Encoding_Unknown;

         p++; // skip '?'
         if ( *(p + 1) == '?' )
         {
            if ( *p == 'B' || *p == 'b' )
               enc2047 = Encoding_Base64;
            else if ( *p == 'Q' || *p == 'q' )
               enc2047 = Encoding_QuotedPrintable;
         }
         //else: multi letter encoding unrecognized

         if ( enc2047 == Encoding_Unknown )
         {
            wxLogDebug("Unrecognized header encoding in '%s'.",
                       pEncWordStart);

            // scan until the end of the encoded word
            const char *pEncWordEnd = strstr(p, "?=");
            if ( !pEncWordEnd )
            {
               wxLogDebug("Missing encoded word end marker in '%s'.",
                          pEncWordStart);
               out += pEncWordStart;

               break;
            }
            else
            {
               out += String(pEncWordStart, pEncWordEnd);

               p = pEncWordEnd;

               continue;
            }
         }

         p += 2; // skip "Q?" or "B?"

         // get the encoded text
         bool hasUnderscore = false;
         const char *pEncTextStart = p;
         while ( *p && (p[0] != '?' || p[1] != '=') )
         {
            // this is needed for QP hack below
            if ( *p == '_' )
               hasUnderscore = true;

            p++;
         }

         unsigned long lenEncWord = p - pEncTextStart;

         if ( !*p )
         {
            wxLogDebug("Missing encoded word end marker in '%s'.",
                       pEncWordStart);
            out += pEncWordStart;

            continue;
         }

         // skip '=' following '?'
         p++;

         // now decode the text using cclient functions
         unsigned long len;
         unsigned char *start = (unsigned char *)pEncTextStart; // for cclient
         char *text;
         if ( enc2047 == Encoding_Base64 )
         {
            text = (char *)rfc822_base64(start, lenEncWord, &len);
         }
         else // QP
         {
            // cclient rfc822_qprint() behaves correctly and leaves '_' in the
            // QP encoded text because this is what RFC says, however many
            // broken clients replace spaces with underscores and so we undo it
            // here - better user-friendly than standard conformant
            String strWithoutUnderscores;
            if ( hasUnderscore )
            {
               strWithoutUnderscores = String(pEncTextStart, lenEncWord);
               strWithoutUnderscores.Replace("_", " ");
               start = (unsigned char *)strWithoutUnderscores.c_str();
            }

            text = (char *)rfc822_qprint(start, lenEncWord, &len);
         }

         out += String(text, (size_t)len);

         fs_give((void **)&text);
      }
      else
      {
         // just another normal char
         out += *p;
      }
   }

   if ( pEncoding )
      *pEncoding = encoding;

   return out;
}

// ----------------------------------------------------------------------------
// MailFolderCC auth info
// ----------------------------------------------------------------------------

// login data
String MailFolderCC::MF_user;
String MailFolderCC::MF_pwd;

/* static */
void
MailFolderCC::SetLoginData(const String &user, const String &pw)
{
   MailFolderCC::MF_user = user;
   MailFolderCC::MF_pwd = pw;
}

// ----------------------------------------------------------------------------
// MailFolderCC construction/opening
// ----------------------------------------------------------------------------

MailFolderCC::MailFolderCC(const MFolder *mfolder)
{
   m_mfolder = (MFolder *)mfolder; // const_cast needed for IncRef()/DecRef()
   m_mfolder->IncRef();

   m_Profile = mfolder->GetProfile();

   Create(mfolder->GetType(), mfolder->GetFlags());

   m_ImapSpec = ::GetImapSpec(mfolder);

   m_Login = mfolder->GetLogin();
   m_Password = mfolder->GetPassword();

   // do this at the very end as it calls RequestUpdate() and so anything may
   // happen from now on
   UpdateConfig();
}

void
MailFolderCC::Create(FolderType type, int flags)
{
   m_MailStream = NIL;
   m_nMessages = 0;

   UpdateTimeoutValues();

   Init();

   m_FolderListing = NULL;

   m_ASMailFolder = NULL;

   m_SearchMessagesFound = NULL;

   m_Mutex = new MMutex;
   m_InFilterCode = new MMutex;
}

void MailFolderCC::Init()
{
   m_LastUId = UID_ILLEGAL;

   m_expungedMsgnos =
   m_expungedPositions =
   m_statusChangedMsgnos = 
   m_statusChangedOld =
   m_statusChangedNew = NULL;

   m_InCritical = false;

   m_chDelimiter = ILLEGAL_DELIMITER;
}

MailFolderCC::~MailFolderCC()
{
   // we don't want to call Close() if we had been closed by outside code
   // before (this happens if the user chooses "close folder" manually)
   if ( m_MailStream )
   {
      Close();
   }

   // check that our temporary data isn't hanging around
   if ( m_SearchMessagesFound )
   {
      FAIL_MSG( "m_SearchMessagesFound unexpectedly != NULL" );

      delete m_SearchMessagesFound;
   }

   // normally this one should be deleted as well but POP3 server never sends
   // us mm_expunged() (well, because it never expunges the messages until we
   // close the folder but by this time the notifications are blocked) so in
   // this case it may be left lying around
   if ( m_expungedMsgnos )
   {
      FAIL_MSG( "m_expungedMsgnos unexpectedly != NULL" );

      delete m_expungedMsgnos;
      delete m_expungedPositions;
   }

   // these arrays must have been cleared by OnMsgStatusChanged() earlier
   if ( m_statusChangedMsgnos )
   {
      FAIL_MSG( "m_statusChangedMsgnos unexpectedly != NULL" );

      delete m_statusChangedMsgnos;
      delete m_statusChangedOld;
      delete m_statusChangedNew;
   }

   // we might still be listed, so we better remove ourselves from the
   // list to make sure no more events get routed to this (now dead) object
   RemoveFromMap();

   delete m_Mutex;
   delete m_InFilterCode;

   m_Profile->DecRef();
   m_mfolder->DecRef();
}

/* static */
bool MailFolderCC::AskPasswordIfNeeded(const String& name,
                                       FolderType type,
                                       int flags,
                                       String *login,
                                       String *password,
                                       bool *didAsk)
{
   CHECK( login && password, false, "can't be NULL here" );

   if ( FolderTypeHasUserName(type)
        && !(flags & MF_FLAGS_ANON)
        && (login->empty() || password->empty()) )
   {
      if ( !MDialog_GetPassword(name, login, password) )
      {
         ERRORMESSAGE((_("Cannot access this folder without a password.")));

         mApplication->SetLastError(M_ERROR_AUTH);

         return false;
      }

      // remember that the password was entered interactively and propose to
      // remember it if it really works later
      if ( didAsk )
         *didAsk = true;
   }

   return true;
}

/* static */
bool MailFolderCC::CreateIfNeeded(const MFolder *folder)
{
   Profile_obj profile(folder->GetProfile());

   if ( !profile->readEntryFromHere(MP_FOLDER_TRY_CREATE, false) )
   {
      // don't even try
      return true;
   }

   String imapspec = ::GetImapSpec(folder);

   wxLogTrace(TRACE_MF_CALLS, "Trying to create MailFolderCC '%s'.",
              imapspec.c_str());

   // disable callbacks as we don't have the folder to redirect them to
   CCAllDisabler noCallbacks;

   // and create the mailbox
   mail_create(NIL, (char *)imapspec.c_str());

   // try to open it now
   MAILSTREAM *stream = mail_open(NULL, (char *)imapspec.c_str(),
                                  mm_show_debug ? OP_DEBUG : NIL);

   if ( stream )
   {
      mail_close(stream);
   }

   // don't try any more, whatever the result was
   profile->DeleteEntry(MP_FOLDER_TRY_CREATE);

   return stream != NULL;
}

/* static */
MailFolderCC *
MailFolderCC::OpenFolder(const MFolder* mfolder,
                         OpenMode openmode)
{
   String login = mfolder->GetLogin();
   String mboxpath = ::GetImapSpec(mfolder);

   //FIXME: This should somehow be done in MailFolder.cpp
   MailFolderCC *mf = FindFolder(mboxpath, login);
   if(mf)
   {
      //FIXME: is this really needed? 
      mf->PingReopen(); // make sure it's updated

      return mf;
   }

   // ask the password for the folders which need it but for which it hadn't
   // been specified during creation
   bool userEnteredPwd = false;
   String password = mfolder->GetPassword();
   if ( !AskPasswordIfNeeded(mfolder->GetFullName(),
                             mfolder->GetType(),
                             mfolder->GetFlags(),
                             &login,
                             &password,
                             &userEnteredPwd) )
   {
      // can't continue
      mApplication->SetLastError(M_ERROR_CANCEL);

      return NULL;
   }

   // create the mail folder and init its members
   mf = new MailFolderCC(mfolder);

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
      // reset the last error
      mApplication->SetLastError(M_ERROR_OK);

      ok = mf->Open(openmode);

      if ( !ok && !mApplication->GetLastError() )
      {
         // catch all error code
         mApplication->SetLastError(M_ERROR_CCLIENT);
      }
   }

   if ( !ok )
   {
      mf->DecRef();
      mf = NULL;
   }
   else if ( userEnteredPwd )
   {
      // ask the user if he'd like to remember the password for the future:
      // this is especially useful for the folders created initially by the
      // setup wizard as it doesn't ask for the passwords
      if ( MDialog_YesNoDialog(_("Would you like to remember the password "
                                 "for this folder?\n"
                                 "(WARNING: don't do it if you are concerned "
                                 "about security)"),
                               NULL,
                               MDIALOG_YESNOTITLE,
                               TRUE, /* [Yes] default */
                               GetPersMsgBoxName(M_MSGBOX_REMEMBER_PWD)) )
      {
         // MFolder doesn't have methods to set them
         Profile *profile = mfolder->GetProfile();
         profile->writeEntry(MP_FOLDER_LOGIN, login);
         profile->writeEntry(MP_FOLDER_PASSWORD, strutil_encrypt(password));
      }
   }

   return mf;
}

void MailFolderCC::CreateFileFolder()
{
   FolderType folderType = GetType();

   CHECK_RET( folderType == MF_FILE || folderType == MF_MH,
              "shouldn't be called for non file folders" );

   bool exists;
   if ( folderType == MF_FILE )
   {
      exists = wxFileExists(m_ImapSpec);
   }
   else // if ( folderType == MF_MH )
   {
      // construct the filename from MH folder name
      String path;
      path << InitializeMH()
           << m_ImapSpec.c_str() + 4; // 4 == strlen("#mh/")
      exists = wxDirExists(path);
   }

   // if the file folder doesn't exist, we should create it first
   if ( !exists )
   {
      // This little hack makes it root (uid 0) safe and allows us
      // to choose the file format, too:
      String tmp;
      if ( folderType == MF_FILE )
      {
         static const char *cclient_drivers[] =
         {
            "mbx",
            "unix",
            "mmdf",
            "tenex"
         };

         ASSERT_MSG( WXSIZEOF(cclient_drivers) == FileMbox_Max,
                     "forgot to update something" );

         long format = READ_CONFIG(m_Profile, MP_FOLDER_FILE_DRIVER);
         if ( format < 0  || (size_t)format > FileMbox_Max )
         {
            FAIL_MSG( "invalid mailbox format" );
            format = 0;
         }

         tmp = "#driver.";
         tmp << cclient_drivers[format] << '/';

         wxLogDebug("Trying to create folder '%s' in %s format.",
                    GetName().c_str(), cclient_drivers[format]);
      }
      else // MF_MH folder
      {
         // the only one still not handled
         ASSERT_MSG( folderType == MF_MH, "unexpected folder type" );

         tmp = "#driver.mh/";
      }

      tmp += m_ImapSpec;
      mail_create(NIL, (char *)tmp.c_str());
   }
}

static String
GetOperationName(MailFolder::OpenMode openmode)
{
   String name;
   switch ( openmode )
   {
      case MailFolder::Normal:
         name = _("Opening");
         break;

      case MailFolder::ReadOnly:
         name = _("Opening in read only mode");
         break;

      case MailFolder::HalfOpen:
         name = _("Connecting to");
         break;

      default:
         FAIL_MSG( "unknown open mode" );
         name = "???";
   }

   return name;
}

bool
MailFolderCC::CheckForFileLock()
{
   String lockfile;

   FolderType folderType = GetType();
   if( folderType == MF_FILE )
      lockfile = m_ImapSpec;
#ifdef OS_UNIX
   else if ( folderType == MF_INBOX )
   {
      // get INBOX path name
      MCclientLocker lock;
      lockfile = (char *) mail_parameters (NIL,GET_SYSINBOX,NULL);
      if(lockfile.empty()) // another c-client stupidity
         lockfile = (char *) sysinbox();
   }
#endif // OS_UNIX
   else
   {
      FAIL_MSG( "shouldn't be called for not file based folders!" );
      return true;
   }

   lockfile << ".lock*"; //FIXME: is this fine for MS-Win? (NO!)
   lockfile = wxFindFirstFile(lockfile, wxFILE);
   while ( !lockfile.empty() )
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
               wxLogWarning(_("Could not remove lock-file.\n"
                              "Other process may have terminated.\n"
                              "Will try to continue as normal."));
         }
         else
         {
            wxLogError(_("Cannot open the folder while lock-file exists."));
            wxLogError(_("Could not open mailbox %s."), GetName().c_str());

            return false;
         }
      }
      lockfile = wxFindNextFile();
   }

   return true;
}

bool
MailFolderCC::Open(OpenMode openmode)
{
   wxLogTrace(TRACE_MF_CALLS, "%s \"'%s'\"",
              GetOperationName(openmode).c_str(), GetName().c_str());

   MFrame *frame = GetInteractiveFrame();
   if ( frame )
   {
      STATUSMESSAGE((frame, _("%s mailbox \"%s\"..."),
                     GetOperationName(openmode).c_str(), GetName().c_str()));
   }

   // Now, we apply the very latest c-client timeout values, in case they have
   // changed.
   UpdateTimeoutValues();

   FolderType folderType = GetType();
   if( FolderTypeHasUserName(folderType) )
   {
      SetLoginData(m_Login, m_Password);
   }

   // for files, check whether mailbox is locked, c-client library is
   // to dumb to handle this properly
   if ( openmode == Normal &&
        (
         folderType == MF_FILE
#ifdef OS_UNIX
         || folderType == MF_INBOX
#endif
        )
      )
   {
      if ( !CheckForFileLock() )
      {
         // folder is locked, can't open
         return false;
      }
   }
   //else: not a file based folder, no locking problems

   // lock cclient inside this block
   {
      MCclientLocker lock;

      // Make sure that all events to unknown folder go to us
      CCDefaultFolder def(this);

      long ccOptions = mm_show_debug ? OP_DEBUG : 0;
      bool tryOpen = true;
      switch ( openmode )
      {
         case Normal:
            // create the mailbox if it doesn't exist yet: this must be done
            // separately for the file folders to specify the format we want
            // this mailbox to be in
            if ( folderType == MF_FILE || folderType == MF_MH )
            {
               CreateFileFolder();
            }
            else // and for all the others
            {
               // check if this is the first time we're opening this folder: in
               // this case, try creating it first
               if ( !CreateIfNeeded(m_mfolder) )
               {
                  // CreateIfNeeded() returns false only if the folder couldn't
                  // be opened at all, no need to retry again
                  tryOpen = false;
               }
            }
            break;

         case ReadOnly:
            ccOptions |= OP_READONLY;
            break;

         case HalfOpen:
            ccOptions |= OP_HALFOPEN;
            break;
      }

      if ( tryOpen )
      {
         m_MailStream = mail_open(NIL, (char *)m_ImapSpec.c_str(), ccOptions);
      }
   } // end of cclient lock block

   // for some folders (notably the IMAP ones), mail_open() will return a not
   // NULL pointer but set halfopen flag if it couldn't be SELECTed - as we
   // really want to open it here and not halfopen, this is an error for us
   if ( m_MailStream && m_MailStream->halfopen && openmode != HalfOpen )
   {
      // unfortunately, at least the IMAP folder will have halfopen flag
      // set even in some situations when the folder can't be opened at all -
      // for example, if the INBOX is locked by another session (yes, this
      // goes against the IMAP RFC, but some servers just break the connection
      // if the mailbox is locked by another session...), so we need to try to
      // half open it now to know if this is just a folder with NOSELECT flag
      // or if we really can't open it

      // redirect all notifications to us again
      CCDefaultFolder def(this);
      MAILSTREAM *msHalfOpened = mail_open
                                 (
                                  NIL,
                                  (char *)m_ImapSpec.c_str(),
                                  (mm_show_debug ? OP_DEBUG : 0) | OP_HALFOPEN
                                 );
      if ( msHalfOpened )
      {
         mail_close(msHalfOpened);

         MFolder_obj folder(m_Profile);
         if ( folder )
         {
            // remember that we can't open it
            folder->AddFlags(MF_FLAGS_NOSELECT);
         }
         else
         {
            // were we deleted without noticing?
            FAIL_MSG( "associated folder somehow disappeared?" );
         }

         mApplication->SetLastError(M_ERROR_HALFOPENED_ONLY);
      }
      //else: it can't be opened at all

      mail_close(m_MailStream);
      m_MailStream = NIL;
   }

   // so did we eventually succeed in opening it or not?
   if ( m_MailStream == NIL )
   {
      wxLogError(_("Could not open mailbox '%s'."), GetName().c_str());
      return false;
   }

   // folder really opened!

   // now we are known
   AddToMap(m_MailStream);

   if ( frame )
   {
      String msg;
      switch ( openmode )
      {
         case Normal:
            msg = _("Mailbox \"%s\" opened.");
            break;

         case ReadOnly:
            msg = _("Mailbox \"%s\" opened in read only mode.");
            break;

         case HalfOpen:
            msg = _("Successfully connected to \"%s\"");
            break;

         default:
            FAIL_MSG( "unknown open mode" );
            break;
      }

      if ( !msg.empty() )
      {
         STATUSMESSAGE((frame, msg, GetName().c_str()));
      }
   }

   PY_CALLBACK(MCB_FOLDEROPEN, 0, GetProfile());

   return true;   // success
}

// ----------------------------------------------------------------------------
// MailFolderCC closing
// ----------------------------------------------------------------------------

void
MailFolderCC::Close()
{
   wxLogTrace(TRACE_MF_CALLS, "Closing folder '%s'", GetName().c_str());

   MailFolderCmn::Close();

   if ( m_MailStream )
   {
      /*
         DO NOT SEND EVENTS FROM HERE, ITS CALLED FROM THE DESTRUCTOR AND
         THE OBJECT *WILL* DISAPPEAR!
       */
      CCAllDisabler no;

      if ( !NeedsNetwork() || mApplication->IsOnline() )
      {
         // it is wrong to do this as it may result in mm_exists() callbacks
         // which we ignore (per above), so we miss new mail
#if 0
         if ( !m_MailStream->halfopen )
         {
            // update flags, etc, .newsrc
            mail_check(m_MailStream);
         }
#endif // 0

         mail_close(m_MailStream);
      }
      else // a remote folder but we're not connected
      {
         // delay closing as we can't do it properly right now
         gs_CCStreamCleaner->Add(m_MailStream);
      }

      m_MailStream = NIL;
   }

   // we could be closing before we had time to process all events
   //
   // FIXME: is this really true, i.e. does it ever happen?
   if ( m_expungedMsgnos )
   {
      delete m_expungedMsgnos;
      delete m_expungedPositions;

      m_expungedMsgnos = NULL;
      m_expungedPositions = NULL;
   }

   if ( m_statusChangedMsgnos )
   {
      delete m_statusChangedMsgnos;
      delete m_statusChangedOld;
      delete m_statusChangedNew;

      m_statusChangedMsgnos =
      m_statusChangedOld =
      m_statusChangedNew = NULL;
   }

   // normally the folder won't be reused any more but reset them just in case
   m_LastUId = UID_ILLEGAL;
   m_nMessages = 0;
}

/* static */
bool
MailFolderCC::CloseFolder(const MFolder *folder)
{
   // find the mailstream
   MailFolderCC *mf = FindFolder(folder);
   if ( !mf )
   {
      // nothing to close
      return false;
   }

   mf->Close();

   mf->DecRef();

   return true;
}

/* static */
int
MailFolderCC::CloseAll()
{
   size_t count = ms_StreamList.size();
   StreamConnection **connections = new StreamConnection *[count];

   size_t n = 0;
   for ( StreamConnectionList::iterator i = ms_StreamList.begin();
         i != ms_StreamList.end();
         i++ )
   {
      connections[n++] = *i;
   }

   for ( n = 0; n < count; n++ )
   {
      MailFolderCC *mf = connections[n]->folder;

      mf->Close();

      // notify any opened folder views
      MEventManager::Send(new MEventFolderClosedData(mf) );
   }

   delete [] connections;

   return count;
}

// ----------------------------------------------------------------------------
// MailFolderCC timeouts
// ----------------------------------------------------------------------------

void
MailFolderCC::ApplyTimeoutValues(void)
{
   (void) mail_parameters(NIL, SET_OPENTIMEOUT, (void *) m_TcpOpenTimeout);
   (void) mail_parameters(NIL, SET_READTIMEOUT, (void *) m_TcpReadTimeout);
   (void) mail_parameters(NIL, SET_WRITETIMEOUT, (void *) m_TcpWriteTimeout);
   (void) mail_parameters(NIL, SET_CLOSETIMEOUT, (void *) m_TcpCloseTimeout);
   (void) mail_parameters(NIL, SET_RSHTIMEOUT, (void *) m_TcpRshTimeout);
   (void) mail_parameters(NIL, SET_SSHTIMEOUT, (void *) m_TcpSshTimeout);

   (void) mail_parameters(NIL, SET_LOOKAHEAD, (void *) m_LookAhead);

   // only set the paths if we do use rsh/ssh
   if ( m_TcpRshTimeout )
      (void) mail_parameters(NIL, SET_RSHPATH, (char *)m_RshPath.c_str());
   if ( m_TcpSshTimeout )
      (void) mail_parameters(NIL, SET_SSHPATH, (char *)m_SshPath.c_str());
}



void
MailFolderCC::UpdateTimeoutValues(void)
{
   Profile *p = GetProfile();

   // We now use only one common config setting for all TCP timeouts
   m_TcpOpenTimeout = READ_CONFIG(p, MP_TCP_OPENTIMEOUT);
   m_TcpReadTimeout = READ_CONFIG(p, MP_TCP_READTIMEOUT);
   m_TcpWriteTimeout = READ_CONFIG(p, MP_TCP_WRITETIMEOUT);
   m_TcpCloseTimeout = READ_CONFIG(p, MP_TCP_CLOSETIMEOUT);
   m_LookAhead = READ_CONFIG(p, MP_IMAP_LOOKAHEAD);

   // but a separate one for rsh timeout to allow enabling/disabling rsh
   // independently of TCP timeout
   m_TcpRshTimeout = READ_CONFIG(p, MP_TCP_RSHTIMEOUT);

   // and another one for SSH
   m_TcpSshTimeout = READ_CONFIG(p, MP_TCP_SSHTIMEOUT);

   // also read the paths for the commands if we use them
   if ( m_TcpRshTimeout )
      m_RshPath = READ_CONFIG_TEXT(p, MP_RSH_PATH);
   if ( m_TcpSshTimeout )
      m_SshPath = READ_CONFIG_TEXT(p, MP_SSH_PATH);

   ApplyTimeoutValues();
}

// ----------------------------------------------------------------------------
// Working with the list of all open mailboxes
// ----------------------------------------------------------------------------

StreamConnectionList MailFolderCC::ms_StreamList(FALSE);

bool MailFolderCC::ms_CClientInitialisedFlag = false;

MailFolderCC *MailFolderCC::ms_StreamListDefaultObj = NULL;

/// adds this object to Map
void
MailFolderCC::AddToMap(MAILSTREAM const *stream) const
{
   wxLogTrace(TRACE_MF_CACHE, "MailFolderCC::AddToMap() for folder %s",
              GetName().c_str());

   CHECK_RET( stream, "Attempting to AddToMap() a NULL MAILSTREAM" );

   CHECK_STREAM_LIST();

#ifdef DEBUG
   // check that the folder is not already in the list
   StreamConnectionList::iterator i;
   for ( i = ms_StreamList.begin(); i != ms_StreamList.end(); i++ )
   {
      StreamConnection *conn = *i;
      if ( conn->folder == this )
      {
         FAIL_MSG("adding a folder to the stream list twice");

         return;
      }
   }
#endif // DEBUG

   // add it now
   StreamConnection  *conn = new StreamConnection;
   conn->folder = (MailFolderCC *) this;
   conn->stream = stream;
   conn->name = m_ImapSpec;
   conn->login = m_Login;
   ms_StreamList.push_front(conn);

   CHECK_STREAM_LIST();
}

/* static */
bool
MailFolderCC::CanExit(String *which)
{
   CHECK_STREAM_LIST();

   which->clear();

   bool canExit = true;
   StreamConnectionList::iterator i;
   for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
   {
      if( (*i)->folder->InCritical() )
      {
         canExit = false;
         *which << (*i)->folder->GetName() << '\n';
      }
   }

   return canExit;
}

// it's ok to return NULL from this function
/* static */
MailFolderCC *MailFolderCC::LookupStream(const MAILSTREAM *stream)
{
   CHECK_STREAM_LIST();

   StreamConnectionList::iterator i;
   for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
   {
      StreamConnection *conn = *i;
      if ( conn->stream == stream )
         return conn->folder;
   }

   return NULL;
}

// this function should normally always return a non NULL folder
/* static */
MailFolderCC *
MailFolderCC::LookupObject(MAILSTREAM const *stream, const char *name)
{
   MailFolderCC *mf = LookupStream(stream);
   if ( mf )
      return mf;

   /* Sometimes the IMAP code (imap4r1.c) allocates a temporary stream
      for e.g. mail_status(), that is difficult to handle here, we
      must compare the name parameter which might not be 100%
      identical, but we can check the hostname for identity and the
      path. However, that seems a bit far-fetched for now, so I just
      make sure that ms_StreamListDefaultObj gets set before any call to
      mail_status(). If that doesn't work, we need to parse the name
      string for hostname, portnumber and path and compare these.

      FIXME: This might be an issue for MT code.
   */
   if(name)
   {
      StreamConnectionList::iterator i;
      for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
      {
         StreamConnection *conn = *i;
         if( conn->name == name )
            return conn->folder;
      }
   }

   if(ms_StreamListDefaultObj)
   {
      wxLogTrace(TRACE_MF_CACHE, "Routing call to default mailfolder.");
      return ms_StreamListDefaultObj;
   }

   FAIL_MSG("Cannot find mailbox for callback!");

   return NULL;
}

/* static */
MailFolderCC *
MailFolderCC::FindFolder(const MFolder* folder)
{
   String login = folder->GetLogin();
   String mboxpath = MailFolder::GetImapSpec
                     (
                      folder->GetType(),
                      folder->GetFlags(),
                      folder->GetPath(),
                      folder->GetServer(),
                      login
                     );

   return FindFolder(mboxpath, login);
}

/* static */
MailFolderCC *
MailFolderCC::FindFolder(String const &path, String const &login)
{
   CHECK_STREAM_LIST();

   StreamConnectionList::iterator i;
   wxLogTrace(TRACE_MF_CACHE, "Looking for folder to re-use: '%s',login '%s'",
              path.c_str(), login.c_str());

   for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
   {
      StreamConnection *conn = *i;
      if( conn->name == path && conn->login == login)
      {
         wxLogTrace(TRACE_MF_CACHE, "  Re-using entry: '%s',login '%s'",
                    conn->name.c_str(), conn->login.c_str());

         MailFolderCC *mf = conn->folder;
         mf->IncRef();

         return mf;
      }
   }

   wxLogTrace(TRACE_MF_CACHE, "  No matching entry found.");

   return NULL;
}

/// remove this object from Map
void
MailFolderCC::RemoveFromMap(void) const
{
   wxLogTrace(TRACE_MF_CACHE, "MailFolderCC::RemoveFromMap() for folder %s",
              GetName().c_str());
   CHECK_STREAM_LIST();

   StreamConnectionList::iterator i;
   for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
   {
      if( (*i)->folder == this )
      {
         StreamConnection *conn = *i;
         delete conn;
         ms_StreamList.erase(i);

         break;
      }
   }

   CHECK_STREAM_LIST();

   if( ms_StreamListDefaultObj == this )
   {
      // no more
      ms_StreamListDefaultObj = NULL;
   }
}


/// Gets first mailfolder in map.
/* static */
MailFolderCC *
MailFolderCC::GetFirstMapEntry(StreamConnectionList::iterator &i)
{
   CHECK_STREAM_LIST();

   i = ms_StreamList.begin();
   if( i != ms_StreamList.end())
      return (**i).folder;
   else
      return NULL;
}

/// Gets next mailfolder in map.
/* static */
MailFolderCC *
MailFolderCC::GetNextMapEntry(StreamConnectionList::iterator &i)
{
   CHECK_STREAM_LIST();

   ASSERT_MSG( i != ms_StreamList.end(), "no next entry in the map" );
#ifdef DEBUG
   wxLogTrace(TRACE_MF_CACHE, "GetNextMapEntry before: %s", i.Debug().c_str());
#endif
   i++;
#ifdef DEBUG
   wxLogTrace(TRACE_MF_CACHE, "GetNextMapEntry after: %s", i.Debug().c_str());
#endif
   if( i != ms_StreamList.end())
      return (**i).folder;
   else
      return NULL;
}

// ----------------------------------------------------------------------------
// MailFolderCC options
// ----------------------------------------------------------------------------

void MailFolderCC::ReadConfig(MailFolderCmn::MFCmnOptions& config)
{
   mm_show_debug = READ_APPCONFIG_BOOL(MP_DEBUG_CCLIENT);

   MailFolderCmn::ReadConfig(config);
}

// ----------------------------------------------------------------------------
// MailFolderCC pinging
// ----------------------------------------------------------------------------

void
MailFolderCC::Checkpoint(void)
{
   if ( m_MailStream == NULL )
     return; // nothing we can do anymore

   // nothing to do for halfopened folders (and doing CHECK on a half opened
   // IMAP folder is a protocol error, too)
   if ( m_MailStream->halfopen )
      return;

   // an MBOX file created by wxMessageView::MimeHandle() is deleted
   // immediately afterwards, so don't try to access it from here
   if ( GetType() == MF_FILE )
   {
      if ( !wxFile::Exists(m_MailStream->mailbox) )
      {
         DBGMESSAGE(("MBOX folder '%s' already deleted.", m_MailStream->mailbox));
         return;
      }
   }

   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC::Checkpoint() on %s.",
              GetName().c_str());

   if ( NeedsNetwork() && ! mApplication->IsOnline() )
   {
      ERRORMESSAGE((_("System is offline, cannot access mailbox ´%s´"),
                   GetName().c_str()));
      return;
   }

   MailFolderLocker lock(this);
   if ( lock )
   {
      mail_check(m_MailStream); // update flags, etc, .newsrc
   }
}

bool
MailFolderCC::PingReopen(void)
{
   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC::PingReopen() on Folder %s.",
              GetName().c_str());

   bool rc = false;

   MailFolderLocker lockFolder(this);
   if ( lockFolder )
   {
      rc = true;

      // This is terribly inefficient to do, but needed for some sick
      // POP3 servers which don't report new mail otherwise
      if( (GetFlags() & MF_FLAGS_REOPENONPING)
          // c-client 4.7-bug: MH folders don't immediately notice new
          // messages:
          || GetType() == MF_MH )
      {
         wxLogTrace(TRACE_MF_CALLS,
                    "MailFolderCC::Ping() forcing close on folder %s.",
                    GetName().c_str());

         // FIXME!!!!
         Close();
      }

      if( NeedsNetwork() && ! mApplication->IsOnline() &&
          !MDialog_YesNoDialog
           (
            wxString::Format
            (
               _("Dial-Up network is down.\n"
                 "Do you want to try and check folder '%s' anyway?"),
               GetName().c_str()
            ),
            NULL,
            MDIALOG_YESNOTITLE,
            false,
            GetName()+"/NoNetPingAnyway")
        )
      {
         Close(); // now folder is dead

         return false;
      }

      if ( !m_MailStream || !mail_ping(m_MailStream) )
      {
         RemoveFromMap(); // will be added again by Open()
         MFrame *frame = GetInteractiveFrame();
         if ( frame )
         {
            STATUSMESSAGE((frame,
                           _("Mailstream for folder '%s' has been closed, trying to reopen it."),
                           GetName().c_str()));
         }

         rc = Open();
         if( !rc )
         {
            ERRORMESSAGE((_("Re-opening closed folder '%s' failed."), GetName().c_str()));
            m_MailStream = NIL;
         }
      }

      if ( m_MailStream )
      {
         rc = true;
         wxLogTrace(TRACE_MF_CALLS, "Folder '%s' is alive.", GetName().c_str());
      }
   }

   return rc;
}

/* static */ bool
MailFolderCC::PingReopenAll(bool fullPing)
{
   // try to reopen all streams
   bool rc = true;

   CHECK_STREAM_LIST();

   /*
      Ping() might close/reopen a mailfolder, which means that some folder we
      had already pinged might be readded to the list, so we have to make a
      copy of the list before iterating
    */
   size_t count = ms_StreamList.size();
   StreamConnection **connections = new StreamConnection *[count];

   size_t n = 0;
   for ( StreamConnectionList::iterator i = ms_StreamList.begin();
         i != ms_StreamList.end();
         i++ )
   {
      // skip half opened streams, we can't ping them
      if ( (*i)->stream && !(*i)->stream->halfopen )
      {
         connections[n++] = *i;
      }
   }

   // we can have fewer of them because of half opened streams
   count = n;
   for ( n = 0; n < count; n++ )
   {
      MailFolderCC *mf = connections[n]->folder;

      // don't ping locked folders, they will have to wait for the next time
      if ( !mf->IsLocked() )
      {
         wxLogTrace("collect",
                    "Checking for new mail in folder '%s'...",
                    mf->GetName().c_str());

         if ( !(fullPing ? mf->PingReopen() : mf->Ping()) )
         {
            // failed to ping at least one folder
            rc = false;
         }
      }
   }

   delete [] connections;

   return rc;
}

bool
MailFolderCC::Ping(void)
{
   if ( NeedsNetwork() && ! mApplication->IsOnline() )
   {
      ERRORMESSAGE((_("System is offline, cannot access mailbox ´%s´"), GetName().c_str()));
      return FALSE;
   }

   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC::Ping() on Folder %s.",
              GetName().c_str());

   // we don't want to reopen the folder from here, this leads to inifinite
   // loops if the network connection goes down because we are called from a
   // timer event and so if folder pinging taks too long, we will be called
   // again and again and again ... before we get the chance to do anything
   return m_MailStream ? PingReopen() : true;
}

// ----------------------------------------------------------------------------
// MailFolderCC locking
// ----------------------------------------------------------------------------

bool
MailFolderCC::Lock(void) const
{
   m_Mutex->Lock();

   return true;
}

bool
MailFolderCC::IsLocked(void) const
{
   return m_Mutex->IsLocked() || m_InFilterCode->IsLocked();
}

void
MailFolderCC::UnLock(void) const
{
   m_Mutex->Unlock();
}

// ----------------------------------------------------------------------------
// Adding and removing (expunging) messages
// ----------------------------------------------------------------------------

// this is the common part of both AppendMessage() functions
void
MailFolderCC::UpdateAfterAppend()
{
   // this mail will be stored as uid_last+1 which isn't updated
   // yet at this point
   m_LastNewMsgUId = m_MailStream->uid_last + 1;

   // don't update if we don't have listing - it will be rebuilt the next time
   // it is needed
   if ( !IsUpdateSuspended() )
   {
      // make the folder notice the new messages
      mail_ping(m_MailStream);
   }
}

bool
MailFolderCC::AppendMessage(const String& msg)
{
   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC(%s)::AppendMessage(string)",
              GetName().c_str());

   CHECK_DEAD_RC("Appending to closed folder '%s' failed.", false);

   STRING str;
   INIT(&str, mail_string, (void *) msg.c_str(), msg.Length());

   if ( !mail_append(m_MailStream, (char *)m_ImapSpec.c_str(), &str) )
   {
      wxLogError(_("Failed to save message to the folder '%s'"),
                 GetName().c_str());

      return false;
   }

   UpdateAfterAppend();

   return true;
}

bool
MailFolderCC::AppendMessage(const Message& msg)
{
   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC(%s)::AppendMessage(Message)",
              GetName().c_str());

   CHECK_DEAD_RC("Appending to closed folder '%s' failed.", false);

   String date;
   msg.GetHeaderLine("Date", date);

   char *dateptr = NULL;
   char datebuf[128];
   MESSAGECACHE mc;
   if ( mail_parse_date(&mc, (char *) date.c_str()) == T )
   {
     mail_date(datebuf, &mc);
     dateptr = datebuf;
   }

   // append message text to folder
   String tmp;
   if ( !msg.WriteToString(tmp) )
   {
      wxLogError(_("Failed to retrieve the message text."));

      return false;
   }

   String flags = GetImapFlags(msg.GetStatus());

   STRING str;
   INIT(&str, mail_string, (void *) tmp.c_str(), tmp.Length());
   if ( !mail_append_full(m_MailStream,
                          (char *)m_ImapSpec.c_str(),
                          (char *)flags.c_str(),
                          dateptr,
                          &str) )
   {
      // useful to know which message exactly we failed to copy
      wxLogError(_("Message details: subject '%s', from '%s'"),
                 msg.Subject().c_str(), msg.From().c_str());

      wxLogError(_("Failed to save message to the folder '%s'"),
                 GetName().c_str());

      return false;
   }

   UpdateAfterAppend();

   return true;
}

bool
MailFolderCC::SaveMessages(const UIdArray *selections, MFolder *folder)
{
   CHECK( folder, false, "SaveMessages() needs a valid folder pointer" );

   size_t count = selections->Count();
   CHECK( count, true, "SaveMessages(): nothing to save" );

   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC(%s)::SaveMessages()",
              GetName().c_str());

   /*
      This is an optimisation: if both mailfolders are IMAP and on the same
      server, we ask the server to copy the message, which is more efficient
      (think of a modem!).

      If that is not the case, or it fails, then we fall back to the base class
      version which simply retrieves the message and appends it to the second
      folder.
   */
   bool didServerSideCopy = false;
   if ( GetType() == MF_IMAP && folder->GetType() == MF_IMAP )
   {
      // they're both IMAP but do they live on the same server?
      // check if host, protocol, user all identical:
      String specDst = ::GetImapSpec(folder);
      String serverSrc = GetFirstPartFromImapSpec(specDst);
      String serverDst = GetFirstPartFromImapSpec(GetImapSpec());

      if ( serverSrc == serverDst )
      {
         // before trying to copy messages to this folder, create it if hadn't
         // been done yet
         if ( CreateIfNeeded(folder) )
         {
            String sequence = BuildSequence(*selections);
            String pathDst = GetPathFromImapSpec(specDst);
            if ( mail_copy_full(m_MailStream,
                                (char *)sequence.c_str(),
                                (char *)pathDst.c_str(),
                                CP_UID) )
            {
               didServerSideCopy = true;
            }
            else
            {
               // don't give an error as it is not fatal and there is no way to
               // disable it, but still log it
               wxLogMessage(_("Server side copy from '%s' to '%s' failed, "
                              "trying inefficient append now."),
                            GetName().c_str(),
                            folder->GetName().c_str());
            }
         }
      }
   }

   if ( !didServerSideCopy )
   {
      // use the inefficient retrieve-append way
      return MailFolderCmn::SaveMessages(selections, folder);
   }

   String nameDst = folder->GetFullName();

   // copying to ourselves?
   if ( nameDst == GetName() )
   {
      // yes, no need to update status as it will happen anyhow
      return true;
   }

   // update status of the target folder
   MfStatusCache *mfStatusCache = MfStatusCache::Get();
   MailFolderStatus status;
   if ( !mfStatusCache->GetStatus(nameDst, &status) )
   {
      // assume it was empty... this is, of course, false, but it allows us to
      // show the folder as having unseen/new/... messages in the tree without
      // having to open it (slow!) and so this hack is well worth it
      status.total = 0;
   }

   // examine flags of all messages we copied
   HeaderInfoList_obj headers = GetHeaders();
   for ( size_t n = 0; n < count; n++ )
   {
      MsgnoType msgno = GetMsgnoFromUID(selections->Item(n));
      if ( msgno == MSGNO_ILLEGAL )
      {
         FAIL_MSG("inexistent message was copied??");

         continue;
      }

      size_t idx = headers->GetIdxFromMsgno(msgno);
      HeaderInfo *hi = headers->GetItemByIndex(idx);
      if ( !hi )
      {
         FAIL_MSG( "UpdateMessageStatus: no header info for the given msgno?" );

         continue;
      }

      int flags = hi->GetStatus();

      // deleted messages don't count, except for the total number
      if ( !(flags & MailFolder::MSG_STAT_DELETED) )
      {
         bool isRecent = (flags & MailFolder::MSG_STAT_RECENT) != 0;

         if ( !(flags & MailFolder::MSG_STAT_SEEN) )
         {
            if ( isRecent )
               status.newmsgs++;

            status.unread++;
         }
         else if ( isRecent )
         {
            status.recent++;
         }

         if ( flags & MailFolder::MSG_STAT_FLAGGED )
            status.flagged++;
         if ( flags & MailFolder::MSG_STAT_SEARCHED )
            status.searched++;
      }

      status.total++;
   }

   // always send this one to update the number of messages in the tree
   mfStatusCache->UpdateStatus(nameDst, status);

   // if the folder is opened, we must also update the display
   MailFolder *mfDst = FindFolder(folder);
   if ( mfDst )
   {
      mfDst->Ping();
      mfDst->DecRef();
   }
   //else: not opened

   return true;
}

void
MailFolderCC::ExpungeMessages(void)
{
   CHECK_DEAD("Cannot expunge messages from closed folder '%s'.");

   if ( PY_CALLBACK(MCB_FOLDEREXPUNGE,1,GetProfile()) )
   {
      wxLogTrace(TRACE_MF_CALLS, "MailFolderCC(%s)::ExpungeMessages()",
                 GetName().c_str());

      mail_expunge(m_MailStream);

      // for some types of folders (IMAP) mm_exists() is called from
      // mail_expunge() but for the others (POP) it isn't and we have to call
      // it ourselves: check is based on the fact that m_expungedMsgnos is
      // reset in mm_exists handler
      if ( m_expungedMsgnos )
      {
         RequestUpdateAfterExpunge();
      }
   }
}

// ----------------------------------------------------------------------------
// Counting messages
// ----------------------------------------------------------------------------

unsigned long MailFolderCC::GetMessageCount() const
{
   CHECK( m_MailStream, 0, "GetMessageCount: folder is closed" );

   return m_MailStream->nmsgs;
}

unsigned long MailFolderCC::CountNewMessages() const
{
   MsgnoArray *messages = SearchByFlag(MSG_STAT_NEW);
   if ( !messages )
      return 0;

   unsigned long newmsgs = messages->GetCount();
   delete messages;

   return newmsgs;
}

unsigned long MailFolderCC::CountRecentMessages() const
{
   CHECK( m_MailStream, 0, "CountRecentMessages: folder is closed" );

   return m_MailStream->recent;
}

unsigned long MailFolderCC::CountUnseenMessages() const
{
   // we can't use mail_status here because the mailbox is opened and IMAP
   // doesn't like STATUS on SELECTed mailbox

   SEARCHPGM *pgm = mail_newsearchpgm();
   pgm->unseen = 1;
   pgm->undeleted = 1;

   return SearchAndCountResults(pgm);
}

unsigned long MailFolderCC::CountDeletedMessages() const
{
   SEARCHPGM *pgm = mail_newsearchpgm();
   pgm->deleted = 1;

   return SearchAndCountResults(pgm);
}

bool MailFolderCC::DoCountMessages(MailFolderStatus *status) const
{
   CHECK( status, false, "DoCountMessages: NULL pointer" );

   CHECK( m_MailStream, false, "DoCountMessages: folder is closed" );

   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC(%s)::DoCountMessages()",
              GetName().c_str());

   status->Init();

   status->total = m_MailStream->nmsgs;
   status->recent = m_MailStream->recent;

   status->unread = CountUnseenMessages();

   if ( status->recent || status->unread )
   {
      status->newmsgs = CountNewMessages();
   }
   else // no recent or no unseen
   {
      // hence no new
      status->newmsgs = 0;
   }

   // then get the number of flagged messages
   SEARCHPGM *pgm = mail_newsearchpgm();
   pgm->flagged = 1;
   pgm->undeleted = 1;

   status->flagged = SearchAndCountResults(pgm);

   // TODO: get the number of searched ones
   status->searched = 0;

   return true;
}

// ----------------------------------------------------------------------------
// MailFolderCC accessing the messages
// ----------------------------------------------------------------------------

MsgnoType
MailFolderCC::GetMsgnoFromUID(UIdType uid) const
{
   // garbage in, garbage out
   CHECK( uid != UID_ILLEGAL, MSGNO_ILLEGAL, "GetMsgnoFromUID: bad uid" );

   CHECK( m_MailStream, MSGNO_ILLEGAL, "GetMsgnoFromUID: folder closed" );

   // mail_msgno() is a slow function because it iterates over entire c-client
   // internal cache but I don't see how to make it faster
   return mail_msgno(m_MailStream, uid);
}

Message *
MailFolderCC::GetMessage(unsigned long uid)
{
   CHECK_DEAD_RC("Cannot access closed folder '%s'.", NULL);

   HeaderInfoList_obj headers = GetHeaders();
   CHECK( headers, NULL, "GetMessage: failed to get headers" );

   UIdType idx = headers->GetIdxFromUId(uid);
   CHECK( idx != UID_ILLEGAL, NULL, "GetMessage: no UID with this message" );

   HeaderInfo *hi = headers->GetItemByIndex((size_t)idx);
   CHECK( hi, NULL, "invalid UID in GetMessage" );

   return MessageCC::Create(this, *hi);
}

String
MailFolderCC::GetMessageUID(unsigned long msgno) const
{
   CHECK( m_MailStream, "", "GetMessageUID(): folder is closed" );

   String uidString;

   if ( GetType() == MF_POP )
   {
      // no real UID in the IMAP sense of the word, but we can use POP3 UIDL
      // instead
      if ( pop3_send_num(m_MailStream, "UIDL", msgno) )
      {
         char *reply = ((POP3LOCAL *)m_MailStream->local)->reply;
         unsigned long msgnoFromUIDL;

         // RFC says 70 characters max, but avoid buffer overflows just in case
         char *buf = new char[strlen(reply) + 1];
         if ( (sscanf(reply, "%lu %s", &msgnoFromUIDL, buf) != 2) ||
              (msgnoFromUIDL != msgno) )
         {
            // something is wrong
            wxLogDebug("Unexpected POP3 server UIDL response to UIDL %lu: %s",
                       msgno, reply);
         }
         else // parsed UIDL successfully
         {
            uidString = buf;
         }

         delete [] buf;
      }
      //else: failed to get the UIDL from POP3 server
   }
   else // try to get the real UID for all other folders
   {
      unsigned long uid = mail_uid(m_MailStream, msgno);
      if ( uid )
      {
         // ok, got the real thing
         uidString = strutil_ultoa(uid);
      }
      //else: no way to get an UID, leave uidString empty
   }

   return uidString;
}

// ----------------------------------------------------------------------------
// MailFolderCC: searching
// ----------------------------------------------------------------------------

MsgnoArray *MailFolderCC::DoSearch(struct search_program *pgm) const
{
   CHECK( m_MailStream, NULL, "SearchAndCountResults: folder is closed" );

   // at best we're going to have a memory leak, at worse c-client is locked
   // and we will just crash
   ASSERT_MSG( !m_SearchMessagesFound, "MailFolderCC::DoSearch() reentrancy" );

   MailFolderCC *self = (MailFolderCC *)this; // const_cast
   self->m_SearchMessagesFound = new UIdArray;

   mail_search_full(m_MailStream, NIL, pgm, SE_FREE | SE_NOPREFETCH);

   CHECK( m_SearchMessagesFound, NULL, "who deleted m_SearchMessagesFound?" );

   MsgnoArray *searchMessagesFound = m_SearchMessagesFound;
   self->m_SearchMessagesFound = NULL;

   return searchMessagesFound;
}

unsigned long
MailFolderCC::SearchAndCountResults(struct search_program *pgm) const
{
   MsgnoArray *searchResults = DoSearch(pgm);

   unsigned long count;
   if ( searchResults )
   {
      count = searchResults->Count();

      delete searchResults;
   }
   else
   {
      count = 0;
   }

   return count;
}

MsgnoArray *MailFolderCC::SearchByFlag(MessageStatus flag,
                                       bool set,
                                       bool undeletedOnly) const
{
   CHECK( m_MailStream, 0, "SearchByFlag: folder is closed" );

   SEARCHPGM *pgm = mail_newsearchpgm();

   switch ( flag )
   {
      case MSG_STAT_SEEN:
         if ( set )
            pgm->seen = 1;
         else
            pgm->unseen = 1;
         break;

      case MSG_STAT_DELETED:
         if ( set )
         {
            // we don't risk finding many messages like this!
            ASSERT_MSG( !undeletedOnly, "deleted and undeleted at once?" );

            pgm->deleted = 1;
         }
         else
            pgm->undeleted = 1;
         break;

      case MSG_STAT_ANSWERED:
         if ( set )
            pgm->answered = 1;
         else
            pgm->unanswered = 1;
         break;

      case MSG_STAT_RECENT:
         if ( set )
            pgm->recent = 1;
         else
            pgm->old = 1;
         break;

      case MSG_STAT_FLAGGED:
         if ( set )
            pgm->flagged = 1;
         else
            pgm->unflagged = 1;
         break;

      case MSG_STAT_NEW:
         if ( set )
         {
            pgm->recent = 1;
            pgm->unseen = 1;
         }
         else
         {
            // although it is formally possible, this must be an error
            FAIL_MSG( "do you really want to search for unnew messages??" );
         }
         break;

      case MSG_STAT_SEARCHED: // unsupported and unneeded
      default:
         FAIL_MSG( "unexpected flag in SearchByFlag" );

         mail_free_searchpgm(&pgm);

         return NULL;
   }

   if ( undeletedOnly )
   {
      pgm->undeleted = 1;
   }

   return DoSearch(pgm);
}

UIdArray *
MailFolderCC::SearchMessages(const SearchCriterium *crit)
{
   CHECK( !m_SearchMessagesFound, NULL, "SearchMessages reentered" );
   CHECK( crit, NULL, "no criterium in SearchMessages" );

   HeaderInfoList_obj hil = GetHeaders();
   CHECK( hil, NULL, "no listing in SearchMessages" );

   m_SearchMessagesFound = new UIdArray;

   // server side searching doesn't support all possible search criteria,
   // check if it can do this search first

   SEARCHPGM *pgm = mail_newsearchpgm();
   STRINGLIST **slistMatch;

   switch ( crit->m_What )
   {
      case SearchCriterium::SC_FULL:
         slistMatch = &pgm->text;
         break;

      case SearchCriterium::SC_BODY:
         slistMatch = &pgm->body;
         break;

      case SearchCriterium::SC_SUBJECT:
         slistMatch = &pgm->subject;
         break;

      case SearchCriterium::SC_TO:
         slistMatch = &pgm->to;
         break;

      case SearchCriterium::SC_FROM:
         slistMatch = &pgm->from;
         break;

      case SearchCriterium::SC_CC:
         slistMatch = &pgm->cc;
         break;

      default:
         slistMatch = NULL;
         mail_free_searchpgm(&pgm);
   }

   if ( slistMatch )
   {
      *slistMatch = mail_newstringlist();

      (*slistMatch)->text.data = (unsigned char *)strdup(crit->m_Key);
      (*slistMatch)->text.size = crit->m_Key.length();

      if ( crit->m_Invert )
      {
         // represent the inverted search as "TRUE && !pgm"
         SEARCHPGM *pgmReal = pgm;

         pgm = mail_newsearchpgm();
         pgm->msgno = mail_newsearchset();
         pgm->msgno->first = 1;
         pgm->msgno->last = GetMessageCount();

         pgm->cc_not = mail_newsearchpgmlist();
         pgm->cc_not->pgm = pgmReal;
      }

      // the m_SearchMessagesFound array is filled from mm_searched
      mail_search_full (m_MailStream,
                        NIL /* charset: use default (US-ASCII) */,
                        pgm,
                        SE_UID | SE_FREE | SE_NOPREFETCH);
   }
   else // can't do server side search
   {
      // how many did we find?
      unsigned long countFound = 0;

      MProgressDialog *progDlg = NULL;

      MsgnoType nMessages = GetMessageCount();
      if ( nMessages > (unsigned long)READ_CONFIG(m_Profile,
                                                  MP_FOLDERPROGRESS_THRESHOLD) )
      {
         String msg;
         msg.Printf(_("Searching in %lu messages..."), nMessages);
         {
            MGuiLocker locker;
            progDlg = new MProgressDialog(GetName(),
                                          msg,
                                          nMessages,
                                          NULL,
                                          false, true);// open a status window:
         }
      }

      String what;
      for ( size_t idx = 0; idx < nMessages; idx++ )
      {
         HeaderInfo *hi = hil->GetItemByIndex(idx);

         if ( !hi )
         {
            FAIL_MSG( "SearchMessages: can't get header info" );

            continue;
         }

         if ( crit->m_What == SearchCriterium::SC_SUBJECT )
         {
            what = hi->GetSubject();
         }
         else if ( crit->m_What == SearchCriterium::SC_FROM )
         {
            what = hi->GetFrom();
         }
         else if ( crit->m_What == SearchCriterium::SC_TO )
         {
            what = hi->GetTo();
         }
         else
         {
            Message *msg = GetMessage(hi->GetUId());
            switch ( crit->m_What )
            {
               case SearchCriterium::SC_FULL:
               case SearchCriterium::SC_BODY:
                  // FIXME: wrong for body as it checks the whole message
                  // including header
                  what = msg->FetchText();
                  break;

               case SearchCriterium::SC_HEADER:
                  what = msg->GetHeader();
                  break;

               case SearchCriterium::SC_CC:
                  msg->GetHeaderLine("CC", what);
                  break;

               default:
                  FAIL_MSG("Unknown search criterium!");
            }

            msg->DecRef();
         }

         bool found = what.Contains(crit->m_Key);
         if(crit->m_Invert)
            found = !found;
         if(found)
            m_SearchMessagesFound->Add(hil->GetItemByIndex(idx)->GetUId());

         if(progDlg)
         {
            String msg;
            msg.Printf(_("Searching in %lu messages..."), nMessages);
            String msg2;
            unsigned long cnt = m_SearchMessagesFound->Count();
            if(cnt != countFound)
            {
               msg2.Printf(_(" - %lu matches found."), cnt);
               msg = msg + msg2;
               if ( ! progDlg->Update(idx, msg ) )
               {
                  // abort searching
                  break;
               }

               countFound = cnt;
            }
            else if( !progDlg->Update(idx) )
            {
               // abort searching
               break;
            }
         }
      }

      delete progDlg;
   }

   UIdArray *rc = m_SearchMessagesFound; // will get freed by caller!
   m_SearchMessagesFound = NULL;

   return rc;
}

// ----------------------------------------------------------------------------
// Message flags
// ----------------------------------------------------------------------------

/* static */
String MailFolderCC::BuildSequence(const UIdArray& messages)
{
   Sequence seq;

   size_t count = messages.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      seq.Add(messages[n]);
   }

   return seq.GetString();
}

bool
MailFolderCC::SetSequenceFlag(const String& sequence,
                              int flag,
                              bool set)
{
   return DoSetSequenceFlag(SEQ_UID, sequence, flag, set);
}

bool
MailFolderCC::DoSetSequenceFlag(SequenceKind kind,
                                const String& sequence,
                                int flag,
                                bool set)
{
   CHECK_DEAD_RC("Cannot access closed folder '%s'.", false);
   String flags = GetImapFlags(flag);

   if(PY_CALLBACKVA((set ? MCB_FOLDERSETMSGFLAG : MCB_FOLDERCLEARMSGFLAG,
                     1, this, this->GetClassName(),
                     GetProfile(), "ss", sequence.c_str(), flags.c_str()),1)  )
   {
      int opFlags = 0;
      if ( set )
         opFlags |= ST_SET;
      if ( kind == SEQ_UID )
         opFlags |= ST_UID;

      mail_flag(m_MailStream,
                (char *)sequence.c_str(),
                (char *)flags.c_str(),
                opFlags);
   }
   //else: blocked by python callback

   return true;
}

bool
MailFolderCC::SetFlag(const UIdArray *selections, int flag, bool set)
{
   return SetSequenceFlag(BuildSequence(*selections), flag, set);
}

bool
MailFolderCC::SetFlagForAll(int flag, bool set)
{
   if ( !m_nMessages )
   {
      // no messages to set the flag for
      return true;
   }

   Sequence sequence;
   sequence.AddRange(1, m_nMessages);

   return DoSetSequenceFlag(SEQ_MSGNO, sequence.GetString(), flag, set);
}

bool
MailFolderCC::IsNewMessage(const HeaderInfo *hi)
{
   UIdType msgId = hi->GetUId();
   bool isNew = false;
   int status = hi->GetStatus();

   if( (status & MSG_STAT_SEEN) == 0
       && ( status & MSG_STAT_RECENT)
       && ! ( status & MSG_STAT_DELETED) )
      isNew = true;
   if(m_LastNewMsgUId != UID_ILLEGAL
      && m_LastNewMsgUId >= msgId)
      isNew = false;

   return isNew;
}

void
MailFolderCC::OnMsgStatusChanged()
{
   // we only send MEventId_MailFolder_OnMsgStatus events if something really
   // changed, so this is not suposed to happen
   CHECK_RET( m_statusChangedMsgnos, "unexpected OnMsgStatusChanged() call" );

   // first update the cached status
   MailFolderStatus status;
   MfStatusCache *mfStatusCache = MfStatusCache::Get();
   if ( mfStatusCache->GetStatus(GetName(), &status) )
   {
      size_t count = m_statusChangedMsgnos->GetCount();
      for ( size_t n = 0; n < count; n++ )
      {
         int statusNew = m_statusChangedNew->Item(n),
             statusOld = m_statusChangedOld->Item(n);

         bool wasDeleted = (statusOld & MSG_STAT_DELETED) != 0,
              isDeleted = (statusNew & MSG_STAT_DELETED) != 0;

         MsgStatus msgStatusOld = AnalyzeStatus(statusOld),
                   msgStatusNew = AnalyzeStatus(statusNew);

         // we consider that a message has some flag only if it is not deleted
         // (which is discussable at least for flagged and searched flags
         // although, OTOH, why flag a deleted message?)

         #define UPDATE_NUM_OF(what)   \
            if ( (!isDeleted && msgStatusNew.what) && \
                 (wasDeleted || !msgStatusOld.what) ) \
               status.what++; \
            else if ( (!wasDeleted && msgStatusOld.what) && \
                      (isDeleted || !msgStatusNew.what) ) \
               if ( status.what > 0 ) \
                  status.what--; \
               else \
                  FAIL_MSG( "error in msg status change logic" )

         UPDATE_NUM_OF(recent);
         UPDATE_NUM_OF(unread);
         UPDATE_NUM_OF(newmsgs);
         UPDATE_NUM_OF(flagged);
         UPDATE_NUM_OF(searched);

         #undef UPDATE_NUM_OF
      }

      mfStatusCache->UpdateStatus(GetName(), status);
   }

   // next notify everyone else about the status change
   wxLogTrace(TRACE_MF_EVENTS,
              "Sending MsgStatus event for %u msgs in folder '%s'",
              m_statusChangedMsgnos->GetCount(), GetName().c_str());

   MEventManager::Send(new MEventMsgStatusData(this,
                                               m_statusChangedMsgnos,
                                               m_statusChangedOld,
                                               m_statusChangedNew));

   // MEventMsgStatusData will delete them
   m_statusChangedMsgnos =
   m_statusChangedOld =
   m_statusChangedNew = NULL;
}

void
MailFolderCC::UpdateMessageStatus(unsigned long msgno)
{
   // if we're retrieving the headers right now, we can't use
   // headers->GetItemByIndex() below as this risks to reenter c-client which
   // is a fatal error
   //
   // besides, we don't need to do it neither as mm_exists() will follow soon
   // which will invalidate the current status anyhow
   if ( IsLocked() )
      return;

   MESSAGECACHE *elt = mail_elt(m_MailStream, msgno);
   CHECK_RET( elt, "UpdateMessageStatus: no elt for the given msgno?" );

   HeaderInfoList_obj headers = GetHeaders();
   CHECK_RET( headers, "UpdateMessageStatus: couldn't get headers" );

   HeaderInfo *hi = headers->GetItemByMsgno(msgno);
   CHECK_RET( hi, "UpdateMessageStatus: no header info for the given msgno?" );

   int statusNew = GetMsgStatus(elt),
       statusOld = hi->GetStatus();
   if ( statusNew != statusOld )
   {
      hi->m_Status = statusNew;

      // we send the event telling us that we have some messages with the
      // changed status only once, when we get the first notification - and
      // also when we allocate the arrays for msg status change data
      bool sendEvent;
      if ( !m_statusChangedMsgnos )
      {
         m_statusChangedMsgnos = new wxArrayInt;
         m_statusChangedOld = new wxArrayInt;
         m_statusChangedNew = new wxArrayInt;

         sendEvent = true;
      }
      else
      {
         sendEvent = false;
      }

      // remember who changed and how
      m_statusChangedMsgnos->Add(msgno);
      m_statusChangedOld->Add(statusOld);
      m_statusChangedNew->Add(statusNew);

      // and schedule call to our OnMsgStatusChanged() if not done yet
      if ( sendEvent )
      {
         MEventManager::Send(new MEventFolderOnMsgStatusData(this));
      }
   }
   //else: flags didn't really change
}

// ----------------------------------------------------------------------------
// Debug only helpers
// ----------------------------------------------------------------------------

#ifdef DEBUG

StreamConnection::~StreamConnection()
{
    wxLogTrace(TRACE_MF_CACHE, "deleting %s from stream list", name.c_str());
}

void
MailFolderCC::Debug(void) const
{
   DebugStreams();
}

void
MailFolderCC::DebugStreams(void)
{
   // this produces a *lot* of output, so enable it separately as needed
#ifdef DEBUG_STREAMS
   wxLogDebug("--list of streams and objects--");

   StreamConnection *conn;
   StreamConnectionList::iterator i;

   for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
   {
      conn = *i;

      if ( !conn )
      {
         FAIL_MSG( "NULL stream in the list" );
         continue;
      }

      wxLogDebug("\t%p -> %p \"%s\"",
                 conn->stream, conn->folder, conn->folder->GetName().c_str());
   }
   wxLogDebug("--end of list--");
#endif // DEBUG_STREAMS
}

String
MailFolderCC::DebugDump() const
{
   String str = MObjectRC::DebugDump();
   str << "mailbox '" << m_ImapSpec << "' of type " << (int) GetType();

   return str;
}

#endif // DEBUG

// ----------------------------------------------------------------------------
// MailFolderCC sorting/threading
// ----------------------------------------------------------------------------

bool
MailFolderCC::SortMessages(MsgnoType *msgnos, const SortParams& sortParams)
{
   CHECK( m_MailStream, false, "can't sort closed folder" );

   /*
      if we want to do server side sorting and the server supports it
         if it can sort with the criteria from sortParams
            construct the sort program
            call mail_sort() to do server side sorting
            if ok
               return

      call base class version to do local sorting
    */

   // does the server support sorting? and if so, do we want to use it?
   if ( GetType() == MF_IMAP && LEVELSORT(m_MailStream) &&
        READ_CONFIG(m_Profile, MP_MSGS_SERVER_SORT) )
   {
      // construct the sort program checking that all sort criteria are
      // supported by the server side sort
      SORTPGM *pgmSort = NULL;
      SORTPGM **ppgmStep = &pgmSort;

      // iterate over all individual sort criteriums
      for ( long sortOrder = sortParams.sortOrder;
            sortOrder;
            sortOrder = GetSortNextCriterium(sortOrder) )
      {
         // c-client sort function
         int sortFunction;

         MessageSortOrder crit = GetSortCritDirect(sortOrder);
         switch ( crit )
         {
            case MSO_DATE:
               sortFunction = SORTDATE;
               break;

            case MSO_SUBJECT:
               sortFunction = SORTSUBJECT;
               break;

            case MSO_SIZE:
               sortFunction = SORTSIZE;
               break;

            case MSO_SENDER:
               if ( !sortParams.detectOwnAddresses )
                  sortFunction = SORTFROM;
               //else: server side sorting doesn't support this, fall through

            case MSO_STATUS:
            case MSO_SCORE:
               sortFunction = -1;
               break;

            case MSO_NONE:
               // MSO_NONE is not supposed to occur in sortOrder except as the
               // first (and only) criterium in which case we wouldn't get
               // here because of the loop test
               //
               // but as the user can currently create such sort orders using
               // the controls in the sort dialog (there are no checks there,
               // just ignore it here instead of giving an error)
               wxLogDebug("Unexpected MSO_NONE in the sort order");
               continue;

            default:
               FAIL_MSG("unexpected sort criterium");

               sortFunction = -1;
         }

         if ( sortFunction == -1 )
         {
            // criterium not supported, don't do server side sorting
            mail_free_sortpgm(&pgmSort);

            break;
         }

         // either start of continue the program
         *ppgmStep = mail_newsortpgm();

         // fill in the sort struct
         (*ppgmStep)->reverse = IsSortCritReversed(sortOrder);
         (*ppgmStep)->function = sortFunction;
         ppgmStep = &(*ppgmStep)->next;
      }

      // call mail_sort() to do server side sorting if we can
      if ( pgmSort )
      {
         wxLogTrace(TRACE_MF_CALLS, "MailFolderCC(%s)::SortMessages()",
                    GetName().c_str());

         // we need to provide a search program, otherwise c-client doesn't
         // sort anything - but if we give it an uninitialized SEARCHPGM, it
         // sorts all the messages [it's just a pleasure to use this library]
         SEARCHPGM *pgmSrch = mail_newsearchpgm();
         MsgnoType *results = mail_sort(m_MailStream,
                                        "US-ASCII",
                                        pgmSrch,
                                        pgmSort,
                                        SE_FREE | SO_FREE);
         if ( !results )
         {
            wxLogWarning(_("Server side sorting failed, trying to sort "
                           "messages locally."));
         }
         else // sorted ok
         {
            // we need to copy the results as we can't just reuse this buffer
            // because c-client has its own memory allocation function which
            // could be incompatible with our malloc/free
            for ( MsgnoType n = 0; n < m_MailStream->nmsgs; n++ )
            {
               *msgnos++ = *results++;
            }

            // do we have to free them or not??
            //fs_give((void **)&results);

            return true;
         }
      }
   }

   // call base class version to do local sorting
   return MailFolderCmn::SortMessages(msgnos, sortParams);
}

static void ThreadMessagesHelper(THREADNODE *thr,
                                 size_t level,
                                 MsgnoType& n,
                                 ThreadData *thrData,
                                 const ThreadParams& params)
{
   while ( thr )
   {
      MsgnoType msgno = thr->num;

      size_t indentChildren;
      if ( msgno == 0 )
      {
         // dummy message, representing the thread root which is not in the
         // mailbox: ignore it but don't forget to indent messages under it
         // if the corresponding option is set
         indentChildren = params.indentIfDummyNode;
      }
      else // valid message
      {
         // save this message details
         thrData->m_tableThread[n++] = msgno;
         thrData->m_indents[msgno - 1] = level;   // -1 to convert to index

         // always indent children below
         indentChildren = 1;
      }

      // do we have subtree?
      MsgnoType numChildren;
      if ( thr->next )
      {
         // process it and count the number of children
         MsgnoType nOld = n;
         ThreadMessagesHelper(thr->next, level + indentChildren, n,
                              thrData, params);

         ASSERT_MSG( n > nOld, "no children in a subthread?" );

         numChildren = n - nOld;
      }
      else
      {
         numChildren = 0;
      }

      if ( msgno )
      {
         // remember the number of children under this message, used later
         thrData->m_children[msgno - 1] = numChildren;
      }

      // pass to the next sibling
      thr = thr->branch;
   }
}

static bool MailStreamHasThreader(MAILSTREAM *stream, const char *thrName)
{
   IMAPLOCAL *imapLocal = (IMAPLOCAL *)stream->local;
   THREADER *thr;
   for ( thr = imapLocal->threader;
         thr && mail_compare_cstring(thr->name, (char *)thrName);
         thr = thr->next )
      ;

   return thr != NULL;
}


//
// Copy the tree given to us to some 'known' memory
//
static THREADNODE* CopyTree(THREADNODE* th) {
   THREADNODE* thrNode = new THREADNODE;
   thrNode->num = th->num;
   thrNode->next = CopyTree(th->next);
   thrNode->branch = CopyTree(th->branch);
   return thrNode;
}


bool MailFolderCC::ThreadMessages(const ThreadParams& thrParams,
                                  ThreadData *thrData)
{
   CHECK( m_MailStream, false, "can't thread closed folder" );

   // does the server support threading at all?
   if ( GetType() == MF_IMAP && LEVELSORT(m_MailStream) &&
        READ_CONFIG(m_Profile, MP_MSGS_SERVER_THREAD) )
   {
      const char *threadingAlgo;

      // it does, but maybe we want only threading by references (best) and it
      // only provides dumb threading by subject?
      if ( !MailStreamHasThreader(m_MailStream, "REFERENCES") )
      {
         if ( READ_CONFIG(m_Profile, MP_MSGS_SERVER_THREAD_REF_ONLY) )
         {
            // don't use server side threading at all if it only provides the
            // dumb way to do it
            threadingAlgo = NULL;
         }
         else // we are allowed to fall back to subject threading
         {
            if ( MailStreamHasThreader(m_MailStream, "ORDEREDSUBJECT") )
            {
               threadingAlgo = "ORDEREDSUBJECT";
            }
            else // no ORDEREDSUBJECT neither?
            {
               // well, it does have to support something if it announced
               // threading support in its CAPABILITY reply, so just use the
               // first threading method available
               IMAPLOCAL *imapLocal = (IMAPLOCAL *)m_MailStream->local;
               threadingAlgo = imapLocal->threader->name;
            }
         }
      }
      else // have REFERENCES threading
      {
         // this is the best method available, just use it
         threadingAlgo = "REFERENCES";
      }

      if ( threadingAlgo )
      {
         // do server side threading

         wxLogTrace(TRACE_MF_CALLS, "MailFolderCC(%s)::ThreadMessages()",
                    GetName().c_str());

         THREADNODE *thrRoot = mail_thread
                               (
                                 m_MailStream,
                                 (char *)threadingAlgo,
                                 NULL,                // default charset
                                 mail_newsearchpgm(), // thread all messages
                                 SE_FREE
                               );

         if ( thrRoot )
         {
            // We copy the tree so that we can release the c-client
            // memory right now. After that, we know that whatever
            // the algo that was used, we can destroy the tree the
            // same way.
            thrData->m_root = CopyTree(thrRoot);

            mail_free_threadnode(&thrRoot);

            // everything done
            return true;
         }
         else
         {
            wxLogWarning(_("Server side threading failed, trying to thread "
                           "messages locally."));
         }
      }
      //else: no appropriate thread method
   }

   // call base class version to do local sorting
   return MailFolderCmn::ThreadMessages(thrParams, thrData);
}

// ----------------------------------------------------------------------------
// MailFolderCC working with the headers
// ----------------------------------------------------------------------------

MsgnoType MailFolderCC::GetHeaderInfo(ArrayHeaderInfo& headers,
                                      const Sequence& seq)
{
   CHECK( m_MailStream, 0, "GetHeaderInfo: folder is closed" );

   MailFolderLocker lockFolder(this);

   String sequence = seq.GetString();
   wxLogTrace(TRACE_MF_CALLS, "Retrieving headers %s for '%s'...",
              sequence.c_str(), GetName().c_str());

   // prepare overviewData to be used by OverviewHeaderEntry()
   // --------------------------------------------------------

   MsgnoType nMessages = seq.GetCount();
   OverviewData overviewData(seq, headers, nMessages);

   // don't show the progress dialog if we're not in interactive mode
   if ( GetInteractiveFrame() && !mApplication->IsInAwayMode() )
   {
      // show the progress dialog if there are many messages to retrieve,
      // the check for threshold > 0 allows to disable the progress dialog
      // completely if the user wants
      long threshold = READ_CONFIG(m_Profile, MP_FOLDERPROGRESS_THRESHOLD);
      if ( threshold > 0 && nMessages > (unsigned long)threshold )
      {
         String msg;
         msg.Printf(_("Retrieving %lu message headers..."), nMessages);
         MGuiLocker locker;

         // create the progress meter which we will just have to Update() from
         // OverviewHeaderEntry() later
         overviewData.SetProgressDialog(
               new MProgressDialog(GetName(), msg, nMessages,
                                   NULL, false, true));
      }
   }
   //else: no progress dialog

   // do fill the listing
   size_t n;
   for ( UIdType i = seq.GetFirst(n); i != UID_ILLEGAL; i = seq.GetNext(i, n) )
   {
      MESSAGECACHE *elt = mail_elt(m_MailStream, i);
      if ( !elt )
      {
         FAIL_MSG( "failed to get sequence element?" );
         continue;
      }

      ENVELOPE *env = mail_fetch_structure(m_MailStream, i, NIL, NIL);
      if ( !env )
      {
         if ( !m_MailStream )
         {
            // connection was broken, don't try to get the others
            break;
         }

         FAIL_MSG( "failed to get sequence element envelope?" );

         continue;
      }

      if ( !OverviewHeaderEntry(&overviewData, elt, env) )
      {
         // cancelled
         break;
      }
   }

   return overviewData.GetRetrievedCount();
}

/* static */
String
MailFolderCC::ParseAddress(ADDRESS *adr)
{
   AddressList *addrList = AddressListCC::Create(adr);
   String address = addrList->GetAddresses();
   addrList->DecRef();

   return address;
}

bool
MailFolderCC::OverviewHeaderEntry(OverviewData *overviewData,
                                  MESSAGECACHE *elt,
                                  ENVELOPE *env)
{
   // overviewData must have been created in GetHeaderInfo()
   CHECK( overviewData, false, "OverviewHeaderEntry: no overview data?" );

   // it is possible that new messages have arrived in the meantime, ignore
   // them (FIXME: is it really normal?)
   if ( overviewData->Done() )
   {
      wxLogDebug("New message(s) appeared in folder while overviewing it, "
                 "ignored.");

      // stop overviewing
      return false;
   }

   // update the progress dialog and also check if it wasn't cancelled by the
   // user in the meantime
   if ( !overviewData->UpdateProgress() )
   {
      // cancelled by user
      return false;
   }

   // store what we've got
   HeaderInfo& entry = *overviewData->GetCurrent();

   // status
   entry.m_Status = GetMsgStatus(elt);

   // date
   MESSAGECACHE selt;
   mail_parse_date(&selt, env->date);
   entry.m_Date = (time_t) mail_longdate(&selt);

   // from and to
   entry.m_From = ParseAddress(env->from);

   FolderType folderType = GetType();
   if ( folderType == MF_NNTP || folderType == MF_NEWS )
   {
      entry.m_NewsGroups = env->newsgroups;
   }
   else
   {
      entry.m_To = ParseAddress(env->to);
   }

   // deal with encodings for the text header fields
   wxFontEncoding encoding;
   if ( !entry.m_To.empty() )
   {
      entry.m_To = DecodeHeader(entry.m_To, &encoding);
   }
   else
   {
      encoding = wxFONTENCODING_SYSTEM;
   }

   wxFontEncoding encodingMsg = encoding;

   entry.m_From = DecodeHeader(entry.m_From, &encoding);
   if ( (encoding != wxFONTENCODING_SYSTEM) &&
        (encoding != encodingMsg) )
   {
      if ( encodingMsg != wxFONTENCODING_SYSTEM )
      {
         // VZ: I just want to know if this happens, we can't do anything
         //     smart about this so far anyhow as we can't display the
         //     different fields in different encodings
         wxLogDebug("Different encodings for From and To fields, "
                    "From may be displayed incorrectly.");
      }
      else
      {
         encodingMsg = encoding;
      }
   }

   // subject

   entry.m_Subject = DecodeHeader(env->subject, &encoding);
   if ( (encoding != wxFONTENCODING_SYSTEM) &&
        (encoding != encodingMsg) )
   {
      if ( encodingMsg != wxFONTENCODING_SYSTEM )
      {
         wxLogDebug("Different encodings for From and Subject fields, "
                    "subject may be displayed incorrectly.");
      }
      else
      {
         encodingMsg = encoding;
      }
   }

   // all the other fields
   entry.m_Size = elt->rfc822_size;
   entry.m_Lines = 0;   // TODO: calculate them?
   entry.m_Id = env->message_id;
   entry.m_References = env->references;
   entry.m_InReplyTo = env->in_reply_to;
   entry.m_UId = mail_uid(m_MailStream, elt->msgno);

   // set the font encoding to be used for displaying this entry
   entry.m_Encoding = encodingMsg;

   overviewData->Next();

   // continue
   return true;
}

// ----------------------------------------------------------------------------
// MailFolderCC notification sending
// ----------------------------------------------------------------------------

void MailFolderCC::RequestUpdateAfterExpunge()
{
   CHECK_RET( m_expungedMsgnos, "shouldn't be called if we didn't expunge" );

   // FIXME: we ignore IsUpdateSuspended() here, should we? and if not,
   //        what to do?

   // tell GUI to update
   wxLogTrace(TRACE_MF_EVENTS, "Sending FolderExpunged event for folder '%s'",
              GetName().c_str());

   MEventManager::Send(new MEventFolderExpungeData(this,
                                                   m_expungedMsgnos,
                                                   m_expungedPositions));

   // MEventFolderExpungeData() will delete them
   m_expungedMsgnos =
   m_expungedPositions = NULL;
}

void
MailFolderCC::RequestUpdate()
{
   if ( IsUpdateSuspended() )
      return;

   wxLogTrace(TRACE_MF_EVENTS, "Sending FolderUpdate event for folder '%s'",
              GetName().c_str());

   // tell all interested that the folder changed
   MEventManager::Send(new MEventFolderUpdateData(this));
}

void MailFolderCC::OnMailExists(struct mail_stream *stream, MsgnoType msgnoMax)
{
   // NB: use stream and not m_MailStream because the latter might not be set
   //     yet if we're called from mail_open()

   // ignore any callbacks for closed or half opened folders: normally we
   // shouldn't get them at all but somehow we do (ask c-client why...)
   if ( !stream || stream->halfopen )
   {
      wxLogDebug("mm_exists() for not opened folder '%s' ignored.",
                 GetName().c_str());

      return;
   }

   // special case: when we get the first mm_exists() for an empty folder,
   // msgnoMax will be equal to m_nMessages (both will be 0), so catch this
   // with an additional check for m_LastUId
   //
   // TODO: would it be enough to just check if m_LastUId has changed?
   if ( msgnoMax != m_nMessages || m_LastUId == UID_ILLEGAL )
   {
      ASSERT_MSG( msgnoMax >= m_nMessages, "where have they gone?" );

      // update the number of headers in the listing
      if ( m_headers )
      {
         m_headers->OnAdd(msgnoMax);
      }

      // our cached idea of the number of messages we have doesn't correspond
      // to reality any more
      MfStatusCache *mfStatusCache = MfStatusCache::Get();
      if ( msgnoMax )
      {
         // flushing it like we do here is a bit dumb, so maybe we could
         // analyze the status of only the new messages? i.e. add "range"
         // parameters to DoCountMessages() and call it from here?
         mfStatusCache->InvalidateStatus(GetName());
      }
      else // no messages
      {
         // for an empty folder, we know the status
         MailFolderStatus status;

         // all other members are already set to 0
         status.total = 0;
         mfStatusCache->UpdateStatus(GetName(), status);
      }

      // update to use in the enclosing "if" test the next time
      m_nMessages = msgnoMax;

      // we may need to apply the filtering code but we can't do it from here
      // because we're inside c-client now and it is not reentrant, so we
      // send an event to ourselves to do it slightly later
      if ( msgnoMax )
      {
         MEventManager::Send(new MEventFolderOnNewMailData(this));
      }
   }
   else // same number of messages
   {
      if ( m_expungedMsgnos )
      {
         // we can update the status faster here as we have decremented the
         // number of recent/unseen/... when the messages were deleted (before
         // being expunged), so we just have to update the total now
         //
         // FIXME: this has all chances to break down with manual expunge or
         //        even with automatic one due to race condition (if the
         //        message status changed from outside...) - but this is so
         //        much faster and the problem is not really fatal that I
         //        still prefer to do it like this
         MailFolderStatus status;
         MfStatusCache *mfStatusCache = MfStatusCache::Get();
         if ( mfStatusCache->GetStatus(GetName(), &status) )
         {
            status.total = m_MailStream->nmsgs;

            mfStatusCache->UpdateStatus(GetName(), status);
         }

         RequestUpdateAfterExpunge();
      }
      //else: nothing at all
   }

   m_LastUId = stream->uid_last;
}

void MailFolderCC::OnMailExpunge(MsgnoType msgno)
{
   size_t idx = msgno - 1;

   // if we're applying the filters to the new messages, the GUI hadn't been
   // notified about existence of these messages yet, so we don't need to
   // notify it about their disappearance neither - it just will never know
   // they were there at all
   if ( !m_InFilterCode->IsLocked() )
   {
      // let all the others know that some messages were expunged: we don't do it
      // right now but wait until mm_exists() which is sent after all
      // mm_expunged() as it is much faster to delete all messages at once
      // instead of one at a time
      if ( !m_expungedMsgnos )
      {
         // create new arrays
         m_expungedMsgnos = new wxArrayInt;
         m_expungedPositions = new wxArrayInt;
      }

      // add the msgno to the list of expunged messages
      m_expungedMsgnos->Add(msgno);

      // and remember its position as well if we have it
      m_expungedPositions->Add(m_headers ? m_headers->GetPosFromIdx(idx)
                                         : INDEX_ILLEGAL);
   }

   // remove the message from the header info list if we have headers
   // (it would be wasteful to create them just to do it)
   if ( m_headers )
   {
      m_headers->OnRemove(idx);
   }

   // update the total number of messages
   ASSERT_MSG( m_nMessages > 0, "expunged message from an empty folder?" );

   m_nMessages--;

   // we don't change the cached status here as it will be done in
   // OnMailExists() later
}

// ----------------------------------------------------------------------------
// MailFolderCC new mail handling
// ----------------------------------------------------------------------------

void MailFolderCC::OnNewMail()
{
   CHECK_RET( m_MailStream, "OnNewMail: folder is closed" );

   // c-client is not reentrant, this is why we have to call this function
   // when we are not inside any c-client call!
   CHECK_RET( !m_MailStream->lock, "OnNewMail: folder is locked" );

   wxLogTrace(TRACE_MF_EVENTS, "Got new mail notification for '%s'",
              GetName().c_str());

   // see if we have any new messages
   //
   // FIXME: having recent messages doesn't mean havign new ones and although
   //        FilterNewMail() does count new messages before doing anything,
   //        this is still not very efficient - we'd better try to determine if
   //        we have any really new new messages. The trouble is that it just
   //        remembering stream->recent in some m_nRecent and comparing it here
   //        doesn't work as some recent messages could be expunged and new
   //        arrived without changing the total stream->recent. So then we'd
   //        have to check in OnMailExpunge() if we're expunging a recent
   //        message...
   if ( m_MailStream->recent )
   {
      // we don't want to process any events while we're filtering mail as
      // there can be a lot of them and all of them can be processed later
      MEventManagerSuspender suspendEvents;

      // don't allow changing the folder while we're filtering it
      MLocker filterLock(m_InFilterCode);

      FilterNewMail();

      CollectNewMail();
   }

   // we delayed sending the update notification in OnMailExists() because we
   // wanted to filter the new messages first - now we can notify the GUI
   // about the change
   RequestUpdate();
}

bool MailFolderCC::FilterNewMail()
{
   return MailFolderCmn::FilterNewMail();
}

// ----------------------------------------------------------------------------
// CClient initialization and clean up
// ----------------------------------------------------------------------------

void
MailFolderCC::CClientInit(void)
{
   if(ms_CClientInitialisedFlag == true)
      return;

   // link in all drivers (SSL is initialized later, when it is needed for the
   // first time)
#ifdef OS_WIN
#  include "linkage.c"
#else
#  include "linkage_no_ssl.c"
#endif

   // this triggers c-client initialisation via env_init()
#ifdef OS_UNIX
   if(geteuid() != 0) // running as root, skip init:
#endif
      (void *) myusername();

   // 1 try is enough, the default (3) is too slow: notice that this only sets
   // the number of trials for SMTP and not for the mailbox drivers for which
   // we have to set this separately!
   mail_parameters(NULL, SET_MAXLOGINTRIALS, (void *)1);

   (*imapdriver.parameters)(SET_MAXLOGINTRIALS, (void *)1);
   (*pop3driver.parameters)(SET_MAXLOGINTRIALS, (void *)1);

#ifdef USE_BLOCK_NOTIFY
   mail_parameters(NULL, SET_BLOCKNOTIFY, (void *)mahogany_block_notify);
#endif // USE_BLOCK_NOTIFY

#ifdef USE_READ_PROGRESS
   mail_parameters(NULL, SET_READPROGRESS, (void *)mahogany_read_progress);
#endif // USE_READ_PROGRESS

#ifdef OS_UNIX
   // install our own sigpipe handler to ignore (and not die) if a SIGPIPE
   // happens
   struct sigaction sa;
   sigset_t sst;
   sigemptyset(&sst);
   sa.sa_handler = sigpipe_handler;
   sa.sa_mask = sst;
   sa.sa_flags = SA_RESTART;
   if( sigaction(SIGPIPE,  &sa, NULL) != 0)
   {
      wxLogError(_("Cannot set signal handler for SIGPIPE."));
   }
#endif // OS_UNIX

   ms_CClientInitialisedFlag = true;

   ASSERT(gs_CCStreamCleaner == NULL);
   gs_CCStreamCleaner = new CCStreamCleaner();

   ASSERT_MSG( !gs_CCEventReflector, "couldn't be created yet" );
   gs_CCEventReflector = new CCEventReflector;
}

CCStreamCleaner::~CCStreamCleaner()
{
   wxLogTrace(TRACE_MF_CALLS, "CCStreamCleaner: checking for left-over streams");

   // don't send us any notifications any more
   delete gs_CCEventReflector;

   if(! mApplication->IsOnline())
   {
      // brutally free all memory without closing stream:
      MAILSTREAM *stream;
      StreamList::iterator i;
      for(i = m_List.begin(); i != m_List.end(); i++)
      {
         DBGMESSAGE(("CCStreamCleaner: freeing stream offline"));
         stream = *i;
         // copied from c-client mail.c:
         if (stream->mailbox) fs_give ((void **) &stream->mailbox);
         stream->sequence++;  /* invalidate sequence */
         /* flush user flags */
         for (int n = 0; n < NUSERFLAGS; n++)
            if (stream->user_flags[n]) fs_give ((void **) &stream->user_flags[n]);
         mail_free_cache (stream);  /* finally free the stream's storage */
         /*if (!stream->use)*/ fs_give ((void **) &stream);
      }
   }
   else
   {
      // we are online, so we can close it properly:
      StreamList::iterator i;
      for(i = m_List.begin(); i != m_List.end(); i++)
      {
         DBGMESSAGE(("CCStreamCleaner: closing stream"));
         mail_close(*i);
      }
   }
}

// ----------------------------------------------------------------------------
// functions used by MailFolder initialization/shutdown code
// ----------------------------------------------------------------------------

extern bool MailFolderCCInit(void)
{
   MailFolderCC::CClientInit();

   return true;
}

extern void MailFolderCCCleanup(void)
{
   // as c-client lib doesn't seem to think that deallocating memory is
   // something good to do, do it at it's place...
   //
   // NB: do not reverse the ordering: GET_NEWSRC may need GET_HOMEDIR
   free(mail_parameters((MAILSTREAM *)NULL, GET_NEWSRC, NULL));
   free(mail_parameters((MAILSTREAM *)NULL, GET_HOMEDIR, NULL));

   if ( gs_CCStreamCleaner )
   {
      delete gs_CCStreamCleaner;
      gs_CCStreamCleaner = NULL;
   }

   ASSERT_MSG( MailFolderCC::ms_StreamList.empty(), "some folder objects leaked" );

   // FIXME: we really need to clean up these entries, so we just
   //        decrement the reference count until they go away.
   while ( !MailFolderCC::ms_StreamList.empty() )
   {
      MailFolderCC *folder = MailFolderCC::ms_StreamList.front()->folder;
      wxLogDebug("\tFolder '%s' leaked", folder->GetName().c_str());
      folder->DecRef();
   }
}

// ----------------------------------------------------------------------------
// Some news spool support (TODO: move out from here)
// ----------------------------------------------------------------------------

static String gs_NewsSpoolDir;

/// return the directory of the newsspool:
/* static */
String
MailFolderCC::GetNewsSpool(void)
{
   CClientInit();
   return (const char *)mail_parameters (NIL,GET_NEWSSPOOL,NULL);
}

const String& MailFolder::InitializeNewsSpool()
{
   if ( !gs_NewsSpoolDir )
   {
      // first, init cclient
      MailFolderCCInit();

      gs_NewsSpoolDir = (char *)mail_parameters(NULL, GET_NEWSSPOOL, NULL);
      if ( !gs_NewsSpoolDir )
      {
         gs_NewsSpoolDir = READ_APPCONFIG_TEXT(MP_NEWS_SPOOL_DIR);
         mail_parameters(NULL, SET_NEWSSPOOL, (char *)gs_NewsSpoolDir.c_str());
      }
   }

   return gs_NewsSpoolDir;
}

// ----------------------------------------------------------------------------
// callbacks called by cclient
// ----------------------------------------------------------------------------

/// tells program that there are this many messages in the mailbox
void
MailFolderCC::mm_exists(MAILSTREAM *stream, unsigned long msgnoMax)
{
   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET( mf, "number of messages changed in unknown mail folder" );

   mf->OnMailExists(stream, msgnoMax);
}

/* static */
void
MailFolderCC::mm_expunged(MAILSTREAM *stream, unsigned long msgno)
{
   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET(mf, "mm_expunged for non existent folder");

   mf->OnMailExpunge(msgno);
}

/// this message matches a search
void
MailFolderCC::mm_searched(MAILSTREAM * stream,
                          unsigned long msgno)
{
   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET( mf, "messages found in unknown mail folder" );

   // this must have been allocated before starting the search
   CHECK_RET( mf->m_SearchMessagesFound, "logic error in search code" );

   mf->m_SearchMessagesFound->Add(msgno);
}

/** this mailbox name matches a listing request
       @param stream mailstream
       @param delim  character that separates hierarchies
       @param name   mailbox name
       @param attrib mailbox attributes
 */
void
MailFolderCC::mm_list(MAILSTREAM * stream,
                      char delim,
                      String  name,
                      long  attrib)
{
   if(gs_mmListRedirect)
   {
      gs_mmListRedirect(stream, delim, name, attrib);
      return;
   }

   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET(mf,"NULL mailfolder");

   // create the event corresponding to the folder
   ASMailFolder::ResultFolderExists *result =
      ASMailFolder::ResultFolderExists::Create
      (
         mf->m_ASMailFolder,
         mf->m_Ticket,
         name,
         delim,
         attrib,
         mf->m_UserData
      );

   // and send it
   MEventManager::Send(new MEventASFolderResultData (result) );

   // don't forget to free the result - MEventASFolderResultData makes a copy
   // (i.e. calls IncRef) of it
   result->DecRef();

   // wxYield() is needed to send the events resulting from (previous) calls
   // to MEventManager::Send(), but don't forget to prevent any other calls to
   // c-client from happening - this will result in a fatal error as it is not
   // reentrant (FIXME!!!)
#if wxCHECK_VERSION(2, 2, 6)
   wxYieldIfNeeded();
#else // wxWin <= 2.2.5
   wxYield();
#endif // wxWin 2.2.5/2.2.6
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
                        String mailbox,
                        MAILSTATUS *status)
{
   MailFolderCC *mf = LookupObject(stream, mailbox);
   CHECK_RET(mf, "mm_status for non existent folder");

   wxLogTrace(TRACE_MF_CALLBACK, "mm_status: folder '%s', %lu messages",
              mf->m_ImapSpec.c_str(), status->messages);

   // do nothing here for now
}

/** log a verbose message
       @param stream mailstream
       @param str message str
       @param errflg error level
 */
void
MailFolderCC::mm_notify(MAILSTREAM * stream, String str, long errflg)
{
   MailFolderCC *mf = MailFolderCC::LookupObject(stream);

   // detect the notifications about stream being closed
   if ( mf && errflg == BYE )
   {
      // reset it to prevent us from trying to close it - won't work
      mf->m_MailStream = NULL;

      wxLogWarning(_("Connection to the folder '%s' lost unexpectedly."),
                   mf->GetName().c_str());
   }

   mm_log(str, errflg, mf);
}

/** log a message
    @param str message str
    @param errflg error level
*/
void
MailFolderCC::mm_log(const String& str, long errflg, MailFolderCC *mf)
{
   GetLogCircle().Add(str);

   if ( str.StartsWith("SELECT failed") )
   {
      // send this to the debug window anyhow, but don't show it to the user
      // as this message can only result from trying to open a folder which
      // has a \Noselect flag and we handle this ourselves
      mm_dlog(str);

      return;
   }

   if ( errflg == 0 && !mm_show_debug )
   {
      // this is a verbose information message and we're not interested in
      // them normally (there are tons of them...)
      return;
   }

   String msg = _("Mail log");
   if( mf )
      msg << " (" << mf->GetName() << ')';
   msg << ':' << str
#ifdef DEBUG
       << _(", error level: ") << strutil_ultoa(errflg)
#endif
      ;

   wxLogLevel loglevel;
   switch ( errflg )
   {
      case 0: // a.k.a INFO
         loglevel = wxLOG_User;
         break;

      case BYE:
      case WARN:
         loglevel = wxLOG_Status; // do not WARN user, it's annoying
         break;

      default:
         FAIL_MSG( "unknown cclient log level" );
         // fall through

      case ERROR:
         // usually just wxLOG_Error but may be set to something else to
         // temporarily suppress the errors
         loglevel = cc_loglevel;
         break;

      case PARSE:
         loglevel = wxLOG_Status; // goes to window anyway
         break;
   }

   wxLogGeneric(loglevel, msg);
}

/** log a debugging message
    @param str    message str
*/
void
MailFolderCC::mm_dlog(const String& str)
{
   if ( str.empty() )
   {
      // avoid calling GetWriteBuf(0) below, this is invalid
      return;
   }

   GetLogCircle().Add(str);

   // replace the passwords in the log window output (which can be seen by
   // others or be cut-and-pasted to show some problems) with stars if plain
   // text authentification is used: PASS for POP or LOGIN for IMAP
   if ( mm_show_debug )
   {
      String msg, username, password;
      unsigned long seq;

      // the max len of username/password
      size_t len = str.length();

      bool isPopLogin = sscanf(str, "PASS %s", password.GetWriteBuf(len)) == 1;
      password.UngetWriteBuf();

      if ( isPopLogin )
      {
         msg << "PASS " << wxString('*', password.length());
      }
      else
      {
         bool isImapLogin = sscanf(str, "%08lx LOGIN %s %s",
                                   &seq,
                                   username.GetWriteBuf(len),
                                   password.GetWriteBuf(len)) == 3;
         username.UngetWriteBuf();
         password.UngetWriteBuf();

         if ( isImapLogin )
         {
            msg.Printf("%08lx LOGIN %s %s",
                       seq,
                       username.c_str(),
                       wxString('*', password.length()).c_str());
         }
         else
         {
            msg = str;
         }
      }

      // send it to the window
      wxLogGeneric(M_LOG_WINONLY, _("Mail log: %s"), msg.c_str());
   }
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
   // normally this shouldn't happen
   ASSERT_MSG( !MF_user.empty(), "no username in mm_login()?" );

   strcpy(user, MF_user.c_str());
   strcpy(pwd, MF_pwd.c_str());
}

/* static */
void
MailFolderCC::mm_flags(MAILSTREAM *stream, unsigned long msgno)
{
   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET(mf, "mm_flags for non existent folder");

   // message flags really changed, cclient checks for it
   mf->UpdateMessageStatus(msgno);
}

/** alert that c-client will run critical code
    @param  stream   mailstream
 */
void
MailFolderCC::mm_critical(MAILSTREAM *stream)
{
   if ( stream )
   {
      ms_LastCriticalFolder = stream->mailbox;
      MailFolderCC *mf = LookupObject(stream);
      if(mf)
      {
         mf->m_InCritical = true;
      }
   }
   else
   {
      ms_LastCriticalFolder = "";
   }
}

/**   no longer running critical code
   @param   stream mailstream
     */
void
MailFolderCC::mm_nocritical(MAILSTREAM *stream)
{
   ms_LastCriticalFolder = "";
   if ( stream )
   {
      MailFolderCC *mf = LookupObject(stream);
      if(mf)
      {
         mf->m_InCritical = false;
      }
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
   return 1;   // abort!
}

/** program is about to crash!
    @param  str   message str
*/
void
MailFolderCC::mm_fatal(char *str)
{
   GetLogCircle().Add(str);
   wxLogError(_("Fatal error: %s"), str);

   String msg2 = str;
   if(ms_LastCriticalFolder.length())
      msg2 << _("\nLast folder in a critical section was: ")
           << ms_LastCriticalFolder;
   FatalError(msg2);
}

// ----------------------------------------------------------------------------
// Listing/subscribing/deleting the folders
// ----------------------------------------------------------------------------

/* static */
bool
MailFolderCC::Subscribe(const String &host,
                        FolderType protocol,
                        const String &mailboxname,
                        bool subscribe)
{
   String spec = MailFolder::GetImapSpec(protocol, 0, mailboxname, host, "");
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
   String spec = m_ImapSpec;

   // make sure that there is a folder name delimiter before pattern - this
   // only makes sense for non empty spec
   if ( !spec.empty() )
   {
      char ch = spec.Last();

      if ( ch != '}' )
      {
         char chDelim = GetFolderDelimiter();

         if ( ch != chDelim )
            spec += chDelim;
      }
   }

   spec << reference << pattern;

   ASSERT(asmf);

   // set user data (retrieved by mm_list)
   m_UserData = ud;
   m_Ticket = ticket;
   m_ASMailFolder = asmf;
   m_ASMailFolder->IncRef();

   if ( subscribedOnly )
   {
      mail_lsub (m_MailStream, NULL, (char *) spec.c_str());
   }
   else
   {
      mail_list (m_MailStream, NULL, (char *) spec.c_str());
   }

   // Send event telling about end of listing:
   ASMailFolder::ResultFolderExists *result =
      ASMailFolder::ResultFolderExists::Create
      (
         m_ASMailFolder,
         m_Ticket,
         "",  // empty name == no more entries
         0,   // no delim
         0,   // no flags
         m_UserData
      );

   // and send it
   MEventManager::Send(new MEventASFolderResultData (result) );

   result->DecRef();
   m_ASMailFolder->DecRef();
   m_ASMailFolder = NULL;
}

// ----------------------------------------------------------------------------
// gettting the folder delimiter separator
// ----------------------------------------------------------------------------

static char gs_delimiter = '\0';

static void GetDelimiterMMList(MAILSTREAM *stream,
                               char delim,
                               String name,
                               long attrib)
{
   if ( delim )
      gs_delimiter = delim;
}

char MailFolderCC::GetFolderDelimiter() const
{
   // may be we already have it cached?
   if ( m_chDelimiter == ILLEGAL_DELIMITER )
   {
      MailFolderCC *self = (MailFolderCC *)this; // const_cast

      // the only really interesting case is IMAP as we need to query the
      // server for this one: to do it we issue 'LIST "" ""' command which is
      // guaranteed by RFC 2060 to return the delimiter
      if ( GetType() == MF_IMAP )
      {
         CHECK( m_MailStream, '\0', "folder closed in GetFolderDelimiter" );

         MMListRedirector redirect(GetDelimiterMMList);

         // pass the spec for the IMAP server itself to mail_list()
         String spec = m_ImapSpec.BeforeFirst('}');
         if ( !spec.empty() )
            spec += '}';

         gs_delimiter = '\0';
         mail_list(m_MailStream, NULL, (char *)spec.c_str());

         // well, except that in practice some IMAP servers do *not* return
         // anything in reply to this command! try working around this bug
         if ( !gs_delimiter )
         {
            spec += '%';
            mail_list(m_MailStream, NULL, (char *)spec.c_str());
         }

         // we must have got something!
         ASSERT_MSG( gs_delimiter,
                     "broken IMAP server returned no folder name delimiter" );

         self->m_chDelimiter = gs_delimiter;
      }
      else
      {
         // not IMAP, let the base class version do it
         self->m_chDelimiter = MailFolder::GetFolderDelimiter();
      }
   }

   ASSERT_MSG( m_chDelimiter != ILLEGAL_DELIMITER, "should have delimiter" );

   return m_chDelimiter;
}

// ----------------------------------------------------------------------------
// delete folder
// ----------------------------------------------------------------------------

/* static */
bool
MailFolderCC::Rename(const MFolder *mfolder, const String& name)
{
   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC::Rename(): %s -> %s",
              mfolder->GetPath().c_str(), name.c_str());

   // I'm unsure if this is needed but I suppose we're going to have problems
   // if we rename the folder being used - or maybe not?
   (void)MailFolderCC::CloseFolder(mfolder);

   if ( !mail_rename(NULL,
                     (char *)::GetImapSpec(mfolder).c_str(),
                     (char *)name.c_str()) )
   {
      wxLogError(_("Failed to rename the mailbox for folder '%s' "
                   "from '%s' to '%s'."),
                 mfolder->GetFullName().c_str(),
                 mfolder->GetPath().c_str(),
                 name.c_str());

      return false;
   }

   return true;
}

// ----------------------------------------------------------------------------
// clear folder
// ----------------------------------------------------------------------------

/* static */
long
MailFolderCC::ClearFolder(const MFolder *mfolder)
{
   FolderType type = mfolder->GetType();
   int flags = mfolder->GetFlags();
   String login = mfolder->GetLogin(),
          password = mfolder->GetPassword(),
          fullname = mfolder->GetFullName();
   String mboxpath = MailFolder::GetImapSpec(type,
                                             flags,
                                             mfolder->GetPath(),
                                             mfolder->GetServer(),
                                             login);

   // if this folder is opened, use its stream - and also notify about message
   // deletion
   MAILSTREAM *stream;
   unsigned long nmsgs;
   CCCallbackDisabler *noCCC;

   MailFolderCC *mf = FindFolder(mboxpath, login);
   if ( mf )
   {
      stream = mf->m_MailStream;
      nmsgs = mf->GetMessageCount();
      mf->DecRef();

      noCCC = NULL;
   }
   else // this folder is not opened
   {
      if ( !AskPasswordIfNeeded(fullname,
                                type, flags, &login, &password ) )
      {
         return false;
      }

      SetLoginData(login, password);

      // we don't want any notifications
      noCCC = new CCCallbackDisabler;

      // open the folder: although we don't need to do it to get its status, we
      // have to do it anyhow below, so better do it right now
      stream = mail_open(NIL, (char *)mboxpath.c_str(),
                         mm_show_debug ? OP_DEBUG : NIL);

      if ( !stream )
      {
         wxLogError(_("Impossible to open folder '%s'"), fullname.c_str());

         delete noCCC;

         return false;
      }

      // get the number of messages (only)
      MAILSTATUS mailstatus;
      MMStatusRedirector statusRedir(stream->mailbox, &mailstatus);
      mail_status(stream, stream->mailbox, SA_MESSAGES);
      nmsgs = mailstatus.messages;
   }

   if ( nmsgs > 0 )
   {
      wxBusyCursor busyCursor;

      String seq;
      seq << "1:" << nmsgs;

      // now mark them all as deleted (we don't need notifications about the
      // status change so save a *lot* of bandwidth by using ST_SILENT)
      mail_flag(stream, (char *)seq.c_str(), "\\DELETED", ST_SET | ST_SILENT);

      // and expunge
      if ( mf )
      {
         mf->ExpungeMessages();

         mf->DecRef();
      }
      else // folder is not opened, just expunge quietly
      {
         mail_expunge(stream);

         // we need to update the status manually in this case
         MailFolderStatus status;

         // all other members are already set to 0
         status.total = 0;
         MfStatusCache::Get()->UpdateStatus(fullname, status);
      }
   }
   //else: no messages to delete

   delete noCCC;

   return nmsgs;
}

// ----------------------------------------------------------------------------
// delete folder
// ----------------------------------------------------------------------------

/* static */
bool
MailFolderCC::DeleteFolder(const MFolder *mfolder)
{
   FolderType type = mfolder->GetType();
   int flags = mfolder->GetFlags();
   String login = mfolder->GetLogin(),
          password = mfolder->GetPassword();

   if ( !AskPasswordIfNeeded(mfolder->GetFullName(),
                             type, flags, &login, &password ) )
   {
      return false;
   }

   String mboxpath = MailFolder::GetImapSpec(type,
                                             flags,
                                             mfolder->GetPath(),
                                             mfolder->GetServer(),
                                             login);

   MCclientLocker lock;
   SetLoginData(login, password);

   return mail_delete(NIL, (char *) mboxpath.c_str()) != NIL;
}

// ----------------------------------------------------------------------------
// HasInferiors() function: check if the folder has subfolders
// ----------------------------------------------------------------------------

/*
  Small function to check if the mailstream has \inferiors flag set or
  not, i.e. if it is a mailbox or a directory on the server.
*/

static int gs_HasInferiorsFlag = -1;

static void HasInferiorsMMList(MAILSTREAM *stream,
                               char delim,
                               String name,
                               long attrib)
{
   gs_HasInferiorsFlag = ((attrib & LATT_NOINFERIORS) == 0);
}

/* static */
bool
MailFolderCC::HasInferiors(const String &imapSpec,
                           const String &login,
                           const String &passwd)
{
   // We lock the complete c-client code as we need to immediately
   // redirect the mm_list() callback to tell us what we want to know.
   MCclientLocker lock;

   // we want to disable all the usual callbacks as we might not have the
   // folder corresponding to imapSpec yet (as this function is called from
   // the folder creation dialog) and, even if we did, we are not interested
   // in them for now
   CCCallbackDisabler noCallbacks;

   SetLoginData(login, passwd);
   gs_HasInferiorsFlag = -1;

   MAILSTREAM *mailStream = mail_open(NIL, (char *)imapSpec.c_str(),
                                      (mm_show_debug ? OP_DEBUG : NIL) |
                                      OP_HALFOPEN);

   if(mailStream != NIL)
   {
     MMListRedirector redirect(HasInferiorsMMList);
     mail_list (mailStream, NULL, (char *) imapSpec.c_str());

     /* This does happen for for folders where the server does not know
        if they have inferiors, i.e. if they don't exist yet.
        I.e. -1 is an unknown/undefined status
        ASSERT(gs_HasInferiorsFlag != -1);
     */
     mail_close(mailStream);
   }

   return gs_HasInferiorsFlag == 1;
}

// ----------------------------------------------------------------------------
// network operations progress
// ----------------------------------------------------------------------------

void MailFolderCC::StartReading(unsigned long total)
{
#ifdef USE_READ_PROGRESS
   ASSERT_MSG( !gs_readProgressInfo, "can't start another read operation" );

   // don't show the progress dialogs for the local folders - in practice, it
   // will never take really long time to read from them
   if ( IsLocalQuickFolder(GetType()) )
      return;

   // here is the idea: if the message is big, we show the progress dialog
   // immediately, otherwise we only do it if retrieving it takes longer than
   // the specified time
   long size = READ_CONFIG(m_Profile, MP_MESSAGEPROGRESS_THRESHOLD_SIZE);
   if ( size > 0 )
   {
      int delay = total > (unsigned long)size * 1024
                  ? 0
                  : READ_CONFIG(m_Profile, MP_MESSAGEPROGRESS_THRESHOLD_TIME);

      gs_readProgressInfo = new ReadProgressInfo(total, delay);
   }
#endif // USE_READ_PROGRESS
}

void MailFolderCC::EndReading()
{
#ifdef USE_READ_PROGRESS
   if ( gs_readProgressInfo )
   {
      delete gs_readProgressInfo;
      gs_readProgressInfo = NULL;
   }
   //else: we didn't create it in StartReading()
#endif // USE_READ_PROGRESS
}

// ----------------------------------------------------------------------------
// C-Client callbacks: the ones which must be implemented
// ----------------------------------------------------------------------------

// define a macro TRACE_CALLBACKn() for tracing a callback with n + 1 params
#ifdef DEBUG // EXPERIMENTAL_log_callbacks
   static const char *GetErrorLevel(long errflg)
   {
      return errflg == ERROR ? "error"
                             : errflg == WARN ? "warn"
                                              : errflg == PARSE ? "parse"
                                                                : "other";
   }

   static inline const char *GetStreamMailbox(MAILSTREAM *stream)
   {
      return stream ? stream->mailbox : "<no stream>";
   }

   #define TRACE_CALLBACK0(name) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(%s)", \
                 #name, GetStreamMailbox(stream))

   #define TRACE_CALLBACK1(name, fmt, parm) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(%s, " fmt ")", \
                 #name, GetStreamMailbox(stream), parm)

   #define TRACE_CALLBACK2(name, fmt, parm1, parm2) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(%s, " fmt ")", \
                 #name, GetStreamMailbox(stream), parm1, parm2)

   #define TRACE_CALLBACK3(name, fmt, parm1, parm2, parm3) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(%s, " fmt ")", \
                 #name, GetStreamMailbox(stream), parm1, parm2, parm3)

   #define TRACE_CALLBACK_NOSTREAM_1(name, fmt, parm1) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(" fmt ")", \
                 #name, parm1)

   #define TRACE_CALLBACK_NOSTREAM_2(name, fmt, parm1, parm2) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(" fmt ")", \
                 #name, parm1, parm2)

   #define TRACE_CALLBACK_NOSTREAM_5(name, fmt, parm1, parm2, parm3, parm4, parm5) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(" fmt ")", \
                 #name, parm1, parm2, parm3, parm4, parm5)
#else // !EXPERIMENTAL_log_callbacks
   #define TRACE_CALLBACK0(name)
   #define TRACE_CALLBACK0(name)
   #define TRACE_CALLBACK1(name, fmt, p1)
   #define TRACE_CALLBACK2(name, fmt, p1, p2)
   #define TRACE_CALLBACK3(name, fmt, p1, p2, p3)

   #define TRACE_CALLBACK_NOSTREAM_1(name, fmt, p1)
   #define TRACE_CALLBACK_NOSTREAM_2(name, fmt, p1, p2)
   #define TRACE_CALLBACK_NOSTREAM_5(name, fmt, p1, p2, p3, p4, p5)
#endif

// they're called from C code
extern "C"
{

void
mm_searched(MAILSTREAM *stream, unsigned long msgno)
{
   if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK1(mm_searched, "%lu", msgno);

      MailFolderCC::mm_searched(stream,  msgno);
   }
}

void
mm_expunged(MAILSTREAM *stream, unsigned long msgno)
{
   if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK1(mm_expunged, "%lu", msgno);

      MailFolderCC::mm_expunged(stream, msgno);
   }
}

void
mm_flags(MAILSTREAM *stream, unsigned long msgno)
{
   if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK1(mm_flags, "%lu", msgno);

      MailFolderCC::mm_flags(stream, msgno);
   }
}

void
mm_notify(MAILSTREAM *stream, char *str, long errflg)
{
   if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK2(mm_notify, "%s (%s)", str, GetErrorLevel(errflg));

      MailFolderCC::mm_notify(stream, str, errflg);
   }
}

void
mm_list(MAILSTREAM *stream, int delim, char *name, long attrib)
{
   TRACE_CALLBACK3(mm_list, "%d, `%s', %ld", delim, name, attrib);

   MailFolderCC::mm_list(stream, delim, name, attrib);
}

void
mm_lsub(MAILSTREAM *stream, int delim, char *name, long attrib)
{
   TRACE_CALLBACK3(mm_lsub, "%d, `%s', %ld", delim, name, attrib);

   MailFolderCC::mm_lsub(stream, delim, name, attrib);
}

void
mm_exists(MAILSTREAM *stream, unsigned long msgno)
{
   if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK1(mm_exists, "%lu", msgno);

      MailFolderCC::mm_exists(stream, msgno);
   }
}

void
mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS *status)
{
   // allow redirected callbacks even while all others are disabled
   if ( gs_mmStatusRedirect )
   {
      gs_mmStatusRedirect(stream, mailbox, status);
   }
   else if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK1(mm_status, "%s", mailbox);

      MailFolderCC::mm_status(stream, mailbox, status);
   }
}

void
mm_log(char *str, long errflg)
{
   if ( mm_disable_callbacks || mm_ignore_errors )
      return;

   TRACE_CALLBACK_NOSTREAM_2(mm_log, "%s (%s)", str, GetErrorLevel(errflg));

   String msg(str);

   // TODO: what's going on here?
   if(errflg >= 4) // fatal imap error, reopen-mailbox
   {
      if(!MailFolderCC::PingReopenAll())
         msg << _("\nAttempt to re-open all folders failed.");
   }

   MailFolderCC::mm_log(msg, errflg);
}

void
mm_dlog(char *str)
{
   // always show debug logs, even if other callbacks are disabled - this
   // makes it easier to understand what's going on

   // if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK_NOSTREAM_1(mm_dlog, "%s", str);

      MailFolderCC::mm_dlog(str);
   }
}

void
mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
   // don't test for mm_disable_callbacks here, we can be called even while
   // other callbacks are disabled (example is call to mail_open() in
   // ClearFolder())
   TRACE_CALLBACK_NOSTREAM_5(mm_login, "%s %s@%s:%ld, try #%ld",
                             mb->service, mb->user, mb->host, mb->port,
                             trial);

   MailFolderCC::mm_login(mb, user, pwd, trial);
}

void
mm_critical(MAILSTREAM *stream)
{
   TRACE_CALLBACK0(mm_critical);

   MailFolderCC::mm_critical(stream);
}

void
mm_nocritical(MAILSTREAM *stream)
{
   TRACE_CALLBACK0(mm_nocritical);

   MailFolderCC::mm_nocritical(stream);
}

long
mm_diskerror(MAILSTREAM *stream, long int errorcode, long int serious)
{
   if ( mm_disable_callbacks )
      return 1;

   return MailFolderCC::mm_diskerror(stream,  errorcode, serious);
}

void
mm_fatal(char *str)
{
   MailFolderCC::mm_fatal(str);
}

// ----------------------------------------------------------------------------
// some more cclient callbacks: these are optional but we set them up to have
// more control over cclient operation
// ----------------------------------------------------------------------------

#ifdef USE_BLOCK_NOTIFY

#include <wx/datetime.h>

void *mahogany_block_notify(int reason, void *data)
{
   #define LOG_BLOCK_REASON(what) \
      if ( reason == BLOCK_##what ) \
         printf("%s: mm_blocknotify(%s, %p)\n", \
                wxDateTime::Now().FormatTime().c_str(), #what, data); \
      else \

   LOG_BLOCK_REASON(NONE)
   LOG_BLOCK_REASON(SENSITIVE)
   LOG_BLOCK_REASON(NONSENSITIVE)
   LOG_BLOCK_REASON(DNSLOOKUP)
   LOG_BLOCK_REASON(TCPOPEN)
   LOG_BLOCK_REASON(TCPREAD)
   LOG_BLOCK_REASON(TCPWRITE)
   LOG_BLOCK_REASON(TCPCLOSE)
   LOG_BLOCK_REASON(FILELOCK)
   printf("mm_blocknotify(UNKNOWN, %p)\n", data);

   return NULL;
}

#endif // USE_BLOCK_NOTIFY

#ifdef USE_READ_PROGRESS

void mahogany_read_progress(GETS_DATA *md, unsigned long count)
{
   if ( gs_readProgressInfo )
      gs_readProgressInfo->OnProgress(count);
}

#endif // USE_READ_PROGRESS

} // extern "C"

