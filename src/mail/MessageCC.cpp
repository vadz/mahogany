///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MessageCC.cpp - implements MessageCC class
// Purpose:     implementation of a message structure using cclient API
// Author:      Karsten Ballüder (Ballueder@gmx.net)
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 M Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "MessageCC.h"
#endif

#include   "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "Profile.h"

#  include "MApplication.h"
#endif // USE_PCH

#include "AddressCC.h"
#include "MailFolderCC.h"
#include "MessageCC.h"
#include "MimePartCC.h"

#include "SendMessage.h"

#include "HeaderInfo.h"

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

/// check for dead mailstream
#define CHECK_DEAD()                                                          \
   if ( !m_folder->IsOpened() )                                               \
   {                                                                          \
      ERRORMESSAGE((_("Cannot access closed folder '%s'."),                   \
                   m_folder->GetName().c_str()));                             \
      return;                                                                 \
   }

/// check for dead mailstream (version with ret code)
#define CHECK_DEAD_RC(rc)                                                     \
   if ( !m_folder->IsOpened() )                                               \
   {                                                                          \
      ERRORMESSAGE((_("Cannot access closed folder '%s'."),                   \
                   m_folder->GetName().c_str()));                             \
      return rc;                                                              \
   }

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MessageCC creation
// ----------------------------------------------------------------------------

/* static */
Message *
Message::Create(const char *text, UIdType uid, Profile *profile)
{
   return MessageCC::Create(text, uid, profile);
}

MessageCC *
MessageCC::Create(MailFolderCC *folder, const HeaderInfo& hi)
{
   CHECK(folder, NULL, _T("NULL m_folder"));

   return new MessageCC(folder, hi);
}

void MessageCC::Init()
{
   m_mimePartTop = NULL;
   m_mailFullText = NULL;
   m_Body = NULL;
   m_Envelope = NULL;
   m_partContentPtr = NULL;
   m_msgText = NULL;

   m_ownsPartContent = false;

   m_folder = NULL;
   m_Profile = NULL;
   m_uid = UID_ILLEGAL;
}

MessageCC::MessageCC(MailFolderCC *folder, const HeaderInfo& hi)
{
   Init();

   m_uid = hi.GetUId();
   m_date = hi.GetDate();
   m_folder = folder;

   if ( m_folder )
   {
      m_folder->IncRef();
      m_Profile = folder->GetProfile();
      if ( m_Profile )
      {
         m_Profile->IncRef();
      }
      else
      {
         FAIL_MSG( _T("no profile in MessageCC ctor") );
      }
   }
   else
   {
      // we must have one in this ctor
      FAIL_MSG( _T("no folder in MessageCC ctor") );
   }
}

MessageCC::MessageCC(const char *text, UIdType uid, Profile *profile)
{
   Init();

   m_uid = uid;

   m_Profile = profile;
   if(!m_Profile)
      m_Profile = mApplication->GetProfile();

   m_Profile->IncRef();

   // move \n --> \r\n convention
   m_msgText = strutil_strdup(strutil_enforceCRLF(text));

   // find end of header "\012\012" (FIXME: not "\015\012"???)
   char
      *header = NULL,
      *bodycptr = NULL;
   unsigned long
      headerLen = 0;

   for ( unsigned long pos = 0; m_msgText[pos]; pos++ )
   {
      if((m_msgText[pos] == '\012' && m_msgText[pos+1] == '\012') // empty line
         // is end of header
         || m_msgText[pos+1] == '\0')
      {
         header = new char [pos+2];
         strncpy(header, m_msgText, pos+1);
         header[pos+1] = '\0';
         headerLen = pos+1;
         bodycptr = m_msgText + pos + 2;
         break;
      }
      pos++;
   }

   if(! header)
      return;  // failed

   STRING str;
   INIT(&str, mail_string, (void *) bodycptr, strlen(bodycptr));
   rfc822_parse_msg (&m_Envelope, &m_Body, header, headerLen,
                     &str, ""   /*defaulthostname */, 0);
   delete [] header;
}

