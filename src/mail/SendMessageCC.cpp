/*-*- c++ -*-********************************************************
 * SendMessageCC class: c-client implementation of a compose message*
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$ *
 *                                                                  *
 *******************************************************************/

/*
  This is just a quick note I put here to remind me how to do the
  application/remote-printing support for sending faxes via
  remote-printer.12345@iddd.tpc.int.

      puts $f "Content-Type: application/remote-printing"
    puts $f ""
    puts $f "Recipient:    $recipient(name)"
    puts $f "Title:        "
    puts $f "Organization: $recipient(organisation)"
    puts $f "Address:      "
    puts $f "Telephone:     "
    puts $f "Facsimile:    +$recipient(country) $recipient(local) $recipient(number)"
    puts $f "Email:        $recipient(email)"
    puts $f ""
    puts $f "Originator:   $user(name)"
    puts $f "Organization: $user(organisation)"
    puts $f "Telephone:    $user(tel)"
    puts $f "Facsimile:    $user(fax)"
    puts $f "Email:        $user(email)"

 */

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
#include "Mcommon.h"

#ifndef  USE_PCH
#   include "Profile.h"
#   include "strutil.h"
#   include "strings.h"
#   include "guidef.h"
#   include <strings.h>
#endif // USE_PCH

#include "miscutil.h"
#include "Mdefaults.h"
#include "MApplication.h"
#include "Message.h"
#include "MailFolderCC.h"

#include "MThread.h"

#include "SendMessage.h"
#include "SendMessageCC.h"

#include "XFace.h"
#include "MDialogs.h"
#include "gui/wxIconManager.h"

#include <wx/utils.h> // wxGetFullHostName()
#include <wx/file.h>

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

SendMessage *SendMessage::Create(Profile *prof, Protocol protocol)
{
   return new SendMessageCC(prof, protocol);
}

SendMessage::~SendMessage()
{
}

SendMessageCC::SendMessageCC(Profile *iprof,
                             Protocol protocol)
{
   Create(protocol, iprof);
}

// ----------------------------------------------------------------------------
// SendMessageCC creation and destruction
// ----------------------------------------------------------------------------

void
SendMessageCC::Create(Protocol protocol,
                      Profile *prof)
{
   String tmpstr;

   m_encHeaders = wxFONTENCODING_SYSTEM;

   m_headerNames = NULL;
   m_headerValues = NULL;
   m_Protocol = protocol;

   m_Envelope = mail_newenvelope();
   m_Body = mail_newbody();

   m_Body->type = TYPEMULTIPART;
   m_Body->nested.part = mail_newbody_part();
   m_Body->nested.part->next = NULL;
   m_NextPart = m_Body->nested.part;
   m_LastPart = m_NextPart;

   CHECK_RET(prof,"SendMessageCC::Create() requires profile");

   (void) miscutil_GetFromAddress(prof, &m_FromPersonal, &m_FromAddress);
   m_ReplyTo = miscutil_GetReplyAddress(prof);
   m_ReturnAddress = m_FromAddress;

   /*
      Sender logic: by default, use the SMTP login if it is set and differs
      from the "From" valu, otherwise leave it empty. If guessing it is
      disabled, then we use the sender value specified by user as is instead.
   */
   if ( READ_CONFIG(prof, MP_GUESS_SENDER) )
   {
      m_Sender = READ_CONFIG(prof, MP_SMTPHOST_LOGIN);
      m_Sender.Trim().Trim(FALSE); // remove all spaces on begin/end

      if ( Message::CompareAddresses(m_FromAddress, m_Sender) )
      {
         // leave Sender empty if it is the same as From, redundant
         m_Sender = "";
      }
   }
   else // don't guess, use provided value
   {
      m_Sender = READ_CONFIG(prof, MP_SENDER);
   }

   if(READ_CONFIG(prof,MP_COMPOSE_USE_XFACE) != 0)
      m_XFaceFile = prof->readEntry(MP_COMPOSE_XFACE_FILE,"");
   if(READ_CONFIG(prof, MP_USE_OUTBOX) != 0)
      m_OutboxName = READ_CONFIG(prof,MP_OUTBOX_NAME);
   if(READ_CONFIG(prof,MP_USEOUTGOINGFOLDER) )
      m_SentMailName = READ_CONFIG(prof,MP_OUTGOINGFOLDER);
   m_CharSet = READ_CONFIG(prof,MP_CHARSET);

   m_DefaultHost = miscutil_GetDefaultHost(prof);
   if ( !m_DefaultHost )
   {
      // trick c-client into accepting addresses without host names
      m_DefaultHost = '@';
   }

   m_SendmailCmd = READ_CONFIG(prof, MP_USE_SENDMAIL) ?
      READ_CONFIG(prof,MP_SENDMAILCMD) : String("");

   if(protocol == Prot_SMTP)
   {
      m_ServerHost = READ_CONFIG(prof, MP_SMTPHOST);
      m_UserName = READ_CONFIG(prof,MP_SMTPHOST_LOGIN);
      m_Password = strutil_decrypt(READ_CONFIG(prof,MP_SMTPHOST_PASSWORD));
   }
   else
   {
      m_ServerHost = READ_CONFIG(prof, MP_NNTPHOST);
      m_UserName = READ_CONFIG(prof,MP_NNTPHOST_LOGIN);
      m_Password = strutil_decrypt(READ_CONFIG(prof,MP_NNTPHOST_PASSWORD));
   }

#ifdef USE_SSL
   m_UseSSL = READ_CONFIG(prof, MP_SMTPHOST_USE_SSL) != 0;
#endif
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
}

