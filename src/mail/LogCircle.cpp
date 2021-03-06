///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/LogCircle.cpp
// Purpose:     implements MLogCircle class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.02 (extracted from MailFolder.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "LogCircle.h"

// ============================================================================
// MLogCircle implementation
// ============================================================================

MLogCircle::MLogCircle(int n)
{
   m_N = n;
   m_Messages = new String[m_N];
   m_Next = 0;
}

MLogCircle::   ~MLogCircle()
{
   delete [] m_Messages;
}

void
MLogCircle:: Add(const String &txt)
{
   m_Messages[m_Next++] = txt;
   if(m_Next == m_N)
      m_Next = 0;
}

bool
MLogCircle:: Find(const String needle, String *store) const
{
   // searches backwards (most relevant first)
   // search from m_Next-1 to 0
   if(m_Next > 0)
      for(int i = m_Next-1; i >= 0 ; i--)
      {
         wxLogTrace(_T("logcircle"), _T("checking msg %d, %s"), i, m_Messages[i].c_str());
         if(m_Messages[i].Contains(needle))
         {
            if(store)
               *store = m_Messages[i];
            return true;
         }
      }
   // search from m_N-1 down to m_Next:
   for(int i = m_N-1; i >= m_Next; i--)
   {
      wxLogTrace(_T("logcircle"), _T("checking msg %d, %s"), i, m_Messages[i].c_str());
      if(m_Messages[i].Contains(needle))
      {
         if(store)
            *store = m_Messages[i];
         return true;
      }
   }

   return false;
}

/* static */
String
MLogCircle::GuessError(void) const
{
   String guess;

   if ( Find(_T("No such host")) )
   {
      guess = _("The server name could not be resolved "
                "(maybe the network connection is down?)");
   }
   else if ( Find(_T("User unknown")) )
   {
      guess = _("One or more email addresses were not recognised.");
   }
   // check for various POP3 bad login/password messages
   else if ( Find(_T("authorization failed")) ||
             Find(_T("password wrong")) ||
             Find(_T("bad password")) ||
             Find(_T("unknown user name")) ||
             Find(_T("Bad authentication")) ||
             Find(_T("Password supplied for")) || // ... <email addr> is incorrect
             Find(_T("Invalid password")) ||
             Find(_T("Invalid login")) )
   {
      guess = _("Incorrect username or password: "
                "please verify if they are correct.");
   }
   // SMTP 554 error
   else if ( Find(_T("recipients failed")) )
   {
      guess = _("Mail server didn't accept one or more of the message recipients");
   }
   // these are generated by c-client
   else if ( Find(_T("INVALID_ADDRESS")) ||
             Find(_T(".SYNTAX-ERROR.")) )
   {
      guess = _("The message contained an invalid address specification.");
   }
   // check for various POP3 server messages telling us that another session
   // is active
   else if ( Find(_T("mailbox locked")) ||
             Find(_T("lock busy")) ||
             Find(_T("[IN-USE]")) )
   {
      guess = _("The mailbox is already opened from another session, "
                "please close it first and retry.");
   }
   else if ( Find(_T("Self-signed certificate or untrusted authority")) )
   {
      guess = _("You chose not to trust the SSL certificate of this server.\n"
                "\n"
                "You may want to check the \"Accept unsigned certificates\" option\n"
                "in the \"Access\" page of the folder properties dialog if you\n"
                "want to accept self-signed certificates.");
   }
   else if ( Find(_T("SMTP SERVER BUG (invalid challenge)")) )
   {
      guess = _("SMTP server advertises support for PLAIN authorization "
                "mechanism, but doesn't really support it.\n"
                "\n"
                "Please enter \"PLAIN\" in the \"Methods NOT to use\" field "
                "of the \"Network\" page of the program options dialog and "
                "retry");
   }

   return guess;
}

void
MLogCircle::Clear(void)
{
   for(int i = 0; i < m_N; i++)
   {
      m_Messages[i].clear();
   }
}


