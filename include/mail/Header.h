///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   mail/Header.h
// Purpose:     Helpers for working with email messages headers
// Author:      Vadim Zeitlin
// Created:     2015-03-04
// Copyright:   (c) 2015 Vadim Zeitlin <vz-mahogany@zeitlins.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_MAIL_HEADER_H_
#define M_MAIL_HEADER_H_

#include <wx/string.h>

/**
    Class representing an email message header name.
 */
class HeaderName
{
public:
   /**
       Constructor from wxString.
    */
   explicit HeaderName(const wxString& name)
      : m_name(name)
   {
      ASSERT( !name.empty() );
   }

   /**
       Compare this header name with the given string.
    */
   bool operator==(const char* name) const
   {
      return m_name.CmpNoCase(name) == 0;
   }

   bool operator!=(const char* name) const
   {
      return !(*this == name);
   }

   /**
       Checks if this is a valid header name conforming to RFC 2822.

       @return empty string if the name is valid, otherwise the string contains
          the user readable explanation of why it isn't.
    */
   wxString IsValid() const
   {
      for ( wxString::const_iterator p = m_name.begin(); p != m_name.end(); ++p )
      {
         // RFC 822 allows '/' but we don't because it has a special meaning for
         // the profiles/wxConfig; other characters are excluded in accordance
         // with the definition of the header name in the section 2.2 of RFC 2822
         wxChar c = *p;
         if ( c < 32 || c > 126 )
            return _("only ASCII characters are allowed");

         if ( c == ':' )
            return _("colons are not allowed");
      }

      return wxString();
   }

   /**
       Check if this header can be set by user.

       Some headers are always generated by the program and setting them
       explicitly can't be allowed as this would result in duplicate headers in
       the outgoing message and a lot of confusion.
    */
   bool CanBeSetByUser() const
   {
      return *this != "DATE" &&
             *this != "DELIVERED-TO" &&
             *this != "CONTENT-TYPE" &&
             *this != "CONTENT-DISPOSITION" &&
             *this != "CONTENT-TRANSFER-ENCODING" &&
             *this != "MESSAGE-ID" &&
             *this != "MIME-VERSION" &&
             *this != "RECEIVED" &&
             *this != "RETURN-PATH";
   }

private:
   const wxString m_name;
};

#endif // M_MAIL_HEADER_H_
