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
#include   "Mcommon.h"

#ifndef USE_PCH
#   include "strutil.h"
#endif // USE_PCH

#include <wx/fontmap.h>

#include "gui/wxMFrame.h"
#include "FolderView.h"

#include "Mdefaults.h"

#include "MailFolder.h"
#include "MailFolderCC.h"

#include "Message.h"
#include "MessageCC.h"

#include "SendMessageCC.h"

#include "MApplication.h"
#include "MDialogs.h"

#include <ctype.h>

/// temporary buffer for storing message headers, be generous:
#define   HEADERBUFFERSIZE 100*1024

/// check for dead mailstream
#define CHECK_DEAD()   if( folder && folder->Stream() == NIL && ! folder->PingReopen() ) { ERRORMESSAGE((_("Cannot access inactive folder '%s'."), folder->GetName().c_str())); return; }
#define CHECK_DEAD_RC(rc)   if( folder && folder->Stream() == NIL && ! folder->PingReopen()) {   ERRORMESSAGE((_("Cannot access inactive folder '%s'."), folder->GetName().c_str())); return rc; }

MessageCC *
MessageCC::CreateMessageCC(MailFolderCC *ifolder,
                           unsigned long iuid)
{
   CHECK(ifolder, NULL, "NULL folder");
   return new MessageCC(ifolder, iuid);
}

MessageCC::MessageCC(MailFolderCC *ifolder,unsigned long iuid)
{
   mailText = NULL;
   m_Body = NULL;
   m_Envelope = NULL;
   partInfos = NULL; // this vector gets initialised when needed
   numOfParts = -1;
   partContentPtr = NULL;
   text = NULL;
   folder = ifolder;
   folder->IncRef();
   m_uid = iuid;
   m_Profile=ifolder->GetProfile();
   m_Profile->IncRef();
   Refresh();
}


