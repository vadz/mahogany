///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   PGPClickInfo.cpp: implementation of ClickablePGPInfo
// Purpose:     ClickablePGPInfo is for "(inter)active" PGP objects which can
//              appear in MessageView
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.12.02 (extracted from viewfilt/PGP.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2002 Mahogany team
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
   #include "MApplication.h"
   #include "gui/wxIconManager.h"

   #include <wx/menu.h>
#endif //USE_PCH

#include "MessageView.h"
#include "gui/wxMDialogs.h"

#include "modules/MCrypt.h"

#include "PGPClickInfo.h"

extern const MOption MP_PGP_KEYSERVER;

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
   DECLARE_NO_COPY_CLASS(PGPMenu)
};

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

// ============================================================================
// ClickablePGPInfo implementation
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

/* static */
ClickablePGPInfo *
ClickablePGPInfo::CreateFromSigStatusCode(MCryptoEngine *engine,
                                          MCryptoEngine::Status code,
                                          MessageView *msgView,
                                          MCryptoEngineOutputLog *log)
{
   ClickablePGPInfo *pgpInfo;
   const String& user = log->GetUserID();

   switch ( code )
   {
      case MCryptoEngine::OK:
         // create an icon for the sig just to show that it was there
         pgpInfo = new PGPInfoGoodSig(msgView, user);
         break;

      case MCryptoEngine::OPERATION_CANCELED_BY_USER:
      case MCryptoEngine::CANNOT_EXEC_PROGRAM:
         pgpInfo = new PGPSignatureInfo
                       (
                        msgView,
                        _("Unverified PGP signature"),
                        "pgpsig_bad",
                        *wxLIGHT_GREY
                       );
         break;

      case MCryptoEngine::SIGNATURE_EXPIRED_ERROR:
         pgpInfo = new PGPInfoExpiredSig(msgView, user);
         break;

      case MCryptoEngine::SIGNATURE_UNTRUSTED_WARNING:
         pgpInfo = new PGPInfoUntrustedSig(msgView, user);
         break;

      case MCryptoEngine::NONEXISTING_KEY_ERROR:
         pgpInfo = new PGPInfoKeyNotFoundSig(msgView, user, engine, log);
         break;

      default:
         // we use unmodified text but still create an icon showing that
         // the signature check failed
         pgpInfo = new PGPInfoBadSig(msgView, user);

         // and also warn the user in case [s]he doesn't notice the icon
         wxLogWarning(_("This message is cryptographically signed but "
                        "its signature is invalid!"));
   }

   return pgpInfo;
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
ClickablePGPInfo::OnLeftClick() const
{
   ShowDetails();
}

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
                       "PGPDetails");
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
                    "PGPRawText");
}

// ----------------------------------------------------------------------------
// PGPInfoKeyNotFoundSig
// ----------------------------------------------------------------------------

void PGPInfoKeyNotFoundSig::OnLeftClick() const
{
   MessageView * const mview = GetMessageView();
   CHECK_RET( mview, "should have the associated message view" );

   String keyserver = READ_APPCONFIG_TEXT(MP_PGP_KEYSERVER);
   if ( keyserver.empty() )
   {
      if ( !MInputBox
            (
               &keyserver,
               _("Public key server"),
               _("Please enter GPG public ket server:"),
               mview->GetWindow(),
               "PGPKeyServer"
            ) )
      {
         // cancelled by user
         return;
      }
   }

   if ( m_engine->GetPublicKey(m_log->GetPublicKey(), keyserver, m_log) ==
            MCryptoEngine::OK )
   {
      // force the message view to refresh this message: we can now decrypt it
      // or verify its signature
      const UIdType uidOld = mview->GetUId();
      mview->ShowMessage(UID_ILLEGAL);
      mview->ShowMessage(uidOld);
   }
}

