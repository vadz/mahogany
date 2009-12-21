///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimePartVirtual.h
// Purpose:     a "virtual" MimePart object containing arbitrary data
// Author:      Xavier Nodet (old MimePartRaw), Vadim Zeitlin
// Modified by:
// Created:     2004-07-13 by VZ (extracted from modules/viewflt/UUdecode.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Xavier Nodet <xavier.nodet@free.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/**
   @file MimePartVirtual.h
   @brief MimePartVirtual class represents a "virtual" MimePart.

   We use MimePartVirtual when decoding uuencoded or PGP-encrypted messages.
 */

#ifndef _M_MIMEPARTRAW_H_
#define _M_MIMEPARTRAW_H_

#include "MimePartCCBase.h"

/**
   MimePartVirtual is a MIME part built from raw data.
 */
class MimePartVirtual : public MimePartCCBase
{ 
public:
   /**
      Construct a MIME part from the entire part text (body + header).

      Use IsOk() to check if parsing the buffer succeeded.

      Notice that @a msgText must be in canonical RFC 822 format, in particular
      use CR LF as line separators.
    */
   MimePartVirtual(const wxCharBuffer& msgText);

   /**
      Construct a nested MIME part.

      Note that pStart points directly into the (grand) parents buffer.

      @param body structure of this part
      @param parent the parent MIME part
      @param nPart the 1-based number of this part among its siblings
      @param pHeader start of the header
    */
   MimePartVirtual(struct mail_bodystruct *body,
                   MimePartVirtual *parent,
                   size_t nPart,
                   const char *pHeader);

   virtual ~MimePartVirtual();

   /**
      Check if the part was successfully initialized.

      If it wasn't, it can't be used and must be deleted.
    */
   bool IsOk() const { return m_env && m_body; }

   virtual const void *GetRawContent(unsigned long *len = NULL) const;
   virtual String GetHeaders() const;

private:
   // create all nested subparts
   void CreateSubParts();

   // get pointer to the start of the body (+2 is for "\r\n")
   const char *GetBodyStart() const { return m_pStart + m_lenHeader + 2; }


   // the entire text of the message, only non empty for the top level part
   const wxCharBuffer m_msgText;

   // pointer to the start of headers of this part
   const char *m_pStart;

   // the length of the header (offset of the start of the body at m_pStart)
   size_t m_lenHeader;

   // the length of (just) the body
   size_t m_lenBody;

   // message envelope (base class already has the body)
   struct mail_envelope *m_env;
};

#endif // _M_MIMEPARTRAW_H_

