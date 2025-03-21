//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MimeDecode.h: functions for MIME words encoding/decoding
// Author:      Vadim Zeitlin
// Created:     2007-07-29
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2007 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_MAIL_MIMEDECODE_H
#define M_MAIL_MIMEDECODE_H

#include <wx/fontenc.h>
#include <wx/buffer.h>

/**
   Various MIME helpers.
 */
namespace MIME
{

/**
   MIME encodings defined by RFC 2047.

   NB: don't change the values of the enum elements, EncodeHeader() relies on
       them being what they are!
 */
enum Encoding
{
   Encoding_Unknown,
   Encoding_Base64 = 'B',
   Encoding_QuotedPrintable = 'Q'
};

/**
   Return the MIME encoding which should be preferrably used for the given font
   encoding.

   For encodings which use a lot of ASCII characters, QP MIME encoding is
   preferred as it is more space efficient and results in more or less readable
   headers. For the others, Base64 is used.

   @param enc encoding not equal to wxFONTENCODING_SYSTEM or
              wxFONTENCODING_DEFAULT
   @return the corresponding MIME encoding or Encoding_Unknown if enc is invalid
 */
Encoding GetEncodingForFontEncoding(wxFontEncoding enc);

/**
   Return the MIME charset corresponding to the given font encoding.

   @param enc encoding not equal to wxFONTENCODING_SYSTEM or
              wxFONTENCODING_DEFAULT
   @return the charset or empty string if encoding is invalid
 */
String GetCharsetForFontEncoding(wxFontEncoding enc);

/**
   Encode a header containing special symbols using RFC 2047 mechanism.

   @param in text containing arbitrary Unicode characters
   @param enc suggestion for the encoding to use for encoding the input text,
              another encoding (typically UTF-8) will be used if the input
              can't be converted to the specified encoding; default means to
              use the encoding of the current locale
   @return the encoded text or NULL buffer if encoding failed
 */
std::string
EncodeHeader(const wxString& in, wxFontEncoding enc = wxFONTENCODING_SYSTEM);

/**
   RFC 2047 compliant message decoding.

   All encoded words from the header are decoded, but only the encoding of the
   first of them is returned in encoding output parameter (if non NULL).

   @param in a message header
   @param encoding the pointer to the charset of the string (may be NULL)
   @return the fully decoded string
*/
String DecodeHeader(const String& in, wxFontEncoding *encoding = NULL);

/**
   Helper for decoding the given data using the specified encoding.

   This method tries hard to return something useful and ignores invalid
   characters when decoding using UTF-8 instead of failing and returning an
   empty string, which is not useful.
 */
String DecodeText(const char *p, size_t len, wxFontEncoding enc);

} // namespace MIME

#endif // M_MAIL_MIMEDECODE_H

