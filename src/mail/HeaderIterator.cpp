///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/HeaderIterator.cpp - implements HeaderIterator class
// Purpose:     HeaderIterator allows to extract individual headers from the
//              full (multiline) message header
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.09.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
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
#endif // USE_PCH

#include "Message.h"

// ============================================================================
// HeaderIterator implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor &c
// ----------------------------------------------------------------------------

HeaderIterator::HeaderIterator(const String& header)
              : m_header(header)
{
   Reset();

   m_str.reserve(1024);
}

void HeaderIterator::Reset()
{
   m_pcCurrent = m_header;
}

// ----------------------------------------------------------------------------
// the worker functions
// ----------------------------------------------------------------------------

bool HeaderIterator::GetNext(String *name, String *value, int flags)
{
   CHECK( name, false, _T("NULL header name in HeaderIterator::GetNext()") );

   name->clear();

   // we are first looking for the name (before ':') and the value (after),
   // inName is true until we find the colon
   bool inName = true;

   // while we haven't exhausted the header
   for ( m_str.clear(); *m_pcCurrent; m_pcCurrent++ )
   {
      // look if the following char is special
      switch ( *m_pcCurrent )
      {
         case '\r':
            if ( m_pcCurrent[1] != '\n' )
            {
               // this is not supposed to happen in RFC822 headers!
               wxLogDebug(_T("Bare '\\r' in header ignored"));
               continue;
            }

            // skip '\n' too
            m_pcCurrent++;

            // look what have we got in the last which just ended
            if ( inName )
            {
               // we still haven't seen the colon
               if ( !m_str.empty() )
               {
                  // but have seen something -- this is not normal
                  wxLogDebug(_T("Header line '%s' is malformed; ignored."),
                             m_str.c_str());
               }
               else // no name neither
               {
                  // blank line, header must end here
                  //
                  // update: apparently, sometimes it doesn't... it's non
                  // fatal anyhow, but report it as this is weird
                  if ( m_pcCurrent[1] != '\0' )
                  {
                     wxLogDebug(_T("Blank line inside header?"));
                  }
               }
            }
            else // we have a valid header name in this line
            {
               if ( m_str.empty() )
               {
                  // suspicious...
                  wxLogDebug(_T("Empty header value?"));
               }

               // this header may continue on the next line if it begins
               // with a space or tab - check if it does
               m_pcCurrent++;
               if ( *m_pcCurrent != ' ' && *m_pcCurrent != '\t' )
               {
                  if ( value )
                     *value = m_str;

                  return true;
               }
               else // continued on the next line
               {
                  if ( flags & MultiLineOk )
                  {
                     m_str += _T('\n');
                     m_str += *m_pcCurrent;
                  }

                  // continue with the current header and ignore all leading
                  // whitespace on the next line
                  while ( m_pcCurrent[1] == ' ' || m_pcCurrent[1] == '\t' )
                  {
                     m_pcCurrent++;
                     if ( flags & MultiLineOk )
                        m_str += *m_pcCurrent;
                  }
               }
            }
            break;

         case ':':
            // is this the colon which terminates the header name?
            if ( inName )
            {
               *name = m_str;

               if ( *++m_pcCurrent != ' ' )
               {
                  // oops... skip back
                  m_pcCurrent--;

                  // although this is allowed by the RFC 822 (but not
                  // 822bis), it is quite uncommon and so may indicate a
                  // problem -- log it
                  wxLogDebug(_T("Header without space after colon?"));
               }

               m_str.clear();

               inName = false;

               break;
            }
            //else: fall through, this ':' is part of the header value

         default:
            m_str += *m_pcCurrent;
      }
   }

   if ( !inName )
   {
      // make values and names arrays always of the same size
      wxLogDebug(_T("Last header didn't have a valid value!"));

      if ( value )
         value->clear();
   }

   // have we found anything at all?
   return !name->empty();
}

size_t
HeaderIterator::GetAll(wxArrayString *names, wxArrayString *values, int flags)
{
   CHECK( names && values, 0, _T("NULL pointer in HeaderIterator::GetAll()") );

   String name, value;
   while ( GetNext(&name, &value, flags) )
   {
      int idxName = names->Index(name);
      if ( idxName == wxNOT_FOUND )
      {
         // a header we haven't seen yet
         names->Add(name);
         values->Add(value);
      }
      else // another occurence of a previously seen header
      {
         // append to the existing value
         (*values)[(size_t)idxName] << _T("\r\n") << value;
      }
   }

   return names->GetCount();
}

