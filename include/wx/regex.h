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
   virtual bool Match(const wxString &str, Flags flags = RE_NONE) const = 0;
   /** Replaces the current regular expression in the string pointed
       to by pattern, with the text in replacement.
       @param pattern the string to change
       @param replacement the new string to put there
       @return number of matches replaced or -1 on error
   */
   virtual int Replace(wxString *pattern,
                        const wxString &replacement) const = 0;
   virtual bool IsValid(void) const = 0;
   virtual ~wxRegExBase() {}
};

#ifdef HAVE_POSIX_REGEX

extern "C" {    // Suns don't protect their C headers (yet?)
#include <regex.h>
}

#ifndef WX_REGEX_MAXMATCHES
#   define WX_REGEX_MAXMATCHES 1024
#endif

class wxRegExPOSIX : public wxRegExBase
{
 public:
   wxRegExPOSIX(const wxString &expr,
                Flags flags = RE_DEFAULT)
      {
         SetFlags(flags);
         m_Valid = FALSE;
         m_Valid = Compile(expr);
         m_nMatches = WX_REGEX_MAXMATCHES;
         m_Matches = new regmatch_t[m_nMatches];
      }
   ~wxRegExPOSIX()
      {
         if(m_Valid) regfree(&m_RegEx);
         delete [] m_Matches;
      }
   virtual bool IsValid(void) const
      {
         return m_Valid;
      }

   virtual void SetFlags(Flags flags)
      {
         m_Flags = flags;
      }
   virtual bool Match(const wxString &str, Flags flags) const
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
        return
          regexec((regex_t *)&m_RegEx, str.c_str(),
                  m_nMatches, m_Matches,
                  myflags) == 0;
      }

   /** Replaces the current regular expression in the string pointed
       to by pattern, with the text in replacement.
       @param pattern the string to change
       @param replacement the new string to put there
       @return number of matches replaced or -1 on error
   */
   virtual int Replace(wxString *pattern,
                        const wxString &replacement) const
      {
         ASSERT(pattern);
         if(! IsValid()) return -1;

         int replaced = 0;
         size_t lastpos = 0;
         wxString newstring;

         for(size_t idx = 0;
             m_Matches[idx].rm_so != -1 && idx < m_nMatches;
             idx++)
         {
            // copy non-matching bits:
            newstring << pattern->Mid(lastpos,
                                      m_Matches[idx].rm_so - lastpos);
            // copy replacement:
            newstring << replacement;
            // remember how far we got:
            lastpos = m_Matches[idx].rm_eo;
            replaced ++;
         }
         if(replaced > 0)
            *pattern = newstring;
         return replaced;
      }
   /** Compile string into regular expression.
       @param expr string expression
       @return true on success
   */
   bool Compile(const wxString &expr)
      {
         if(IsValid()) regfree(&m_RegEx);
         int errorcode = regcomp(&m_RegEx, expr, GetCFlags());
         if(errorcode)
         {
            char buffer[256];
            regerror(errorcode, &m_RegEx, buffer, 256);
            wxLogError(_("Regular expression syntax error: '%s'"),
                       buffer);
            m_Valid = FALSE;
            return FALSE;
         }
         else
         {
            m_Valid = FALSE;
            return TRUE;
         }
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
   bool        m_Valid;
   int         m_Flags;
   regex_t     m_RegEx;
   regmatch_t *m_Matches;
   size_t      m_nMatches;
};

#   define wxRegEx   wxRegExPOSIX

#define WX_HAVE_REGEX

#endif

#endif
