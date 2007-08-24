///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMGuiUtils.cpp
// Purpose:     miscellaneous GUI helpers
// Author:      Vadim Zeitlin
// Created:     2006-08-19
// CVS-ID:      $Id$
// Copyright:   (c) 2006 Vadim Zeitlin
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
#  include "Mcommon.h"
#  include "guidef.h"
#endif // USE_PCH

#include <wx/fontmap.h>
#include <wx/encconv.h>

// ============================================================================
// implementation
// ============================================================================

#if wxUSE_UNICODE

bool
EnsureAvailableTextEncoding(wxFontEncoding *enc, wxString *text, bool mayAskUser)
{
   CHECK( enc, false, _T("CheckEncodingAvailability: NULL encoding") );

   if ( !wxFontMapper::Get()->IsEncodingAvailable(*enc) )
   {
      // try to find another encoding
      wxFontEncoding encAlt;
      if ( wxFontMapper::Get()->
            GetAltForEncoding(*enc, &encAlt, wxEmptyString, mayAskUser) )
      {
         // translate the text (if any) to the equivalent encoding
         if ( text && !text->empty() )
         {
#if wxUSE_WCHAR_T
            // try converting via Unicode
            wxCSConv a2w(*enc);
            wxWCharBuffer wbuf(a2w.cMB2WC(text->c_str()));
            if ( *wbuf )
            {
               wxString textConv;

               // special case of UTF-8 which is used all the time under wxGTK
               if ( encAlt == wxFONTENCODING_UTF8 )
               {
                  textConv = wxConvUTF8.cWC2MB(wbuf);
               }
               else // all the other encodings, use generic converter
               {
                  wxCSConv w2a(encAlt);
                  textConv = w2a.cWC2MB(wbuf);
               }

               if ( !textConv.empty() )
               {
                  *text = textConv;
                  return true;
               }
               //else: fall back to wxEncodingConverter
            }
            //else: conversion to Unicode failed
#endif // wxUSE_WCHAR_T

            wxEncodingConverter conv;
            if ( !conv.Init(*enc, encAlt) )
            {
               // failed to convert the text
               return false;
            }

            *text = conv.Convert(*text);
         }
         //else: just return the encoding

         *enc = encAlt;
      }
      else // no equivalent encoding
      {
         return false;
      }
   }

   // we have either the requested encoding or an equivalent one
   return true;
}

#endif // !wxUSE_UNICODE

wxFont
CreateFontFromDesc(const String& fontDesc, int fontSize, int fontFamily)
{
   wxFont font;

   // use the native font description if we have it
   if ( !fontDesc.empty() )
   {
      wxNativeFontInfo fontInfo;
      if ( fontInfo.FromString(fontDesc) )
      {
         font.SetNativeFontInfo(fontInfo);
      }
   }

   // if we don't, or if creating font from it failed, create the font with the
   // given size and family if we have [either of] them
   if ( !font.Ok() && (fontSize != -1 || fontFamily != wxFONTFAMILY_DEFAULT) )
   {
      font = wxFont
             (
               fontSize == -1 ? wxNORMAL_FONT->GetPointSize()
                              : fontSize,
               fontFamily == wxFONTFAMILY_DEFAULT ? wxNORMAL_FONT->GetFamily()
                                                  : fontFamily,
               wxFONTSTYLE_NORMAL,
               wxFONTWEIGHT_NORMAL
             );
   }

   return font;
}

