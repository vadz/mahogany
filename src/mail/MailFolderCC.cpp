/*-*- C++ -*-********************************************************
* MailFolderCC class: handling of mail folders with c-client lib   *
*                                                                  *
* (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
*                                                                  *
* $Id$
*******************************************************************/


/*
  Some attempt to document the update behaviour.

  When a callback (mm_xxxx()) thinks that the folder listing needs
  updating, i.e. a message status changed, new messages got detected,
  etc, it calls RequestUpdate(). This function updates the available
  information immediately (the number of messages) and queues a
  MailFolderCC::Event which will cause the folder listing update
  MEvent to be sent out to the application after the current folder
  operation is done. To update all immediately available information,
  UpdateStatus() is used which will also generate new mail events if
  necessary.
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
#endif // USE_PCH

#include "Mdefaults.h"
#include "MDialogs.h"
#include "MPython.h"
#include "FolderView.h"

#include "MailFolderCC.h"
#include "MessageCC.h"

#include "MEvent.h"
#include "ASMailFolder.h"
#include "Mpers.h"

#include "HeaderInfoImpl.h"

// just to use wxFindFirstFile()/wxFindNextFile() for lockfile checking
#include <wx/filefn.h>

// DecodeHeader() uses CharsetToEncoding()
#include <wx/fontmap.h>

#include <ctype.h>   // isspace()

#ifdef OS_UNIX
#include <signal.h>

static void sigpipe_handler(int)
{
   // do nothing
}
#endif

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#define CHECK_DEAD(msg)   if(m_MailStream == NIL && ! PingReopen()) { ERRORMESSAGE((_(msg), GetName().c_str())); return; }
#define CHECK_DEAD_RC(msg, rc)   if(m_MailStream == NIL && ! PingReopen()) {   ERRORMESSAGE((_(msg), GetName().c_str())); return rc; }

// enable this as needed as it produces *lots* of outputrun
#ifdef DEBUG_STREAMS
#   define CHECK_STREAM_LIST() MailFolderCC::DebugStreams()
#else // !DEBUG
#   define CHECK_STREAM_LIST()
#endif // DEBUG/!DEBUG

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------


static const char * cclient_drivers[] =
{ "mbx", "unix", "mmdf", "tenex" };
#define CCLIENT_MAX_DRIVER 3

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
#ifdef DEBUG
         LOGMESSAGE((M_LOG_DEBUG, "CCStreamCleaner: closing stream"));
#endif
         mail_close(*i);
      }
   }
}

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

/// global cclient lock (FIXME: MT)
static int gs_lockCclient = 0;

/// exactly one object
static CCStreamCleaner *gs_CCStreamCleaner = NULL;

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

void CC_Cleanup(void)
{
   if ( MailFolderCC::IsInitialized() )
   {
      // as c-client lib doesn't seem to think that deallocating memory is
      // something good to do, do it at it's place...
      free(mail_parameters((MAILSTREAM *)NULL, GET_HOMEDIR, NULL));
      free(mail_parameters((MAILSTREAM *)NULL, GET_NEWSRC, NULL));
   }

   if(gs_CCStreamCleaner)
   {
      delete gs_CCStreamCleaner;
      gs_CCStreamCleaner = NULL;
   }
}

static String GetImapFlags(int flag)
{
   String flags;
   if(flag & MailFolder::MSG_STAT_SEEN)
      flags <<"\\SEEN ";
// RECENT flag cannot be set?
//   if(flag & MailFolder::MSG_STAT_RECENT)
//      flags <<"\\RECENT ";
   if(flag & MailFolder::MSG_STAT_ANSWERED)
      flags <<"\\ANSWERED ";
   if(flag & MailFolder::MSG_STAT_DELETED)
      flags <<"\\DELETED ";
   if(flag & MailFolder::MSG_STAT_FLAGGED)
      flags <<"\\FLAGGED ";
   if(flags.Length() > 0) // chop off trailing space
     flags.Trim();
   return flags;
}

extern String GetImapSpec(int type, int flags,
                          const String &name,
                          const String &iserver,
                          const String &login)
{
   String mboxpath;

   String server = iserver;
   strutil_tolower(server);
   
#ifdef USE_SSL
   bool InitSSL(void);

   // SSL only for NNTP/IMAP/POP:
   if(((flags & MF_FLAGS_SSLAUTH) != 0)
      && ! FolderTypeSupportsSSL( (FolderType) type))
   {
      flags ^= MF_FLAGS_SSLAUTH;
      STATUSMESSAGE((_("Ignoring SSL authentication for folder '%s'"), name.c_str()));
   }

   if( (flags & MF_FLAGS_SSLAUTH) != 0
       && ! InitSSL() )
#else
   if( (flags & MF_FLAGS_SSLAUTH) != 0 )
#endif
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
      mboxpath = strutil_expandfoldername(name, (FolderType) type);
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

/*
static
String GetHostFromImapSpec(const String &imap)
{
   if(imap[0] != '{') return "";
   String host;
   const char *cptr = imap.c_str()+1;
   while(*cptr && *cptr != '/' && *cptr != '}')
      host << *cptr++;
   return host;
}
*/
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


// ============================================================================
// implementation
// ============================================================================

/* This allows us to temporarily redirect mm_list calls: */
static void (*gs_mmListRedirect)(MAILSTREAM * stream,
                                 char delim,
                                 String  name,
                                 long  attrib) = NULL;

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

   SetLoginData(login, passwd);
   MAILSTREAM *mailStream = mail_open(NIL, (char *)imapSpec.c_str(),
                                      OP_HALFOPEN);
   ProcessEventQueue();
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

            continue; // break, in fact
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




/*----------------------------------------------------------------------------------------
 * MailFolderCC code
 *---------------------------------------------------------------------------------------*/
String MailFolderCC::MF_user;
String MailFolderCC::MF_pwd;


/* static */
void
MailFolderCC::SetLoginData(const String &user, const String &pw)
{
   MailFolderCC::MF_user = user;
   MailFolderCC::MF_pwd = pw;
}

// a variable telling c-client to shut up
static bool mm_ignore_errors = false;
/// a variable disabling all events
static bool mm_disable_callbacks = false;
/// show cclient debug output
#ifdef DEBUG
static bool mm_show_debug = true;
#else
static bool mm_show_debug = false;
#endif


// be quiet
static inline void CCQuiet(bool disableCallbacks = false)
{
   mm_ignore_errors = true;
   if(disableCallbacks) mm_disable_callbacks = true;
}

// normal logging
static inline void CCVerbose(void)
{
   mm_ignore_errors = mm_disable_callbacks = false;
}

/* static */
void
MailFolderCC::UpdateCClientConfig()
{
   mm_show_debug = READ_APPCONFIG(MP_DEBUG_CCLIENT) != 0;
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
   UpdateConfig();

   if(! ms_CClientInitialisedFlag)
      CClientInit();
   Create(typeAndFlags);
   if(GetType() == MF_FILE)
      m_ImapSpec = strutil_expandpath(path);
   else
         m_ImapSpec = path;
   m_Login = login;
   m_Password = password;
}