MessageCC::~MessageCC()
{
   delete m_mimePartTop;

   if ( m_ownsPartContent )
      fs_give(&m_partContentPtr);

   delete [] m_msgText;

   SafeDecRef(m_folder);
   SafeDecRef(m_Profile);
}

// ----------------------------------------------------------------------------
// sending
// ----------------------------------------------------------------------------

// TODO: this should be moved into SendMessage probably and cleaned up
bool
MessageCC::SendOrQueue(Protocol iprotocol, bool send)
{
   Protocol protocol = Prot_SMTP; // default

   if(iprotocol == Prot_Illegal)
   {
      // autodetect protocol:
      String tmp;
      if ( GetHeaderLine("Newsgroups", tmp) )
         protocol = Prot_NNTP;
   }

   SendMessage *sendMsg = SendMessage::Create(m_Profile, protocol);

   sendMsg->SetSubject(Subject());

   // VZ: I'm not sure at all about what exactly we're trying to do here so
   //     this is almost surely wrong - but the old code was even more wrong
   //     than the current one! (FIXME)
   AddressList_obj addrListReplyTo(GetAddressList(MAT_REPLYTO));
   Address *addrReplyTo = addrListReplyTo ? addrListReplyTo->GetFirst() : NULL;

   AddressList_obj addrListFrom(GetAddressList(MAT_FROM));
   Address *addrFrom = addrListFrom ? addrListFrom->GetFirst() : NULL;
   if ( !addrFrom )
      addrFrom = addrReplyTo;

   if ( !addrFrom )
   {
      // maybe send it nevertheless?
      wxLogError(_("Can't send the message without neither \"From\" nor "
                   "\"Reply-To\" address."));
      return false;
   }

   String replyto;
   if ( addrReplyTo )
      replyto = addrReplyTo->GetAddress();

   sendMsg->SetFrom(addrFrom->GetAddress(), replyto);

   switch ( protocol )
   {
      case Prot_SMTP:
         {
            static const char *headers[] = { "To", "Cc", "Bcc", NULL };
            wxArrayString recipients = GetHeaderLines(headers);

            sendMsg->SetAddresses(recipients[0], recipients[1], recipients[2]);
         }
         break;

      case Prot_NNTP:
         {
            String newsgroups;
            GetHeaderLine("Newsgroups",newsgroups);
            sendMsg->SetNewsgroups(newsgroups);
         }
         break;

      // make gcc happy
      case Prot_Illegal:
      default:
         FAIL_MSG(_T("unknown protocol"));
   }

   for(int i = 0; i < CountParts(); i++)
   {
      unsigned long len;
      const void *data = GetPartContent(i, &len);
      String dispType;
      MessageParameterList const &dlist = GetDisposition(i, &dispType);
      MessageParameterList const &plist = GetParameters(i);

      sendMsg->AddPart(GetPartType(i),
                       data, len,
                       strutil_after(GetPartMimeType(i),'/'), //subtype
                       dispType,
                       &dlist, &plist);
   }

   String header = GetHeader();
   String headerLine;
   const char *cptr = header;
   String name, value;
   do
   {
      while(*cptr && *cptr != '\r' && *cptr != '\n')
         headerLine << *cptr++;
      while(*cptr == '\r' || *cptr == '\n')
         cptr ++;
      if(*cptr == ' ' || *cptr == '\t') // continue
      {
         while(*cptr == ' ' || *cptr == '\t')
            cptr++;
         continue;
      }
      // end of this line
      name = headerLine.BeforeFirst(':').Lower();
      value = headerLine.AfterFirst(':');
      if ( name != "date" &&
           name != "from" &&
           name != "message-id" &&
           name != "mime-version" &&
           name != "content-type" &&
           name != "content-disposition" &&
           name != "content-transfer-encoding" )
         sendMsg->AddHeaderEntry(name, value);
      headerLine = "";
   }while(*cptr && *cptr != '\012');

   bool rc = sendMsg->SendOrQueue(send);

   delete sendMsg;

   return rc;
}

