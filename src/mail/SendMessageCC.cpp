/*-*- c++ -*-********************************************************
 * SendMessageCC class: c-client implementation of a compose message*
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$ *
 *                                                                  *
 * $Log$
 * Revision 1.6  1998/05/18 17:48:49  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 * *
 *                                                                  *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "SendMessageCC.h"
#endif

#include    "Mpch.h"

#ifndef  USE_PCH
#   include   "Mcommon.h"
#   include   "kbList.h"
#   include   "Profile.h"
#   include   "Mdefaults.h"
#   include   "strutil.h"
#   include   <strings.h>
#   include   "SendMessageCC.h"
#   include   "MApplication.h"
#   include   "MDialogs.h"
// includes for c-client library
extern "C"
{
#   include <stdio.h>
#   include <osdep.h>
#   include <rfc822.h>
#   include <smtp.h>
#   include <nntp.h>
#   include <misc.h>
}
#endif

extern "C"
{
#include <misc.h>

   void rfc822_setextraheaders(const char **names, const char **values);
}

#define  CPYSTR(x)   cpystr(x)

#ifdef   OS_WIN
#  define   TEXT_DATA_CAST(x)    ((unsigned char *)x)
#else
#  define   TEXT_DATA_CAST(x)    ((char *)x)
#endif

#define   StringCast(iterator)   ((String *)*i)


SendMessageCC::SendMessageCC(ProfileBase *iprof)
{
   Create(iprof);
}

SendMessageCC::SendMessageCC(ProfileBase *iprof, String const &subject,
                             String const &to, String const &cc,
                             String const &bcc)
{
   Create(iprof, subject, to, cc, bcc);
}

void
SendMessageCC::Create(ProfileBase *iprof)
{
     
   profile = iprof;

}
      
void
SendMessageCC::Create(ProfileBase *iprof,
                      String const &subject,
                      String const &to, String const &cc, String const &bcc)
{
   char  *tmp, *tmp2;
   
   Create(iprof);
   env = mail_newenvelope();
   body = mail_newbody();
   env->from = mail_newaddr();
   env->from->personal =
      CPYSTR(profile->readEntry(MP_PERSONALNAME, MP_PERSONALNAME_D));
   env->from->mailbox =
      CPYSTR(profile->readEntry(MP_USERNAME, MP_USERNAME_D));
   env->from->host =
      CPYSTR(profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D));
   env->return_path = mail_newaddr ();
   env->return_path->mailbox =
      CPYSTR(profile->readEntry(MP_RETURN_USERNAME,
                                profile->readEntry(MP_USERNAME,MP_USERNAME_D)));
   env->return_path->host =
      CPYSTR(profile->readEntry(MP_RETURN_HOSTNAME,
                                profile->readEntry(MP_HOSTNAME,MP_HOSTNAME_D)));

   tmp = strutil_strdup(to); tmp2 = strutil_strdup(profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D));
   rfc822_parse_adrlist (&env->to,tmp,tmp2);
   delete [] tmp; delete [] tmp2;
  
   tmp = strutil_strdup(cc);  tmp2 = strutil_strdup(profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D));
   rfc822_parse_adrlist (&env->cc,tmp,tmp2);
   delete [] tmp; delete [] tmp2;

   tmp = strutil_strdup(bcc); tmp2 = strutil_strdup(profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D));
   rfc822_parse_adrlist (&env->bcc,tmp,tmp2);
   delete [] tmp; delete [] tmp2;

   env->subject = CPYSTR(subject.c_str());
   body->type = TYPEMULTIPART;
   body->nested.part = mail_newbody_part();
   body->nested.part->next = NULL;
   nextpart = body->nested.part;
   lastpart = nextpart;
}

void
SendMessageCC::AddPart(int type, const char *buf, size_t len,
                       String const &subtype)
{
   BODY
      *bdy;
   char
      *data;

   data = (char *) fs_get (len);
   memcpy(data,buf,len);

   bdy = &(nextpart->body);
   
   switch(type)
   {
   case TYPETEXT:
   case TYPEMESSAGE:
      bdy->type = type;
      bdy->subtype = (char *) fs_get(subtype.length()+1);
      strcpy(bdy->subtype,(char *)subtype.c_str());
      bdy->contents.text.data = TEXT_DATA_CAST(data);
      bdy->contents.text.size = len;
      bdy->encoding = ENC8BIT;
      break;
   default:
      bdy->type = type;
      bdy->subtype = (char *) fs_get(subtype.length()+1);
      strcpy(bdy->subtype,(char *)subtype.c_str());
      bdy->contents.text.data = TEXT_DATA_CAST(data);
      bdy->contents.text.size = len;
      bdy->encoding = ENCBINARY;
      break;
   }
   nextpart->next = mail_newbody_part();
   lastpart = nextpart;
   nextpart = nextpart->next;
   nextpart->next = NULL;
}

void
SendMessageCC::Send(void)
{
   SENDSTREAM
      *stream = NIL;
   int
      j;
   char
      *headers,
      tmpbuf[MAILTMPLEN];
   const char
      *hostlist [2],
      **headerNames,
      **headerValues;
   kbList
      headerList;
   kbListIterator
      i;
   
   headers = strutil_strdup(profile->readEntry(MP_EXTRAHEADERS,MP_EXTRAHEADERS_D));
   strutil_tokenise(headers,";",headerList);

   headerNames = new const char*[headerList.size()+1];
   headerValues = new const char*[headerList.size()+1];
   for(i = headerList.begin(), j = 0; i != headerList.end(); i++, j++)
   {
      headerNames[j] = strutil_strdup(StringCast(i)->c_str());
      headerValues[j] = profile->readEntry(StringCast(i)->c_str(),"");
   }
   headerNames[j] = NULL;
   headerValues[j] = NULL;
   rfc822_setextraheaders(headerNames,headerValues);
   
   mail_free_body_part(&lastpart->next);
   lastpart->next = NULL;
   rfc822_date (tmpbuf);
   env->date = (char *) fs_get (1+strlen (tmpbuf));
   strcpy (env->date,tmpbuf);

   
   hostlist[0] = profile->readEntry(MP_SMTPHOST, MP_SMTPHOST_D);
   hostlist[1] = NIL;
   if ((stream = smtp_open ((char **)hostlist,NIL)) != 0)
   {
      if (smtp_mail (stream,"MAIL",env,body))
         LOGMESSAGE((LOG_DEFAULT,"SMTP: MAIL [Ok]"));
      else
      {
         sprintf (tmpbuf, "[Failed - %s]",stream->reply);
         ERRORMESSAGE((tmpbuf));
      }
   }

   if (stream)
      smtp_close (stream);
   else
      ERRORMESSAGE (("[Can't open connection to any server]"));

   for(i = headerList.begin(), j = 0; i != headerList.end(); i++, j++)
      delete [] headerNames;
   delete [] headerNames;
   delete [] headerValues;
   delete [] headers;
}

SendMessageCC::~SendMessageCC()
{
   mail_free_envelope (&env);
   mail_free_body (&body);
}
