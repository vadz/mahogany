/*-*- c++ -*-********************************************************
 * Message class: entries for message header                        *
 *                      implementation for MailFolderCC             *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "MessageCC.h"
#endif

#include   "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "strutil.h"

#   include "MApplication.h"
#   include "MDialogs.h"
#   include "gui/wxMFrame.h"
#endif // USE_PCH

#include <wx/fontmap.h>

#include "miscutil.h"
#include "FolderView.h"

#include "Mdefaults.h"

#include "MailFolder.h"
#include "MailFolderCC.h"

#include "Message.h"
#include "MessageCC.h"

#include "SendMessageCC.h"

#include <ctype.h>

/// temporary buffer for storing message headers, be generous:
#define   HEADERBUFFERSIZE 100*1024

/// check for dead mailstream
#define CHECK_DEAD()   if( m_folder && m_folder->Stream() == NIL && ! m_folder->PingReopen() ) { ERRORMESSAGE((_("Cannot access inactive m_folder '%s'."), m_folder->GetName().c_str())); return; }
#define CHECK_DEAD_RC(rc)   if( m_folder && m_folder->Stream() == NIL && ! m_folder->PingReopen()) {   ERRORMESSAGE((_("Cannot access inactive m_folder '%s'."), m_folder->GetName().c_str())); return rc; }

MessageCC *
MessageCC::CreateMessageCC(MailFolderCC *ifolder,
                           unsigned long iuid)
{
   CHECK(ifolder, NULL, "NULL m_folder");
   return new MessageCC(ifolder, iuid);
}

MessageCC::MessageCC(MailFolderCC *ifolder,unsigned long iuid)
{
   m_mailFullText = NULL;
   m_Body = NULL;
   m_Envelope = NULL;
   m_partInfos = NULL; // this vector gets initialised when needed
   m_numOfParts = -1;
   partContentPtr = NULL;
   m_msgText = NULL;
   m_folder = ifolder;
   m_folder->IncRef();
   m_uid = iuid;
   m_Profile=ifolder->GetProfile();
   m_Profile->IncRef();
   Refresh();
}


MessageCC::~MessageCC()
{
   delete [] m_partInfos;
   delete [] partContentPtr;
   delete [] m_msgText;
   SafeDecRef(m_folder);
   SafeDecRef(m_Profile);
}

MessageCC::MessageCC(const char * itext, UIdType uid, Profile *iprofile)
{
   char
      *header = NULL,
      *bodycptr = NULL;
   unsigned long
      headerLen = 0;

   m_Body = NULL;
   m_mailFullText = NULL;
   bodycptr = NULL;
   m_Envelope = NULL;
   m_Profile = iprofile;
   if(m_Profile)
      m_Profile->IncRef();
   else
      m_Profile = Profile::CreateEmptyProfile();
   m_partInfos = NULL; // this vector gets initialised when needed
   m_numOfParts = -1;
   partContentPtr = NULL;
   m_folder = NULL;
   m_uid = uid;

   // move \n --> \r\n convention
   m_msgText = strutil_strdup(strutil_enforceCRLF(itext));

   unsigned long pos = 0;
   // find end of header "\012\012"
   while(m_msgText[pos])
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

   SendMessageCC sendMsg(m_Profile, protocol);

   sendMsg.SetSubject(Subject());

   String name, reply;
   reply = Address( name, MAT_REPLYTO);

   switch(protocol)
   {
      case Prot_SMTP:
      {
         String to, cc, bcc;
         GetHeaderLine("To", to);
         GetHeaderLine("CC", cc);
         GetHeaderLine("BCC",bcc);
         sendMsg.SetAddresses(to, cc, bcc);
         sendMsg.SetFrom(From(), name, reply);
      }
      break;

      case Prot_NNTP:
      {
         String newsgroups;
         GetHeaderLine("Newsgroups",newsgroups);
         sendMsg.SetNewsgroups(newsgroups);
         sendMsg.SetFrom(From(), name, reply);
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

      sendMsg.AddPart(GetPartType(i),
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
         sendMsg.AddHeaderEntry(name, value);
      headerLine = "";
   }while(*cptr && *cptr != '\012');

   if(send)
   {
      sendMsg.Build();
      return sendMsg.Send();
   }
   else
      return sendMsg.SendOrQueue();
}

void
MessageCC::Refresh(void)
{
   if(GetBody())
   {
      if ( m_Envelope->date )
         m_headerDate = m_Envelope->date;
      else
         m_headerDate.clear();

      if ( m_Envelope->subject )
         m_headerSubject = MailFolderCC::DecodeHeader(m_Envelope->subject);
      else
         m_headerSubject.clear();
   }
}

String const &
MessageCC::Subject(void) const
{
   return m_headerSubject;
}

String const
MessageCC::From(void) const
{
   String name;
   String email = Address(name, MAT_FROM);
   return GetFullEmailAddress(name, email);
}

String
MessageCC::GetHeader(void) const
{
   String str;

   CHECK_DEAD_RC(str);

   if( m_folder && m_folder->Lock() )
   {
      unsigned long len = 0;
      const char *cptr = mail_fetchheader_full(m_folder->Stream(), m_uid,
                                               NULL, &len, FT_UID);
      m_folder->UnLock();
      str = String(cptr, (size_t)len);
      MailFolderCC::ProcessEventQueue();
   }

   return str;
}

bool
MessageCC::GetHeaderLine(const String &line,
                         String &value, wxFontEncoding *encoding) const
{
   CHECK_DEAD_RC(false);
   if(! m_folder)
      return false;

   STRINGLIST  slist;
   slist.next = NULL;
   slist.text.size = line.length();
   slist.text.data = (unsigned char *)strutil_strdup(line);

   if(!m_folder->Lock())
      return false;

   unsigned long len;
   char *
      rc = mail_fetchheader_full (m_folder->Stream(),
                                  m_uid,
                                  &slist,
                                  &len,FT_UID);
   m_folder->UnLock();
   value = String(rc, (size_t)len);
   char *val = strutil_strdup(value);
   // trim off trailing newlines/crs
   if(strlen(val))
   {
      // start by the last char and move backwards until we don't throw off all
      // line termination chars - be careful to not underflow the (almost)
      // empty string!
      char *cptr  = val + strlen(val) - 1;
      while( cptr >= val && (*cptr == '\n' || *cptr == '\r') )
         cptr--;
      cptr++;
      *cptr = '\0';
   }
   value = strutil_after(val,':');
   delete [] val;
   strutil_delwhitespace(value);
   value = MailFolderCC::DecodeHeader(value, encoding);
   delete [] slist.text.data;
   MailFolderCC::ProcessEventQueue();

   return value.length() != 0;
}

// ----------------------------------------------------------------------------
// address stuff
// ----------------------------------------------------------------------------

ADDRESS *
MessageCC::GetAddressStruct(MessageAddressType type) const
{
   ((MessageCC *)this)->GetBody();
   CHECK(m_Envelope, NULL, _("Non-existent message data."))

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

const String
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

String const &
MessageCC::Date(void) const
{
   return m_headerDate;
}

String
MessageCC::FetchText(void)
{
   CHECK_DEAD_RC("");
   if(m_folder)
   {
      if(m_folder->Lock())
      {
         m_mailFullText = mail_fetchtext_full(m_folder->Stream(), m_uid,
                                        &m_MailTextLen, FT_UID);
         m_folder->UnLock();
         ASSERT_MSG(strlen(m_mailFullText) == m_MailTextLen,
                    "DEBUG: Mailfolder corruption detected");

         // there once has been an assert here checking that the message
         // length was positive, but it makes no sense as 0 length messages do
         // exist - so I removed it (VZ)

         MailFolderCC::ProcessEventQueue();
         return m_mailFullText;
      }
      else
      {
         ERRORMESSAGE(("Cannot get lock for obtaining message text."));
         return "";
      }
   }
   else // from a text
   {
      return m_msgText;
   }
}

/*
  This function is based on display_body() from mtest example.
*/

