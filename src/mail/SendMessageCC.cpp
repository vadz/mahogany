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

#ifdef __GNUG__
#   pragma implementation "SendMessageCC.h"
#endif

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

#include "LogCircle.h"

#include "AddressCC.h"
#include "Message.h"
#include "MFolder.h"

// has to be included before SendMessage.h, as it includes windows.h which
// defines SendMessage under Windows
#include <wx/fontmap.h>          // for GetEncodingName()
#if defined(__CYGWIN__) || defined(__MINGW32__)
#  undef SendMessage
#endif

#include "SendMessage.h"
#include "SendMessageCC.h"

#include "XFace.h"
#include "gui/wxMDialogs.h"

#include <wx/file.h>
#include <wx/datetime.h>

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
extern const MOption MP_SMTP_DISABLED_AUTHS;
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

// the encodings defined by RFC 2047
//
// NB: don't change the values of the enum elements, EncodeHeaderString()
//     relies on them being what they are!
enum MimeEncoding
{
   MimeEncoding_Unknown,
   MimeEncoding_Base64 = 'B',
   MimeEncoding_QuotedPrintable = 'Q'
};

// trace mask for message sending/queuing operations
#define TRACE_SEND   _T("send")

// ----------------------------------------------------------------------------
// prototypes
// ----------------------------------------------------------------------------

static long write_stream_output(void *, char *);
static long write_str_output(void *, char *);

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// an object of this class temporarily redirects cclient rfc822_output to our
// function abd restores the old function in its dtor
//
// this class is effectively a singleton as it locks a mutex in its ctor to
// prevent us from trying to send more than one message at once (bad things
// will happen inside cclient if we do)
class Rfc822OutputRedirector
{
public:
   // the ctor may redirect rfc822 output to write bcc as well or to not do
   // it, it is important to get it right or BCC might be sent as a message
   // header and be seen by the recipient!
   Rfc822OutputRedirector(bool outputBcc, SendMessageCC *msg);
   ~Rfc822OutputRedirector();

   static long FullRfc822Output(char *, ENVELOPE *, BODY *,
                                soutr_t, void *, long);

private:
   void SetHeaders(const char **names, const char **values);

   // the old output routine
   static void *ms_oldRfc822Output;

   // should we output BCC?
   static bool ms_outputBcc;

   // the extra headers written by FullRfc822Output()
   static const char **ms_HeaderNames;
   static const char **ms_HeaderValues;

   // and a mutex to protect them
   static MMutex ms_mutexExtraHeaders;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// SendMessage
// ----------------------------------------------------------------------------

/* static */
SendMessage *
SendMessage::Create(Profile *profile, Protocol protocol, wxFrame *frame)
{
   return new SendMessageCC(profile, protocol, frame);
}

/* static */
SendMessage *
SendMessage::CreateResent(Profile *profile,
                          const Message *message,
                          wxFrame *frame)
{
   return new SendMessageCC(profile, Prot_Default, frame, message);
}

SendMessage::~SendMessage()
{
}

// ----------------------------------------------------------------------------
// SendMessageCC creation and destruction
// ----------------------------------------------------------------------------

SendMessageCC::SendMessageCC(Profile *profile,
                             Protocol protocol,
                             wxFrame *frame,
                             const Message *message)
{
   m_frame = frame;
   m_encHeaders = wxFONTENCODING_SYSTEM;

   m_headerNames =
   m_headerValues = NULL;

   m_wasBuilt = false;

   m_Envelope = mail_newenvelope();
   m_Body = mail_newbody();

   m_NextPart =
   m_LastPart = NULL;

   if ( !profile )
   {
      FAIL_MSG( _T("SendMessageCC::Create() requires profile") );

      profile = mApplication->GetProfile();
   }

   m_profile = profile;
   m_profile->IncRef();

   // choose the protocol: mail (SMTP, Sendmail) or NNTP
   if ( protocol == Prot_Default )
   {
#ifdef OS_UNIX
      if ( READ_CONFIG_BOOL(profile, MP_USE_SENDMAIL) )
         protocol = Prot_Sendmail;
      else
#endif // OS_UNIX
         protocol = Prot_SMTP;
   }

   m_Protocol = protocol;

   switch ( m_Protocol )
   {
      default:
         FAIL_MSG( _T("unknown SendMessage protocol") );
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
   AddressList_obj addrList(AddressList::CreateFromAddress(m_profile));
   Address *addrFrom = addrList->GetFirst();
   if ( addrFrom )
   {
      m_From = addrFrom->GetAddress();
   }

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

   // check that we have password if we use it
   //
   // FIXME: why do we do it here and not when sending??
   if ( !m_UserName.empty() && m_Password.empty() )
   {
      MDialog_GetPassword(protocol, m_ServerHost,
                          &m_Password, &m_UserName, m_frame);
   }
   else // we do have it stored
   {
      m_Password = strutil_decrypt(m_Password);
   }

   // finally, special init for resent messages
   // -----------------------------------------

   if ( message )
      InitResent(message);
   else
      InitNew();
}

void SendMessageCC::InitNew()
{
   // FIXME: why do we do it here, it seems to be overwritten in Build()??
   m_Body->type = TYPEMULTIPART;
   m_Body->nested.part = mail_newbody_part();
   m_Body->nested.part->next = NULL;
   m_NextPart = m_Body->nested.part;
   m_LastPart = m_NextPart;

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
      m_Sender.Trim().Trim(FALSE); // remove all spaces on begin/end

      if ( Message::CompareAddresses(m_From, m_Sender) )
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
      m_XFaceFile = m_profile->readEntry(MP_COMPOSE_XFACE_FILE, _T(""));

   m_CharSet = READ_CONFIG_TEXT(m_profile,MP_CHARSET);
}

void SendMessageCC::InitResent(const Message *message)
{
   CHECK_RET( message, _T("message being resent can't be NULL") );

   CHECK_RET( m_Envelope, _T("envelope must be created in InitResent") );

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

         if ( STARTS_WITH(p, _T("Delivered-To:")) ||
               STARTS_WITH(p, _T("Received:")) ||
                 STARTS_WITH(p, _T("Resent-")) )
         {
            hdr += _T("X-");
         }

#undef STARTS_WITH
      }

      hdr += *p;
   }

