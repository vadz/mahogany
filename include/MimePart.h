//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimePart.h: declaration of MimePart class
// Purpose:     MimePart represents a part of MIME message with the convention
//              that the main message is also considered as a part (it is not
//              according to MIME RFCs) - it is the root of the tree of
//              MimeParts and is returned by Message::GetMainMimePart()
// Author:      Vadim Zeitlin
// Modified by:
// Created:     28.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 by Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MIMEPART_H_
#define _MIMEPART_H_

// for wxFontEncoding enum
#include <wx/fontenc.h>

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

   /// copy
   MimeType(const MimeType& mt) { Assign(mt); }

   /// assignment operator (NB: no need to test for this != &mt)
   MimeType& operator=(const MimeType& mt) { return Assign(mt); }

   /// set to string
   MimeType& Assign(const String& mimetype);

   /// set to another MimeType
   MimeType& Assign(const MimeType& mt)
      { m_primary = mt.m_primary; m_subtype = mt.m_subtype; }
   //@}

   /** @name Accessors

       Get thee MIME type as number or string.
    */
   //@{

   /// is this MimeType initialized?
   bool IsOk() const { return m_primary != INVALID; }

   /// is this MimeType a wildcard?
   bool IsWildcard() const { return m_subtype == '*'; }

   /// get the numeric primary id
   String GetPrimary() const { return m_primary; }

   /// just the type ("MESSAGE")
   String GetType() const { return ms_types[m_primary]; }

   /// just the subtype ("RFC822")
   String GetSubType() const { return m_subtype; }

   /// get the full MIME string ("MESSAGE/RFC822")
   String GetFull() const { return GetType() + '/' + GetSubType(); }

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
   bool Matches(const wxMimeType& wildcard) const;

   //@}

private:
   /// the numeric MIME type
   Primary m_primary;

   /// the subtype string in upper case
   String m_subtype;

   /// the table for mapping m_primary to the string types
   static const char **ms_types;
};

// ----------------------------------------------------------------------------
// MimePart: represents a MIME message part of the main message
// ----------------------------------------------------------------------------

class MimePart
{
public:
   /** @name Headers access

       Get the information from "Content-Type" and "Content-Disposition"
       headers.
    */
   //@{

   /// get the MIME type of the part
   virtual MimeType GetType() const = 0;

   /// get the description of the part, if any
   virtual String GetDescription() const = 0;

   /// get the filename if the part is an attachment
   virtual String GetFilename() const = 0;

   /// get the value of the specified Content-Type parameter
   virtual String GetParam(const String& name) const = 0;

   /// get the value of the specified Content-Disposition parameter
   virtual String GetDispositionParam(const String& name) const = 0;

   /// get all parameters, return their count
   virtual size_t GetParameters(wxArrayString *names,
                                wxArrayString *values) const = 0;

   /// get all disposition parameters, return their count
   virtual size_t GetDispositionParameters(wxArrayString *names,
                                           wxArrayString *values) const = 0;

   //@}

   /** @name Data access

       Allows to get the part data. Note that it is always returned in the
       decoded form (why would we need it otherwise?) so its size in general
       won't be the same as the number returned by GetSize()
    */
   //@{

   /// get the decoded contents of this part
   virtual const char *GetContent(size_t *len = NULL) const = 0;

   /// get the encoding of the part
   virtual MimeXferEncoding GetTransferEncoding() const = 0;

   /// get the part size in bytes
   virtual size_t GetSize() const = 0;

   //@}

   /** @name Text part methods

       These methods can only be called for the part of type MimeType::TEXT or
       MESSAGE (if the message has a single text part)
    */
   //@{

   /// get the encoding of the text (only if GetType() == TEXT!)
   virtual wxFontEncoding GetTextEncoding() const = 0;

   /// get the size in lines
   virtual size_t GetNumberOfLines() const = 0;

   //@}
};

#endif // _MIMEPART_H_

