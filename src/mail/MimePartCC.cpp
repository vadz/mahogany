//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimePartCC.cpp: implementation of MimePartCC class
// Purpose:     MimePartCC is an implementation of MimePartCC ABC using c-client
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 by Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "MimePartCC.h"
#endif

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "Mcclient.h" // for body_types
#endif // USE_PCH

#include "MimePartCC.h"
#include "MessageCC.h"
#include "MailFolder.h"    // for DecodeHeader

// ============================================================================
// MimePartCC implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctors
// ----------------------------------------------------------------------------

MimePartCC::MimePartCC(MessageCC *message, BODY *body)
          : MimePartCCBase(body)
{
   m_message = message;

   CreateSubParts();
}

MimePartCC::MimePartCC(BODY *body, MimePartCC *parent, size_t nPart)
          : MimePartCCBase(body, parent, nPart)
{
   // only the top level message has m_message set, so delegate to parent if we
   // have one
   m_message = parent->GetMessage();

   CreateSubParts();
}

void MimePartCC::CreateSubParts()
{
   if ( m_body->type != TYPEMULTIPART )
   {
      // is it an encapsulated message?
      if ( m_body->type == TYPEMESSAGE &&
               strcmp(m_body->subtype, "RFC822") == 0 )
      {
         BODY *bodyNested = m_body->nested.msg->body;
         if ( bodyNested )
         {
            m_nested = new MimePartCC(bodyNested, this, 1);
         }
         else
         {
            // this is not fatal but not expected neither - I don't know if it
            // can ever happen, in fact
            wxLogDebug(_T("Embedded message/rfc822 without body structure?"));
         }
      }
   }
   else // multi part
   {
      MimePartCCBase **prev = &m_nested;

      // NB: message parts are counted from 1
      size_t n = 1;
      for ( PART *part = m_body->nested.part; part; part = part->next, n++ )
      {
         *prev = new MimePartCC(&part->body, this, n);

         // static_cast<> needed to access protected member
         prev = &(static_cast<MimePartCC *>(*prev)->m_next);
      }
   }
}

// ----------------------------------------------------------------------------
// data access
// ----------------------------------------------------------------------------

const void *MimePartCC::GetRawContent(unsigned long *len) const
{
   return GetMessage()->GetRawPartData(*this, len);
}

String MimePartCC::GetHeaders() const
{
   return GetMessage()->GetPartHeaders(*this);
}