void
MessageCC::decode_body(BODY *body, const String & pfx, long i,
                       int * count, bool write)
{
   if ( body->type == TYPEMULTIPART )
   {
      // parse all parts recursively

      // if we do have prefix, extend it to include our part number
      String prefix = pfx;
      if( !prefix.empty() )
         prefix << strutil_ltoa(++i) << '.';

      PART *part;
      long n;
      for ( n = 0, part = body->nested.part; part; part = part->next, n++ )
      {
         decode_body(&part->body, prefix, n, count, write);
      }
   }
   else // not multipart
   {
      // construct the MIME part sequence for this part: they are dot
      // separated part numbers in each message (i.e. the 3rd part of an
      // embedded message which is itself the 2nd part of the main one should
      // have the MIME id of 2.3)
      String mimeId;
      mimeId << pfx << strutil_ltoa(++i);

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
            if(write)
            {
               parameter = new MessageParameter();
               parameter->name = par->attribute;
               parameter->value = par->value;
               plist.push_back(parameter);
            }
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
            if(write)
            {
               parameter = new MessageParameter();
               parameter->name = par->attribute;
               parameter->value = par->value;
               dispPlist.push_back(parameter);
            }

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

      if ( write )
      {
         PartInfo &pi = m_partInfos[*count];
         pi.mimeId = mimeId;
         pi.type = type;
         pi.numericalType  = body->type;
         pi.numericalEncoding = body->encoding;
         pi.params = parms;
         pi.description = desc;
         pi.id = id;
         pi.size_lines = body->size.lines;
         pi.size_bytes = body->size.bytes;

         MessageParameterList::iterator plist_iterator;
         for ( plist_iterator = plist.begin();
               plist_iterator != plist.end();
               plist_iterator++ )
            pi.parameterList.push_back(*plist_iterator);

         pi.dispositionType = body->disposition.type;
         for( plist_iterator = dispPlist.begin();
              plist_iterator != dispPlist.end();
              plist_iterator++ )
            pi.dispositionParameterList.push_back(*plist_iterator);
      }

      (*count)++;

#ifdef DEBUG
      // don't do it too many times, as we're currently always called twice in
      // a row (see the comment in GetBody()), only do it the second time
      if ( write )
      {
         wxLogTrace("msgparse", partDesc.c_str());
      }
#endif // DEBUG

      // encapsulated message?
      if ((body->type == TYPEMESSAGE) && !strcmp (body->subtype,"RFC822") &&
          (body = body->nested.msg->body))
      {
         if ( body->type == TYPEMULTIPART )
         {
            // FIXME I don't understand why is there "i - 1" here (VZ)
            decode_body(body, pfx, i - 1, count, write);
         }
         else
         {
            // build encapsulation prefix
            String prefix;
            prefix << pfx << strutil_ltoa(i) << '.';
            decode_body(body, prefix, 0l, count, write);
         }
      }
   }
}

