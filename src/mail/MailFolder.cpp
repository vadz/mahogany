/*-*- c++ -*-********************************************************
 * MailFolder class: handling of Unix mail folders                  *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "MailFolder.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
#   include  "Mcommon.h"
#   include  "strutil.h"
#   include  "Profile.h"
#endif

#include  "Mdefaults.h"

#include  "MailFolder.h"
#include  "MailFolderCC.h"

MailFolder *
MailFolder::OpenFolder(MailFolder::Type i_type,
                       String const &i_name,
                       ProfileBase *parentProfile,
                       String const &i_server,
                       String const &i_login,
                       String const &i_passwd)
{
   // open a folder:
   ProfileBase *profile = NULL;
   String login, passwd, name, server;
   MailFolder::Type type;

   
   if(i_type == MF_PROFILE)
   {
      profile = ProfileBase::CreateFolderProfile(i_name, parentProfile);
      CHECK(profile, NULL, "can't create profile");   // return if it fails
      login = READ_CONFIG(profile, MP_POP_LOGIN);
      passwd = READ_CONFIG(profile, MP_POP_PASSWORD);
      type = (MailFolder::Type)READ_CONFIG(profile, MP_FOLDER_TYPE);
      name = READ_CONFIG(profile, MP_FOLDER_PATH);
   }
   else // type != PROFILE
   {
      profile = ProfileBase::CreateEmptyProfile(parentProfile);
      CHECK(profile, NULL, "can't create profile");   // return if it fails
      
      server = i_server;
      name = i_name;
      passwd = i_passwd;
      login = i_login;
      type = i_type;
   }
   
   switch(type)
   {
   case MF_NNTP:
      if(strutil_isempty(i_server))
         server = READ_CONFIG(profile, MP_NNTPHOST);
      // FIXME this login is, in fact, a newsgroup name - which is
      // stored in folder path config entry. Clear, eh?
      break;
   case MF_FILE:
      if( strutil_isempty(name) )
      {
         name = READ_CONFIG(profile, MP_FOLDER_PATH);
         if(name == "INBOX")
            type = MF_INBOX;
         else
            type = (MailFolder::Type) MP_FOLDER_TYPE_D;
      }
      break;
   case MF_POP:
   case MF_IMAP:
      if(strutil_isempty(server))
         server = READ_CONFIG(profile, MP_POP_HOST);
      if(strutil_isempty(login))
         login = READ_CONFIG(profile, MP_POP_LOGIN);
      if(strutil_isempty(passwd))
         passwd = READ_CONFIG(profile, MP_POP_PASSWORD);
      if(strutil_isempty(name))
         name = READ_CONFIG(profile, MP_FOLDER_PATH);
      break;
   default:
      // not sure about others, but fall through for now
      ;
   }

   // @@ calling MailFolderCC::OpenFolder() explicitly here is "anti-OO"
   MailFolder *mf = MailFolderCC::OpenFolder(type, name, profile,
                                             server, login, passwd);
   if ( mf )
      mf->m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);

   profile->DecRef();

   return mf;
}