   m_Envelope->remail = cpystr(wxConvertWX2MB(hdr.c_str()));

   // now copy the body: note that we have to use ENC7BIT here to prevent
   // c-client from (re)encoding the body
   m_Body->type = TYPETEXT;
   m_Body->encoding = ENC7BIT;
   m_Body->subtype = cpystr("PLAIN");

   // FIXME: we potentially copy a lot of data here!
   String text = message->FetchText();
   m_Body->contents.text.data = (unsigned char *)cpystr(wxConvertWX2MB(text.c_str()));
   m_Body->contents.text.size = text.length();
}

SendMessageCC::~SendMessageCC()
{
   mail_free_envelope (&m_Envelope);
   mail_free_body (&m_Body);

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

   m_profile->DecRef();
}

// ----------------------------------------------------------------------------
// SendMessageCC encodings
// ----------------------------------------------------------------------------

static MimeEncoding GetMimeEncodingForFontEncoding(wxFontEncoding enc)
{
   // QP should be used for the encodings which mostly overlap with US_ASCII,
   // Base64 for the others - choose the encoding method
   switch ( enc )
   {
      case wxFONTENCODING_ISO8859_1:
      case wxFONTENCODING_ISO8859_2:
      case wxFONTENCODING_ISO8859_3:
      case wxFONTENCODING_ISO8859_4:
      case wxFONTENCODING_ISO8859_9:
      case wxFONTENCODING_ISO8859_10:
      case wxFONTENCODING_ISO8859_13:
      case wxFONTENCODING_ISO8859_14:
      case wxFONTENCODING_ISO8859_15:

      case wxFONTENCODING_CP1250:
      case wxFONTENCODING_CP1252:
      case wxFONTENCODING_CP1254:
      case wxFONTENCODING_CP1257:

      case wxFONTENCODING_UTF7:
      case wxFONTENCODING_UTF8:

         return MimeEncoding_QuotedPrintable;

      case wxFONTENCODING_ISO8859_5:
      case wxFONTENCODING_ISO8859_6:
      case wxFONTENCODING_ISO8859_7:
      case wxFONTENCODING_ISO8859_8:
      case wxFONTENCODING_ISO8859_11:
      case wxFONTENCODING_ISO8859_12:

      case wxFONTENCODING_CP1251:
      case wxFONTENCODING_CP1253:
      case wxFONTENCODING_CP1255:
      case wxFONTENCODING_CP1256:

      case wxFONTENCODING_KOI8:
         return MimeEncoding_Base64;

      default:
         FAIL_MSG( _T("unknown encoding") );

      case wxFONTENCODING_SYSTEM:
         return MimeEncoding_Unknown;
   }
}

// ----------------------------------------------------------------------------
// SendMessageCC header stuff
// ----------------------------------------------------------------------------

void
SendMessageCC::SetHeaderEncoding(wxFontEncoding enc)
{
   m_encHeaders = enc;
}

// returns true if the character must be encoded in an SMTP [address] header
static inline bool NeedsEncodingInHeader(unsigned char c)
{
   return iscntrl(c) || c >= 127;
}