// ----------------------------------------------------------------------------
// SendMessageCC header stuff
// ----------------------------------------------------------------------------

void
SendMessageCC::SetHeaderEncoding(wxFontEncoding enc)
{
   m_encHeaders = enc;
}

String
SendMessageCC::EncodeHeader(const String& header)
{
   // only encode the strings which contain the characters unallowed in RFC
   // 822 headers (FIXME should we quote RFC 822 specials? probably too...)
   const unsigned char *p;
   for ( p = (unsigned char *)header.c_str(); *p; p++ )
   {
      if ( *p < 32 || *p > 127 )
         break;
   }

   if ( !*p )
   {
      // string has only 7bit chars, don't encode
      return header;
   }

   // get the encoding in RFC 2047 sense
   enum
   {
      Encoding_Unknown,
      Encoding_Base64 = 'B',           // the values should be like this!
      Encoding_QuotedPrintable = 'Q'
   } enc2047;

   // QP should be used for the encodings which mostly overlap with US_ASCII,
   // Base64 for the others - choose the encoding method
   switch ( m_encHeaders )
   {
      case wxFONTENCODING_ISO8859_1:
      case wxFONTENCODING_ISO8859_2:
      case wxFONTENCODING_ISO8859_3:
      case wxFONTENCODING_ISO8859_4:
      case wxFONTENCODING_ISO8859_10:
      case wxFONTENCODING_ISO8859_12:
      case wxFONTENCODING_ISO8859_13:
      case wxFONTENCODING_ISO8859_14:
      case wxFONTENCODING_ISO8859_15:

      case wxFONTENCODING_CP1250:
      case wxFONTENCODING_CP1252:
      case wxFONTENCODING_CP1257:
         enc2047 = Encoding_QuotedPrintable;
         break;

      case wxFONTENCODING_ISO8859_5:
      case wxFONTENCODING_ISO8859_6:
      case wxFONTENCODING_ISO8859_7:
      case wxFONTENCODING_ISO8859_8:
      case wxFONTENCODING_ISO8859_9:
      case wxFONTENCODING_ISO8859_11:

      case wxFONTENCODING_CP1251:
      case wxFONTENCODING_CP1253:
      case wxFONTENCODING_CP1254:
      case wxFONTENCODING_CP1255:
      case wxFONTENCODING_CP1256:

      case wxFONTENCODING_KOI8:
         enc2047 = Encoding_Base64;
         break;

      default:
         FAIL_MSG( "unknown encoding" );

      case wxFONTENCODING_SYSTEM:
         enc2047 = Encoding_Unknown;
         break;
   }

   String headerEnc;
   if ( enc2047 == Encoding_Unknown )
   {
      // nothing to do
      headerEnc = header;
   }
   else
   {
      unsigned long len; // length of the encoded text
      unsigned char *text = (unsigned char *)header.c_str(); // cast for cclient
      unsigned char *textEnc;
      if ( enc2047 == Encoding_QuotedPrintable )
      {
            textEnc = rfc822_8bit(text, header.length(), &len);
      }
      else // Encoding_Base64
      {
            textEnc = rfc822_binary(text, header.length(), &len);
            if ( textEnc[len - 2] == '\r' && textEnc[len - 1] == '\n' )
            {
               // discard eol
               len -= 2;
            }
      }

      // FIXME should we check that the length is not greater than 76 here or
      //       does cclient take care of it?

      // create a RFC 2047 encoded word
      headerEnc << "=?" << EncodingToCharset(m_encHeaders)
                << '?' << (char)enc2047 << '?'
                << String(textEnc, (size_t)len)
                << "?=";

      fs_give((void **)&textEnc);
   }

   return headerEnc;
}

