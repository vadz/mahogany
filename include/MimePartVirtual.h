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
    */
   MimePartVirtual(const String& msgText);
   virtual ~MimePartVirtual();

   virtual const void *GetRawContent(unsigned long *len = NULL) const;
   virtual String GetHeaders() const;

private:
   // the entire text of the message
   String m_msgText;

   // message envelope (base class already ahs the body)
   struct mail_envelope *m_env;

   // the length of the header (at the start of m_msgText)
   size_t m_lenHeader;
};

#endif // _M_MIMEPARTRAW_H_

