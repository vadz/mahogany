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

// ----------------------------------------------------------------------------
// MMutex: can be in locked or unlocked state, can't be locked more than once
// ----------------------------------------------------------------------------

#if USE_THREADS

// use inheritance and not typedef to allow forward declaring it
class MMutex : public wxMutex { };

#else // !USE_THREADS

class MMutex
{
public:
   MMutex() { m_locked = FALSE; }

   bool IsLocked() const{ return m_locked; }

   void Lock()
   {
      ASSERT_MSG( !IsLocked(), _T("attempting to lock locked mutex") );

      m_locked = true;
   }

   void Unlock()
   {
      ASSERT_MSG( IsLocked(), _T("attempting to unlock unlocked mutex") );

      m_locked = false;
   }

   ~MMutex() { ASSERT_MSG( !IsLocked(), _T("deleting locked mutex") ); }

private:
   bool m_locked;
};

#endif // USE_THREADS/!USE_THREADS

// ----------------------------------------------------------------------------
// MLocker: lock the mutex in ctor, unlock in dtor
//
// NB: unfortunately we already have MMutexLocker elsewhere
// ----------------------------------------------------------------------------

class MLocker
{
public:
   MLocker(MMutex& mutex) : m_mutex(mutex) { m_mutex.Lock(); }
   MLocker(MMutex *mutex) : m_mutex(*mutex) { m_mutex.Lock(); }
   ~MLocker() { m_mutex.Unlock(); }

   bool IsLocked() const { return m_mutex.IsLocked(); }
   operator bool() const { return IsLocked(); }

private:
   MMutex& m_mutex;

   DECLARE_NO_COPY_CLASS(MLocker)
};

#endif // _MTHREAD_H_
