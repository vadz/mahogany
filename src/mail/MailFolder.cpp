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
#include "MFolder.h"

#include "Sequence.h"

#include "Message.h"
#include "MailFolder.h"

// we use many MailFolderCC methods explicitly currently, this is wrong: we
// should have some sort of a redirector to emulate virtual static methods
// (TODO)
#include "MailFolderCC.h"

#include "MFPrivate.h"

#ifdef EXPERIMENTAL
#include "MMailFolder.h"
#endif

#include "Composer.h"
#include "MApplication.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_DEFAULT_REPLY_KIND;
extern const MOption MP_FOLDER_COMMENT;
extern const MOption MP_FOLDER_LOGIN;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_FOLDER_PATH;
extern const MOption MP_FOLDER_TYPE;
extern const MOption MP_FORWARD_PREFIX;
extern const MOption MP_FROM_REPLACE_ADDRESSES;
extern const MOption MP_IMAPHOST;
extern const MOption MP_LIST_ADDRESSES;
extern const MOption MP_NNTPHOST;
extern const MOption MP_POPHOST;
extern const MOption MP_REPLY_COLLAPSE_PREFIX;
extern const MOption MP_REPLY_PREFIX;
extern const MOption MP_SET_REPLY_FROM_TO;
extern const MOption MP_USERNAME;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MLogCircle
// ----------------------------------------------------------------------------

#define MF_LOGCIRCLE_SIZE   10

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

/* static */
MailFolder *
MailFolder::OpenFolder(const MFolder *folder, OpenMode mode, wxFrame *frame)
{
   if ( !Init() )
      return NULL;

   CHECK( folder, NULL, "NULL MFolder in OpenFolder()" );

   return MailFolderCC::OpenFolder(folder, mode, frame);
}

/* static */
bool
MailFolder::CloseFolder(const MFolder *mfolder)
{
   // don't try to close the folder which can't be opened
   if ( !mfolder->CanOpen() )
   {
      return true;
   }

   // for now there is only one implementation to call:
   return MailFolderCC::CloseFolder(mfolder);
}

/* static */
int
MailFolder::CloseAll()
{
   MailFolder **mfOpened = GetAllOpened();
   if ( !mfOpened )
      return 0;

   size_t n = 0;
   while ( mfOpened[n] )
   {
      MailFolder *mf = mfOpened[n++];

      mf->Close();

      // notify any opened folder views
      MEventManager::Send(new MEventFolderClosedData(mf));
   }

   delete [] mfOpened;

   return n;
}

// ----------------------------------------------------------------------------
// MailFolder: other operations
// ----------------------------------------------------------------------------

/* static */
MailFolder **
MailFolder::GetAllOpened()
{
   // if we ever have other kinds of folders, we should append them to the
   // same array too
   return MailFolderCC::GetAllOpened();
}

MailFolder *
MailFolder::GetOpenedFolderFor(const MFolder *folder)
{
   return MailFolderCC::FindFolder(folder);
}

/* static */
bool MailFolder::PingAllOpened(wxFrame *frame)
{
   MailFolder **mfOpened = GetAllOpened();
   if ( !mfOpened )
      return 0;

   bool rc = true;
   for ( size_t n = 0; mfOpened[n]; n++ )
   {
      MailFolder *mf = mfOpened[n];

      NonInteractiveLock noInter(mf, frame != NULL);

      if ( !mf->Ping() )
      {
         // failed with at least one folder
         rc = false;
      }
   }

   delete [] mfOpened;

   return rc;
}

