///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   UUDecode.cpp: implementation of UU decoding viewer filter
// Purpose:     Decoding attachments
// Author:      Xavier Nodet
// Modified by:
// Created:     11.05.04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Xavier Nodet <xavier.nodet@free.fr>
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

#include "MessageViewer.h"
#include "ViewFilter.h"
#include "ClickAtt.h"

#include <wx/bitmap.h>

// ----------------------------------------------------------------------------
// UUDecodeFilter itself
// ----------------------------------------------------------------------------

class UUDecodeFilter : public ViewFilter
{
public:
   UUDecodeFilter(MessageView *msgView, ViewFilter *next, bool enable);

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// all UUencoded data start with a line whose prefix is this
#define UU_BEGIN_PREFIX _T("begin ")

// end of the UU encoded data is with a line equal to this
#define UU_END_PREFIX _T("end")

// ============================================================================
// UUDecodeFilter implementation
// ============================================================================

IMPLEMENT_VIEWER_FILTER(UUDecodeFilter,
                        ViewFilter::Priority_High - 5,
                        true,      // initially disabled
                        gettext_noop("UUdecode"),
                        _T("(c) 2004 Xavier Nodet <xavier.nodet@free.fr>"));

// ----------------------------------------------------------------------------
// UUDecodeFilter ctor
// ----------------------------------------------------------------------------

UUDecodeFilter::UUDecodeFilter(MessageView *msgView, ViewFilter *next, bool enable)
         : ViewFilter(msgView, next, enable)
{}

// ----------------------------------------------------------------------------
// UUDecodeFilter work function
// ----------------------------------------------------------------------------

void
UUDecodeFilter::DoProcess(String& text,
                     MessageViewer *viewer,
                     MTextStyle& style)
{
   // do we have something looking like UUencoded data?
   //
   // there should be a BEGIN line near the start of the message
   bool foundBegin = false;
   const wxChar *start = text.c_str();
   for ( size_t numLines = 0; numLines < 10; numLines++ )
   {
      if ( wxStrncmp(start, UU_BEGIN_PREFIX, wxStrlen(UU_BEGIN_PREFIX)) == 0 )
      {
         foundBegin = true;
         break;
      }

      // try the next line (but only if not already at the end)
      if ( *start )
         start = wxStrchr(start, '\n');
      else
         break;
      if ( start )
         start++; // skip '\n' itself
      else
         break; // no more text
   }

   if ( foundBegin )
   {
      const wxChar *tail = start + wxStrlen(UU_BEGIN_PREFIX);
      // this flag tells us if everything is ok so far -- as soon as it becomes
      // false, we skip all subsequent steps
      bool ok = true;

      // Let's check that the next 4 chars after the 'begin ' are
      // digit, digit, digit, space
      if ( ok && !(    tail[0] >= '0' && tail[0] <= '9'
                    && tail[1] >= '0' && tail[1] <= '9'
                    && tail[2] >= '0' && tail[2] <= '9'
                    && tail[3] == ' ' ) )
      {
         wxLogWarning(_("The BEGIN line is not correctly formed."));

         ok = false;
      }

      String fileName;
      if ( ok )
      {
         const wxChar* startName = tail+4;
         tail = startName;
         // Rest of the line is the name
         while (*tail != '\r')
         {
           ++tail;
           ASSERT_MSG( tail < text.c_str()+text.Length(), _T("'begin' line does not end?") );
         }
         ASSERT_MSG( tail[1] == '\n', _T("'begin' line does not end with \"\\r\\n\"?") );
         fileName = String(startName, tail);
      }

      // end of the UU part
      const wxChar *end = NULL; // unneeded but suppresses the compiler warning
      if ( ok ) // ok, it starts with something valid
      {
         // now locate the end line
         const wxChar *pc = text.c_str() + text.Length() - 1;

         bool foundEnd = false;
         for ( ;; )
         {
            // optimistically suppose that this line will be the END one
            end = pc + 2;

            // find the beginning of this line
            while ( *pc != '\n' && pc >= start )
            {
               pc--;
            }

            // we took one extra char
            pc++;

            if ( wxStrncmp(pc, UU_END_PREFIX, wxStrlen(UU_END_PREFIX)) == 0 )
            {
               tail = pc + wxStrlen(UU_END_PREFIX);

               foundEnd = true;
               break;
            }

            // undo the above
            pc--;

            if ( pc < start )
            {
               // we exhausted the message without finding the END line, leave
               // foundEnd at false and exit the loop
               break;
            }

            pc--;
            ASSERT_MSG( *pc == '\r', _T("line doesn't end in\"\\r\\n\"?") );
         }

         if ( !foundEnd )
         {
            wxLogWarning(_("END line not found."));

            ok = false;
         }
      }

      // if everything was ok so far, continue with decoding/verifying
      if ( ok )
      {
         // output the part before the BEGIN line, if any
         String prolog(text.c_str(), start);
         if ( !prolog.empty() )
         {
            m_next->Process(prolog, viewer, style);
         }

#if 0
         String decodedData; // TODO: decode, of course...
         MimePartText* decodedMime = new MimePartText(decodedData);
         ClickableAttachment *attachment = 
           new ClickableAttachment((MessageView*)0, decodedMime);

          // get the icon for the attachment using its 
          // filename extension (if any)
          wxIcon icon = mApplication->GetIconManager()->
                            GetIconFromMimeType("", fileName.AfterLast('.'));

          viewer->InsertAttachment(icon, attachment);
#else
          viewer->InsertText(_T("UUencoded file named \""), style);
          viewer->InsertText(fileName, style);
          viewer->InsertText(_T("\"\n"), style);
#endif
          // output the part after the END line, if any
          String epilog(end);
          if ( !epilog.empty() )
          {
              m_next->Process(epilog, viewer, style);
          }
      }

      if ( ok )
      {
         // skip the normal display below
         return;
      }

      wxLogWarning(_("Part of this message seems to be UU encoded "
                    "but in fact is not."));
   }

   m_next->Process(text, viewer, style);
}

