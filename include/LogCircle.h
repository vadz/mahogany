///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   LogCircle.h
// Purpose:     declares MLogCircle class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.02 (extracted from MailFolder.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _LOGCIRCLE_H_
#define _LOGCIRCLE_H_

#ifdef __GNUG__
   #pragma interface "LogCircle.h"
#endif

// ----------------------------------------------------------------------------
// MLogCircle
// ----------------------------------------------------------------------------

/**
  A small class to hold the last N log messages for analysis: when an error
  happens you may call its GuessError() method to try to give the user a
  human-readable description of the problem.
 */
class MLogCircle
{
public:
   /**
      Creates an empty log circle.

      @param n the size of the internal buffer, i.e. the number of last error
               messages stored
    */
   MLogCircle(int n);

   /// Trivial destructor
   ~MLogCircle();

   /**
      Adds a log message to the buffer.

      This method must be called for all error messages if you want
      GuessError() to be able to do its job.

      @param txt the error message
    */
   void Add(const String& txt);

   /**
      Forgets the remembered error messages.
    */
   void Clear();

   /**
      Looks at log data and tries to guess what went wrong.

      If we could determine the reason for the error, we return the string and
      also possibly call LOGMESSAGE() with some more explanations. The reason
      for doing it like this is that the calling code usually does its own
      LOGERROR() and if we did LOGMESSAGE() directly from this function, our
      explanation wouldn't be directly visible to the user and as 90% of the
      users never click on "Details" button, they would never see it at all
      (and these 90% are also probably those who need to see it most...).

      So instead the caller should append our string, usually after a new line,
      to the string it passes to LOGERROR().
    */
   String GuessError() const;

private:
   bool Find(const String needle, String *store = NULL) const;

   int m_N,
       m_Next;

   String *m_Messages;
};

#endif // _LOGCIRCLE_H_

