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

#ifdef __GNUG__
   #pragma implementation "MFPool.h"
#endif

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "lists.h"

#include "mail/Driver.h"
#include "mail/FolderPool.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// our debugging trace mask
#define TRACE_MFPOOL "mfpool"

// ----------------------------------------------------------------------------
// global module variables
// ----------------------------------------------------------------------------

// as MFPool is a singleton class we use the module globals instead of the
// member static variables to reduce compilation dependencies

// a cached folder connection
struct MFConnection
{
   // the (opened) mail folder
   MailFolder *mf;

   // the full folder spec as returned by MFDriver::GetFullSpec()
   String spec;

   MFConnection(MailFolder *mf_, const String& s) : spec(s) { mf = mf_; }
};

M_LIST_OWN(MFConnectionList, MFConnection);

// the pool of folders of one class
struct MFClassPool
{
   String driverName;
   MFConnectionList connections;

   MFClassPool(const String& kind_) : driverName(kind_) { }
};

// the global pool is a linked list of class pools
M_LIST_OWN(MFClassPoolList, MFClassPool) gs_pool;

// ----------------------------------------------------------------------------
// Cookie: used to store state information by the iteration functions
// ----------------------------------------------------------------------------

class CookieImpl
{
public:
   void Reset() { SetPool(gs_pool.begin()); }

   MailFolder *GetAndAdvance(String *driverName)
   {
      if ( m_iterPool == gs_pool.end() )
         return NULL;

      if ( m_iterConn == m_iterPool->connections.end() )
      {
         SetPool(++m_iterPool);

         return GetAndAdvance(driverName);
      }

      MailFolder *mf = m_iterConn->mf;

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
// helper functions
// ----------------------------------------------------------------------------

static MFClassPool *FindClassPool(const String& driverName)
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

static
MFConnection *
FindConnectionInPool(const MFClassPool *pool,
                     const String& spec)
{
   for ( MFConnectionList::iterator i = pool->connections.begin();
         i != pool->connections.end();
         ++i )
   {
      if ( i->spec == spec )
         return *i;
   }

   return NULL;
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

   MFClassPool *pool = FindClassPool(driverName);
   if ( !pool )
   {
      // create new class pool
      pool = new MFClassPool(driverName);

      gs_pool.push_back(pool);
   }

   const String spec = driver->GetFullSpec(folder, login);

   MFConnection *conn = FindConnectionInPool(pool, spec);
   CHECK_RET( !conn, _T("MFPool::Add(): folder already in the pool") );

   pool->connections.push_back(new MFConnection(mf, spec));

   wxLogTrace(TRACE_MFPOOL, _T("Added '%s' to the pool."), mf->GetName().c_str());
}

/* static */
MailFolder *
MFPool::Find(MFDriver *driver,
             const MFolder *folder,
             const String& login)
{
   CHECK( driver, NULL, _T("MFPool::Find(): NULL driver") );

   MFClassPool *pool = FindClassPool(driver->GetName());
   if ( !pool )
   {
      // no cached folders of this class at all
      return NULL;
   }

   MFConnection *
      conn = FindConnectionInPool(pool, driver->GetFullSpec(folder, login));

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
MailFolder *MFPool::GetFirst(Cookie& cookie, String *driverName)
{
   cookie.m_impl->Reset();

   return GetNext(cookie, driverName);
}

/* static */
MailFolder *MFPool::GetNext(Cookie& cookie, String *driverName)
{
   return cookie.m_impl->GetAndAdvance(driverName);
}

