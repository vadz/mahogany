//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimeType.h: declaration of MimeType
// Purpose:     MimeType represents the full MIME type, i.e. type/subtype
// Author:      Vadim Zeitlin
// Modified by:
// Created:     02.09.02 (extracted from MimePart.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2002 by Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MIMETYPE_H_
#define _MIMETYPE_H_

// ----------------------------------------------------------------------------
// MimeType: represents a string of the form "type/subtype"
// ----------------------------------------------------------------------------

class MimeType
{
public:
   /** This enum describes the primary body types as defined in RFC 2046.

       NB: their values coincide with those of c-client, don't change!
    */
   enum Primary
   {
      /// unformatted text
      TEXT = 0,
      /// multipart content
      MULTIPART = 1,
      /// encapsulated message
      MESSAGE = 2,
      /// application data
      APPLICATION = 3,
      /// audio
      AUDIO = 4,
      /// static image
      IMAGE = 5,
      /// video
      VIDEO = 6,
      /// model
      MODEL = 7,
      /// unknown
      OTHER = 8,
      /// custom MIME type
      CUSTOM1 = 9,
      /// custom MIME type
      CUSTOM2 = 10,
      /// custom MIME type
      CUSTOM3 = 11,
      /// custom MIME type
      CUSTOM4 = 12,
      /// custom MIME type
      CUSTOM5 = 13,
      /// custom MIME type
      CUSTOM6 = 14,
      /// invalid type code
      INVALID = 15
   };

   /** @name Constructors
    */
   //@{

   /// default
   MimeType() { m_primary = INVALID; }

   /// from string
   MimeType(const String& mimetype) { Assign(mimetype); }

   /// from type and subtype
   MimeType(Primary primary, const String& subtype);

   /// copy
   MimeType(const MimeType& mt) { Assign(mt); }

   /// assignment operator (NB: no need to test for this != &mt)
   MimeType& operator=(const MimeType& mt) { return Assign(mt); }

   /// set to string
   MimeType& Assign(const String& mimetype);

   /// set to another MimeType
   MimeType& Assign(const MimeType& mt)
      { m_primary = mt.m_primary; m_subtype = mt.m_subtype; return *this; }
   //@}

   /** @name Accessors

       Get thee MIME type as number or string.
    */
   //@{

   /// is this MimeType initialized?
   bool IsOk() const { return m_primary != INVALID; }

   /// is this MimeType a wildcard?
   bool IsWildcard() const { return m_subtype == '*'; }

   /// does this type represent a text contents?
   bool IsText() const { return m_primary == TEXT || m_primary == MESSAGE; }

   /// get the numeric primary id
   Primary GetPrimary() const { return m_primary; }

   /// just the type ("MESSAGE")
   String GetType() const;

   /// just the subtype ("RFC822")
   String GetSubType() const { return m_subtype; }

   /// get the full MIME string ("MESSAGE/RFC822")
   String GetFull() const { return GetType() + _T('/') + GetSubType(); }

   /// return true if this is a type which it makes sense to show inline
   bool ShouldShowInline() const
   {
      return m_primary == TEXT || m_primary == IMAGE || m_primary == VIDEO;
   }

   //@}

   /** @name Comparison

       MIME supports wildcards for subtype: '*' matches any.
    */
   //@{

   /// are two MimeTypes exactly the same?
   bool operator==(const MimeType& mt) const
      { return m_primary == mt.m_primary && m_subtype == mt.m_subtype; }

   // different?
   bool operator!=(const MimeType& mt) const
      { return !(*this == mt); }

   /// is this MimeType the same as or specialization of the other?
   bool Matches(const MimeType& wildcard) const;

   /// this allows to write tests like mimeType == "TEXT/PLAIN"
   bool operator==(const String& wildcard) const { return Matches(wildcard); }

   //@}

private:
   /// the numeric MIME type
   Primary m_primary;

   /// the subtype string in upper case
   String m_subtype;
};

#endif // _MIMETYPE_H_

