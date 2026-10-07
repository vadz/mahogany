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

#include "MFolder.h"
#include "mail/Driver.h"
#include "mail/FolderPool.h"

#include "pointers.h"

#include <list>

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
   RefCounter<MFolder> folder;


   MFConnection(MailFolder *mf_, const String& spec_, const MFolder *folder_)
      : spec(spec_),
        folder(RefCounter<MFolder>::convert(const_cast<MFolder *>(folder_)))
   {
      mf = mf_;
   }
};

// Note that we must use std::list and not std::vector here (and for
// MFClassPoolList below) because CookieImpl keeps iterators into these lists
// while the code iterating over the pool runs and this code can add new
// connections to the pool or remove the existing ones from it (e.g. closing
// the folder removes it from the pool), and the iterators must remain valid.
using MFConnectionList = std::list<MFConnection>;

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
   MFConnection *FindConnection(const String& spec);


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
using MFClassPoolList = std::list<MFClassPool>;

MFClassPoolList gs_pool;

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
         return nullptr;

      if ( m_iterConn == m_iterPool->connections.end() )
      {
         SetPool(++m_iterPool);

         return GetAndAdvance(driverName, pFolder);
      }

      MailFolder *mf = m_iterConn->mf;
      if ( pFolder )
      {
         *pFolder = m_iterConn->folder.get();
         (*pFolder)->IncRef();
      }

      ++m_iterConn;

      CHECK( mf, nullptr, _T("NULL mailfolder in MFPool?") );

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
   for ( MFClassPool& pool : gs_pool )
   {
      if ( pool.driverName == driverName )
         return &pool;
   }

   return nullptr;
}

MFConnection *MFClassPool::FindConnection(const String& spec)
{
   for ( MFConnection& conn : connections )
   {
      if ( conn.spec == spec )
         return &conn;
   }

   return nullptr;
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
      pool = &gs_pool.emplace_back(driverName);
   }

   const String spec = driver->GetFullSpec(folder, login);

   MFConnection *conn = pool->FindConnection(spec);
   CHECK_RET( !conn, _T("MFPool::Add(): folder already in the pool") );

   pool->connections.emplace_back(mf, spec, folder);

   wxLogTrace(TRACE_MFPOOL, _T("Added '%s' to the pool."), mf->GetName());
}

/* static */
MailFolder *
MFPool::Find(MFDriver *driver,
             const MFolder *folder,
             const String& login)
{
   CHECK( driver, nullptr, _T("MFPool::Find(): NULL driver") );

   MFClassPool *pool = MFClassPool::Find(driver->GetName());
   if ( !pool )
   {
      // no cached folders of this class at all
      return nullptr;
   }

   MFConnection * const
      conn = pool->FindConnection(driver->GetFullSpec(folder, login));

   if ( !conn )
      return nullptr;

   MailFolder *mf = conn->mf;
   CHECK( mf, nullptr, _T("NULL mailfolder in MFPool?") );

   mf->IncRef();
   return mf;
}

/* static */
bool MFPool::Remove(MailFolder *mf)
{
   for ( MFClassPool& pool : gs_pool )
   {
      for ( auto i = pool.connections.begin();
            i != pool.connections.end();
            ++i )
      {
         if ( i->mf == mf )
         {
            wxLogTrace(TRACE_MFPOOL, _T("Removing '%s' from the pool."),
                       mf->GetName());

            pool.connections.erase(i);

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