MessageCC::~MessageCC()
{
   if(partInfos != NULL)
      delete [] partInfos;
   if(partContentPtr)
      delete [] partContentPtr;
   if(text)
      delete [] text;
   if(folder)
      folder->DecRef();
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
   mailText = NULL;
   bodycptr = NULL;
   m_Envelope = NULL;
   m_Profile = iprofile;
   if(m_Profile)
      m_Profile->IncRef();
   else
      m_Profile = Profile::CreateEmptyProfile();
   partInfos = NULL; // this vector gets initialised when needed
   numOfParts = -1;
   partContentPtr = NULL;
   text = NULL;
   folder = NULL;
   m_uid = uid;

   // move \n --> \r\n convention
   text = strutil_strdup(strutil_enforceCRLF(itext));

   unsigned long pos = 0;
   // find end of header "\012\012"
   while(text[pos])
   {
      if((text[pos] == '\012' && text[pos+1] == '\012') // empty line
         // is end of header
         || text[pos+1] == '\0')
      {
         header = new char [pos+2];
         strncpy(header, text, pos+1);
         header[pos+1] = '\0';
         headerLen = pos+1;
         bodycptr = text + pos + 2;
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
      return sendMsg.Send();
   else
      return sendMsg.SendOrQueue();
}

void
MessageCC::Refresh(void)
{
   if(GetBody())
   {
      hdr_date = m_Envelope->date ? String(m_Envelope->date) :  :: String("");
      hdr_subject = m_Envelope->subject ? String(m_Envelope->subject) :
         String("");
      hdr_subject = MailFolderCC::DecodeHeader(hdr_subject);
   }
}

String const &
MessageCC::Subject(void) const
{
   return   hdr_subject;
}

String const
MessageCC::From(void) const
{
   String tmp;
   return Address(tmp, MAT_REPLYTO);
}

String
MessageCC::GetHeader(void) const
{
   String str;

   CHECK_DEAD_RC(str);

   if( folder && folder->Lock() )
   {
      unsigned long len = 0;
      const char *cptr = mail_fetchheader_full(folder->Stream(), m_uid,
                                               NULL, &len, FT_UID);
      folder->UnLock();
      str = String(cptr, (size_t)len);
      MailFolderCC::ProcessEventQueue();
   }

   return str;
}

bool
MessageCC::GetHeaderLine(const String &line,
                         String &value, wxFontEncoding *encoding)
{
   CHECK_DEAD_RC(false);
   if(! folder)
      return false;

   STRINGLIST  slist;
   slist.next = NULL;
   slist.text.size = line.length();
   slist.text.data = (unsigned char *)strutil_strdup(line);

   if(!folder->Lock())
      return false;

   unsigned long len;
   char *
      rc = mail_fetchheader_full (folder->Stream(),
                                  m_uid,
                                  &slist,
                                  &len,FT_UID);
   folder->UnLock();
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

String const
MessageCC::Address(String &name, MessageAddressType type) const
{
   ((MessageCC *)this)->GetBody();
   CHECK(m_Envelope, "", _("Non-existent message data."))

   ADDRESS
      * addr = NULL;
   String email0, name0;
   String email;

   name = "";
   switch(type)
   {
   case MAT_FROM:
      addr = m_Envelope->from;
      break;
   case MAT_TO:
      addr = m_Envelope->to;
      break;
   case MAT_CC:
      addr = m_Envelope->cc;
      break;
   case MAT_SENDER:
      addr = m_Envelope->sender;
      break;
   case MAT_REPLYTO:
      addr = m_Envelope->reply_to;
      if(! addr)
         addr = m_Envelope->from;
      if(! addr)
         addr = m_Envelope->sender;
      if(addr && addr->personal && strlen(addr->personal))
         name = String(addr->personal);
/*
  This is a rather nice hack which takes the "From:" personal name and
  combines it with the reply-to to return a complete mail address
  including name and email, but it seems to cause too much confusion
  to others if used on mailing lists.

  else
      {
         if(m_Envelope->from &&
            m_Envelope->from->personal &&
            strlen(m_Envelope->from->personal))
            name = String(m_Envelope->from->personal);
      }
*/
      break;
   }

   if(! addr)
      return "";

   while(addr)
   {
      email0 = String(addr->mailbox);
      if(addr->host && strlen(addr->host)
         && (strcmp(addr->host,BADHOST) != 0))
         email0 += String("@") + String(addr->host);
      email0 = MailFolderCC::DecodeHeader(email0);
      if(addr->personal && strlen(addr->personal))
      {
         name0 = String(addr->personal);
         name0 = MailFolderCC::DecodeHeader(name0);
      }
      else
         name0 = "";
      if(strchr(name0, ',') || strchr(name0,'<') || strchr(name0,'>'))
         name0 = String("\"") + name0 + String("\"");

      if(email[0]) email += ", ";
      email += email0;

      // for now: use first name found:
      if(name[0] == '\0') 
        name = name0;
      else if (name0[0]) // found another name
        name << " ...";

      
      addr = addr->next;
   }
   return email;
}

String const &
MessageCC::Date(void) const
{
   return   hdr_date;
}

String
MessageCC::FetchText(void)
{
   CHECK_DEAD_RC("");
   if(folder && folder->Lock())
   {
      mailText = mail_fetchtext_full(folder->Stream(), m_uid,
                                     &m_MailTextLen, FT_UID);
      folder->UnLock();
      ASSERT_MSG(strlen(mailText) == m_MailTextLen,
                 "DEBUG: Mailfolder corruption detected");
      MailFolderCC::ProcessEventQueue();
#if 0
      String str; 
      str.reserve(m_MailTextLen);
      const char *cptr = mailText;
      for(size_t i = 0; i < m_MailTextLen; i++)
         str[i] += *cptr++;
      str[m_MailTextLen] = '\0';
      return str;
#else
	return mailText;
#endif
   }
   else // from a text
      return text;
}

/*
  This function is taken straight from the mtest IMAP example. Instead
  of printing the body parts, it is now submitting this information to
  the list of body parts.
*/

void
MessageCC::decode_body (BODY *body, String & pfx,long i,
                        int * count, bool write, bool firsttime)
{
   String   line;
   PARAMETER *par;
   PART *part;
   String   mimeId, type, desc, parms, id;
   MessageParameter *parameter;
   MessageParameterList plist(false); // temporary, doesn't own entries
   MessageParameterList dispPlist(false); // temporary, doesn't own entries
   MessageParameterList::iterator plist_iterator;

   /* multipart doesn't have a row to itself */
   if (body->type == TYPEMULTIPART)
   {
      /* if not first time, extend prefix */
      if(! firsttime)
         line = pfx + strutil_ltoa(++i) + String(".");
      for (i = 0,part = body->nested.part; part; part = part->next)
         decode_body (&part->body,line,i++, count, write, false);
   }
   else
   {
      line = pfx;
      /* non-multipart, output oneline descriptor */
      line = pfx + strutil_ltoa(++i);
      mimeId = line;
      type = body_types[body->type];
      if (body->subtype)
         type = type + '/' + body->subtype;
      line += type;
      if (body->description)
      {
         desc  = body->description;
         line = line + " (" + desc + ')';
      }
      if ((par = body->parameter) != NULL)
      {
         do
         {
            if(write)
            {
               parameter = new MessageParameter();
               parameter->name = par->attribute;
               parameter->value = par->value;
               plist.push_back(parameter);
            }
            parms = parms + par->attribute + '=' + par->value;
            if(par->next)
               parms += ':';
         }
         while ((par = par->next) != NULL);
         line  += parms;
      }
      if ((par = body->disposition.parameter) != NULL)
      {
         do
         {
            if(write)
            {
               parameter = new MessageParameter();
               parameter->name = par->attribute;
               parameter->value = par->value;
               dispPlist.push_back(parameter);
            }
         }
         while ((par = par->next) != NULL);
      }
      if (body->id)
      {
         id = body->id;
         line = line + String(" , id=") + id;
      }
      switch (body->type)
      {
         /* bytes or lines depending upon body type */
      case TYPEMESSAGE:    /* encapsulated message */
      case TYPETEXT:    /* plain text */
         line = line + String(" (") + strutil_ltoa(body->size.lines) + String(" lines)");
         break;
      default:
         line = line + String(" (") + strutil_ltoa(body->size.bytes) + String(" bytes)");
         break;
      }

      if(write)
      {
         PartInfo &pi = partInfos[*count];
         pi.mimeId = mimeId;
         pi.type = type;
         pi.numericalType  = body->type;
         pi.numericalEncoding = body->encoding;
         pi.params = parms;
         pi.description = desc;
         pi.id = id;
         pi.size_lines = body->size.lines;
         pi.size_bytes = body->size.bytes;
         for(plist_iterator = plist.begin(); plist_iterator !=
                plist.end(); plist_iterator++)
            pi.parameterList.push_back(*plist_iterator);
         pi.dispositionType = body->disposition.type;
         for(plist_iterator = dispPlist.begin(); plist_iterator !=
                dispPlist.end(); plist_iterator++)
            pi.dispositionParameterList.push_back(*plist_iterator);
      }
      (*count)++;

      /* encapsulated message? */
      if ((body->type == TYPEMESSAGE) && !strcmp (body->subtype,"RFC822") &&
          (body = body->nested.msg->body))
      {
         if (body->type == TYPEMULTIPART)
            decode_body (body,pfx,i-1,count, write, false);
         else
         {       /* build encapsulation prefix */
            line = pfx;
            line = line + strutil_ltoa(i);
            decode_body (body,line,(long) 0,count, write, false);
         }
      }
   }
}

BODY *
MessageCC::GetBody(void)
{
   if(folder == NULL) // this message has no folder associated
      return m_Body;

   int retry = 1;

   // Forget what we  know and re-fetch the body, it is cached anyway.
   m_Body = NULL;
   do
   {
      if(m_Body == NULL && folder)
      {
         if(folder->Stream() && folder->Lock())
         {
            m_Envelope = mail_fetchstructure_full(folder->Stream(),m_uid, &m_Body,
                                                  FT_UID);
            folder->UnLock();
         }
         else
         {
            folder->PingReopen();
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
   if(folder == NULL
      || ! ((MessageCC *)this)->GetBody()
      || ! folder->Lock())
      return MailFolder::MSG_STAT_NONE;


   MESSAGECACHE *mc = mail_elt(folder->Stream(), mail_msgno(folder->Stream(),m_uid));
   folder->UnLock();

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

   if(partInfos == NULL)
   {
      int nparts = CountParts();
      partInfos = new PartInfo[nparts];
      int count = 0;
      String tmp = "" ;
      decode_body(m_Body,tmp, 0, &count, true);
   }
}

int
MessageCC::CountParts(void)
{
   if(numOfParts == -1)
   {
      if(!GetBody())
         return -1 ;
      // don't gather info, just count the parts
      String a = "";
      decode_body(m_Body,a, 0l, &numOfParts, false);
      numOfParts ++;
   }
   return numOfParts;
}


const char *
MessageCC::GetPartContent(int n, unsigned long *lenptr)
{
   DecodeMIME();

   if(! folder)
      return NULL;

   long  unsigned
      len = 0;
   String const
      & sp = GetPartSpec(n);

   if(! folder->Lock())
      return NULL;
   char *cptr = mail_fetchbody_full(folder->Stream(),
                                    m_uid,
                                    (char *)sp.c_str(),
                                    &len, FT_UID);
   folder->UnLock();
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
   const char * returnVal = NULL;
   switch(GetPartTransferEncoding(n))
   {
   case ENCBASE64:      // base-64 encoded data
      returnVal = (const char *) rfc822_base64((unsigned char
                                                *)partContentPtr,GetPartSize(n),lenptr);
      break;
   case ENCQUOTEDPRINTABLE:   // human-readable 8-as-7 bit data
      returnVal = (const char *) rfc822_qprint((unsigned char
                                                *)partContentPtr,GetPartSize(n,true),lenptr);
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

   return partInfos[n].parameterList;
}


MessageParameterList const &
MessageCC::GetDisposition(int n, String *disptype)
{
   DecodeMIME();

   if(disptype)
      *disptype = partInfos[n].dispositionType;
   return partInfos[n].dispositionParameterList;
}

Message::ContentType
MessageCC::GetPartType(int n)
{
   DecodeMIME();

   // by a greatest of hazards, the numerical types used by cclient lib are
   // just the same as Message::ContentType enum values - of course, if it
   // were not the case, we would have to translate them somehow!
   return (Message::ContentType)partInfos[n].numericalType;
}

int
MessageCC::GetPartTransferEncoding(int n)
{
   DecodeMIME();
   return partInfos[n].numericalEncoding;
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
MessageCC::GetPartSize(int n, bool forceBytes)
{
   DecodeMIME();

   if((m_Body->type == TYPEMESSAGE || m_Body->type == TYPETEXT)
      && ! forceBytes)
      return partInfos[n].size_lines;
   else
      return partInfos[n].size_bytes;
}



String const &
MessageCC::GetPartMimeType(int n)
{
   DecodeMIME();
   return partInfos[n].type;
}

String const &
MessageCC::GetPartSpec(int n)
{
   DecodeMIME();
   return partInfos[n].mimeId;
}

String const &
MessageCC::GetPartDesc(int n)
{
   DecodeMIME();
   return partInfos[n].description;
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

void
MessageCC::WriteToString(String &str, bool headerFlag) const
{
   if(! headerFlag)
   {
      str = ((MessageCC*)this)->FetchText();
   }
   else
   {
      unsigned long len;

      ((MessageCC *)this)->GetBody(); // circumvene const restriction

      if(folder && folder->Lock())
      {
         CHECK_DEAD();
         char *headerPart =
            mail_fetchheader_full(folder->Stream(),m_uid,NIL,&len,FT_UID); 
         folder->UnLock();
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
         str = text;
      }
   }
}



/* static */
class Message *
Message::Create(const char * itext,
               UIdType uid,
               Profile *iprofile)
{
   return MessageCC::Create(itext, uid, iprofile);
}
