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
#   include   "Mdefaults.h"
#   include   "strutil.h"
#   include   "Profile.h"
#   include   "MailFolder.h"
#   include   "MailFolderCC.h"
#endif


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
      profile = ProfileBase::CreateProfile(name,parentProfile);
      ASSERT(profile);
      login = READ_CONFIG(profile, MP_POP_LOGIN);
      passwd = READ_CONFIG(profile, MP_POP_PASSWORD);
      type = (MailFolder::Type)READ_CONFIG(profile, MP_FOLDER_TYPE);
      name = READ_CONFIG(profile, MP_FOLDER_PATH);
      if(strutil_isempty(name))
      {
         name = i_name;
         if(name == "INBOX")
            type = MF_INBOX;
         else
            type = (MailFolder::Type) MP_FOLDER_TYPE_D;
      }
   }
   else
   {
      profile = ProfileBase::CreateEmptyProfile(parentProfile);
      ASSERT(profile);
      login = i_login;
      passwd = i_passwd;
      type = i_type;
      name = i_name;
   }
   MailFolder *mf = MailFolderCC::OpenFolder(type, name, profile, login, passwd);
   mf->m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);
   profile->DecRef();
   return mf;
}
