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
MailFolder::OpenFolder(int typeAndFlags,
                       String const &i_name,
                       ProfileBase *parentProfile,
                       String const &i_server,
                       String const &i_login,
                       String const &i_passwd)
{
   // open a folder:
   ProfileBase *profile = NULL;
   String login, passwd, name, server;
   FolderType type = GetFolderType(typeAndFlags);
   int flags = GetFolderFlags(typeAndFlags);
   
   if ( type == MF_PROFILE || type == MF_PROFILE_OR_FILE )
   {
      profile = ProfileBase::CreateFolderProfile(i_name, parentProfile);
      CHECK(profile, NULL, "can't create profile");   // return if it fails
      int typeflags = READ_CONFIG(profile, MP_FOLDER_TYPE);
      if(typeflags == MF_ILLEGAL)  // no such profile!
      {
         type = MF_FILE;
         flags = 0;
         name = i_name;
      }
      else
      {
         type = GetFolderType(typeflags);
         flags = GetFolderFlags(typeflags);
         login = READ_CONFIG(profile, MP_POP_LOGIN);
         passwd = READ_CONFIG(profile, MP_POP_PASSWORD);
         name = READ_CONFIG(profile, MP_FOLDER_PATH);
      }
   }
   else // type != PROFILE
   {
      profile = ProfileBase::CreateEmptyProfile(parentProfile);
      CHECK(profile, NULL, "can't create profile");   // return if it fails
      
      server = i_server;
      name = i_name;
      passwd = i_passwd;
      login = i_login;
   }
   
   switch( type )
   {
      case MF_NNTP:
         if(strutil_isempty(i_server))
            server = READ_CONFIG(profile, MP_NNTPHOST);
         break;

      case MF_FILE:
      case MF_MH:
         if( strutil_isempty(name) )
            name = READ_CONFIG(profile, MP_FOLDER_PATH);
         if(name == "INBOX")
            type = MF_INBOX;
         name = strutil_expandfoldername(name);
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

      case MF_INBOX:
         // nothing special to do
         break;

      default:
         FAIL_MSG("unknown folder type");
   }

   // FIXME calling MailFolderCC::OpenFolder() explicitly here is "anti-OO"
   typeAndFlags = CombineFolderTypeAndFlags(type, flags);

   MailFolder *mf = MailFolderCC::OpenFolder(typeAndFlags, name, profile,
                                             server, login, passwd);
   if ( mf )
      mf->m_UpdateInterval = READ_CONFIG(profile, MP_UPDATEINTERVAL);

   profile->DecRef();

   return mf;
}

/* static */ String 
MailFolder::ConvertMessageStatusToString(MessageStatus status)
{
   String strstatus = "";
   
   strstatus = "";
   if(status & MSG_STAT_RECENT)
   {
      if(status & MSG_STAT_SEEN)
         strstatus << 'U'; // seen but not read 
      else
         strstatus << 'N'; // not seen yet
   }
   else
      strstatus << ' ';
   strstatus << ((status & MSG_STAT_FLAGGED) ? 'F' : ' ');
   strstatus << ((status & MSG_STAT_ANSWERED) ? 'A' : ' ');
   strstatus << ((status & MSG_STAT_DELETED) ? 'D' : ' ');

   return strstatus;
}
