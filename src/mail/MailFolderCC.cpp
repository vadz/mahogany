///////////////////////////////////////////////////////////////////////////////
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

/* TODO: several things are still unclear...

   1. is mm_status() ever called? we never use mail_status(), but maybe
      cclient does this internally?

  +2. mm_expunged() definitely shouldn't rebuild the entire listing (by
      callling RequestUpdate) but should just remove the expunged entries from
      the existing one

   3. moreover, maybe mm_exists() doesn't need to do it neither but just add
      the entries to the existing listing?

   4. there might be problems with limiting number of messages in
      BuildListing(), this has to be tested
 */

/*
   Update logic documentation (accurate as of 30.01.01):

   The mail folder maintains the listing containing the information about all
   messages in the folder in m_Listing. The listing is modified only by
   BuildListing() function or, more precisely, by OverviewHeaderEntry() called
   from it through a number of others (partly ours, partly c-client)
   functions. While the listing is being built, m_InListingRebuild is locked
   and nothing else can be done!

   The listing has to be rebuilt when the number of the messages in the
   mailbox changes (either because the new messages arrived or because the
   existing messages are expunged). In this case mm_exists is called - note
   that it happens after mm_expunged is called one or more times in the latter
   case.

   mm_exists() deletes the current listing and any mailbox operation on this
   folder will rebuild it the next time it is called.

   So we have 3 states for the folder:

   a) normal: we have a valid listing and can use it
   b) dirty: we don't have listing, we will build it the next it is needed
   c) busy: we're building the listing right now

   The folder is created in the dirty state and transits to the normal state
   via the busy state when it is used for the first time non trivially. A call
   to mm_exists() puts it to the dirty state again.

   Further, there is another subtlety related to the filters: the filters must
   be executed from GetHeaders() as we want to apply them to all new messages.
   However filters can modify the folder resulting in more mm_expunged() calls
   and also call GUI code (message boxes) reentering the GUI event loop
   implicitly and thus the events may be dispatched from isnide a call to
   ApplyFilterRules(). Because of this we just don't send any events while the
   filters are being executed.

   The last twist is that we also avoid sending events when the application is
   being shut down as it results in the calls to the obejcts which don't exist
   any more later from the GUI code [FIXME: this might be a kludge and could
   change]
 */

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
#   include "MPython.h"
#endif // USE_PCH

#include "Mdefaults.h"
#include "MDialogs.h"
#include "FolderView.h"

#include "MailFolderCC.h"
#include "MessageCC.h"

#include "MEvent.h"
#include "MThread.h"
#include "ASMailFolder.h"
#include "Mpers.h"

#include "HeaderInfoImpl.h"

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

extern "C"
{
   #undef LOCAL         // previously defined in other cclient headers
   #include <pop3.h>    // for pop3_xxx() in GetMessageUID()
}

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// check that the folder is not in busy state - if it is, no external calls
// are allowed
#define CHECK_NOT_BUSY() \
   CHECK_RET( !m_InListingRebuild->IsLocked() && !m_InFilterCode->IsLocked(), \
              "folder is busy" )

#define CHECK_NOT_BUSY_RC(rc) \
   CHECK( !m_InListingRebuild->IsLocked() && !m_InFilterCode->IsLocked(), \
          rc, "folder is busy" )

// check if the folder is still available
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
// constants
// ----------------------------------------------------------------------------

static const char * cclient_drivers[] =
{
   "mbx", "unix", "mmdf", "tenex"
};

#define CCLIENT_MAX_DRIVER (sizeof(cclient_drivers)/sizeof(cclient_drivers[0]))

#define NO_PROGRESS_DLG ((MProgressDialog *)1)

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
// private types
// ----------------------------------------------------------------------------

typedef int (*overview_x_t) (MAILSTREAM *stream,unsigned long uid,OVERVIEW_X *ov);

// extended OVERVIEW struct additionally containing the destination address
struct OVERVIEW_X : public mail_overview
{
   ADDRESS *to;
};

// type of the callback for mail_fetch_overview()
typedef int (*overview_x_t) (MAILSTREAM *stream,
                             unsigned long uid,
                             OVERVIEW_X *ov);

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

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

extern "C"
{
   static int mm_overview_header (MAILSTREAM *stream,unsigned long uid, OVERVIEW_X *ov);
   void mail_fetch_overview_nonuid (MAILSTREAM *stream,char *sequence,overview_t ofn);
   void mail_fetch_overview_x (MAILSTREAM *stream,char *sequence,overview_x_t ofn);
};

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

/// exactly one object
static CCStreamCleaner *gs_CCStreamCleaner = NULL;

/// This allows us to temporarily redirect mm_list calls
static void (*gs_mmListRedirect)(MAILSTREAM * stream,
                                 char delim,
                                 String  name,
                                 long  attrib) = NULL;

// a variable telling c-client to shut up
static bool mm_ignore_errors = false;

/// a variable disabling most events (not mm_list, mm_lsub, mm_overview_header)
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

// ----------------------------------------------------------------------------
// FolderListing stuff (TODO: move elsewhere)
// ----------------------------------------------------------------------------

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

// VZ: it was a nice idea, but unfortunately GTK+ timers send one last event
//     even after being stopped! So it doesn't make much sense to do this as
//     we still get the timer event and the collection code still runs - so
//     the fix is now in MailCollector() itself
#ifdef TIMER_SUSPENDING_WORKS

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

#endif // TIMER_SUSPENDING_WORKS

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
      mboxpath << '{' << server << "/pop3";
      if(flags & MF_FLAGS_SSLAUTH)
         mboxpath << "/ssl";
      mboxpath << '}';
      break;
   case MF_IMAP:  // do we need /imap flag?
      if(flags & MF_FLAGS_ANON)
         mboxpath << '{' << server << "/anonymous";
      else
      {
         if(login.Length())
            mboxpath << '{' << server << "/user=" << login ;
         else // we get asked  later FIXME!!
            mboxpath << '{' << server << '}'<< name;
      }
      if(flags & MF_FLAGS_SSLAUTH)
         mboxpath << "/ssl";
      mboxpath << '}' << name;
      break;
   case MF_NEWS:
      mboxpath << "#news." << name;
      break;
   case MF_NNTP:
      mboxpath << '{' << server << "/nntp";
      if(flags & MF_FLAGS_SSLAUTH)
         mboxpath << "/ssl";
      mboxpath << '}' << name;
      break;
   default:
      FAIL_MSG("Unsupported folder type.");
   }
   return mboxpath;
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

/*
 * Small function to translate c-client status flags into ours.
 */