String
SendMessageCC::EncodeHeaderString(const String& header, bool /* isaddr */)
{
   // if a header contains "=?", encode it anyhow to avoid generating invalid
   // encoded words
   if ( !wxStrstr(header, _T("=?")) )
   {
      // only encode the strings which contain the characters unallowed in RFC
      // 822 headers
      const unsigned char *p;
      for ( p = (unsigned char *)header.c_str(); *p; p++ )
      {
         if ( NeedsEncodingInHeader(*p) )
            break;
      }

      if ( !*p )
      {
         // string has only valid chars, don't encode
         return header;
      }
   }

   // get the encoding in RFC 2047 sense: choose the most reasonable one
   wxFontEncoding enc = m_encHeaders == wxFONTENCODING_SYSTEM
                           ? wxFONTENCODING_ISO8859_1
                           : m_encHeaders;

   MimeEncoding enc2047 = GetMimeEncodingForFontEncoding(enc);

   if ( enc2047 == MimeEncoding_Unknown )
   {
      FAIL_MSG( _T("should have valid MIME encoding") );

      enc2047 = MimeEncoding_QuotedPrintable;
   }

   // get the name of the charset to use
   String csName = EncodingToCharset(enc);
   if ( csName.empty() )
   {
      FAIL_MSG( _T("should have a valid charset name!") );

      csName = _T("UNKNOWN");
   }

   // the entire encoded header
   String headerEnc;
   headerEnc.reserve(csName.length() + 2*header.length() + 16);

   // encode the header splitting it in the chunks such that they will be no
   // longer than 75 characters each
   const wxChar *s = header.c_str();
   while ( *s )
   {
      // if this is not the first line, insert a line break
      if ( !headerEnc.empty() )
      {
         headerEnc << _T("\r\n ");
      }

      static const size_t RFC2047_MAXWORD_LEN = 75;

      // how many characters may we put in this encoded word?
      size_t len = 0;

      // take into account the length of "=?charset?...?="
      int lenRemaining = RFC2047_MAXWORD_LEN - (5 + csName.length());

      // for QP we need to examine all characters
      if ( enc2047 == MimeEncoding_QuotedPrintable )
      {
         for ( ; s[len]; len++ )
         {
            const char c = s[len];

            // normal characters stand for themselves in QP, the encoded ones
            // take 3 positions (=XX)
            lenRemaining -= (NeedsEncodingInHeader(c) || strchr(" \t=?", c))
                              ? 3 : 1;

            if ( lenRemaining <= 0 )
            {
               // can't put any more chars into this word
               break;
            }
         }
      }
      else // Base64
      {
         // we can calculate how many characters we may put into lenRemaining
         // directly
         len = (lenRemaining / 4) * 3 - 2;

         // but not more than what we have
         size_t lenMax = wxStrlen(s);
         if ( len > lenMax )
         {
            len = lenMax;
         }
      }

      // do encode this word
      unsigned char *text = (unsigned char *)s; // cast for cclient

      // length of the encoded text and the text itself
      unsigned long lenEnc;
      unsigned char *textEnc;

      if ( enc2047 == MimeEncoding_QuotedPrintable )
      {
            textEnc = rfc822_8bit(text, len, &lenEnc);
      }
      else // MimeEncoding_Base64
      {
            textEnc = rfc822_binary(text, len, &lenEnc);
            while ( textEnc[lenEnc - 2] == '\r' && textEnc[lenEnc - 1] == '\n' )
            {
               // discard eol which we don't need in the header
               lenEnc -= 2;
            }
      }

      // put into string as we might want to do some more replacements...
      String encword((wxChar*)textEnc, (size_t)lenEnc);
      //String encword = strutil_strdup(wxConvertMB2WX(textEnc));

      // hack: rfc822_8bit() doesn't encode spaces normally but we must
      // do it inside the headers
      //
      // we also have to encode '?'s in the headers which are not encoded by it
      if ( enc2047 == MimeEncoding_QuotedPrintable )
      {
         String encword2;
         encword2.reserve(encword.length());

         bool replaced = false;
         for ( const wxChar *p = encword.c_str(); *p; p++ )
         {
            switch ( *p )
            {
               case ' ':
                  encword2 += _T("=20");
                  break;

               case '\t':
                  encword2 += _T("=09");
                  break;

               case '?':
                  encword2 += _T("=3F");
                  break;

               default:
                  encword2 += *p;

                  // skip assignment to replaced below
                  continue;
            }

            replaced = true;
         }

         if ( replaced )
         {
            encword = encword2;
         }
      }

      // append this word to the header
      headerEnc << _T("=?") << csName << _T('?') << (char)enc2047 << _T('?')
                << encword
                << _T("?=");

      fs_give((void **)&textEnc);

      // skip the already encoded part
      s += len;
   }

   return headerEnc;
}

// unlike EncodeHeaderString(), we should only encode the personal name part of the
// address headers
void
SendMessageCC::EncodeAddress(struct mail_address *adr)
{
   if ( adr->personal )
   {
      char *tmp = adr->personal;
      adr->personal = cpystr(wxConvertWX2MB(EncodeHeaderString(wxConvertMB2WX(tmp), true /* address field */)));

      fs_give((void **)&tmp);
   }
}

void
SendMessageCC::EncodeAddressList(struct mail_address *adr)
{
   while ( adr )
   {
      EncodeAddress(adr);

      adr = adr->next;
   }
}

