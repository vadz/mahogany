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
#   include "Mcommon.h"
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
                        gettext_noop("Trivial Markup"),
                        "(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>");

/*
   render all the words surrounded by asterisks/underscores in bold/italic font

   this is not as simple as it seems, however, because these characters may be
   used not for markup purposes in the text and we try hard to avoid any false
   positives while, OTOH, we want to highlight _stuff_like_this_ entirely
   italicized.
 */

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

   // the strings containing the text for the current state (normal or
   // highlighted)
   String textNormal,
          textSpecial;
   for ( const wxChar *pc = text.c_str(); ; pc++ )
   {
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

                  // change state and reset the associated info
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
               // should we highlight what we've got so far?
               if ( *pc != chLastSpecial || textSpecial.empty() )
               {
                  // markup mismatch or nothing between 2 delimiters --
                  // consider there was no intention to use the last special
                  // character for markup at all
                  textNormal = chLastSpecial + textSpecial + *pc;

                  state = Normal;
               }
               else // matching tag
               {
                  // is it really the end of markup or can it continue further?
                  if ( wxIsalnum(pc[1]) )
                  {
                     // it seems to continue, don't output anything for now

                     // this will be replaced by a space later if it turns out
                     // that it was indeed highlighted (i.e. if we find a
                     // matchin markup terminator later)
                     textSpecial += *pc;
                  }
                  else // end of markup
                  {
                     // output the highlighted text
                     wxFont fontOld = style.GetFont();

                     wxFont font = fontOld;
                     if ( chLastSpecial == _T('*') )
                        font.SetWeight(wxFONTWEIGHT_BOLD);
                     else // italic
                        font.SetStyle(wxFONTSTYLE_ITALIC);

                     // replace intermediate markup characters with spaces to
                     // render _the_multi_word_examples_ correctly
                     textSpecial.Replace(String(chLastSpecial), _T(" "));

                     style.SetFont(font);
                     m_next->Process(textSpecial, viewer, style);
                     style.SetFont(fontOld);

                     state = Normal;
                     textSpecial.clear();
                  }
               }
            }
            break;

         default:
            if ( state != Normal )
            {
               // only letters, numbers and some special symbols may appear
               // inside a marked up word
               //
               // apostrophe catches cases like "... *don't* do this ..."
               if ( !wxIsalnum(*pc) && *pc != _T('\'') )
               {
                  // we decide that the last special character wasn't meant
                  // to begin the markup after all

                  // don't forget to treat the markup symbol which isn't one
                  // literally
                  textNormal = chLastSpecial;

                  textNormal += textSpecial;
                  textSpecial.clear();
                  state = Normal;
               }
            }

            (state == Normal ? textNormal : textSpecial) += *pc;
      }

      if ( *pc == _T('\0') )
      {
         // end of text
         ASSERT_MSG( state == Normal, _T("logic error in TextMarkupFilter") );

         m_next->Process(textNormal, viewer, style);

         break;
      }

      atWordStart = wxIsspace(*pc);
   }
}