// ----------------------------------------------------------------------------
// MessageCC: envelop (i.e. fast) headers access
// ----------------------------------------------------------------------------

String MessageCC::Subject(void) const
{
   CheckEnvelope();

   String subject;
   if ( m_Envelope )
      subject = m_Envelope->subject;
   else
      FAIL_MSG( _T("should have envelop in Subject()") );

   return subject;
}

time_t
MessageCC::GetDate() const
{
   return m_date;
}

String MessageCC::Date(void) const
{
   CheckEnvelope();

   String date;
   if ( m_Envelope )
      date = m_Envelope->date;
   else
      FAIL_MSG( _T("should have envelop in Date()") );

   return date;
}

String MessageCC::From(void) const
{
   return GetAddressesString(MAT_FROM);
}

String
MessageCC::GetId(void) const
{
   CheckEnvelope();

   String id;

   if ( !m_Envelope )
   {
      FAIL_MSG( _T("no envelope in GetId") );
   }
   else
   {
      id = m_Envelope->message_id;
   }

   return id;
}

String
MessageCC::GetNewsgroups(void) const
{
   CheckEnvelope();

   String newsgroups;

   if ( !m_Envelope )
   {
      FAIL_MSG( _T("no envelope in GetNewsgroups") );
   }
   else
   {
      if ( !m_Envelope->ngbogus )
      {
         newsgroups = m_Envelope->newsgroups;
      }
      //else: what does ngbogus really mean??
   }

   return newsgroups;
}

String
MessageCC::GetReferences(void) const
{
   CheckEnvelope();

   String ref;

   if ( !m_Envelope )
   {
      FAIL_MSG( _T("no envelope in GetReferences") );
   }
   else
   {
      ref = m_Envelope->references;
   }

   return ref;
}

String
MessageCC::GetInReplyTo(void) const
{
   CheckEnvelope();

   String inreplyto;

   if ( !m_Envelope )
   {
      FAIL_MSG( _T("no envelope in GetReferences") );
   }
   else
   {
      inreplyto = m_Envelope->references;
   }

   return inreplyto;
}

// ----------------------------------------------------------------------------
// MessageCC: any header access (may require a trip to server)
// ----------------------------------------------------------------------------

String MessageCC::GetHeader(void) const
{
   String str;

   if ( m_folder )
   {
      CHECK_DEAD_RC(str);

      if ( m_folder->Lock() )
      {
         unsigned long len = 0;
         const char *cptr = mail_fetchheader_full(m_folder->Stream(), m_uid,
                                                  NULL, &len, FT_UID);
         m_folder->UnLock();
         str = String(wxConvertMB2WX(cptr), (size_t)len);
      }
      else
      {
         ERRORMESSAGE((_("Cannot fetch message header")));
      }
   }
   else
   {
      FAIL_MSG( _T("MessageCC::GetHeader() can't be called for this message") );
   }

   return str;
}