void
SendMessageCC::SetSubject(const String &subject)
{
   if(m_Envelope->subject)
      fs_give((void **)&m_Envelope->subject);

   String subj = EncodeHeaderString(subject);
   m_Envelope->subject = cpystr(wxConvertWX2MB(subj.c_str()));
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
      ASSERT_MSG( m_Envelope->return_path == NIL, _T("Return-Path already set?") );

      m_Envelope->return_path = mail_newaddr();
      m_Envelope->return_path->mailbox = cpystr(adr->mailbox);
      m_Envelope->return_path->host = cpystr(adr->host);
   }
}

void
SendMessageCC::SetAddressField(ADDRESS **pAdr, const String& address)
{
   ASSERT_MSG( !*pAdr, _T("shouldn't be called twice") );

   if ( address.empty() )
   {
      // nothing to do
      return;
   }

   // parse into ADDRESS struct
   *pAdr = ParseAddressList(address, m_DefaultHost);

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

         DBGMESSAGE((_T("Invalid recipient address '%s' ignored."),
                    AddressCC(adr).GetAddress().c_str()));

         // prevent mail_free_address() from freeing the entire list tail
         adr->next = NULL;

         mail_free_address(&adr);
      }

      adr = adrNext;
   }

   EncodeAddressList(adrStart);
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
   ASSERT_MSG( m_Protocol == Prot_NNTP, _T("can't post and send message") );

   if(groups.Length())
   {
      ASSERT(m_Envelope->newsgroups == NIL);
      m_Envelope->newsgroups = strdup(wxConvertWX2MB(groups));
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

String
SendMessageCC::EncodingToCharset(wxFontEncoding enc)
{
   // translate encoding to the charset
   wxString cs;
   if ( enc != wxFONTENCODING_SYSTEM && enc != wxFONTENCODING_DEFAULT )
   {
      cs = wxFontMapper::GetEncodingName(enc).Upper();
   }

   return cs;
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

   if (name == _T("TO"))
      ; //TODO: Fix this?SetAddresses(*value);
   else if(name == _T("CC"))
      ; //SetAddresses("",*value);
   else if(name == _T("BCC"))
      ; //SetAddresses("","",*value);
   else if ( name == _T("MIME-VERSION") ||
             name == _T("CONTENT-TYPE") ||
             name == _T("CONTENT-DISPOSITION") ||
             name == _T("CONTENT-TRANSFER-ENCODING") ||
             name == _T("MESSAGE-ID") )
   {
      ERRORMESSAGE((_("The value of the header '%s' cannot be modified."),
                    name.c_str()));

      return;
   }
   else if ( name == _T("SUBJECT") )
   {
      SetSubject(value);
   }
   else // any other headers
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
   CHECK_RET( i != m_extraHeaders.end(), _T("RemoveHeaderEntry(): no such header") );

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

   return String::Format(_T("<Mahogany-%s-%lu-%s.%02u@%s>"),
                         M_VERSION,
                         s_pid,
                         dt.Format(_T("%Y%m%d-%H%M%S")).c_str(),
                         s_numInSec,
                         hostname);
}

void
SendMessageCC::Build(bool forStorage)
{
   if ( m_wasBuilt )
   {
      // message was already build
      return;
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
   // NB: we do allow the user to override the date header because this is
   //     useful when editing a previously postponed message
   if ( !HasHeaderEntry(_T("Date")) )
   {
      char tmpbuf[MAILTMPLEN];
      rfc822_date (tmpbuf);
      m_Envelope->date = cpystr(tmpbuf);
   }

   // Message-Id: we should always generate it ourselves (section 3.6.4 of RFC
   // 2822) but at least some MTAs (exim) reject it if the id-right part of it
   // is not a FQDN so don't do it in this case
   if ( m_DefaultHost.find('.') != String::npos )
   {
      m_Envelope->message_id = cpystr(wxConvertWX2MB(BuildMessageId(wxConvertWX2MB(m_DefaultHost))));
   }

   // don't add any more headers to the message being resent
   if ( m_Envelope->remail )
   {
      return;
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
         AddHeaderEntry(_T("X-BCC"), m_Bcc);
   }
   else // send, not store
   {
      /*
         If sending directly, we need to do the opposite: this message
         might have come from the Outbox queue, so we translate X-BCC
         back to a proper bcc setting:
       */
      if ( HasHeaderEntry(_T("X-BCC")) )
      {
         if ( m_Envelope->bcc )
         {
            mail_free_address(&m_Envelope->bcc);
         }

         SetAddressField(&m_Envelope->bcc, GetHeaderEntry(_T("X-BCC")));

         // don't send X-BCC field or the recipient would still see the BCC
         // contents (which is highly undesirable!)
         RemoveHeaderEntry(_T("X-BCC"));
      }
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
         ++i, ++h )
   {
      m_headerNames[h] = strutil_strdup(wxConvertWX2MB(i->m_name));
      if ( wxStricmp(wxConvertMB2WX(m_headerNames[h]), _T("Reply-To")) == 0 )
         replyToSet = true;
      else if ( wxStricmp(wxConvertMB2WX(m_headerNames[h]), _T("X-Mailer")) == 0 )
         xmailerSet = true;

      m_headerValues[h] = strutil_strdup(wxConvertWX2MB(i->m_value));
   }

   // add X-Mailer header if it wasn't overridden by the user (yes, we do allow
   // it - why not?)
   if ( !xmailerSet )
   {
      m_headerNames[h] = strutil_strdup("X-Mailer");

      // NB: do *not* translate these strings, this doesn't make much sense
      //     (the user doesn't usually see them) and, worse, we shouldn't
      //     include 8bit chars (which may - and do - occur in translations) in
      //     headers!
      String version;
      version << _T("Mahogany ") << M_VERSION_STRING;
#ifdef OS_UNIX
      version  << _T(", compiled for ") << M_OSINFO;
#else // Windows
      version << _T(", running under ") << wxGetOsDescription();
#endif // Unix/Windows
      m_headerValues[h++] = strutil_strdup(wxConvertWX2MB(version));
   }

   // set Reply-To if it hadn't been set by the user as a custom header
   if ( !replyToSet )
   {
      ASSERT_MSG( !HasHeaderEntry(_T("Reply-To")), _T("logic error") );

      if ( !m_ReplyTo.empty() )
      {
         m_headerNames[h] = strutil_strdup("Reply-To");
         m_headerValues[h++] = strutil_strdup(wxConvertWX2MB(m_ReplyTo));
      }
   }

#ifdef HAVE_XFACES
   // add an XFace?
   if ( !HasHeaderEntry(_T("X-Face")) && !m_XFaceFile.empty() )
   {
      XFace xface;
      if ( xface.CreateFromFile(m_XFaceFile) )
      {
         m_headerNames[h] = strutil_strdup("X-Face");
         m_headerValues[h] = strutil_strdup(wxConvertWX2MB(xface.GetHeaderLine()));
         if(strlen(m_headerValues[h]))  // paranoid, I know.
         {
            ASSERT_MSG( ((char*) (m_headerValues[h]))[strlen(m_headerValues[h])-2] == '\r', _T("String should have been DOSified") );
            ASSERT_MSG( ((char*) (m_headerValues[h]))[strlen(m_headerValues[h])-1] == '\n', _T("String should have been DOSified") );
            ((char*) (m_headerValues[h]))[strlen(m_headerValues[h])-2] =
               '\0'; // cut off \n
         }
         h++;
      }
      //else: couldn't read X-Face from file (complain?)
   }
#endif // HAVE_XFACES

   m_headerNames[h] = NULL;
   m_headerValues[h] = NULL;

   mail_free_body_part(&m_LastPart->next);
   m_LastPart->next = NULL;

   // check if there is only one part, then we don't need multipart/mixed
   if(m_LastPart == m_Body->nested.part)
   {
      BODY *oldbody = m_Body;
      m_Body = &(m_LastPart->body);
      oldbody->nested.part = NULL;
      mail_free_body(&oldbody);
   }
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
   BODY *bdy;
   unsigned char *data;

   // the text must be NUL terminated or it will not be encoded correctly and
   // it won't hurt to add a NUL after the end of data in the other cases as
   // well (note that cclient won't get this last NUL as len will be
   // decremented below)
   len += sizeof(char);
   data = (unsigned char *) fs_get (len);
   len -= sizeof(char);
   data[len] = '\0';
   memcpy(data, buf, len);

   String subtype(subtype_given);
   if( subtype.length() == 0 )
   {
      if ( type == TYPETEXT )
         subtype = _T("PLAIN");
      else if ( type == TYPEAPPLICATION )
         subtype = _T("OCTET-STREAM");
      else
      {
         // shouldn't send message without MIME subtype, but we don't have any
         // and can't find the default!
         ERRORMESSAGE((_("MIME type specified without subtype and\n"
                         "no default subtype for this type.")));
         subtype = _T("UNKNOWN");
      }
   }

   bdy = &(m_NextPart->body);
   bdy->type = type;

   bdy->subtype = cpystr((char *)subtype.c_str());

   bdy->contents.text.data = data;
   bdy->contents.text.size = len;

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
         if ( enc == wxFONTENCODING_SYSTEM || strutil_is7bit(data) )
         {
            bdy->encoding = ENC7BIT;
         }
         else // we have 8 bit chars, need to encode them
         {
            // some encodings should be encoded in QP as they typically contain
            // only a small number of non printable characters while others
            // should be incoded in Base64 as almost all characters used in them
            // are outside basic Ascii set
            switch ( GetMimeEncodingForFontEncoding(enc) )
            {
               case MimeEncoding_Unknown:
               case MimeEncoding_QuotedPrintable:
                  // automatically translated to QP by c-client
                  bdy->encoding = ENC8BIT;
                  break;

               default:
                  FAIL_MSG( _T("unknown MIME encoding") );
                  // fall through

               case MimeEncoding_Base64:
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
         bdy->encoding = ENCBINARY;
   }

   m_NextPart->next = mail_newbody_part();
   m_LastPart = m_NextPart;
   m_NextPart = m_NextPart->next;
   m_NextPart->next = NULL;

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
         if ( name.Lower() == _T("charset") )
         {
            if ( hasCharset )
            {
               // although not fatal, this shouldn't happen
               wxLogDebug(_T("Multiple CHARSET parameters!"));
            }

            hasCharset = true;
         }

         par->attribute = strdup(wxConvertWX2MB(name));
         par->value     = strdup(wxConvertWX2MB(i->value));
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
         cs = _T("US-ASCII");
      }
      else // 8bit message
      {
         cs = EncodingToCharset(enc);
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
         par->value     = strdup(wxConvertWX2MB(cs));
         par->next      = lastpar;
         lastpar = par;
      }
   }

   bdy->parameter = lastpar;
   bdy->disposition.type = strdup(wxConvertWX2MB(disposition));
   if ( dlist )
   {
      PARAMETER *lastpar = NULL,
                *par;

      MessageParameterList::iterator i;
      for ( i = dlist->begin(); i != dlist->end(); i++ )
      {
         par = mail_newbody_parameter();
         par->attribute = strdup(wxConvertWX2MB(i->name));
         par->value     = strdup(wxConvertWX2MB(i->value));
         par->next      = NULL;
         if(lastpar)
            lastpar->next = par;
         else
            bdy->disposition.parameter = par;
      }
   }
}

// ----------------------------------------------------------------------------
// SendMessageCC sending
// ----------------------------------------------------------------------------

bool
SendMessageCC::SendOrQueue(int flags)
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
         return false;
      }
   }
