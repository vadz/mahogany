///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   QuotedText.cpp
// Purpose:     implementation of CountQuoteLevel() function
// Author:      Vadim Zeitlin
// Created:     2006-04-08 (extracted from src/modules/viewflt/QuoteURL.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2002-2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "Profile.h"
   #include "Mdefaults.h"
#endif

#include "QuotedText.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_MVIEW_QUOTED_MAXWHITESPACE;
extern const MOption MP_MVIEW_QUOTED_MAXALPHA;

// ============================================================================
// CountQuoteLevel() and helper functions implementation
// ============================================================================

static bool IsBlankLine(const wxChar *p)
{
   for ( ;; )
   {
      switch ( *p++ )
      {
         case ' ':
         case '\t':
         case '\r':
            break;

         case '\n':
         case '\0':
            return true;

         default:
            return false;
      }
   }
}

// type of UpdateLineStatus() parameter
enum LineResult
{
   Line_Blank = -2,
   Line_Unknown = -1,
   Line_Different,
   Line_Same
};

// advance the *pp pointer if it points to the same thing as c
static void
UpdateLineStatus(const wxChar *c, const wxChar **pp, LineResult *pSameAs)
{
   if ( *pSameAs == Line_Unknown || *pSameAs == Line_Same )
   {
      if ( **pp != *c )
      {
         *pSameAs = IsBlankLine(*pp) ? Line_Blank : Line_Different;
      }
      else
      {
         *pSameAs = Line_Same; // could have been Line_Unknown

         (*pp)++;
      }
   }
}

