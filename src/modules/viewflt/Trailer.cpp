///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Trailer.cpp: implementation of trailer viewer filter
// Purpose:     Trailer handles the tails appended to the end of the message
// Author:      Vadim Zeitlin
// Modified by:
// Created:     30.11.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
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
#endif //USE_PCH

#include "MTextStyle.h"
#include "MessageViewer.h"

#include "ViewFilter.h"

// ----------------------------------------------------------------------------
// TrailerFilter declaration
// ----------------------------------------------------------------------------

class TrailerFilter : public ViewFilter
{
public:
   TrailerFilter(MessageView *msgView, ViewFilter *next, bool enable)
      : ViewFilter(msgView, next, enable) { }

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// TrailerFilter
// ----------------------------------------------------------------------------

// this filter has a high priority as it should be normally applied before
// all the filters working on the message body
IMPLEMENT_VIEWER_FILTER(TrailerFilter,
                        ViewFilter::Priority_High + 20,
                        true,      // initially enabled
                        gettext_noop("Trailer"),
                        _T("(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>"));

void
TrailerFilter::DoProcess(String& text,
                         MessageViewer *viewer,
                         MTextStyle& style)
{
   // we try to detect a line formed by only dashes or underscores not too far
   // away from the message end

   // we assume the string is non empty below
   if ( text.empty() )
      return;

   const wxChar *start = text.c_str();
   const wxChar *pc = start + text.length() - 1;

   // while we're not too far from end
   for ( size_t numLinesFromEnd = 0; numLinesFromEnd < 10; numLinesFromEnd++ )
   {
      // does this seem to be a separator line?
      char chDel = *pc;
      if ( chDel == '-' || chDel == '_' )
      {
         // look for the start of this line checking if it has only delimiter
         // characters:
         size_t lenDel = 0;
         while ( *pc != '\n' && pc > start )
         {
            if ( *pc-- != chDel )
            {
               // it's not a delimiter line, finally
               lenDel = 0;
               break;
            }

            lenDel++;
         }

         // did we find a delimiter line?
         if ( lenDel >= 40 )
         {
            // yes, but it may start either at pc or at pc + 1
            if ( *pc == '\n' )
               pc++;

            // remember the tail and cut it off
            String tail = pc;
            text.resize(pc - start);

            // trailers may be embedded, so call ourselves recursively to check
            // for them again
            Process(text, viewer, style);

            // the main message text ends here
            m_next->EndText();

            // and now show the trailer in special style
            wxColour colOld = style.GetTextColour();
            style.SetTextColour(*wxLIGHT_GREY); // TODO: allow to customize

            m_next->Process(tail, viewer, style);

            style.SetTextColour(colOld);

            // done!
            return;
         }
      }

      // if we get here, it was not a delimiter line
      //
      // rewind to its beginning
      while ( *pc != '\n' && pc > start )
      {
         pc--;
      }

      if ( pc <= start )
      {
         // we came to the very beginning of the message and found nothing
         break;
      }

      // continue going backwards after skipping the new line ("\r\n")
      ASSERT_MSG( *pc == '\n', _T("why did we stop then?") );

      // skip '\n' and '\r' if it's present -- surprizingly enough, we might
      // not have it (this happens to me inside a PGP encrypted message)
      if ( *--pc == '\r' )
      {
         // skip '\r' as well
         --pc;
      }
   }

   // nothing found, process the rest normally
   m_next->Process(text, viewer, style);
}