void
MailFolderCC::Create(int typeAndFlags)
{
   m_MailStream = NIL;

   m_InCritical = false;

   UpdateTimeoutValues();

   SetRetrievalLimit(0); // no limit
   m_nMessages = 0;
   m_nRecent = 0;
   m_LastUId = UID_ILLEGAL;
   m_OldNumOfMessages = 0;
   m_Listing = NULL;
   m_NeedFreshListing = true;
   m_ListingFrozen = false;
   m_ExpungeRequested = FALSE;
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

   // we might still be listed, so we better remove ourselves from the
   // list to make sure no more events get routed to this (now dead) object
   RemoveFromMap();
   
#ifdef USE_THREADS
   delete m_Mutex;
#endif

   if( m_Listing )
   {
      m_Listing->DecRef();
      m_Listing = NULL;
   }
   if(m_Profile)
      m_Profile->DecRef();
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
   CClientInit();

   MailFolderCC *mf;
   String mboxpath;

   ASSERT(profile);

   int flags = GetFolderFlags(typeAndFlags);
   int type = GetFolderType(typeAndFlags);

   String login = loginGiven;
   mboxpath = ::GetImapSpec(type, flags, name, server, login);

   //FIXME: This should somehow be done in MailFolder.cc
   mf = FindFolder(mboxpath,login);
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

/* DEBUG only to test
  if(type == MF_IMAP)
   {
      bool inferiors = HasInferiors(mboxpath, login, password);
   }
*/ 
   mf = new
      MailFolderCC(typeAndFlags, mboxpath, profile, server, login, password);
   mf->m_Name = symname;
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

#if 0
   // check if our folder flags are suitable or not
   if( GetFolderType(typeAndFlags) == MF_IMAP )
   {
      Ticket ticket = ASMailFolder::GetTicket();
      ListFolders(NULL /* asmf */,
                  mboxpath, FALSE, "" /* reference */,
                  NULL /* userdata */, ticket/* Ticket */);
   }
#endif
   
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

   //FIXME: is this really needed if(mf) mf->PingReopen();
   return mf;
}

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


bool MailFolderCC::HalfOpen()
{
   // well, we're not going to tell the user we're half opening it...
   STATUSMESSAGE((_("Opening mailbox %s..."), GetName().c_str()));

   if( FolderTypeHasUserName(GetType()) )
      SetLoginData(m_Login, m_Password);

   MCclientLocker lock;
   SetDefaultObj();
   CCVerbose();
   m_MailStream = mail_open(NIL, (char *)m_ImapSpec.c_str(),
                            (mm_show_debug ? OP_DEBUG : NIL)|OP_HALFOPEN);
   ProcessEventQueue();
   SetDefaultObj(false);

   if ( !m_MailStream )
      return false;

   AddToMap(m_MailStream);

   return true;
}

bool
MailFolderCC::Open(void)
{
   STATUSMESSAGE((_("Opening mailbox %s..."), GetName().c_str()));


   /** Now, we apply the very latest c-client timeout values, in case
       they have changed.
   */
   UpdateTimeoutValues();

   if( FolderTypeHasUserName(GetType()) )
      SetLoginData(m_Login, m_Password);

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
            lockfile = m_ImapSpec;
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

      // create the mailbox if it doesn't exist yet
      bool exists = TRUE;
      if ( GetType() == MF_FILE )
      {
         exists = wxFileExists(m_ImapSpec);
      }
      else if ( GetType() == MF_MH )
      {
         // construct the filename from MH folder name
         String path;
         path << InitializeMH()
              << m_ImapSpec.c_str() + 4; // 4 == strlen("#mh/")
         exists = wxFileExists(path);
      }
      else
      {
         // for all other types we assume that it doesn't exist, as
         // IMAP servers allow us to open a non-existing mailbox but
         // not to write to it, so we force a creation attempt
         CCQuiet(); // always try creation
         mail_create(NIL, (char *)m_ImapSpec.c_str());
         CCVerbose();
      }
      
      // Make sure the event handling code knows us:
      SetDefaultObj();

      // if the file folder doesn't exist, we should create it first
      bool alreadyCreated = FALSE;
      if ( !exists
           && (GetType() == MF_FILE || GetType() == MF_MH))
      {
         // This little hack makes it root (uid 0) safe and allows us
         // to choose the file format, too:
         String tmp;
         if(GetType() == MF_FILE)
         {
            long format = READ_CONFIG(m_Profile, MP_FOLDER_FILE_DRIVER);
            ASSERT(! (format < 0  || format > CCLIENT_MAX_DRIVER));
            if(format < 0  || format > CCLIENT_MAX_DRIVER)
               format = 0;
            tmp = "#driver.";
            tmp << cclient_drivers[format] << '/';
               STATUSMESSAGE((_("Trying to create folder '%s' in %s format."),
                           GetName().c_str(),
                           cclient_drivers[format]
                  ));
         }
         else
            tmp = "#driver.mh/";
         tmp += m_ImapSpec;
         mail_create(NIL, (char *)tmp.c_str());
         //mail_create(NIL, (char *)m_ImapSpec.c_str());
         alreadyCreated = TRUE;
      }

      // first try, don't log errors (except in debug mode)
      m_MailStream = mail_open(m_MailStream,(char *)m_ImapSpec.c_str(),
                               mm_show_debug ? OP_DEBUG : NIL);

      // try to create it if hadn't tried yet
      if ( !m_MailStream && !alreadyCreated )
      {
         CCVerbose();
         mail_create(NIL, (char *)m_ImapSpec.c_str());
         m_MailStream = mail_open(m_MailStream,(char *)m_ImapSpec.c_str(),
                                  mm_show_debug ? OP_DEBUG : NIL);
      }

      ProcessEventQueue();
      SetDefaultObj(false);
   }

   if ( m_MailStream == NIL )
   {
      STATUSMESSAGE((_("Could not open mailbox %s."), GetName().c_str()));
      return false;
   }

   AddToMap(m_MailStream); // now we are known
   ProcessEventQueue();

   // listing already built

   // for some folders (notably the IMAP ones), mail_open() will return a
   // NULL pointer but set halfopen flag if it couldn't be SELECTed - as we
   // really want to open it here and not halfopen, this is an error for us
   ASSERT(m_MailStream);
   if ( m_MailStream->halfopen )
   {
      MFolder_obj(m_Profile)->AddFlags(MF_FLAGS_NOSELECT);

      mApplication->SetLastError(M_ERROR_HALFOPENED_ONLY);

      RemoveFromMap();
      return false;
   }
   else // folder really opened
   {
      STATUSMESSAGE((_("Mailbox %s opened."), GetName().c_str()));
      PY_CALLBACK(MCB_FOLDEROPEN, 0, GetProfile());
      return true;   // success
   }
}

MailFolderCC *
MailFolderCC::FindFolder(String const &path, String const &login)
{
   CHECK_STREAM_LIST();

   StreamConnectionList::iterator i;
   DBGMESSAGE(("Looking for folder to re-use: '%s',login '%s'",
               path.c_str(), login.c_str()));

   for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
   {
      StreamConnection *conn = *i;
      if( conn->name == path && conn->login == login)
      {
         DBGMESSAGE(("  Re-using entry: '%s',login '%s'",
                     conn->name.c_str(), conn->login.c_str()));
         return conn->folder;
      }
   }
   DBGMESSAGE(("  No matching entry found."));
   return NULL;
}

