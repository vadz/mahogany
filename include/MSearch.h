///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MSearch.h
// Purpose:     various search-related stuff
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.02 (extracted from MailFolder.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MSEARCH_H_
#define _MSEARCH_H_

/**
  Search criterium for searching folder for certain messages.
 */
class SearchCriterium
{
public:
   enum Type
   {
      SC_ILLEGAL = -1,
      SC_FULL = 0,
      SC_BODY,
      SC_HEADER,
      SC_SUBJECT,
      SC_TO,
      SC_FROM,
      SC_CC
   };

   Type m_What;
   bool m_Invert;
   String m_Key;

   SearchCriterium() { m_What = SC_ILLEGAL; m_Invert = false; }
};

#endif // _MSEARCH_H_

