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

   #include <wx/menu.h>
#endif //USE_PCH

#include "MessageView.h"
#include "MessageViewer.h"
#include "ViewFilter.h"

#include "modules/MCrypt.h"

#include "PGPClickInfo.h"


// ----------------------------------------------------------------------------
// PGPMenu: used by ClickablePGPInfo
// ----------------------------------------------------------------------------

class PGPMenu : public wxMenu
{
public:
   PGPMenu(const ClickablePGPInfo *pgpInfo, const wxChar *title);

   void OnCommandEvent(wxCommandEvent &event);

private:
   // menu command ids
   enum
   {
      RAW_TEXT,
      DETAILS
   };

   const ClickablePGPInfo * const m_pgpInfo;

   DECLARE_EVENT_TABLE()
};

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
// PGPFilter implementation
// ============================================================================

// this filter has a rather high priority because it should be applied before
// all the filters working on the "real" message body, but after the trailer
// filter because the mailing list trailers appear after the entire PGP encoded
// message
IMPLEMENT_VIEWER_FILTER(PGPFilter,
                        ViewFilter::Priority_High + 10,
                        false,      // initially disabled
                        _("PGP"),
                        "(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>");

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
      else
         break; // no more text
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
      const char *end = NULL; // unneeded but suppresses the compiler warning
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

         CHECK_RET( m_engine, _T("PGP filter can't work without PGP engine") );

         ClickablePGPInfo *pgpInfo = NULL;
         MCryptoEngineOutputLog *log = new MCryptoEngineOutputLog;

         String in(start, end),
                out;
         if ( isSigned )
         {
            // pass everything between start and end to PGP for verification
            switch ( m_engine->VerifySignature(in, out, log) )
            {
               case MCryptoEngine::OK:
                  // create an icon for the sig just to show that it was there
                  pgpInfo = new PGPInfoGoodSig(m_msgView, log->GetUserID());
                  break;

               case MCryptoEngine::SIGNATURE_EXPIRED_ERROR:
                  pgpInfo = new PGPInfoExpiredSig(m_msgView, log->GetUserID());
                  break;

               case MCryptoEngine::SIGNATURE_UNTRUSTED_WARNING:
                  pgpInfo = new PGPInfoUntrustedSig(m_msgView, log->GetUserID());
                  break;

               default:
                  // use unmodified text
                  out = in;

                  // but still create an icon showing that signature check
                  // failed
                  pgpInfo = new PGPInfoBadSig(m_msgView, log->GetUserID());
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

         if ( pgpInfo )
         {
            pgpInfo->SetLog(log);
            pgpInfo->SetRaw(in);
            // we want the PGP stuff to stand out
            viewer->InsertText(_T("\r\n"), style);

            viewer->InsertClickable(pgpInfo->GetBitmap(),
                                    pgpInfo,
                                    pgpInfo->GetColour());

            viewer->InsertText(_T("\r\n"), style);
         }
         else
         {
            delete log;
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

// ============================================================================
// ClickablePGPInfo
// ============================================================================

// ----------------------------------------------------------------------------
// ClickablePGPInfo ctor/dtor
// ----------------------------------------------------------------------------

ClickablePGPInfo::ClickablePGPInfo(MessageView *msgView,
                                   const String& label,
                                   const String& bmpName,
                                   const wxColour& colour)
                : ClickableInfo(msgView),
                  m_label(label),
                  m_bmpName(bmpName),
                  m_colour(colour)
{
   m_log = NULL;
}

ClickablePGPInfo::~ClickablePGPInfo()
{
   delete m_log;
}

// ----------------------------------------------------------------------------
// ClickablePGPInfo accessors
// ----------------------------------------------------------------------------

wxBitmap
ClickablePGPInfo::GetBitmap() const
{
   return mApplication->GetIconManager()->GetBitmap(m_bmpName);
}

wxColour
ClickablePGPInfo::GetColour() const
{
   return m_colour;
}

String
ClickablePGPInfo::GetLabel() const
{
   return m_label;
}

// ----------------------------------------------------------------------------
// ClickablePGPInfo operations
// ----------------------------------------------------------------------------

void
ClickablePGPInfo::OnRightClick(const wxPoint& pt) const
{
   PGPMenu menu(this, m_label);

   m_msgView->GetWindow()->PopupMenu(&menu, pt);
}

void
ClickablePGPInfo::ShowDetails() const
{
   // TODO: something better
   if ( m_log )
   {
      String allText;
      allText.reserve(4096);

      const size_t count = m_log->GetMessageCount();
      for ( size_t n = 0; n < count; n++ )
      {
         allText << m_log->GetMessage(n) << _T('\n');
      }

      MDialog_ShowText(m_msgView->GetWindow(),
                       _("PGP Information"),
                       allText,
                       _T("PGPDetails"));
   }
   else // no log??
   {
      wxLogMessage(_("Sorry, no PGP details available."));
   }
}

void
ClickablePGPInfo::ShowRawText() const
{
   MDialog_ShowText(m_msgView->GetWindow(),
                    _("PGP Message Raw Text"),
                    m_textRaw,
                    _T("PGPRawText"));
}

// ============================================================================
// PGPMenu implementation
// ============================================================================

BEGIN_EVENT_TABLE(PGPMenu, wxMenu)
   EVT_MENU(-1, PGPMenu::OnCommandEvent)
END_EVENT_TABLE()

PGPMenu::PGPMenu(const ClickablePGPInfo *pgpInfo, const wxChar *title)
       : wxMenu(wxString::Format(_("PGP: %s"), title)),
         m_pgpInfo(pgpInfo)
{
   // create the menu items
   Append(RAW_TEXT, _("Show ra&w text..."));
   AppendSeparator();
   Append(DETAILS, _("&Details..."));
}

void
PGPMenu::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      case DETAILS:
         m_pgpInfo->ShowDetails();
         break;

      case RAW_TEXT:
         m_pgpInfo->ShowRawText();
         break;

      default:
         FAIL_MSG( _T("unexpected command in PGPMenu") );
   }
}

