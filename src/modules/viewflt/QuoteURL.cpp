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
// private functions
// ----------------------------------------------------------------------------

/** Count levels of quoting on the first line of passed string.

    It understands standard e-mail quoting methods such as ">" and "XY>".

    @param string the string to check
    @param prev the start of previous line or NULL if start of text
    @param max_white max number of white characters before quotation mark
    @param max_alpha max number of A-Z characters before quotation mark
    @return number of quoting levels (0 for unquoted text)
  */
static int
CountQuoteLevel(const char *string,
                const char *prev,
                int max_white,
                int max_alpha,
                bool *nextWrapped);

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
   size_t GetQuotedLevel(const char *line,
                         const char *prev,
                         bool *nextWrapped) const;

   // get the colour for the given quote level
   wxColour GetQuoteColour(size_t qlevel) const;

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
                const char *prev,
                int max_white,
                int max_alpha,
                bool *nextWrapped)
{
   *nextWrapped = false;

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
   int levels = 0;
   for ( const char *c = string; *c != 0 && *c != '\n'; c++, prev++, next++ )
   {
      // skip leading white space
      for ( int num_white = 0; *c == '\t' || *c == ' '; c++ )
      {
         if ( ++num_white > max_white )
         {
            // too much whitespace for this to be a quoted string
            return levels;
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
            return levels;
         }

         UpdateLineStatus(c, &prev, &sameAsPrev);
         UpdateLineStatus(c, &next, &sameAsNext);
      }

      // check if we have a quoted line or not now: we consider the line to be
      // quoted if it starts with a special quoting character and if the
      // next lines starts with the same prefix as well or is blank and if the
      // previous line isn't similar to but not the same as this one (this is
      // necessary to catch last line in a numbered list which would otherwise
      // be considered quoted)

      // first check if we have a quote character at all
      // TODO: make the string of "quoting characters" configurable
      static const char *QUOTE_CHARS = ">|})*";
      if ( !strchr(QUOTE_CHARS, *c) )
         break;

      // next check if the next line has the same prefix
      if ( sameAsNext != Line_Different )
      {
         // so far it does
         if ( *next != *c && !IsBlankLine(next) )
         {
            // but then it diverges, so this is unlikely to be a quote marker
            sameAsNext = Line_Different;
         }
      }
      else // not the same one
      {
         // if the next line is blank, this one is considered to be quoted
         // (otherwise the last line of a quoted paragraph would never be
         // recognized as quoted)
         sameAsNext = IsBlankLine(next + 1) ? Line_Blank : Line_Different;
      }

      // last chance: it is possible that the next line is a wrapped part of
      // this one, so check the line after it too
      if ( sameAsNext == Line_Different )
      {
         // as this has a lot of potential for false positives, only do it for
         // the most common quoting character
         if ( !nextStart || *c != '>' )
            break;

         const char *nextnext = strchr(nextStart + 1 /* skip '\n' */, '\n');
         if ( !nextnext ||
               (!IsBlankLine(nextnext + 1) &&
                strncmp(string, nextnext + 1, next - nextStart + 1) != 0) )
         {
            // the line after next doesn't start with the same prefix as
            // this one so it's improbable that the next line was garbled
            // because of quoting -- chances are this line is simply not
            // quoted at all
            break;
         }

         // it does look like the next line is wrapped tail of this one
         *nextWrapped = true;
      }

      // finally check the previous line
      if ( sameAsPrev == Line_Same && *prev != *c )
      {
         break;
      }
      //else: it's either identical or completely different, both are fine

      levels++;
   }

   return levels;
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
QuoteURLFilter::GetQuotedLevel(const char *line,
                               const char *prev,
                               bool *nextWrapped) const
{
   size_t qlevel = CountQuoteLevel
                   (
                     line,
                     prev,
                     m_options.quotedMaxWhitespace,
                     m_options.quotedMaxAlpha,
                     nextWrapped
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

void
QuoteURLFilter::DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style)
{
   // the default foreground colour
   m_options.QuotedCol[0] = style.GetTextColour();

   String url,
          before;

   size_t level = LEVEL_INVALID,
          levelBeforeURL = LEVEL_INVALID;

   bool nextWrapped = false;

   do
   {
      if ( m_options.highlightURLs )
      {
         // extract the first URL into url string and put all preceding
         // text into before, text is updated to contain only the text
         // after the URL
         before = strutil_findurl(text, url);
      }
      else // no URL highlighting
      {
         before = text;

         text.clear();
      }

      if ( m_options.quotedColourize )
      {
         // if we have just inserted an URL, restore the same level we were
         // using before as otherwise foo in a line like "> URL foo" wouldn't
         // be highlighted correctly
         if ( levelBeforeURL != LEVEL_INVALID )
         {
            level = levelBeforeURL;
            levelBeforeURL = LEVEL_INVALID;
         }
         else // no preceding URL, we're really at the start of line
         {
            if ( nextWrapped )
               nextWrapped = false;
            else
               level = GetQuotedLevel(before, NULL, &nextWrapped);
         }

         style.SetTextColour(GetQuoteColour(level));

         // lineCur is the start of the current line, lineNext of the next one
         const wxChar *lineCur = before.c_str();
         const wxChar *lineNext = wxStrchr(lineCur, '\n');
         while ( lineNext )
         {
            // skip '\n'
            lineNext++;

            // calculate the quoting level for this line
            if ( nextWrapped )
            {
               // quoting level doesn't change anyhow
               nextWrapped = false;
            }
            else
            {
               size_t levelNew = GetQuotedLevel(lineNext, lineCur, &nextWrapped);
               if ( levelNew != level )
               {
                  String line(lineCur, lineNext);
                  m_next->Process(line, viewer, style);

                  level = levelNew;
                  style.SetTextColour(GetQuoteColour(level));

                  lineCur = lineNext;
               }
               //else: same level as the previous line, just continue
            }

            if ( !*lineNext )
            {
               // nothing left
               break;
            }

            // we can use +1 here because there must be '\r' before the next
            // '\n' anyhow, i.e. the very next char can't be '\n'
            lineNext = wxStrchr(lineNext + 1, '\n');
         }

         if ( lineCur )
         {
            String line(lineCur);
            m_next->Process(line, viewer, style);
         }

         // remember the current quoting level to be able to restore it later
         levelBeforeURL = level;
      }
      else // no quoted text colourizing
      {
         m_next->Process(before, viewer, style);
      }

      if ( !strutil_isempty(url) )
      {
         // we use the URL itself for text here
         m_next->ProcessURL(url, url, viewer);
      }
   }
   while ( !text.empty() );
}

