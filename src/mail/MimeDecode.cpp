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
inline unsigned char *UCHAR_CCAST(const char *s)
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
         return Encoding_Base64;

      default:
         FAIL_MSG( _T("unknown encoding") );

      case wxFONTENCODING_SYSTEM:
         return Encoding_Unknown;
   }
}

// ----------------------------------------------------------------------------
// decoding
// ----------------------------------------------------------------------------

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

   // there can be words in different encodings inside the same header so this
   // variable doesn't really make sense but in practice only one encoding will
   // be used in the entire header
   wxFontEncoding encodingHeader = wxFONTENCODING_SYSTEM;

   // if the header starts with an encoded word, preceding whitespace must be
   // ignored, so the flag must be set to true initially
   bool maybeBetweenEncodedWords = true;

   String out,
          space;
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

         // pass false to prevent asking the user from here: we can be called
         // during non-interactive operations and popping up a dialog for an
         // unknown charset can be inappropriate
         const wxFontEncoding encodingWord = wxFontMapperBase::Get()->
                                               CharsetToEncoding(csName, false);

         if ( encodingWord == wxFONTENCODING_SYSTEM )
         {
            wxLogDebug(_T("Unrecognized charset name \"%s\""), csName.mb_str());
         }

         // this is not a problem in Unicode build
#if !wxUSE_UNICODE
         if ( encodingHeader != wxFONTENCODING_SYSTEM &&
               encodingHeader != encodingWord )
         {
            // this is a bug (well, missing feature) in ANSI build of Mahogany
            wxLogDebug(_T("This header contains encoded words with different ")
                       _T("encodings and won't be rendered correctly."));
         }
#endif // !wxUSE_UNICODE

         encodingHeader = encodingWord;


         // get the encoding in RFC 2047 sense
         enum
         {
            Encoding_Unknown,
            Encoding_Base64,
            Encoding_QuotedPrintable
         } enc2047 = Encoding_Unknown;

         ++p; // skip '?'
         if ( *(p + 1) == '?' )
         {
            if ( *p == 'B' || *p == 'b' )
               enc2047 = Encoding_Base64;
            else if ( *p == 'Q' || *p == 'q' )
               enc2047 = Encoding_QuotedPrintable;
         }
         //else: multi letter encoding unrecognized

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

            // +2 to skip "?="
            p += posEncWordEnd - posEncWordStart + 2;

            // leave this word undecoded
            out += wxString(encWordStart, p + 1);

            continue;
         }

         p += 2; // skip "Q?" or "B?"

         // get the encoded text
         bool hasUnderscore = false;
         const wxString::const_iterator encTextStart = p;
         while ( p != last && (*p != '?' || *(p + 1) != '=') )
         {
            // this is needed for QP hack below
            if ( *p == '_' )
               hasUnderscore = true;

            ++p;
         }

         if ( p == last )
         {
            wxLogDebug(_T("Missing encoded word end marker in '%s'."),
                       in.c_str());
            out += wxString(encWordStart, end);

            break;
         }

         // convert the encoded word to char[] buffer for c-client
         wxCharBuffer encWord(wxString(encTextStart, p).To8BitData());

         // skip '=' following '?'
         ++p;

         String textDecoded;
         if ( encWord )
         {
            const unsigned long lenEncWord = strlen(encWord);

            // now decode the text using c-client functions
            unsigned long len;
            void *text;
            if ( enc2047 == Encoding_Base64 )
            {
               text = rfc822_base64(UCHAR_CCAST(encWord), lenEncWord, &len);
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
                  for ( char *pc = encWord.data(); *pc; ++pc )
                  {
                     if ( *pc == '_' )
                        *pc = ' ';
                  }
               }

               text = rfc822_qprint(UCHAR_CCAST(encWord), lenEncWord, &len);
            }

            if ( text )
            {
               textDecoded = wxString((char *)text, wxCSConv(encodingWord), len);

               fs_give(&text);
            }
         }

         if ( textDecoded.empty() )
         {
            // if decoding failed it is probably better to show undecoded
            // text than nothing at all
            textDecoded = wxString(encWordStart, p + 1);
         }

         // normally we leave the (8 bit) string as is and remember its
         // encoding so that we may choose the font for displaying it
         // correctly, but in case of UTF-7/8 we really need to transform it
         // here as we don't have any UTF-7/8 fonts, so we should display a
         // different string
#if !wxUSE_UNICODE
         if ( encodingHeader == wxFONTENCODING_UTF7 ||
                  encodingHeader == wxFONTENCODING_UTF8 )
         {
            encodingHeader = ConvertUTFToMB(&textDecoded, encodingHeader);
         }
#endif // !wxUSE_UNICODE

         out += textDecoded;

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

   if ( pEncoding )
      *pEncoding = encodingHeader;

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
static inline bool NeedsEncodingInHeader(wxUChar c)
{
   return iscntrl(c) || c >= 127;
}

// return true if the string contains any characters which must be encoded
static bool NeedsEncoding(const String& in)
{
   // if input contains "=?", encode it anyhow to avoid generating invalid
   // encoded words
   if ( in.find(_T("=?")) == wxString::npos )
   {
      // only encode the strings which contain the characters unallowed in RFC
      // 822 headers
      wxString::const_iterator p;
      const wxString::const_iterator end = in.end();
      for ( p = in.begin(); p != end; ++p )
      {
         if ( NeedsEncodingInHeader(*p) )
            break;
      }

      if ( p == end )
      {
         // string has only valid chars, don't encode
         return false;
      }
   }

   return true;
}