/* static */
bool
MailFolder::CheckFolder(const MFolder *folder, wxFrame *frame)
{
   if ( !Init() )
      return false;

   bool rc;

   MailFolder *mf = MailFolder::GetOpenedFolderFor(folder);
   if ( mf )
   {
      NonInteractiveLock noInter(mf, frame != NULL);

      // just pinging it is enough
      rc = mf->Ping();
      mf->DecRef();
   }
   else // not opened
   {
      // check its status without opening it
      return MailFolderCC::CheckStatus(folder);
   }

   return rc;
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
MailFolder::Rename(const MFolder *mfolder, const String& name)
{
   if ( !Init() )
      return false;

   return MailFolderCC::Rename(mfolder, name);
}

/* static */
long
MailFolder::ClearFolder(const MFolder *folder)
{
   if ( !Init() )
      return -1;

   // for now there is only one implementation to call:
   return MailFolderCC::ClearFolder(folder);
}

/* static */
bool
MailFolder::CreateFolder(const String& name,
                         MFolderType type,
                         int flags,
                         const String &path,
                         const String &comment)
{
   if ( !Init() )
      return false;

   /* So far the drivers do an auto-create when we open a folder, so
      now we attempt to open the folder to see what happens: */
   MFolder *mfolder = MFolder::Create(name, type);
   if ( !mfolder )
      return false;

   MailFolder_obj mf = MailFolder::OpenFolder(mfolder);

   mfolder->DecRef();

   return true;
}

// ----------------------------------------------------------------------------
// message size
// ----------------------------------------------------------------------------

/* static */
String MailFolder::SizeToString(unsigned long sizeBytes,
                                unsigned long sizeLines,
                                MessageSizeShow show,
                                bool verbose)
{
   String s;

   switch ( show )
   {
      case MessageSize_Max: // to silence gcc warning
         FAIL_MSG( "unexpected message size format" );
         // fall through

      case MessageSize_Automatic:
         if ( sizeLines > 0 )
         {
            s.Printf(_("%lu lines"),  sizeLines);
            break;
         }
         // fall through

      case MessageSize_AutoBytes:
         if ( sizeBytes == 0 )
         {
            s = _("empty");
         }
         else if ( sizeBytes < 10*1024 )
         {
            s = SizeToString(sizeBytes, 0, MessageSize_Bytes);
         }
         else if ( sizeBytes < 10*1024*1024 )
         {
            s = SizeToString(sizeBytes, 0, MessageSize_KBytes);
         }
         else // Mb
         {
            s = SizeToString(sizeBytes, 0, MessageSize_MBytes);
         }
         break;

      case MessageSize_Bytes:
         s.Printf("%lu%s", sizeBytes, verbose ? _(" bytes") : "");
         break;

      case MessageSize_KBytes:
         s.Printf(_("%lu%s"), sizeBytes / 1024,
                              verbose ? _(" kilobytes") : _("Kb"));
         break;

      case MessageSize_MBytes:
         s.Printf(_("%lu%s"), sizeBytes / (1024*1024),
                              verbose ? _(" megabytes") : _("Mb"));
         break;
   }

   return s;
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

         // if we already have status, nothing to do
         m_init = status->IsValid();
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
               mf->CountAllMessages(m_status);
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

            case 'u':               // unread
               result += strutil_ultoa(stat->unread);
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

/* static */
bool MailFolder::CanExit(String *which)
{
   return MailFolderCC::CanExit(which);
}

/* static */
bool
MailFolder::Subscribe(const String &host, MFolderType protocol,
                      const String &mailboxname, bool subscribe)
{
   return MailFolderCC::Subscribe(host, protocol, mailboxname, subscribe);
}

// ----------------------------------------------------------------------------
// reply/forward messages
// ----------------------------------------------------------------------------

// extract the address string from List-Post header, return an empty string if
// none found
static String
ExtractListPostAddress(const String& listPostHeader)
{
   // the format of List-Post is described in the RFC 2369 but basicly
   // it's an address-like field except that it may also have a special
   // value "NO" if posting to this list is prohibited

   // FIXME: it would be nice to use c-client functions for parsing instead

   for ( const wxChar *p = listPostHeader; ; p++ )
   {
      switch ( *p )
      {
         case '<':
            {
               static const size_t MAILTO_LEN = 7; // strlen("mailto:")

               // start of an URL, get it
               if ( strncmp(++p, "mailto:", MAILTO_LEN) != 0 )
               {
                  wxLogDebug("Unknown URL scheme in List-Post (%s)",
                             listPostHeader.c_str());
                  return "";
               }

               String listPostAddress;
               for ( p += MAILTO_LEN; *p && *p != '>'; p++ )
               {
                  if ( *p == '?' )
                  {
                     // start of URL parameters - ignore them for now (TODO)
                     break;
                  }

                  listPostAddress += *p;
               }

               if ( !*p )
               {
                  // so that p++ in the loop sets it at '\0' during next iteration
                  p--;
               }
               else // successfully extracted an URL
               {
                  return listPostAddress;
               }
            }
            break;

         case '(':
            // start of the comment, skip it
            for ( p++; *p && *p != ')'; p++ )
               ;

            if ( !*p )
            {
               // so that p++ in the loop sets it at '\0' during next iteration
               p--;
            }
            break;

         case 'N':
            // possible "NO"
            if ( p[1] == 'O' )
            {
               // posting is forbidden, hence no list posting address
               return "";
            }
            //else: fall through

         // catches "case '\0':", so the loop always exits
         default:
            // this is just for me, so that I could check for possible bugs in
            // this code
            wxLogDebug("Malformed List-Post header '%s'!",
                       listPostHeader.c_str());
            return "";
      }
   }
}

// add the recipients addresses extracted from the message being replied to to
// the composer according to the MailFolder::Params::replyKind value
static void
InitRecipients(Composer *cv,
               Message *msg,
               const MailFolder::Params& params,
               Profile *profile)
{
   // first get the real reply kind
   MailFolder::ReplyKind replyKind = params.replyKind;

   bool explicitReplyKind;
   if ( replyKind == MailFolder::REPLY )
   {
      // this means we should use the default reply kind for this folder
      long rk = READ_CONFIG(profile, MP_DEFAULT_REPLY_KIND);

      if ( rk < 0 || rk >= MailFolder::REPLY )
      {
         wxLogDebug("Corrupted config data: invalid MP_DEFAULT_REPLY_KIND!");

         rk = MailFolder::REPLY_SENDER;
      }

      // now cast is safe
      replyKind = (MailFolder::ReplyKind)rk;

      // implicit because we used the default value
      explicitReplyKind = false;
   }
   else
   {
      explicitReplyKind = true;
   }

   wxSortedArrayString replyToAddresses;
   size_t n,
          countReplyTo = msg->GetAddresses(MAT_REPLYTO, replyToAddresses);

   // REPLY_LIST overrides any Reply-To in the header
   if ( replyKind != MailFolder::REPLY_LIST )
   {
      // otherwise reply to Reply-To address, if any, by default
      if ( !countReplyTo )
      {
         // try from address
         //
         // FIXME: original encoding is lost here
         cv->AddTo(MailFolder::DecodeHeader(msg->From()));
      }
      else // have Reply-To
      {
         for ( n = 0; n < countReplyTo; n++ )
         {
            // FIXME: same as above
            cv->AddTo(MailFolder::DecodeHeader(replyToAddresses[n]));
         }
      }
   }

   // don't bother extracting the other addresses if we know in advance that
   // we don't need them anyhow
   //
   // this is sort of a hack: if the user really chose "Reply to sender" from
   // the menu, then we won't even add the other addresses to the composer at
   // all because it clearly means that he doesn't need them
   //
   // OTOH, if REPLY_SENDER just happens to be the default reply value (and so
   // could be invoked just by one key press and is the fastest way to reply
   // anyhow), then we'll still add the other recipients to the composer but
   // just disable them by default
   //
   // IMHO this does the right thing in all cases, but maybe we need an
   // additional option to handle this ("add unused addresses to composer")?
   if ( replyKind == MailFolder::REPLY_SENDER && explicitReplyKind )
      return;

   // now get all other addresses
   wxSortedArrayString otherAddresses;

   if ( replyKind == MailFolder::REPLY_LIST )
   {
      // take all address because we didn't use them yet
      WX_APPEND_ARRAY(otherAddresses, replyToAddresses);

      otherAddresses.Add(msg->From());

      // don't want to exclude the Reply-To addresses in the code below
      replyToAddresses.Clear();
   }
   else // we already have used some addresses
   {
      // use From only if not done already above
      if ( countReplyTo > 0 )
         otherAddresses.Add(msg->From());
   }

   msg->GetAddresses(MAT_CC, otherAddresses);
   msg->GetAddresses(MAT_TO, otherAddresses);

   // this is probably an overkill - you never want to reply to the sender
   // address, do you?
#if 0
   msg->GetAddresses(MAT_SENDER, otherAddresses);
#endif // 0

   // decode headers before comparing them - the problem is that we lost their
   // original encoding when doing this
   for ( n = 0; n < otherAddresses.GetCount(); n++ )
   {
      otherAddresses[n] = MailFolder::DecodeHeader(otherAddresses[n]);
   }

   // remove duplicates
   wxArrayString uniqueAddresses = strutil_uniq_array(otherAddresses);

   // and also filter out the addresses used in to as well as our own
   // address(es) by putting their indices into addressesToIgnore array
   String returnAddrs = READ_CONFIG(profile, MP_FROM_REPLACE_ADDRESSES);
   wxArrayString ownAddresses = strutil_restore_array(returnAddrs);

   // finally, if we're in REPLY_LIST mode, also remember which reply
   // addresses correspond to the mailing lists as we want to reply to them
   // only
   wxArrayString listAddresses;
   if ( replyKind == MailFolder::REPLY_LIST )
   {
      // the manually configured mailing lists
      listAddresses = strutil_restore_array(READ_CONFIG(profile,
                                                        MP_LIST_ADDRESSES));

      // also check if we can't find some in the message itself
      String listPostAddress;
      if ( msg->GetHeaderLine("List-Post", listPostAddress) )
      {
         listPostAddress = ExtractListPostAddress(listPostAddress);
         if ( !listPostAddress.empty() )
         {
            listAddresses.Add(listPostAddress);
         }
      }
   }

   // some addresses may appear also in Reply-To (assuming we used it) and
   // some also may correpond to our own addresses - we exclude them as we
   // don't usually want to reply to ourselves
   //
   // and in the same loop we also detect which addresses correspond to the
   // mailing lists
   wxArrayInt addressesToIgnore,
              addressesList;
   for ( n = 0; n < uniqueAddresses.GetCount(); n++ )
   {
      String addr = uniqueAddresses[n];
      if ( Message::FindAddress(replyToAddresses, addr) != wxNOT_FOUND ||
           Message::FindAddress(ownAddresses, addr) != wxNOT_FOUND )
      {
         addressesToIgnore.Add(n);
      }

      // if we're not in REPLY_LIST mode the listAddresses array is empty
      // anyhow so it doesn't hurt
      if ( Message::FindAddress(listAddresses, addr) != wxNOT_FOUND )
      {
         addressesList.Add(n);
      }
   }

   if ( replyKind == MailFolder::REPLY_LIST )
   {
      // again special case for list reply: we wish to have the list addresses
      // first and all the others later
      size_t countLists = addressesList.GetCount();
      if ( countLists )
      {
         for ( size_t n = 0; n < countLists; n++ )
         {
            cv->AddTo(uniqueAddresses[addressesList[n]]);
         }
      }
      else // no mailing list addresses found
      {
         // again, the same logic: if we explicitly asked to reply to the list
         // than we must think that the message was sent to the list and so we
         // should complain loudly if this is not the case - OTOH, if replying
         // to the list is just the default reply mode, it should work for all
         // messages without errors
         if ( explicitReplyKind )
         {
            wxLogWarning(_("None of the message recipients is a configured "
                           "mailing list, maybe you should configure the "
                           "list of the mailing list addresses?"));
         }

         // in any case fall back to group reply - this seems to be the safest
         // solution
         replyKind = MailFolder::REPLY_ALL;
      }
   }

   size_t count = uniqueAddresses.GetCount();
   for ( n = 0; n < count; n++ )
   {
      if ( addressesToIgnore.Index(n) != wxNOT_FOUND )
      {
         // ignore this one
         continue;
      }

      // what we do with this address depends on the kind of reply
      Composer::RecipientType rcptType;
      switch ( replyKind )
      {
         default:
            FAIL_MSG( "unexpected reply kind" );
            // fall through

         case MailFolder::REPLY_SENDER:
            // still add addresses  to allow easily adding them to the
            // recipient list - just disable them by default
            rcptType = Composer::Recipient_None;
            break;

         case MailFolder::REPLY_ALL:
            // reply to everyone and their dog
            rcptType = Composer::Recipient_To;
            break;

         case MailFolder::REPLY_LIST:
            // if it is a list address, we already dealt with it above
            if ( addressesList.Index(n) != wxNOT_FOUND )
               continue;

            // do keep the others but disabled by default
            rcptType = Composer::Recipient_None;
            break;
      }

      cv->AddRecipients(uniqueAddresses[n], rcptType);
   }
}

void
MailFolder::ReplyMessage(Message *msg,
                         const MailFolder::Params& params,
                         Profile *profile,
                         wxWindow *parent)
{
   CHECK_RET(msg, "no message to reply to");

   if(! profile)
      profile = mApplication->GetProfile();

   Composer *cv = Composer::CreateReplyMessage(params,
                                               profile,
                                               msg);

   InitRecipients(cv, msg, params, profile);

   // construct the new subject
   String newSubject;
   String replyPrefix = READ_CONFIG(profile, MP_REPLY_PREFIX);

   // FIXME: original subject encoding is lost here
   String subject = DecodeHeader(msg->Subject());

   // we may collapse "Re:"s in the subject if asked for it
   enum CRP // this is an abbreviation of "collapse reply prefix"
   {
      NoCollapse,
      DumbCollapse,
      SmartCollapse
   } collapse = (CRP)(long)READ_CONFIG(profile, MP_REPLY_COLLAPSE_PREFIX);

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
   static const char *headers[] =
   {
      "Message-Id",
      "References",
      NULL
   };

   wxArrayString headersOrig = msg->GetHeaderLines(headers);

   String messageid = headersOrig[0].Trim(TRUE).Trim(FALSE);

   // some message don't contain Message-Id at all
   if ( !messageid.empty() )
   {
      String references = headersOrig[1].Trim(TRUE).Trim(FALSE);
      if ( !references.empty() )
      {
         // continue "References" header on the next line
         references += "\015\012 ";
      }
      references += messageid;

      cv->AddHeaderEntry("References", references);
      cv->AddHeaderEntry("In-Reply-To", messageid);
   }

   // if configured, set Reply-To to the same address the message we're
   // replying to was sent
   if ( READ_CONFIG(profile, MP_SET_REPLY_FROM_TO) )
   {
      String rt;
      msg->GetHeaderLine("To", rt);
      cv->AddHeaderEntry("Reply-To", rt);
   }

   cv->InitText(msg, params.msgview);
}

/* static */
void
MailFolder::ForwardMessage(Message *msg,
                           const MailFolder::Params& params,
                           Profile *profile,
                           wxWindow *parent)
{
   CHECK_RET(msg, "no message to forward");

   if ( !profile )
      profile = mApplication->GetProfile();

   Composer *cv = Composer::CreateFwdMessage(params, profile, msg);

   // FIXME: we shouldn't assume that all headers have the same encoding
   //        so we should set the subject text and tell the composer its
   //        encoding as well
   cv->SetSubject(READ_CONFIG(profile, MP_FORWARD_PREFIX) +
                     DecodeHeader(msg->Subject()));
   cv->InitText(msg, params.msgview);
}

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

char MailFolder::GetFolderDelimiter() const
{
   switch ( GetType() )
   {
      default:
         FAIL_MSG( "Don't call GetFolderDelimiter() for this type" );
         // fall through nevertheless

      case MF_POP:
         // the folders of this type don't have subfolders at all
         return '\0';

      case MF_FILE:
      case MF_MH:
      case MF_MDIR:
      case MF_MFILE:
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
// misc
// ----------------------------------------------------------------------------

extern String GetSequenceString(const UIdArray *messages)
{
   Sequence seq;

   size_t count = messages ? messages->GetCount() : 0;
   for ( size_t n = 0; n < count; n++ )
   {
      seq.Add((*messages)[n]);
   }

   return seq.GetString();
}

