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

/** Count levels of quoting on the first line of passed string
    (i.e. before the first \n). It understands standard e-mail
    quoting methods such as ">" and "XY>"

    @param string the string to check
    @param max_white max number of white characters before quotation mark
    @param max_alpha max number of A-Z characters before quotation mark
    @return number of quoting levels (0 for unquoted text)
  */
static
int CountQuoteLevel(const wxChar *string, int max_white, int max_alpha);


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

   // helpers
   size_t GetQuotedLevel(const wxChar *text) const;
   wxColour GetQuoteColour(size_t qlevel) const;

   Options m_options;
};

// ============================================================================
// CountQuoteLevel implementation
// ============================================================================

int
CountQuoteLevel(const wxChar *string, int max_white, int max_alpha)
{
   int levels = 0;

   for ( const wxChar *c = string; *c != 0 && *c != '\n'; c++)
   {
      // skip white space
      int num_white = 0;
      while ( *c == '\t' || *c == ' ' )
      {
         if ( ++num_white > max_white )
         {
            // too much whitespace for this to be a quoted string
            return levels;
         }

         c++;
      }

      // skip optional alphanumeric prefix
      int num_alpha = 0;
      while ( isalpha((unsigned char)*c) )
      {
         if ( ++num_alpha > max_alpha )
         {
            // prefix too long, not start of the quote
            return levels;
         }

         c++;
      }

      // check if we have a quoted line or not now: normally it is enough to
      // simply check whether the first character is one of the admitted
      // "quoting characters" but for ')' and '*' (which some people do use to
      // quote!) we have to also check the next character as otherwise it
      // results in too many false positives
      //
      // TODO: make the string of "quoting characters" configurable
      if ( *c != '>' && *c != '|' && *c != '}' &&
            (*c != ')'  || c[1] != ' ') )
//            ((*c != ')' && *c != '*') || c[1] != ' ') )  // '* ' still gives too many false positives
      {
         // not quoted (according to our heuristics anyhow)
         break;
      }

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
QuoteURLFilter::GetQuotedLevel(const wxChar *text) const
{
   size_t qlevel = CountQuoteLevel
                   (
                     text,
                     m_options.quotedMaxWhitespace,
                     m_options.quotedMaxAlpha
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

   size_t levelBeforeURL = LEVEL_INVALID;

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
         size_t level;

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
            level = GetQuotedLevel(before);
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
            size_t levelNew = GetQuotedLevel(lineNext);
            if ( levelNew != level )
            {
               String line(lineCur, lineNext);
               m_next->Process(line, viewer, style);

               level = levelNew;
               style.SetTextColour(GetQuoteColour(level));

               lineCur = lineNext;
            }
            //else: same level as the previous line, just continue

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
         viewer->InsertURL(url, url);
      }
   }
   while ( !text.empty() );
}

