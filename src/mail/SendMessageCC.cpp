/*-*- c++ -*-********************************************************
 * SendMessageCC class: c-client implementation of a compose message*
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$ *
 *                                                                  *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "SendMessageCC.h"
#endif

#include "Mpch.h"
#include "Mcommon.h"

#ifndef  USE_PCH
#   include "Profile.h"
#   include "strutil.h"
#   include "strings.h"
#   include "MDialogs.h"

#   include <strings.h>


#endif // USE_PCH

#include   "Mdefaults.h"
#include   "MApplication.h"
#include   "Message.h"
#include   "MailFolderCC.h"
#include   "SendMessageCC.h"
#include   "XFace.h"
#include   "gui/wxIconManager.h"

extern "C"
{
#  include <misc.h>
   
   void rfc822_setextraheaders(const char **names, const char **values);
}

#define  CPYSTR(x)   cpystr(x)

#define   StringCast(iterator)   ((String *)*i)

/// temporary buffer for storing message headers, be generous:
#define   HEADERBUFFERSIZE 100*1024

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
   profile->IncRef(); // make sure it doesn't go away
   m_headerNames = NULL;
   m_headerValues = NULL;
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

void
SendMessageCC::Create(ProfileBase *iprof,
                      String const &subject,
                      String const &to, String const &cc, String const &bcc)
{
   char  *tmp, *tmp2;
   String tmpstr;
   
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

   tmpstr = profile->readEntry(MP_RETURN_ADDRESS, MP_RETURN_ADDRESS_D);
   if(strutil_isempty(tmpstr))
      tmpstr = profile->readEntry(MP_USERNAME,MP_USERNAME_D);
   else
      tmpstr = strutil_before(tmpstr,'@');
   env->return_path->mailbox = CPYSTR(tmpstr);

   tmpstr = profile->readEntry(MP_RETURN_ADDRESS, MP_RETURN_ADDRESS_D);
   if(strutil_isempty(tmpstr))
      tmpstr = profile->readEntry(MP_HOSTNAME,MP_HOSTNAME_D);
   else
      tmpstr = strutil_after(tmpstr,'@');
   env->return_path->host = CPYSTR(tmpstr);

   tmpstr = to;   ExtractFccFolders(tmpstr);
   tmp = strutil_strdup(tmpstr);
   tmp2 = strutil_strdup(profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D));
   rfc822_parse_adrlist (&env->to,tmp,tmp2);
   delete [] tmp; delete [] tmp2;
  
   tmpstr = cc;   ExtractFccFolders(tmpstr);
   tmp = strutil_strdup(tmpstr);  tmp2 = strutil_strdup(profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D));
   rfc822_parse_adrlist (&env->cc,tmp,tmp2);
   delete [] tmp; delete [] tmp2;

   tmpstr = bcc;   ExtractFccFolders(tmpstr);
   tmp = strutil_strdup(tmpstr); tmp2 = strutil_strdup(profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D));
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
SendMessageCC::AddPart(Message::ContentType type,
                       const char *buf, size_t len,
                       String const &subtype,
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

   bdy = &(nextpart->body);
   
   switch(type)
   {
   case TYPEMESSAGE:
      bdy->nested.msg = mail_newmsg(); // needed to allow c-client to free 
      // it again (stupid thing, tsts)
   case TYPETEXT:
      bdy->type = type;
      bdy->subtype = (char *) fs_get(subtype.length()+1);
      strcpy(bdy->subtype,(char *)subtype.c_str());
      bdy->contents.text.data = data;
      bdy->contents.text.size = len;
      bdy->encoding = ENC8BIT;
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
   nextpart->next = mail_newbody_part();
   lastpart = nextpart;
   nextpart = nextpart->next;
   nextpart->next = NULL;

   if(plist)
   {
      MessageParameterList::iterator i;
      PARAMETER *lastpar = NULL, *par;
      
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
   
   // notice that we _must_ assign the result to this string!
   String host = READ_CONFIG(profile, MP_SMTPHOST);
   hostlist[0] = host;
   hostlist[1] = NIL;

   if ((stream = smtp_open ((char **)hostlist,NIL)) != 0)
   {
      if (smtp_mail (stream,"MAIL",env,body))
         LOGMESSAGE((M_LOG_DEFAULT,"SMTP: MAIL [Ok]"));
      else
      {
         sprintf (tmpbuf, "[Failed - %s]",stream->reply);
         ERRORMESSAGE((tmpbuf));
         success = false;
      }
   }

   if (stream)
      smtp_close (stream);
   else
   {
      ERRORMESSAGE (("[Can't open connection to any server]"));
      success = false;
   }
   return success;
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
      i;

   if(m_headerNames != NULL) // message was already build
      return;
   
   headers = strutil_strdup(READ_CONFIG(profile, MP_EXTRAHEADERS));
   strutil_tokenise(headers,";",m_headerList);
   delete [] headers;

   // +4: 1 for X-Mailer, 1 for X-Face, 1 for reply to and 1 for the last NULL entry
   m_headerNames = new const char*[m_headerList.size()+2];
   m_headerValues = new const char*[m_headerList.size()+2];
   for(i = m_headerList.begin(), j = 0; i != m_headerList.end(); i++, h++)
   {
      m_headerNames[h] = strutil_strdup(StringCast(i)->c_str());
      m_headerValues[h] = strutil_strdup(profile->readEntry(StringCast(i)->c_str(),""));
   }
   //always add mailer header:
   m_headerNames[h] = strutil_strdup("X-Mailer");
   m_headerValues[h++] = strutil_strdup(String("M, ")+M_VERSION_STRING);
   //always add reply-to header:
   tmpstr = profile->readEntry(MP_RETURN_ADDRESS, MP_RETURN_ADDRESS_D);
   if(!strutil_isempty(tmpstr))
   {
      m_headerNames[h] = strutil_strdup("Reply-To");
      m_headerValues[h++] = strutil_strdup(tmpstr);
   }

#ifdef HAVE_XFACES
   // add an XFace?
   char **xpmarray = NULL;
   String xpmdata;
   if(profile->readEntry(MP_COMPOSE_USE_XFACE,MP_COMPOSE_USE_XFACE_D))
   {
      xpmarray =
         wxIconManager::LoadImage(profile->readEntry(
               MP_COMPOSE_XFACE_FILE,MP_COMPOSE_XFACE_FILE_D)); 
      if(! xpmarray)
      {
         bool found;
         PathFinder pf(READ_APPCONFIG(MP_ICONPATH), true);
         pf.AddPaths(mApplication->GetLocalDir()+"/icons", true);
         pf.AddPaths(mApplication->GetGlobalDir()+"/icons", true);
         String name = pf.FindFile("xface.xpm", &found);
         if(found)
            xpmarray = wxIconManager::LoadImage(name);
      }
      if(xpmarray)
      {
         XFace xface;
         for(j = 0; xpmarray[j]; j++)
         {
            xpmdata += xpmarray[j];
            xpmdata += '\n';
         }
         wxIconManager::FreeImage(xpmarray);
         if(xface.CreateFromXpm(xpmdata.c_str()))
         {
            m_headerNames[h] = strutil_strdup("X-Face");
            m_headerValues[h] = strutil_strdup(xface.GetHeaderLine());
            //FIXME: find more elegant solution for this (GetHeaderLine())
            if(strlen(m_headerValues[h]))  // paranoid, I know.
               ((char*)
                (m_headerValues[h]))[strlen(m_headerValues[h])-1] =
                  '\0'; // cut off \n
            h++;
         }
      }
   }
#endif

   m_headerNames[h] = NULL;
   m_headerValues[h] = NULL;
   rfc822_setextraheaders(m_headerNames,m_headerValues);

   mail_free_body_part(&lastpart->next);
   lastpart->next = NULL;
   rfc822_date (tmpbuf);
   env->date = (char *) fs_get (1+strlen (tmpbuf));
   strcpy (env->date,tmpbuf);
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

   if(! rfc822_output(buffer, env, body, write_str_output,&output,NIL))
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
      
   if(! rfc822_output(buffer, env, body, write_output,ostr,NIL))
      ERRORMESSAGE (("[Can't write message to file %s]",
                     filename.c_str()));
   delete [] buffer;
   delete ostr;
}

void
SendMessageCC::WriteToFolder(String const &name)
{
   Build();

   String str;

   WriteToString(str);
   MailFolder *mf =
      MailFolder::OpenFolder(MailFolder::MF_PROFILE,name);
   if(! mf) // there is no profile folder called name
   {
      String filename = strutil_expandfoldername(name);
      mf = MailFolder::OpenFolder(MailFolder::MF_PROFILE,name);
   }
   CHECK_RET(mf,String(_("Cannot open folder to save to:")+name));
   mf->AppendMessage(str);
   mf->DecRef();
}

SendMessageCC::~SendMessageCC()
{
   mail_free_envelope (&env);
   mail_free_body (&body);

   int j;
   kbStringList::iterator i;
   for(i = m_headerList.begin(), j = 0; i != m_headerList.end(); i++,
          j++)
   {
      // @ const cast
      delete [] (char *)m_headerNames[j];
   }

   if(m_headerNames)  delete [] m_headerNames;
   if(m_headerValues) delete [] m_headerValues;

   profile->DecRef();
}
