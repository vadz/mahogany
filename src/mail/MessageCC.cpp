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

#include "miscutil.h"
#include "FolderView.h"

#include "Mdefaults.h"

#include "MailFolder.h"
#include "MailFolderCC.h"

#include "Message.h"
#include "MessageCC.h"

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
   if ( !m_folder->Stream() && !m_folder->PingReopen() )                      \
   {                                                                          \
      ERRORMESSAGE((_("Cannot access closed folder '%s'."),                   \
                   m_folder->GetName().c_str()));                             \
      return;                                                                 \
   }

/// check for dead mailstream (version with ret code)
#define CHECK_DEAD_RC(rc)                                                     \
   if ( !m_folder->Stream() && !m_folder->PingReopen() )                      \
   {                                                                          \
      ERRORMESSAGE((_("Cannot access closed folder '%s'."),                   \
                   m_folder->GetName().c_str()));                             \
      return rc;                                                              \
   }

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

typedef MessageCC::PartInfo MessagePartInfo;
WX_DEFINE_ARRAY(MessagePartInfo *, PartInfoArray);

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

/** A function to recursively collect information about all the body parts.

    Note that the caller is responsible for freeing partInfos array.

    @param  partInfos the array to store the part infos we find in
    @param  body the body part to look at
    @param  pfx  the MIME part prefix, our MIME part is "pfx.partNo+1"
    @param  partNo the MIME part number (counted from 0)
    @param  topLevel is true only when this is called for the entire message

*/
static void decode_body(PartInfoArray& partInfos,
                        BODY *body,
                        const String &pfx,
                        long partNo,
                        bool topLevel = false);

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
   m_mailFullText = NULL;
   m_Body = NULL;
   m_Envelope = NULL;
   m_partInfos = NULL;
   m_partContentPtr = NULL;
   m_msgText = NULL;

   m_folder = NULL;
   m_Profile = NULL;
   m_uid = UID_ILLEGAL;
}

