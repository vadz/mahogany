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

#  include <wx/dynarray.h>

#  include "MApplication.h"
#  include "MDialogs.h"
#  include "gui/wxMFrame.h"
#endif // USE_PCH

#include <wx/fontmap.h>

#include "FolderView.h"

#include "Mdefaults.h"

#include "AddressCC.h"
#include "MailFolderCC.h"
#include "MessageCC.h"
#include "MimePartCC.h"

#include "SendMessage.h"

#include "HeaderInfo.h"

#include <ctype.h>

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// temporary buffer for storing message headers, be generous:
#define   HEADERBUFFERSIZE 100*1024

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
   CHECK(folder, NULL, "NULL m_folder");

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
         FAIL_MSG( "no profile in MessageCC ctor" );
      }
   }
   else
   {
      // we must have one in this ctor
      FAIL_MSG( "no folder in MessageCC ctor" );
   }
}

MessageCC::MessageCC(const char *text, UIdType uid, Profile *profile)
{
   Init();

   m_uid = uid;

   m_Profile = profile;
   if(m_Profile)
      m_Profile->IncRef();
   else
      m_Profile = Profile::CreateEmptyProfile();

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

   fs_give(&m_partContentPtr);

   delete [] m_msgText;

   SafeDecRef(m_folder);
   SafeDecRef(m_Profile);
}

// ----------------------------------------------------------------------------
// sending
// ----------------------------------------------------------------------------

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
   AddressList_obj addrListReplyTo = GetAddressList(MAT_REPLYTO);
   Address *addrReplyTo = addrListReplyTo ? addrListReplyTo->GetFirst() : NULL;

   AddressList_obj addrListFrom = GetAddressList(MAT_FROM);
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
         FAIL_MSG("unknown protocol");
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
      name = headerLine.BeforeFirst(':');
      value = headerLine.AfterFirst(':');
      if ( name != "Date" && 
           name != "From" && 
           name != "MIME-Version" &&
           name != "Content-Type" &&
           name != "Content-Disposition" &&
           name != "Content-Transfer-Encoding" )
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
      FAIL_MSG( "should have envelop in Subject()" );

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
      FAIL_MSG( "should have envelop in Date()" );

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
      FAIL_MSG( "no envelope in GetId" );
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
      FAIL_MSG( "no envelope in GetNewsgroups" );
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
      FAIL_MSG( "no envelope in GetReferences" );
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
      FAIL_MSG( "no envelope in GetReferences" );
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
         str = String(cptr, (size_t)len);
      }
      else
      {
         ERRORMESSAGE(("Cannot fetch message header"));
      }
   }
   else
   {
      FAIL_MSG( "MessageCC::GetHeader() can't be called for this message" );
   }

   return str;
}

