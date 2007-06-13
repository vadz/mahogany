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
#   include "Mcommon.h"
#endif //USE_PCH

#include "MessageViewer.h"
#include "ViewFilter.h"

#include "PGPClickInfo.h"

#include <wx/bitmap.h>

// ----------------------------------------------------------------------------
// PGPFilter itself
// ----------------------------------------------------------------------------

class PGPFilter : public ViewFilter
{
public:
   PGPFilter(MessageView *msgView, ViewFilter *next, bool enable);

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style);

   MCryptoEngine *m_engine;
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// all PGP messages in ASCII armour start with a line whose prefix is this
#define PGP_BEGIN_PREFIX _T("-----BEGIN PGP ")

// .. and suffix is that
#define PGP_BEGIN_SUFFIX _T("MESSAGE-----")

// PGP signature block starts with this line
#define PGP_BEGIN_SIG PGP_BEGIN_PREFIX PGP_END_SIG_SUFFIX

// end of the PGP encrypted message or signature block starts with this
#define PGP_END_PREFIX _T("-----END PGP ")

// end ends with either of those
#define PGP_END_CRYPT_SUFFIX _T("MESSAGE-----")
#define PGP_END_SIG_SUFFIX _T("SIGNATURE-----")

// ----------------------------------------------------------------------------
// module global functions
// ----------------------------------------------------------------------------

// checks whether the given string starts with this substring and advances it
// past it if it does
static inline bool AdvanceIfMatches(const wxChar **p, const wxChar *substr)
{
   const size_t len = wxStrlen(substr);
   if ( wxStrncmp(*p, substr, len) != 0 )
      return false;

   *p += len;

   return true;
}

// ============================================================================
// PGPFilter implementation
// ============================================================================

// this filter has a rather high priority because it should be applied before
// all the filters working on the "real" message body, but after the trailer
// filter because the mailing list trailers appear after the entire PGP encoded
// message
IMPLEMENT_VIEWER_FILTER(PGPFilter,
                        ViewFilter::Priority_High + 10,
                        false,      // initially disabled
                        gettext_noop("PGP"),
                        _T("(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>"));

// ----------------------------------------------------------------------------
// PGPFilter ctor
// ----------------------------------------------------------------------------

PGPFilter::PGPFilter(MessageView *msgView, ViewFilter *next, bool enable)
         : ViewFilter(msgView, next, enable)
{
   MModuleListing *listing =
      MModule::ListAvailableModules(CRYPTO_ENGINE_INTERFACE);
   if ( listing )
   {
      const size_t count = listing->Count();
      for ( size_t n = 0; n < count; n++ )
      {
         const MModuleListingEntry& entry = (*listing)[n];

         const String& name = entry.GetName();
         if ( name == _T("PGPEngine") )
         {
            MCryptoEngineFactory * const factory
               = (MCryptoEngineFactory *)MModule::LoadModule(name);
            if ( factory )
            {
               m_engine = factory->Get();

               factory->DecRef();
            }
            else
            {
               FAIL_MSG( _T("failed to create PGPEngineFactory") );
            }
         }
      }

      listing->DecRef();
   }

   if ( !m_engine )
   {
      wxLogError(_("Failed to load PGP engine module.\n"
                   "\n"
                   "Support for PGP in message viewer will be unavailable") );

      // TODO: disable completely
      Enable(false);
   }
}

// ----------------------------------------------------------------------------
// PGPFilter work function
// ----------------------------------------------------------------------------