void
MailFolderCC::Checkpoint(void)
{
   if(m_MailStream == NULL)
     return; // nothing we can do anymore

   // nothing to do for halfopened folders (and doing CHECK on a half opened
   // IMAP folder is a protocol error, too)
   if ( m_MailStream->halfopen )
      return;

   DBGMESSAGE(("MailFolderCC::Checkpoint() on %s.", GetName().c_str()));
   if(NeedsNetwork() && ! mApplication->IsOnline())
   {
      ERRORMESSAGE((_("System is offline, cannot access mailbox ´%s´"), GetName().c_str()));
      return;
   }
   if(Lock())
   {
     mail_check(m_MailStream); // update flags, etc, .newsrc
     UnLock();
     ProcessEventQueue();
   }
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
      STATUSMESSAGE((_("Mailstream for folder '%s' has been closed, trying to reopen it."),
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
MailFolderCC::PingReopenAll(bool fullPing)
{
   // try to reopen all streams
   bool rc = true;

   /* We cannot simply traverse the list of mailfolders here, as they
      might get closed/reopened by Ping(), which invalidates the
      iterators into the list. Instead, we traverse a copy of the list */
   CHECK_STREAM_LIST();

   StreamConnectionList streamListCopy(FALSE);
   StreamConnectionList::iterator i;
   for ( i = ms_StreamList.begin(); i != ms_StreamList.end(); i++ )
   {
      streamListCopy.push_back(*i);
   }

   for ( i = streamListCopy.begin(); i != streamListCopy.end(); i++ )
   {
      StreamConnection *conn = *i;
      MailFolderCC *mf = conn->folder;

      rc &= fullPing ? mf->PingReopen() : mf->Ping();
   }
   return rc;
}

bool
MailFolderCC::Ping(void)
{
   if ( gs_lockCclient ) // FIXME: MT
      return FALSE;

   /* We do not want to do anything funny while autocollecting mail
      and such. Ping might be called from the Ping timer while we are
      autocollecting. This isn't a real problem, but causes
      unnecessary delays and confusion, so we just ignore the
      request. */
   if( m_ListingFrozen )
      return FALSE;

   
   if(NeedsNetwork() && ! mApplication->IsOnline())
   {
      ERRORMESSAGE((_("System is offline, cannot access mailbox ´%s´"), GetName().c_str()));
      return FALSE;
   }
   UIdType count = CountMessages();
   DBGMESSAGE(("MailFolderCC::Ping() on Folder %s.",
               GetName().c_str()));

   bool rc = FALSE;
      
   if(Lock())
   {
      int ccl = CC_SetLogLevel(M_LOG_WINONLY);
      // This is terribly inefficient to do, but needed for some sick
      // POP3 servers.
      if( (m_FolderFlags & MF_FLAGS_REOPENONPING)
          // c-client 4.7-bug: MH folders don't immediately notice new
          // messages:
          || GetType() == MF_MH)
      {
         DBGMESSAGE(("MailFolderCC::Ping() forcing close on folder %s.",
                     GetName().c_str()));
         Close();
      }

      if(PingReopen())
      {
         ASSERT(m_MailStream);
         rc = TRUE;
         if(! mail_ping(m_MailStream))
            rc = FALSE;
         //mail_check(m_MailStream); // update flags, etc, .newsrc
         UnLock();
         ProcessEventQueue();
         Lock();
      }

      CC_SetLogLevel(ccl);
      if(CountMessages() != count)
         RequestUpdate();
      UnLock();
      ProcessEventQueue();
      
      // Check if we want to collect all mail from this folder:
      if( (GetFlags() & MF_FLAGS_INCOMING) != 0)
      {
         // this triggers the mail collection code
         HeaderInfoList *hil = GetHeaders();
         hil->DecRef();
      }
   }
   else
   {
      ASSERT_MSG(0,"Ping() called on locked folder");
   }
   return rc;
}

void
MailFolderCC::Close()
{
   /*
     DO NOT SEND EVENTS FROM HERE, ITS CALLED FROM THE DESTRUCTOR AND
     THE OBJECT *WILL* DISAPPEAR!
   */

   // can cause references to this folder, cannot be allowd:
//   UpdateStatus(); // send last status event before closing
//   ProcessEventQueue();  //FIXMe: is this safe or not?
//   MEventManager::DispatchPending();

   CCQuiet(true); // disable all callbacks!
   // We cannot run ProcessEventQueue() here as we must not allow any
   // Message to be created from this stream. If we miss an event -
   // that's a pity.
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
   CCVerbose();
}

bool
MailFolderCC::Lock(void) const
{
#ifdef USE_THREADS
   ((MailFolderCC *)this)->m_Mutex->Lock();
   return true;
#else
   if(!m_Mutex)
   {
      ((MailFolderCC *)this)->m_Mutex = true;
      return true;
   }
   else
   {
#ifdef EXPERIMENTAL_log_callbacks
      printf("**********Attempt to lock locked folder\n");	// FIXME
      //mm_log("Attempt to lock locked folder.", 0, (MailFolderCC *)this);
#endif
      return false;
   }
#endif
}

bool
MailFolderCC::IsLocked(void) const
{
#ifdef USE_THREADS
   return m_Mutex->IsLocked();
#else
   return m_Mutex;
#endif
}

void
MailFolderCC::UnLock(void) const
{
#ifdef USE_THREADS
   ((MailFolderCC *)this)->m_Mutex->Unlock();
#else
   ASSERT_MSG(m_Mutex, "Attempt to unlock unlocked folder.");
   ((MailFolderCC *)this)->m_Mutex = false;
#endif
}




bool
MailFolderCC::AppendMessage(String const &msg, bool update)
{
   CHECK_DEAD_RC("Appending to closed folder '%s' failed.", false);

   STRING str;

   INIT(&str, mail_string, (void *) msg.c_str(), msg.Length());
   if(update) ProcessEventQueue();

   bool rc = mail_append(
      m_MailStream, (char *)m_ImapSpec.c_str(), &str) != 0;
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
     ProcessEventQueue();
     FilterNewMail(m_Listing);
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


/// return number of recent messages
unsigned long
MailFolderCC::CountRecentMessages(void) const
{
   return m_nRecent;
}

unsigned long
MailFolderCC::CountNewMessagesQuick(void) const
{
   if(m_Listing)
      return CountNewMessages();
   else
      return UID_ILLEGAL;
}

unsigned long
MailFolderCC::CountMessages(int mask, int value) const
{
   if(mask == MSG_STAT_NONE)
      return m_nMessages;
   else if(mask == MSG_STAT_RECENT && value == MSG_STAT_RECENT)
      return m_nRecent;
   else
   {
      HeaderInfoList *hil = GetHeaders();
      if ( !hil )
         return 0;
      unsigned long numOfMessages = m_nMessages;

      // FIXME there should probably be a much more efficient way (using
      //       cclient functions?) to do it
      for ( unsigned long msgno = 0; msgno < m_nMessages; msgno++ )
      {
         if ( ((*hil)[msgno]->GetStatus() & mask) != value)
            numOfMessages--;
      }
      hil->DecRef();
      return numOfMessages;
   }
}

Message *
MailFolderCC::GetMessage(unsigned long uid)
{
   CHECK_DEAD_RC("Cannot access closed folder\n'%s'.", NULL);

   // This is a test to see if the UID is valid:

   bool uidValid = false;
   HeaderInfoList *hil = GetHeaders();
   if ( !hil ) return NULL;
   for ( unsigned long msgno = 0; msgno < hil->Count() && ! uidValid; msgno++ )
   {
      if ( (*hil)[msgno]->GetUId() == uid)
         uidValid = true;
   }
   hil->DecRef();
   ASSERT_MSG(uidValid, "DEBUG: Attempting to get a non-existing message.\n"
              "(OK when filtering as message might have disappeared)");
   if(! uidValid) return NULL;

   MessageCC *m = MessageCC::CreateMessageCC(this,uid);
   ProcessEventQueue();
   return m;
}


/// Counts the number of new mails
UIdType
MailFolderCC::CountNewMessages(void) const
{
   const int mask = MSG_STAT_RECENT | MSG_STAT_SEEN;
   const int value = MSG_STAT_RECENT;

   UIdType num = 0;
   if( m_Listing )
   {
      for ( UIdType msgno = 0; msgno < m_Listing->Count(); msgno++ )
      {
         int status = (*m_Listing)[msgno]->GetStatus();
         if( (status & mask) == value )
            num ++;
      }
   }
   return num;
}

/* FIXME-MT: we must add some clever locking to this function! */

/*
  This function looks much more complicated than it is. All it does is
  to return a folder listing.
  Before doing so it checks if the listing must be rebuild, if so it
  does that.

  In addition, a listing can be marked as "frozen". This means it
  should not yet be rebuild, no matter whether it is out of date or
  not. This is to avoid recursion and unnecessary intermediate
  updates, e.g. when filtering messages.
 */
class HeaderInfoList *
MailFolderCC::GetHeaders(void) const
{
   /* A listing can be frozen if we want to suppress updates at
      present to avoid recursion. */
   if(m_ListingFrozen)
   {
      ASSERT_MSG(m_Listing,
                 "GetHeaders() returning frozen listing - "
                 "should be harmless"); 
      if(m_Listing) m_Listing->IncRef();
      ASSERT_MSG(m_Listing, "GetHeaders() returning NULL listing - dubious");
      return m_Listing;
   }

   // remove const from this:
   MailFolderCC *that = (MailFolderCC *)this;

   // Listing is not frozen, check if we still need to expunge some
   // messages:
   if( m_ExpungeRequested == TRUE )
   {
      that->ExpungeMessages();
      that->m_ExpungeRequested = FALSE;
   }
   
   // check if we are still alive:
   if(m_MailStream == NIL)
   {
      if(! PingReopen())
      {
         ERRORMESSAGE((_("Cannot get listing for closed mailbox '%s'."),
                       GetName().c_str()));
         return NULL;
      }
      that->m_NeedFreshListing = true; // re-build it!
   }

   // if nothing needs to be done, just return current listing
   if( m_NeedFreshListing == FALSE )
   {
      m_Listing->IncRef();
      return m_Listing;
   }

   
   /*
     OK, we need to rebuild the current folder listing.
     This involves retrieving it, filtering it and re-retrieving it if
     the filters caused any change. */

   /* What we are about to do might calls GetHeaders(), so we must
      freeze the listing. We leave it frozen until we are done. */
   that-> m_ListingFrozen = TRUE;

   // we need to re-generate the listing:
   that->BuildListing();

   /* The filter code returns a TRUE if there is a chance that the
      folder has been changed. */
   bool changed = that->FilterNewMail(m_Listing);

   if(changed)
      that->BuildListing();

   // sort/thread listing:
   that->ProcessHeaderListing(m_Listing);

   // enforce consistency:
   ASSERT_MSG(m_nMessages == m_Listing->Count(),
              "(DEBUG, harmless): Inconsistend message counts");
   that->m_nMessages = m_Listing->Count();

   // check if we need to update our data structures or send new
   // mail events:
   that->CheckForNewMail(m_Listing);
   that->UpdateStatus();

   /* Now we are done with rebuilding the listing and internal folder
      data and can safely tell the application to use this
      information. */

   that->m_ListingFrozen = FALSE;

   /* Some event processing might have been delayed because the
      listing was frozen. So now that it's thawed, we can flush the
      queue again to make sure everything is updated.
   */
   ProcessEventQueue();
   m_Listing->IncRef(); // for the caller who uses it
   return m_Listing;
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
      ProcessEventQueue();
   }
   return true; /* not supported by c-client */
}


bool
MailFolderCC::SetFlag(const UIdArray *selections, int flag, bool set)
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
   /* We must not expunge messages while the listing is
      frozen. Otherwise it can happen that a message disappears and
      thus c-client bombs when we try to retrieve that message. So we
      simply set a flag which causes ExpungeMessages() to be called
      when the listing is no longer frozen.
   */
   if(m_ListingFrozen)
   {
      //set a flag to tell the next update to run an expunge first
      m_ExpungeRequested = TRUE;
      return;
   }

   m_ExpungeRequested = FALSE;
   if(PY_CALLBACK(MCB_FOLDEREXPUNGE,1,GetProfile()))
      mail_expunge (m_MailStream);
   RequestUpdate();
   ProcessEventQueue();
}


UIdArray *
MailFolderCC::SearchMessages(const class SearchCriterium *crit)
{
   ASSERT(m_SearchMessagesFound == NULL);
   ASSERT(crit);

   HeaderInfoList *hil = GetHeaders();
   if ( !hil )
      return 0;

   unsigned long lastcount = 0;
   MProgressDialog *progDlg = NULL;
   m_SearchMessagesFound = new UIdArray;
   // Error is set to true if we cannot handle it on the server side
   // or later, if a real error occurs.
   bool error = false;

   /* Do server-side searches if we are using IMAP */
   if( GetType() == MF_IMAP )
   {
      SEARCHPGM *pgm = mail_newsearchpgm();
      STRINGLIST *slist = mail_newstringlist();

      slist->text.data = (unsigned char *)strutil_strdup(crit->m_Key);
      slist->text.size = crit->m_Key.length();
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
         error = true;
      }
      if(! error)
         mail_search_full (m_MailStream,
                           /* charset */ NIL,
                           pgm,
                           SE_UID|SE_FREE|SE_NOPREFETCH);
      else
         mail_free_searchpgm(&pgm);
   }
   /* our own search for all other folder types or unsuppported server
      searches */
   if( GetType() != MF_IMAP || error == TRUE)
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

      for ( unsigned long msgno = 0; msgno < m_nMessages; msgno++ )
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
            m_SearchMessagesFound->Add((*hil)[msgno]->GetUId());

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
               if(! progDlg->Update( msgno, msg ))
                  break;  // abort searching
               lastcount = cnt;
            }
            else if(! progDlg->Update( msgno ))
               break;  // abort searching
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


#ifdef DEBUG

StreamConnection::~StreamConnection()
{
    wxLogDebug("deleting %s from stream list", name.c_str());
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

/// remove this object from Map
void
MailFolderCC::RemoveFromMap(void) const
{
   DBGMESSAGE(("MailFolderCC::RemoveFromMap() for folder %s", GetName().c_str()));
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

   if(ms_StreamListDefaultObj == this)
      SetDefaultObj(false);
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

   ASSERT( i != ms_StreamList.end());
   DBGMESSAGE(( i.Debug() ));
   i++;
   DBGMESSAGE(( i.Debug() ));
   if( i != ms_StreamList.end())
      return (**i).folder;
   else
      return NULL;
}


extern "C"
{
   static int mm_overview_header (MAILSTREAM *stream,unsigned long uid, OVERVIEW_X *ov);
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
       && ( status & MSG_STAT_RECENT)
       && ! ( status & MSG_STAT_DELETED) )
      isNew = true;
   if(m_LastNewMsgUId != UID_ILLEGAL
      && m_LastNewMsgUId >= msgId)
      isNew = false;

   return isNew;
}

/* This is called by the UpdateListing method of the common code. */
void
MailFolderCC::BuildListing(void)
{
   CHECK_DEAD("Cannot access closed folder\n'%s'.");

   if(! Lock())
      return;

   String tmp;
   tmp.Printf("Building listing for folder '%s'...", GetName().c_str());
   LOGMESSAGE((M_LOG_DEBUG, tmp));

   // make sure our message counts are up to date
   UpdateStatus();

   // we don't want MEvents to be handled while we are in here, as
   // they might query this folder:
   MEventLocker locker;
   MEventManager::Suspend(true);

   m_BuildListingSemaphore = true;

   if ( m_FirstListing )
   {
      // Hopefully this will only count really existing messages for
      // NNTP connections.
//      if(GetType() == MF_NNTP)
//         m_nMessages = m_MailStream->recent;
//      else
         m_nMessages = m_MailStream->nmsgs;
   }

   if(m_Listing)
   {
      m_Listing->DecRef();
      m_Listing = NULL;
   }

   if(! m_Listing )
      m_Listing =  HeaderInfoListImpl::Create(m_nMessages);

   // by default, we retrieve all messages
   unsigned long numMessages = m_nMessages;
   // if the number of the messages in the folder is bigger than the
   // configured limit, ask the user whether he really wants to retrieve them
   // all. The value of 0 disables the limit. Ask only once and never for file
   // folders (loading headers from them is quick)
   if ( !IsLocalQuickFolder(GetType()) &&
        (m_RetrievalLimit > 0)
        && m_FirstListing
        && (m_nMessages > m_RetrievalLimit) )
   {
      // too many messages - ask the user how many of them he really wants
      String msg, prompt, title;
      title.Printf(_("How many messages to retrieve from folder '%s'?"),
            GetName().c_str());
      msg.Printf(_("This folder contains %lu messages, which is greater than\n"
               "the current threshold of %lu (set it to 0 to avoid this "
               "question - or\n"
               "you can also choose [Cancel] to download all messages)."),
            m_nMessages, m_RetrievalLimit);
      prompt = _("How many of them do you want to retrieve?");

      long nRetrieve;
      {
         MGuiLocker locker;
        nRetrieve = MGetNumberFromUser(msg, prompt, title,
                                            m_RetrievalLimit,
                                            1, m_nMessages);
      }
      if ( nRetrieve != -1 )
      {
         numMessages = nRetrieve;
         if(numMessages > m_nMessages)
            numMessages = m_nMessages;
      }
      //else: cancelled, retrieve all
   }

   m_BuildNextEntry = 0;
   if(numMessages > 0) // anything to do at all?
   {
      if ( m_ProgressDialog == NULL &&
           numMessages > (unsigned)READ_CONFIG(m_Profile,
                                               MP_FOLDERPROGRESS_THRESHOLD) )
      {
         String msg;
         msg.Printf(_("Reading %lu message headers..."), numMessages);
         MGuiLocker locker;
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

      UIdType uid;
      if(numMessages <= m_nMessages)
         uid = m_nMessages-numMessages+1;
      else
         uid = m_nMessages;


/*      if ( numMessages != m_nMessages )
      {
      // the first one to retrieve
      unsigned long firstMessage = 1;

      sequence = strutil_ultoa(mail_uid(m_MailStream, uid));
      sequence += ":*";
      }
      else
      {
         sequence.Printf("%lu", firstMessage);
         if( GetType() == MF_NNTP )
         {  // FIXME: no idea why this works for NNTP but not for the other types
            //  sequence << '-';
         }
         else
            sequence << ":*";
      }

*/
      UIdType from, to;
      from = mail_uid(m_MailStream, uid);
      to = mail_uid(m_MailStream,m_nMessages);
      sequence = strutil_ultoa(from);
      if(to != from) // don't produce sequences like "1:1"
      {
/*         if( GetType() == MF_NNTP)
            sequence << '-' << strutil_ultoa(to);
         else
*/
         sequence << ":*";
      }

      mail_fetch_overview_x(m_MailStream, (char *)sequence.c_str(), mm_overview_header);
      if ( m_ProgressDialog != (MProgressDialog *)1 )
      {
         MGuiLocker locker;
         delete m_ProgressDialog;
      }
   }

   // We set it to an illegal address here to suppress further updating. This
   // value is checked against in OverviewHeader(). The reason is that we only
   // want it the first time that the folder is being opened.
   m_ProgressDialog = (MProgressDialog *)1;

   // for NNTP, it will not show all messages
   //ASSERT(m_BuildNextEntry == m_nMessages || m_folderType == MF_NNTP);
   m_nMessages = m_BuildNextEntry;
   m_Listing->SetCount(m_nMessages);

   m_FirstListing = false;
   m_BuildListingSemaphore = false;
   m_NeedFreshListing = false;
   m_UpdateNeeded = true;
   // now we must release the MEvent queue again:
   MEventManager::Suspend(false);
   UnLock();
   tmp.Printf("Finished building listing for folder '%s'...",
              GetName().c_str());
   LOGMESSAGE((M_LOG_DEBUG, tmp));
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
   ASSERT(m_Listing);

   /* Ignore all entries that have arrived in the meantime to avoid
      overflowing the array.
   */
   if(m_BuildNextEntry >= m_nMessages)
      return 0;

   // This is 1 if we don't want any further updates.
   if(m_ProgressDialog && m_ProgressDialog != (MProgressDialog *)1)
   {
      MGuiLocker locker;
      if(! m_ProgressDialog->Update( m_BuildNextEntry ))
         return 0;
   }

   // This seems to occasionally happen. Some race condition?
   if(m_BuildNextEntry >= m_Listing->Count())
   {
      m_nMessages = m_Listing->Count();
      return 0;
   }

   HeaderInfoImpl & entry = *(HeaderInfoImpl *)(*m_Listing)[m_BuildNextEntry];

   unsigned long msgno = mail_msgno (m_MailStream,uid);
   MESSAGECACHE *elt = mail_elt (m_MailStream,msgno);
   MESSAGECACHE selt;

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
      if(m_folderType == MF_NNTP
            || m_folderType == MF_NEWS)
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

      m_BuildNextEntry++;

      // This is 1 if we don't want any further updates.
      if(m_ProgressDialog && m_ProgressDialog != (MProgressDialog *)1)
      {
         MGuiLocker locker;
         m_ProgressDialog->Update( m_BuildNextEntry - 1 );
      }
   }
   else
   {
      // parsing the message failed in c-client lib. Why?
      ASSERT_MSG(0,"DEBUG (harmless, don't worry): parsing folder overview failed");
      // don't do anything
   }
   return 1;
}



//-------------------------------------------------------------------
// static class member functions:
//-------------------------------------------------------------------

StreamConnectionList MailFolderCC::ms_StreamList(FALSE);

bool MailFolderCC::ms_CClientInitialisedFlag = false;

MailFolderCC *MailFolderCC::ms_StreamListDefaultObj = NULL;

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
      LOGMESSAGE((M_LOG_ERROR, _("Cannot set signal handler for SIGPIPE.")));
   }
