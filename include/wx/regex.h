// -*- c++ -*- ////////////////////////////////////////////////////////////////
// Name:        regex.h
// Purpose:     regular expression matching
// Author:      Karsten Ballüder
// Modified by:
// Created:     05.02.2000
// RCS-ID:      $Id$
// Copyright:   (c) 2000 Karsten Ballüder <ballueder@gmx.net>
// Licence:     wxWindows license or Mahogany license
// ////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WXREGEXH__
#define _WX_WXREGEXH__

#ifdef __GNUG__
#   pragma interface "regex.h"
#endif

class wxRegExBase
{
 public:
   /// just like POSIX flags
   enum Flags
   {
      RE_DEFAULT = -1,
      RE_EXTENDED,
      RE_ICASE,
      RE_NOSUB,
      RE_NEWLINE,
      RE_NOTBOL,
      RE_NOTEOL
   };
   virtual void SetFlags(Flags flags) = 0;
   virtual int Match(const wxString &str, Flags flags = RE_DEFAULT) = 0;
};

#ifdef HAVE_POSIX_REGEX

#include <regex.h>

class wxRegExPOSIX : public wxRegExBase
{
 public:
   wxRegExPOSIX(const wxString &expr,
                Flags flags = RE_DEFAULT);
   virtual void SetFlags(Flags flags);
   virtual int Match(const wxString &str, Flags flags = RE_DEFAULT);

   void Compile(const wxString &expr)
      {
         regcomp(&m_RegEx, expr, GetCFlags());
      }
private:
   int GetCFLags(void)
      {
         int flags = 0;
         if( m_Flags & RE_EXTENDED ) flags |= REG_EXTENDED;
         if( m_Flags & RE_ICASE ) flags |= REG_ICASE;         
         if( m_Flags & RE_NOSUB ) flags |= REG_NOSUB;         
         if( m_Flags & RE_NEWLINE ) flags |= REG_NEWLINE;
         return flags;
      }
   int      m_Flags;
   regex_t  m_RegEx;
};


#   define wxRegEx   wxRegExPOSIX

#define WX_HAS_REGEX

#endif



#endif
