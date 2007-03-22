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

#include "QuotedText.h"

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
       READ_CONFIG(profile, MP_MVIEW_QUOTED_MAXWHITESPACE);
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
         // insert the text before URL (endURL is the end of previous URL, not
         // this one or the start of line initially)
         String textBefore(endURL, startURL);
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
      String textAfter(endURL, lineNext ? lineNext + 1
                                        : text.c_str() + text.length());
      m_next->Process(textAfter, viewer, style);

      if ( !lineNext )
         break;

      // go to the next line (skip '\n')
      lineCur = lineNext + 1;
   }
}