#endif // USE_DIALUP

   // prepare the message for sending or queuing
   Build(!send);

   // then either send or queue it
   bool success;
   if ( send )
   {
      success = Send(flags);

      if ( success )
      {
         // save it in the local folders, if any
         for ( StringList::iterator i = m_FccList.begin();
               i != m_FccList.end();
               i++ )
         {
            wxLogTrace(TRACE_SEND, _T("FCCing message to %s"), (*i)->c_str());

            WriteToFolder(**i);
         }
      }
   }
   else // store in outbox
   {
      WriteToFolder(m_OutboxName);

      // increment counter in statusbar immediately
      mApplication->UpdateOutboxStatus();

      // and also show what we have done with the message
      wxString msg;
      if(m_Protocol == Prot_SMTP || m_Protocol == Prot_Sendmail)
         msg.Printf(_("Message queued in ´%s´."), m_OutboxName.c_str());
      else
         msg.Printf(_("Article queued in '%s'."), m_OutboxName.c_str());

      STATUSMESSAGE((msg));

      success = true;
   }

   return success;
}

void SendMessageCC::Preview(String *text)
{
   String textTmp;
   WriteToString(textTmp);
   MDialog_ShowText(m_frame, _T("Outgoing message text"), textTmp, _T("SendPreview"));

   if ( text )
      *text = textTmp;
}