// unlike EncodeHeader(), we should only encode the personal name part of the
// address headers
String
SendMessageCC::EncodeAddress(const String& addr)
{
   if ( m_encHeaders == wxFONTENCODING_SYSTEM )
      return addr;

   String name = Message::GetNameFromAddress(addr),
          email = Message::GetEMailFromAddress(addr);

   return EncodeHeader(name) + " <" + email + '>';
}

void
SendMessageCC::EncodeAddressList(struct mail_address *adr)
{
   while ( adr )
   {
      if ( adr->personal )
      {
         char *tmp = adr->personal;
         adr->personal = cpystr(EncodeHeader(tmp));

         fs_give((void **)&tmp);
      }

      adr = adr->next;
   }
}

void
SendMessageCC::SetSubject(const String &subject)
{
   if(m_Envelope->subject) fs_give((void **)&m_Envelope->subject);

   String subj = EncodeHeader(subject);
   m_Envelope->subject = cpystr(subj.c_str());
}

void
SendMessageCC::SetFrom(const String & from,
                       const String & ipersonal,
                       const String & replyaddress,
                       const String & sender)
{
   if(from.Length())
      m_FromAddress = EncodeAddress(from);
   if(ipersonal.Length())
      m_FromPersonal = ipersonal;
   if(replyaddress.Length())
      m_ReplyTo = EncodeAddress(replyaddress);
   if(sender.Length())
      m_Sender = EncodeAddress(sender);
}

void
SendMessageCC::SetupAddresses(void)
{
   if(m_Envelope->from != NIL)
      mail_free_address(&m_Envelope->from);
   if(m_Envelope->return_path != NIL)
      mail_free_address(&m_Envelope->return_path);

   // From: line:
   m_Envelope->from = mail_newaddr();
   m_Envelope->return_path = mail_newaddr ();

   String mailbox, mailhost;

   String email = Message::GetEMailFromAddress(m_FromAddress);
   mailbox = strutil_before(email, '@');
   mailhost = strutil_after(email,'@');

   m_Envelope->from->personal = cpystr(EncodeHeader(m_FromPersonal));
   m_Envelope->from->mailbox = cpystr(mailbox);
   m_Envelope->from->host = cpystr(mailhost);

   if(m_Sender.Length() > 0)
   {
      m_Envelope->sender = mail_newaddr();
      email = Message::GetEMailFromAddress(m_Sender);
      mailbox = strutil_before(email, '@');
      mailhost = strutil_after(email,'@');

      if ( mailhost.empty() )
      {
         // SMTP servers often won't accept unqualified username as sender, so
         // always use some domain name
         mailhost = m_ServerHost;
      }

      String tmp = Message::GetNameFromAddress(m_Sender);
      if(tmp != email) // it might just be the same name@host
         m_Envelope->sender->personal = cpystr(tmp);
      m_Envelope->sender->mailbox = cpystr(mailbox);
      m_Envelope->sender->host = cpystr(mailhost);
   }
   m_Envelope->return_path->mailbox = cpystr(mailbox);
   m_Envelope->return_path->host = cpystr(mailhost);
}

void
SendMessageCC::SetAddresses(const String &to,
                            const String &cc,
                            const String &bcc)
{
   // TODO should call EncodeHeader() on the name parts of the addresses!

   String
      tmpstr;
   char
      * tmp,
      * tmp2;

   // If Build() has already been called, then it's too late to change
   // anything.
   ASSERT(m_headerNames == NULL);
   ASSERT(m_Protocol == Prot_SMTP || m_Protocol == Prot_Sendmail);

   if(to.Length())
   {
      ASSERT(m_Envelope->to == NIL);
      tmpstr = to;
      ExtractFccFolders(tmpstr);
      tmp = strutil_strdup(tmpstr);
      tmp2 = m_DefaultHost.Length() ? strutil_strdup(m_DefaultHost) : NIL;
      rfc822_parse_adrlist (&m_Envelope->to,tmp,tmp2);
      delete [] tmp;
      delete [] tmp2;
   }
   if(cc.Length())
   {
      ASSERT(m_Envelope->cc == NIL);
      tmpstr = cc;
      ExtractFccFolders(tmpstr);
      tmp = strutil_strdup(tmpstr);
      tmp2 = m_DefaultHost.Length() ? strutil_strdup(m_DefaultHost) : NIL;
      rfc822_parse_adrlist (&m_Envelope->cc,tmp,tmp2);
      delete [] tmp;
      delete [] tmp2;
   }
   if(bcc.Length())
   {
      m_Bcc = bcc; // in case we send later, we need this
      ASSERT(m_Envelope->bcc == NIL);
      tmpstr = bcc;
      ExtractFccFolders(tmpstr);
      tmp = strutil_strdup(tmpstr);
      tmp2 = m_DefaultHost.Length() ? strutil_strdup(m_DefaultHost) : NIL;
      rfc822_parse_adrlist (&m_Envelope->bcc,tmp,tmp2);
      delete [] tmp;
      delete [] tmp2;
   }

   EncodeAddressList(m_Envelope->to);
   EncodeAddressList(m_Envelope->cc);
   EncodeAddressList(m_Envelope->bcc);
}