MessageCC::MessageCC(MailFolderCC *folder, const HeaderInfo& hi)
         : m_subject(hi.GetSubject())
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
   if ( m_partInfos )
   {
      WX_CLEAR_ARRAY((*m_partInfos));

      delete m_partInfos;
   }

   delete [] m_partContentPtr;
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

   String name, reply;
   reply = Address( name, MAT_REPLYTO);

   switch(protocol)
   {
      case Prot_SMTP:
      {
         static const char *headers[] = { "To", "Cc", "Bcc", NULL };
         wxArrayString recipients = GetHeaderLines(headers);

         sendMsg->SetAddresses(recipients[0], recipients[1], recipients[2]);
         sendMsg->SetFrom(From(), name, reply);
      }
      break;

      case Prot_NNTP:
      {
         String newsgroups;
         GetHeaderLine("Newsgroups",newsgroups);
         sendMsg->SetNewsgroups(newsgroups);
         sendMsg->SetFrom(From(), name, reply);
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
      const char *data = GetPartContent(i, &len);
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
   String value;
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
      if(name != "Date" && name != "From")
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
   return m_subject;
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
   String name;
   String email = Address(name, MAT_FROM);
   return GetFullEmailAddress(name, email);
}

String
MessageCC::GetId(void) const
{
   CheckEnvelope();

   String id;

   if ( !m_Body )
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
                     ASSERT_MSG( pc[1] == '\0', "blank line inside header?" );
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

/* static */ void
MessageCC::AddressToNameAndEmail(ADDRESS *addr, wxString& name, wxString& email)
{
   CHECK_RET( addr, "ADDRESS can't be NULL here" );

   name.clear();
   email.clear();

   email = String(addr->mailbox);
   if(addr->host && strlen(addr->host)
      && (strcmp(addr->host,BADHOST) != 0))
      email += String("@") + String(addr->host);
   email = MailFolderCC::DecodeHeader(email);
   if(addr->personal && strlen(addr->personal))
   {
      name = String(addr->personal);
      name = MailFolderCC::DecodeHeader(name);
   }
   else
      name = "";
   if(strchr(name, ',') || strchr(name,'<') || strchr(name,'>'))
      name = String("\"") + name + String("\"");
}

size_t
MessageCC::GetAddresses(MessageAddressType type,
                        wxArrayString& addresses) const
{
   ADDRESS *addr = GetAddressStruct(type);

   while ( addr )
   {
      String name, email;
      AddressToNameAndEmail(addr, name, email);

      addresses.Add(GetFullEmailAddress(name, email));

      addr = addr->next;
   }

   return addresses.GetCount();
}

String
MessageCC::Address(String &nameAll, MessageAddressType type) const
{
   ADDRESS *addr = GetAddressStruct(type);

   // special case for Reply-To: we want to find a valid reply address
   if ( type == MAT_REPLYTO )
   {
      if(! addr)
         addr = GetAddressStruct(MAT_FROM);
      if(! addr)
         addr = GetAddressStruct(MAT_SENDER);
   }

   // concatenate all found addresses together
   String emailAll;
   while ( addr )
   {
      String name, email;
      AddressToNameAndEmail(addr, name, email);

      // concatenate emails together
      if ( !emailAll.empty() )
         emailAll += ", ";

      emailAll += email;

      // for now: use first name found (VZ: FIXME, this is just wrong)
      if ( nameAll.empty() )
        nameAll = name;
      else if ( !name.empty() ) // found another name
        nameAll << " ...";

      addr = addr->next;
   }

   return emailAll;
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

/*
  This function is based on display_body() from mtest example of UW imap
  distribution.
*/

static void decode_body(PartInfoArray& partInfos,
                        BODY *body,
                        const String &pfx,
                        long partNo,
                        bool topLevel)
{
   // in any case, the MIME part number increases (we do it before using it as
   // part 0 has MIME id of "1")
   partNo++;

   if ( body->type == TYPEMULTIPART )
   {
      // extend the prefix: the subparts of the first multipart MIME part
      // are 1.1, 1.2, ... , 1.N
      String prefix;
      if ( !topLevel )
      {
         // top level parts don't have prefix "1.", but all others do!
         prefix << pfx << strutil_ltoa(partNo) << '.';
      }

      // parse all parts recursively
      PART *part;
      long n;
      for ( n = 0, part = body->nested.part; part; part = part->next, n++ )
      {
         decode_body(partInfos, &part->body, prefix, n);
      }
   }
   else // not multipart
   {
      // construct the MIME part sequence for this part: they are dot
      // separated part numbers in each message (i.e. the 3rd part of an
      // embedded message which is itself the 2nd part of the main one should
      // have the MIME id of 2.3)
      String mimeId;
      mimeId << pfx << strutil_ltoa(partNo);

      String type = body_types[body->type];
      if ( body->subtype )
         type = type + '/' + body->subtype;

#ifdef DEBUG
      // the description of the part for debugging purposes
      String partDesc;
      partDesc << mimeId << ' ' << type;
#endif // DEBUG

      String desc;
      if ( body->description )
      {
         desc = body->description;

#ifdef DEBUG
         partDesc << '(' << desc << ')';
#endif // DEBUG
      }
      //else: leave empty

      MessageParameter *parameter;
      MessageParameterList plist(false); // temporary, doesn't own entries

      String parms;
      PARAMETER *par;
      if ((par = body->parameter) != NULL)
      {
#ifdef DEBUG
         // separate the parameters
         partDesc << "; ";
#endif // DEBUG

         do
         {
            parameter = new MessageParameter();
            parameter->name = par->attribute;
            parameter->value = par->value;
            plist.push_back(parameter);

            parms << par->attribute << '=' << par->value;
            if( par->next )
               parms << ':';
         }
         while ((par = par->next) != NULL);

#ifdef DEBUG
         partDesc << parms;
#endif // DEBUG
      }

      MessageParameterList dispPlist(false); // temporary, doesn't own entries
      if ((par = body->disposition.parameter) != NULL)
      {
#ifdef DEBUG
         partDesc << "; ";
#endif // DEBUG

         do
         {
            parameter = new MessageParameter();
            parameter->name = par->attribute;
            parameter->value = par->value;
            dispPlist.push_back(parameter);

#ifdef DEBUG
            partDesc << par->attribute << '=' << par->value;
            if( par->next )
               partDesc << ':';
#endif // DEBUG
         }
         while ((par = par->next) != NULL);
      }

      String id;
      if ( body->id )
      {
         id = body->id;

#ifdef DEBUG
         partDesc << " , id=" << id;
#endif // DEBUG
      }

#ifdef DEBUG
      // show bytes or lines depending upon body type
      partDesc << " (";
      switch ( body->type )
      {
         case TYPEMESSAGE:
         case TYPETEXT:
            partDesc << strutil_ltoa(body->size.lines) << " lines";
            break;

         default:
            partDesc << strutil_ltoa(body->size.bytes) << " bytes";
      }
      partDesc << ')';
#endif // DEBUG

      MessagePartInfo *pi = new MessagePartInfo;
      pi->mimeId = mimeId;
      pi->type = type;
      pi->numericalType  = body->type;
      pi->numericalEncoding = body->encoding;
      pi->params = parms;
      pi->description = desc;
      pi->id = id;
      pi->size_lines = body->size.lines;
      pi->size_bytes = body->size.bytes;

      MessageParameterList::iterator plist_iterator;
      for ( plist_iterator = plist.begin();
            plist_iterator != plist.end();
            plist_iterator++ )
      {
         pi->parameterList.push_back(*plist_iterator);
      }

      pi->dispositionType = body->disposition.type;
      for( plist_iterator = dispPlist.begin();
           plist_iterator != dispPlist.end();
           plist_iterator++ )
      {
         pi->dispositionParameterList.push_back(*plist_iterator);
      }

      partInfos.Add(pi);

#ifdef DEBUG
      wxLogTrace("msgparse", partDesc.c_str());
#endif // DEBUG

      // encapsulated message?
      if ((body->type == TYPEMESSAGE) && !strcmp (body->subtype,"RFC822") &&
          (body = body->nested.msg->body))
      {
         if ( body->type == TYPEMULTIPART )
         {
            // FIXME I don't understand why is there "- 1" here (VZ)
            decode_body(partInfos, body, pfx, partNo - 1);
         }
         else
         {
            // build encapsulation prefix
            String prefix;
            prefix << pfx << strutil_ltoa(partNo) << '.';
            decode_body(partInfos, body, prefix, 0l);
         }
      }
   }
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
      mc = mail_elt(stream, mail_msgno(stream, m_uid));

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

bool
MessageCC::ParseMIMEStructure()
{
   // this is the worker function, it is wasteful to call it more than once
   CHECK( !m_partInfos, false,
          "ParseMIMEStructure() should only be called once for each message" );

   m_partInfos = new PartInfoArray;
   decode_body(*m_partInfos, m_Body, "", 0l, true /* top level message */);

   return true;
}

void
MessageCC::DecodeMIME(void)
{
   if ( !m_partInfos )
   {
      CheckBody();

      (void)ParseMIMEStructure();
   }
}

int
MessageCC::CountParts(void)
{
   DecodeMIME();

   return m_partInfos->GetCount();
}


const char *
MessageCC::GetPartContent(int n, unsigned long *lenptr)
{
   DecodeMIME();

   if(! m_folder)
      return NULL;

   long  unsigned
      len = 0;
   String const
      & sp = GetPartSpec(n);

   if(! m_folder->Lock())
      return NULL;

   m_folder->StartReading(GetPartInfo(n)->size_bytes);
   char *cptr = mail_fetchbody_full(m_folder->Stream(),
                                    m_uid,
                                    (char *)sp.c_str(),
                                    &len, FT_UID);
   m_folder->EndReading();

   m_folder->UnLock();

   if(lenptr == NULL)
      lenptr  = &len;   // so to give c-client lib a place where to write to

   if(len == 0)
   {
      *lenptr = 0;
      return NULL;
   }

   if(m_partContentPtr)
      delete [] m_partContentPtr;
   m_partContentPtr = new char[len+1];
   strncpy(m_partContentPtr, cptr, len);
   m_partContentPtr[len] = '\0';
   //fs_give(&cptr); // c-client's free() function

   /* This bit is a bit suspicious, it fails sometimes in
      rfc822_qprint() when HTML code is fed into it and while Mahogany
      has long done all this, it seems to be not significantly worse
      if I comment it all out. So what I do now, is to just return the
      plain text if the decoding failed.
      FIXME I should really find out whether this is correct :-)
   */

   unsigned char *data = (unsigned char *)m_partContentPtr; // cast for cclient
   const char * returnVal = NULL;
   switch(GetPartTransferEncoding(n))
   {
   case ENCBASE64:      // base-64 encoded data
      returnVal = (const char *) rfc822_base64(data, GetPartSize(n), lenptr);
      break;
   case ENCQUOTEDPRINTABLE:   // human-readable 8-as-7 bit data
      returnVal = (const char *) rfc822_qprint(data, GetPartSize(n), lenptr);
      break;
   case  ENCBINARY:     // 8 bit binary data
      *lenptr = GetPartSize(n);
      return m_partContentPtr;

   case ENC7BIT:     // 7 bit SMTP semantic data
   case ENC8BIT:        // 8 bit SMTP semantic data
   case ENCOTHER:    //unknown
   default:
      *lenptr = strlen(m_partContentPtr);
      returnVal = m_partContentPtr;
   }
   if(returnVal == NULL)
      return m_partContentPtr;
   else if(returnVal != m_partContentPtr)
   { // we need to copy it over
      if(m_partContentPtr) delete [] m_partContentPtr;
      m_partContentPtr = new char [(*lenptr)+1];
      memcpy(m_partContentPtr, returnVal, *lenptr);
      m_partContentPtr[(*lenptr)] = '\0';
      fs_give((void **)&returnVal);
   }
   return m_partContentPtr;
}

const MessageCC::PartInfo *MessageCC::GetPartInfo(int n) const
{
   CHECK( m_partInfos, NULL, "no MIME part info available" );

   return m_partInfos->Item(n);
}

MessageParameterList const &
MessageCC::GetParameters(int n)
{
   DecodeMIME();

   return GetPartInfo(n)->parameterList;
}


MessageParameterList const &
MessageCC::GetDisposition(int n, String *disptype)
{
   DecodeMIME();

   if ( disptype )
      *disptype = GetPartInfo(n)->dispositionType;

   return GetPartInfo(n)->dispositionParameterList;
}

Message::ContentType
MessageCC::GetPartType(int n)
{
   DecodeMIME();

   // by a greatest of hazards, the numerical types used by cclient lib are
   // just the same as Message::ContentType enum values - of course, if it
   // were not the case, we would have to translate them somehow!
   return (Message::ContentType)GetPartInfo(n)->numericalType;
}

int
MessageCC::GetPartTransferEncoding(int n)
{
   DecodeMIME();

   return GetPartInfo(n)->numericalEncoding;
}

wxFontEncoding
MessageCC::GetTextPartEncoding(int n)
{
   String charset;
   if ( GetParameter(n, "charset", &charset) )
      return wxTheFontMapper->CharsetToEncoding(charset);
   else
      return wxFONTENCODING_SYSTEM;
}

size_t
MessageCC::GetPartSize(int n, bool useNaturalUnits)
{
   DecodeMIME();

   CHECK( m_Body, (size_t)-1, "should have the body in GetPartSize" );

   // return the size in lines for the text messages if requested, otherwise
   // return size in bytes
   if( useNaturalUnits &&
       (m_Body->type == TYPEMESSAGE || m_Body->type == TYPETEXT) )
      return GetPartInfo(n)->size_lines;
   else
      return GetPartInfo(n)->size_bytes;
}

String const &
MessageCC::GetPartMimeType(int n)
{
   DecodeMIME();
   return GetPartInfo(n)->type;
}

String const &
MessageCC::GetPartSpec(int n)
{
   DecodeMIME();
   return GetPartInfo(n)->mimeId;
}

String const &
MessageCC::GetPartDesc(int n)
{
   DecodeMIME();
   return GetPartInfo(n)->description;
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