wxArrayString
MessageCC::GetHeaderLines(const char **headersOrig,
                          wxArrayInt *encodings) const
{
   // loop variable for iterating over headersOrig
   const char **headers;

   // we should always return the arrays of correct size, this makes the
   // calling code simpler as it doesn't have to check for the number of
   // elements but only their values
   wxArrayString values;
   for ( headers = headersOrig; *headers; headers++ )
   {
      values.Add("");
      if ( encodings )
      {
         encodings->Add(wxFONTENCODING_SYSTEM);
      }
   }


   CHECK( m_folder, values,
          _T("GetHeaderLines() can't be called for this message") );

   CHECK_DEAD_RC(values);

   if ( !m_folder->Lock() )
      return values;

   // construct the string list containing all the headers we're interested in
   STRINGLIST *slist = mail_newstringlist();
   STRINGLIST *scur = slist;
   for ( headers = headersOrig; ; )
   {
      scur->text.size = strlen(*headers);
      scur->text.data = (unsigned char *)cpystr(*headers);
      if ( !*++headers )
      {
         // terminating NULL
         break;
      }

      scur =
      scur->next = mail_newstringlist();
   }

   // go fetch it
   unsigned long len;
   char *rc = mail_fetchheader_full(m_folder->Stream(),
                                    m_uid,
                                    slist,
                                    &len,
                                    FT_UID);
   m_folder->UnLock();
   mail_free_stringlist(&slist);

   if ( rc )
   {
      // note that we can't assume here that the headers are returned in the
      // order requested!
      wxArrayString names,
                    valuesInDisorder;

      // extract the headers values
      HeaderIterator hdrIter(rc);
      hdrIter.GetAll(&names, &valuesInDisorder);

      // and then copy the headers in order into the dst array
      size_t nHdr = 0;
      wxFontEncoding encoding;
      String value;
      for ( headers = headersOrig; *headers; headers++ )
      {
         int n = names.Index(*headers, FALSE /* not case sensitive */);
         if ( n != wxNOT_FOUND )
         {
            value = MailFolderCC::DecodeHeader
                    (
                     valuesInDisorder[(size_t)n],
                     &encoding
                    );
         }
         else // no such header
         {
            value.clear();
            encoding = wxFONTENCODING_SYSTEM;
         }

         // store them
         values[nHdr] = value;
         if ( encodings )
         {
            (*encodings)[nHdr] = encoding;
         }

         nHdr++;
      }
   }
   else // mail_fetchheader_full() failed
   {
      wxLogError(_("Failed to retrieve headers of the message from '%s'."),
                 m_folder->GetName().c_str());
   }

   return values;
}

// ----------------------------------------------------------------------------
// address stuff
// ----------------------------------------------------------------------------

ADDRESS *
MessageCC::GetAddressStruct(MessageAddressType type) const
{
   CheckEnvelope();
   CHECK( m_Envelope, NULL, _T("no envelop in GetAddressStruct()") )

   ADDRESS *addr;

   switch ( type )
   {
      case MAT_FROM:       addr = m_Envelope->from; break;
      case MAT_TO:         addr = m_Envelope->to; break;
      case MAT_CC:         addr = m_Envelope->cc; break;
      case MAT_BCC:        addr = m_Envelope->bcc; break;
      case MAT_SENDER:     addr = m_Envelope->sender; break;
      case MAT_REPLYTO:    addr = m_Envelope->reply_to; break;
      case MAT_RETURNPATH: addr = m_Envelope->return_path; break;
      default:
         FAIL_MSG( _T("unknown address type") );
         addr = NULL;
   }

   return addr;
}

size_t
MessageCC::GetAddresses(MessageAddressType type,
                        wxArrayString& addresses) const
{
   ADDRESS *adr = GetAddressStruct(type);

   while ( adr )
   {
      AddressCC addrCC(adr);
      addresses.Add(addrCC.GetAddress());

      adr = adr->next;
   }

   return addresses.GetCount();
}

AddressList *MessageCC::GetAddressList(MessageAddressType type) const
{
   ADDRESS *adr = GetAddressStruct(type);
   if ( type == MAT_REPLYTO )
   {
      // try hard to find some address to which we can reply
      if ( !adr )
      {
         adr = GetAddressStruct(MAT_FROM);
         if ( !adr )
            adr = GetAddressStruct(MAT_SENDER);
      }
   }

   return AddressListCC::Create(adr);
}

// ----------------------------------------------------------------------------
// message text
// ----------------------------------------------------------------------------