void
SendMessageCC::SetNewsgroups(const String &groups)
{
   // If Build() has already been called, then it's too late to change
   // anything.
   ASSERT(m_headerNames == NULL);
   ASSERT(m_Protocol == Prot_NNTP);

   if(groups.Length())
   {
      ASSERT(m_Envelope->newsgroups == NIL);
      m_Envelope->newsgroups = strdup(groups);
   }

}

void
SendMessageCC::ExtractFccFolders(String &addresses)
{
   kbStringList addrList;
   kbStringList::iterator i;

   char *buf = strutil_strdup(addresses);
   // addresses are comma separated:
   strutil_tokenise(buf, ",", addrList);
   delete [] buf;
   addresses = ""; // clear the address list
   for(i = addrList.begin(); i != addrList.end(); i++)
   {
      strutil_delwhitespace(**i);
      if( *((*i)->c_str()) == '#')  // folder aliases are #foldername
         m_FccList.push_back(new String((*i)->c_str()+1)); // only the foldername
      else
      {
         if(! strutil_isempty(addresses))
            addresses += ", ";
         addresses += **i;
      }
   }
}

bool
SendMessageCC::HasHeaderEntry(const String &entry)
{
   kbStringList::iterator i;
   for(i = m_ExtraHeaderLinesNames.begin();
       i != m_ExtraHeaderLinesNames.end();
       i++)
      if(**i == entry)
         return true;
   return false;
}

String
SendMessageCC::GetHeaderEntry(const String &key)
{
   kbStringList::iterator i;
   kbStringList::iterator j;
   for(
      i = m_ExtraHeaderLinesNames.begin(),j = m_ExtraHeaderLinesValues.begin();
       i != m_ExtraHeaderLinesNames.end();
       i++, j++)
      if(**i == key)
         return **j ;
   return "";
}

/** Adds an extra header line.
    @param entry name of header entry
    @param value value of header entry
    */
void
SendMessageCC::AddHeaderEntry(String const &entry, String const
                              &ivalue)
{
   String
      *name = new String(),
      *value = new String();
   *name = entry; *value = ivalue;
   strutil_delwhitespace(*name);
   strutil_delwhitespace(*value);
   if(entry == "To")
      ; //TODO: Fix this?SetAddresses(*value);
   else if(entry == "CC")
      ; //SetAddresses("",*value);
   else if(entry == "BCC")
      ; //SetAddresses("","",*value);
   else if(entry == "MIME-Version"
           || entry == "Content-Type"
           || entry == "Content-Disposition"
           || entry == "Content-Transfer-Encoding"
      )
      ;  // ignore
   else if(entry == "Subject")
      SetSubject(*value);
   else
   {
      kbStringList::iterator i, j;
      for(i = m_ExtraHeaderLinesNames.begin(),
             j = m_ExtraHeaderLinesValues.begin();
          i != m_ExtraHeaderLinesNames.end();
          i++, j++)
      {
         if(**i == entry)
         {
            // change existing list entry
            **j = *value;
            return;
         }
      }
      // new entry
      m_ExtraHeaderLinesNames.push_back(name);
      m_ExtraHeaderLinesValues.push_back(value);
   }
}

