/*-*- c++ -*-********************************************************
 * SendMessageCC class: c-client implementation of a compose message*
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
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
    puts $f "Facsimile:    +$recipient(country) $recipient(local) $recipient(num
ber)"
    puts $f "Email:        $recipient(email)"
    puts $f ""
    puts $f "Originator:   $user(name)"
    puts $f "Organization: $user(organisation)"
    puts $f "Telephone:    $user(tel)"
    puts $f "Facsimile:    $user(fax)"
    puts $f "Email:        $user(email)"




 */

#ifdef __GNUG__
#   pragma implementation "SendMessageCC.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#ifndef  USE_PCH
#   include "Profile.h"
#   include "strutil.h"
#   include "strings.h"
#   include "guidef.h"

#   include <strings.h>


#endif // USE_PCH

#include "Mdefaults.h"
#include "MApplication.h"
#include "Message.h"
#include "MailFolderCC.h"
#include "SendMessageCC.h"
#include "XFace.h"
#include "MDialogs.h"
#include "gui/wxIconManager.h"

#include <wx/utils.h> // wxGetFullHostName()

extern "C"
{
#  include <misc.h>

   void rfc822_setextraheaders(const char **names, const char **values);
}

#define  CPYSTR(x)   cpystr(x)

/// temporary buffer for storing message headers, be generous:
#define   HEADERBUFFERSIZE 100*1024

SendMessageCC::SendMessageCC(ProfileBase *iprof,
                             Protocol protocol)
{
   Create(protocol, iprof);
}

void
SendMessageCC::Create(Protocol protocol,
                      ProfileBase *iprof)
{
   String tmpstr;

   m_Profile = iprof;
   m_Profile->IncRef(); // make sure it doesn't go away
   m_headerNames = NULL;
   m_headerValues = NULL;
   m_protocol = protocol;

   m_Envelope = mail_newenvelope();
   m_Body = mail_newbody();

   m_Envelope->from = mail_newaddr();
   m_Envelope->from->personal =
      CPYSTR(m_Profile->readEntry(MP_PERSONALNAME, MP_PERSONALNAME_D));
   m_Envelope->from->mailbox =
      CPYSTR(m_Profile->readEntry(MP_USERNAME, MP_USERNAME_D));

   tmpstr = m_Profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D);
   m_Envelope->from->host = tmpstr.Length() ? CPYSTR(tmpstr) : NIL;
   m_Envelope->return_path = mail_newaddr ();

   tmpstr = m_Profile->readEntry(MP_RETURN_ADDRESS, MP_RETURN_ADDRESS_D);
   if(strutil_isempty(tmpstr))
      tmpstr = m_Profile->readEntry(MP_USERNAME,MP_USERNAME_D);
   else
      tmpstr = strutil_before(tmpstr,'@');
   m_Envelope->return_path->mailbox = CPYSTR(tmpstr);

   tmpstr = m_Profile->readEntry(MP_RETURN_ADDRESS, MP_RETURN_ADDRESS_D);
   if(strutil_isempty(tmpstr))
      tmpstr = m_Profile->readEntry(MP_HOSTNAME,MP_HOSTNAME_D);
   else
      tmpstr = strutil_after(tmpstr,'@');
   if(tmpstr.Length() == 0)
   {
      // we need a valid return path!
      m_Envelope->return_path->host = CPYSTR(wxGetFullHostName());
   }
   else
      m_Envelope->return_path->host = CPYSTR(tmpstr);

   m_Body->type = TYPEMULTIPART;
   m_Body->nested.part = mail_newbody_part();
   m_Body->nested.part->next = NULL;
   m_NextPart = m_Body->nested.part;
   m_LastPart = m_NextPart;
}

void
SendMessageCC::SetSubject(const String &subject)
{
   if(m_Envelope->subject) delete [] m_Envelope->subject;
   m_Envelope->subject = CPYSTR(subject.c_str());
}

