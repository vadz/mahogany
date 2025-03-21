//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/MimeDecode.cpp: functions for MIME words decoding
// Author:      Vadim Zeitlin
// Created:     2007-07-29
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2007 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef  USE_PCH
   #include "Mcclient.h"
#endif // !USE_PCH

#include "mail/MimeDecode.h"

#include <wx/fontmap.h>
#include <wx/tokenzr.h>

// ----------------------------------------------------------------------------
// local helper functions
// ----------------------------------------------------------------------------

// this should only be used for const-incorrect c-client functions
static inline unsigned char *UCHAR_CCAST(const char *s)
{
   return reinterpret_cast<unsigned char *>(const_cast<char *>(s));
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// font encoding <-> MIME functions
// ----------------------------------------------------------------------------

String MIME::GetCharsetForFontEncoding(wxFontEncoding enc)
{
   // translate encoding to the charset
   wxString cs;
   if ( enc != wxFONTENCODING_SYSTEM && enc != wxFONTENCODING_DEFAULT )
   {
      cs = wxFontMapper::GetEncodingName(enc).Upper();
   }

   return cs;
}

MIME::Encoding MIME::GetEncodingForFontEncoding(wxFontEncoding enc)
{
   // QP should be used for the encodings which mostly overlap with US_ASCII,
   // Base64 for the others - choose the encoding method
   switch ( enc )
   {
      case wxFONTENCODING_ISO8859_1:
      case wxFONTENCODING_ISO8859_2:
      case wxFONTENCODING_ISO8859_3:
      case wxFONTENCODING_ISO8859_4:
      case wxFONTENCODING_ISO8859_9:
      case wxFONTENCODING_ISO8859_10:
      case wxFONTENCODING_ISO8859_13:
      case wxFONTENCODING_ISO8859_14:
      case wxFONTENCODING_ISO8859_15:

      case wxFONTENCODING_CP1250:
      case wxFONTENCODING_CP1252:
      case wxFONTENCODING_CP1254:
      case wxFONTENCODING_CP1257:

      case wxFONTENCODING_UTF7:
      case wxFONTENCODING_UTF8:
         return Encoding_QuotedPrintable;


      case wxFONTENCODING_ISO8859_5:
      case wxFONTENCODING_ISO8859_6:
      case wxFONTENCODING_ISO8859_7:
      case wxFONTENCODING_ISO8859_8:
      case wxFONTENCODING_ISO8859_11:
      case wxFONTENCODING_ISO8859_12:

      case wxFONTENCODING_CP1251:
      case wxFONTENCODING_CP1253:
      case wxFONTENCODING_CP1255:
      case wxFONTENCODING_CP1256:

      case wxFONTENCODING_KOI8:
#ifdef wxFONTENCODING_ISO2022_JP
      case wxFONTENCODING_ISO2022_JP:
#endif
      case wxFONTENCODING_SHIFT_JIS:
      case wxFONTENCODING_GB2312:
      case wxFONTENCODING_BIG5:
         return Encoding_Base64;

      default:
         FAIL_MSG( _T("unknown encoding") );
         // fall through

      case wxFONTENCODING_SYSTEM:
         return Encoding_Unknown;
   }
}

// ----------------------------------------------------------------------------
// decoding
// ----------------------------------------------------------------------------

// Local wrapper around public function taking char* and length.
static
String DecodeString(const std::string& s, wxFontEncoding enc)
{
   return MIME::DecodeText(s.data(), s.length(), enc);
}

/*
   See RFC 2047 for the description of the encodings used in the mail headers.
   Briefly, "encoded words" can be inserted which have the form of

      encoded-word = "=?" charset "?" encoding "?" encoded-text "?="

   where charset and encoding can't contain space, control chars or "specials"
   and encoded-text can't contain spaces nor "?".

   NB: don't be confused by 2 meanings of encoding here: it is both the
       charset encoding for us and also QP/Base64 encoding for RFC 2047
 */
static
String DecodeHeaderOnce(const String& in, wxFontEncoding *pEncoding)
{
   // we don't enforce the sanity checks on charset and encoding - should we?
   // const char *specials = "()<>@,;:\\\"[].?=";

   // encoding of the previous word or wxFONTENCODING_SYSTEM if it wasn't
   // encoded (or there wasn't any previous word at all yet)
   wxFontEncoding encodingLastWord = wxFONTENCODING_SYSTEM;

   // the not yet decoded text using encodingLastWord, we'll convert it from
   // this encoding all at once when we can be sure that there is nothing
   // following it any more
   //
   // notice that this is more than just an optimization: RFC 2047 encoding can
   // separate bytes that are part of the same multibyte characters, e.g. if
   // the string is sufficiently long and needs to be wrapped it's perfectly
   // possible that the leading byte of UTF-8 encoding is part of one encoded
   // word while the rest of them are in the other one and so converting each
   // of them from UTF-8 on their own wouldn't work, but combining them and
   // only converting both at once would
   std::string textLastWord;

   if ( pEncoding )
      *pEncoding = wxFONTENCODING_SYSTEM;

   // if the header starts with an encoded word, preceding whitespace must be
   // ignored, so the flag must be set to true initially
   bool maybeBetweenEncodedWords = true;

   String out,
          space;
   // we can't define a valid "last" iterator below for empty string
   if ( in.empty() )
      return String();

   out.reserve(in.length());
   for ( wxString::const_iterator p = in.begin(),
                                end = in.end(),
                               last = in.end() - 1; p != end; ++p )
   {
      if ( *p == '=' && p != last && *(p + 1) == '?' )
      {
         // found encoded word

         // save the start of it
         const wxString::const_iterator encWordStart = p++;

         // get the charset
         String csName;
         for ( ++p; p != end && *p != '?'; ++p ) // initial "++" to skip '?'
         {
            csName += *p;
         }

         if ( p == end )
         {
            wxLogDebug(_T("Invalid encoded word syntax in '%s': missing charset."),
                       in.c_str());
            out += wxString(encWordStart, end);

            break;
         }

         if ( csName.empty() )
         {
            wxLogDebug("Invalid encoded word \"%s\": missing encoding.", in);
            out += wxString(encWordStart, end);

            break;
         }

         // pass false to prevent asking the user from here: we can be called
         // during non-interactive operations and popping up a dialog for an
         // unknown charset can be inappropriate
         const wxFontEncoding encodingWord = wxFontMapperBase::Get()->
                                               CharsetToEncoding(csName, false);

         if ( encodingWord == wxFONTENCODING_SYSTEM )
         {
            wxLogDebug(_T("Unrecognized charset name \"%s\""), csName.mb_str());
         }

         if ( encodingWord != encodingLastWord )
         {
            if ( encodingLastWord != wxFONTENCODING_SYSTEM )
            {
               // The last word must be complete now, decode it.
               out += DecodeString(textLastWord, encodingLastWord);
            }

            encodingLastWord = encodingWord;
            textLastWord.clear();

            if ( pEncoding )
               *pEncoding = encodingWord;
         }

         // get the encoding in RFC 2047 sense
         enum
         {
            Encoding_Unknown,
            Encoding_Base64,
            Encoding_QuotedPrintable
         } enc2047 = Encoding_Unknown;

         ++p; // skip '?'

         if ( p >= end - 2 )
         {
            wxLogDebug(wxS("Unterminated quoted word in \"%s\" ignored."), in);
            out += wxString(encWordStart, end);

            break;
         }
         else // We have at least 2 more characters in the string.
         {
            if ( *(p + 1) == '?' )
            {
               if ( *p == 'B' || *p == 'b' )
                  enc2047 = Encoding_Base64;
               else if ( *p == 'Q' || *p == 'q' )
                  enc2047 = Encoding_QuotedPrintable;
            }
            //else: multi letter encoding unrecognized
         }

         if ( enc2047 == Encoding_Unknown )
         {
            wxLogDebug(_T("Unrecognized header encoding in '%s'."), in.c_str());

            // scan until the end of the encoded word
            const size_t posEncWordStart = p - in.begin();
            const size_t posEncWordEnd = in.find("?=", p - in.begin());
            if ( posEncWordEnd == wxString::npos )
            {
               wxLogDebug(_T("Missing encoded word end marker in '%s'."),
                          in.c_str());
               out += wxString(encWordStart, end);

               break;
            }

            // +1 to skip '?' of "?=" (don't skip '=', this will be accounted
            // for by p increment in the loop statement)
            p += posEncWordEnd - posEncWordStart + 1;

            // leave this word undecoded
            out += wxString(encWordStart, p + 1);

            continue;
         }

         p += 2; // skip "Q?" or "B?"

         // get the encoded text
         std::string encWord;
         bool hasUnderscore = false;
         while ( p != last && (*p != '?' || *(p + 1) != '=') )
         {
            // this is needed for QP hack below
            if ( *p == '_' )
               hasUnderscore = true;

            encWord += *p;

            ++p;
         }

         if ( p == last )
         {
            wxLogDebug(_T("Missing encoded word end marker in '%s'."),
                       in.c_str());
            out += wxString(encWordStart, end);

            break;
         }

         // skip '=' following '?'
         ++p;

         if ( !encWord.empty() )
         {
            const unsigned long lenEncWord = encWord.length();

            // now decode the text using c-client functions
            unsigned long len;
            void *text;
            if ( enc2047 == Encoding_Base64 )
            {
               text = rfc822_base64(UCHAR_CCAST(encWord.data()), lenEncWord, &len);
            }
            else // QP
            {
               // cclient rfc822_qprint() behaves correctly and leaves '_' in the
               // QP encoded text because this is what RFC says, however many
               // broken clients replace spaces with underscores and so we undo it
               // here -- in this case it's better to be user-friendly than
               // standard-conforming
               if ( hasUnderscore )
               {
                  for ( auto& c : encWord )
                  {
                     if ( c == '_' )
                        c = ' ';
                  }
               }

               text = rfc822_qprint(UCHAR_CCAST(encWord.data()), lenEncWord, &len);
            }

            if ( text )
            {
               const char * const ctext = static_cast<char *>(text);

               textLastWord.append(ctext, ctext + len);

               fs_give(&text);
            }
         }

         // forget the space before this encoded word, it must be ignored
         space.clear();
         maybeBetweenEncodedWords = true;
      }
      else if ( maybeBetweenEncodedWords &&
                  (*p == ' ' || *p == '\r' || *p == '\n') )
      {
         // spaces separating the encoded words must be ignored according
         // to section 6.2 of the RFC 2047, so we don't output them immediately
         // but delay until we know that what follows is not an encoded word
         space += *p;
      }
      else // just another normal char
      {
         // the last word won't be continued any more, so decode it, if any
         if ( encodingLastWord != wxFONTENCODING_SYSTEM )
         {
            out += DecodeString(textLastWord, encodingLastWord);

            encodingLastWord = wxFONTENCODING_SYSTEM;
            textLastWord.clear();
         }

         // if we got any delayed whitespace (see above), flush it now
         if ( !space.empty() )
         {
            out += space;
            space.clear();
         }

         out += *p;

         maybeBetweenEncodedWords = false;
      }
   }

   // decode anything that remains
   if ( encodingLastWord != wxFONTENCODING_SYSTEM )
      out += DecodeString(textLastWord, encodingLastWord);

   return out;
}

String MIME::DecodeHeader(const String& in, wxFontEncoding *pEncoding)
{
   if ( pEncoding )
      *pEncoding = wxFONTENCODING_SYSTEM;

   // some brain dead mailer encode the already encoded headers so to obtain
   // the real header we keep decoding it until it stabilizes
   String header,
          headerOrig = in;
   for ( ;; )
   {
      wxFontEncoding encoding;
      header = DecodeHeaderOnce(headerOrig, &encoding);
      if ( header == headerOrig )
         break;

      if ( pEncoding )
         *pEncoding = encoding;

      headerOrig = header;
   }

   return header;
}

// ----------------------------------------------------------------------------
// encoding
// ----------------------------------------------------------------------------

// returns true if the character must be encoded in a MIME header
//
// NB: we suppose that any special characters had been already escaped
static inline bool NeedsEncodingInHeader(unsigned c)
{
   return c < 20 || c >= 127;
}

// return true if the string contains any characters which must be encoded
static bool NeedsEncoding(const String& in)
{
   // if input contains "=?", encode it anyhow to avoid generating invalid
   // encoded words
   if ( in.find(_T("=?")) != wxString::npos )
      return true;

   // only encode the strings which contain the characters unallowed in RFC
   // 822 headers
   for ( auto c : in )
   {
      if ( NeedsEncodingInHeader(c.GetValue()) )
         return true;
   }

   // string has only valid chars, don't encode
   return false;
}

// Some constants used by the encoding functions below.
static constexpr size_t RFC2047_MAXWORD_LEN = 75;
static constexpr size_t MIME_WORD_OVERHEAD = 7; // =?...?X?...?=

// encode the text in the charset with the given name in QP
static std::string
EncodeTextQP(const char* in, const std::string& csName)
{
   // encode the text splitting it in the chunks such that they will be no
   // longer than maximum length each
   const size_t overhead = MIME_WORD_OVERHEAD + csName.length();

   const char* const HEX_DIGITS = "0123456789ABCDEF";

   std::string out;
   out.reserve(strlen(in) + overhead);

   // Each iteration of this loop corresponds to a physical line.
   while ( *in )
   {
      if ( !out.empty() )
         out += "\r\n  ";

      out += "=?";
      out += csName;
      out += "?Q?";

      // Iterate over the characters that fit into the current line.
      size_t lineLen = overhead;
      for ( ;; ++in )
      {
         const unsigned char c = *in;

         if ( c == '\0' )
            break;

         // If this character is 0, we must encode "c" instead.
         unsigned char unencoded = '\0';
         switch ( c )
         {
            case ' ':
               // Encode spaces as underscores rather than =20, as this is more
               // readable, even if both are allowed.
               unencoded = '_';
               break;

               // These characters need to be encoded in headers to avoid
               // clashing with the encoded word syntax. In principle, we could
               // let them remain unencoded if they don't occur in the same
               // combination as in the encoded word syntax, but for now keep
               // things simple and always encode them.
            case '=':
            case '?':
               break;

            default:
               if ( !NeedsEncodingInHeader(c) )
                  unencoded = c;
               break;
         }

         const auto lenNeeded = unencoded ? 1 : 3;
         if ( lineLen + lenNeeded > RFC2047_MAXWORD_LEN )
            break;

         if ( unencoded )
         {
            out += unencoded;
         }
         else
         {
            out += '=';
            out += HEX_DIGITS[c >> 4];
            out += HEX_DIGITS[c & 0xf];
         }

         lineLen += lenNeeded;
      }

      out += "?=";
   }

   return out;
}

// same as the function above but use Base64 encoding
static std::string
EncodeTextBase64(const char* in, const std::string& csName)
{
   const size_t overhead = MIME_WORD_OVERHEAD + csName.length();

   // encode the word splitting it in the chunks such that they will be no
   // longer than maximum length each
   std::string out;
   out.reserve(strlen(in) + overhead);

   auto *s = reinterpret_cast<const unsigned char*>(in);
   while ( *s )
   {
      // if we wrapped, insert a line break
      if ( !out.empty() )
         out += "\r\n  ";

      // rfc822_binary() splits lines after 60 characters so don't make
      // chunks longer than this as the base64-encoded headers can't have
      // EOLs in them
      static const int CCLIENT_MAX_BASE64_LEN = 60;

      // but if the charset name is sufficiently long, we may need to make them
      // shorter than this
      size_t lenRemaining = wxMin(CCLIENT_MAX_BASE64_LEN,
                                  RFC2047_MAXWORD_LEN - overhead);

      // we can calculate how many characters we may put on one line directly
      size_t len = (lenRemaining / 4) * 3;

      // but not more than what we have
      size_t lenMax = strlen(reinterpret_cast<const char*>(s));
      if ( len > lenMax )
      {
         len = lenMax;
      }

      // do encode this word
      unsigned char *text = const_cast<unsigned char*>(s); // cast for cclient

      // length of the encoded text and the text itself
      unsigned long lenEnc;
      unsigned char *textEnc = rfc822_binary(text, len, &lenEnc);

      while ( textEnc[lenEnc - 2] == '\r' && textEnc[lenEnc - 1] == '\n' )
      {
         // discard eol which we don't need in the header
         lenEnc -= 2;
      }

      // put into string as we might want to do some more replacements...
      std::string encword(CHAR_CAST(textEnc), lenEnc);

      // append this word to the header
      out += "=?";
      out += csName;
      out += "?B?";
      out += encword;
      out += "?=";

      fs_give((void **)&textEnc);

      // skip the already encoded part
      s += len;
   }

   return out;
}

std::string MIME::EncodeHeader(const String& in, wxFontEncoding enc)
{
   if ( !NeedsEncoding(in) )
      return std::string(in.ToAscii());

   // If we were given an explicit encoding to use, check if we can use it, and
   // if not fall back to the same UTF-8 (which can always be used) as we use
   // by default if no encoding was specified in the first place.
   if ( enc == wxFONTENCODING_SYSTEM ||
         wxCSConv(enc).FromWChar(NULL, 0, in.wc_str()) == wxCONV_FAILED )
   {
      enc = wxFONTENCODING_UTF8;
   }

   // get the name of the charset to use
   std::string csName = MIME::GetCharsetForFontEncoding(enc).utf8_string();
   if ( csName.empty() )
   {
      FAIL_MSG( _T("should have a valid charset name!") );

      csName = "UNKNOWN";
   }

   wxCharBuffer inbuf;
   if ( enc != wxFONTENCODING_UTF8 )
      inbuf = in.mb_str(wxCSConv(enc));

   if ( !inbuf )
   {
      // if the header can't be encoded using the given encoding, use UTF-8
      // which always works
      inbuf = in.utf8_str();
   }

   std::string out;
   switch ( MIME::GetEncodingForFontEncoding(enc) )
   {
      case MIME::Encoding_Unknown:
         FAIL_MSG( "using unknown MIME encoding?" );
         wxFALLTHROUGH;

      case MIME::Encoding_QuotedPrintable:
         out = EncodeTextQP(inbuf.data(), csName);
         break;

      case MIME::Encoding_Base64:
         out = EncodeTextBase64(inbuf.data(), csName);
         break;
   }

   return out;
}

String MIME::DecodeText(const char *p, size_t len, wxFontEncoding enc)
{
   // Use always successful conversion from UTF-8 as fallback because it's
   // better to return some garbage (which could, hopefully, contain readable
   // parts of text) than nothing at all.
   String s;
   if ( enc != wxFONTENCODING_UTF8 )
      s = String(p, wxCSConv(enc), len);

   if ( s.empty() )
      s = String(p, wxMBConvUTF8(wxMBConvUTF8::MAP_INVALID_UTF8_TO_PUA), len);

   return s;
}