String
SendMessageCC::EncodingToCharset(wxFontEncoding enc)
{
   // translate encoding to the charset
   wxString cs;
   switch ( enc )
   {
      case wxFONTENCODING_ISO8859_1:
      case wxFONTENCODING_ISO8859_2:
      case wxFONTENCODING_ISO8859_3:
      case wxFONTENCODING_ISO8859_4:
      case wxFONTENCODING_ISO8859_5:
      case wxFONTENCODING_ISO8859_6:
      case wxFONTENCODING_ISO8859_7:
      case wxFONTENCODING_ISO8859_8:
      case wxFONTENCODING_ISO8859_9:
      case wxFONTENCODING_ISO8859_10:
      case wxFONTENCODING_ISO8859_11:
      case wxFONTENCODING_ISO8859_12:
      case wxFONTENCODING_ISO8859_13:
      case wxFONTENCODING_ISO8859_14:
      case wxFONTENCODING_ISO8859_15:
         cs.Printf("ISO-8859-%d", enc + 1 - wxFONTENCODING_ISO8859_1);
         break;

      case wxFONTENCODING_CP1250:
      case wxFONTENCODING_CP1251:
      case wxFONTENCODING_CP1252:
      case wxFONTENCODING_CP1253:
      case wxFONTENCODING_CP1254:
      case wxFONTENCODING_CP1255:
      case wxFONTENCODING_CP1256:
      case wxFONTENCODING_CP1257:
         cs.Printf("windows-%d", 1250 + enc - wxFONTENCODING_CP1250);
         break;

      case wxFONTENCODING_KOI8:
         cs = "koi8-r";
         break;

      default:
         FAIL_MSG( "unknown encoding" );

      case wxFONTENCODING_SYSTEM:
      case wxFONTENCODING_DEFAULT:
         // no special encoding
         break;
   }

   return cs;
}

// ----------------------------------------------------------------------------
// SendMessageCC building
// ----------------------------------------------------------------------------

void
SendMessageCC::Build(bool forStorage)
{
   int
      h = 0;
   char
      tmpbuf[MAILTMPLEN];
   String tmpstr;
   kbStringList::iterator
      i, i2;

   if(m_headerNames != NULL) // message was already build
      return;

   SetupAddresses();

   bool replyToSet = false;

   /* Is the message supposed to be sent later? In that case, we need
      to store the BCC header as an X-BCC or it will disappear when
      the message is saved to the outbox. */
   if(forStorage)
   {
      // The X-BCC will be converted back to BCC by Send()
      if(m_Envelope->bcc)
         AddHeaderEntry("X-BCC", m_Bcc);
   }
   else
   /* If sending directly, we need to do the opposite: this message
      might have come from the Outbox queue, so we translate X-BCC
      back to a proper bcc setting: */
   {
      if(HasHeaderEntry("X-BCC"))
      {
         if(m_Envelope->bcc)
            mail_free_address(&(m_Envelope->bcc));
         String tmpstr = GetHeaderEntry("X-BCC");
         ExtractFccFolders(tmpstr);
         char *tmp = strutil_strdup(tmpstr);
         char *tmp2 = m_DefaultHost.Length() ? strutil_strdup(m_DefaultHost) : NIL;
         rfc822_parse_adrlist (&m_Envelope->bcc,tmp,tmp2);
         delete [] tmp;
         delete [] tmp2;
         EncodeAddressList(m_Envelope->bcc);
      }
   }

   // +4: 1 for X-Mailer, 1 for X-Face, 1 for reply to and 1 for the
   // last NULL entry
   size_t n = m_headerList.size() + m_ExtraHeaderLinesNames.size() + 4;
   m_headerNames = new const char*[n];
   m_headerValues = new const char*[n];

   /* Add directly added additional header lines: */
   i = m_ExtraHeaderLinesNames.begin();
   i2 = m_ExtraHeaderLinesValues.begin();
   for(; i != m_ExtraHeaderLinesNames.end(); i++, i2++, h++)
   {
      m_headerNames[h] = strutil_strdup(**i);
      if(strcmp(m_headerNames[h], "Reply-To") == 0)
         replyToSet = true;
      m_headerValues[h] = strutil_strdup(**i2);
   }

   //always add mailer header:
   if(! HasHeaderEntry("X-Mailer"))
   {
      m_headerNames[h] = strutil_strdup("X-Mailer");
      String version;
      version << "Mahogany, " << M_VERSION_STRING;
#ifdef OS_UNIX
      version  << _(", compiled for ") << M_OSINFO;
#else // Windows
      version << _(", running under ") << wxGetOsDescription();
#endif // Unix/Windows
      m_headerValues[h++] = strutil_strdup(version);
   }
   if(! replyToSet)
   {
      if(! HasHeaderEntry("Reply-To"))
      {
         //(always add reply-to header) add only if not empty:
         if(m_ReplyTo.Length() > 0)
         {
            tmpstr = m_ReplyTo;
            if(!strutil_isempty(tmpstr))
            {
               m_headerNames[h] = strutil_strdup("Reply-To");
               m_headerValues[h++] = strutil_strdup(tmpstr);
            }
         }
      }
   }
#ifdef HAVE_XFACES
   // add an XFace?
   if(! HasHeaderEntry("X-Face") && m_XFaceFile.Length() > 0)
   {
      XFace xface;
      if ( xface.CreateFromFile(m_XFaceFile) )
      {
         m_headerNames[h] = strutil_strdup("X-Face");
         m_headerValues[h] = strutil_strdup(xface.GetHeaderLine());
         if(strlen(m_headerValues[h]))  // paranoid, I know.
            ((char*) (m_headerValues[h]))[strlen(m_headerValues[h])-1] =
               '\0'; // cut off \n
         h++;
      }
      //else: couldn't read X-Face from file (complain?)
   }
#endif

   m_headerNames[h] = NULL;
   m_headerValues[h] = NULL;

   mail_free_body_part(&m_LastPart->next);
   m_LastPart->next = NULL;

   /* Check if there is only one part, then we don't need
      multipart/mixed: */
   if(m_LastPart == m_Body->nested.part)
   {
      BODY *oldbody = m_Body;
      m_Body = &(m_LastPart->body);
      oldbody->nested.part = NULL;
      mail_free_body(&oldbody);
   }

   rfc822_date (tmpbuf);
   m_Envelope->date = (char *) fs_get (1+strlen (tmpbuf));
   strcpy (m_Envelope->date,tmpbuf);
}