void
SendMessageCC::SetAddresses(const String &to,
                            const String &cc,
                            const String &bcc)
{
   String
      tmpstr;
   char
      * tmp,
      * tmp2;

   // If Build() has already been called, then it's too late to change
   // anything.
   ASSERT(m_headerNames == NULL);
   ASSERT(m_protocol == Prot_SMTP);

   String
      defaulthost;
   if(READ_CONFIG(m_Profile, MP_ADD_DEFAULT_HOSTNAME))
        defaulthost = m_Profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D);

   if(to.Length())
   {
      ASSERT(m_Envelope->to == NIL);
      tmpstr = to;   ExtractFccFolders(tmpstr);
      tmp = strutil_strdup(tmpstr);
      tmp2 = defaulthost.Length() ? strutil_strdup(defaulthost) : NIL;
      rfc822_parse_adrlist (&m_Envelope->to,tmp,tmp2);
      delete [] tmp; delete [] tmp2;
   }
   if(cc.Length())
   {
      ASSERT(m_Envelope->cc == NIL);
      tmpstr = cc;   ExtractFccFolders(tmpstr);
      tmp = strutil_strdup(tmpstr);
      tmp2 = defaulthost.Length() ? strutil_strdup(defaulthost) : NIL;
      rfc822_parse_adrlist (&m_Envelope->cc,tmp,tmp2);
      delete [] tmp; delete [] tmp2;
   }
   if(bcc.Length())
   {
      ASSERT(m_Envelope->bcc == NIL);
      tmpstr = bcc;   ExtractFccFolders(tmpstr);
      tmp = strutil_strdup(tmpstr);
      tmp2 = defaulthost.Length() ? strutil_strdup(defaulthost) : NIL;
      rfc822_parse_adrlist (&m_Envelope->bcc,tmp,tmp2);
      delete [] tmp; delete [] tmp2;
   }
}

