///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/MThread.h: various thread related classes
// Purpose:     mutexes, semaphores and all that
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.01.01
// CVS-ID:      $Id$
// Copyright:   (C) 2001 Vadim Zeitlin (vadim@wxwindows.org)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MTHREAD_H_
#define _MTHREAD_H_

/**
    @file
    @brief Various thread-related classes.

    For historical reasons we used to have MMutex class which was defined
    differently depending on whether M was compiled with @c USE_THREADS on or
    off. As we always use threads now, this doesn't make sense any longer but
    the existing code using "single thread" version of MMutex can't be blindly
    changed to work with the real "multi threaded" version. So we currently
    define both of them and will get rid of the single threaded one in the
    (distant) future when all code becomes really MT-safe.
 */

#include <wx/thread.h>

/**
    Real mutex object.

    This is a real mutex which prevents more than one thread from acquiring it
    at any given time. It is non-recursive meaning that it can't locked twice
    even by a thread that had already locked it before.

    It is implemented simply as wxMutex right now but we could switch to using
    some other mutex class (e.g. from Boost.Thread) in the future.
 */
typedef wxMutex MTMutex;

/**
    Stand in for a mutex for single-threaded code.

    This class emulates a mutex for single threaded code by just preventing
    reentrancy, i.e. it makes it impossible for the same thread to lock it
    twice.
 */
class STMutex
{
public:
   /// Creates an initially unlocked mutex.
   STMutex() { m_locked = false; }

   /**
       Check if the mutex is locked.

       This method doesn't exist in the real MTMutex class so it shouldn't be
       used in the code which is meant to become MT-safe.
    */
   bool IsLocked() const { return m_locked; }

   /**
       Lock the mutex.

       @return wxMUTEX_NO_ERROR or wxMUTEX_DEAD_LOCK.
    */
   wxMutexError Lock()
   {
      if ( IsLocked() )
      {
         FAIL_MSG( "attempting to lock locked mutex" );

         return wxMUTEX_DEAD_LOCK;
      }

      m_locked = true;

      return wxMUTEX_NO_ERROR;
   }

   /**
       Unlock the mutex.

       @return wxMUTEX_NO_ERROR or wxMUTEX_UNLOCKED.
    */
   wxMutexError Unlock()
   {
      if ( !IsLocked() )
      {
         FAIL_MSG( "attempting to unlock unlocked mutex" );

         return wxMUTEX_UNLOCKED;
      }

      m_locked = false;

      return wxMUTEX_NO_ERROR;
   }

   /// Dtor checks that the mutex had been unlocked before.
   ~STMutex() { ASSERT_MSG( !IsLocked(), "deleting locked mutex" ); }

private:
   bool m_locked;

   wxDECLARE_NO_COPY_CLASS(STMutex);
};

/**
    Compatibility name, don't use any longer.

    Currently this is just a synonym for STMutex.
 */
typedef STMutex MMutex;

/**
   Lock the mutex in ctor, unlock in dtor.
 */
template <class M>
class MutexLocker
{
public:
   typedef M Mutex;

   MutexLocker(Mutex& mutex) : m_mutex(mutex) { m_mutex.Lock(); }
   ~MutexLocker() { m_mutex.Unlock(); }

protected:
   Mutex& m_mutex;

   wxDECLARE_NO_COPY_TEMPLATE_CLASS(MutexLocker, M);
};

/**
    Compatibility name, don't use any longer.
 */
class MLocker : public MutexLocker<MMutex>
{
public:
   MLocker(MMutex& mutex) : MutexLocker<MMutex>(mutex) { }
   MLocker(MMutex *mutex) : MutexLocker<MMutex>(*mutex) { }

   bool IsLocked() const { return m_mutex.IsLocked(); }
   operator bool() const { return IsLocked(); }

private:
   wxDECLARE_NO_COPY_CLASS(MLocker);
};

#endif // _MTHREAD_H_