static int GetMsgStatus(MESSAGECACHE *elt)
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
String MailFolderCC::DecodeHeader(const String &in, wxFontEncoding *pEncoding)
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
         //else: multi letter encoding unreckognized

         if ( enc2047 == Encoding_Unknown )
         {
            wxLogDebug("Unreckognized header encoding in '%s'.",
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

         // get the encoded text
         p += 2; // skip "Q?" or "B?"
         const char *pEncTextStart = p;
         while ( *p && *p != '?' )
            p++;

         unsigned long lenEncWord = p - pEncTextStart;

         if ( !*p || *++p != '=' )
         {
            wxLogDebug("Missing encoded word end marker in '%s'.",
                       pEncWordStart);
            if ( !*p )
               out += pEncWordStart;
            else
               out += String(pEncWordStart, p);

            continue;
         }

         // now decode the text using cclient functions
         unsigned long len;
         unsigned char *start = (unsigned char *)pEncTextStart; // for cclient
         char *text;
         if ( enc2047 == Encoding_Base64 )
         {
            text = (char *)rfc822_base64(start, lenEncWord, &len);
         }
         else
         {
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
// MailFolderCC
// ----------------------------------------------------------------------------

String MailFolderCC::MF_user;
String MailFolderCC::MF_pwd;


/* static */
void
MailFolderCC::SetLoginData(const String &user, const String &pw)
{
   MailFolderCC::MF_user = user;
   MailFolderCC::MF_pwd = pw;
}

/* static */
void
MailFolderCC::UpdateCClientConfig()
{
   mm_show_debug = READ_APPCONFIG(MP_DEBUG_CCLIENT) != 0;
}

// ----------------------------------------------------------------------------
// MailFolderCC construction/opening
// ----------------------------------------------------------------------------

MailFolderCC::MailFolderCC(int typeAndFlags,
                           String const &path,
                           Profile *profile,
                           String const &server,
                           String const &login,
                           String const &password)
            : MailFolderCmn()
{
   m_Profile = profile;
   if(m_Profile)
      m_Profile->IncRef();
   else
      m_Profile = Profile::CreateEmptyProfile();

   if(! ms_CClientInitialisedFlag)
      CClientInit();
   Create(typeAndFlags);
   if(GetType() == MF_FILE)
      m_ImapSpec = strutil_expandpath(path);
   else
      m_ImapSpec = path;
   m_Login = login;
   m_Password = password;

   // do this at the very end as it calls RequestUpdate() and so anything may
   // happen from now on
   UpdateConfig();
}

void
MailFolderCC::Create(int typeAndFlags)
{
   m_MailStream = NIL;

   m_InCritical = false;

   UpdateTimeoutValues();

   SetRetrievalLimits(0, 0); // no limits by default

   m_nMessages = 0;
   m_msgnoMax = 0;
   m_nRecent = UID_ILLEGAL;
   m_LastUId = UID_ILLEGAL;
   m_Listing = NULL;
   m_expunged = false;

   // currently not used, but might be in the future
   m_GotNewMessages = false;
   m_FirstListing = true;

   m_FolderListing = NULL;

   m_InCritical = false;
   m_ASMailFolder = NULL;

   FolderType type = GetFolderType(typeAndFlags);
   m_FolderFlags = GetFolderFlags(typeAndFlags);
   m_SearchMessagesFound = NULL;
   SetType(type);
   if( !FolderTypeHasUserName(type) )
      m_Login = ""; // empty login for these types

   m_Mutex = new MMutex;
   m_PingReopenSemaphore = new MMutex;
   m_InListingRebuild = new MMutex;
   m_InFilterCode = new MMutex;
}

void MailFolderCC::SetRetrievalLimits(unsigned long soft, unsigned long hard)
{
   m_RetrievalLimit = soft;
   m_RetrievalLimitHard = hard;
}

MailFolderCC::~MailFolderCC()
{
   PreClose();

   CCAllDisabler ccDis;

   Close();

   ASSERT(m_SearchMessagesFound == NULL);

   // we might still be listed, so we better remove ourselves from the
   // list to make sure no more events get routed to this (now dead) object
   RemoveFromMap();

   delete m_Mutex;
   delete m_PingReopenSemaphore;
   delete m_InListingRebuild;
   delete m_InFilterCode;

   SafeDecRef(m_Listing);
   SafeDecRef(m_Profile);
}

/*
  This gets called with a folder path as its name, NOT with a symbolic
  folder/profile name.
*/

/* static */
MailFolderCC *
MailFolderCC::OpenFolder(int typeAndFlags,
                         String const &name,
                         Profile *profile,
                         String const &server,
                         String const &loginGiven,
                         String const &passwordGiven,
                         String const &symname,
                         bool halfopen)
{
   CHECK( profile, NULL, "no profile in OpenFolder" );

   CClientInit();

   int flags = GetFolderFlags(typeAndFlags);
   int type = GetFolderType(typeAndFlags);

   String login = loginGiven;
   String mboxpath = MailFolder::GetImapSpec(type, flags, name, server, login);

   //FIXME: This should somehow be done in MailFolder.cpp
   MailFolderCC *mf = FindFolder(mboxpath,login);
   if(mf)
   {
      mf->IncRef();
      mf->PingReopen(); //FIXME: is this really needed? // make sure it's updated
      return mf;
   }

   bool userEnteredPwd = false;
   String password = passwordGiven;

   // ask the password for the folders which need it but for which it hadn't
   // been specified during creation
   if ( FolderTypeHasUserName( (FolderType) GetFolderType(typeAndFlags))
        && ((GetFolderFlags(typeAndFlags) & MF_FLAGS_ANON) == 0)
        && (login.empty() || password.empty())
      )
   {
      if ( !MDialog_GetPassword(symname, &login, &password) )
      {
         ERRORMESSAGE((_("Cannot access this folder without a password.")));

         mApplication->SetLastError(M_ERROR_CANCEL);

         return NULL; // cannot open it
      }

      // remember that the password was entered interactively and propose to
      // remember it if it really works
      userEnteredPwd = true;
   }

   mf = new MailFolderCC(typeAndFlags, mboxpath, profile, server, login, password);
   mf->m_Name = symname;
   mf->SetRetrievalLimits(READ_CONFIG(profile, MP_MAX_HEADERS_NUM),
                          READ_CONFIG(profile, MP_MAX_HEADERS_NUM_HARD));

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

      ok = halfopen ? mf->HalfOpen() : mf->Open();

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
   else if ( profile && userEnteredPwd )
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
         profile->writeEntry(MP_FOLDER_LOGIN, login);
         profile->writeEntry(MP_FOLDER_PASSWORD, strutil_encrypt(password));
      }
   }

   return mf;
}

bool MailFolderCC::HalfOpen()
{
   MFrame *frame = GetInteractiveFrame();
   if ( frame )
   {
      // well, we're not going to tell the user we're half opening it...
      STATUSMESSAGE((frame, _("Opening mailbox %s..."), GetName().c_str()));
   }

   if( FolderTypeHasUserName(GetType()) )
      SetLoginData(m_Login, m_Password);

   MCclientLocker lock;

   {
      CCDefaultFolder def(this);
      m_MailStream = mail_open(NIL, (char *)m_ImapSpec.c_str(),
                               (mm_show_debug ? OP_DEBUG : NIL)|OP_HALFOPEN);
   }

   if ( !m_MailStream )
      return false;

   AddToMap(m_MailStream);

   return true;
}

bool
MailFolderCC::Open(void)
{
   MFrame *frame = GetInteractiveFrame();
   if ( frame )
   {
      STATUSMESSAGE((frame, _("Opening mailbox %s..."), GetName().c_str()));
   }

   FolderType folderType = GetType();

   // Now, we apply the very latest c-client timeout values, in case they have
   // changed.
   UpdateTimeoutValues();

   if( FolderTypeHasUserName(folderType) )
   {
      SetLoginData(m_Login, m_Password);
   }

   // for files, check whether mailbox is locked, c-client library is
   // to dumb to handle this properly
   if ( folderType == MF_FILE
#ifdef OS_UNIX
         || folderType == MF_INBOX
#endif
      )
      {
         String lockfile;
         if(folderType == MF_FILE)
            lockfile = m_ImapSpec;
#ifdef OS_UNIX
         else // INBOX
         {
            // get INBOX path name
            MCclientLocker lock;
            lockfile = (char *) mail_parameters (NIL,GET_SYSINBOX,NULL);
            if(lockfile.IsEmpty()) // another c-client stupidity
               lockfile = (char *) sysinbox();
      }
#endif // OS_UNIX

      lockfile << ".lock*"; //FIXME: is this fine for MS-Win? (NO!)
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
   }
   //else: not a file based folder, no locking problems

   {
      MCclientLocker lock;

      // Make sure that all events to unknown folder go to us
      CCDefaultFolder def(this);

      // create the mailbox if it doesn't exist yet
      bool alreadyTriedToCreate = FALSE;
      if ( folderType == MF_FILE || folderType == MF_MH )
      {
         bool exists;
         if ( folderType == MF_FILE )
         {
            exists = wxFileExists(m_ImapSpec);
         }
         else if ( folderType == MF_MH )
         {
            // construct the filename from MH folder name
            String path;
            path << InitializeMH()
                 << m_ImapSpec.c_str() + 4; // 4 == strlen("#mh/")
            exists = wxFileExists(path);
         }

         // if the file folder doesn't exist, we should create it first
         if ( !exists )
         {
            // This little hack makes it root (uid 0) safe and allows us
            // to choose the file format, too:
            String tmp;
            if(folderType == MF_FILE)
            {
               long format = READ_CONFIG(m_Profile, MP_FOLDER_FILE_DRIVER);
               if ( format < 0  || (size_t)format > CCLIENT_MAX_DRIVER )
               {
                  FAIL_MSG( "invalid mailbox format" );
                  format = 0;
               }

               tmp = "#driver.";
               tmp << cclient_drivers[format] << '/';

               if ( frame )
               {
                  STATUSMESSAGE((
                     frame,
                     _("Trying to create folder '%s' in %s format."),
                     GetName().c_str(),
                     cclient_drivers[format]
                  ));
               }
            }
            else // MF_MH folder
            {
               // forgot to handle this one...
               ASSERT_MSG( folderType == MF_MH, "unexpected folder type" );

               tmp = "#driver.mh/";
            }

            tmp += m_ImapSpec;
            mail_create(NIL, (char *)tmp.c_str());
            alreadyTriedToCreate = TRUE;
         }
      }

      // first try, don't log errors (except in debug mode)
      {
#ifndef DEBUG
         CCErrorDisabler noErrors;
#endif // DEBUG
         m_MailStream = mail_open(m_MailStream,(char *)m_ImapSpec.c_str(),
                                  mm_show_debug ? OP_DEBUG : NIL);
      }

      // try to create it if opening failed and we hadn't tried to create yet
      // (note that if the mailbox doesn't exist, the mail stream will still
      // be non NULL, but the folder will be only half opened)
      if ( (!m_MailStream || m_MailStream->halfopen) && !alreadyTriedToCreate )
      {
         mail_create(NIL, (char *)m_ImapSpec.c_str());

         // do we need this: if ( m_MailStream ) mail_close(m_MailStream) ?

         // retry again, showing the errors this time
         m_MailStream = mail_open(m_MailStream, (char *)m_ImapSpec.c_str(),
                                  mm_show_debug ? OP_DEBUG : NIL);
      }
   }

   // for some folders (notably the IMAP ones), mail_open() will return a not
   // NULL pointer but set halfopen flag if it couldn't be SELECTed - as we
   // really want to open it here and not halfopen, this is an error for us
   if ( m_MailStream && m_MailStream->halfopen )
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

         MFolder_obj(m_Profile)->AddFlags(MF_FLAGS_NOSELECT);

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
      STATUSMESSAGE((frame, _("Mailbox %s opened."), GetName().c_str()));
   }

   PY_CALLBACK(MCB_FOLDEROPEN, 0, GetProfile());

   return true;   // success
}