BODY *
MessageCC::GetBody(void)
{
   if(m_folder == NULL) // this message has no m_folder associated
      return m_Body;

   int retry = 1;

   // Forget what we  know and re-fetch the body, it is cached anyway.
   m_Body = NULL;
   do
   {
      if(m_Body == NULL && m_folder)
      {
         if(m_folder->Stream() && m_folder->Lock())
         {
            m_Envelope =
               mail_fetchstructure_full(m_folder->Stream(),m_uid,
                                        &m_Body, FT_UID);
            m_folder->UnLock();
         }
         else
         {
            m_folder->PingReopen();
            MailFolderCC::ProcessEventQueue();
         }
      }
      else
         retry = 0;
   }while(retry-- && ! (m_Envelope && m_Body));
   MailFolderCC::ProcessEventQueue();

   CHECK(m_Body && m_Envelope, NULL, _("Non-existent message data."));
   return m_Body;
}


/** Return the numeric status of message. */
int
MessageCC::GetStatus(
   unsigned long *size,
   unsigned int *day,
   unsigned int *month,
   unsigned int *year) const
{
   if(m_folder == NULL
      || ! ((MessageCC *)this)->GetBody()
      || ! m_folder->Lock())
      return MailFolder::MSG_STAT_NONE;


   MESSAGECACHE *mc = mail_elt(m_folder->Stream(), mail_msgno(m_folder->Stream(),m_uid));
   m_folder->UnLock();

   if(size)    *size = mc->rfc822_size;
   if(day)  *day = mc->day;
   if(month)   *month = mc->month;
   if(year) *year = mc->year + BASEYEAR;

   int status = MailFolder::MSG_STAT_NONE;
   if(mc->seen)      status |= MailFolder::MSG_STAT_SEEN;
   if(mc->answered)  status |= MailFolder::MSG_STAT_ANSWERED;
   if(mc->deleted)   status |= MailFolder::MSG_STAT_DELETED;
   if(mc->searched)  status |= MailFolder::MSG_STAT_SEARCHED;
   if(mc->recent)    status |= MailFolder::MSG_STAT_RECENT;
   if(mc->flagged)   status |= MailFolder::MSG_STAT_FLAGGED;
   return (MailFolder::MessageStatus) status;
}


void
MessageCC::DecodeMIME(void)
{
   if(!GetBody())
      return;

   if(m_partInfos == NULL)
   {
      // FIXME it is stupid to invoke decode_body() twice: first time from
      //       inside CountParts and the second time just below, it should
      //       instead allocate m_partInfos itself as needed (VZ)
      int nparts = CountParts();
      m_partInfos = new PartInfo[nparts];
      int count = 0;
      decode_body(m_Body, "", 0l, &count, true);
   }
}