bool
SendMessageCC::Send(int flags)
{
   ASSERT_MSG( m_wasBuilt, _T("Build() must have been called!") );

   if ( !MailFolder::Init() )
      return false;

   MCclientLocker locker();

#ifndef OS_UNIX
   // For non-unix systems we make sure that no-one tries to run
   // Sendmail which is unix specific. This could happen if someone
   // imports a configuration from a remote server or something like
   // this, so we play it safe and map all sendmail calls to SMTP
   // instead:
   if(m_Protocol == Prot_Sendmail)
      m_Protocol = Prot_SMTP;
#endif // !OS_UNIX

   SENDSTREAM *stream = NIL;

   // SMTP/NNTP server reply
   String reply;

   // construct the server string for c-client
   String server = m_ServerHost;

   // use authentification if the user name is specified
   if ( !m_UserName.empty() )
   {
      server << _T("/user=\"") << m_UserName << _T('"');
      MailFolderCC::SetLoginData(m_UserName, m_Password);
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
         server << _T("/notls");
      }
      else // do use SSL/TLS
      {
         if ( !InitSSL() )
         {
            if ( useSSL != SSLSupport_TLSIfAvailable )
            {
               ERRORMESSAGE((_T("SSL support is unavailable; try disabling SSL/TLS.")));

               return false;
            }
            //else: not a fatal error, fall back to unencrypted
         }

         switch ( useSSL )
         {
            case SSLSupport_SSL:
               server << _T("/ssl");
               break;

            case SSLSupport_TLS:
               server << _T("/tls");
               break;

            default:
               FAIL_MSG( _T("unknown value of SSLSupport") );
               // fall through

            case SSLSupport_TLSIfAvailable:
               // nothing to do -- this is the default
               break;
         }

         if ( acceptUnsigned )
         {
            server << _T("/novalidate-cert");
         }
      }
   }
   //else: other protocols don't use SSL