String
MessageCC::FetchText(void) const
{
   if ( m_folder )
   {
      if ( !m_mailFullText )
      {
         CHECK_DEAD_RC("");

         if ( m_folder->Lock() )
         {
            // FIXME: FT_PEEK normally shouldn't be there as this function is
            //        called from, for example, WriteToString() and in this
            //        case the message should be marked as read
            //
            //        but it is also called when moving a new message from non
            //        IMAP folder to "New Mail" in which case the message
            //        status should be unchanged
            //
            //        having FT_PEEK here for now is a lesser evil, in the
            //        future we really must have PeekText() and GetText()!
            MessageCC *self = (MessageCC *)this;
            self->m_mailFullText = mail_fetchtext_full
                                   (
                                    m_folder->Stream(),
                                    m_uid,
                                    &self->m_MailTextLen,
                                    FT_UID | FT_PEEK
                                   );

            m_folder->UnLock();

            // there once has been an assert here checking that the message
            // length was positive, but it makes no sense as 0 length messages
            // do exist - so I removed it
            //
            // there was also another assert for strlen(m_mailFullText) ==
            // m_MailTextLen but this one is simply wrong: the message text may
            // contain NUL bytes and we should not use C string functions such
            // as strlen() on it!
         }
         else
         {
            ERRORMESSAGE((_("Cannot get lock for obtaining message text.")));
         }
      }
      //else: already have it, reuse as msg text doesn't change

      return m_mailFullText;
   }
   else // from a text
   {
      return m_msgText;
   }
}

// ----------------------------------------------------------------------------
// MIME message structure
// ----------------------------------------------------------------------------

void
MessageCC::CheckMIME(void) const
{
   if ( !m_mimePartTop )
   {
      CheckBody();

      ((MessageCC *)this)->ParseMIMEStructure();
   }
}

bool
MessageCC::ParseMIMEStructure()
{
   // this is the worker function, it is wasteful to call it more than once
   CHECK( !m_mimePartTop, false,
          _T("ParseMIMEStructure() should only be called once for each message") );

   if ( !m_Body )
   {
      return false;
   }

   m_numParts = 0;
   m_mimePartTop = new MimePartCC(this);

   DecodeMIME(m_mimePartTop, m_Body);

   return true;
}

void MessageCC::DecodeMIME(MimePartCC *mimepart, BODY *body)
{
   CHECK_RET( mimepart && body, _T("NULL pointer(s) in DecodeMIME") );

   mimepart->m_body = body;

   if ( body->type != TYPEMULTIPART )
   {
      m_numParts++;

      // is it an encapsulated message?
      if ( body->type == TYPEMESSAGE && strcmp(body->subtype, "RFC822") == 0 )
      {
         body = body->nested.msg->body;
         if ( body )
         {
            mimepart->m_nested = new MimePartCC(mimepart);

            DecodeMIME(mimepart->m_nested, body);
         }
         else
         {
            // this is not fatal but not expected neither - I don't know if it
            // can ever happen, in fact
            wxLogDebug(_T("Embedded message/rfc822 without body structure?"));
         }
      }
   }
   else // multi part
   {
      // note that we don't increment m_numParts here as we only count parts
      // containing something and GetMimePart(n) ignores multitype parts

      MimePartCC **prev = &mimepart->m_nested;
      PART *part;

      // NB: message parts are counted from 1
      size_t n;
      for ( n = 1, part = body->nested.part; part; part = part->next, n++ )
      {
         *prev = new MimePartCC(mimepart, n);

         DecodeMIME(*prev, &part->body);

         prev = &((*prev)->m_next);
      }
   }
}

const MimePart *MessageCC::GetTopMimePart() const
{
   CheckMIME();

   return m_mimePartTop;
}

int
MessageCC::CountParts(void) const
{
   CheckMIME();

   return m_numParts;
}

/* static */
MimePart *MessageCC::FindPartInMIMETree(MimePart *mimepart, int& n)
{
   if ( mimepart->GetNested() )
   {
      // MULTIPARTs don't count for total part count but MESSAGE (which also
      // have nested parts) do
      if ( mimepart->GetType().GetPrimary() != MimeType::MULTIPART )
      {
         n--;
      }

      return FindPartInMIMETree(mimepart->GetNested(), n);
   }
   else // a simple part
   {
      // n may be already 0 if we're looking for the first part with contents
      // (i.e. not multipart one), in this case just return this one
      if ( !n )
      {
         return mimepart;
      }

      n--;

      MimePart *next = mimepart->GetNext();
      while ( !next )
      {
         mimepart = mimepart->GetParent();
         if ( !mimepart )
            break;

         next = mimepart->GetNext();
      }

      return next;
   }
}

