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
                       String const &i_login,
                       String const &i_passwd)
{
   // open a folder:
   ProfileBase *profile = NULL;
   String login, passwd, name;
   MailFolder::Type type;

   if(i_type == MF_PROFILE)
   {
      String profileName;
      if ( !i_name.IsEmpty() && i_name[0u] == '/' )
         profileName = i_name.c_str() + 1;
      else
         profileName = i_name;

      profile = ProfileBase::CreateFolderProfile(profileName,parentProfile);
      CHECK(profile, NULL, "can't create profile");   // return if it fails

      login = READ_CONFIG(profile, MP_POP_LOGIN);
      passwd = READ_CONFIG(profile, MP_POP_PASSWORD);
      type = (MailFolder::Type)READ_CONFIG(profile, MP_FOLDER_TYPE);

      switch ( type )
      {
      case MF_NNTP:
         name = READ_CONFIG(profile, MP_NNTPHOST);
         // FIXME this login is, in fact, a newsgroup name - which is
         //       stored in folder path config entry. Clear, eh?
         login = READ_CONFIG(profile, MP_FOLDER_PATH);
         break;
      case MF_FILE:
         name = READ_CONFIG(profile, MP_FOLDER_PATH);
         if( strutil_isempty(name) )
         {
            name = profileName; // i_name is wrong here because we
                                // need a relative path
            if(name == "INBOX")
               type = MF_INBOX;
            else
               type = (MailFolder::Type) MP_FOLDER_TYPE_D;
         }
         break;

      case MF_POP:
      case MF_IMAP:
         name = READ_CONFIG(profile, MP_POP_HOST);
         if ( strutil_isempty(name) )
            name = "localhost";
         break;
      default:
         // not sure about others, but fall through for now
         ;
      }
   }
   else // type != PROFILE
   {
      profile = ProfileBase::CreateEmptyProfile(parentProfile);
      CHECK(profile, NULL, "can't create profile");   // return if it fails

      login = i_login;
      passwd = i_passwd;
      type = i_type;
      name = i_name;
   }

   // @@ calling MailFolderCC::OpenFolder() explicitly here is "anti-OO"
   MailFolder *mf = MailFolderCC::OpenFolder(type, name, profile, login, passwd);
   if ( mf )
      mf->m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);

   profile->DecRef();

   return mf;
}