int
CountQuoteLevel(const wxChar *string,
                int max_white,
                int max_alpha,
                QuoteData& quoteData)
{
   // update the previous line pointer for the next call
   const wxChar *prev = quoteData.linePrev;
   quoteData.linePrev = string;

   // check if this line had been already detected as a quoted tail of the
   // previous one
   int level = quoteData.levelWrapped;
   if ( level )
   {
      quoteData.levelWrapped = 0;
      return level;
   }


   // find the beginning of the and next line
   const wxChar *nextStart = wxStrchr(string, _T('\n'));

   // it's simpler to pretend that the next line is the same as this one
   // instead of checking for it all the time below
   const wxChar *next = nextStart ? nextStart + 1 /* skip '\n' */ : string;

   if ( !prev )
   {
      // same as above for next
      prev = string;
   }


   // look at the beginning of this string and count (nested) quoting levels
   LineResult sameAsNext = Line_Unknown,
              sameAsPrev = Line_Unknown;
   const wxChar *lastQuote = string;
   for ( const wxChar *c = string; *c != 0 && *c != '\n'; c++, lastQuote = c )
   {
      // skip leading white space
      for ( int num_white = 0; *c == '\r' || *c == '\t' || *c == ' '; c++ )
      {
         if ( ++num_white > max_white )
         {
            // too much whitespace for this to be a quoted string
            return level;
         }

         UpdateLineStatus(c, &prev, &sameAsPrev);
         UpdateLineStatus(c, &next, &sameAsNext);
      }

      // skip optional alphanumeric prefix
      for ( int num_alpha = 0; wxIsalpha(*c); c++ )
      {
         if ( ++num_alpha > max_alpha )
         {
            // prefix too long, not start of the quote
            return level;
         }

         UpdateLineStatus(c, &prev, &sameAsPrev);
         UpdateLineStatus(c, &next, &sameAsNext);
      }

      // check if we have a quoted line or not now: we consider the line to be
      // quoted if it starts with a special quoting character and if the
      // next lines starts with the same prefix as well or is blank and if the
      // previous line is similar to but not the same as this one

      // first check if we have a quote prefix for this level at all
      if ( !quoteData.HasQuoteAtLevel(level) )
      {
         // detect the quoting character used, and remember it for the rest of
         // the message

         // TODO: make the string of "quoting characters" configurable
         static const wxChar *QUOTE_CHARS = _T(">|})*");

         // strchr would find NUL in the string so test for it separately
         if ( *c == '\0' || !wxStrchr(QUOTE_CHARS, *c) )
            break;

         // '*' and '}' are too often used for other purposes, check that we
         // really have something like a whole paragraph quoted with it before
         // deciding that we really should accept it as a quoting character
         if ( (*c == '*' || *c == '}') && *next != *c )
            break;

         quoteData.SetQuoteAtLevel(level, String(lastQuote, c + 1));
      }
      else // we have already seen this quoting prefix
      {
         // check that we have the same prefix
         if ( wxStrncmp(lastQuote, quoteData.GetQuoteAtLevel(level),
                           c - lastQuote + 1) != 0 )
         {
            // consider that '>' is always a "true" quote character...
            // otherwise we fail to recognize too many strange messages where
            // different quoting styles are used
            if ( *c != '>' )
               break;

            quoteData.SetQuoteAtLevel(level, String(lastQuote, c + 1));
         }
      }


      // look at what we really have in the previous/next lines
      UpdateLineStatus(c, &prev, &sameAsPrev);
      UpdateLineStatus(c, &next, &sameAsNext);

      // if this line has the same prefix as the previous one, it surely must
      // be a continuation of a quoted paragraph
      bool isQuoted = sameAsPrev == Line_Same;

      switch ( sameAsNext )
      {
         default:
         case Line_Unknown:
            FAIL_MSG( _T("logical error: unexpected sameAsNext value") );
            // fall through

         case Line_Different:
            // check for wrapped quoted lines

            // as this has a lot of potential for false positives, only do it
            // for the most common quoting character
            if ( !isQuoted && (!nextStart || *c != '>') )
               break;

            // empty or very short lines shouldn't be wrapped: this catches a
            // not uncommon case of
            //
            //          > 111
            //          >
            //          333
            //
            // where "333" would otherwise have been recognized as wrapped
            // quotation
            if ( next - string > 50 )
            {
               // we also check "wrapped" line is short enough
               const wxChar *nextnext = wxStrchr(nextStart + 1 /* skip \n */,
                                                 _T('\n'));
               if ( !nextnext ||
                     (nextnext - next > 25) ||
                      (!IsBlankLine(nextnext + 1) &&
                        wxStrncmp(string, nextnext + 1, next - nextStart) != 0) )
               {
                  // the line after next doesn't start with the same prefix as
                  // this one so it's improbable that the next line was garbled
                  // because of quoting -- chances are this line is simply not
                  // quoted at all unless we had already recognized it such
                  if ( !isQuoted )
                  {
                     // last chance: we suppose that a quoted line preceded by
                     // a blank line is really quoted
                     if ( sameAsPrev == Line_Blank )
                        isQuoted = true;
                  }
               }
               else // looks like the next line is indeed our wrapped tail
               {
                  quoteData.levelWrapped = level + 1;

                  isQuoted = true;
               }
            }
            break;

         case Line_Blank:
            // we probably should check here that either the previous line is
            // empty or it seems to be an attribution line (easier said than
            // done)

            // fall through

         case Line_Same:
            isQuoted = true;
      }

      if ( !isQuoted )
         break;

      level++;
   }

   return level;
}

// ============================================================================
// other public functions
// ============================================================================

String GetUnquotedText(const String& text, Profile *profile)
{
   String unquoted;
   unquoted.reserve(text.length());

   const int maxWhite = READ_CONFIG(profile, MP_MVIEW_QUOTED_MAXWHITESPACE);
   const int maxAlpha = READ_CONFIG(profile, MP_MVIEW_QUOTED_MAXALPHA);
   QuoteData qd;
   for ( const wxChar *lineCur = text.c_str(); *lineCur; )
   {
      // find the start of the next line
      const wxChar *lineNext = wxStrchr(lineCur, _T('\n'));

      if ( CountQuoteLevel(lineCur, maxWhite, maxAlpha, qd) == 0 )
      {
         // +1 to take '\n' too
         unquoted += String(lineCur, lineNext ? lineNext + 1 - lineCur
                                              : String::npos);
      }

      if ( !lineNext )
         break;

      // go to the next line (skip '\n')
      lineCur = lineNext + 1;
   }

   return unquoted;
}