const MimePart *MessageCC::GetMimePart(int n) const
{
   CheckMIME();

   CHECK( n >= 0 && (size_t)n < m_numParts, NULL, _T("invalid part number") );

   MimePart *mimepart = m_mimePartTop;

   // skip the MULTIPART pseudo parts - this is consistent with DecodeMIME()
   // which doesn't count them in m_numParts neither
   while ( n || mimepart->GetType().GetPrimary() == MimeType::MULTIPART )
   {
      mimepart = FindPartInMIMETree(mimepart, n);

      if ( !mimepart )
      {
         FAIL_MSG( _T("logic error in MIME tree traversing code") );

         break;
      }
   }

   return mimepart;
}

const char *
MessageCC::DoGetPartAny(const MimePart& mimepart,
                        unsigned long *lenptr,
                        char *(*fetchFunc)(MAILSTREAM *,
                                           unsigned long,
                                           char *,
                                           unsigned long *,
                                           long))
{
   CHECK( m_folder, NULL, _T("MessageCC::GetPartData() without folder?") );

   CheckMIME();

   MAILSTREAM *stream = m_folder->Stream();
   if ( !stream )
   {
      ERRORMESSAGE((_("Impossible to retrieve message text: "
                      "folder '%s' is closed."),
                    m_folder->GetName().c_str()));
      return NULL;
   }

   if ( !m_folder->Lock() )
   {
      ERRORMESSAGE((_("Impossible to retrieve message text: "
                      "failed to lock folder '%s'."),
                    m_folder->GetName().c_str()));
      return NULL;
   }

   unsigned long size = mimepart.GetSize();
   m_folder->StartReading(size);

   const String& sp = mimepart.GetPartSpec();
   unsigned long len = 0;

   // NB: this pointer shouldn't be freed
   char *cptr = (*fetchFunc)(stream, m_uid, (char *)sp.c_str(), &len, FT_UID);

   m_folder->EndReading();

   m_folder->UnLock();

   if ( lenptr )
      *lenptr = len;

   // have we succeeded in retrieveing anything?
   return len ? cptr : NULL;
}

const char *
MessageCC::GetRawPartData(const MimePart& mimepart, unsigned long *lenptr)
{
   return DoGetPartAny(mimepart, lenptr, mail_fetch_body);
}

String
MessageCC::GetPartHeaders(const MimePart& mimepart)
{
   String s;

   unsigned long len = 0;
   const char *cptr = DoGetPartAny(mimepart, &len, mail_fetch_mime);
   if ( cptr )
   {
      s.assign(wxConvertMB2WX(cptr), len);
   }

   return s;
}