bool
MailFolderCC::IsAlive() const
{
   return m_MailStream != NULL;
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

   // We now use only one common config setting for all TCP timeout
   m_TcpOpenTimeout = READ_CONFIG(p, MP_TCP_OPENTIMEOUT);
   m_TcpReadTimeout = m_TcpOpenTimeout;
   m_TcpWriteTimeout = m_TcpOpenTimeout;
   m_TcpCloseTimeout = m_TcpOpenTimeout;
   m_LookAhead = READ_CONFIG(p, MP_IMAP_LOOKAHEAD);

   // but a separate one for rsh timeout to allow enabling/disabling rsh
   // independently of TCP timeout
   m_TcpRshTimeout = READ_CONFIG(p, MP_TCP_RSHTIMEOUT);

   // and another one for SSH
   m_TcpSshTimeout = READ_CONFIG(p, MP_TCP_SSHTIMEOUT);

   // also read the paths for the commands if we use them
   if ( m_TcpRshTimeout )
      m_RshPath = READ_CONFIG(p, MP_RSH_PATH);
   if ( m_TcpSshTimeout )
      m_SshPath = READ_CONFIG(p, MP_SSH_PATH);

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

   CHECK_STREAM_LIST();

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
/* static */ MailFolderCC *
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
      wxLogTrace(TRACE_MF_CALLBACK, "Routing call to default mailfolder.");
      return ms_StreamListDefaultObj;
   }

   FAIL_MSG("Cannot find mailbox for callback!");

   return NULL;
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
         return conn->folder;
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
   if ( (m_folderType == MF_FILE) )
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
   bool rc = false;

   wxLogTrace(TRACE_MF_CALLS, "MailFolderCC::PingReopen() on Folder %s.",
              GetName().c_str());

   CHECK_NOT_BUSY_RC(rc);

   MailFolderLocker lockFolder(this);
   if ( lockFolder )
   {
      MLocker lockPing(m_PingReopenSemaphore);
      rc = true;

      // This is terribly inefficient to do, but needed for some sick
      // POP3 servers which don't report new mail otherwise
      if( (m_FolderFlags & MF_FLAGS_REOPENONPING)
          // c-client 4.7-bug: MH folders don't immediately notice new
          // messages:
          || GetType() == MF_MH )
      {
         wxLogTrace(TRACE_MF_CALLS,
                    "MailFolderCC::Ping() forcing close on folder %s.",
                    GetName().c_str());
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

   /* Ping() might close/reopen a mailfolder, which means that some folder we
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
      connections[n++] = *i;
   }

   for ( n = 0; n < count; n++ )
   {
      MailFolderCC *mf = connections[n]->folder;

      // don't ping locked folders, they will have to wait for the next time
      if ( !mf->IsLocked() )
      {
         rc &= fullPing ? mf->PingReopen() : mf->Ping();
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

   return PingReopen();
}

// ----------------------------------------------------------------------------
// MailFolderCC closing
// ----------------------------------------------------------------------------

void
MailFolderCC::Close()
{
   /*
     DO NOT SEND EVENTS FROM HERE, ITS CALLED FROM THE DESTRUCTOR AND
     THE OBJECT *WILL* DISAPPEAR!
   */

   CCAllDisabler no;

   if( m_MailStream )
   {
      if(!NeedsNetwork() || mApplication->IsOnline())
      {
         if ( !m_MailStream->halfopen )
         {
            // update flags, etc, .newsrc
            mail_check(m_MailStream);
         }

         mail_close(m_MailStream);
      }
      else
         gs_CCStreamCleaner->Add(m_MailStream); // delay closing
      m_MailStream = NIL;
   }
}

// ----------------------------------------------------------------------------
// MailFolderCC locking
// ----------------------------------------------------------------------------

bool
MailFolderCC::Lock(void) const
{
   ((MailFolderCC *)this)->m_Mutex->Lock();

   return true;
}

bool
MailFolderCC::IsLocked(void) const
{
   return m_Mutex->IsLocked() ||
          m_InListingRebuild->IsLocked() ||
          m_InFilterCode->IsLocked();
}

void
MailFolderCC::UnLock(void) const
{
   ((MailFolderCC *)this)->m_Mutex->Unlock();
}

// ----------------------------------------------------------------------------
// Adding and removing (expunging) messages
// ----------------------------------------------------------------------------

bool
MailFolderCC::AppendMessage(String const &msg, bool update)
{
   CHECK_DEAD_RC("Appending to closed folder '%s' failed.", false);

   STRING str;

   INIT(&str, mail_string, (void *) msg.c_str(), msg.Length());

   bool rc = mail_append(m_MailStream, (char *)m_ImapSpec.c_str(), &str) != 0;
   if(! rc)
      ERRORMESSAGE(("cannot append message"));
   else
   {
      if( ( (m_UpdateFlags & UF_DetectNewMail) == 0)
          && ( (m_UpdateFlags & UF_UpdateCount) != 0) )
      {
         // this mail will be stored as uid_last+1 which isn't updated
         // yet at this point
         m_LastNewMsgUId = m_MailStream->uid_last+1;
      }
      // triggers asserts:
   }
   if(update)
   {
     FilterNewMail(GetHeaders());
   }
   return rc;
}

bool
MailFolderCC::AppendMessage(Message const &msg, bool update)
{
   CHECK_DEAD_RC("Appending to closed folder '%s' failed.", false);

   long rc = 0; //failure
   /* This is an optimisation: if both mailfolders are IMAP and on the
      same server, we ask the server to copy the message, which is
      more efficient (think of a modem!).
      If that is not the case, or it fails, then it simply retrieves
      the message and appends it to the second folder.
   */
   if(GetType() == MF_IMAP)
   {
      MailFolder *mf = ((Message &)msg).GetFolder();
      if(mf->GetType() == MF_IMAP)
      {
         // Now we know mf is MailFolderCC:
         MAILSTREAM *ms = ((MailFolderCC *)mf)->m_MailStream;
         String tmp1 = GetFirstPartFromImapSpec(mf->GetImapSpec());
         String tmp2 = GetFirstPartFromImapSpec(GetImapSpec());
         // Host, protocol, user all identical:
         if(tmp1 == tmp2)
         {
            String sequence;
            sequence.Printf("%lu", msg.GetUId());
            String path = GetPathFromImapSpec(GetImapSpec());
            rc =
               mail_copy_full (ms,
                               (char *)sequence.c_str(),
                               (char *)path.c_str(),
                               CP_UID);
            // if failed, we try to append normally
         }
      }
   }
   if(rc == 0) // no success yet, almost "else" :-)
   {
      String flags = GetImapFlags(msg.GetStatus());
      String date;
      msg.GetHeaderLine("Date", date);
      MESSAGECACHE mc;
      const char *dateptr = NULL;
      char datebuf[128];
      if(mail_parse_date(&mc, (char *) date.c_str()) == T)
      {
        mail_date(datebuf, &mc);
        dateptr = datebuf;
      }
      // different folders, so we actually copy the message around:
      String tmp;
      if(msg.WriteToString(tmp))
      {
         STRING str;
         INIT(&str, mail_string, (void *) tmp.c_str(), tmp.Length());
         rc =  mail_append_full(m_MailStream,
                                (char *)m_ImapSpec.c_str(),
                                (char *)flags.c_str(),
                                (char *)dateptr,
                                &str);
      }
      else
         rc = 0;
   }
   if(rc == 0)
   {
      ERRORMESSAGE(("cannot append message"));
      return FALSE;
   }
   else
      return TRUE;
}

void
MailFolderCC::ExpungeMessages(void)
{
   CHECK_DEAD("Cannot access closed folder\n'%s'.");

   // don't try to modify the listing while it's being built!
   CHECK_NOT_BUSY()

   if ( PY_CALLBACK(MCB_FOLDEREXPUNGE,1,GetProfile()) )
   {
      mail_expunge(m_MailStream);
   }
}


// ----------------------------------------------------------------------------
// Counting messages
// ----------------------------------------------------------------------------

// msg status info
//
// NB: name MessageStatus is already taken by MailFolder
struct MsgStatus
{
   bool recent;
   bool unseen;
   bool newmsgs;
   bool flagged;
   bool searched;
};

// is this message recent/new/unseen/...?
static MsgStatus AnalyzeStatus(int stat)
{
   MsgStatus status;

   // deal with recent and new (== recent and !seen)
   status.recent = (stat & MailFolder::MSG_STAT_RECENT) != 0;
   status.unseen = !(stat & MailFolder::MSG_STAT_SEEN);
   status.newmsgs = status.recent && status.unseen;

   // and also count flagged and searched messages
   status.flagged = (stat & MailFolder::MSG_STAT_FLAGGED) != 0;
   status.searched = (stat & MailFolder::MSG_STAT_SEARCHED ) != 0;

   return status;
}

// are we asked to count recent messages in WantRecent()?
static bool WantRecent(int mask, int value)
{
   return mask == MailFolder::MSG_STAT_RECENT && value == mask;
}

unsigned long
MailFolderCC::CountMessages(int mask, int value) const
{
   // we need to rebuild listing if we don't have it because either we use it
   // anyhow or we need to ensure that m_nMessages and m_nRecent have correct
   // values
   HeaderInfoList_obj hil = GetHeaders();
   CHECK( hil, UID_ILLEGAL, "no listing in CountMessages" );

   size_t numOfMessages;
   if ( mask == MSG_STAT_NONE )
   {
      // total number of messages
      numOfMessages = m_nMessages;
   }
   else if ( WantRecent(mask, value) && m_nRecent != UID_ILLEGAL )
   {
      // asked for recent messages and we have them already
      return m_nRecent;
   }
   else // general case or we asked for recent messages and we don't have them
   {
      wxLogTrace("count",
                 "Counting all messages with given mask in '%s' (%lu all)",
                 GetName().c_str(), m_nMessages);

      // FIXME there should probably be a much more efficient way (using
      //       mail_search()?) to do it
      numOfMessages = 0;
      for ( size_t idx = 0; idx < m_nMessages; idx++ )
      {
         if ( (hil->GetItemByIndex(idx)->GetStatus() & mask) == value )
            numOfMessages++;
      }

      // if we were asked for recent messages, cache them
      if ( WantRecent(mask, value) )
      {
         ((MailFolderCC *)this)->m_nRecent = numOfMessages;
      }
   }

   return numOfMessages;
}

bool MailFolderCC::CountInterestingMessages(MailFolderStatus *status) const
{
   CHECK( status, false, "NULL pointer in CountInterestingMessages" );

   // do we already have the number of messages?
   if ( m_status.IsValid() )
   {
      // do at least a simple consistency check
      ASSERT_MSG( m_status.total == m_nMessages,
                  "caching of number of messages broken" );

      *status = m_status;

      return true;
   }

   status->Init();

   HeaderInfoList_obj hil = GetHeaders();
   if ( !hil )
   {
      // this is not a (programming) error - maybe we just failed to open the
      // folder - so don't assert here
      return false;
   }

   wxLogTrace("count",
              "Counting all interesting messages in '%s' (%lu all)",
              GetName().c_str(), m_nMessages);

   status->total = m_nMessages;
   for ( size_t idx = 0; idx < m_nMessages; idx++ )
   {
      int stat = hil->GetItemByIndex(idx)->GetStatus();

      // ignore deleted messages
      if ( stat & MSG_STAT_DELETED )
         continue;

      MsgStatus msgStat = AnalyzeStatus(stat);

      #define INC_NUM_OF(what)   if ( msgStat.what ) status->what++

      INC_NUM_OF(recent);
      INC_NUM_OF(unseen);
      INC_NUM_OF(newmsgs);
      INC_NUM_OF(flagged);
      INC_NUM_OF(searched);

      #undef INC_NUM_OF
   }

   // cache the number of messages
   MailFolderCC *self = (MailFolderCC *)this; // const_cast
   self->m_status = *status;
   self->m_nRecent = status->recent;

   return status->newmsgs ||
          status->recent ||
          status->unseen ||
          status->flagged ||
          status->searched;
}

// ----------------------------------------------------------------------------
// Getting messages
// ----------------------------------------------------------------------------

Message *
MailFolderCC::GetMessage(unsigned long uid)
{
   CHECK_DEAD_RC("Cannot access closed folder\n'%s'.", NULL);

   HeaderInfoList_obj hil = GetHeaders();
   CHECK( hil, NULL, "no listing in GetMessage" );

   // test to see if the UID is valid:
   bool uidValid = false;
   size_t count = hil->Count();
   for ( size_t idx = 0; idx < count && !uidValid; idx++ )
   {
      if ( hil->GetItemByIndex(idx)->GetUId() == uid )
         uidValid = true;
   }

   CHECK( uidValid, NULL, "invalid UID in GetMessage" );

   return MessageCC::CreateMessageCC(this, uid);
}

UIdArray *
MailFolderCC::SearchMessages(const SearchCriterium *crit)
{
   CHECK( !m_SearchMessagesFound, NULL, "SearchMessages reentered" );
   CHECK( crit, NULL, "no criterium in SearchMessages" );

   HeaderInfoList *hil = GetHeaders();
   CHECK( hil, NULL, "no listing in SearchMessages" );

   unsigned long lastcount = 0;
   MProgressDialog *progDlg = NULL;
   m_SearchMessagesFound = new UIdArray;

   // Error is set to true if we cannot handle it on the server side
   // or later, if a real error occurs.
   bool error = false;

   // attempt server side search first: this currently only works for IMAP and
   // NNTP but maybe it will work for other folders too later
   {
      SEARCHPGM *pgm = mail_newsearchpgm();
      STRINGLIST *slist = mail_newstringlist();

      slist->text.data = (unsigned char *)strdup(crit->m_Key);
      slist->text.size = crit->m_Key.length();

      switch ( crit->m_What )
      {
         case SearchCriterium::SC_FULL:
            pgm->text = slist; break;
         case SearchCriterium::SC_BODY:
            pgm->body = slist; break;
         case SearchCriterium::SC_SUBJECT:
            pgm->subject = slist; break;
         case SearchCriterium::SC_TO:
            pgm->to = slist; break;
         case SearchCriterium::SC_FROM:
            pgm->from = slist; break;
         case SearchCriterium::SC_CC:
            pgm->cc = slist; break;

         default:
            error = true;
            mail_free_searchpgm(&pgm);
      }

      if ( !error )
      {
         mail_search_full (m_MailStream,
                           NIL /* charset: use default (US-ASCII) */,
                           pgm,
                           SE_UID | SE_FREE | SE_NOPREFETCH);
      }
   }

   // our own search for all other folder types or unsuppported server
   // searches
   if ( error )
   {
      String what;
      error = false;

      if ( m_nMessages > (unsigned)READ_CONFIG(m_Profile,
                                               MP_FOLDERPROGRESS_THRESHOLD) )
      {
         String msg;
         msg.Printf(_("Searching in %lu messages..."),
                    (unsigned long) m_nMessages);
         {
            MGuiLocker locker;
            progDlg = new MProgressDialog(GetName(),
                                          msg,
                                          m_nMessages,
                                          NULL,
                                          false, true);// open a status window:
         }
      }

      for ( size_t idx = 0; idx < m_nMessages; idx++ )
      {
         if ( crit->m_What == SearchCriterium::SC_SUBJECT )
         {
            what = hil->GetItemByIndex(idx)->GetSubject();
         }
         else
         {
            if ( crit->m_What == SearchCriterium::SC_FROM )
            {
               what = hil->GetItemByIndex(idx)->GetFrom();
            }
            else
            {
               Message *msg = GetMessage(hil->GetItemByIndex(idx)->GetUId());
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
                     FAIL_MSG("Unknown search criterium!");
                     error = true;
               }
               msg->DecRef();
            }
         }

         bool found =  what.Contains(crit->m_Key);
         if(crit->m_Invert)
            found = ! found;
         if(found)
            m_SearchMessagesFound->Add(hil->GetItemByIndex(idx)->GetUId());

         if(progDlg)
         {
            String msg;
            msg.Printf(_("Searching in %lu messages..."),
                       (unsigned long) m_nMessages);
            String msg2;
            unsigned long cnt = m_SearchMessagesFound->Count();
            if(cnt != lastcount)
            {
               msg2.Printf(_(" - %lu matches found."), cnt);
               msg = msg + msg2;
               if ( ! progDlg->Update(idx, msg ) )
               {
                  // abort searching
                  break;
               }

               lastcount = cnt;
            }
            else if( !progDlg->Update(idx) )
            {
               // abort searching
               break;
            }
         }
      }
      hil->DecRef();
   }

   UIdArray *rc = m_SearchMessagesFound; // will get freed by caller!
   m_SearchMessagesFound = NULL;

   if(progDlg)
      delete progDlg;
   return rc;
}