static String
EncodeWord(const String& in,
           wxFontEncoding enc,
           MIME::Encoding enc2047,
           const String& csName)
{
   if ( !NeedsEncoding(in) )
      return in;


   // encode the word splitting it in the chunks such that they will be no
   // longer than 75 characters each
   wxCharBuffer buf(in.mb_str(wxCSConv(enc)));
   if ( !buf )
   {
      // if the header can't be encoded using the given encoding, use UTF-8
      // which always works
      buf = in.utf8_str();
   }

   String out;
   out.reserve(csName.length() + strlen(buf) + 7 /* for =?...?X?...?= */);

   const char *s = buf;
   while ( *s )
   {
      // if we wrapped, insert a line break
      if ( !out.empty() )
         out += "\r\n  ";

      static const size_t RFC2047_MAXWORD_LEN = 75;

      // how many characters may we put in this encoded word?
      size_t len = 0;

      // take into account the length of "=?charset?...?="
      int lenRemaining = RFC2047_MAXWORD_LEN - (5 + csName.length());

      // for QP we need to examine all characters
      if ( enc2047 == MIME::Encoding_QuotedPrintable )
      {
         for ( ; s[len]; len++ )
         {
            const char c = s[len];

            // normal characters stand for themselves in QP, the encoded ones
            // take 3 positions (=XX)
            lenRemaining -= (NeedsEncodingInHeader(c) || strchr(" \t=?", c))
                              ? 3 : 1;

            if ( lenRemaining <= 0 )
            {
               // can't put any more chars into this word
               break;
            }
         }
      }
      else // Base64
      {
         // we can calculate how many characters we may put into lenRemaining
         // directly
         len = (lenRemaining / 4) * 3 - 2;

         // but not more than what we have
         size_t lenMax = wxStrlen(s);
         if ( len > lenMax )
         {
            len = lenMax;
         }
      }

      // do encode this word
      unsigned char *text = (unsigned char *)s; // cast for cclient

      // length of the encoded text and the text itself
      unsigned long lenEnc;
      unsigned char *textEnc;

      if ( enc2047 == MIME::Encoding_QuotedPrintable )
      {
            textEnc = rfc822_8bit(text, len, &lenEnc);
      }
      else // Encoding_Base64
      {
            textEnc = rfc822_binary(text, len, &lenEnc);
            while ( textEnc[lenEnc - 2] == '\r' && textEnc[lenEnc - 1] == '\n' )
            {
               // discard eol which we don't need in the header
               lenEnc -= 2;
            }
      }

      // put into string as we might want to do some more replacements...
      String encword(wxString::FromAscii(CHAR_CAST(textEnc)
#if wxCHECK_VERSION(2, 9, 0)
                                         , lenEnc
#endif
                                        ));

      // hack: rfc822_8bit() doesn't encode spaces normally but we must
      // do it inside the headers
      //
      // we also have to encode '?'s in the headers which are not encoded by it
      if ( enc2047 == MIME::Encoding_QuotedPrintable )
      {
         String encword2;
         encword2.reserve(encword.length());

         bool replaced = false;
         for ( const wxChar *p = encword.c_str(); *p; p++ )
         {
            switch ( *p )
            {
               case ' ':
                  encword2 += _T("=20");
                  break;

               case '\t':
                  encword2 += _T("=09");
                  break;

               case '?':
                  encword2 += _T("=3F");
                  break;

               default:
                  encword2 += *p;

                  // skip assignment to replaced below
                  continue;
            }

            replaced = true;
         }

         if ( replaced )
         {
            encword = encword2;
         }
      }

      // append this word to the header
      out << _T("=?") << csName << _T('?') << (char)enc2047 << _T('?')
          << encword
          << _T("?=");

      fs_give((void **)&textEnc);

      // skip the already encoded part
      s += len;
   }

   return out;
}

wxCharBuffer MIME::EncodeHeader(const String& in, wxFontEncoding enc)
{
   if ( !NeedsEncoding(in) )
      return in.ToAscii();

   // get the encoding in RFC 2047 sense: choose the most reasonable one
   if ( enc == wxFONTENCODING_SYSTEM )
      enc = wxLocale::GetSystemEncoding();

   MIME::Encoding enc2047 = MIME::GetEncodingForFontEncoding(enc);

   if ( enc2047 == MIME::Encoding_Unknown )
   {
      FAIL_MSG( _T("should have valid MIME encoding") );

      enc2047 = MIME::Encoding_QuotedPrintable;
   }

   // get the name of the charset to use
   String csName = MIME::GetCharsetForFontEncoding(enc);
   if ( csName.empty() )
   {
      FAIL_MSG( _T("should have a valid charset name!") );

      csName = _T("UNKNOWN");
   }


   String headerEnc;
   headerEnc.reserve(2*in.length());

   // for QP we encode each header word separately so that the header remains
   // readable, but for Base64 it's useless to do this as it's unreadable
   // anyhow so we just encode everything at once
   if ( enc2047 == MIME::Encoding_QuotedPrintable )
   {
      // encode each word of the header
      const wxArrayString words(wxStringTokenize(in));
      const size_t count = words.size();
      for ( size_t n = 0; n < count; ++n )
      {
         headerEnc += EncodeWord(words[n], enc, enc2047, csName);
         if ( n + 1 < count )
            headerEnc += ' ';
      }
   }
   else // MIME::Encoding_Base64
   {
      headerEnc = EncodeWord(in, enc, enc2047, csName);
   }

   return headerEnc.ToAscii();
}
