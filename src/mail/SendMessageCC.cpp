/*-*- c++ -*-********************************************************
 * SendMessageCC class: c-client implementation of a compose message*
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$ *
 *                                                                  *
 * $Log$
 * Revision 1.9  1998/06/14 12:24:27  KB
 * started to move wxFolderView to be a panel, Python improvements
 *
 * Revision 1.8  1998/06/05 16:56:34  VZ
 *
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.7  1998/05/24 08:23:31  KB
 * changed the creation/destruction of MailFolders, now done through
 * MailFolder::Open/CloseFolder, made constructor/destructor private,
 * this allows multiple view on the same folder
 *
 * Revision 1.6  1998/05/18 17:48:49  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 * *
 *                                                                  *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "SendMessageCC.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#ifndef  USE_PCH
#  include "Profile.h"
#  include "strutil.h"
#  include "strings.h"
#  include "MDialogs.h"

#  include <strings.h>

   // includes for c-client library
   extern "C"
   {
#     include <stdio.h>
#     include <mail.h>      
#     include <osdep.h>
#     include <rfc822.h>
#     include <smtp.h>
#     include <nntp.h>
#     include <misc.h>
   }

#endif // USE_PCH

#include "Mdefaults.h"
#include "MApplication.h"
#include "Message.h"
#include "SendMessageCC.h"

extern "C"
{
#  include <misc.h>
   
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
   kbStringList
      headerList;
   kbStringList::iterator
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
         LOGMESSAGE((M_LOG_DEFAULT,"SMTP: MAIL [Ok]"));
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

   for(i = headerList.begin(), j = 0; i != headerList.end(); i++, j++) {
      // @ const cast
      delete [] (char *)headerNames[j];
   }

   delete [] headerNames;
   delete [] headerValues;
   delete [] headers;
}

SendMessageCC::~SendMessageCC()
{
   mail_free_envelope (&env);
   mail_free_body (&body);
}
