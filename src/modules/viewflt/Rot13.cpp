///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Rot13Filter.cpp: implementation of ROT13 viewer filter
// Purpose:     Rot13Filter is more a test bed for filters than anything useful
// Author:      Vadim Zeitlin
// Modified by:
// Created:     28.11.02
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
#endif //USE_PCH

#include "ViewFilter.h"

// ----------------------------------------------------------------------------
// Rot13Filter declaration
// ----------------------------------------------------------------------------

class Rot13Filter : public ViewFilter
{
public:
   Rot13Filter(MessageView *msgView, ViewFilter *next, bool enable)
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
// Rot13Filter
// ----------------------------------------------------------------------------

// this filter has a very low priority, it should be applied after all the
// others to avoid mangling the URLs and what not
IMPLEMENT_VIEWER_FILTER(Rot13Filter,
                        ViewFilter::Priority_Low,
                        false,      // initially disabled
                        gettext_noop("ROT13"),
                        _T("(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>"));

void
Rot13Filter::DoProcess(String& text,
                       MessageViewer *viewer,
                       MTextStyle& style)
{
   for ( char *pc = (char *)text.c_str(); *pc; pc++ )
   {
      // don't use locale-dependent version isalpha() here
      char base;
      if ( *pc >= 'a' && *pc <= 'z' )
         base = 'a';
      else if ( *pc >= 'A' && *pc <= 'Z' )
         base = 'A';
      else
         continue;

      // apply ROT 13 algorithm: shift all letters by 13 positions mod 26
      *pc = base + ((*pc - base) + 13) % 26;
   }

   m_next->Process(text, viewer, style);
}

