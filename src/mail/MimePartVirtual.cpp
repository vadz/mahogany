///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MimePartVirtual.cpp
// Purpose:     a "virtual" MimePart object containing arbitrary data
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-13
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
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

   #include "Mcclient.h"
#endif

#include "MimePartVirtual.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// strlen("\r\n")
static const size_t lenEOL = 2;

// ============================================================================
// MimePartVirtual implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor/dtor
// ----------------------------------------------------------------------------

// ctor for top level part
MimePartVirtual::MimePartVirtual(const wxCharBuffer& msgText)
               : m_msgText(msgText)
{
   m_pStart = m_msgText;

   m_env = NULL;

   m_lenHeader =
   m_lenBody = 0;

   if ( !CclientParseMessage(m_pStart, &m_env, &m_body, &m_lenHeader) )
   {
      wxLogError(_("Failed to parse the text of the embedded message."));

      return;
   }

   m_lenBody = m_msgText.length() - m_lenHeader - lenEOL;

   // c-client can't determine it itself correctly (the "string" we pass to it
   // can have embedded NULs and we don't pass the real length), so fix it here
   m_body->size.bytes =
   m_body->contents.text.size = m_lenBody;


   CreateSubParts();
}

MimePartVirtual::MimePartVirtual(BODY *body,
                                 MimePartVirtual *parent,
                                 size_t nPart,
                                 const char *pHeader)
{
   m_env = NULL;

   Create(body, parent, nPart);

   m_pStart = pHeader + body->mime.offset;
   m_lenHeader = body->mime.text.size - lenEOL;
   m_lenBody = body->contents.text.size;

   CreateSubParts();
}

void MimePartVirtual::CreateSubParts()
{
   // set up the nested parts, if any
   switch ( m_body->type )
   {
      case TYPEMULTIPART:
         {
            // NB: message parts are counted from 1
            size_t n = 1;
            MimePartCCBase **prev = &m_nested;
            const char *pStart = GetBodyStart();
            for ( PART *part = m_body->nested.part; part; part = part->next )
            {
               *prev = new MimePartVirtual(&part->body, this, n++, pStart);

               // static_cast<> needed to access protected member
               prev = &(static_cast<MimePartVirtual *>(*prev)->m_next);
            }
         }
         break;

      case TYPEMESSAGE:
         {
            mail_body_message *msg = m_body->nested.msg;
            if ( !msg )
            {
               wxLogError(_("Corrupted nested message."));
               break;
            }

            m_nested = new MimePartVirtual(msg->body, this, 1, GetBodyStart());
         }
         break;

      default:
         // a simple part, nothing to do
         ;
   }
}

MimePartVirtual::~MimePartVirtual()
{
   // only the top level part frees the cclient structures
   if ( !m_parent )
   {
      mail_free_envelope(&m_env);
      mail_free_body(&m_body);
   }
}

// ----------------------------------------------------------------------------
// data access
// ----------------------------------------------------------------------------

String MimePartVirtual::GetHeaders() const
{
   return String(m_pStart, m_lenHeader);
}

const void *MimePartVirtual::GetRawContent(unsigned long *len) const
{
   if ( len )
      *len = m_lenBody;

   return GetBodyStart();
}

