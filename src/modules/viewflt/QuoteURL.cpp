///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   QuoteURL.cpp: implementation of "default" viewer filter
// Purpose:     default filter handles URL detection and quoting levels
// Author:      Vadim Zeitlin
// Modified by:
// Created:     01.12.02 (extracted from src/classes/MessageView.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/*
   Normally we should have 2 different filters instead of only one which does
   both quote level and URL detection but this is difficult because it's not
   clear which one should be applied first: URLs may span several lines so we
   can't handle quoting first, then URLs (at least not if we want to detect
   quoetd wrapped URLs -- which we don't do now). And detecting URLs first and
   then passing the rest through quote filter is quite unnatural -- but maybe
   could still be done?
 */

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "strutil.h"
#   include "Mdefaults.h"
#endif //USE_PCH

#include "ViewFilter.h"

#include "MessageViewer.h"
#include "MTextStyle.h"

#include "ColourNames.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_HIGHLIGHT_URLS;
extern const MOption MP_MVIEW_QUOTED_COLOURIZE;
extern const MOption MP_MVIEW_QUOTED_CYCLE_COLOURS;
extern const MOption MP_MVIEW_QUOTED_COLOUR1;
extern const MOption MP_MVIEW_QUOTED_COLOUR2;
extern const MOption MP_MVIEW_QUOTED_COLOUR3;
extern const MOption MP_MVIEW_QUOTED_MAXWHITESPACE;
extern const MOption MP_MVIEW_QUOTED_MAXALPHA;
extern const MOption MP_MVIEW_URLCOLOUR;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the max quoting level for which we have unique colours (after that we
// recycle them or just reuse the last one)
static const size_t QUOTE_LEVEL_MAX = 3;

// invalid quote level
static const size_t LEVEL_INVALID = (size_t)-1;

// ----------------------------------------------------------------------------
// local classes
// ----------------------------------------------------------------------------

// this struct is used by CountQuoteLevel() to store global information about
// the message
class QuoteData
{
public:
   QuoteData() { levelWrapped = 0; linePrev = NULL; }

   // functions for querying/setting quote prefix for the given (0-based) level
   bool HasQuoteAtLevel(size_t level) const
   {
      return m_quoteAtLevel.size() > level;
   }

   void SetQuoteAtLevel(size_t level, const String& s)
   {
      ASSERT_MSG( level == m_quoteAtLevel.size(),
                     _T("should set quote prefixes in order") );

      m_quoteAtLevel.push_back(s);
   }

   const String& GetQuoteAtLevel(size_t level) const
   {
      return m_quoteAtLevel[level];
   }


   // pointer to previous line we examined
   const char *linePrev;

   // set to > 0 if the next line is wrapped tail of this one
   int levelWrapped;

private:
   // array of quote markers for all quoting levels we have seen so far
   wxArrayString m_quoteAtLevel;
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

/** Count levels of quoting on the first line of passed string.

    It understands standard e-mail quoting methods such as ">" and "XY>".

    @param string the string to check
    @param max_white max number of white characters before quotation mark
    @param max_alpha max number of A-Z characters before quotation mark
    @param quoteData global quoting data, should be saved between function
                     calls
    @return number of quoting levels (0 for unquoted text)
  */
static int
CountQuoteLevel(const char *string,
                int max_white,
                int max_alpha,
                QuoteData& quoteData);

/**
   Check if there is only whitespace until the end of line.
 */
static bool IsBlankLine(const char *p);

// ----------------------------------------------------------------------------
// QuoteURLFilter declaration
// ----------------------------------------------------------------------------

class QuoteURLFilter : public ViewFilter
{
public:
   QuoteURLFilter(MessageView *msgView, ViewFilter *next, bool enable)
      : ViewFilter(msgView, next, enable)
   {
      ReadOptions(m_options, msgView->GetProfile());
   }

   virtual bool UpdateOptions(Profile *profile);

protected:
   struct Options
   {
      // the colours for quoted text (only used if quotedColourize)
      //
      // the first element in this array is the normal foreground colour, i.e.
      // quote level == 0 <=> unquoted
      wxColour QuotedCol[QUOTE_LEVEL_MAX + 1];

      // max number of whitespaces before >
      int quotedMaxWhitespace;

      // max number of A-Z before >
      int quotedMaxAlpha;

      // do quoted text colourizing?
      bool quotedColourize:1;

      // if there is > QUOTE_LEVEL_MAX levels of quoting, recycle colours?
      bool quotedCycleColours:1;

      /// highlight URLs?
      bool highlightURLs:1;

      bool operator==(const Options& o) const
      {
         bool eq = quotedMaxWhitespace == o.quotedMaxWhitespace &&
                   quotedColourize == o.quotedColourize &&
                   quotedCycleColours == o.quotedCycleColours &&
                   highlightURLs == o.highlightURLs;
         if ( eq && quotedColourize )
         {
            for ( size_t n = 0; n <= QUOTE_LEVEL_MAX; n++ )
            {
               if ( QuotedCol[n] != o.QuotedCol[n] )
               {
                 eq = false;
                 break;
               }
            }
         }

         return eq;
      }
   };

   // the main work function
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);

   // fill m_options using the values from the given profile
   void ReadOptions(Options& options, Profile *profile);

   // get the quote level for the line (prev is for CountQuoteLevel() only)
   size_t GetQuotedLevel(const char *line, QuoteData& quote) const;

   // get the colour for the given quote level
   wxColour GetQuoteColour(size_t qlevel) const;

   // finds the next URL if we're configured to detect them or always returns
   // NULL if we are not
   const wxChar *FindURLIfNeeded(const wxChar *s, int& len);


   Options m_options;
};

// ============================================================================
// CountQuoteLevel implementation
// ============================================================================

bool IsBlankLine(const char *p)
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

enum LineResult
{
   Line_Blank = -2,
   Line_Unknown = -1,
   Line_Different,
   Line_Same
};

// advance the *pp pointer if it points to the same thing as c
static void
UpdateLineStatus(const char *c, const char **pp, LineResult *pSameAs)
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
CountQuoteLevel(const char *string,
                int max_white,
                int max_alpha,
                QuoteData& quoteData)
{
   // update the previous line pointer for the next call
   const char *prev = quoteData.linePrev;
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
   const char *nextStart = strchr(string, '\n');

   // it's simpler to pretend that the next line is the same as this one
   // instead of checking for it all the time below
   const char *next = nextStart ? nextStart + 1 /* skip '\n' */ : string;

   if ( !prev )
   {
      // same as above for next
      prev = string;
   }


   // look at the beginning of this string and count (nested) quoting levels
   LineResult sameAsNext = Line_Unknown,
              sameAsPrev = Line_Unknown;
   const char *lastQuote = string;
   for ( const char *c = string; *c != 0 && *c != '\n'; c++, lastQuote = c )
   {
      // skip leading white space
      for ( int num_white = 0; *c == '\t' || *c == ' '; c++ )
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
      for ( int num_alpha = 0; isalpha((unsigned char)*c); c++ )
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
         static const char *QUOTE_CHARS = ">|})*";

         // strchr would find NUL in the string so test for it separately
         if ( *c == '\0' || !strchr(QUOTE_CHARS, *c) )
            break;

         // '*' is too often used for other purposes, check that we really have
         // something like a whole paragraph quoted with it before deciding
         // that we really should accept it as a quoting character
         if ( *c == '*' && *next != *c )
            break;

         quoteData.SetQuoteAtLevel(level, String(lastQuote, c + 1));
      }
      else // we have already seen this quoting prefix
      {
         // check that we have the same prefix
         if ( wxStrncmp(lastQuote, quoteData.GetQuoteAtLevel(level),
                           c - lastQuote + 1) != 0 )
            break;
      }


      // look at what we really have in the previos/next lines
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
               const char *nextnext = strchr(nextStart + 1 /* skip \n */, '\n');
               if ( !nextnext ||
                     (nextnext - next > 25) ||
                      (!IsBlankLine(nextnext + 1) &&
                        strncmp(string, nextnext + 1, next - nextStart) != 0) )
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
// QuoteURLFilter implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the required macro
// ----------------------------------------------------------------------------

// this filter has the default priority -- in fact, the definition of the
// default priority is exactly that this filter has it
IMPLEMENT_VIEWER_FILTER(QuoteURLFilter,
                        ViewFilter::Priority_Default,
                        true,      // enabled by default
                        gettext_noop("Quotes and URLs"),
                        _T("(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>"));

// ----------------------------------------------------------------------------
// QuoteURLFilter options
// ----------------------------------------------------------------------------

void
QuoteURLFilter::ReadOptions(QuoteURLFilter::Options& options,
                            Profile *profile)
{
   // a macro to make setting many colour options less painful
   #define GET_COLOUR_FROM_PROFILE(col, name) \
      ReadColour(&col, profile, MP_MVIEW_ ## name)

   GET_COLOUR_FROM_PROFILE(options.QuotedCol[1], QUOTED_COLOUR1);
   GET_COLOUR_FROM_PROFILE(options.QuotedCol[2], QUOTED_COLOUR2);
   GET_COLOUR_FROM_PROFILE(options.QuotedCol[3], QUOTED_COLOUR3);

   #undef GET_COLOUR_FROM_PROFILE

   options.quotedColourize =
       READ_CONFIG_BOOL(profile, MP_MVIEW_QUOTED_COLOURIZE);
   options.quotedCycleColours =
       READ_CONFIG_BOOL(profile, MP_MVIEW_QUOTED_CYCLE_COLOURS);
   options.quotedMaxWhitespace =
       READ_CONFIG_BOOL(profile, MP_MVIEW_QUOTED_MAXWHITESPACE);
   options.quotedMaxAlpha = READ_CONFIG(profile, MP_MVIEW_QUOTED_MAXALPHA);

   options.highlightURLs = READ_CONFIG_BOOL(profile, MP_HIGHLIGHT_URLS);
}

bool QuoteURLFilter::UpdateOptions(Profile *profile)
{
   Options optionsNew;
   ReadOptions(optionsNew, profile);

   if ( optionsNew == m_options )
      return false;

   m_options = optionsNew;

   return true;
}

// ----------------------------------------------------------------------------
// QuoteURLFilter helpers for quoting level code
// ----------------------------------------------------------------------------

size_t
QuoteURLFilter::GetQuotedLevel(const char *line, QuoteData& quoteData) const
{
   size_t qlevel = CountQuoteLevel
                   (
                     line,
                     m_options.quotedMaxWhitespace,
                     m_options.quotedMaxAlpha,
                     quoteData
                   );

   // note that qlevel is counted from 1, really, as 0 means unquoted and that
   // GetQuoteColour() relies on this
   if ( qlevel > QUOTE_LEVEL_MAX )
   {
      if ( m_options.quotedCycleColours )
      {
         // cycle through the colours: use 1st level colour for QUOTE_LEVEL_MAX
         qlevel = (qlevel - 1) % QUOTE_LEVEL_MAX + 1;
      }
      else
      {
         // use the same colour for all levels deeper than max
         qlevel = QUOTE_LEVEL_MAX;
      }
   }

   return qlevel;
}

wxColour
QuoteURLFilter::GetQuoteColour(size_t qlevel) const
{
   CHECK( qlevel < QUOTE_LEVEL_MAX + 1, wxNullColour,
          _T("QuoteURLFilter::GetQuoteColour(): invalid quoting level") );

   return m_options.QuotedCol[qlevel];
}

// ----------------------------------------------------------------------------
// QuoteURLFilter::DoProcess() itself
// ----------------------------------------------------------------------------

const wxChar *
QuoteURLFilter::FindURLIfNeeded(const wxChar *s, int& len)
{
   if ( !m_options.highlightURLs )
      return NULL;

   extern int FindURL(const wxChar *s, int& len);

   int pos = FindURL(s, len);

   return pos == -1 ? NULL : s + pos;
}

void
QuoteURLFilter::DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style)
{
   // the default foreground colour
   m_options.QuotedCol[0] = style.GetTextColour();

   size_t level = LEVEL_INVALID;

   QuoteData quoteData;

   const wxChar *lineCur = text.c_str();

   int lenURL;
   const wxChar *startURL = FindURLIfNeeded(lineCur, lenURL);
   while ( *lineCur )
   {
      if ( m_options.quotedColourize )
      {
         size_t levelNew = GetQuotedLevel(lineCur, quoteData);
         if ( levelNew != level )
         {
            level = levelNew;
            style.SetTextColour(GetQuoteColour(level));
         }
      }

      // find the start of the next line
      const wxChar *lineNext = wxStrchr(lineCur, _T('\n'));

      // and look for all URLs on the current line
      const wxChar *endURL = lineCur;
      while ( startURL && (lineCur <= startURL && startURL < lineNext) )
      {
         // insert the text before URL
         String textBefore(lineCur, startURL);
         m_next->Process(textBefore, viewer, style);

         // then the URL itself (we use the same string for text and URL)
         endURL = startURL + lenURL;
         String url(startURL, endURL);
         m_next->ProcessURL(url, url, viewer);

         // if the URL wraps to the next line, we consider that we're still on
         // the same logical line, i.e. that quoting level doesn't change if
         // the line is wrapped
         while ( lineNext && endURL > lineNext )
         {
            lineNext = wxStrchr(lineNext + 1, _T('\n'));
         }

         // now look for the next URL
         startURL = FindURLIfNeeded(endURL, lenURL);
      }

      // finally insert everything after the last URL (if any)
      String textAfter(endURL, lineNext ? lineNext + 1 : text.end());
      m_next->Process(textAfter, viewer, style);

      if ( !lineNext )
         break;

      // go to the next line (skip '\n')
      lineCur = lineNext + 1;
   }
}