const void *
MessageCC::GetPartData(const MimePart& mimepart, unsigned long *lenptr)
{
   CHECK( lenptr, NULL, _T("MessageCC::GetPartData(): NULL len ptr") );

   // first get the raw text
   // ----------------------

   const char *cptr = GetRawPartData(mimepart, lenptr);
   if ( !cptr || !*lenptr )
      return NULL;

   // now decode it
   // -------------

   // free old text
   if (m_ownsPartContent )
   {
      m_ownsPartContent = false;

      fs_give(&m_partContentPtr);
   }

   // total size of this message part
   unsigned long size = mimepart.GetSize();

   // just for convenience...
   unsigned char *text = (unsigned char *)cptr;

   switch ( mimepart.GetTransferEncoding() )
   {
      case ENCQUOTEDPRINTABLE:   // human-readable 8-as-7 bit data
         m_partContentPtr = rfc822_qprint(text, size, lenptr);

         // some broken mailers sent messages with QP specified as the content
         // transfer encoding in the headers but don't encode the message
         // properly - in this case show it as plain text which is better than
         // not showing it at all
         if ( m_partContentPtr )
         {
            m_ownsPartContent = true;

            break;
         }
         //else: treat it as plain text

         // it was overwritten by rfc822_qprint() above
         *lenptr = size;

         // fall through

      case ENC7BIT:        // 7 bit SMTP semantic data
      case ENC8BIT:        // 8 bit SMTP semantic data
      case ENCBINARY:      // 8 bit binary data
      case ENCOTHER:       // unknown
      default:
         // nothing to do
         m_partContentPtr = text;
         break;


      case ENCBASE64:      // base-64 encoded data
         // the size of possible extra non Base64 encoded text following a
         // Base64 encoded part
         const unsigned char *startSlack = NULL;
         size_t sizeSlack = 0;

         // there is a frequent problem with mail list software appending the
         // mailing list footer (i.e. a standard signature containing the
         // instructions about how to [un]subscribe) to the top level part of a
         // Base64-encoded message thus making it invalid - worse c-client code
         // doesn't complain about it but simply returns some garbage in this
         // case
         //
         // we try to detect this case and correct for it: note that neither
         // '-' nor '_' which typically start the signature are valid
         // characters in base64 so the logic below should work for all common
         // cases

         // only check the top level part
         if ( !mimepart.GetParent() )
         {
            const unsigned char *p;
            for ( p = text; *p; p++ )
            {
               // we do *not* want to use the locale-specific settings here,
               // hence don't use isalpha()
               const unsigned char ch = *p;
               if ( (ch >= 'A' && ch <= 'Z') ||
                     (ch >= 'a' && ch <= 'z') ||
                      (ch >= '0' && ch <= '9') ||
                       (ch == '+' || ch == '/' || ch == '\r' || ch == '\n') )
               {
                  // valid Base64 char
                  continue;
               }

               if ( ch == '=' )
               {
                  p++;

                  // valid, but can only occur at the end of data as padding,
                  // so still break below -- but not before:

                  // a) skipping a possible second '=' (can't be more than 2 of
                  // them)
                  if ( *p == '=' )
                     p++;

                  // b) skipping the terminating "\r\n"
                  if ( p[0] == '\r' && p[1] == '\n' )
                     p += 2;
               }

               // what (if anything) follows can't appear in a valid Base64
               // message
               break;
            }

            size_t sizeValid = p - text;
            if ( sizeValid != size )
            {
               ASSERT_MSG( sizeValid < size,
                           _T("logic error in base64 validity check") );

               // take all the rest verbatim below
               startSlack = p;
               sizeSlack = size - sizeValid;

               // and decode just the (at least potentially) valid part
               size = sizeValid;
            }
         }

         m_partContentPtr = rfc822_base64(text, size, lenptr);
         if ( !m_partContentPtr )
         {
            FAIL_MSG( _T("rfc822_base64() failed") );

            m_partContentPtr = text;
         }
         else // ok
         {
            // append the non Base64-encoded chunk, if any, to the end of decoded
            // data
            if ( sizeSlack )
            {
               fs_resize(&m_partContentPtr, *lenptr + sizeSlack);
               memcpy((char *)m_partContentPtr + *lenptr, startSlack, sizeSlack);

               *lenptr += sizeSlack;
            }

            m_ownsPartContent = true;
         }
   }

   return m_partContentPtr;
}

const void *MimePartCC::GetRawContent(unsigned long *len) const
{
   return GetMessage()->GetRawPartData(*this, len);
}

const void *MimePartCC::GetContent(unsigned long *len) const
{
   return GetMessage()->GetPartData(*this, len);
}

String MimePartCC::GetHeaders() const
{
   return GetMessage()->GetPartHeaders(*this);
}

String MimePartCC::GetTextContent() const
{
   unsigned long len;
   const char *p = reinterpret_cast<const char *>(GetContent(&len));
   if ( !p )
      return wxGetEmptyString();

#if wxUSE_UNICODE
   #error "We need the original encoding here, TODO"
#else // ANSI
   return wxString(p, len);
#endif // Unicode/ANSI
}

// ----------------------------------------------------------------------------
// get the body and/or envelope information from cclient
// ----------------------------------------------------------------------------