#endif // USE_SSL

   // prepare the hostlist for c-client: we use only one server
   char *hostlist[2];
   hostlist[0] = (char *)server.c_str();
   hostlist[1] = NIL;

   // preview message being sent if asked for it
   String msgText;
   bool confirmSend;
   if ( READ_CONFIG(m_profile, MP_PREVIEW_SEND) )
   {
      Preview(&msgText);

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
         mApplication->SetLastError(M_ERROR_CANCEL);

         return false;
      }
   }

   int options = READ_CONFIG(m_profile, MP_DEBUG_CCLIENT) ? OP_DEBUG : 0;

   switch ( m_Protocol )
   {
      case Prot_SMTP:
         {
            wxLogTrace(TRACE_SEND,
                       _T("Trying to open connection to SMTP server '%s'"),
                       m_ServerHost.c_str());

            if ( READ_CONFIG(m_profile, MP_SMTP_USE_8BIT) )
            {
               options |= SOP_8BITMIME;
            }

            // do we need to disable any authentificators (presumably because
            // they're incorrectly implemented by the server)?
            const String authsToDisable(READ_CONFIG_TEXT(m_profile,
                                                         MP_SMTP_DISABLED_AUTHS));
            if ( !authsToDisable.empty() )
            {
               smtp_parameters(SET_SMTPDISABLEDAUTHS,
                                 (char *)authsToDisable.c_str());
            }

            stream = smtp_open(hostlist, options);

            // don't leave any dangling pointers
            if ( !authsToDisable.empty() )
            {
               smtp_parameters(SET_SMTPDISABLEDAUTHS, NIL);
            }
         }
         break;

      case Prot_NNTP:
         wxLogTrace(TRACE_SEND, _T("Trying to open connection to NNTP server '%s'"),
                    m_ServerHost.c_str());

         stream = nntp_open(hostlist, options);
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
#if 1 // VZ: wxGetTempFileName() is broken beyond repair, don't use it for now (NB - maybe not already?)
            const wxChar *filename = wxGetTempFileName(_T("Mtemp"));
#else
            // tmpnam() is POSIX, so use it even if mk(s)temp() would be better
            // because here we have a race condition
            const char *filename = tmpnam(NULL);
#endif

            bool success = false;
            if ( filename )
            {
               wxFile out;

               // don't overwrite because someone could have created file with "bad"
               // (i.e. world readable) permissions in the meanwhile
               if ( out.Create(filename, FALSE /* don't overwrite */,
                               wxS_IRUSR | wxS_IWUSR) )
               {
                  size_t written = out.Write(lfOnly, lfOnly.Length());
                  out.Close();
                  if ( written == lfOnly.Length() )
                  {
                     String command;
                     command.Printf(_T("%s < '%s'; exec /bin/rm -f '%s'"),
                                    m_SendmailCmd.c_str(),
                                    filename, filename);
                     // HORRIBLE HACK: this should be `const char *' but wxExecute's
                     // prototype doesn't allow it...
                     wxChar *argv[4];
                     argv[0] = (wxChar *)"/bin/sh";
                     argv[1] = (wxChar *)"-c";
                     argv[2] = (wxChar *)command.c_str();
                     argv[3] = 0;  // NULL
                     success = wxExecute(argv) != 0;
                  }
               }
            }
            else
            {
               ERRORMESSAGE((_("Failed to get a temporary file name")));
            }

            if ( success )
            {
               if ( !(flags & Silent) )
               {
                  MDialog_Message(_("Message sent."),
                                  m_frame, // parent window
                                  MDIALOG_MSGTITLE,
                                  _T("MailSentMessage"));
               }
            }
            else
            {
               ERRORMESSAGE((_("Failed to send message via '%s'"),
                             m_SendmailCmd.c_str()));
            }

            return success;
         }
