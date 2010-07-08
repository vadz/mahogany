//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/SendMessageCC.cpp: implementation of SendMessageCC
// Purpose:     sending/posting of mail messages with c-client lib
// Author:      Karsten Ballüder
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (C) 1999-2001 by M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef  USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "Mdefaults.h"
#  include "MApplication.h"
#  include "Profile.h"

#  include <wx/frame.h>                 // for wxFrame
#endif // USE_PCH

#include "Mversion.h"
#include "MailFolderCC.h"
#include "mail/MimeDecode.h"

#include "LogCircle.h"

#include "AddressCC.h"
#include "Message.h"
#include "MFolder.h"

#ifdef OS_UNIX
#  include "sysutil.h"
#endif // OS_UNIX

// has to be included before SendMessage.h, as it includes windows.h which
// defines SendMessage under Windows
#include <wx/fontmap.h>          // for GetEncodingName()
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__WINE__)
#  undef SendMessage
#endif

#include "SendMessage.h"
#include "SendMessageCC.h"
#include "Mcclient.h"

#include "XFace.h"
#include "gui/wxMDialogs.h"

#include "modules/MCrypt.h"

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/datetime.h>
#include <wx/scopeguard.h>

extern bool InitSSL(); // from src/util/ssl.cpp

class MOption;
class MPersMsgBox;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CHARSET;
extern const MOption MP_COMPOSE_USE_XFACE;
extern const MOption MP_COMPOSE_XFACE_FILE;
extern const MOption MP_CONFIRM_SEND;
extern const MOption MP_DEBUG_CCLIENT;
extern const MOption MP_GUESS_SENDER;
extern const MOption MP_HOSTNAME;
extern const MOption MP_NNTPHOST;
extern const MOption MP_NNTPHOST_LOGIN;
extern const MOption MP_NNTPHOST_PASSWORD;
extern const MOption MP_NNTPHOST_USE_SSL;
extern const MOption MP_NNTPHOST_USE_SSL_UNSIGNED;
extern const MOption MP_OUTBOX_NAME;
extern const MOption MP_OUTGOINGFOLDER;
extern const MOption MP_PREVIEW_SEND;
extern const MOption MP_REPLY_ADDRESS;
extern const MOption MP_SENDER;
#ifdef USE_OWN_CCLIENT
extern const MOption MP_SMTP_DISABLED_AUTHS;
#endif // USE_OWN_CCLIENT
extern const MOption MP_SMTP_USE_8BIT;
extern const MOption MP_SMTPHOST;
extern const MOption MP_SMTPHOST_LOGIN;
extern const MOption MP_SMTPHOST_PASSWORD;
extern const MOption MP_SMTPHOST_USE_SSL;
extern const MOption MP_SMTPHOST_USE_SSL_UNSIGNED;
extern const MOption MP_USEOUTGOINGFOLDER;
extern const MOption MP_USE_OUTBOX;

#ifdef OS_UNIX
extern const MOption MP_USE_SENDMAIL;
extern const MOption MP_SENDMAILCMD;
#endif // OS_UNIX

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_SEND_OFFLINE;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// trace mask for message sending/queuing operations
#define TRACE_SEND   "send"

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static long write_stream_output(void *, char *);
static long write_str_output(void *, char *);