void
MessageCC::GetEnvelope()
{
   if ( !m_folder )
      return;

   // Forget what we know and re-fetch the body, it is cached anyway.
   m_Envelope = NULL;

   // reopen the folder if needed
   CHECK_DEAD();

   if ( !m_folder->Lock() )
   {
      ERRORMESSAGE((_("Impossible to retrieve message headers: "
                      "failed to lock folder '%s'."),
                    m_folder->GetName().c_str()));
   }

   m_Envelope = mail_fetch_structure(m_folder->Stream(),
                                     m_uid,
                                     NULL, // without body
                                     FT_UID);
   m_folder->UnLock();

   ASSERT_MSG( m_Envelope, _T("failed to get message envelope!") );
}

void
MessageCC::GetBody(void)
{
   if ( !m_folder )
      return;

   // Forget what we know and re-fetch the body, it is cached anyway.
   m_Body = NULL;

   // reopen the folder if needed
   CHECK_DEAD();

   if ( !m_folder->Lock() )
   {
      ERRORMESSAGE((_("Impossible to retrieve message body: "
                      "failed to lock folder '%s'."),
                    m_folder->GetName().c_str()));
   }

   m_Envelope = mail_fetchstructure_full(m_folder->Stream(),
                                         m_uid,
                                         &m_Body,
                                         FT_UID);
   m_folder->UnLock();

   ASSERT_MSG( m_Body && m_Envelope, _T("failed to get body and envelope!") );
}

MESSAGECACHE *
MessageCC::GetCacheElement() const
{
   MESSAGECACHE *mc = NULL;

   if ( m_folder && m_folder->Lock() )
   {
      MAILSTREAM *stream = m_folder->Stream();

      if ( stream )
      {
         unsigned long msgno = mail_msgno(stream, m_uid);
         if ( msgno )
         {
            mc = mail_elt(stream, msgno);
         }
         else
         {
            FAIL_MSG( _T("failed to get cache element for the message?") );
         }
      }
      else
      {
         FAIL_MSG( _T("folder is closed in GetCacheElement") );
      }

      m_folder->UnLock();
   }

   return mc;
}

/** Return the numeric status of message. */
int
MessageCC::GetStatus() const
{
   MESSAGECACHE *mc = GetCacheElement();

   int status = 0;
   if ( mc )
   {
      if(mc->seen)
         status |= MailFolder::MSG_STAT_SEEN;
      if(mc->answered)
         status |= MailFolder::MSG_STAT_ANSWERED;
      if(mc->deleted)
         status |= MailFolder::MSG_STAT_DELETED;
      if(mc->searched)
         status |= MailFolder::MSG_STAT_SEARCHED;
      if(mc->recent)
         status |= MailFolder::MSG_STAT_RECENT;
      if(mc->flagged)
         status |= MailFolder::MSG_STAT_FLAGGED;
   }
   else
   {
      FAIL_MSG( _T("no cache element in GetStatus?") );
   }

   return (MailFolder::MessageStatus) status;
}

/// return the size of the message
unsigned long
MessageCC::GetSize() const
{
   MESSAGECACHE *mc = GetCacheElement();

   CHECK( mc, 0, _T("no cache element in GetSize?") );

   return mc->rfc822_size;
}

// ----------------------------------------------------------------------------
// MessageCC::WriteToString
// ----------------------------------------------------------------------------

bool
MessageCC::WriteToString(String &str, bool headerFlag) const
{
   str.clear();

   // get the headers if asked for them
   if ( headerFlag )
   {
      if ( m_folder )
      {
         ((MessageCC *)this)->CheckBody(); // const_cast<>

         CHECK_DEAD_RC(false);

         if ( m_folder->Lock())
         {
            unsigned long len;
            char *header = mail_fetchheader_full(m_folder->Stream(),
                                                 m_uid, NIL,
                                                 &len, FT_UID);
            m_folder->UnLock();

            ASSERT_MSG(strlen(header) == len,
                       _T("DEBUG: Mailfolder corruption detected"));

            str = String(header, (size_t)len);
         }
      }
      else // folder-less message doesn't have headers (why?)!
      {
         FAIL_MSG( _T("WriteToString can't be called for this message") );
      }
   }

   // and the text too
   str += ((MessageCC*)this)->FetchText();

   return true;
}