#endif // OS_UNIX

         // make gcc happy
         case Prot_Illegal:
         default:
            FAIL_MSG(_T("illegal protocol"));
   }

   bool success;
   if ( stream )
   {
      Rfc822OutputRedirector redirect(false /* no bcc */, this);

      switch ( m_Protocol )
      {
         case Prot_SMTP:
            success = smtp_mail (stream,"MAIL",m_Envelope,m_Body) != 0;
            reply = wxConvertMB2WX(stream->reply);
            smtp_close (stream);
            break;

         case Prot_NNTP:
            success = nntp_mail (stream,m_Envelope,m_Body) != 0;
            reply = wxConvertMB2WX(stream->reply);
            nntp_close (stream);
            break;

         // make gcc happy
         case Prot_Illegal:
         default:
            FAIL_MSG(_T("illegal protocol"));
            success = false;
      }

      if ( success )
      {
         if ( !(flags & Silent) )
         {
            MDialog_Message(m_Protocol == Prot_SMTP ? _("Message sent.")
                                                    : _("Article posted."),
                            m_frame, // parent window
                            MDIALOG_MSGTITLE,
                            _T("MailSentMessage"));
         }
      }
      else // failed to send/post
      {
         MLogCircle& log = MailFolder::GetLogCircle();
         if ( !reply.empty() )
         {
            log.Add(reply);
         }

         const String explanation = log.GuessError();

         // give the general error message anyhow
         String err = m_Protocol == Prot_SMTP ? _("Failed to send the message.")
                                              : _("Failed to post the article.");

         // and then try to give more details about what happened
         if ( !explanation.empty() )
         {
            err << _T("\n\n") << explanation;
         }
         else if ( !reply.empty() )
         {
            // no explanation, at least show the server error message
            err << _T("\n\n") << _("Server error: ") << reply;
         }

         ERRORMESSAGE((_T("%s"), err.c_str()));
      }
   }
   else // error in opening stream
   {
      String err;
      err.Printf(_("Cannot open connection to the %s server '%s'."),
               m_Protocol == Prot_SMTP ? "SMTP" : "NNTP",
                 m_ServerHost.c_str());

      String explanation = MailFolder::GetLogCircle().GuessError();
      if ( !explanation.empty() )
      {
         err << _T("\n\n") << explanation;
      }

      ERRORMESSAGE((_T("%s"), err.c_str()));

      success = false;
   }

   return success;
}

// ----------------------------------------------------------------------------
// SendMessageCC output routines
// ----------------------------------------------------------------------------

bool SendMessageCC::WriteMessage(soutr_t writer, void *where)
{
   Build();

   // this buffer is only for the message headers, so normally 16Kb should be
   // enough - but ideally we'd like to have some way to be sure it doesn't
   // overflow and I don't see any :-(
   char headers[16*1024];

   // install our output routine temporarily
   Rfc822OutputRedirector redirect(true /* with bcc */, this);

   return rfc822_output(headers, m_Envelope, m_Body, writer, where, NIL) != NIL;
}

bool
SendMessageCC::WriteToString(String& output)
{
   output.Empty();
   output.Alloc(4096); // FIXME: quick way go get message size?

   if ( !WriteMessage(write_str_output, &output) )
   {
      ERRORMESSAGE ((_T("Can't write message to string.")));

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
   ofstream ostr(filename.fn_str(),
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
   if ( !WriteToString(str) )
      return false;

   mf->AppendMessage(str);

   return true;
}

// ----------------------------------------------------------------------------
// output helpers
// ----------------------------------------------------------------------------

// rfc822_output() callback for writing into a file
static long write_stream_output(void *stream, char *string)
{
   ostream *o = (ostream *)stream;
   *o << string;
   if ( o->fail() )
      return NIL;

   return 1;
}

// rfc822_output() callback for writing to a string
static long write_str_output(void *stream, char *string)
{
   String *o = (String *)stream;
   *o << wxConvertMB2WX(string);
   return 1;
}

// ----------------------------------------------------------------------------
// Rfc822OutputRedirector
// ----------------------------------------------------------------------------

bool Rfc822OutputRedirector::ms_outputBcc = false;

void *Rfc822OutputRedirector::ms_oldRfc822Output = NULL;

const char **Rfc822OutputRedirector::ms_HeaderNames = NULL;
const char **Rfc822OutputRedirector::ms_HeaderValues = NULL;

MMutex Rfc822OutputRedirector::ms_mutexExtraHeaders;

Rfc822OutputRedirector::Rfc822OutputRedirector(bool outputBcc,
                                               SendMessageCC *msg)
{
   ms_mutexExtraHeaders.Lock();

   ms_outputBcc = outputBcc;
   ms_oldRfc822Output = mail_parameters(NULL, GET_RFC822OUTPUT, NULL);
   (void)mail_parameters(NULL, SET_RFC822OUTPUT, (void *)FullRfc822Output);

   SetHeaders(msg->m_headerNames, msg->m_headerValues);
}

void Rfc822OutputRedirector::SetHeaders(const char **names, const char **values)
{
   ms_HeaderNames = names;
   ms_HeaderValues = values;
}

Rfc822OutputRedirector::~Rfc822OutputRedirector()
{
   (void)mail_parameters(NULL, SET_RFC822OUTPUT, ms_oldRfc822Output);

   SetHeaders(NULL, NULL);

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
     wxFAIL_MSG(_T("cclient message header doesn't have terminating blank line?"));
  }

  // save the pointer as rfc822_address_line() modifies it
  char *headersOrig = headers;

  // cclient omits bcc, but we need it sometimes
  if ( ms_outputBcc )
  {
     rfc822_address_line(&headers, "Bcc", env, env->bcc);
  }

  // and add all other additional custom headers at the end
  if ( ms_HeaderNames )
  {
     for ( size_t n = 0; ms_HeaderNames[n]; n++ )
     {
        rfc822_header_line(&headers,
                           (char *)ms_HeaderNames[n],
                           env,
                           (char *)ms_HeaderValues[n]);
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