void
SendMessageCC::SetNewsgroups(const String &groups)
{
   // If Build() has already been called, then it's too late to change
   // anything.
   ASSERT(m_headerNames == NULL);
   ASSERT(m_protocol == Prot_NNTP);

   if(groups.Length())
   {
      ASSERT(m_Envelope->newsgroups == NIL);
      m_Envelope->newsgroups = strutil_strdup(groups);
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



void
SendMessageCC::Build(void)
{
   int
      j, h = 0;
   char
      tmpbuf[MAILTMPLEN];
   String tmpstr;
   char
      *headers;
   kbStringList::iterator
      i, i2;

   if(m_headerNames != NULL) // message was already build
      return;

   headers = strutil_strdup(READ_CONFIG(m_Profile, MP_EXTRAHEADERS));
   strutil_tokenise(headers,";",m_headerList);
   delete [] headers;

   bool replyToSet = false;

   // +4: 1 for X-Mailer, 1 for X-Face, 1 for reply to and 1 for the
   // last NULL entry
   size_t n = m_headerList.size() + m_ExtraHeaderLinesNames.size() + 4;
   m_headerNames = new const char*[n];
   m_headerValues = new const char*[n];
   /* Add the extra headers as defined in list of extra headers in
      m_Profile (backward compatibility): */
   for(i = m_headerList.begin(), j = 0; i != m_headerList.end(); i++, h++)
   {
      m_headerNames[h] = strutil_strdup(**i);
      if(strcmp(m_headerNames[h], "Reply-To") == 0)
         replyToSet = true;
      // this is clever: read header value from m_Profile:
      m_headerValues[h] = strutil_strdup(m_Profile->readEntry(**i,""));
   }

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
#ifdef OS_UNIX
      version << "Mahogany, " << M_VERSION_STRING << _(" , compiled for ") << M_OSINFO;
#else // Windows
      // TODO put Windows version info here
      version << "Mahogany, " << M_VERSION_STRING << _(" , compiled for ") << "Windows";
#endif // Unix/Windows
      m_headerValues[h++] = strutil_strdup(version);
   }
   if(! replyToSet)
   {
      if(! HasHeaderEntry("Reply-To"))
      {
         //always add reply-to header:
         tmpstr = m_Profile->readEntry(MP_RETURN_ADDRESS, MP_RETURN_ADDRESS_D);
         if(!strutil_isempty(tmpstr))
         {
            m_headerNames[h] = strutil_strdup("Reply-To");
            m_headerValues[h++] = strutil_strdup(tmpstr);
         }
      }
   }
#ifdef HAVE_XFACES
   // add an XFace?
   if(READ_CONFIG(m_Profile,MP_COMPOSE_USE_XFACE) && ! HasHeaderEntry("X-Face"))
   {
      XFace xface;
      xface.CreateFromFile(READ_CONFIG(m_Profile, MP_COMPOSE_XFACE_FILE));
      m_headerNames[h] = strutil_strdup("X-Face");
      m_headerValues[h] = strutil_strdup(xface.GetHeaderLine());
      if(strlen(m_headerValues[h]))  // paranoid, I know.
         ((char*) (m_headerValues[h]))[strlen(m_headerValues[h])-1] =
            '\0'; // cut off \n
      h++;
   }
#endif

   m_headerNames[h] = NULL;
   m_headerValues[h] = NULL;
   rfc822_setextraheaders(m_headerNames,m_headerValues);

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
                       MessageParameterList const *plist)
{
   BODY
      *bdy;
   unsigned char
      *data;

   data = (unsigned char *) fs_get (len);
   memcpy(data,buf,len);

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

   switch(type)
   {
   case TYPEMESSAGE:
      //FIXME: leads to empty body?
      bdy->nested.msg = mail_newmsg();
   case TYPETEXT:
      bdy->type = type;
      bdy->subtype = (char *) fs_get( subtype.length()+1);
      strcpy(bdy->subtype,(char *)subtype.c_str());
      bdy->contents.text.data = data;
      bdy->contents.text.size = len;
      bdy->encoding = (type == TYPETEXT) ? ENC8BIT : ENC7BIT;
      /* ENC8BIT for text is auto-converted to quoted pritable */
      break;

   default:
      bdy->type = type;
      bdy->subtype = (char *) fs_get(subtype.length()+1);
      strcpy(bdy->subtype,(char *)subtype.c_str());
      bdy->contents.text.data = data;
      bdy->contents.text.size = len;
      bdy->encoding = ENCBINARY;
      break;
   }
   m_NextPart->next = mail_newbody_part();
   m_LastPart = m_NextPart;
   m_NextPart = m_NextPart->next;
   m_NextPart->next = NULL;

   if(plist)
   {
      MessageParameterList::iterator i;
      PARAMETER *lastpar = NULL, *par;

      // set default charset, currently we always use ISO-8859-1,
      // which is latin1

      if(type == TYPETEXT)
      {
         String charSet = READ_CONFIG(m_Profile, MP_CHARSET);
         if(charSet.Length() != 0)
         {
            par = mail_newbody_parameter();
            par->attribute = strutil_strdup("CHARSET");
            par->value     = strutil_strdup(charSet);
            par->next      = NULL;
            lastpar = par;
         }
      }
      for(i=plist->begin(); i != plist->end(); i++)
      {
         par = mail_newbody_parameter();
         par->attribute = strutil_strdup((*i)->name);
         par->value     = strutil_strdup((*i)->value);
         par->next      = NULL;
         if(lastpar)
            lastpar->next = par;
         else
            bdy->parameter = par;
      }
      
   }
   bdy->disposition.type = strutil_strdup(disposition);
   bdy->parameter = NULL;
   if(dlist)
   {
      MessageParameterList::iterator i;
      PARAMETER *lastpar = NULL, *par;

      for(i=dlist->begin(); i != dlist->end(); i++)
      {
         par = mail_newbody_parameter();
         par->attribute = strutil_strdup((*i)->name);
         par->value     = strutil_strdup((*i)->value);
         par->next      = NULL;
         if(lastpar)
            lastpar->next = par;
         else
            bdy->disposition.parameter = par;
      }
   }
}

bool
SendMessageCC::Send(void)
{
   Build();


   MCclientLocker locker();
   
   SENDSTREAM
      *stream = NIL;
   const char
      *hostlist[2];
   char
      tmpbuf[MAILTMPLEN];
   bool
      success = true;

   kbStringList::iterator i;
   for(i = m_FccList.begin(); i != m_FccList.end(); i++)
      WriteToFolder(**i);

   String host;
   hostlist[1] = NIL;
   switch(m_protocol)
   {
   case Prot_SMTP:
      // notice that we _must_ assign the result to this string!
      host = READ_CONFIG(m_Profile, MP_SMTPHOST);
      hostlist[0] = host;
      DBGMESSAGE(("Trying to open connection to SMTP server '%s'", host.c_str()));
      stream = smtp_open ((char **)hostlist,NIL);
      break;
   case Prot_NNTP:
      // notice that we _must_ assign the result to this string!
      host = READ_CONFIG(m_Profile, MP_NNTPHOST);
      hostlist[0] = host;
      DBGMESSAGE(("Trying to open connection to NNTP server '%s'", host.c_str()));
      stream = nntp_open ((char **)hostlist,NIL);
      break;
   }

   if (stream)
   {
      switch(m_protocol)
      {
      case Prot_SMTP:
         success = smtp_mail (stream,"MAIL",m_Envelope,m_Body) != 0;
         smtp_close (stream);
         break;
      case Prot_NNTP:
         success = nntp_mail (stream,m_Envelope,m_Body) != 0;
         nntp_close (stream);
         break;
      }
      if(success)
      {
         MDialog_Message(m_protocol==Prot_SMTP?_("Message sent."):_("Article posted."),
                         NULL, // parent window
                         MDIALOG_MSGTITLE,
                         "MailSentMessage");
      }
      else
      {
         sprintf (tmpbuf, _("Failed to send - %s"),
                  (stream->reply)?stream->reply:_("unknown error"));
         ERRORMESSAGE((tmpbuf));
         success = false;
      }
   }
   else
   {
      ERRORMESSAGE ((_("Cannot open connection to any server")));
      success = false;
   }
   return success;
}

static long write_output(void *stream, char *string)
{
   ostream *o = (ostream *)stream;
   *o << string;
   if(o->fail())
      return NIL;
   else
      return T;
}

static long write_str_output(void *stream, char *string)
{
   String *o = (String *)stream;
   *o << string;
   return T;
}

void
SendMessageCC::WriteToString(String  &output)
{
   Build();

   char *buffer = new char[HEADERBUFFERSIZE];

   if(! rfc822_output(buffer, m_Envelope, m_Body, write_str_output,&output,NIL))
      ERRORMESSAGE (("[Can't write message to string.]"));
   delete [] buffer;
}

/** Writes the message to a file
    @param filename file where to write to
    */
void
SendMessageCC::WriteToFile(String const &filename, bool append)
{
   Build();

   // buffer for message header, be generous:
   char *buffer = new char[HEADERBUFFERSIZE];
   ofstream  *ostr = new ofstream(filename.c_str(), ios::out | (append ? 0 : ios::trunc));

   if(! rfc822_output(buffer, m_Envelope, m_Body, write_output,ostr,NIL))
      ERRORMESSAGE (("[Can't write message to file %s]",
                     filename.c_str()));
   delete [] buffer;
   delete ostr;
}

void
SendMessageCC::WriteToFolder(String const &name, MailFolder::Type type)
{
   Build();

   String str;

   WriteToString(str);
   MailFolder *mf =
      MailFolder::OpenFolder(type,name);
   CHECK_RET(mf,String(_("Cannot open folder to save to:")+name));

   // we don't want this to create new mail events
   bool events = mf->SendsNewMailEvents();
   mf->EnableNewMailEvents(false, true);
   mf->AppendMessage(str);
   mf->Ping();
   mf->EnableNewMailEvents(events, true);
   mf->DecRef();
}

SendMessageCC::~SendMessageCC()
{
   mail_free_envelope (&m_Envelope);
   mail_free_body (&m_Body);

   rfc822_setextraheaders(NULL,NULL);

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
   m_Profile->DecRef();
}