int
MessageCC::CountParts(void)
{
   if ( m_numOfParts == -1 )
   {
      if(!GetBody())
         return -1 ;

      // don't gather info, just count the parts
      decode_body(m_Body, "", 0l, &m_numOfParts, false);
      m_numOfParts++;
   }

   return m_numOfParts;
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
   char *cptr = mail_fetchbody_full(m_folder->Stream(),
                                    m_uid,
                                    (char *)sp.c_str(),
                                    &len, FT_UID);
   m_folder->UnLock();
   MailFolderCC::ProcessEventQueue();

   if(lenptr == NULL)
      lenptr  = &len;   // so to give c-client lib a place where to write to

   if(len == 0)
   {
      *lenptr = 0;
      return NULL;
   }

   if(partContentPtr)
      delete [] partContentPtr;
   partContentPtr = new char[len+1];
   strncpy(partContentPtr, cptr, len);
   partContentPtr[len] = '\0';
   //fs_give(&cptr); // c-client's free() function

   /* This bit is a bit suspicious, it fails sometimes in
      rfc822_qprint() when HTML code is fed into it and while Mahogany
      has long done all this, it seems to be not significantly worse
      if I comment it all out. So what I do now, is to just return the
      plain text if the decoding failed.
      FIXME I should really find out whether this is correct :-)
   */

   unsigned char *data = (unsigned char *)partContentPtr; // cast for cclient
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
      return partContentPtr;

   case ENC7BIT:     // 7 bit SMTP semantic data
   case ENC8BIT:        // 8 bit SMTP semantic data
   case ENCOTHER:    //unknown
   default:
      *lenptr = strlen(partContentPtr);
      returnVal = partContentPtr;
   }
   if(returnVal == NULL)
      return partContentPtr;
   else if(returnVal != partContentPtr)
   { // we need to copy it over
      if(partContentPtr) delete [] partContentPtr;
      partContentPtr = new char [(*lenptr)+1];
      memcpy(partContentPtr, returnVal, *lenptr);
      partContentPtr[(*lenptr)] = '\0';
      fs_give((void **)&returnVal);
   }
   return partContentPtr;
}

MessageParameterList const &
MessageCC::GetParameters(int n)
{
   DecodeMIME();

   return m_partInfos[n].parameterList;
}


MessageParameterList const &
MessageCC::GetDisposition(int n, String *disptype)
{
   DecodeMIME();

   if(disptype)
      *disptype = m_partInfos[n].dispositionType;
   return m_partInfos[n].dispositionParameterList;
}

Message::ContentType
MessageCC::GetPartType(int n)
{
   DecodeMIME();

   // by a greatest of hazards, the numerical types used by cclient lib are
   // just the same as Message::ContentType enum values - of course, if it
   // were not the case, we would have to translate them somehow!
   return (Message::ContentType)m_partInfos[n].numericalType;
}

int
MessageCC::GetPartTransferEncoding(int n)
{
   DecodeMIME();
   return m_partInfos[n].numericalEncoding;
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

   // return the size in lines for the text messages if requested, otherwise
   // return size in bytes
   if( useNaturalUnits &&
       (m_Body->type == TYPEMESSAGE || m_Body->type == TYPETEXT) )
      return m_partInfos[n].size_lines;
   else
      return m_partInfos[n].size_bytes;
}



String const &
MessageCC::GetPartMimeType(int n)
{
   DecodeMIME();
   return m_partInfos[n].type;
}

String const &
MessageCC::GetPartSpec(int n)
{
   DecodeMIME();
   return m_partInfos[n].mimeId;
}

String const &
MessageCC::GetPartDesc(int n)
{
   DecodeMIME();
   return m_partInfos[n].description;
}


String
MessageCC::GetId(void) const
{
   if(m_Body == NULL)
      ((MessageCC *)this)-> GetBody();
   ASSERT(m_Body);
   if(m_Body)
      return String(m_Body->id);
   else
      return "";
}

String
MessageCC::GetReferences(void) const
{
   if(m_Body == NULL)
      ((MessageCC *)this)-> GetBody();
   ASSERT(m_Body);
   if(m_Body)
      return String(m_Envelope->references);
   else
      return "";
}

bool
MessageCC::WriteToString(String &str, bool headerFlag) const
{
   if(! headerFlag)
   {
      str = ((MessageCC*)this)->FetchText();
      return str.Length() > 0;
   }
   else
   {
      unsigned long len;

      ((MessageCC *)this)->GetBody(); // circumvene const restriction

      if(m_folder && m_folder->Lock())
      {
         CHECK_DEAD_RC(FALSE);
         char *headerPart =
            mail_fetchheader_full(m_folder->Stream(),m_uid,NIL,&len,FT_UID);
         m_folder->UnLock();
         ASSERT_MSG(strlen(headerPart) == len,
                    "DEBUG: Mailfolder corruption detected");
#if 0
         str.reserve(len + 1);
         const char *cptr = headerPart;
         for(size_t i = 0; i < len; i++)
            str[i] = *cptr++;
         str[len] = '\0';
#else
         str += String(headerPart, (size_t)len);
#endif
         str += ((MessageCC*)this)->FetchText();
         MailFolderCC::ProcessEventQueue();
      }
      else
      {
         str = m_msgText;
      }
   }
   return (m_Body != NULL);
}



/* static */
class Message *
Message::Create(const char * itext,
               UIdType uid,
               Profile *iprofile)
{
   return MessageCC::Create(itext, uid, iprofile);
}