void
PGPFilter::DoProcess(String& text,
                     MessageViewer *viewer,
                     MTextStyle& style)
{
   // do we have something looking like a PGP message?
   //
   // there should be a BEGIN line near the start of the message
   const wxChar *beginNext = NULL;
   const wxChar *start = text.c_str();
   for ( size_t numLines = 0; numLines < 10; numLines++ )
   {
      const wxChar *p = start;
      if ( AdvanceIfMatches(&p, PGP_BEGIN_PREFIX) )
      {
         beginNext = p;
         break;
      }

      // try the next line (but only if not already at the end)
      if ( !*p )
         break;

      p = wxStrchr(start, _T('\n'));
      if ( !p )
         break; // no more text

      start = p + 1; // skip '\n' itself
   }

   if ( beginNext )
   {
      // is the message just signed or encrypted?
      bool isKey = false;
      const bool isSigned = AdvanceIfMatches(&beginNext, _T("SIGNED "));
      if ( !isSigned )
      {
         isKey = AdvanceIfMatches(&beginNext, _T("PUBLIC KEY "));
      }

      // this flag tells us if everything is ok so far -- as soon as it becomes
      // false, we skip all subsequent steps
      // We do not know (yet) what to do with public key blocks, so let's consider
      // that they're not ok.
      // TODO: propose to import it into the keyring?
      bool ok = !isKey;

      if ( ok && !AdvanceIfMatches(&beginNext, PGP_BEGIN_SUFFIX) )
      {
         wxLogWarning(_("The BEGIN line doesn't end with expected suffix."));

         ok = false;
      }

      // end of the PGP part
      const wxChar *end = NULL; // unneeded but suppresses the compiler warning
      const wxChar *endNext = NULL; // same
      if ( ok ) // ok, it starts with something valid
      {
         // now locate the end line
         const wxChar *pc = start + text.length() - 1;

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

            if ( AdvanceIfMatches(&pc, PGP_END_PREFIX) )
            {
               endNext = pc;

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
         const wxChar * const suffix = isSigned ? PGP_END_SIG_SUFFIX
                                                : PGP_END_CRYPT_SUFFIX;

         if ( !AdvanceIfMatches(&endNext, suffix) )
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

         CHECK_RET( m_engine, _T("PGP filter can't work without PGP engine") );

         ClickablePGPInfo *pgpInfo = NULL;
         MCryptoEngineOutputLog *
            log = new MCryptoEngineOutputLog(m_msgView->GetWindow());

         String in(start, end),
                out;
         if ( isSigned )
         {
            // pass everything between start and end to PGP for verification
            const MCryptoEngine::Status
               rc = m_engine->VerifySignature(in, out, log);
            pgpInfo = ClickablePGPInfo::
                        CreateFromSigStatusCode(rc, m_msgView, log);

            // if we failed to check the signature, we need to remove the
            // BEGIN/END lines from output ourselves (otherwise it would have
            // been done by VerifySignature() itself)
            if ( rc != MCryptoEngine::OK )
            {
               // beginNext points to the end of BEGIN line, go forward to the
               // end of the headers (signalled by an empty line i.e. 2 EOLs)
               beginNext = wxStrstr(beginNext, _T("\r\n\r\n"));
               if ( beginNext )
               {
                  // endNext currently points to the end of END PGP SIGNATURE
                  // line, rewind to the PGP_BEGIN_SIG line
                  const wxChar *pc = endNext;
                  for ( ;; )
                  {
                     // find the beginning of this line
                     while ( *pc != '\n' && pc >= start )
                     {
                        pc--;
                     }

                     if ( pc < start )
                     {
                        // we exhausted the message without finding the
                        // PGP_BEGIN_SIG line
                        break;
                     }

                     pc++; // skip the "\n"

                     if ( AdvanceIfMatches(&pc, PGP_BEGIN_SIG) )
                     {
                        // chop off PGP_BEGIN_SIG line as well
                        while ( *pc != '\n' && pc >= start )
                           pc--;

                        if ( pc > start )
                        {
                           out = String(beginNext + 4, pc - 1);
                        }

                        break;
                     }

                     pc -= 3; // rewind beyond "\r\n"
                     ASSERT_MSG( pc[1] == '\r',
                                  _T("line doesn't end in\"\\r\\n\"?") );
                  }
               }
            }
         }
         else // encrypted
         {
            // try to decrypt
            MCryptoEngine::Status rc = m_engine->Decrypt(in, out, log);
            switch ( rc )
            {
               case MCryptoEngine::OK:
                  pgpInfo = new PGPInfoGoodMsg(m_msgView);
                  break;

               default:
                  wxLogError(_("Decrypting the PGP message failed."));
                  // fall through

               // if the user cancelled decryption, don't complain about it
               case MCryptoEngine::OPERATION_CANCELED_BY_USER:
                  // using unmodified text is not very helpful here anyhow so
                  // simply replace it with an icon
                  pgpInfo = new PGPInfoBadMsg(m_msgView);
            }
         }

         m_next->Process(out, viewer, style);

         pgpInfo->SetLog(log);
         pgpInfo->SetRaw(in);

         // we want the PGP stuff to stand out
         viewer->InsertText(_T("\r\n"), style);

         viewer->InsertClickable(pgpInfo->GetBitmap(),
                                 pgpInfo,
                                 pgpInfo->GetColour());

            viewer->InsertText(_T("\r\n"), style);

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

      // give a warning (unless this is a KEY blok and display the message normally
      if ( !isKey )
      {
         wxLogWarning(_("This message seems to be PGP signed or encrypted "
                        "but in fact is not."));
      }
   }

   m_next->Process(text, viewer, style);
}

