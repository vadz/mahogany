///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MAtExit.h
// Purpose:     atexit() replacement for Mahogany
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

/**
   @file MAtExit.h
   @brief Declaration of MRunAtExit class.

   This class allows to schedule things to run on program termination with
   minimum hassle.
 */

#ifndef _M_MATEXIT_H_
#define _M_MATEXIT_H_

/**
   MRunAtExit allows to run some cleanup code right before shutting down.

   This is the analog of standard atexit() but it will be called before
   MObject::CheckLeaks() is so it can be used to free any memory you don't want
   to see leaked. Unfortunately, the order of execution is undefined here.

   To use this class, create a static object 
 */
class MRunAtExit
{
public:
   /**
      Constructor puts the object in the list of things to do on shutdown.
    */
   MRunAtExit() : m_next(ms_first) { ms_first = this; }

   /**
      Virtual dtor just to suppress compiler warnings.

      The dtor might not be virtual because it must be trivial anyhow: it is
      ran too late to do anything!
    */
   virtual ~MRunAtExit() { }

   /// Get the first object in the list
   static MRunAtExit *GetFirst() { return ms_first; }

   /// Get the next one or NULL
   MRunAtExit *GetNext() const { return m_next; }


   /**
      The operation to perform on shut down.

      This is called from MApplication dtor right before checking for memory
      leaks, all other subsystems can't be relied upon any longer!
    */
   virtual void Do() = 0;

private:
   // next object in the linked list
   MRunAtExit * const m_next;

   // this is defined in classes/MApplication.cpp
   static MRunAtExit *ms_first;

   DECLARE_NO_COPY_CLASS(MRunAtExit);
};

/**
   MRunFunctionAtExit allows to run a simple function on exit.

   This just simplifies the life a little if the function to be executed on
   shut down already exists.
 */
class MRunFunctionAtExit : public MRunAtExit
{
public:
   /**
      Constructor takes a pointer to the function to run.

      @param fn the function to execute on shutdown.
    */
   MRunFunctionAtExit(void (*fn)()) : m_fn(fn) { }

   virtual void Do() { (*m_fn)(); }

private:
   void (*m_fn)();

   DECLARE_NO_COPY_CLASS(MRunFunctionAtExit);
};

#endif // _M_MATEXIT_H_

