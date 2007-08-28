//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MFPool.cpp: MFPool class implementation
// Purpose:     MFPool manages a pool of MailFolders
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.07.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "lists.h"

#include "MFolder.h"
#include "mail/Driver.h"
#include "mail/FolderPool.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// our debugging trace mask
#define TRACE_MFPOOL _T("mfpool")

// ----------------------------------------------------------------------------
// MFConnection caches information about a single connection
// ----------------------------------------------------------------------------

// a cached folder connection
struct MFConnection
{
   // the (opened) mail folder
   MailFolder *mf;

   // the full folder spec as returned by MFDriver::GetFullSpec()
   String spec;

   // also cache the MFolder which can be used to (re)open this mf later
   MFolder *folder;


   MFConnection(MailFolder *mf_, const String& spec_, const MFolder *folder_)
      : spec(spec_)
   {
      mf = mf_;
      folder = const_cast<MFolder *>(folder_);
      if ( folder )
         folder->IncRef();
   }

   ~MFConnection()
   {
      if ( folder )
         folder->DecRef();
   }
};

M_LIST_OWN(MFConnectionList, MFConnection);

// ----------------------------------------------------------------------------
// MFClassPool caches information about all connections for the given driver
// ----------------------------------------------------------------------------

// the pool of folders of one class
struct MFClassPool
{
   // ctor for a new pool corresponding to the driver with the given name
   MFClassPool(const String& driverName_) : driverName(driverName_) { }

   // find the class pool for the given driver name
   //
   // returns NULL if not found
   static MFClassPool *Find(const String& driverName);

   // find the connection with the given spec in this pool
   //
   // returns NULL if not found
   MFConnection *FindConnection(const String& spec) const;


   const String driverName;
   MFConnectionList connections;


   // no assignment operator because driverName is const
   MFClassPool& operator=(const MFClassPool&);
};

// ----------------------------------------------------------------------------
// global module variables
// ----------------------------------------------------------------------------

// as MFPool is a singleton class we use the module globals instead of the
// member static variables to reduce compilation dependencies

// the global pool is a linked list of class pools
M_LIST_OWN(MFClassPoolList, MFClassPool) gs_pool;

// ----------------------------------------------------------------------------
// Cookie: used to store state information by the iteration functions
// ----------------------------------------------------------------------------

class CookieImpl
{
public:
   void Reset() { SetPool(gs_pool.begin()); }

   MailFolder *GetAndAdvance(String *driverName, MFolder **pFolder)
   {
      if ( m_iterPool == gs_pool.end() )
         return NULL;

      if ( m_iterConn == m_iterPool->connections.end() )
      {
         SetPool(++m_iterPool);

         return GetAndAdvance(driverName, pFolder);
      }

      MailFolder *mf = m_iterConn->mf;
      if ( pFolder )
      {
         *pFolder = m_iterConn->folder;
         (*pFolder)->IncRef();
      }

      ++m_iterConn;

      CHECK( mf, NULL, _T("NULL mailfolder in MFPool?") );

      if ( driverName )
      {
         *driverName = m_iterPool->driverName;
      }

      mf->IncRef();
      return mf;
   }

private:
   void SetPool(MFClassPoolList::iterator i)
   {
      m_iterPool = i;

      if ( i != gs_pool.end() )
      {
         m_iterConn = i->connections.begin();
      }
   }

   MFClassPoolList::iterator m_iterPool;
   MFConnectionList::iterator m_iterConn;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MFClassPool
// ----------------------------------------------------------------------------

MFClassPool *MFClassPool::Find(const String& driverName)
{
   for ( MFClassPoolList::iterator i = gs_pool.begin();
         i != gs_pool.end();
         ++i )
   {
      if ( i->driverName == driverName )
         return *i;
   }

   return NULL;
}

MFConnection *MFClassPool::FindConnection(const String& spec) const
{
   for ( MFConnectionList::iterator i = connections.begin();
         i != connections.end();
         ++i )
   {
      if ( i->spec == spec )
         return *i;
   }

   return NULL;
}

// ----------------------------------------------------------------------------
// MFPool::Cookie
// ----------------------------------------------------------------------------

MFPool::Cookie::Cookie()
{
   m_impl = new CookieImpl;
}

MFPool::Cookie::~Cookie()
{
   delete m_impl;
}

// ----------------------------------------------------------------------------
// MFPool operations
// ----------------------------------------------------------------------------

/* static */
void
MFPool::Add(MFDriver *driver,
            MailFolder *mf,
            const MFolder *folder,
            const String& login)
{
   CHECK_RET( driver, _T("MFPool::Add(): NULL driver") );

   const String driverName = driver->GetName();

   MFClassPool *pool = MFClassPool::Find(driverName);
   if ( !pool )
   {
      // create new class pool
      pool = new MFClassPool(driverName);

      gs_pool.push_back(pool);
   }

   const String spec = driver->GetFullSpec(folder, login);

   MFConnection *conn = pool->FindConnection(spec);
   CHECK_RET( !conn, _T("MFPool::Add(): folder already in the pool") );

   pool->connections.push_back(new MFConnection(mf, spec, folder));

   wxLogTrace(TRACE_MFPOOL, _T("Added '%s' to the pool."), mf->GetName().c_str());
}

/* static */
MailFolder *
MFPool::Find(MFDriver *driver,
             const MFolder *folder,
             const String& login)
{
   CHECK( driver, NULL, _T("MFPool::Find(): NULL driver") );

   MFClassPool *pool = MFClassPool::Find(driver->GetName());
   if ( !pool )
   {
      // no cached folders of this class at all
      return NULL;
   }

   MFConnection * const
      conn = pool->FindConnection(driver->GetFullSpec(folder, login));

   if ( !conn )
      return NULL;

   MailFolder *mf = conn->mf;
   CHECK( mf, NULL, _T("NULL mailfolder in MFPool?") );

   mf->IncRef();
   return mf;
}

/* static */
bool MFPool::Remove(MailFolder *mf)
{
   for ( MFClassPoolList::iterator pool = gs_pool.begin();
         pool != gs_pool.end();
         ++pool )
   {
      for ( MFConnectionList::iterator i = pool->connections.begin();
            i != pool->connections.end();
            ++i )
      {
         if ( i->mf == mf )
         {
            wxLogTrace(TRACE_MFPOOL, _T("Removing '%s' from the pool."),
                       mf->GetName().c_str());

            pool->connections.erase(i);

            // there can be only one node containing this folder so stop here
            return true;
         }
      }
   }

   return false;
}

/* static */
void MFPool::DeleteAll()
{
   wxLogTrace(TRACE_MFPOOL, _T("Clearing the pool."));

   for ( MFClassPoolList::iterator pool = gs_pool.begin();
         pool != gs_pool.end();
         ++pool )
   {
      pool->connections.clear();
   }

   gs_pool.clear();
}

// ----------------------------------------------------------------------------
// MFPool iteration
// ----------------------------------------------------------------------------

/* static */
MailFolder *
MFPool::GetFirst(Cookie& cookie, String *driverName, MFolder **pFolder)
{
   cookie.m_impl->Reset();

   return GetNext(cookie, driverName, pFolder);
}

/* static */
MailFolder *
MFPool::GetNext(Cookie& cookie, String *driverName, MFolder **pFolder)
{
   return cookie.m_impl->GetAndAdvance(driverName, pFolder);
}

