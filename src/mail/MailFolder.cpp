//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MailFolder.cpp: generic MailFolder methods which don't
//              use cclient (i.e. don't really work with mail folders)
// Purpose:     handling of mail folders with c-client lib
// Author:      Karsten Ballüder
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

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

#include <wx/dynarray.h>

#include "MDialogs.h" // for MDialog_FolderChoose

#include "Mdefaults.h"

#include "Message.h"
#include "MailFolder.h"
#include "MailFolderCC.h" // CC_Cleanup (FIXME: shouldn't be there!)

#ifdef EXPERIMENTAL
#include "MMailFolder.h"
#endif

#include "miscutil.h"   // GetFullEmailAddress

#include "gui/wxComposeView.h"
#include "MApplication.h"

/*-------------------------------------------------------------------*
 * local classes
 *-------------------------------------------------------------------*/

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
   if ( !Init() )
   {
      return NULL;
   }

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
   if ( !Init() )
      return NULL;

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
   if ( !Init() )
      return NULL;

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
   if ( !Init() )
      return NULL;

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
   if ( !Init() )
      return false;

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
   if ( !Init() )
      return false;

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

/* static */
bool
MailFolder::Init()
{
   static bool s_initialized = false;

   if ( !s_initialized )
   {
      if ( !MailFolderCmnInit() || !MailFolderCCInit() )
      {
         wxLogError(_("Failed to initialize mail subsystem."));
      }
      else // remember that we're initialized now
      {
         s_initialized = true;
      }
   }

   return s_initialized;
}

/* static */
void
MailFolder::CleanUp(void)
{
   MailFolderCmnCleanup();
   MailFolderCCCleanup();
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