wxArrayString
MessageCC::GetHeaderLines(const char **headersOrig,
                          wxArrayInt *encodings) const
{
   wxArrayString values;
   CHECK( m_folder, values,
          "GetHeaderLines() can't be called for this message" );

   CHECK_DEAD_RC(values);

   if ( !m_folder->Lock() )
      return values;

   // construct the string list containing all the headers we're interested in
   STRINGLIST *slist = mail_newstringlist();
   STRINGLIST *scur = slist;
   const char **headers = headersOrig;
   for ( ;; )
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

   // now extract the headers values
   if ( rc )
   {
      // first look at what we got: I don't assume here that the headers are
      // returned in the order requested - althouh this complicates the code a
      // bit assuming otherwise would be unsafe
      wxArrayString names;
      wxArrayString valuesInDisorder;

      String s;
      s.reserve(1024);

      // we are first looking for the name (before ':') and the value (after)
      bool inName = true;

      // note that we can stop when *pc == 0 as the header must be terminated
      // by "\r\n" preceding it anyhow
      for ( const char *pc = rc; *pc ; pc++ )
      {
         switch ( *pc )
         {
            case '\r':
               if ( pc[1] != '\n' )
               {
                  // this is not supposed to happen in RFC822 headers!
                  wxLogDebug("Bare '\\r' in header ignored");
                  continue;
               }

               // skip '\n' too
               pc++;

               if ( inName )
               {
                  if ( !s.empty() )
                  {
                     wxLogDebug("Header line '%s' ignored", s.c_str());
                  }
                  else
                  {
                     // blank line, header must end here
                     //
                     // update: apparently, sometimes it doesn't... it's non
                     // fatal anyhow, but report it as this is weird
                     if ( pc[1] != '\0' )
                     {
                        wxLogDebug("Blank line inside header?");
                     }
                  }
               }
               else // we have a valid header name in this line
               {
                  if ( s.empty() )
                  {
                     wxLogDebug("Empty header value?");
                  }

                  // this header may continue on the next line if it begins
                  // with a space or tab - check if it does
                  if ( pc[1] != ' ' && pc[1] != '\t' )
                  {
                     valuesInDisorder.Add(s);
                     inName = true;

                     s.clear();
                  }
                  else
                  {
                     // continue with the current s but add "\r\n" to the
                     // header value as it is part of it
                     s += "\r\n";
                  }
               }
               break;

            case ':':
               if ( inName )
               {
                  names.Add(s);
                  if ( *++pc != ' ' )
                  {
                     // oops... skip back
                     pc--;

                     // although this is allowed by the RFC 822 (but not
                     // 822bis), it is quite uncommon and so may indicate a
                     // problem - log it
                     wxLogDebug("Header without space after colon?");
                  }

                  s.clear();

                  inName = false;

                  break;
               }
               //else: fall through

            default:
               s += *pc;
         }
      }

      // and finally copy the headers in order into the dst array
      wxFontEncoding encoding;
      headers = headersOrig;
      while ( *headers )
      {
         int n = names.Index(*headers, FALSE /* not case sensitive */);
         if ( n != wxNOT_FOUND )
         {
            values.Add(MailFolderCC::DecodeHeader(valuesInDisorder[(size_t)n],
                                                  &encoding));
         }
         else // no such header
         {
            values.Add("");
            encoding = wxFONTENCODING_SYSTEM;
         }

         if ( encodings )
         {
            encodings->Add(encoding);
         }

         headers++;
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
   CHECK( m_Envelope, NULL, "no envelop in GetAddressStruct()" )

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
         FAIL_MSG( "unknown address type" );
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
MessageCC::FetchText(void)
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
            m_mailFullText = mail_fetchtext_full(m_folder->Stream(), m_uid,
                                                 &m_MailTextLen,
                                                 FT_UID | FT_PEEK);
            m_folder->UnLock();

            ASSERT_MSG(strlen(m_mailFullText) == m_MailTextLen,
                       "DEBUG: Mailfolder corruption detected");

            // there once has been an assert here checking that the message
            // length was positive, but it makes no sense as 0 length messages
            // do exist - so I removed it (VZ)
         }
         else
         {
            ERRORMESSAGE(("Cannot get lock for obtaining message text."));
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
          "ParseMIMEStructure() should only be called once for each message" );

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
   CHECK_RET( mimepart && body, "NULL pointer(s) in DecodeMIME" );

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
            wxLogDebug("Embedded message/rfc822 without body structure?");
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

   CHECK( n >= 0 && (size_t)n < m_numParts, NULL, "invalid part number" );

   MimePart *mimepart = m_mimePartTop;

   // skip the MULTIPART pseudo parts - this is consistent with DecodeMIME()
   // which doesn't count them in m_numParts neither
   while ( n || mimepart->GetType().GetPrimary() == MimeType::MULTIPART )
   {
      mimepart = FindPartInMIMETree(mimepart, n);

      if ( !mimepart )
      {
         FAIL_MSG( "logic error in MIME tree traversing code" );

         break;
      }
   }

   return mimepart;
}

const void *
MessageCC::GetPartData(const MimePart& mimepart, unsigned long *lenptr)
{
   CHECK( m_folder, NULL, "MessageCC::GetPartData() without folder?" );

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
   char *cptr = mail_fetchbody_full(stream,
                                    m_uid,
                                    (char *)sp.c_str(),
                                    &len, FT_UID);
   m_folder->EndReading();

   m_folder->UnLock();

   // ensure that lenptr is always valid
   if ( lenptr == NULL )
   {
      lenptr  = &len;
   }

   // have we succeeded in retrieveing anything?
   if ( len == 0 )
   {
      *lenptr = 0;

      return NULL;
   }

   // free old text
   fs_give(&m_partContentPtr);

   switch ( mimepart.GetTransferEncoding() )
   {
      case ENCBASE64:      // base-64 encoded data
         m_partContentPtr = rfc822_base64((unsigned char *)cptr, size, lenptr);
         break;

      case ENCQUOTEDPRINTABLE:   // human-readable 8-as-7 bit data
         m_partContentPtr = rfc822_qprint((unsigned char *)cptr, size, lenptr);
         break;

      case  ENCBINARY:     // 8 bit binary data
         // don't use string functions, string may contain NULs
         *lenptr = size;
         m_partContentPtr = fs_get(size);
         memcpy(m_partContentPtr, cptr, size);
         break;

      case ENC7BIT:        // 7 bit SMTP semantic data
      case ENC8BIT:        // 8 bit SMTP semantic data
      case ENCOTHER:       //unknown
      default:
         *lenptr = strlen(cptr);
         m_partContentPtr = cpystr(cptr);
   }

   return m_partContentPtr;
}

const void *MimePartCC::GetContent(unsigned long *len) const
{
   return GetMessage()->GetPartData(*this, len);
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

   ASSERT_MSG( m_Envelope, "failed to get message envelope!" );
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

   ASSERT_MSG( m_Body && m_Envelope, "failed to get body and envelope!" );
}

MESSAGECACHE *
MessageCC::GetCacheElement() const
{
   MESSAGECACHE *mc = NULL;

   if ( m_folder && m_folder->Lock() )
   {
      MAILSTREAM *stream = m_folder->Stream();

      unsigned long msgno = mail_msgno(stream, m_uid);
      if ( msgno )
      {
         mc = mail_elt(stream, msgno);
      }
      else
      {
         FAIL_MSG( "failed to get cache element for the message?" );
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
      FAIL_MSG( "no cache element in GetStatus?" );
   }

   return (MailFolder::MessageStatus) status;
}

/// return the size of the message
unsigned long
MessageCC::GetSize() const
{
   MESSAGECACHE *mc = GetCacheElement();

   CHECK( mc, 0, "no cache element in GetSize?" );

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
                       "DEBUG: Mailfolder corruption detected");

            str = String(header, (size_t)len);
         }
      }
      else // folder-less message doesn't have headers (why?)!
      {
         FAIL_MSG( "WriteToString can't be called for this message" );
      }
   }

   // and the text too
   str += ((MessageCC*)this)->FetchText();

   return true;
}

