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
   m_msgText = strutil_strdup(wxConvertWX2MB(strutil_enforceCRLF(wxConvertMB2WX(text))));

   if ( !CclientParseMessage(m_msgText, &m_Envelope, &m_Body) )
   {
      wxLogError(_("Failed to create a mail message."));
   }
}

MessageCC::~MessageCC()
{
   delete m_mimePartTop;

   if ( m_folder )
   {
      m_folder->DecRef();
   }
   else
   {
      delete [] m_msgText;

      mail_free_envelope(&m_Envelope);
      mail_free_body(&m_Body);
   }

   SafeDecRef(m_Profile);
}

// ----------------------------------------------------------------------------
// MessageCC: envelop (i.e. fast) headers access
// ----------------------------------------------------------------------------

String MessageCC::Subject(void) const
{
   CheckEnvelope();

   String subject;
   if ( m_Envelope )
      subject = wxConvertMB2WX(m_Envelope->subject);
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
      date = wxConvertMB2WX(m_Envelope->date);
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
      id = wxConvertMB2WX(m_Envelope->message_id);
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
      if ( !m_Envelope->newsgroups )
      {
         newsgroups = wxConvertMB2WX(m_Envelope->newsgroups);
      }
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
      ref = wxConvertMB2WX(m_Envelope->references);
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
      inreplyto = wxConvertMB2WX(m_Envelope->references);
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
MessageCC::GetHeaderLines(const wxChar **headersOrig,
                          wxArrayInt *encodings) const
{
   // loop variable for iterating over headersOrig
   const wxChar **headers;

   // we should always return the arrays of correct size, this makes the
   // calling code simpler as it doesn't have to check for the number of
   // elements but only their values
   wxArrayString values;
   for ( headers = headersOrig; *headers; headers++ )
   {
      values.Add(wxEmptyString);
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
      scur->text.size = strlen(wxConvertWX2MB(*headers));
      scur->text.data = (unsigned char *)cpystr(wxConvertWX2MB(*headers));
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
      HeaderIterator hdrIter(wxConvertMB2WX(rc));
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
   CHECK( m_Envelope, NULL, _T("no envelop in GetAddressStruct()") );

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
         CHECK_DEAD_RC(wxEmptyString);

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

      return wxConvertMB2WX(m_mailFullText);
   }
   else // from a text
   {
      return wxConvertMB2WX(m_msgText);
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

   m_mimePartTop = new MimePartCC(this, m_Body);

   return true;
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

   return m_mimePartTop->GetPartsCount();
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

   CHECK( n >= 0 && n < CountParts(), NULL, _T("invalid part number") );

   MimePart *mimepart = m_mimePartTop;

   // skip the MULTIPART pseudo parts -- this is consistent with how
   // MimePartCCBase numbers the MIME parts as it doesn't count them in
   // neither
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
   char *cptr = (*fetchFunc)(stream, m_uid, (char *)sp.mb_str(), &len, FT_UID);

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

MailFolder *MessageCC::GetFolder() const
{
   return m_folder;
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

            str = wxConvertMB2WX(header);
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

// ============================================================================
// functions from Mcclient.h
// ============================================================================

bool
CclientParseMessage(const char *msgText,
                    ENVELOPE **ppEnv,
                    BODY **ppBody,
                    size_t *pHdrLen)
{
   const char *bodycptr = NULL;
   unsigned long headerLen = 0;

   // find end of header "\r\n\r\n"
   bool eol = true;
   for ( unsigned long pos = 0; msgText[pos]; pos++ )
   {
      // empty line or end of text?
      if ( msgText[pos] == '\r' && msgText[++pos] == '\n' )
      {
         if ( eol )
         {
            headerLen = pos - 1;
            bodycptr = msgText + pos + 1; // skip eol
            break;
         }

         eol = true;
      }
      else
      {
         eol = false;
      }
   }

   if ( !headerLen )
      return false;

   STRING str;
   INIT(&str, mail_string, const_cast<char *>(bodycptr), strlen(bodycptr));
   rfc822_parse_msg(ppEnv, ppBody,
                    const_cast<char *>(msgText), headerLen,
                    &str, "", 0);

   if ( pHdrLen )
      *pHdrLen = headerLen;

   return true;
}