void
SendMessageCC::AddPart(Message::ContentType type,
                       const char *buf, size_t len,
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

   bdy = &(m_NextPart->body);
   bdy->type = type;

   bdy->subtype = (char *) fs_get( subtype.length()+1);
   strcpy(bdy->subtype,(char *)subtype.c_str());

   bdy->contents.text.data = data;
   bdy->contents.text.size = len;

   // set the transfer encoding
   switch ( type )
   {
      case TYPEMESSAGE:
         // FIXME: leads to empty body?
         bdy->nested.msg = mail_newmsg();

         // fall through

      case TYPETEXT:
         // FIXME: why do we use ENC7BIT for TYPEMESSAGE?
         bdy->encoding = (type == TYPETEXT) && !strutil_is7bit(buf)
                           ? ENC8BIT  // auto converted to QP by cclient
                           : ENC7BIT; // either ASCII text or something else
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

   // do we already have CHARSER parameter?
   bool hasCharset = false;

   if( plist )
   {
      MessageParameterList::iterator i;
      for( i = plist->begin(); i != plist->end(); i++ )
      {
         par = mail_newbody_parameter();

         String name = (*i)->name;
         if ( name.Lower() == "charset" )
         {
            if ( hasCharset )
            {
               // although not fatal, this shouldn't happen
               wxLogDebug("Multiple CHARSET parameters!");
            }

            hasCharset = true;
         }

         par->attribute = strdup(name);
         par->value     = strdup((*i)->value);
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
         par->value     = strdup(cs);
         par->next      = lastpar;
         lastpar = par;
      }
   }

   bdy->parameter = lastpar;
   bdy->disposition.type = strdup(disposition);
   if ( dlist )
   {
      PARAMETER *lastpar = NULL,
                *par;

      MessageParameterList::iterator i;
      for ( i = dlist->begin(); i != dlist->end(); i++ )
      {
         par = mail_newbody_parameter();
         par->attribute = strdup((*i)->name);
         par->value     = strdup((*i)->value);
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
SendMessageCC::SendOrQueue(bool send)
{
   bool success;

   // send directly?
   bool send_directly = TRUE;
   if ( !send && m_OutboxName.Length() )
      send_directly = FALSE;
   else if(! mApplication->IsOnline())
   {
/*
  This cannot work at present as we have no Outbox setting that we
  could use. We need to make a difference between Outbox and the
  "send_directly" configuration settings.

      MDialog_Message(
         _("No network connection available at present.\n"
           "Message will be queued in outbox."),
         NULL, MDIALOG_MSGTITLE,"MailNoNetQueuedMessage");
*/
      MDialog_Message(
         _("No network connection available at present.\n"
           "Message cannot be sent."),
         NULL, MDIALOG_MSGTITLE);
      return FALSE;
   }

   kbStringList::iterator i;
   for(i = m_FccList.begin(); i != m_FccList.end(); i++)
      WriteToFolder(**i);

   if( send_directly )
   {
      Build();
      success = Send();
   }
   else // store in outbox
   {
      if( m_OutboxName.Length() == 0)
         success = FALSE;
      else
      {
         Build(TRUE); // build for sending later
         WriteToFolder(m_OutboxName);
         // increment counter in statusbar immediately
         mApplication->UpdateOutboxStatus();
         success = TRUE;
      }

      if(success)
      {
         wxString msg;
         if(m_Protocol == Prot_SMTP || m_Protocol == Prot_Sendmail)
            msg.Printf(_("Message queued in ´%s´."),
                       m_OutboxName.c_str());
         else
            msg.Printf(_("Article queued in '%s'."),
                    m_OutboxName.c_str());
         STATUSMESSAGE((msg));
      }
   }
   // make copy to "Sent" folder?
   if ( success && m_SentMailName.Length() )
   {
      WriteToFolder(m_SentMailName);
   }
   mApplication->UpdateOutboxStatus();
   return success;
}

bool
SendMessageCC::Send(void)
{
   ASSERT_MSG(m_headerNames != NULL, "Build() must have been called!");

   MCclientLocker locker();

   SENDSTREAM *stream = NIL;

   // SMTP/NNTP server reply
   String reply;

   String service,
          server = m_ServerHost;

   if(m_UserName.Length() > 0) // activate authentication
   {
      server << "/user=\"" << m_UserName << '"';
      MailFolderCC::SetLoginData(m_UserName, m_Password);
   }

   const char *hostlist[2];
   hostlist[0] = server;
   hostlist[1] = NIL;

#ifndef OS_UNIX
   // For non-unix systems we make sure that no-one tries to run
   // Sendmail which is unix specific. This could happen if someone
   // imports a configuration from a remote server or something like
   // this, so we play it safe and map all sendmail calls to SMTP
   // instead:
   if(m_Protocol == Prot_Sendmail)
      m_Protocol = Prot_SMTP;
#endif

   // preview message being sent if asked for it
   String msgText;
   bool confirmSend;
   if ( READ_APPCONFIG(MP_PREVIEW_SEND) )
   {
      WriteToString(msgText);
      MDialog_ShowText(NULL, "Outgoing message text", msgText);

      // if we preview it, we want to confirm it too
      confirmSend = true;
   }
   else
   {
      confirmSend = READ_APPCONFIG(MP_CONFIRM_SEND) != 0;
   }

   if ( confirmSend )
   {
      if ( !MDialog_YesNoDialog(_("Send this message?")) )
      {
         return false;
      }
   }

   bool success = true;
   switch ( m_Protocol )
   {
      case Prot_SMTP:
         service = "smtp";
         DBGMESSAGE(("Trying to open connection to SMTP server '%s'",
                     m_ServerHost.c_str()));
#ifdef USE_SSL
         if(m_UseSSL)
         {
            STATUSMESSAGE(("Sending message via SSL..."));
            service << "/ssl";
         }
#endif // USE_SSL
         stream = smtp_open_full(NIL,
                                 (char **)hostlist,
                                 (char *)service.c_str(),
                                 SMTPTCPPORT,
                                 OP_DEBUG);
         break;

      case Prot_NNTP:
         service = "nntp";
         DBGMESSAGE(("Trying to open connection to NNTP server '%s'",
                    m_ServerHost.c_str()));
#ifdef USE_SSL
         if( m_UseSSL )
         {
            STATUSMESSAGE(("Posting message via SSL..."));
            service << "/ssl";
         }
#endif // USE_SSL

         stream = nntp_open_full(NIL,
                                 (char **)hostlist,
                                 (char *)service.c_str(),
                                 NNTPTCPPORT,
                                 OP_DEBUG);
         break;

#ifdef OS_UNIX
         case Prot_Sendmail:
         {
            if ( msgText.empty() )
            {
               WriteToString(msgText);
            }
            //else: already done for preview above

            // write to temp file:
#if 0 // VZ: wxGetTempFileName() is broken beyond repair, don't use it for now
            const char *filename = wxGetTempFileName("Mtemp");
#else
            // tmpnam() is POSIX, so use it even if mk(s)temp() would be better
            // because here we have a race condition
            const char *filename = tmpnam(NULL);
#endif

            if ( filename )
            {
               wxFile out;

               // don't overwrite because someone could have created file with "bad"
               // (i.e. world readable) permissions in the meanwhile
               if ( out.Create(filename, FALSE /* don't overwrite */,
                               wxS_IRUSR | wxS_IWUSR) )
               {
                  size_t written = out.Write(msgText, msgText.Length());
                  out.Close();
                  if ( written == msgText.Length() )
                  {
                     String command;
                     command.Printf("%s < '%s'; exec /bin/rm -f '%s'",
                                    m_SendmailCmd.c_str(),
                                    filename, filename);
                     // HORRIBLE HACK: this should be `const char *' but wxExecute's
                     // prototype doesn't allow it...
                     char *argv[4];
                     argv[0] = (char *)"/bin/sh";
                     argv[1] = (char *)"-c";
                     argv[2] = (char *)command.c_str();
                     argv[3] = 0;  // NULL
                     success = (wxExecute(argv) != 0);
                  }
               }
            }
            if(success)
               MDialog_Message(_("Message sent."),
                               NULL, // parent window
                               MDIALOG_MSGTITLE,
                               "MailSentMessage");
            else
               ERRORMESSAGE((_("Failed to send message via '%s'"),
                             m_SendmailCmd.c_str()));
            return success;
         }
#endif // OS_UNIX

         // make gcc happy
         case Prot_Illegal:
         default:
            FAIL_MSG("illegal protocol");
   }

   if ( stream )
   {
      Rfc822OutputRedirector redirect(false /* no bcc */, this);

      switch(m_Protocol)
      {
         case Prot_SMTP:
            success = smtp_mail (stream,"MAIL",m_Envelope,m_Body) != 0;
            reply = stream->reply;
            smtp_close (stream);
            break;

         case Prot_NNTP:
            success = nntp_mail (stream,m_Envelope,m_Body) != 0;
            reply = stream->reply;
            nntp_close (stream);
            break;

         // make gcc happy
         case Prot_Illegal:
         default:
            FAIL_MSG("illegal protocol");
      }

      if ( success )
      {
         MDialog_Message(m_Protocol == Prot_SMTP ? _("Message sent.")
                                                 :_("Article posted."),
                         NULL, // parent window
                         MDIALOG_MSGTITLE,
                         "MailSentMessage");
      }
      else // failed to send/post
      {
         String tmpbuf = MailFolder::GetLogCircle().GuessError();
         if(tmpbuf[0])
            ERRORMESSAGE((tmpbuf));
         tmpbuf.Printf(_("Failed to send - %s\n"),
                       (reply.Length() > 0) ? reply.c_str()
                                            :_("unknown error"));
         ERRORMESSAGE((tmpbuf));
      }
   }
   else // error in opening stream
   {
      String tmpbuf = MailFolder::GetLogCircle().GuessError();
      if(tmpbuf[0])
         ERRORMESSAGE((tmpbuf));
      ERRORMESSAGE((_("Cannot open connection to any server.\n")));
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

   return rfc822_output(headers, m_Envelope, m_Body, writer, where, NIL) == T;
}

void
SendMessageCC::WriteToString(String& output)
{
   output.Empty();
   output.Alloc(4096); // FIXME: quick way go get message size?

   if ( !WriteMessage(write_str_output, &output) )
   {
      ERRORMESSAGE (("Can't write message to string."));
   }
}

/** Writes the message to a file
    @param filename file where to write to
    */
void
SendMessageCC::WriteToFile(const String &filename, bool append)
{
   ofstream *ostr = new ofstream(filename.c_str(),
                                 ios::out | (append ? 0 : ios::trunc));
   bool ok = !(!ostr || ostr->bad());
   if ( ok )
      ok = WriteMessage(write_stream_output, ostr);

   if ( !ok )
   {
      ERRORMESSAGE((_("Failed to write message to file '%s'."),
                    filename.c_str()));
   }
}

void
SendMessageCC::WriteToFolder(String const &name)
{
   MailFolder *mf = MailFolder::OpenFolder(name);
   if ( !mf )
   {
      ERRORMESSAGE((_("Can't open folder '%s' to save the message to."),
                    name.c_str()));
      return;
   }

   String str;
   WriteToString(str);

   // we don't want this to create new mail events
   int updateFlags = mf->GetUpdateFlags();
   mf->SetUpdateFlags( MailFolder::UF_UpdateCount);
   mf->AppendMessage(str);
   mf->SetUpdateFlags(updateFlags);
   mf->DecRef();
}

// ----------------------------------------------------------------------------
// output helpers
// ----------------------------------------------------------------------------

// rfc822_output() callback for writing into a file
static long write_stream_output(void *stream, char *string)
{
   ostream *o = (ostream *)stream;
   *o << string;
   if( o->fail() )
      return NIL;
   else
      return T;
}

// rfc822_output() callback for writing to a string
static long write_str_output(void *stream, char *string)
{
   String *o = (String *)stream;
   *o << string;
   return T;
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
     wxFAIL_MSG("cclient message header doesn't have terminating blank line?");
  }

  // save the pointer as rfc822_address_line() modifies it
  char *headersOrig = headers;

  // cclient omits bcc, but we need it sometimes
  if ( ms_outputBcc )
  {
     rfc822_address_line(&headers, "bcc", env, env->bcc);
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

  return T;
}