#endif

   ms_CClientInitialisedFlag = true;
   ASSERT(gs_CCStreamCleaner == NULL);
   gs_CCStreamCleaner = new CCStreamCleaner();
}

String MailFolderCC::ms_NewsPath;

String MailFolderCC::ms_LastCriticalFolder = "";

/// return the directory of the newsspool:
/* static */
String
MailFolderCC::GetNewsSpool(void)
{
   CClientInit();
   return (const char *)mail_parameters
      (NIL,GET_NEWSSPOOL,NULL);
}

const String& MailFolderCC::InitializeNewsSpool()
{
   if ( !ms_NewsPath )
   {
      // first, init cclient
      if ( !ms_CClientInitialisedFlag )
      {
         CClientInit();
      }

      ms_NewsPath = (char *)mail_parameters(NULL, GET_NEWSSPOOL, NULL);
      if ( !ms_NewsPath )
      {
         ms_NewsPath = READ_APPCONFIG(MP_NEWS_SPOOL_DIR);
         mail_parameters(NULL, SET_NEWSSPOOL, (char *)ms_NewsPath.c_str());
      }
   }

   return ms_NewsPath;
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

/// adds this object to Map
void
MailFolderCC::AddToMap(MAILSTREAM const *stream) const
{
   DBGMESSAGE(("MailFolderCC::RemoveFromMap() for folder %s", GetName().c_str()));
   CHECK_STREAM_LIST();

   StreamConnection  *conn = new StreamConnection;
   conn->folder = (MailFolderCC *) this;
   conn->stream = stream;
   conn->name = m_ImapSpec;
   conn->login = m_Login;
   /** A push_front() would be slightly faster as the folder is likely
       to be accessed often after opening it, but PingReopenAll()
       relies on the fact that newly (re-)opened folders are appended
       to the list, not pre-pended.
   */
   ms_StreamList.push_back(conn);

   CHECK_STREAM_LIST();
}


void
MailFolderCC::UpdateStatus(void)
{
   if(m_MailStream == NIL && ! PingReopen())
      return; // fail

   if(m_nMessages != m_MailStream->nmsgs ||
      m_nRecent != m_MailStream->recent)
   {
      m_nMessages = m_MailStream->nmsgs;
      m_nRecent = m_MailStream->recent;
      // Little sanity check, needed as c-client is insane:
      if(m_nMessages < m_nRecent)
         m_nRecent = m_nMessages;
      MailFolderCC::Event *evptr = new
         MailFolderCC::Event(m_MailStream,
                             MailFolderCC::Status,__LINE__);
      MailFolderCC::QueueEvent(evptr);
   }
   m_LastUId = m_MailStream->uid_last;
   // and another sanity check:
   if(m_nMessages < m_nRecent)
      m_nRecent = m_nMessages;
}

/* static */
bool
MailFolderCC::CanExit(String *which)
{
   CHECK_STREAM_LIST();

   bool canExit = true;
   *which = "";
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

/// lookup object in Map
/* static */ MailFolderCC *
MailFolderCC::LookupObject(MAILSTREAM const *stream, const char *name)
{
   CHECK_STREAM_LIST();

   StreamConnectionList::iterator i;
   for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
   {
      StreamConnection *conn = *i;
      if( conn->stream == stream )
         return conn->folder;
   }

   /* Sometimes the IMAP code (imap4r1.c) allocates a temporary stream
      for e.g. mail_status(), that is difficult to handle here, we
      must compare the name parameter which might not be 100%
      identical, but we can check the hostname for identity and the
      path. However, that seems a bit far-fetched for now, so I just
      make sure that ms_StreamListDefaultObj gets set before any call to
      mail_status(). If that doesn't work, we need to parse the name
      string for hostname, portnumber and path and compare these.
      //FIXME: This might be an issue for MT code.
   */
   if(name)
   {
      for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
      {
         StreamConnection *conn = *i;
         if( conn->name == name )
            return conn->folder;
      }
   }
   if(ms_StreamListDefaultObj)
   {
      LOGMESSAGE((M_LOG_DEBUG, "Routing call to default mailfolder."));
      return ms_StreamListDefaultObj;
   }
   ASSERT_MSG(0,"DEBUG (harmless): Cannot find mailbox for callback!");
   return NULL;
}

void
MailFolderCC::SetDefaultObj(bool setit) const
{
   if(setit)
   {
      CHECK_RET(ms_StreamListDefaultObj == NULL,
                "conflicting mail folder default object access (trying to override non-NULL)!");
      ms_StreamListDefaultObj = (MailFolderCC *)this;
   }
   else
   {
      CHECK_RET(ms_StreamListDefaultObj == this,
                "conflicting mail folder default object access (trying to re-set to NULL)!");
      ms_StreamListDefaultObj = NULL;
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
      ASSERT(mf->m_SearchMessagesFound);
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
      String tmp = "MailFolderCC::mm_exists() for folder " + mf->m_ImapSpec
                   + String(" n: ") + strutil_ultoa(number);
      LOGMESSAGE((M_LOG_DEBUG, Str(tmp)));
#endif

      // this test seems necessary for MH folders, otherwise we're going into
      // an infinite loop (and it shouldn't (?) break anything for other
      // folders)
      if ( (mf->m_nMessages == 0 && number != 0) ||
           (mf->m_nMessages != number) )
      {
         mf->m_nMessages = number;
         // This can be called from within mail_open(), before we have a
         // valid m_MailStream, so check. It's slightly more efficient
         // to check here than in UpdateStatus().
         if(mf->m_MailStream != NULL)
            mf->RequestUpdate();
      }
      MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Exists,__LINE__);
      MailFolderCC::QueueEvent(evptr);
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
   // reentrant
   gs_lockCclient++;  // FIXME: MT
   wxYield();
   gs_lockCclient--;
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
              mf->m_ImapSpec.c_str(), status->messages);

   if(status->flags & SA_MESSAGES)
      mf->RequestUpdate();
}

/** log a message
    @param str message str
    @param errflg error level
*/
void
MailFolderCC::mm_log(String str, long errflg, MailFolderCC *mf )
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
         loglevel = CC_GetLogLevel();
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
MailFolderCC::mm_dlog(String str)
{
   String msg;
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
   GetLogCircle().Add(str);
   if(mm_show_debug)
   {
      msg = _("Mail-debug: ") + msg;
      STATUSMESSAGE((Str(msg)));
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

/** alert that c-client will run critical code
       @param  stream   mailstream
   */
void
MailFolderCC::mm_critical(MAILSTREAM * stream)
{
   ms_LastCriticalFolder = stream ? stream->mailbox : NULL;
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
   {
#ifdef DEBUG
      String tmp = "MailFolderCC::mm_critical() for folder " + mf->m_ImapSpec;
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
   ms_LastCriticalFolder = "";
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
   {
#ifdef DEBUG
      String tmp = "MailFolderCC::mm_nocritical() for folder " + mf->m_ImapSpec;
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
   GetLogCircle().Add(str);
   String   msg = (String) "Fatal Error:" + (String) str;
   LOGMESSAGE((M_LOG_ERROR, Str(msg)));

   String msg2 = str;
   if(ms_LastCriticalFolder.length())
      msg2 << _("\nLast folder in a critical section was: ")
           << ms_LastCriticalFolder;
   FatalError(msg2);
}

void
MailFolderCC::UpdateMessageStatus(unsigned long seqno)
{
   /* If a new listing is required, we don't need to do anything to
      update the old one. Only exception is if the listing is frozen,
      in that case we try to update it but don't sent a status change
      event. */
   if( (m_NeedFreshListing && ! m_ListingFrozen )
       || m_Listing == NULL )
      return; // will be regenerated anyway

   // Otherwise: update the current listing information:

   // Find the listing entry for this message:
   UIdType uid = mail_uid(m_MailStream, seqno);
   size_t i;
   for(i = 0; i < m_Listing->Count() && (*m_Listing)[i]->GetUId() != uid; i++)
      ;
   ASSERT_RET(i < m_Listing->Count());

   MESSAGECACHE *elt = mail_elt (m_MailStream,seqno);
   ((HeaderInfoImpl *)(*m_Listing)[i])->m_Status = GetMsgStatus(elt);
   
   if(m_NeedFreshListing || m_Listing == NULL)
      return; // will be regenerated anyway

   MailFolderCC::Event *evptr = new
      MailFolderCC::Event(m_MailStream,
                          MailFolderCC::MsgStatus, __LINE__);
   evptr->m_args[0].m_ulong = i;
   MailFolderCC::QueueEvent(evptr);
}

/* static */
void
MailFolderCC::mm_flags(MAILSTREAM * stream, unsigned long number)
{
   MailFolderCC *mf = LookupObject(stream);
   if(mf)
      mf->UpdateMessageStatus(number);
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
      case MsgStatus:
      {
         MailFolderCC *mf = MailFolderCC::LookupObject(evptr->m_stream);
         ASSERT_MSG(mf,"got msg status event for unknown folder");
         if(mf && ! mf->m_UpdateNeeded && ! mf->m_ListingFrozen)
         {
            // tell all interested that status changed
            DBGMESSAGE(("Sending MsgStatusData event for folder '%s'",
                        mf->GetName().c_str()));
            ASSERT_MSG(mf->m_Listing, "FATAL bug: m_Listing == NULL");
            MEventManager::Send( new MEventMsgStatusData (
               mf, evptr->m_args[0].m_ulong, mf->m_Listing) );
            break;
         }
      }
      case Exists:
      case Status:
      case Expunged: // invalidate our header listing:
      {
         MailFolderCC *mf = MailFolderCC::LookupObject(evptr->m_stream);
         ASSERT_MSG(mf,"DEBUG (harmless): got expunge event for unknown folder");
         if(mf)
         {
            //causes recursion mf->UpdateStatus();
            mf->RequestUpdate();
         }
         break;
      }
      case Searched: // obsolete
      case Flags: // obsolete
      case Update: // obsolete
         ASSERT_MSG(0,"obsolete code called");
         break;
      }// switch
      delete evptr;
   }

   /* Now we check all folders if we need to send update events: */
   CHECK_STREAM_LIST();

   StreamConnectionList::iterator i;
   MailFolderCC *mf;
   for(i = ms_StreamList.begin(); i != ms_StreamList.end(); i++)
   {
      mf = (*i)->folder;
      if(
         // do we need to send an update event for this folder?
         mf && mf->m_UpdateNeeded)
      {
         /* If our current listing is frozen we postpone the processing of
            events until it no longer is. Repeatedly sending events about a
            folder without the listing having been updated hardly makes any
            sense. */
          if(! mf->m_ListingFrozen)
          {
             DBGMESSAGE((
                "Sending update event for folder '%s'",
                mf->GetName().c_str() ));
             // tell all interested that an update might be required
             MEventManager::Send( new MEventFolderUpdateData (mf) );
             mf->m_UpdateNeeded = false;
          }
          else
          {
             DBGMESSAGE((
                "Postponing event processing for frozen folder '%s'",
                mf->GetName().c_str() ));
          }
      }
   }
}

void
MailFolderCC::RequestUpdate(bool sendEvent)
{
   // invalidate current folder listing and queue at least one folder
   // update event.
   m_NeedFreshListing = true;
   if(sendEvent)
      m_UpdateNeeded = true;
}

/* Handles the mm_overview_header callback on a per folder basis. */
int
MailFolderCC::OverviewHeader(MAILSTREAM *stream,
                             unsigned long uid,
                             OVERVIEW_X *ov)
{
   CHECK_STREAM_LIST();
   MailFolderCC *mf = MailFolderCC::LookupObject(stream);
   CHECK(mf, 0, "trying to build overview for non-existent");
   return mf->OverviewHeaderEntry(uid, ov);
}


/* static */
bool
MailFolderCC::Subscribe(const String &host,
                        FolderType protocol,
                        const String &mailboxname,
                        bool subscribe)
{
   String spec = ::GetImapSpec(protocol, 0, mailboxname, host, "");
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
   String mboxpath = ::GetImapSpec(mfolder->GetType(),
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
   ProcessEventQueue();
   return rc;
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
   if (stream->dtb 
//
// We are not using the driver's overview function as we need the
// OVERVIEW_X structure with the extra To field. This might be
// a bit inefficient, but the alternative would be to patch all
// c-client drivers or to check somehow which structure we get.
//       
// && !(stream->dtb->overview && (*stream->dtb->overview)(stream,sequence,(overview_t)ofn)) 
       && mail_uid_sequence (stream,sequence) && mail_ping (stream))
   {
    MESSAGECACHE *elt;
    ENVELOPE *env = NULL;  // initialisation not needed but keeps compiler happy
    OVERVIEW_X ov;
    unsigned long i;
    ov.optional.lines = 0;
    ov.optional.xref = NIL;
    for (i = stream->nmsgs; i>= 1; i--)
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
} // extern "C"

// the callbacks:
extern "C"
{

void
mm_searched(MAILSTREAM *stream, unsigned long number)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_searched(`%s', %lu) %d\n", stream->mailbox, number,
           mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   MailFolderCC::mm_searched(stream,  number);
}

void
mm_expunged(MAILSTREAM *stream, unsigned long number)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_expunged(`%s', %lu) %d\n", stream->mailbox, number,
           mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Expunged,__LINE__);
   evptr->m_args[0].m_ulong = number;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_flags(MAILSTREAM *stream, unsigned long number)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_flags(`%s', %lu) %d\n", stream->mailbox, number,
           mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   MailFolderCC::mm_flags(stream, number);
}

void
mm_notify(MAILSTREAM *stream, char *str, long errflg)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_notify(`%s', `%s', %lu) %d\n", stream->mailbox, str, errflg,
           mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Notify,__LINE__);
   evptr->m_args[0].m_str = new String(str);
   evptr->m_args[1].m_long = errflg;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_list(MAILSTREAM *stream, int delim, char *name, long attrib)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_list(`%s', %d, `%s', %ld) %d\n", stream->mailbox, delim, name,
           attrib, mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   MailFolderCC::mm_list(stream, delim, name, attrib);
}

void
mm_lsub(MAILSTREAM *stream, int delim, char *name, long attrib)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_lsub(`%s', %d, `%s', %ld) %d\n", stream->mailbox, delim, name,
           attrib, mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::LSub,__LINE__);
   evptr->m_args[0].m_int = delim;
   evptr->m_args[1].m_str = new String(name);
   evptr->m_args[2].m_long = attrib;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_exists(MAILSTREAM *stream, unsigned long number)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_EXISTS(%lu, `%s') %d\n", number, stream->mailbox,
           mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   // update count immediately to reflect change:
   MailFolderCC::mm_exists(stream, number);
}

void
mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS *status)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_status(`%s', `%s', %p) %d\n", stream->mailbox, mailbox, status,
           mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(stream,MailFolderCC::Status,__LINE__);
   MailFolderCC::QueueEvent(evptr);
}

void
mm_log(char *str, long errflg)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_log(`%s', %ld) %d\n", str, errflg, mm_disable_callbacks);
#endif
   if(mm_disable_callbacks || mm_ignore_errors)
      return;
   String *msg = new String(str);
   if(errflg >= 4) // fatal imap error, reopen-mailbox
   {
      if(!MailFolderCC::PingReopenAll())
         *msg << _("\nAttempt to re-open all folders failed.");
   }
   MailFolderCC::Event *evptr = new MailFolderCC::Event(NULL,MailFolderCC::Log,__LINE__);
   evptr->m_args[0].m_str = msg;
   evptr->m_args[1].m_long = errflg;
   MailFolderCC::QueueEvent(evptr);
}

void
mm_dlog(char *str)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_dlog(`%s') %d\n", str, mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   MailFolderCC::Event *evptr = new MailFolderCC::Event(NULL,MailFolderCC::DLog,__LINE__);
   evptr->m_args[0].m_str = new String(str);
   MailFolderCC::QueueEvent(evptr);
}

void
mm_login(NETMBX *mb, char *user, char *pwd, long trial)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_login({%s, %s@%s:%ld, %s}, %ld) %d\n", mb->service, mb->user,
           mb->host, mb->port , mb->mailbox, trial, mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return;
   // Cannot use event system, has to be called while c-client is working.
   MailFolderCC::mm_login(mb, user, pwd, trial);
}

void
mm_critical(MAILSTREAM *stream)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_critical(`%s') %d\n", stream->mailbox, mm_disable_callbacks);
#endif
   MailFolderCC::mm_critical(stream);
}

void
mm_nocritical(MAILSTREAM *stream)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_nocritical(`%s') %d\n", stream->mailbox, mm_disable_callbacks);
#endif
   MailFolderCC::mm_nocritical(stream);
}

long
mm_diskerror(MAILSTREAM *stream, long int errorcode, long int serious)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_diskerror(`%s', %ldl, %ld) %d\n", stream->mailbox, errorcode,
           serious, mm_disable_callbacks);
#endif
   if(mm_disable_callbacks)
      return 1;
   return MailFolderCC::mm_diskerror(stream,  errorcode, serious);
}

void
mm_fatal(char *str)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_fatal(`%s') %d\n", str, mm_disable_callbacks);
#endif
   MailFolderCC::mm_fatal(str);
}

/* This callback needs to be processed immediately or ov will become
   invalid. */
static int
mm_overview_header (MAILSTREAM *stream,unsigned long uid, OVERVIEW_X *ov)
{
#ifdef EXPERIMENTAL_log_callbacks
   printf("mm_overview_header(`%s', %lu, %p) %d\n", stream->mailbox, uid, ov,
           mm_disable_callbacks);
#endif
   return MailFolderCC::OverviewHeader(stream, uid, ov);
}

} // extern "C"


#ifdef USE_SSL

/* These functions will serve as stubs for the real openSSL library
   and must be initialised at runtime as c-client actually links
   against these. */

#include <openssl/ssl.h>
/* This is our interface to the library and auth_ssl.c in c-client
   which are all in "C" */
extern "C" {

extern long ssl_init_Mahogany();


#define SSL_DEF(returnval, name, args) \
   typedef returnval (* name##_TYPE) args ; \
   name##_TYPE stub_##name = NULL

#define SSL_LOOKUP(name) \
      stub_##name = (name##_TYPE) wxDllLoader::GetSymbol(slldll, #name); \
      if(stub_##name == NULL) success = FALSE
#define CRYPTO_LOOKUP(name) \
      stub_##name = (name##_TYPE) wxDllLoader::GetSymbol(cryptodll, #name); \
      if(stub_##name == NULL) success = FALSE

SSL_DEF( SSL *, SSL_new, (SSL_CTX *ctx) );
SSL_DEF( void, SSL_free, (SSL *ssl) );
SSL_DEF( int,  SSL_set_rfd, (SSL *s, int fd) );
SSL_DEF( int,  SSL_set_wfd, (SSL *s, int fd) );
SSL_DEF( void, SSL_set_read_ahead, (SSL *s, int yes) );
SSL_DEF( int,  SSL_connect, (SSL *ssl) );
SSL_DEF( int,  SSL_read, (SSL *ssl,char *buf,int num) );
SSL_DEF( int,  SSL_write, (SSL *ssl,const char *buf,int num) );
SSL_DEF( int,  SSL_pending, (SSL *s) );
SSL_DEF( int,  SSL_library_init, (void ) );
SSL_DEF( void, SSL_load_error_strings, (void ) );
SSL_DEF( SSL_CTX *,SSL_CTX_new, (SSL_METHOD *meth) );
SSL_DEF( unsigned long, ERR_get_error, (void) );
SSL_DEF( char *, ERR_error_string, (unsigned long e, char *p));
//extern SSL_get_cipher_bits();
SSL_DEF( const char *, SSL_CIPHER_get_name, (SSL_CIPHER *c) );
SSL_DEF( int, SSL_CIPHER_get_bits, (SSL_CIPHER *c, int *alg_bits) );
//extern SSL_get_cipher();
SSL_DEF( SSL_CIPHER *, SSL_get_current_cipher ,(SSL *s) );
#   if defined(SSLV3ONLYSERVER) && !defined(TLSV1ONLYSERVER)
SSL_DEF(SSL_METHOD *, SSLv3_client_method, (void) );
#   elif defined(TLSV1ONLYSERVER) && !defined(SSLV3ONLYSERVER)
SSL_DEF(int, TLSv1_client_method, () );
#   else
SSL_DEF(SSL_METHOD *, SSLv23_client_method, (void) );
#   endif

#undef SSL_DEF

SSL     * SSL_new(SSL_CTX *ctx)
{ return (*stub_SSL_new)(ctx); }
void  SSL_free(SSL *ssl)
{ (*stub_SSL_free)(ssl); }
int  SSL_set_rfd(SSL *s, int fd)
{ return (*stub_SSL_set_rfd)(s,fd); }
int  SSL_set_wfd(SSL *s, int fd)
{ return (*stub_SSL_set_wfd)(s,fd); }
void  SSL_set_read_ahead(SSL *s, int yes)
{ (*stub_SSL_set_read_ahead)(s,yes); }
int   SSL_connect(SSL *ssl)
{ return (*stub_SSL_connect)(ssl); }
int   SSL_read(SSL *ssl,char * buf, int num)
{ return (*stub_SSL_read)(ssl, buf, num); }
int   SSL_write(SSL *ssl,const char *buf,int num)
{ return (*stub_SSL_write)(ssl, buf, num); }
int  SSL_pending(SSL *s)
{ return (*stub_SSL_pending)(s); }
int       SSL_library_init(void )
{ return (*stub_SSL_library_init)(); }
void  SSL_load_error_strings(void )
{ (*stub_SSL_load_error_strings)(); }
SSL_CTX * SSL_CTX_new(SSL_METHOD *meth)
{ return (*stub_SSL_CTX_new)(meth); }
unsigned long ERR_get_error(void)
{ return (*stub_ERR_get_error)(); }
char * ERR_error_string(unsigned long e, char *p)
{ return (*stub_ERR_error_string)(e,p); }
const char * SSL_CIPHER_get_name(SSL_CIPHER *c)
{ return (*stub_SSL_CIPHER_get_name)(c); }
SSL_CIPHER * SSL_get_current_cipher(SSL *s)
{ return (*stub_SSL_get_current_cipher)(s); }
int SSL_CIPHER_get_bits(SSL_CIPHER *c, int *alg_bits)
{
  return (*stub_SSL_CIPHER_get_bits)(c,alg_bits);
}


#   if defined(SSLV3ONLYSERVER) && !defined(TLSV1ONLYSERVER)
SSL_METHOD *  SSLv3_client_method(void)
{ return (*stub_SSLv3_client_method)(); }
#   elif defined(TLSV1ONLYSERVER) && !defined(SSLV3ONLYSERVER)
int TLSv1_client_method(void)
{ (* stub_TLSv1_client_method)(); }
#   else
SSL_METHOD * SSLv23_client_method(void)
{ return (* stub_SSLv23_client_method)(); }
#   endif

} // extern "C"

#include <wx/dynlib.h>

bool gs_SSL_loaded = FALSE;
bool gs_SSL_available = FALSE;



bool InitSSL(void) /* FIXME: MT */
{
   if(gs_SSL_loaded)
      return gs_SSL_available;

   String ssl_dll = READ_APPCONFIG(MP_SSL_DLL_SSL);
   String crypto_dll = READ_APPCONFIG(MP_SSL_DLL_CRYPTO);

   STATUSMESSAGE((_("Trying to load '%s' and '%s'..."),
                  crypto_dll.c_str(),
                  ssl_dll.c_str()));

   bool success = FALSE;
   wxDllType cryptodll = wxDllLoader::LoadLibrary(crypto_dll, &success);
   if(! success) return FALSE;
   wxDllType slldll = wxDllLoader::LoadLibrary(ssl_dll, &success);
   if(! success) return FALSE;

   SSL_LOOKUP(SSL_new );
   SSL_LOOKUP(SSL_free );
   SSL_LOOKUP(SSL_set_rfd );
   SSL_LOOKUP(SSL_set_wfd );
   SSL_LOOKUP(SSL_set_read_ahead );
   SSL_LOOKUP(SSL_connect );
   SSL_LOOKUP(SSL_read );
   SSL_LOOKUP(SSL_write );
   SSL_LOOKUP(SSL_pending );
   SSL_LOOKUP(SSL_library_init );
   SSL_LOOKUP(SSL_load_error_strings );
   SSL_LOOKUP(SSL_CTX_new );
   SSL_LOOKUP(SSL_CIPHER_get_name );
   SSL_LOOKUP(SSL_get_current_cipher );
   CRYPTO_LOOKUP(ERR_get_error);
   CRYPTO_LOOKUP(ERR_error_string);
   SSL_LOOKUP(SSL_CIPHER_get_bits);
#   if defined(SSLV3ONLYSERVER) && !defined(TLSV1ONLYSERVER)
   SSL_LOOKUP(SSLv3_client_method );
#   elif defined(TLSV1ONLYSERVER) && !defined(SSLV3ONLYSERVER)
   SSL_LOOKUP(TLSv1_client_method );
#else
   SSL_LOOKUP(SSLv23_client_method );
#   endif
   gs_SSL_available = success;
   gs_SSL_loaded = TRUE;

   if(success) // only now is it safe to call this
   {
      STATUSMESSAGE((_("Successfully loaded '%s' and '%s' - "
                       "SSL authentification now available."),
                     crypto_dll.c_str(),
                     ssl_dll.c_str()));

      ssl_init_Mahogany();
   }
   return success;
}

#undef SSL_LOOKUP
#endif
