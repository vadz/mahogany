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

         const wxFontEncoding
            encodingWord = wxFontMapper::Get()->CharsetToEncoding(csName);

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
         if ( encoding == wxFONTENCODING_UTF7 ||
                  encoding == wxFONTENCODING_UTF8 )
         {
            encoding = ConvertUTFToMB(&textDecoded, encoding);
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