String
MailFolderCC::GetMessageUID(unsigned long msgno) const
{
   CHECK( m_MailStream, "", "can't get message UID - folder is closed" );

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
// Message flags
// ----------------------------------------------------------------------------

/* static */
String MailFolderCC::BuildSequence(const UIdArray& messages)
{
   String sequence;
   size_t n = messages.GetCount();
   for ( size_t i = 0; i < n; i++ )
   {
      if ( i > 0 )
         sequence += ',';

      sequence += strutil_ultoa(messages[i]);
   }

   return sequence;
}

bool
MailFolderCC::SetSequenceFlag(String const &sequence,
                              int flag,
                              bool set)
{
   CHECK_DEAD_RC("Cannot access closed folder\n'%s'.", false);
   String flags = GetImapFlags(flag);

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
   str << "mailbox '" << m_ImapSpec << "' of type " << (int) m_folderType;

   return str;
}

#endif // DEBUG

// ----------------------------------------------------------------------------
// Working with the listing
// ----------------------------------------------------------------------------

bool MailFolderCC::HasHeaders() const
{
   return m_Listing != NULL;
}

/* FIXME-MT: we must add some clever locking to this function! */
HeaderInfoList *
MailFolderCC::GetHeaders(void) const
{
   if ( !m_Listing )
   {
      // remove const from this:
      MailFolderCC *that = (MailFolderCC *)this;

      // check if we are still alive
      if ( !m_MailStream )
      {
         if( !that->PingReopen() )
         {
            ERRORMESSAGE((_("Cannot get listing for closed mailbox '%s'."),
                          GetName().c_str()));
            return NULL;
         }
      }

      // get the new listing
      that->BuildListing();

      CHECK(m_Listing, NULL, "failed to build folder listing");

      // if we're configured not to detect new mail, just ignore it
      if ( !(GetUpdateFlags() & UF_DetectNewMail) )
      {
         that->m_GotNewMessages = false;
      }

      // if we have any new messages, reapply filters to them
      that->FilterNewMailIfNeeded();

      // sort/thread listing
      that->ProcessHeaderListing(m_Listing);

      // enforce consistency:
      ASSERT_MSG( m_nMessages == m_Listing->Count(), "message count mismatch" );

      if ( m_GotNewMessages )
      {
         // do it right now to prevent any possible recursion
         that->m_GotNewMessages = false;

         // check if we need to update our data structures or send new
         // mail events: notice that we do it after applying the filters above
         // so as to not show new mail notification for folder if all new
         // messages were moved elsewhere by the filters
         that->CheckForNewMail(m_Listing);
      }
   }
   //else: we already have the listing

   m_Listing->IncRef(); // for the caller who uses it

   return m_Listing;
}

void MailFolderCC::FilterNewMailIfNeeded()
{
   CHECK_RET( m_Listing, "can't filter new mail without valid listing" );

   // the problem with filters is that they can both delete messages from
   // the folder *and* show message boxes which can send requests from GUI,
   // so we have to block sending events to the GUI while processing them
   // (this is not stictly necessary as we already have a listing, so there
   // is no reentrancy problem: if GetHeaders() is called once again, it
   // will just return immediately, but this is quite inefficient as the
   // folder views don't have to be updated to reflect any intermidiate
   // states)
   if ( m_GotNewMessages )
   {
      // we don't want to process any events while we're filtering mail as
      // there can be a lot of them and all of them can be processed later
      MEventManagerSuspender suspendEvents;

      MLocker lockFilters(m_InFilterCode);

      // filter code returns true if something might have changed
      m_Listing->IncRef();
      if ( FilterNewMail(m_Listing) )
      {
         // rebuild the listing again
         m_Listing->DecRef();
         m_Listing = NULL;

         // TODO: expunge all messages deleted by filters - but how to do it
         //       without expunging the messages previously deleted by user?

         BuildListing();
      }
   }
}

// rebuild the folder listing completely
void
MailFolderCC::BuildListing(void)
{
   // shouldn't be done unless it's really needed
   CHECK_RET( !m_Listing, "BuildListing() called but we have listing" );

   CHECK_DEAD("Cannot access closed folder\n'%s'.");

   MailFolderLocker lockFolder(this);

   wxLogTrace(TRACE_MF_CALLS, "Building listing for folder '%s'...",
              GetName().c_str());

   MEventLocker locker;

   // we don't want MEvents to be handled while we are in here, as
   // they might query this folder
   MEventManagerSuspender noEvents;

#ifdef TIMER_SUSPENDING_WORKS
   // neither do we want our PingReopen() be called in the middle of update
   MTimerSuspender noTimer(MAppBase::Timer_PollIncoming);
#endif // TIMER_SUSPENDING_WORKS

   MLocker lockListing(m_InListingRebuild);

   // update the number of total and recent messages first
   m_nMessages =
   m_msgnoMax = m_MailStream->nmsgs;
   m_nRecent = m_MailStream->recent;

   // if the number of the messages in the folder is bigger than the
   // configured limit, ask the user whether he really wants to retrieve them
   // all. The value of 0 disables the limit. Ask only once and never for file
   // folders (loading headers from them is quick)
   if ( !IsLocalQuickFolder(GetType()) && m_FirstListing )
   {
      unsigned long retrLimit = m_RetrievalLimit;
      if ( m_RetrievalLimitHard > 0 && m_RetrievalLimitHard < retrLimit )
      {
         // hard limit takes precedence over the soft one
         retrLimit = m_RetrievalLimitHard;
      }
            
      if ( (retrLimit > 0) && (m_nMessages > retrLimit) )
      {
         // too many messages: if we exceeded hard limit, just use it instead
         // of m_msgnoMax, otherwise ask the user how many of them he really
         // wants
         long nRetrieve;
         if ( retrLimit == m_RetrievalLimitHard )
         {
            nRetrieve = m_RetrievalLimitHard;
         }
         else // soft limit exceeded
         {
            // ask the user (in interactive mode only)
            MFrame *frame = GetInteractiveFrame();
            if ( frame )
            {
               MGuiLocker locker;

               String msg, title;
               msg.Printf(
                  _("This folder contains %lu messages, which is greater than\n"
                    "the current threshold of %lu (set it to 0 to avoid this "
                    "question - or\n"
                    "you can also choose [Cancel] to download all messages)."),
                  m_nMessages, m_RetrievalLimit
               );
               title.Printf(_("How many messages to retrieve from folder '%s'?"),
                            GetName().c_str());

               String prompt = _("How many of them do you want to retrieve?");

               nRetrieve = MGetNumberFromUser(msg, prompt, title,
                                              m_RetrievalLimit,
                                              1, m_nMessages,
                                              frame);
            }
            else // not interactive mode
            {
               // get all messages - better do this than present the user with
               // an annoying modal dialog (he can always set the hard limit
               // if he really doesn't want to limit the number of messages
               // retrieved)
               nRetrieve = 0;
            }
         }

         if ( nRetrieve > 0 && (unsigned long)nRetrieve < m_nMessages )
         {
            m_nMessages = nRetrieve;

            // FIXME: how to calculate the number of recent messages now? we
            //        clearly can't leave it as it is/was because not all
            //        recent messages are among the ones we retrieved
            m_nRecent = UID_ILLEGAL;
         }
         //else: cancelled or 0 or invalid number entered, retrieve all
      }
   }

   // create the new listing
   m_Listing = HeaderInfoListImpl::Create(m_nMessages, m_msgnoMax);

   // set the entry listing we're currently building to 0 in the beginning
   m_BuildNextEntry = 0;

   // show the progress dialog if there are many messages to retrieve
   // (see below for the reason of the m_ProgressDialog == NULL check, the
   // check for threshold > 0 allows to disable the progress dialog
   // completely if the user wants)
   long threshold = READ_CONFIG(m_Profile, MP_FOLDERPROGRESS_THRESHOLD);
   if ( m_ProgressDialog == NULL &&
        threshold > 0 &&
        m_nMessages > (unsigned long)threshold )
   {
      String msg;
      msg.Printf(_("Reading %lu message headers..."), m_nMessages);
      MGuiLocker locker;

      // open a status window:
      m_ProgressDialog = new MProgressDialog(GetName(),
                                             msg,
                                             m_nMessages,
                                             NULL,
                                             false, true);
   }

   // do we have any messages to get?
   if ( m_nMessages )
   {
      // find the first and last messages to retrieve
      UIdType from = mail_uid(m_MailStream,
                              m_MailStream->nmsgs - m_nMessages + 1),
              to = mail_uid(m_MailStream, m_nMessages);

      String sequence = strutil_ultoa(from);

      // don't produce sequences like "1:1" or "1:*"
      if ( to != from )
      {
         sequence << ":*";
      }

      // do fill the listing by calling mail_fetch_overview_x() which will
      // call OverviewHeaderEntry() for each entry

      // for some folders (MH) this call generates mm_exists notification
      // resulting in an infinite recursion
      bool oldCB = mm_disable_callbacks;
      mm_disable_callbacks = true;
      mail_fetch_overview_x(m_MailStream,
                            (char *)sequence.c_str(),
                            mm_overview_header);
      mm_disable_callbacks = oldCB;

      // destroy the progress dialog if it had been shown
      if ( m_ProgressDialog != NO_PROGRESS_DLG )
      {
         MGuiLocker locker;
         delete m_ProgressDialog;
      }

      // We set it to an illegal address here to suppress further updating.
      // This value is checked against in OverviewHeader() and also when we're
      // called for the next time. The reason is that we only want it the
      // first time that the folder is being opened because the subsequent
      // calls are supposed to be fast as cclient caches the listing
      // internally
      m_ProgressDialog = NO_PROGRESS_DLG;

      // it is possible that we didn't retrieve all messages if the progress
      // dialog was shown and cancelled, but we shouldn't have retrieved more
      // than we had aske for
      if ( m_BuildNextEntry < m_nMessages )
      {
         m_Listing->SetCount(m_BuildNextEntry);
         m_nMessages = m_BuildNextEntry;
      }
      else
      {
         ASSERT_MSG( m_BuildNextEntry == m_nMessages, "message count mismatch" );
      }
   }
   //else: no messages, perfectly valid if the folder is empty

   m_FirstListing = false;

   wxLogTrace(TRACE_MF_CALLS, "Finished building listing for folder '%s'...",
              GetName().c_str());
}

/* static */
String
MailFolderCC::ParseAddress(ADDRESS *adr)
{
   String from;

   /* get first from address from envelope */
   while ( adr && !adr->host )
      adr = adr->next;

   if(adr)
   {
      from = "";
      if (adr->personal) // a personal name is given
         from << adr->personal;
      if(adr->mailbox)
      {
         if(adr->personal)
            from << " <";
         from << adr->mailbox;
         if(adr->host && strlen(adr->host)
            && (strcmp(adr->host,BADHOST) != 0))
            from << '@' << adr->host;
         if(adr->personal)
            from << '>';
      }
   }
   else
      from = _("<address missing>");

   return from;
}

int
MailFolderCC::OverviewHeaderEntry (unsigned long uid,
                                   OVERVIEW_X *ov)
{
   // it is possible that new messages have arrived in the meantime, ignore
   // them
   if ( m_BuildNextEntry == m_nMessages )
   {
      wxLogDebug("New message(s) appeared in folder while overviewing it, "
                 "ignored.");

      // stop overviewing
      return 0;
   }

   // after the check above this is not supposed to happen
   CHECK( m_BuildNextEntry < m_nMessages, 0, "too many messages" );

   if ( m_ProgressDialog && m_ProgressDialog != NO_PROGRESS_DLG )
   {
      MGuiLocker locker;
      if(! m_ProgressDialog->Update( m_BuildNextEntry ))
      {
         // cancelled by user
         return 0;
      }
   }

   unsigned long msgno = mail_msgno (m_MailStream, uid);

   // as we overview the messages in the reversed order (see comments before
   // mail_fetch_overview_x()) and msgnos are consecutive we should always have
   // this -- and maybe we can even save the call to mail_msgno() above
   ASSERT_MSG( msgno == m_msgnoMax - m_BuildNextEntry,
               "msgno and listing index out of sync" );

   MESSAGECACHE *elt = mail_elt (m_MailStream, msgno);
   MESSAGECACHE selt;

   HeaderInfoImpl& entry = *(HeaderInfoImpl *)
                              m_Listing->GetItemByIndex(m_BuildNextEntry);

   // STATUS:
   entry.m_Status = GetMsgStatus(elt);

   /* For NNTP, do not show deleted messages: */
   if(m_folderType == MF_NNTP && elt->deleted)
      return 1; // ignore but continue

   if(ov)
   {
      // DATE
      mail_parse_date (&selt,ov->date);
      entry.m_Date = (time_t) mail_longdate( &selt);

      // FROM and TO
      entry.m_From = ParseAddress(ov->from);
      if(m_folderType == MF_NNTP || m_folderType == MF_NEWS)
      {
         entry.m_To = ""; // no To: for news postings
      }
      else
      {
         entry.m_To = ParseAddress( ov->to );
      }

      wxFontEncoding encoding;
      entry.m_To = DecodeHeader(entry.m_To, &encoding);
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

      entry.m_Subject = DecodeHeader(ov->subject, &encoding);
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

      entry.m_Size = ov->optional.octets;
      entry.m_Id = ov->message_id;
      entry.m_References = ov->references;
      entry.m_UId = uid;

      // set the font encoding to be used for displaying this entry
      entry.m_Encoding = encodingMsg;

      if ( m_ProgressDialog && m_ProgressDialog != NO_PROGRESS_DLG )
      {
         MGuiLocker locker;
         m_ProgressDialog->Update(m_BuildNextEntry);
      }

      m_BuildNextEntry++;
   }
   else
   {
      // parsing the message failed in c-client lib. Why?
      wxLogDebug("Parsing folder overview failed for message %lu", uid);

      // don't do anything
   }

   // continue
   return 1;
}

// ----------------------------------------------------------------------------
// CClient initialization and clean up
// ----------------------------------------------------------------------------

void
MailFolderCC::CClientInit(void)
{
   if(ms_CClientInitialisedFlag == true)
      return;

   // do further initialisation
#include <linkage.c>

   // this triggers c-client initialisation via env_init()
#ifdef OS_UNIX
   if(geteuid() != 0) // running as root, skip init:
#endif
      (void *) myusername();

   // 1 try is enough, the default (3) is too slow
   mail_parameters(NULL, SET_MAXLOGINTRIALS, (void *)1);

#ifdef OS_UNIX
   // install our own sigpipe handler:
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
}

extern bool CC_Init(void)
{
   MailFolderCC::CClientInit();

   return true;
}

extern void CC_Cleanup(void)
{
   if ( MailFolderCC::IsInitialized() )
   {
      // as c-client lib doesn't seem to think that deallocating memory is
      // something good to do, do it at it's place...
      free(mail_parameters((MAILSTREAM *)NULL, GET_HOMEDIR, NULL));
      free(mail_parameters((MAILSTREAM *)NULL, GET_NEWSRC, NULL));
   }

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

CCStreamCleaner::~CCStreamCleaner()
{
   wxLogTrace(TRACE_MF_CALLS, "CCStreamCleaner: checking for left-over streams");

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
      CC_Init();

      gs_NewsSpoolDir = (char *)mail_parameters(NULL, GET_NEWSSPOOL, NULL);
      if ( !gs_NewsSpoolDir )
      {
         gs_NewsSpoolDir = READ_APPCONFIG(MP_NEWS_SPOOL_DIR);
         mail_parameters(NULL, SET_NEWSSPOOL, (char *)gs_NewsSpoolDir.c_str());
      }
   }

   return gs_NewsSpoolDir;
}

// ----------------------------------------------------------------------------
// callbacks called asynchronously by cclient
// ----------------------------------------------------------------------------

/// this message matches a search
void
MailFolderCC::mm_searched(MAILSTREAM * stream,
                          unsigned long msgno)
{
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
   {
      ASSERT(mf->m_SearchMessagesFound);
      mf->m_SearchMessagesFound->Add(msgno);
   }
}

/// tells program that there are this many messages in the mailbox
void
MailFolderCC::mm_exists(MAILSTREAM *stream, unsigned long msgno)
{
   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET( mf, "number of messages changed in unknown mail folder" );

   if ( msgno > mf->m_msgnoMax )
   {
      mf->m_msgnoMax = msgno;

      // new message(s) arrived, we need to reapply the filters
      mf->m_GotNewMessages = true;

      // rebuild the listing completely
      //
      // TODO: should just add new messages to it!
      if ( mf->m_Listing )
      {
         mf->m_Listing->DecRef();
         mf->m_Listing = NULL;

         // invalidate it as the number of messages changed and we don't have
         // the status of the new message(s) here unfortunately
         mf->m_status.total = UID_ILLEGAL;
      }
   }
   else // the messages were expunged
   {
      // msgno should never be less than m_nMessages because we adjust the
      // latter in mm_expunged immediately when a message is expunged
      ASSERT_MSG( msgno == mf->m_msgnoMax, "msg number unexpected changed" );

      // only request update below if something happened: the only thing which
      // can happen to us in this case is that messages were expunged from the
      // folder, so check for this
      if ( !mf->m_expunged )
      {
         // no, they were not - don't refresh everything!
         return;
      }

      // as we're going to update it below, we can clear the flag
      mf->m_expunged = false;
   }

   // don't request update if we are only half opened: this will cause another
   // attempt to open the folder (which will fail because it can't be opened)
   // and so lead to an infinite loop - besides it's plainly not needed as we
   // don't have any messages in a half open folder anyhow
   if ( stream && !stream->halfopen )
   {
      mf->RequestUpdate();
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
   // I don't really know what the difference with mm_log is...
   mm_log(str, errflg, MailFolderCC::LookupObject(stream));
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
   wxYield();
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
   CHECK_RET(mf, "mm_status for non existent folder");

   wxLogTrace(TRACE_MF_CALLBACK, "mm_status: folder '%s', %lu messages",
              mf->m_ImapSpec.c_str(), status->messages);

   if ( status->flags & SA_MESSAGES )
      mf->RequestUpdate();
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

   String  msg;
   if(mf)
      msg.Printf(_("Folder '%s' : "), mf->GetName().c_str());
   else
      msg = _("Folder Log: ");
   msg += str;
#ifdef DEBUG
   msg << _(", error level: ") << strutil_ultoa(errflg);
#endif

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
   String msg;

   // replace the passwords in the log output with stars if plain text
   // authentification is used (TODO: check for IMAP "LOGIN" too)
   if ( str[0u] == 'P' && str[1u] == 'A' && str[2u] == 'S' && str[3u] == 'S' )
   {
      msg = "PASS";

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
      msg = str;
   }

   GetLogCircle().Add(msg);

   if ( mm_show_debug )
   {
      wxLogGeneric(wxLOG_User, _("Mail debug: %s"), msg.c_str());
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
   strcpy(user,MF_user.c_str());
   strcpy(pwd,MF_pwd.c_str());
}

/* static */
void
MailFolderCC::mm_expunged(MAILSTREAM * stream, unsigned long msgno)
{
   MailFolderCC *mf = LookupObject(stream);
   CHECK_RET(mf, "mm_expunged for non existent folder");

   // don't do anything while executing the filter code, the listing is going
   // to be rebuilt anyhow
   if ( !mf->CanSendUpdateEvents() )
   {
      DBGMESSAGE(("Ignoring mm_expunged() for folder '%s'",
                  mf->GetName().c_str()));
      return;
   }

   if ( mf->HaveListing() )
   {
      // remove one message from listing
      wxLogTrace(TRACE_MF_CALLBACK,
                 "Removing element %lu from listing", msgno);

      HeaderInfoList_obj hil = mf->GetHeaders();

      size_t idx = hil->GetIdxFromMsgno(msgno);
      CHECK_RET( idx < mf->m_nMessages, "invalid msgno in mm_expunged()" );

      mf->m_Listing->Remove(idx);

      ASSERT_MSG( mf->m_nMessages > 0,
                  "expunged a message when we don't have any?" );

      mf->m_expunged = true;

      mf->m_nMessages--;
      mf->m_msgnoMax--;

      // it is not necessary to update the other m_status fields because they
      // were already updated when the message was deleted, but this one must
      // be kept in sync manually as it wasn't updated before
      if ( mf->m_status.IsValid() )
         mf->m_status.total--;
   }
   //else: no need to do anything right now
}

/* static */
void
MailFolderCC::mm_flags(MAILSTREAM * stream, unsigned long msgno)
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
// listing updating
// ----------------------------------------------------------------------------

bool MailFolderCC::CanSendUpdateEvents() const
{
   // we never send update events while the listing is being changed by filter
   // code as it's at the very best inefficient (the listing is going to be
   // regenerated anyhow after we're done with filters) and in the worst case
   // can be fatal
   //
   // also, if the application is shutting down, there is no point in send
   // events
   return !m_InFilterCode->IsLocked() && !mApplication->IsShuttingDown();
}

void
MailFolderCC::UpdateMessageStatus(unsigned long msgno)
{
   /* If a new listing is required, we don't need to do anything to
      update the old one. Only exception is if the listing is frozen,
      in that case we try to update it but don't sent a status change
      event. */
   if( !HaveListing() )
      return; // will be regenerated anyway

   // Otherwise: update the current listing information:
   HeaderInfoList_obj hil = GetHeaders();

   // Get the listing entry for this message:
   size_t idx = hil->GetIdxFromMsgno(msgno);
   HeaderInfo *hi = hil->GetItemByIndex(idx);

   // this should never happen - we must have this message somewhere!
   CHECK_RET( hi, "no HeaderInfo in mm_flags" );

   MESSAGECACHE *elt = mail_elt (m_MailStream, msgno);
   int status = GetMsgStatus(elt),
       statusOld = hi->GetStatus();
   if ( status != statusOld )
   {
      // update the cached message number
      if ( m_status.IsValid() )
      {
         bool wasDeleted = (statusOld & MSG_STAT_DELETED) != 0,
              isDeleted = (status & MSG_STAT_DELETED) != 0;

         MsgStatus msgStatusOld = AnalyzeStatus(statusOld),
                   msgStatus = AnalyzeStatus(status);

         // we consider that a message has some flag only if it is not deleted
         // (which is discussable at least for flagged and searched flags
         // although, OTOH, why flag a deleted message?)

         #define UPDATE_NUM_OF(what)   \
            if ( (!isDeleted && msgStatus.what) && \
                 (wasDeleted || !msgStatusOld.what) ) \
               m_status.what++; \
            else if ( (!wasDeleted && msgStatusOld.what) && \
                      (isDeleted || !msgStatus.what) ) \
               if ( m_status.what > 0 ) \
                  m_status.what--; \
               else \
                  FAIL_MSG( "error in msg status change logic" )

         UPDATE_NUM_OF(recent);
         UPDATE_NUM_OF(unseen);
         UPDATE_NUM_OF(newmsgs);
         UPDATE_NUM_OF(flagged);
         UPDATE_NUM_OF(searched);

         #undef UPDATE_NUM_OF
      }

      ((HeaderInfoImpl *)hi)->m_Status = status;

      // tell all interested that status changed
      wxLogTrace(TRACE_MF_EVENTS, "Sending MsgStatus event for folder '%s'",
                 GetName().c_str());

      size_t pos = hil->GetPosFromIdx(idx);
      MEventManager::Send(new MEventMsgStatusData(this, pos, hi->Clone()));
   }
   //else: flags didn't really change
}

void
MailFolderCC::UpdateStatus(void)
{
   // TODO: what is this supposed to do?
}

void
MailFolderCC::RequestUpdate()
{
   // don't do anything while executing the filter code, the listing is going
   // to be rebuilt anyhow
   if ( !CanSendUpdateEvents() )
   {
      DBGMESSAGE(("Ignoring mm_exists() for folder '%s'",
                 GetName().c_str()));

      return;
   }

   // tell all interested that the listing changed
   wxLogTrace(TRACE_MF_EVENTS, "Sending FolderUpdate event for folder '%s'",
              GetName().c_str());

   MEventManager::Send(new MEventFolderUpdateData(this));
}

/* Handles the mm_overview_header callback on a per folder basis. */
int
MailFolderCC::OverviewHeader(MAILSTREAM *stream,
                             unsigned long uid,
                             OVERVIEW_X *ov)
{
   CHECK_STREAM_LIST();

   MailFolderCC *mf = MailFolderCC::LookupObject(stream);
   CHECK(mf, 0, "trying to build overview for non-existent folder");

   return mf->OverviewHeaderEntry(uid, ov);
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
   if ( !!spec )
   {
      char ch = spec.Last();

      switch ( GetType() )
      {
         case MF_IMAP:
            // FIXME: this is totally bogus, IMAP delimiter may be any char,
            //        not only '/' or '.' even if they are the most common
            //        ones - why do we do this at all?
            if ( ch != '/' && ch != '.' && ch != '}' )
               spec += '/';
            break;

         case MF_MH:
            if ( ch != '/' )
               spec += '/';
            break;

         case MF_NNTP:
         case MF_NEWS:
            if ( ch != '.' && ch != '}' )
               spec += '.';
            break;

         default:
            FAIL_MSG( "unexpected folder type in ListFolders" );
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


/** Physically deletes this folder.
    @return true on success
    */
/* static */
bool
MailFolderCC::DeleteFolder(const MFolder *mfolder)
{
   String mboxpath = MailFolder::GetImapSpec(mfolder->GetType(),
                                             mfolder->GetFlags(),
                                             mfolder->GetPath(),
                                             mfolder->GetServer(),
                                             mfolder->GetLogin());

   String password = mfolder->GetPassword();
   if ( FolderTypeHasUserName( mfolder->GetType() )
        && ((mfolder->GetFlags() & MF_FLAGS_ANON) == 0)
        && (password.Length() == 0)
      )
   {
      String prompt;
      prompt.Printf(_("Please enter the password for folder '%s':"),
                    mfolder->GetName().c_str());
      if(! MInputBox(&password,
                     _("Password required"),
                     prompt, NULL,
                     NULL,NULL, true))
      {
          ERRORMESSAGE((_("Cannot access this folder without a password.")));
          mApplication->SetLastError(M_ERROR_AUTH);
          return FALSE;
      }
   }

   MCclientLocker lock;
   SetLoginData(mfolder->GetLogin(), password);
   bool rc = mail_delete(NIL, (char *) mboxpath.c_str()) != NIL;
   lock.Unlock();
   return rc;
}

// ----------------------------------------------------------------------------
// HasInferiors() function: check if the folder has subfolders
// ----------------------------------------------------------------------------

/*
  Small function to check if the mailstream has \inferiors flag set or
  not, i.e. if it is a mailbox or a directory on the server.
*/

static int gs_HasInferiorsFlag;

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
   MAILSTREAM *mailStream = mail_open(NIL, (char *)imapSpec.c_str(),
                                      OP_HALFOPEN);

   if(mailStream != NIL)
   {
     gs_HasInferiorsFlag = -1;
     gs_mmListRedirect = HasInferiorsMMList;
     mail_list (mailStream, NULL, (char *) imapSpec.c_str());
     gs_mmListRedirect = NULL;
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
// replacements of some cclient functions
// ----------------------------------------------------------------------------

/*
 We have our version of cclient mail_fetch_overview() because:

 1. we need the "To" header which is not retrieved by mail_fetch_overview()

 2. we want to allow the user to abort retrieving the headers

 3. we want to retrieve the headers from larger msgnos down to lower ones,
    i.e. in the reverse order to mail_fetch_overview() - this is useful if we're
    aborted

 Note that now the code elsewhere relies on messages being fetched in reverse
 order, so it shouldn't be changed!
*/

void mail_fetch_overview_x(MAILSTREAM *stream, char *sequence, overview_x_t ofn)
{
   if ( mail_uid_sequence (stream,sequence) && mail_ping (stream) )
   {
      OVERVIEW_X ov;
      ov.optional.lines = 0;
      ov.optional.xref = NIL;

      for ( unsigned long i = stream->nmsgs; i>= 1; i-- )
      {
         MESSAGECACHE *elt;
         ENVELOPE *env = NULL;
         if (((elt = mail_elt (stream,i))->sequence) &&
               (env = mail_fetch_structure (stream,i,NIL,NIL)) && ofn)
         {
            ov.subject = env->subject;
            ov.from = env->from;
            ov.to = env->to;
            ov.date = env->date;
            ov.message_id = env->message_id;
            ov.references = env->references;
            ov.optional.octets = elt->rfc822_size;
            if(! (*ofn) (stream,mail_uid (stream,i),&ov))
               break;
         }
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
      OVERVIEW_X ov;
      unsigned long i;
      ov.optional.lines = 0;
      ov.optional.xref = NIL;
      for (i = 1; i <= stream->nmsgs; i++)
         if (((elt = mail_elt (stream,i))->sequence) &&
             (env = mail_fetch_structure (stream,i,NIL,NIL)) && ofn)
         {
            ov.subject = env->subject;
            ov.from = env->from;
            ov.to = env->to;
            ov.date = env->date;
            ov.message_id = env->message_id;
            ov.references = env->references;
            ov.optional.octets = elt->rfc822_size;
            (*ofn) (stream,mail_uid (stream,i),&ov);
         }
   }
}

// ----------------------------------------------------------------------------
// C-Client callbacks
// ----------------------------------------------------------------------------

// define a macro TRACE_CALLBACKn() for tracing a callback with n + 1 params
#if 1 // def EXPERIMENTAL_log_callbacks
   #define TRACE_CALLBACK0(name) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(%s)", \
                 #name, stream ? stream->mailbox : "<no stream>")

   #define TRACE_CALLBACK1(name, fmt, parm) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(%s, " fmt ")", \
                 #name, stream->mailbox, parm)

   #define TRACE_CALLBACK2(name, fmt, parm1, parm2) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(%s, " fmt ")", \
                 #name, stream->mailbox, parm1, parm2)

   #define TRACE_CALLBACK3(name, fmt, parm1, parm2, parm3) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(%s, " fmt ")", \
                 #name, stream->mailbox, parm1, parm2, parm3)

   #define TRACE_CALLBACK_NOSTREAM_1(name, fmt, parm1) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(" fmt ")", \
                 #name, parm1)

   #define TRACE_CALLBACK_NOSTREAM_2(name, fmt, parm1, parm2) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(" fmt ")", \
                 #name, parm1, parm2)

   #define TRACE_CALLBACK_NOSTREAM_5(name, fmt, parm1, parm2, parm3, parm4, parm5) \
      wxLogTrace(TRACE_MF_CALLBACK, "%s(" fmt ")", \
                 #name, parm1, parm2, parm3, parm4, parm5)

   static const char *GetErrorLevel(long errflg)
   {
      return errflg == ERROR ? "error"
                             : errflg == WARN ? "warn"
                                              : errflg == PARSE ? "parse"
                                                                : "other";
   }
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
   if ( !mm_disable_callbacks )
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
   if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK_NOSTREAM_1(mm_dlog, "%s", str);

      MailFolderCC::mm_dlog(str);
   }
}

void
mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
   if ( !mm_disable_callbacks )
   {
      TRACE_CALLBACK_NOSTREAM_5(mm_login, "%s %s@%s:%ld, try #%ld",
                                mb->service, mb->user, mb->host, mb->port,
                                trial);

      MailFolderCC::mm_login(mb, user, pwd, trial);
   }
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

static int
mm_overview_header (MAILSTREAM *stream, unsigned long uid, OVERVIEW_X *ov)
{
   // this one is never ignored as it always is generated at our request
   TRACE_CALLBACK1(mm_overview_header, "%lu", uid);

   return MailFolderCC::OverviewHeader(stream, uid, ov);
}

} // extern "C"

