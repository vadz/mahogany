///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   TextMarkupFilter.cpp: trivial text markup viewer filter
// Purpose:     trivial text markup is the USENET tradition of using *bold*
//              and _italic_ markup in the text
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.12.02
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

#include "ViewFilter.h"

#include "MTextStyle.h"

// ----------------------------------------------------------------------------
// TextMarkupFilter declaration
// ----------------------------------------------------------------------------

class TextMarkupFilter : public ViewFilter
{
public:
   TextMarkupFilter(MessageView *msgView, ViewFilter *next, bool enable)
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
// TextMarkupFilter
// ----------------------------------------------------------------------------

// this filter has a low priority, it should be applied after all the
// others to avoid interfering with the URLs -- but it should be applied before
// Rot13 filter
IMPLEMENT_VIEWER_FILTER(TextMarkupFilter,
                        ViewFilter::Priority_Low + 10,
                        true,      // enabled by default
                        _("Trivial Markup"),
                        "(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>");

// render all the words surrounded by asterisks/underscores in bold/italic font
void
TextMarkupFilter::DoProcess(String& text,
                            MessageViewer *viewer,
                            MTextStyle& style)
{
   enum
   {
      Normal,
      Bold,
      Italic
   } state = Normal;

   // the '*' or '_' last encountered
   wxChar chLastSpecial = _T('\0'); // unneeded but silence compiler warning

   // are we at the start of a new word?
   bool atWordStart = true;

   String textNormal,
          textSpecial;
   for ( const wxChar *pc = text.c_str(); ; pc++ )
   {
      bool isSpace = false;

      switch ( *pc )
      {
         case _T('*'):
         case _T('_'):
            if ( state == Normal )
            {
               // is it at start of a word?
               if ( atWordStart )
               {
                  // yes: treat it as a markup character

                  // output the normal text we had so far
                  m_next->Process(textNormal, viewer, style);
                  textNormal.clear();

                  // change state
                  chLastSpecial = *pc;
                  state = chLastSpecial == _T('*') ? Bold : Italic;
               }
               else // no, it's in the middle of the word
               {
                  // treat it as a normal character then;
                  textNormal += *pc;
               }
            }
            else // ending markup tag?
            {
               if ( *pc != chLastSpecial )
               {
                  // markup mismatch -- consider there was no intention to use
                  // the last special character for markup at all
                  textNormal = chLastSpecial + textSpecial + *pc;
               }
               else // matching tag
               {
                  // output the highlighted text
                  wxFont fontOld = style.GetFont();

                  wxFont font = fontOld;
                  if ( chLastSpecial == _T('*') )
                     font.SetWeight(wxFONTWEIGHT_BOLD);
                  else // italic
                     font.SetStyle(wxFONTSTYLE_ITALIC);

                  style.SetFont(font);
                  m_next->Process(textSpecial, viewer, style);
                  style.SetFont(fontOld);
               }

               textSpecial.clear();
               state = Normal;
            }
            break;

         case _T(' '):
         case _T('\t'):
         case _T('\r'):
         case _T('\n'):
            isSpace = true;
            // fall through

         case _T('\0'):
            // as a space can't appear inside a marked up word, we decide
            // that the last special character wasn't meant to begin the markup
            // after all
            if ( state != Normal )
            {
               textNormal = chLastSpecial + textSpecial;
               textSpecial.clear();
               state = Normal;
            }
            // fall through

         default:
            (state == Normal ? textNormal : textSpecial) += *pc;
      }

      if ( *pc == _T('\0') )
      {
         // end of text
         ASSERT_MSG( state == Normal, _T("logic error in TextMarkupFilter") );

         m_next->Process(textNormal, viewer, style);

         break;
      }

      atWordStart = isSpace;
   }
}

