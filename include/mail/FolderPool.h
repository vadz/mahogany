//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/FolderPool.h: MFPool class declaration
// Purpose:     MFPool manages a pool of MailFolders
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.07.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAIL_FOLDERPOOL_H_
#define _MAIL_FOLDERPOOL_H_

class MailFolder;
class MFolder;
class MFDriver;

// ----------------------------------------------------------------------------
// MFPool class is used to cache the opened mail folders
// ----------------------------------------------------------------------------

/**
  When a folder is opened, it is added to the pool and, even before trying to
  open it, OpenFolder() checks if the folder isn't already in the pool -- if it
  is, it is simply returned.

  A folder in the pool is identified by the associated MFolder (which stores
  such information as the folder class ("cclient", "virtual", ...), path &c)
  and the login name. The last one is important as we could have different
  accounts on the same server. It also means that we have to pass the login
  name separately around as the user may have chosen to not save it in the
  profile and hence MFolder might not have it.

  Final remark: this is, in fact, a namespace and not a class: all methods are
  static.
 */
class MFPool
{
public:
   /**
     @name Usual operations

     Adding, removing and looking for folders in the pool
    */
   //@{

   /**
     Add a new (opened) folder to the pool. This function doesn't change the
     ref count of mf.

     @param mf the opened folder
     @param driver the driver to use for mf
     @param folder the folder parameters
     @param login the username to use
    */
   static void Add(MFDriver *driver,
                   MailFolder *mf,
                   const MFolder *folder,
                   const String& login);

   /**
     Find a folder in the pool. If the return value is not NULL it must be
     DecRef()'d by the caller.

     We ignore the login parameter if it is empty but using this "feature" is
     just a sign of bad design and all occurrences of this should go away:
     unfortunately, currently we don't keep the login used to open the folder
     anywhere so when we want to close the folder later, for example, we don't
     have the login any more and our only hope is that we have only one folder
     on this server opened -- otherwise we can close the wrong one! (FIXME)

     @param driver the driver to use
     @param folder the folder parameters
     @param login the username to use or empty to not check it
     @return a IncRef()'d pointer to an opened MailFolder or NULL if not found
    */
   static MailFolder *Find(MFDriver *driver,
                           const MFolder *folder,
                           const String& login);

   /**
     Remove the folder from the pool, presumably just before or immediately
     after closing it.

     @param mf pointer to the folder to remove
     @return true if the folder was removed, false if it wasn't found
    */
   static bool Remove(MailFolder *mf);

   /**
     Remove all still opened folders from the pool.
    */
   static void DeleteAll();

   //@}

   /**
     @name Iterating over the pool

     To iterate over all opened folders you have to pass a cookie to MFPool and
     call GetFirst/Next() until they return NULL
    */
   //@{

   /// opaque data type used only with GetFirst() and GetNext()
   class Cookie
   {
   public:
      Cookie();
      ~Cookie();

   private:
      class CookieImpl *m_impl;

      friend class MFPool;
   };

   /**
     Returns the first opened folder and optionally the driver used for it

     @param cookie should be passed to GetNext() later
     @param driverName the name of the driver used for this folder, may be NULL
     @param pFolder filled in with the MFolder originally specified in Add()
                    if non-NULL; must be DecRef()'d by caller
     @return IncRef()'d pointer to an opened folder or NULL if none
    */
   static MailFolder *GetFirst(Cookie& cookie,
                               String *driverName = NULL,
                               MFolder **pFolder = NULL);

   /**
     Returns the next opened folder

     @param cookie the same one as used with GetFirst()
     @param driverName the name of the driver used for this folder, may be NULL
     @param pFolder same meaning as for GetFirst()
     @return IncRef()'d pointer to an opened folder or NULL if no more
    */
   static MailFolder *GetNext(Cookie& cookie,
                              String *driverName = NULL,
                              MFolder **pFolder = NULL);

   //@}
};

#endif // _MAIL_FOLDERPOOL_H_

