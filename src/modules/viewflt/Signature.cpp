///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Signature.cpp: implementation of trailer viewer filter
// Purpose:     Signature handles the signatures detection
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

#include "ViewFilter.h"

// ----------------------------------------------------------------------------
// Signature declaration
// ----------------------------------------------------------------------------

class SignatureFilter : public ViewFilter
{
public:
   SignatureFilter(ViewFilter *next, bool enable) : ViewFilter(next, enable) { }

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// SignatureFilter
// ----------------------------------------------------------------------------

// this filter has a high priority as it should be normally applied before
// all the filters working on the message body -- but after TrailerFilter which
// has Priority_High + 5
IMPLEMENT_VIEWER_FILTER(SignatureFilter,
                        ViewFilter::Priority_High + 4,
                        true,      // initially enabled
                        _("Signature"),
                        "(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>");

void
SignatureFilter::DoProcess(String& text,
                           MessageViewer *viewer,
                           MTextStyle& style)
{
   // we try to detect a line formed by 2 dashes and a space not too far
   // away from the message end

   // we assume the string is non empty below
   if ( text.empty() )
      return;

   String signature;

   const char *start = text.c_str();
   const char *pc = start + text.length() - 1;

   // while we're not too far from end
   for ( size_t numLinesFromEnd = 0; numLinesFromEnd < 10; numLinesFromEnd++ )
   {
      // look for the start of this line:
      while ( *pc != '\n' && pc >= start )
      {
         pc--;
      }

      // we took one char too many
      pc++;

      // is it a signature delimiter?
      //
      // NB: we accept "-- " (canonical) but also just "--" which is
      //     unfortunately used by some people
      if ( pc[0] == '-' && pc[1] == '-' &&
               (pc[2] == ' ' || pc[2] == '\r' || pc[2] == '\n') )
      {
         // remember the signature and cut it off
         signature = pc;
         text.resize(pc - start);

         break;
      }
      //else: no

      if ( pc == start )
      {
         // we came to the very beginning of the message and found nothing
         break;
      }

      // undo pc++ from above
      pc--;

      // continue going backwards after skipping the new line ("\r\n")
      ASSERT_MSG( *pc == '\n', _T("why did we stop then?") );
      ASSERT_MSG( pc[-1] == '\r', _T("line doesn't end in\"\\r\\n\"?") );

      pc -= 2; // '\r' as well
   }

   // first show the main text normally
   m_next->Process(text, viewer, style);

   // and then show the signature specially, if any
   if ( !signature.empty() )
   {
      // and now show the trailer in special style
      wxColour colOld = style.GetTextColour();
      style.SetTextColour(*wxBLUE);

      m_next->Process(signature, viewer, style);

      style.SetTextColour(colOld);
   }
}

