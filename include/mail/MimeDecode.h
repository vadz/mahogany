//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MimeDecode.h: functions for MIME words decoding
// Author:      Vadim Zeitlin
// Created:     2007-07-29
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2007 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_MAIL_MIMEDECODE_H
#define M_MAIL_MIMEDECODE_H

#include <wx/fontenc.h>

/**
   Various MIME helpers.
 */
namespace MIME
{

/**
   RFC 2047 compliant message decoding.

   All encoded words from the header are decoded, but only the encoding of the
   first of them is returned in encoding output parameter (if non NULL).

   @param in a message header
   @param encoding the pointer to the charset of the string (may be NULL)
   @return the fully decoded string
*/
String DecodeHeader(const String& in, wxFontEncoding *encoding = NULL);

} // namespace MIME

#endif // M_MAIL_MIMEDECODE_H

