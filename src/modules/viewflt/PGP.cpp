///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   PGP.cpp: implementation of PGP viewer filter
// Purpose:     PGP filter handles ASCII armoured PGP messages
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

#include "ViewFilter.h"

// ----------------------------------------------------------------------------
// PGPFilter declaration
// ----------------------------------------------------------------------------

class PGPFilter : public ViewFilter
{
public:
   PGPFilter(MessageView *msgView, ViewFilter *next, bool enable)
      : ViewFilter(msgView, next, enable) { }

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// all PGP messages in ASCII armour start with a line whose prefix is this
#define PGP_BEGIN_PREFIX "-----BEGIN PGP "

// .. and suffix is that
#define PGP_BEGIN_SUFFIX "MESSAGE-----"

// PGP signature block starts with this line
#define PGP_BEGIN_SIG PGP_BEGIN_PREFIX PGP_END_SIG_SUFFIX

// end of the PGP encrypted message or signature block starts with this
#define PGP_END_PREFIX "-----END PGP "

// end ends with either of those
#define PGP_END_CRYPT_SUFFIX "MESSAGE-----"
#define PGP_END_SIG_SUFFIX "SIGNATURE-----"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// PGPFilter
// ----------------------------------------------------------------------------

// this filter has a rather high priority because it should be applied before
// all the filters working on the "real" message body, but after the trailer
// filter because the mailing list trailers appear after the entire PGP encoded
// message
IMPLEMENT_VIEWER_FILTER(PGPFilter,
                        ViewFilter::Priority_High + 10,
                        false,      // initially disabled
                        _("PGP"),
                        "(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>");

void
PGPFilter::DoProcess(String& text,
                     MessageViewer *viewer,
                     MTextStyle& style)
{
   // do we have something looking like a PGP message?
   //
   // there should be a BEGIN line near the start of the message
   bool foundBegin = false;
   const char *start = text.c_str();
   for ( size_t numLines = 0; numLines < 10; numLines++ )
   {
      if ( strncmp(start, PGP_BEGIN_PREFIX, strlen(PGP_BEGIN_PREFIX)) == 0 )
      {
         foundBegin = true;
         break;
      }

      // try the next line
      start = strchr(start + 1, '\n');
      if ( start )
         start++; // skip '\n' itself
   }

   if ( foundBegin )
   {
      // is the message just signed or encrypted?
      const char *tail = start + strlen(PGP_BEGIN_PREFIX);
      bool isSigned = strncmp(tail, "SIGNED ", 7 /* strlen("SIGNED ") */) == 0;
      if ( isSigned )
      {
         tail += 7;
      }

      // this flag tells us if everything is ok so far -- as soon as it becomes
      // false, we skip all subsequent steps
      bool ok = true;
      if ( strncmp(tail, PGP_BEGIN_SUFFIX, strlen(PGP_BEGIN_SUFFIX)) != 0 )
      {
         wxLogWarning(_("The BEGIN line doesn't end with expected suffix."));

         ok = false;
      }

      // end of the PGP part
      const char *end;
      if ( ok ) // ok, it starts with something valid
      {
         // now locate the end line
         const char *pc = start + text.length() - 1;

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

            if ( strncmp(pc, PGP_END_PREFIX, strlen(PGP_END_PREFIX)) == 0 )
            {
               tail = pc + strlen(PGP_END_PREFIX);

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

      // check that the END line matches the BEGIN one
      if ( ok )
      {
         const char * const suffix = isSigned ? PGP_END_SIG_SUFFIX
                                              : PGP_END_CRYPT_SUFFIX;

         if ( strncmp(tail, suffix, strlen(suffix)) != 0 )
         {
            wxLogWarning(_("Mismatch between BEGIN and END lines."));

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

         if ( isSigned )
         {
            // TODO: pass everything between start and end to PGP for
            //       verification

            // if ( verified ok )
            {
               // remove the PGP signature and show just the text

               // (a) skip the header: it ends at first blank line
               const char *pc = start;
               while ( pc && (pc[1] != '\r' || pc[2] != '\n') )
               {
                  pc = strchr(pc + 1, '\n');
               }

               if ( !pc )
               {
                  FAIL_MSG( _T("no blank line after PGP BEGIN line?") );
                  pc = start;
               }
               else
               {
                  // skip "\n\r\n"
                  pc += 3;
               }

               // (b) cut off the signature block
               end -= 2; // skip "\r\n"
               for ( ;; )
               {
                  while ( *end != '\n' && end >= start )
                  {
                     end--;
                  }

                  end++;

                  if ( strncmp(end, PGP_BEGIN_SIG, strlen(PGP_BEGIN_SIG)) == 0 )
                     break;

                  if ( end == start )
                  {
                     FAIL_MSG( _T("no PGP signature block in signed msg?") );

                     end = start + text.length() - 1;
                  }

                  end -= 2; // skip "\r\n" again
               }

               m_next->Process(String(pc, end), viewer, style);
            }
            //else: complain about bad signature
         }
         else // encrypted
         {
            // TODO: decrypt the message between start and end into textClear
            String textClear = "PGP encrypted messages not supported yet.\r\n";

            m_next->Process(textClear, viewer, style);
         }
      }

      if ( ok )
      {
         // skip the normal display below
         return;
      }

      // give a warning and display the message normally
      wxLogWarning(_("This message seems to be PGP signed or encrypted "
                     "but in fact is not."));
   }

   m_next->Process(text, viewer, style);
}

