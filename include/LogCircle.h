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
   MLogCircle(int n);
   ~MLogCircle();

   void Add(const String &txt);
   bool Find(const String needle, String *store = NULL) const;
   String GetLog(void) const;
   void Clear(void);

   /**
     Looks at log data and guesses what went wrong and calls LOGMESSAGE()
     if we have any ideas.
    */
   void GuessError(void) const;

private:
   int m_N,
       m_Next;

   String *m_Messages;
};

#endif // _LOGCIRCLE_H_

