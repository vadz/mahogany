#include  "Mcommon.h"

#if       !USE_PCH
  #include	<Profile.h>
  #include	<Mdefaults.h>
  #include	<strutil.h>
  #include	<wx.h>
#endif

// defined in windows.h
#undef  SendMessage

class SendMessageCC
{
   Profile 	*profile;

   ENVELOPE	*env;
   BODY		*body;
public:
   SendMessageCC(Profile *iprof = NULL);
   void Create(void);

   inline Profile *GetProfile(void) { return profile; }
};


#define	CPYSTR(x)	cpystr((char *)(x.c_str()))

SendMessageCC::SendMessage(Profile *iprof)
{
   Create(iprof);
}

void
SendMessageCC::Create(Profile *iprof,
		      String const &subject,
		      String const &to, String const &cc, String const &bcc)
{
   char	*tmp;
   char tmpbuf[MAILTMPLEN];
   SENDSTREAM *stream = NIL;
   
   profile = iprof;

   
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
  tmp = strutil_strdup(to);
  rfc822_parse_adrlist (&env->to,tmp,profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D).c_str());
  delete [] tmp;
  tmp = strutil_strdup(cc);
  rfc822_parse_adrlist (&env->cc,tmp,profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D).c_str());
  delete [] tmp;
  tmp = strutil_strdup(bcc);
  rfc822_parse_adrlist (&env->bcc,tmp,profile->readEntry(MP_HOSTNAME, MP_HOSTNAME_D).c_str());
  delete [] tmp;

  env->subject = CPYSTR(subject);
  body->type = TYPETEXT;

  char text = "DEMO CONTENT FOR A MAIL MESSAGE\n2nd Line\n\r\n";
  
  //  strcat (text,"\r\n");	//\015\012
  
  body->contents.text.data = text;
  body->contents.text.size = strlen (text);
  rfc822_date (tmpbuf);
  env->date = (char *) fs_get (1+strlen (tmpbuf));
  strcpy (env->date,tmpbuf);

  if (stream = smtp_open (profile->readEntry(MP_SMTPHOST,
					     MP_SMTPHOST_D).c_str(),0))
  {
     if (smtp_mail (stream,"MAIL",env,body))
	puts ("[Ok]");
     else
	printf ("[Failed - %s]\n",stream->reply);
    }

  if (stream)
     smtp_close (stream);
  else
     puts ("[Can't open connection to any server]");
  mail_free_envelope (&env);
  mail_free_body (&body);
}
