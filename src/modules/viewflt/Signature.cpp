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
#   include "Mcommon.h"
#   include "Mdefaults.h"
#endif //USE_PCH

#include "MTextStyle.h"
#include "ViewFilter.h"
#include "MessageViewer.h"

#include "miscutil.h"         // for GetColourByName()

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_HIGHLIGHT_SIGNATURE;
extern const MOption MP_MVIEW_SIGCOLOUR;

// ----------------------------------------------------------------------------
// Signature declaration
// ----------------------------------------------------------------------------

class SignatureFilter : public ViewFilter
{
public:
   SignatureFilter(MessageView *msgView, ViewFilter *next, bool enable)
      : ViewFilter(msgView, next, enable)
   {
      ReadOptions(msgView->GetProfile());
   }

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);

   // fill m_options using the values from the given profile
   void ReadOptions(Profile *profile);

   struct Options
   {
      // the colour to use for signatures
      wxColour SigCol;
   } m_options;
};

// ============================================================================
// SignatureFilter implementation
// ============================================================================

// ----------------------------------------------------------------------------
// SignatureFilter options
// ----------------------------------------------------------------------------

void
SignatureFilter::ReadOptions(Profile *profile)
{
   // a macro to make setting many colour options less painful
   #define GET_COLOUR_FROM_PROFILE(col, name) \
      GetColourByName(&col, \
                      READ_CONFIG(profile, MP_MVIEW_##name), \
                      GetStringDefault(MP_MVIEW_##name))

   GET_COLOUR_FROM_PROFILE(m_options.SigCol, SIGCOLOUR);

   #undef GET_COLOUR_FROM_PROFILE

   if ( !READ_CONFIG_BOOL(profile, MP_HIGHLIGHT_SIGNATURE) )
   {
      Enable(false);
   }
}

// ----------------------------------------------------------------------------
// SignatureFilter work function
// ----------------------------------------------------------------------------

// this filter has a high priority as it should be normally applied before
// all the filters working on the message body -- but after the filters
// modifying the message "structure" such as TrailerFilter or PGPFilter
IMPLEMENT_VIEWER_FILTER(SignatureFilter,
                        ViewFilter::Priority_Default + 10,
                        true,      // initially enabled
                        gettext_noop("Signature"),
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
      //     unfortunately used by some people.
      //     But we always make sure that the line ends just after.
      if ( pc[0] == '-' && pc[1] == '-' &&
               ((pc[2] == ' ' && (pc[3] == '\r' || pc[3] == '\n')) || pc[2] == '\r' || pc[2] == '\n') )
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

      // skip '\n' and '\r' if it's present -- surprizingly enough, we might
      // not have it (this happens to me inside a PGP encrypted message)
      if ( *--pc == '\r' )
      {
         // skip '\r' as well
         --pc;
      }
   }

   // first show the main text normally
   m_next->Process(text, viewer, style);

   // and then show the signature specially, if any
   if ( !signature.empty() )
   {
      // the main message text ends here
      viewer->EndText();

      // and now show the trailer in special style
      wxColour colOld = style.GetTextColour();
      style.SetTextColour(m_options.SigCol);

      m_next->Process(signature, viewer, style);

      style.SetTextColour(colOld);
   }
}

