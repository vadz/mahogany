/*-*- c++ -*-********************************************************
 * Message class: entries for message header                        *
 *                      implementation for MailFolderCC             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
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

#include "gui/wxMFrame.h"
#include "FolderView.h"

#include "Mdefaults.h"

#include "MailFolder.h"
#include "MailFolderCC.h"

#include "Message.h"
#include "MessageCC.h"

#include <ctype.h>

/// temporary buffer for storing message headers, be generous:
#define   HEADERBUFFERSIZE 100*1024

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
   body = NULL;
   envelope = NULL;
   partInfos = NULL; // this vector gets initialised when needed
   numOfParts = -1;
   partContentPtr = NULL;
   text = NULL;
   folder = ifolder;
   folder->IncRef();
   m_uid = iuid;
   Refresh();
}


MessageCC::~MessageCC()
{
   if(partInfos != NULL)
      delete [] partInfos;
   if(partContentPtr)
      delete  partContentPtr;
   if(text)
      delete [] text;
   if(folder)
      folder->DecRef();
}

#if  0
MessageCC::MessageCC(const char * /* itext */,  ProfileBase *iprofile)
{
   Create(iprofile);

   char
      *header = NULL;
   unsigned long
      headerLen;
   STRING
      *b;

   text = strutil_strdup(itext);

   unsigned long pos = 0;
   // find end of header "\012\012"
   while(text[pos])
   {
      if(text[pos] == '\012' && text[pos+1] == '\012') // empty line is end of header
      {
         header = new char [pos+2];
         strncpy(header, text, pos+1);
         header[pos+1] = '\0';
         headerLen = pos+1;
         break;
      }
   }
   if(! header)
      return;  // failed

   char *buf = new char [headerLen];
   rfc822_parse_msg (&envelope, &body,header,headerLen, b,
                     ""   /*defaulthostname */,  buf);
   delete [] buf;
   initialisedFlag = true;
}
#endif


