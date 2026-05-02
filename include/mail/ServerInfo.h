//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/ServerInfo.h: ServerInfoEntry class
// Purpose:     ServerInfoEntry is used for server management by MailFolder
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.07.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAIL_SERVERINFO_H
#define _MAIL_SERVERINFO_H

#include "lists.h"

#include "MFolder.h"
#include "MailFolder.h"

// put this in the trace mask to get messages about connection caching
#define TRACE_SERVER_CACHE  _T("servercache")

/**
   Whenever we need to establish a connection with a remote server, we try to
   reuse an existing ServerInfoEntry for it (and create a new one if none was
   found). This allows us to reuse the authentication information and the
   network connections.

   For the former, it's simple: we remember the name and the password after
   connection to the server successfully and use them for any new connections.
   This allows the users who don't want to store their logins/passwords in the
   profile to only enter them once and they will be reused until the end of the
   session (notice that they may explicitly choose to not store sensitive
   information even in memory -- then this isn't done).

   For the latter, we do the following (this part actually happens only in
   ServerInfoEntryCC which is the only implementation caching network
   connections for now):

   a) when opening a folder, check if we have an existing connection to its
      server and use it instead of NULL when calling mail_open()

   b) when closing the folder, don't close the connection with mail_close()
      but give it back to the folders server entry, it will close it later (if
      it's not reused)
 */

class ServerInfoEntry
{
public:
   /**
     @name Server managing

     Functions for iterating over the entire list of servers, finding the
     servers in it and adding the new ones.
    */
   //@{

   /**
     Find the server entry for the specified folder and return it or NULL if
     not found.

     @param folder the folder to find the server entry for
    */
   static ServerInfoEntry *Get(const MFolder *folder)
   {
      // look among the existing ones
      for ( ServerInfoList::iterator i = ms_servers.begin();
            i != ms_servers.end();
            ++i )
      {
         if ( i->CanBeUsedFor(folder) )
         {
            wxLogTrace(TRACE_SERVER_CACHE,
                       _T("Reusing existing server entry for %s(%s)."),
                       folder->GetFullName(), i->m_login);

            // found
            return *i;
         }
      }

      // not found
      wxLogTrace(TRACE_SERVER_CACHE,
                 _T("No server entry for %s found."),
                 folder->GetFullName());

      return NULL;
   }

   /**
     Find the server entry for the specified folder creating one if we hadn't
     had it yet and return it.

     Unlike Get(), this method only returns NULL if folder doesn't refer to a
     remote mailbox which is an error in the caller.

     @param folder the folder to find the server entry for
     @param mf the opened folder used for creating a new entry, if necessary
    */
   static ServerInfoEntry *GetOrCreate(const MFolder *folder, MailFolder *mf)
   {
      ServerInfoEntry *serverInfo = Get(folder);
      if ( !serverInfo )
      {
         wxLogTrace(TRACE_SERVER_CACHE,
                    _T("Creating new server entry for %s(%s)."),
                    folder->GetFullName(), folder->GetLogin());

         serverInfo = mf->CreateServerInfo(folder);

         CHECK( serverInfo, NULL, _T("CreateServerInfo() failed?") );

         ms_servers.push_back(serverInfo);
      }

      return serverInfo;
   }

   /// delete all server entries
   static void DeleteAll()
   {
      ms_servers.clear();
   }

   //@}


   /**
     @name Auth info

     We keep the account information for this server so that normally we don't
     ask the user more than once about it and thus SetAuthInfo() is supposed to
     be called only once while GetAuthInfo() may be called many times after it.
    */
   //@{

   /// set the login and password to use with this server
   void SetAuthInfo(const String& login, const String& password)
   {
      CHECK_RET( !login.empty(), _T("empty login not allowed") );

      ASSERT_MSG( !m_hasAuthInfo, _T("overriding auth info for the server?") );

      m_hasAuthInfo = true;
      m_login = login;
      m_password = password;
   }

   /**
     Retrieve the auth info for the server in the provided variables. If it
     returns false the values of login and password are not modified.

     @param login receives the user name
     @param password receives the password
     @return false if won't have auth info, true if ok
    */
   bool GetAuthInfo(String& login, String& password) const
   {
      if ( !m_hasAuthInfo )
         return false;

      login = m_login;
      password = m_password;

      return true;
   }

   //@}

   /** @name Connection pooling */
   //@{

   /**
     Close the cached connections for which the timeout has elapsed

     @return true if there any cached connections left
    */
   virtual bool CheckTimeout() { return false; }

   /**
     Return true if this server can be used to connect to the given folder
     (by default: no connection caching)
    */
   virtual bool CanBeUsedFor(const MFolder * /* folder */) const
      { return false; }

   //@}


   // dtor must be public in order to use M_LIST_OWN() but nobody should delete
   // us directly!
   virtual ~ServerInfoEntry()
   {
   }

protected:
   // ctor is private, nobody except GetOrCreate() can create us
   ServerInfoEntry(const MFolder *folder)
   {
      m_login = folder->GetLogin();
      m_password = folder->GetPassword();

      m_hasAuthInfo = !m_login.empty() && !m_password.empty();

      wxLogTrace(TRACE_SERVER_CACHE,
                 _T("Created server entry for %s(%s)."),
                 folder->GetFullName(), m_login);
   }

   // login and password for this server if we need it and if we're allowed to
   // store it
   String m_login,
          m_password;

   // if true, we have valid m_login and m_password
   //
   // note that we need this flag because in principle empty passwords are
   // allowed and login could be non empty because it is stored in the config
   // file
   bool m_hasAuthInfo;

   // the list of all servers
   M_LIST_OWN(ServerInfoList, ServerInfoEntry);
   static ServerInfoList ms_servers;

   GCC_DTOR_WARN_OFF
};

#endif // _MAIL_SERVERINFO_H

