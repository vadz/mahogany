//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/Message.cpp: implements some static Message methods
// Purpose:     Message is an ABC but it provides some utility functions which
//              we implement here, the rest is in MessageCC
// Author:      Karsten Ballüder
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 M-Team
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
   #include "Mcommon.h"
#endif // USE_PCH

#include "Message.h"

#include "Address.h"
#include "mail/MimeDecode.h"

// ============================================================================
// implementation of Message methods for working with addresses
// ============================================================================

String Message::GetAddressesString(MessageAddressType type) const
{
   String address;
   AddressList_obj addrList(GetAddressList(type));
   if ( addrList )
   {
      address = addrList->GetAddresses();
   }

   return address;
}

size_t Message::ExtractAddressesFromHeader(wxArrayString& addresses) const
{
   // first get all possible addresses
   GetAddresses(MAT_FROM, addresses);
   GetAddresses(MAT_SENDER, addresses);
   GetAddresses(MAT_REPLYTO, addresses);
   GetAddresses(MAT_RETURNPATH, addresses);
   GetAddresses(MAT_TO, addresses);
   GetAddresses(MAT_CC, addresses);
   GetAddresses(MAT_BCC, addresses);

   // now copy them to the output array filtering the copies
   return addresses.GetCount();
}

// TODO: write a function to extract addresses from the body as well

// ============================================================================
// implementation of Message methods for working with headers
// ============================================================================

// ----------------------------------------------------------------------------
// getting all the headers
// ----------------------------------------------------------------------------

size_t
Message::GetAllHeaders(wxArrayString *names, wxArrayString *values) const
{
   return GetHeaderIterator().GetAll(names, values);
}

// ----------------------------------------------------------------------------
// wrapper around GetHeaderLines
// ----------------------------------------------------------------------------

bool Message::GetHeaderLine(const String& line,
                            String& value,
                            wxFontEncoding *encoding) const
{
   const char *headers[2];
   headers[0] = line.c_str();
   headers[1] = NULL;

   wxArrayString values;
   if ( encoding )
   {
      wxArrayInt encodings;
      values = GetHeaderLines(headers, &encodings);

      *encoding = encodings.IsEmpty() ? wxFONTENCODING_SYSTEM
                                      : (wxFontEncoding)encodings[0];
   }
   else // don't need the encoding
   {
      values = GetHeaderLines(headers);
   }

   if ( values.IsEmpty() )
   {
      value.clear();
   }
   else
   {
      value = values[0];
   }

   return !value.empty();
}

bool Message::GetDecodedHeaderLine(const String& line, String& value) const
{
   if ( !GetHeaderLine(line, value) )
      return false;

   value = MIME::DecodeHeader(value);

   return true;
}

// ----------------------------------------------------------------------------
// Message creation
// ----------------------------------------------------------------------------

/*
  The following function is implemented in MessageCC.cpp:
  static class Message *Message::Create(const char * itext,
                       UIdType uid, Profile *iprofile)
*/

Message::~Message()
{
}