void
MessageCC::Refresh(void)
{
   GetBody();
   hdr_date = envelope->date ? String(envelope->date) :  :: String("");
   hdr_subject = envelope->subject ? String(envelope->subject) : String("");
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

const char *
MessageCC::GetHeader(void) const
{
   const char *cptr = mail_fetchheader_full(folder->Stream(), m_uid,
                                            NULL, NIL, FT_UID);
   MailFolderCC::ProcessEventQueue();
   return cptr;
}

void
MessageCC::GetHeaderLine(const String &line, String &value)
{
   STRINGLIST  slist;
   slist.next = NULL;
   slist.text.size = line.length();
   slist.text.data = (unsigned char *)strutil_strdup(line);

   char *
      rc = mail_fetchheader_full (folder->Stream(),
                                  m_uid,
                                  &slist,
                                  NIL,FT_UID);
   value = rc;
   value = strutil_after(value,':');
   strutil_delwhitespace(value);
   delete [] slist.text.data;
   MailFolderCC::ProcessEventQueue();
}

String const
MessageCC::Address(String &name, MessageAddressType type) const
{
   ((MessageCC *)this)->GetBody();

   ADDRESS
      * addr = NULL;
   String
      email;

   switch(type)
   {
   case MAT_FROM:
      addr = envelope->from;
      break;
   case MAT_SENDER:
      addr = envelope->sender;
      break;
   case MAT_REPLYTO:
      addr = envelope->reply_to;
      if(! addr)
         addr = envelope->from;
      if(! addr)
         addr = envelope->sender;
      break;
   }

   name = "";
   if(! addr)
      return "";

   email = String(addr->mailbox);
   if(addr->host && strlen(addr->host))
      email += String("@") + String(addr->host);

   if(addr->personal && strlen(addr->personal))
      name = String(addr->personal);

   return   email;
}

String const &
MessageCC::Date(void) const
{
   return   hdr_date;
}

char *
MessageCC::FetchText(void)
{
   if(folder)
   {
      mailText = mail_fetchtext_full(folder->Stream(), m_uid,
                                            NIL, FT_UID);
      MailFolderCC::ProcessEventQueue();
      return mailText;
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
   if(body == NULL && folder)
      envelope = mail_fetchstructure_full(folder->Stream(),m_uid, &body,
                                          FT_UID);
   MailFolderCC::ProcessEventQueue();
   return body;
}


/** Return the numeric status of message. */
int
MessageCC::GetStatus(
   unsigned long *size,
   unsigned int *day,
   unsigned int *month,
   unsigned int *year) const
{
   if(! ((MessageCC *)this)->GetBody())
      return MailFolder::MSG_STAT_NONE;

      
   MESSAGECACHE *mc = mail_elt(folder->Stream(), mail_msgno(folder->Stream(),m_uid));
   
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
      decode_body(body,tmp, 0, &count, true);
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
      decode_body(body,a, 0l, &numOfParts, false);
      numOfParts ++;
   }
   return numOfParts;
}


const char *
MessageCC::GetPartContent(int n, unsigned long *lenptr)
{
   DecodeMIME();
   long  unsigned
      len = 0;
   String const
      & sp = GetPartSpec(n);

   char *cptr = mail_fetchbody_full(folder->Stream(),
                                    m_uid,
                                    (char *)sp.c_str(),
                                    &len, FT_UID);
   MailFolderCC::ProcessEventQueue();

   if(lenptr == NULL)
      lenptr  = &len;   // so to give c-client lib a place where to write to

   if(len == 0)
   {
      *lenptr = 0;
      return NULL;
   }

   if(partContentPtr)
      delete partContentPtr;
   partContentPtr = new char[len+1];
   strncpy(partContentPtr, cptr, len);
   partContentPtr[len] = '\0';
   //fs_give(&cptr); // c-client's free() function

   switch(GetPartEncoding(n))
   {
   case  ENCBINARY:     // 8 bit binary data
      *lenptr = GetPartSize(n);
      return partContentPtr;
   case ENCBASE64:      // base-64 encoded data
      return (const char *) rfc822_base64((unsigned char *)partContentPtr,GetPartSize(n),lenptr);
   case ENCQUOTEDPRINTABLE:   // human-readable 8-as-7 bit data
      return (const char *) rfc822_qprint((unsigned char *)partContentPtr,GetPartSize(n,true),lenptr);
   case ENC7BIT:     // 7 bit SMTP semantic data
   case ENC8BIT:        // 8 bit SMTP semantic data
   case ENCOTHER:    //unknown
   default:
      *lenptr = strlen(partContentPtr);
      return partContentPtr;
   }

//   return partContentPtr;
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
MessageCC::GetPartEncoding(int n)
{
   DecodeMIME();
   return partInfos[n].numericalEncoding;
}

size_t
MessageCC::GetPartSize(int n, bool forceBytes)
{
   DecodeMIME();

   if((body->type == TYPEMESSAGE || body->type == TYPETEXT)
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


String const &
MessageCC::GetId(void) const
{
   if(body == NULL)
      ((MessageCC *)this)-> GetBody();
   ASSERT(body);
   return * new String(body->id);
}

String const &
MessageCC::GetReferences(void) const
{
   if(body == NULL)
      ((MessageCC *)this)-> GetBody();
   ASSERT(body);
   return * new String(envelope->references);
}

void
MessageCC::WriteToString(String &str, bool headerFlag) const
{
   unsigned long len;
   unsigned long fulllen = 0;

   ((MessageCC *)this)->GetBody(); // circumvene const restriction

   if(folder)
   {
      if(headerFlag)
      {
         char *headerPart = mail_fetchheader_full(folder->Stream(),m_uid,NIL,&len,FT_UID);
         str += headerPart;
         fulllen += len;
      }

      char *bodyPart = mail_fetchtext_full(folder->Stream(),m_uid,&len,FT_UID);
      str += bodyPart;
      fulllen += len;
      MailFolderCC::ProcessEventQueue();
   }
   else
   {
      str = text;
      fulllen = strlen(text);
   }
}



