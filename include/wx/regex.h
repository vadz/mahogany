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
#   pragma interface "wx/regex.h"
#endif

class wxRegExBase
{
 public:
   /// just like POSIX flags
   enum Flags
   {
      RE_NONE     = 0,
      RE_EXTENDED = 2,
      RE_ICASE    = 4,
      RE_NOSUB    = 8,
      RE_NEWLINE  = 16,
      RE_NOTBOL   = 32,
      RE_NOTEOL   = 64,
      RE_DEFAULT  = RE_NOSUB
   };
   virtual void SetFlags(Flags flags) = 0;
   /** Matches the precompiled regular expression against a string.
       @param Flags only allowed values: RE_NOTBOL/RE_NOTEOL
       @return true if found
   */
   virtual int Match(const wxString &str, Flags flags = RE_NONE) const = 0;
   virtual bool IsValid(void) const = 0;
};

#ifdef HAVE_POSIX_REGEX

#include <regex.h>

class wxRegExPOSIX : public wxRegExBase
{
 public:
   wxRegExPOSIX(const wxString &expr,
                Flags flags = RE_DEFAULT)
      {
         SetFlags(flags);
         m_Valid = Compile(expr);
      }
   virtual bool IsValid(void) const
      {
         return m_Valid;
      }
   
   virtual void SetFlags(Flags flags)
      {
         m_Flags = flags;
      }
   virtual int Match(const wxString &str, Flags flags) const
      {
         if( ! m_Valid )
         {
            wxLogError(_("Cannot match string with invalid regular expression."));
            return FALSE;
         }
         if( (flags & (RE_NOTBOL|RE_NOTEOL)) != flags)
         {
            wxLogError(_("Regular expression matching called with illegal flags."));
            return FALSE;
         }

         int myflags = 0;
         if(flags & RE_NOTBOL) myflags |= REG_NOTBOL;
         if(flags & RE_NOTEOL) myflags |= REG_NOTEOL;
         regcomp((regex_t *)&m_RegEx, str.c_str(), myflags);
      }


   /** Compile string into regular expression.
       @param expr string expression
       @return true on success
   */
   bool Compile(const wxString &expr)
      {
         int errorcode = regcomp(&m_RegEx, expr, GetCFlags());
         if(errorcode)
         {
            char buffer[256];
            regerror(errorcode, &m_RegEx, buffer, 256);
            wxLogError(_("Regular expression syntax error: '%s'"),
                       buffer);
            return FALSE;
         }
         else
            return TRUE;
      }
private:
   int GetCFlags(void)
      {
         int flags = 0;
         if( m_Flags & RE_EXTENDED ) flags |= REG_EXTENDED;
         if( m_Flags & RE_ICASE ) flags |= REG_ICASE;         
         if( m_Flags & RE_NOSUB ) flags |= REG_NOSUB;         
         if( m_Flags & RE_NEWLINE ) flags |= REG_NEWLINE;
         return flags;
      }
   bool     m_Valid;
   int      m_Flags;
   regex_t  m_RegEx;
};


#   define wxRegEx   wxRegExPOSIX

#define WX_HAVE_REGEX

#endif



#endif