namespace
{

// test if the header corresponds to one of the address headers
//
// NB: the header name must be in upper case
inline
bool IsAddressHeader(const String& name)
{
   return name == "FROM" ||
          name == "TO" ||
          name == "CC" ||
          name == "BCC";
}

// test if we allow this header to be set by user
//
// NB: the header name must be in upper case
inline
bool HeaderCanBeSetByUser(const String& name)
{
   return name != "MIME-VERSION" &&
          name != "CONTENT-TYPE" &&
          name != "CONTENT-DISPOSITION" &&
          name != "CONTENT-TRANSFER-ENCODING" &&
          name != "MESSAGE-ID";
}

// check if the header name is valid (as defined in 2.2 of RFC 2822)
bool IsValidHeaderName(const char *name)
{
   if ( !name )
      return false;

   for ( ; *name; name++ )
   {
      if ( *name >= 127 || iscntrl(*name) )
         return false;
   }

   return true;
}

// create a new BODY parameter and initialize it
PARAMETER *CreateBodyParameter(const char *name, const char *value)
{
   PARAMETER * const par = mail_newbody_parameter();
   par->attribute = strdup(name);
   par->value = strdup(value);
   par->next = NULL;

   return par;
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/**
   Redirect c-client RFC 822 output to our own function.

   An object of this class temporarily redirects c-client rfc822_output to our
   function and restores the old function in its dtor.

   This class uses a mutex internally to ensure that only single instance of it
   is used at any time because replacing c-client RFC 822 output pointer is not
   MT-safe. So using it from the main thread may
 */
class Rfc822OutputRedirector
{
public:
   /**
       Flags for ctor.
    */
   enum
   {
      /// Output BCC too. If off, BCC header is not written.
      AddBcc = 1
   };

   /**
       Starts redirecting c-client RFC 822 output.

       Until this object is destroyed, c-client rfc822_output() function will
       use this object to format the message so it will add the extra headers
       specified here and, if necessary, include BCC headers in output.

       Ctor will block if another Rfc822OutputRedirector is already in use.

       @param extraHeadersNames NUL-terminated array of names of extra headers
         to add to the normal output.
       @param extraHeadersValues NUL-terminated array of values of extra
         headers, must have the same number of elements as @a extraHeadersNames.
       @param flags May include AddBcc flag to include BCC header in output, by
         default it is not included to avoid leaking information about BCC
         recipients.
    */
   Rfc822OutputRedirector(const char **extraHeadersNames,
                          const char **extraHeadersValues,
                          int flags = 0);

   /**
       Dtor restores the original c-client output function.
    */
   ~Rfc822OutputRedirector();

private:
   static long FullRfc822Output(char *, ENVELOPE *, BODY *,
                                soutr_t, void *, long);

   // the old output routine, we remember it in ctor and restore in dtor
   void *m_oldRfc822Output;

   // The following variables are static as they're used by FullRfc822Output():

   // should we output BCC?
   static bool ms_outputBcc;

   // the extra headers written by FullRfc822Output()
   static const char **ms_HeaderNames;
   static const char **ms_HeaderValues;

   // and a mutex to protect them
   static MTMutex ms_mutexExtraHeaders;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// SendMessage
// ----------------------------------------------------------------------------

/* static */
SendMessage *
SendMessage::Create(const Profile *profile, Protocol protocol, wxFrame *frame)
{
   return new SendMessageCC(profile, protocol, frame);
}

/* static */
SendMessage *
SendMessage::CreateResent(const Profile *profile,
                          const Message *message,
                          wxFrame *frame)
{
   CHECK( message, NULL, "no Message in SendMessage::CreateResent()" );

   SendMessageCC *msg = new SendMessageCC(profile, Prot_Default, frame, message);
   if ( msg )
      msg->InitResent(message);
   return msg;
}

/* static */
SendMessage *
SendMessage::CreateFromMsg(const Profile *profile,
                           const Message *message,
                           Protocol protocol,
                           wxFrame *frame,
                           const wxArrayInt *partsToOmit)
{
   CHECK( message, NULL, "no Message in SendMessage::CreateFromMsg()" );

   SendMessageCC *msg = new SendMessageCC(profile, protocol, frame, message);
   if ( msg )
      msg->InitFromMsg(message, partsToOmit);
   return msg;
}

/* static */
bool SendMessage::Bounce(const String& address,
                         const Profile *profile,
                         const Message& message,
                         wxFrame *frame)
{
   SendMessage_obj sendMsg(SendMessage::CreateResent(profile, &message, frame));
   if ( !sendMsg )
   {
      ERRORMESSAGE((_("Failed to create the message to bounce.")));
      return false;
   }

   sendMsg->SetAddresses(address);

   // there is no need to store bounced messages in "sent mail" folder
   sendMsg->SetFcc("");

   if ( !sendMsg->SendOrQueue() )
   {
      ERRORMESSAGE((_("Failed to bounce the message to \"%s\"."),
                    address.c_str()));

      return false;
   }

   return true;
}

bool SendMessage::SendOrQueue(int flags)
{
   const Result res = PrepareForSending(flags);
   switch ( res )
   {
      case Result_Cancelled:
         mApplication->SetLastError(M_ERROR_CANCEL);
         return false;

      case Result_Prepared:
         {
            String
               errGeneral,
               errDetailed;

            if ( !SendNow(&errGeneral, &errDetailed) )
            {
               // log the detailed error first so that the general error
               // message appears first to the user
               if ( !errDetailed.empty() )
               {
                  wxLogError("%s", errDetailed);
               }
               wxLogError("%s", errGeneral);
               return false;
            }
            else
            {
               AfterSending();
            }
         }
         break;

      case Result_Queued:
         break;

      case Result_Error:
         return false;
   }

   return true;
}

SendMessage::~SendMessage()
{
}

// ----------------------------------------------------------------------------
// SendMessageCC creation and destruction
// ----------------------------------------------------------------------------

SendMessageCC::SendMessageCC(const Profile *profile,
                             Protocol protocol,
                             wxFrame *frame,
                             const Message *message)
             : m_profile(profile)
{
   m_frame = frame;
   m_encHeaders = wxFONTENCODING_SYSTEM;

   m_cloneOfExisting = false;
   m_sign = false;

   m_headerNames =
   m_headerValues = NULL;

   m_wasBuilt = false;

   m_Envelope = mail_newenvelope();
   m_partTop = NULL;

   if ( !profile )
   {
      FAIL_MSG( "SendMessageCC::Create() requires profile" );

      profile = mApplication->GetProfile();
   }

   const_cast<Profile *>(m_profile)->IncRef();

   // choose the protocol: mail (and whether it is SMTP or sendmail) or news
   if ( protocol == Prot_Default )
   {
      // if we have the original message, try to get protocol from it first
      if ( message )
      {
         // autodetect protocol:
         String tmp;
         if ( message->GetHeaderLine("Newsgroups", tmp) )
            protocol = Prot_NNTP;
      }

      if ( protocol == Prot_Default )
      {
#ifdef OS_UNIX
         if ( READ_CONFIG_BOOL(profile, MP_USE_SENDMAIL) )
            protocol = Prot_Sendmail;
         else
#endif // OS_UNIX
            protocol = Prot_SMTP;
      }
   }

   m_Protocol = protocol;

#ifndef OS_UNIX
   // For non-unix systems we make sure that no-one tries to run
   // Sendmail which is unix specific. This could happen if someone
   // imports a configuration from a remote server or something like
   // this, so we play it safe and map all sendmail calls to SMTP
   // instead:
   if(m_Protocol == Prot_Sendmail)
      m_Protocol = Prot_SMTP;
#endif // !OS_UNIX

   switch ( m_Protocol )
   {
      default:
         FAIL_MSG( "unknown SendMessage protocol" );
         // fall through

      case Prot_SMTP:
         m_ServerHost = READ_CONFIG_TEXT(profile, MP_SMTPHOST);
         m_UserName = READ_CONFIG_TEXT(profile, MP_SMTPHOST_LOGIN);
         m_Password = READ_CONFIG_TEXT(profile, MP_SMTPHOST_PASSWORD);
#ifdef USE_SSL
         m_UseSSLforSMTP = (SSLSupport)(long)
            READ_CONFIG(profile, MP_SMTPHOST_USE_SSL);
         m_UseSSLUnsignedforSMTP = (SSLCert)
            READ_CONFIG_BOOL(profile, MP_SMTPHOST_USE_SSL_UNSIGNED);
#endif // USE_SSL
         break;

#ifdef OS_UNIX
      case Prot_Sendmail:
         m_SendmailCmd = READ_CONFIG_TEXT(profile, MP_SENDMAILCMD);
         break;
#endif // OS_UNIX

      case Prot_NNTP:
         m_ServerHost = READ_CONFIG_TEXT(profile, MP_NNTPHOST);
         m_UserName = READ_CONFIG_TEXT(profile,MP_NNTPHOST_LOGIN);
         m_Password = READ_CONFIG_TEXT(profile,MP_NNTPHOST_PASSWORD);
#ifdef USE_SSL
         m_UseSSLforNNTP = (SSLSupport)(long)
            READ_CONFIG(profile, MP_NNTPHOST_USE_SSL);
         m_UseSSLUnsignedforNNTP = (SSLCert)
            READ_CONFIG_BOOL(profile, MP_NNTPHOST_USE_SSL_UNSIGNED);
#endif // USE_SSL
   }

   // other initializations common to all messages
   // --------------------------------------------

   // set up default value for From (Reply-To is set in InitNew() as it isn't
   // needed for the resent messages)
   m_From = Address::GetSenderAddress(m_profile);

   // remember the default hostname to use for addresses without host part
   m_DefaultHost = READ_CONFIG_TEXT(profile, MP_HOSTNAME);
   if ( !m_DefaultHost )
   {
      // we need a host name!
      m_DefaultHost = READ_APPCONFIG_TEXT(MP_HOSTNAME);
      if ( !m_DefaultHost )
      {
         // we really need something...
         m_DefaultHost = wxGetFullHostName();
      }
   }

   if ( READ_CONFIG_BOOL(profile, MP_USE_OUTBOX) )
      m_OutboxName = READ_CONFIG_TEXT(profile,MP_OUTBOX_NAME);
   if ( READ_CONFIG(profile,MP_USEOUTGOINGFOLDER) )
      m_FccList.push_back(new String(READ_CONFIG_TEXT(profile,MP_OUTGOINGFOLDER)));

   // initialize the message body, unless it's going to be done by our caller
   // for resent/cloned messages
   if ( !message )
   {
      InitNew();
   }
}

void SendMessageCC::InitNew()
{
   m_ReplyTo = READ_CONFIG_TEXT(m_profile, MP_REPLY_ADDRESS);

   /*
      Sender logic: by default, use the SMTP login if it is set and differs
      from the "From" value, otherwise leave it empty. If guessing it is
      disabled, then we use the sender value specified by the user "as is"
      instead.
   */
   if ( READ_CONFIG(m_profile, MP_GUESS_SENDER) )
   {
      m_Sender = READ_CONFIG_TEXT(m_profile, MP_SMTPHOST_LOGIN);
      m_Sender.Trim().Trim(false); // remove all spaces on begin/end

      if ( Address::Compare(m_From, m_Sender) )
      {
         // leave Sender empty if it is the same as From, redundant
         m_Sender.clear();
      }
   }
   else // don't guess, use provided value
   {
      m_Sender = READ_CONFIG_TEXT(m_profile, MP_SENDER);
   }

   if ( READ_CONFIG_BOOL(m_profile, MP_COMPOSE_USE_XFACE) )
      m_XFaceFile = m_profile->readEntry(MP_COMPOSE_XFACE_FILE, "");

   m_CharSet = READ_CONFIG_TEXT(m_profile,MP_CHARSET);
}

void SendMessageCC::InitResent(const Message *message)
{
   CHECK_RET( message, "message being resent can't be NULL" );

   CHECK_RET( m_Envelope, "envelope must be created in InitResent" );

   // get the original message header and copy it to remail envelope field
   // almost without any changes except that we have to mask the transport
   // layer headers as otherwise the SMTP software might get confused: e.g. if
   // we don't quote Delivered-To line some systems think that the message is
   // looping endlessly and we quote "Received:" and "Resent-xxx:" because Pine
   // does it (although I don't know why)
   String hdrOrig = message->GetHeader();

   String hdr;
   hdr.reserve(hdrOrig.length() + 100);   // slightly more for "X-"s

   bool firstHeader = true;
   for ( const wxChar *p = hdrOrig; *p; p++ )
   {
      // start of line?
      if ( firstHeader || (p[0] == '\r' && p[1] == '\n') )
      {
         if ( firstHeader )
         {
            // no longer
            firstHeader = false;
         }
         else // end of previous line
         {
            // copy CR LF as is
            hdr += *p++;
            hdr += *p++;
         }

#define STARTS_WITH(p, what) (!(wxStrnicmp((p), (what), wxStrlen(what))))

         if ( STARTS_WITH(p, "Delivered-To:") ||
               STARTS_WITH(p, "Received:") ||
                 STARTS_WITH(p, "Resent-") )
         {
            hdr += "X-";
         }

#undef STARTS_WITH
      }

      hdr += *p;
   }

   m_Envelope->remail = cpystr(hdr.ToAscii());

   // now copy the body: note that we have to use ENC7BIT here to prevent
   // c-client from (re)encoding the body
   m_partTop = mail_newbody_part();
   BODY& body = m_partTop->body;
   body.type = TYPETEXT;
   body.encoding = ENC7BIT;
   body.subtype = cpystr("PLAIN");

   // FIXME-OPT: we potentially copy a lot of data here!
   const wxWX2MBbuf text8bit(message->FetchText().To8BitData());
   body.contents.text.data = (unsigned char *)cpystr(text8bit);
   body.contents.text.size = strlen(text8bit);
}

void
SendMessageCC::InitFromMsg(const Message *message, const wxArrayInt *partsToOmit)
{
   m_cloneOfExisting = true;

   // set the headers not supported by AddHeaderEntry()

   // VZ: I'm not sure at all about what exactly we're trying to do here so
   //     this is almost surely wrong (FIXME)
   AddressList_obj addrListReplyTo(message->GetAddressList(MAT_REPLYTO));
   Address *addrReplyTo = addrListReplyTo ? addrListReplyTo->GetFirst() : NULL;

   AddressList_obj addrListFrom(message->GetAddressList(MAT_FROM));
   Address *addrFrom = addrListFrom ? addrListFrom->GetFirst() : NULL;
   if ( !addrFrom )
      addrFrom = addrReplyTo;

   String from,
          replyto;
   if ( addrFrom )
      from = addrFrom->GetAddress();
   if ( addrReplyTo )
      replyto = addrReplyTo->GetAddress();

   if ( addrFrom || addrReplyTo )
      SetFrom(from, replyto);

   switch ( m_Protocol )
   {
      case Prot_SMTP:
         {
            static const char *headers[] =
            {
               "To",
               "Cc",
               "Bcc",
               NULL
            };
            wxArrayString recipients = message->GetHeaderLines(headers);

            SetAddresses(recipients[0], recipients[1], recipients[2]);
         }
         break;

      case Prot_NNTP:
         {
            String newsgroups;
            if ( message->GetHeaderLine("Newsgroups", newsgroups) )
               SetNewsgroups(newsgroups);
         }
         break;

      // make gcc happy
      case Prot_Illegal:
      default:
         FAIL_MSG("unknown protocol");
   }

   // next deal with the remaining headers
   String name,
          value,
          nameUpper;
   HeaderIterator hdrIter = message->GetHeaderIterator();
   while ( hdrIter.GetNext(&name, &value) )
   {
      if ( !IsAddressHeader(name.Upper()) )
      {
         // we can't use AddHeaderEntry() here because it replaces the existing
         // header value if it appears multiple times while we want to preserve
         // the headers exactly as they were
         if ( wxStricmp(name, "subject") == 0 )
         {
            SetSubject(value);
         }
         else
         {
            m_extraHeaders.push_back(new MessageHeader(name, value));
         }
      }
   }

   // finally fill the body with the message contents
   const int count = message->CountParts();
   for(int i = 0; i < count; i++)
   {
      if ( partsToOmit && partsToOmit->Index(i) != wxNOT_FOUND )
         continue;

      unsigned long len;
      const void *data = message->GetPartContent(i, &len);
      String dispType;
      MessageParameterList const &dlist = message->GetDisposition(i, &dispType);
      MessageParameterList const &plist = message->GetParameters(i);

      AddPart
      (
         message->GetPartType(i),
         data, len,
         strutil_after(message->GetPartMimeType(i),'/'), //subtype
         dispType,
         &dlist,
         &plist
      );
   }
}

SendMessageCC::~SendMessageCC()
{
   mail_free_envelope(&m_Envelope);
   mail_free_body_part(&m_partTop);

   if(m_headerNames)
   {
      for(int j = 0; m_headerNames[j] ; j++)
      {
         delete [] (char *)m_headerNames[j];
         delete [] (char *)m_headerValues[j];
      }
      delete [] m_headerNames;
      delete [] m_headerValues;
   }

   const_cast<Profile *>(m_profile)->DecRef();
}

// ----------------------------------------------------------------------------
// SendMessageCC login/password handling
// ----------------------------------------------------------------------------

bool
SendMessageCC::GetPassword(String& password) const
{
   ASSERT_MSG( !m_UserName.empty(),
                  "password shouldn't be needed if no login" );

   if ( m_Password.empty() )
   {
      // ask the user for the password (and allow him to change login, too)
      String *username = &const_cast<SendMessageCC *>(this)->m_UserName;
      return MDialog_GetPassword(m_Protocol, m_ServerHost,
                                 &password, username, m_frame);
   }
   //else: we do have it stored

   // do not remember the decrypted password, I prefer to not store it
   // permanently in memory -- this is probably too paranoid or maybe not
   // enough but in any case it doesn't hurt
   password = strutil_decrypt(m_Password);

   return true;
}

// ----------------------------------------------------------------------------
// SendMessageCC encodings
// ----------------------------------------------------------------------------

// Check if the given data can be sent without encoding it (using QP or
// Base64): for this it must not contain 8bit chars nor embedded NUL chars and
// must not have too long lines.
//
// If it can be sent as 7 bit also enforce the CR LF as line end terminators as
// c-client doesn't apply any transformation to 7 bit data and sending CR (or
// LF) delimited data violates RFC 2822.
static bool CanSendAs7BitText(unsigned char *& text, size_t& len)
{
   if ( !text )
      return true;

   // check if we have 7 bit text and also find out whether we need to correct
   // EOLs
   bool isCRLF = true;
   size_t lenLine = 0,
          numLines = 0; // actually number of EOLs
   for ( size_t n = 0; n < len; n++ )
   {
      const unsigned char ch = text[n];

      switch ( ch )
      {
         case '\0':
            // text can't have embedded NULs
            return false;

         case '\r':
            if ( n < len - 1 && text[n + 1] == '\n' )
            {
               lenLine = 0;
               numLines++;
               continue;
            }
            // fall through

         case '\n':
            // we get here for '\n's not preceded by '\r's and '\r's not
            // followed by '\n's
            lenLine = 0;
            numLines++;
            isCRLF = false;
            continue;

         default:
            if ( !isascii(ch) )
               return false;
      }

      // the real limit is bigger (~990) but chances are that anything with
      // lines of such length is not plain text
      if ( ++lenLine > 800 )
         return false;
   }

   // it is a 7 bit text but we may need to correct its line endings
   if ( !isCRLF )
   {
      const unsigned char * const textOld = text;

      // we're going to add at most numLines characters for the EOLs and
      // another one for the trailing NUL
      len += numLines + sizeof(char);
      text = (unsigned char *) fs_get(len);

      if ( !strutil_enforceCRLF(text, len, textOld) )
      {
         FAIL_MSG( "buffer should have been big enough" );
      }

      fs_give((void **)&textOld);
   }

   return true;
}

// ----------------------------------------------------------------------------
// SendMessageCC header stuff
// ----------------------------------------------------------------------------

void
SendMessageCC::SetHeaderEncoding(wxFontEncoding enc)
{
   m_encHeaders = enc;
}

void
SendMessageCC::SetSubject(const String& subject)
{
   if(m_Envelope->subject)
      fs_give((void **)&m_Envelope->subject);

   // don't encode the headers of an existing message second time, we want to
   // preserve them as they are
   wxCharBuffer buf;
   if ( m_cloneOfExisting )
      buf = subject.ToAscii();
   else
      buf = MIME::EncodeHeader(subject, m_encHeaders);
   m_Envelope->subject = cpystr(buf);
}

void
SendMessageCC::SetFrom(const String& from,
                       const String& replyaddress,
                       const String& sender)
{
   if ( !from.empty() )
      m_From = from;

   if ( !replyaddress.empty() )
      m_ReplyTo = replyaddress;

   if ( !sender.empty() )
      m_Sender = sender;
}

void
SendMessageCC::SetupFromAddresses(void)
{
   // From
   SetAddressField(&m_Envelope->from, m_From);

   ADDRESS *adr = m_Envelope->from;

   // Sender
   if ( !m_Sender.empty() )
   {
      SetAddressField(&m_Envelope->sender, m_Sender);

      adr = m_Envelope->sender;
   }

   if ( adr )
   {
      // Return-Path (it is used as SMTP "MAIL FROM: <>" argument)
      ASSERT_MSG( m_Envelope->return_path == NIL, "Return-Path already set?" );

      m_Envelope->return_path = mail_newaddr();
      m_Envelope->return_path->mailbox = cpystr(adr->mailbox);
      m_Envelope->return_path->host = cpystr(adr->host);
   }
}

void
SendMessageCC::SetAddressField(ADDRESS **pAdr, const String& address)
{
   ASSERT_MSG( !*pAdr, "shouldn't be called twice" );

   if ( address.empty() )
   {
      // nothing to do
      return;
   }

   // parse into ADDRESS struct
   *pAdr = ParseAddressList(address, m_DefaultHost, m_encHeaders);

   // finally filter out any invalid addressees
   CheckAddressFieldForErrors(*pAdr);
}

void SendMessageCC::CheckAddressFieldForErrors(ADDRESS *adrStart)
{
   ADDRESS *adrPrev = adrStart,
           *adr = adrStart;
   while ( adr )
   {
      ADDRESS *adrNext = adr->next;

      if ( adr->error )
      {
         adrPrev->next = adr->next;

         DBGMESSAGE(("Invalid recipient address '%s' ignored.",
                    AddressCC(adr).GetAddress().c_str()));

         // prevent mail_free_address() from freeing the entire list tail
         adr->next = NULL;

         mail_free_address(&adr);
      }

      adr = adrNext;
   }
}

void
SendMessageCC::SetAddresses(const String &to,
                            const String &cc,
                            const String &bcc)
{
   // If Build() has already been called, then it's too late to change
   // anything.
   ASSERT(m_headerNames == NULL);

   SetAddressField(&m_Envelope->to, to);
   SetAddressField(&m_Envelope->cc, cc);
   SetAddressField(&m_Envelope->bcc, bcc);

   m_Bcc = bcc; // in case we send later, we need this
}

void
SendMessageCC::SetNewsgroups(const String &groups)
{
   // If Build() has already been called, then it's too late to change
   // anything.
   ASSERT(m_headerNames == NULL);

   // TODO-NEWS: we should support sending and posting the message, doing
   //            it separately if necessary
   ASSERT_MSG( m_Protocol == Prot_NNTP, "can't post and send message" );

   if ( !groups.empty() )
   {
      ASSERT(m_Envelope->newsgroups == NIL);
      m_Envelope->newsgroups = cpystr(groups.ToAscii());
   }

}

bool
SendMessageCC::SetFcc(const String& fcc)
{
   m_FccList.clear();

   wxArrayString fccFolders = strutil_restore_array(fcc, ',');
   size_t count = fccFolders.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      String folderName = fccFolders[n];
      strutil_delwhitespace(folderName);

      if ( folderName.empty() )
      {
         // be lenient and simply ignore
         continue;
      }

      // ignore the leading slash, if any: the user may specify it, but it
      // shouldn't be passed to MFolder
      if ( folderName[0u] == '/' )
         folderName.erase(0, 1);

      MFolder_obj folder(folderName);
      if ( !folder )
      {
         // an interesting idea: what if we interpreted the strings which are
         // not folder names as the file names? this would allow saving
         // outgoing messages to files very easily...
         ERRORMESSAGE((_("The folder '%s' specified in the FCC list "
                         "doesn't exist."), folderName.c_str()));
         return false;
      }

      m_FccList.push_back(new String(folderName));
   }

   return true;
}

// ----------------------------------------------------------------------------
// methods to manage the extra headers
// ----------------------------------------------------------------------------

MessageHeadersList::iterator
SendMessageCC::FindHeaderEntry(const String& name) const
{
   MessageHeadersList::iterator i;

   for ( i = m_extraHeaders.begin(); i != m_extraHeaders.end(); ++i )
   {
      if ( wxStricmp(i->m_name, name) == 0 )
         break;
   }

   return i;
}

bool
SendMessageCC::HasHeaderEntry(const String& name) const
{
   return FindHeaderEntry(name) != m_extraHeaders.end();
}

String
SendMessageCC::GetHeaderEntry(const String& name) const
{
   String value;

   MessageHeadersList::iterator i = FindHeaderEntry(name);
   if ( i != m_extraHeaders.end() )
   {
      value = i->m_value;
   }

   // empty if not found
   return value;
}

void
SendMessageCC::AddHeaderEntry(const String& nameIn, const String& value)
{
   String name = nameIn.Upper();

   strutil_delwhitespace(name);

   if ( IsAddressHeader(name) )
   {
      FAIL_MSG( "Address headers not supported here, use SetFrom() &c!" );
   }
   else if ( !HeaderCanBeSetByUser(name) )
   {
      ERRORMESSAGE((_("The value of the header '%s' cannot be modified."),
                    nameIn.c_str()));
   }
   else if ( name == "SUBJECT" )
   {
      SetSubject(value);
   }
   else // all the other headers
   {
      MessageHeadersList::iterator i = FindHeaderEntry(name);
      if ( i != m_extraHeaders.end() )
      {
         // update existing value
         i->m_value = value;
      }
      else // add a new header entry
      {
         m_extraHeaders.push_back(new MessageHeader(nameIn, value));
      }
   }
}

void
SendMessageCC::RemoveHeaderEntry(const String& name)
{
   MessageHeadersList::iterator i = FindHeaderEntry(name);
   CHECK_RET( i != m_extraHeaders.end(), "RemoveHeaderEntry(): no such header" );

   (void)m_extraHeaders.erase(i);
}

// ----------------------------------------------------------------------------
// SendMessageCC building
// ----------------------------------------------------------------------------

/// build a unique string for the Message-Id header
static
String BuildMessageId(const char *hostname)
{
   // get the PID from OS only once as it doesn't change while we run
   static unsigned long s_pid = 0;

   if ( !s_pid )
   {
      s_pid = wxGetProcessId();
   }

   // get the time to make the message-id unique and use s_numInSec to make the
   // messages sent during the same second unique
   static unsigned int s_numInSec = 0;
   static wxDateTime s_dtLast;

   wxDateTime dt = wxDateTime::Now();
   if ( s_dtLast.IsValid() && s_dtLast == dt )
   {
      s_numInSec++;
   }
   else
   {
      s_dtLast = dt;
      s_numInSec = 0;
   }

   return String::Format("<Mahogany-%s-%lu-%s.%02u@%s>",
                         M_VERSION,
                         s_pid,
                         dt.Format("%Y%m%d-%H%M%S").c_str(),
                         s_numInSec,
                         hostname);
}

bool
SendMessageCC::Sign()
{
   // get the text to sign
   BODY * const bodyOrig = GetBody();
   rfc822_encode_body_7bit(NULL /* env is unused */, bodyOrig);

   String textToSign;
   char tmp[MAILTMPLEN + 1];
   RFC822BUFFER
      buf = { write_str_output, &textToSign, tmp, tmp, tmp + MAILTMPLEN  };

   if ( !rfc822_output_body_header(&buf, bodyOrig) ||
        !rfc822_output_flush(&buf) ||
        !(textToSign += "\r\n", rfc822_output_text(&buf, bodyOrig)) ||
        !rfc822_output_flush(&buf) )
   {
      ERRORMESSAGE((_("Failed to create the text to sign.")));
      return false;
   }

   // according to 5.1.1 of RFC 2046 the EOL before the boundary line in a
   // multipart MIME message belongs to the boundary, and not the preceding
   // line (so that it's possible to have content not terminating with EOL)
   //
   // as the old body will become the first part of multipart/signed message
   // this EOL hence shouldn't be taken into account when signing it
   if ( textToSign.EndsWith("\r\n") )
      textToSign.erase(textToSign.length() - 2);


   // sign it
   MCryptoEngineFactory * const
      factory = (MCryptoEngineFactory *)MModule::LoadModule("PGPEngine");
   CHECK( factory, false, "failed to create PGPEngineFactory" );

   wxON_BLOCK_EXIT_OBJ0(*factory, MCryptoEngineFactory::DecRef);

   MCryptoEngine * const pgpEngine = factory->Get();
   MCryptoEngineOutputLog log(m_frame);

   String signature;
   MCryptoEngine::Status status = pgpEngine->Sign
                                             (
                                                m_signAsUser,
                                                textToSign,
                                                signature,
                                                &log
                                             );
   if ( status != MCryptoEngine::OK )
   {
      const size_t n = log.GetMessageCount();
      String err = n == 0 ? String(_("no error information available"))
                          : log.GetMessage(n - 1);

      ERRORMESSAGE((_("Signing the message failed: %s"), err.c_str()));

      return false;
   }

   // create the new multipart/signed message
   PART * const partOrig = m_partTop;

   m_partTop = mail_newbody_part();
   BODY& body = m_partTop->body;
   body.type = TYPEMULTIPART;
   body.subtype = cpystr("SIGNED");
   body.nested.part = partOrig;

   // fill in required parameters, see RFC 3156 section 5
   body.parameter = CreateBodyParameter("protocol",
                                        "application/pgp-signature");
   body.parameter->next = CreateBodyParameter("micalg",
                                              "pgp-" + log.GetMicAlg());

   const wxWX2MBbuf asciiSig(signature.ToAscii());
   AddPart(MimeType::APPLICATION,
           asciiSig, strlen(asciiSig),
           "PGP-SIGNATURE",
           "" /* no disposition */);

   return true;
}

bool
SendMessageCC::Build(bool forStorage)
{
   if ( m_wasBuilt )
   {
      // message was already build
      return true;
   }

   m_wasBuilt = true;

   // the headers needed for all messages
   // -----------------------------------

   // see section 3.6.6 of RFC 2822 for the full list of headers which must be
   // present in resent messages

   // From:
   SetupFromAddresses();

   // To:, Cc: and Bcc: have been already set by SetAddresses

   // Date:
   //
   if ( m_Envelope->remail && !HasHeaderEntry("Date") )
   {
      char tmpbuf[MAILTMPLEN];
      rfc822_date (tmpbuf);
      m_Envelope->date = (unsigned char *)cpystr(tmpbuf);
   }

   // Message-Id: we should always generate it ourselves (section 3.6.4 of RFC
   // 2822) but at least some MTAs (exim) reject it if the id-right part of it
   // is not a FQDN so don't do it in this case
   if ( m_DefaultHost.find('.') != String::npos )
   {
      m_Envelope->message_id = cpystr(
            BuildMessageId(m_DefaultHost.ToAscii()).ToAscii());
   }

   // don't add any more headers to the message being resent
   if ( m_Envelope->remail )
   {
      return true;
   }

   // the headers needed only for new (and not resent) messages
   // ---------------------------------------------------------

   /*
      Is the message supposed to be sent later? In that case, we need
      to store the BCC header as an X-BCC or it will disappear when
      the message is saved to the outbox.
    */
   if ( forStorage )
   {
      // The X-BCC will be converted back to BCC by Send()
      if ( m_Envelope->bcc )
         AddHeaderEntry("X-BCC", m_Bcc);
   }
   else // send, not store
   {
      /*
         If sending directly, we need to do the opposite: this message
         might have come from the Outbox queue, so we translate X-BCC
         back to a proper bcc setting:
       */
      if ( HasHeaderEntry("X-BCC") )
      {
         if ( m_Envelope->bcc )
         {
            mail_free_address(&m_Envelope->bcc);
         }

         SetAddressField(&m_Envelope->bcc, GetHeaderEntry("X-BCC"));

         // don't send X-BCC field or the recipient would still see the BCC
         // contents (which is highly undesirable!)
         RemoveHeaderEntry("X-BCC");
      }

      char tmpbuf[MAILTMPLEN];
      rfc822_date (tmpbuf);
      m_Envelope->date = (unsigned char *)cpystr(tmpbuf);
   }

   // +4: 1 for X-Mailer, 1 for X-Face, 1 for reply to and 1 for the
   // last NULL entry
   size_t n = m_extraHeaders.size() + 4;
   m_headerNames = new const char*[n];
   m_headerValues = new const char*[n];

   // the current header position in m_headerNames/Values
   int h = 0;

   bool replyToSet = false,
        xmailerSet = false;

   // add the additional header lines added by the user
   for ( MessageHeadersList::iterator i = m_extraHeaders.begin();
         i != m_extraHeaders.end();
         ++i )
   {
      const wxWX2MBbuf name(i->m_name.To8BitData());
      if ( !IsValidHeaderName(name) )
      {
         wxLogError(_("Custom header name \"%s\" is invalid, please use only "
                      "printable ASCII characters in the header names."),
                    i->m_name.c_str());
         return false;
      }

      const wxCharBuffer value(MIME::EncodeHeader(i->m_value));
      if ( !value )
      {
         wxLogError(_("Invalid value \"%s\" for the custom header \"%s\""),
                    i->m_value.c_str(), i->m_name.c_str());

         return false;
      }

      if ( wxStricmp(name, "Reply-To") == 0 )
         replyToSet = true;
      else if ( wxStricmp(name, "X-Mailer") == 0 )
         xmailerSet = true;


      m_headerNames[h] = strutil_strdup(name);
      m_headerValues[h] = strutil_strdup(value);

      h++;
   }

   // don't add any extra headers when cloning an existing message
   if ( !m_cloneOfExisting )
   {
      // add X-Mailer header if it wasn't overridden by the user (yes, we do
      // allow it -- why not?)
      if ( !xmailerSet )
      {
         m_headerNames[h] = strutil_strdup("X-Mailer");

         // NB: do *not* translate these strings, this doesn't make much sense
         //     (the user doesn't usually see them) and, worse, we shouldn't
         //     include 8bit chars (which may - and do - occur in translations)
         //     in headers!
         String version;
         version << "Mahogany " << M_VERSION_STRING;
#ifdef OS_UNIX
         version  << ", compiled for " << M_OSINFO;
#else // Windows
         version << ", running under " << wxGetOsDescription();
#endif // Unix/Windows
         m_headerValues[h++] = strutil_strdup(version.ToAscii());
      }

      // set Reply-To if it hadn't been set by the user as a custom header
      if ( !replyToSet )
      {
         ASSERT_MSG( !HasHeaderEntry("Reply-To"), "logic error" );

         if ( !m_ReplyTo.empty() )
         {
            m_headerNames[h] = strutil_strdup("Reply-To");
            m_headerValues[h++] = strutil_strdup(m_ReplyTo.ToAscii());
         }
      }

#ifdef HAVE_XFACES
      // add an XFace?
      if ( !HasHeaderEntry("X-Face") && !m_XFaceFile.empty() )
      {
         XFace xface;
         if ( xface.CreateFromFile(m_XFaceFile) )
         {
            m_headerNames[h] = strutil_strdup("X-Face");
            m_headerValues[h++] = strutil_strdup(xface.GetHeaderLine().ToAscii());
         }
         //else: couldn't read X-Face from file (complain?)
      }
#endif // HAVE_XFACES
   }

   m_headerNames[h] = NULL;
   m_headerValues[h] = NULL;


   // after fully constructing everything check if we need to add a
   // cryptographic signature
   //
   // notice that we shouldn't sign (or modify in any other way) messages being
   // resent/bounced nor those we don't send right now
   if ( !forStorage && m_sign && !m_Envelope->remail && !Sign() )
      return false;

   return true;
}

void
SendMessageCC::AddPart(MimeType::Primary type,
                       const void *buf, size_t len,
                       String const &subtype_given,
                       String const &disposition,
                       MessageParameterList const *dlist,
                       MessageParameterList const *plist,
                       wxFontEncoding enc)
{
   // adjust the input parameters

   // FIXME-OPT: we're copying a lot of data here, if we could ensure that
   //            buf is already allocated with malloc() and is NUL-terminated
   //            (this is important of encoding it wouldn't work correctly) we
   //            would be able to avoid it
   unsigned char *data = (unsigned char *) fs_get(len + sizeof(char));
   data[len] = '\0';
   memcpy(data, buf, len);

   String subtype(subtype_given);
   if( subtype.empty() )
   {
      if ( type == TYPETEXT )
         subtype = "PLAIN";
      else if ( type == TYPEAPPLICATION )
         subtype = "OCTET-STREAM";
      else
      {
         // shouldn't send message without MIME subtype, but we don't have any
         // and can't find the default!
         ERRORMESSAGE((_("MIME type specified without subtype and\n"
                         "no default subtype for this type.")));
         subtype = "UNKNOWN";
      }
   }

   // create a new MIME part

   // if it's the first one, it [provisionally] becomes the top level one
   BODY *bdy;
   if ( !m_partTop )
   {
      m_partTop = mail_newbody_part();

      bdy = &m_partTop->body;
   }
   else // we already have some part(s)
   {
      PART *part = m_partTop->body.nested.part;
      if ( !part )
      {
         // we need to create a new multipart/mixed top part and make the old
         // part and this one its subparts
         PART * const partOrig = m_partTop;

         m_partTop = mail_newbody_part();
         m_partTop->body.type = TYPEMULTIPART;
         m_partTop->body.subtype = cpystr("MIXED");

         part =
         m_partTop->body.nested.part = partOrig;
      }
      else // we already have a top-level multipart
      {
         // add this part after the existing subparts
         while ( part->next )
            part = part->next;
      }

      PART * const partNew = mail_newbody_part();
      part->next = partNew;

      bdy = &partNew->body;
   }

   bdy->type = type;
   bdy->subtype = cpystr(subtype.c_str());

   // set the transfer encoding
   switch ( type )
   {
      case TYPEMESSAGE:
         // FIXME:
         //    1. leads to empty body?
         //    2. why do we use ENC7BIT for TYPEMESSAGE?
         bdy->nested.msg = mail_newmsg();
         bdy->encoding = ENC7BIT;
         break;

      case TYPETEXT:
         // if the actual message text is in 7 bit, avoid encoding it even if
         // some charset which we would have normally encoded was used
         if ( CanSendAs7BitText(data, len) )
         {
            bdy->encoding = ENC7BIT;
         }
         else // we have 8 bit chars, need to encode them
         {
            // some encodings should be encoded in QP as they typically contain
            // only a small number of non printable characters while others
            // should be encoded in Base64 as almost all characters used in them
            // are outside basic ASCII set
            switch ( MIME::GetEncodingForFontEncoding(enc) )
            {
               case MIME::Encoding_Unknown:
               case MIME::Encoding_QuotedPrintable:
                  // automatically translated to QP by c-client
                  bdy->encoding = ENC8BIT;
                  break;

               default:
                  FAIL_MSG( "unknown MIME encoding" );
                  // fall through

               case MIME::Encoding_Base64:
                  if ( m_Protocol == Prot_SMTP &&
                        READ_CONFIG_BOOL(m_profile, MP_SMTP_USE_8BIT) )
                  {
                     // c-client will encode it as QP if the SMTP server
                     // doesn't support 8BITMIME extension and will send as is
                     // otherwise
                     bdy->encoding = ENC8BIT;
                  }
                  else // we send 7 bits always
                  {
                     // ENCBINARY is automatically translated to Base64 by
                     // c-client so use it instead of ENC8BIT (which would be
                     // encoded using QP) as it is more efficient
                     bdy->encoding = ENCBINARY;
                  }
            }
         }
         break;

      default:
         bdy->encoding = CanSendAs7BitText(data, len) ? ENC7BIT : ENCBINARY;
   }

   bdy->contents.text.data = data;
   bdy->contents.text.size = len;


   PARAMETER *lastpar = NULL,
             *par;

   // do we already have CHARSET parameter?
   bool hasCharset = false;

   if( plist )
   {
      MessageParameterList::iterator i;
      for( i = plist->begin(); i != plist->end(); i++ )
      {
         par = mail_newbody_parameter();

         String name = i->name;
         if ( name.Lower() == "charset" )
         {
            if ( hasCharset )
            {
               // although not fatal, this shouldn't happen
               wxLogDebug("Multiple CHARSET parameters!");
            }

            hasCharset = true;
         }

         par->attribute = strdup(name.ToAscii());
         par->value     = strdup(MIME::EncodeHeader(i->value));
         par->next      = lastpar;
         lastpar = par;
      }
   }

   // add the charset parameter to the param list for the text parts
   if ( !hasCharset && (type == TYPETEXT) )
   {
      String cs;
      if ( bdy->encoding == ENC7BIT )
      {
         // plain text messages should be in US_ASCII as all clients should be
         // able to show them and some might complain [even] about iso8859-1
         cs = "US-ASCII";
      }
      else // 8bit message
      {
         cs = MIME::GetCharsetForFontEncoding(enc);
         if ( cs.empty() )
         {
            cs = m_CharSet;
         }
         else
         {
            // if an encoding is specified, it overrides the default value
            m_CharSet = cs;
         }
      }

      if ( !cs.empty() )
      {
         par = mail_newbody_parameter();
         par->attribute = strdup("CHARSET");
         par->value     = strdup(cs.ToAscii());
         par->next      = lastpar;
         lastpar = par;
      }
   }

   bdy->parameter = lastpar;
   if ( !disposition.empty() )
      bdy->disposition.type = strdup(disposition.ToAscii());
   if ( dlist )
   {
      PARAMETER *lastpar = NULL,
                *par;

      MessageParameterList::iterator i;
      for ( i = dlist->begin(); i != dlist->end(); i++ )
      {
         par = mail_newbody_parameter();
         par->attribute = strdup(i->name.ToAscii());
         par->value     = strdup(MIME::EncodeHeader(i->value));
         par->next      = NULL;
         if(lastpar)
            lastpar->next = par;
         else
            bdy->disposition.parameter = par;
      }
   }
}

void SendMessageCC::EnableSigning(const String& user)
{
   m_sign = true;
   m_signAsUser = user; // can be empty, ok
}

// ----------------------------------------------------------------------------
// SendMessageCC sending
// ----------------------------------------------------------------------------

SendMessage::Result
SendMessageCC::PrepareForSending(int flags, String *outbox)
{
   // send directly either if we're told to do it (e.g. when sending the
   // messages already from Outbox) or if there is no Outbox configured at all
   bool send = (flags & NeverQueue) || m_OutboxName.empty();

#ifdef USE_DIALUP
   if ( send && !mApplication->IsOnline() )
   {
      /*
        This cannot work at present as we have no Outbox setting that we
        could use. We need to make a difference between Outbox and the
        "send" configuration settings.

         MDialog_Message(
            _("No network connection available at present.\n"
              "Message will be queued in outbox."),
            NULL, MDIALOG_MSGTITLE,"MailNoNetQueuedMessage");
      */

      if ( !MDialog_YesNoDialog
            (
               _("No network connection available at present,"
                 "message sending will probably fail.\n"
                 "Do you still want to send it?"),
               m_frame,
               MDIALOG_MSGTITLE,
               M_DLG_NO_DEFAULT,
               M_MSGBOX_SEND_OFFLINE
            ) )
      {
         return Result_Cancelled;
      }
   }
#endif // USE_DIALUP

   // prepare the message for sending or queuing
   if ( !Build(!send) )
      return Result_Error;

   if ( send )
   {
      if ( !PreviewBeforeSending() )
         return Result_Cancelled;

      if ( !m_UserName.empty() )
      {
         // we need to ask the user for password now if we need to do it as we
         // can't use UI later (possibly from another thread)
         String password;
         if ( !GetPassword(password) )
            return Result_Cancelled;
      }

      return Result_Prepared;
   }
   else // don't send the message now, queue it for later
   {
      // we just need to queue it, so do it immediately as saving the message to
      // outbox shouldn't take a long time
      if ( !WriteToFolder(m_OutboxName) )
         return Result_Error;

      // increment counter in statusbar immediately
      mApplication->UpdateOutboxStatus();

      if ( outbox )
         *outbox = m_OutboxName;

      return Result_Queued;
   }
}

bool
SendMessageCC::PreviewBeforeSending()
{
   // preview message being sent if asked for it
   bool confirmSend;
   if ( READ_CONFIG(m_profile, MP_PREVIEW_SEND) )
   {
      Preview();

      // if we preview it, we want to confirm it too
      confirmSend = true;
   }
   else
   {
      confirmSend = READ_CONFIG_BOOL(m_profile, MP_CONFIRM_SEND);
   }

   if ( confirmSend )
   {
      if ( !MDialog_YesNoDialog(_("Send this message?"), m_frame) )
      {
         return false;
      }
   }

   return true;
}

void SendMessageCC::Preview(String *text)
{
   String textTmp;
   if ( !WriteToString(textTmp) )
      return;

   MDialog_ShowText(m_frame, _("Outgoing message text"), textTmp, "SendPreview");

   if ( text )
      *text = textTmp;
}

bool
SendMessageCC::SendNow(String *errGeneral, String *errDetailed)
{
   CHECK( errGeneral && errDetailed, false,
            "must have valid error strings pointers" );

   ASSERT_MSG( m_wasBuilt, "Build() must have been called!" );

   if ( !MailFolder::Init() )
      return false;

   MCclientLocker locker;

   SENDSTREAM *stream = NIL;

   // construct the server string for c-client
   String server = m_ServerHost;

   // use authentication if the user name is specified
   if ( !m_UserName.empty() )
   {
      server << "/user=\"" << m_UserName << '"';

      String password;
      if ( !GetPassword(password) )
      {
         FAIL_MSG( "should have a stored password here" );
      }

      MailFolderCC::SetLoginData(m_UserName, password);
   }

   // do we use SSL for SMTP/NNTP?
#ifdef USE_SSL
   bool supportsSSL = true;
   SSLSupport useSSL = SSLSupport_None; // just to suppress warnings
   SSLCert acceptUnsigned = SSLCert_SignedOnly;
   if ( m_Protocol == Prot_SMTP )
   {
      useSSL = m_UseSSLforSMTP;
      acceptUnsigned = m_UseSSLUnsignedforSMTP;
   }
   else if ( m_Protocol == Prot_NNTP )
   {
      useSSL = m_UseSSLforNNTP;
      acceptUnsigned = m_UseSSLUnsignedforNNTP;
   }
   else // SSL doesn't make sense
   {
      supportsSSL = false;
   }

   if ( supportsSSL )
   {
      if ( useSSL == SSLSupport_None )
      {
         server << "/notls";
      }
      else // do use SSL/TLS
      {
         if ( !InitSSL() )
         {
            if ( useSSL != SSLSupport_TLSIfAvailable )
            {
               *errGeneral = _("SSL support is unavailable; "
                               "try disabling SSL/TLS.");

               return false;
            }
            //else: not a fatal error, fall back to unencrypted
         }

         switch ( useSSL )
         {
            case SSLSupport_SSL:
               server << "/ssl";
               break;

            case SSLSupport_TLS:
               server << "/tls";
               break;

            default:
               FAIL_MSG( "unknown value of SSLSupport" );
               // fall through

            case SSLSupport_TLSIfAvailable:
               // nothing to do -- this is the default
               break;
         }

         if ( acceptUnsigned )
         {
            server << "/novalidate-cert";
         }
      }
   }
   //else: other protocols don't use SSL
#endif // USE_SSL

   // prepare the hostlist for c-client: we use only one server
   wxCharBuffer serverAsCharBuf(server.mb_str());
   char *hostlist[2];
   hostlist[0] = (char *)serverAsCharBuf.data();
   hostlist[1] = NIL;

   int options = READ_CONFIG(m_profile, MP_DEBUG_CCLIENT) ? SOP_DEBUG : 0;

   switch ( m_Protocol )
   {
      case Prot_SMTP:
         {
            wxLogTrace(TRACE_SEND,
                       "Trying to open connection to SMTP server \"%s\"",
                       m_ServerHost.c_str());

            if ( READ_CONFIG(m_profile, MP_SMTP_USE_8BIT) )
            {
               options |= SOP_8BITMIME;
            }

#ifdef USE_OWN_CCLIENT
            // do we need to disable any authentificators (presumably because
            // they're incorrectly implemented by the server)?
            const String authsToDisable(READ_CONFIG_TEXT(m_profile,
                                                         MP_SMTP_DISABLED_AUTHS));
            if ( !authsToDisable.empty() )
            {
               smtp_parameters(SET_SMTPDISABLEDAUTHS,
                                 (char *)authsToDisable.c_str());
            }
#endif // USE_OWN_CCLIENT

            stream = smtp_open_full(NIL, hostlist, CONST_CCAST("smtp"), NIL, options);

#ifdef USE_OWN_CCLIENT
            // don't leave any dangling pointers
            if ( !authsToDisable.empty() )
            {
               smtp_parameters(SET_SMTPDISABLEDAUTHS, NIL);
            }
#endif // USE_OWN_CCLIENT
         }
         break;

      case Prot_NNTP:
         wxLogTrace(TRACE_SEND,
                    "Trying to open connection to NNTP server \"%s\"",
                    m_ServerHost.c_str());

         stream = nntp_open_full(NIL, hostlist, CONST_CCAST("nntp"), NIL, options);
         break;

#ifdef OS_UNIX
         case Prot_Sendmail:
         {
            String lfOnly;

            // We gotta translate CRLF to LF, because string generated
            // by c-client has network newlines (CRLF) and sendmail pipe
            // must have Unix newlines (LF). Maybe WriteToString could do
            // it before, but WriteToFolder (or MailFolderCC method?) would
            // then have to translate back to CRLF before passing it to
            // c-client.
            WriteToString(lfOnly);
            lfOnly = strutil_enforceLF(lfOnly);

            // write to temp file:
            wxFile out;
            MTempFileName tmpFN(&out);
            const String& filename = tmpFN.GetName();

            bool success = false;
            if ( !filename.empty() )
            {
               size_t written = out.Write(lfOnly, lfOnly.Length());
               out.Close();
               if ( written == lfOnly.Length() )
               {
                  int rc = system(m_SendmailCmd + " < " + filename);
                  if ( WEXITSTATUS(rc) != 0 )
                  {
                     errDetailed->Printf(_("Failed to execute local MTA \"%s\""),
                                         m_SendmailCmd.c_str());
                  }
                  else
                  {
                     success = true;
                  }
               }
               else
               {
                  errDetailed->Printf(_("Failed to write to temporary file \"%s\""),
                                      filename.c_str());
               }
            }
            else
            {
               *errDetailed = _("Failed to get a temporary file name");
            }

            if ( !success )
            {
               *errGeneral = _("Failed to send message via local MTA, maybe "
                               "it's not configured correctly?\n"
                               "\n"
                               "Please try using an SMTP server if you are not "
                               "sure.");
            }

            return success;
         }
#endif // OS_UNIX

         // make gcc happy
         case Prot_Illegal:
         default:
            FAIL_MSG("illegal protocol");
   }

   bool success;
   if ( stream )
   {
      Rfc822OutputRedirector redirect(m_headerNames, m_headerValues);

      switch ( m_Protocol )
      {
         case Prot_SMTP:
            success = smtp_mail(stream, CONST_CCAST("MAIL"),
                                m_Envelope, GetBody()) != NIL;
            *errDetailed = wxString::From8BitData(stream->reply);
            smtp_close (stream);
            break;

         case Prot_NNTP:
            success = nntp_mail(stream, m_Envelope, GetBody()) != NIL;
            *errDetailed = wxString::From8BitData(stream->reply);
            nntp_close (stream);
            break;

         // make gcc happy
         case Prot_Illegal:
         default:
            FAIL_MSG("illegal protocol");
            success = false;
      }

      if ( success )
         return true;
   }
   //else: error in opening stream

   MLogCircle& log = MailFolder::GetLogCircle();
   if ( !errDetailed->empty() )
   {
      log.Add(*errDetailed);
   }

   // give the general error message anyhow
   *errGeneral = m_Protocol == Prot_SMTP ? _("Failed to send the message")
                                         : _("Failed to post the article");

   // and then try to give more details about what happened
   const String explanation = log.GuessError();
   if ( explanation.empty() )
   {
      *errGeneral += ".";
   }
   else // have explanation
   {
      *errGeneral += ":\n\n" + explanation;
   }

   return false;
}

void
SendMessageCC::AfterSending()
{
   for ( StringList::iterator i = m_FccList.begin();
         i != m_FccList.end();
         i++ )
   {
      wxLogTrace(TRACE_SEND, "FCCing message to %s", (*i)->c_str());

      WriteToFolder(**i);
   }
}

// ----------------------------------------------------------------------------
// SendMessageCC output routines
// ----------------------------------------------------------------------------

bool SendMessageCC::WriteMessage(soutr_t writer, void *where)
{
   if ( !Build() )
      return false;

   // this buffer is only for the message headers, so normally 16Kb should be
   // enough - but ideally we'd like to have some way to be sure it doesn't
   // overflow and I don't see any :-(
   char headers[16*1024];

   // install our output routine temporarily
   Rfc822OutputRedirector redirect(m_headerNames, m_headerValues,
                                   Rfc822OutputRedirector::AddBcc);

   return rfc822_output(headers, m_Envelope, GetBody(),
                        writer, where, NIL) != NIL;
}

bool
SendMessageCC::WriteToString(String& output)
{
   output.Empty();
   output.Alloc(4096); // FIXME: quick way go get message size?

   if ( !WriteMessage(write_str_output, &output) )
   {
      ERRORMESSAGE(("Failed to create the message text."));

      return false;
   }

   return true;
}

/** Writes the message to a file
    @param filename file where to write to
*/
bool
SendMessageCC::WriteToFile(const String &filename, bool append)
{
   // note that we have to repeat ios::out and binary below, otherwise gcc 3.0
   // refuses to compile it as it converts everything to int and then fails
   ofstream ostr(filename.mb_str(),
                 append ? ios::out | ios::binary
                        : ios::out | ios::binary | ios::trunc);

   bool ok = !(!ostr || ostr.bad());
   if ( ok )
   {
      // we need a valid "From " line or c-client wouldn't recognize this file
      // as a MBOX one
      time_t t = time(NULL);
      ostr << "From Mahogany-AutoSave " << ctime(&t);
      ok = !ostr.fail();
   }

   if ( ok )
      ok = WriteMessage(write_stream_output, &ostr);

   if ( !ok )
   {
      ERRORMESSAGE((_("Failed to write message to file '%s'."),
                    filename.c_str()));
   }

   return ok;
}

bool
SendMessageCC::WriteToFolder(String const &name)
{
   MFolder_obj folder(name);
   if ( !folder )
   {
      ERRORMESSAGE((_("Can't save sent message in the folder '%s' "
                      "which doesn't exist."), name.c_str()));
      return false;
   }

   MailFolder_obj mf(MailFolder::OpenFolder(folder));
   if ( !mf )
   {
      ERRORMESSAGE((_("Can't open folder '%s' to save the message to."),
                    name.c_str()));
      return false;
   }

   String str;
   return WriteToString(str) && mf->AppendMessage(str);
}

// ----------------------------------------------------------------------------
// output helpers
// ----------------------------------------------------------------------------

// rfc822_output() callback for writing into a file
static long write_stream_output(void *stream, char *string)
{
   ostream *o = (ostream *)stream;
   *o << static_cast<const char *>(wxString::From8BitData(string).ToUTF8());
   if ( o->fail() )
      return NIL;

   return 1;
}

// rfc822_output() callback for writing to a string
static long write_str_output(void *stream, char *string)
{
   String *o = (String *)stream;
   o->append(wxString::From8BitData(string));

   return 1;
}

// ----------------------------------------------------------------------------
// Rfc822OutputRedirector
// ----------------------------------------------------------------------------

bool Rfc822OutputRedirector::ms_outputBcc = false;

const char **Rfc822OutputRedirector::ms_HeaderNames = NULL;
const char **Rfc822OutputRedirector::ms_HeaderValues = NULL;

MTMutex Rfc822OutputRedirector::ms_mutexExtraHeaders;

Rfc822OutputRedirector::Rfc822OutputRedirector(const char **extraHeadersNames,
                                               const char **extraHeadersValues,
                                               int flags)
{
   ms_mutexExtraHeaders.Lock();

   ms_outputBcc = (flags & AddBcc) != 0;
   ms_HeaderNames = extraHeadersNames;
   ms_HeaderValues = extraHeadersValues;

   m_oldRfc822Output = mail_parameters(NULL, GET_RFC822OUTPUT, NULL);
   (void)mail_parameters(NULL, SET_RFC822OUTPUT, (void *)FullRfc822Output);
}

Rfc822OutputRedirector::~Rfc822OutputRedirector()
{
   (void)mail_parameters(NULL, SET_RFC822OUTPUT, m_oldRfc822Output);

   // Avoid leaving dangling pointers.
   ms_HeaderNames = NULL;
   ms_HeaderValues = NULL;

   ms_mutexExtraHeaders.Unlock();
}

// our rfc822_output replacement: it also adds the custom headers and writes
// BCC unlike rfc822_output()
long Rfc822OutputRedirector::FullRfc822Output(char *headers,
                                              ENVELOPE *env,
                                              BODY *body,
                                              soutr_t writer,
                                              void *stream,
                                              long ok8bit)
{
  if ( ok8bit )
     rfc822_encode_body_8bit(env, body);
  else
     rfc822_encode_body_7bit(env, body);

  // write standard headers
  rfc822_header(headers, env, body);

  // remove the last blank line
  size_t i = strlen(headers);
  if ( i > 4 && headers[i-4] == '\015' )
  {
     headers[i-2] = '\0';
  }
  else
  {
     // just in case MRC decides to change it in the future...
     wxFAIL_MSG("cclient message header doesn't have terminating blank line?");
  }

  // save the pointer as rfc822_address_line() modifies it
  char *headersOrig = headers;

  // cclient omits bcc, but we need it sometimes
  if ( ms_outputBcc )
  {
     rfc822_address_line(&headers, CONST_CCAST("Bcc"), env, env->bcc);
  }

  // and add all other additional custom headers at the end
  if ( ms_HeaderNames )
  {
     for ( size_t n = 0; ms_HeaderNames[n]; n++ )
     {
        rfc822_header_line(&headers,
                           const_cast<char *>(ms_HeaderNames[n]),
                           env,
                           const_cast<char *>(ms_HeaderValues[n]));
     }
  }

  // terminate the headers part
  strcat(headers, "\015\012");

  if ( !(*writer)(stream, headersOrig) )
     return NIL;

  if ( body && !rfc822_output_body(body, writer, stream) )
     return NIL;

  return 1;
}

