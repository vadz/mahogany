//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimePart.h: declaration of MimeType and MimePart classes
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

#include <wx/fontenc.h>         // for wxFontEncoding

// for MimeParameterList
#include "lists.h"

#include "MimeType.h"

// ----------------------------------------------------------------------------
// MimeParameter
// ----------------------------------------------------------------------------

class MimeParameter
{
public:
   MimeParameter(const String& name_, const String& value_)
      : name(name_), value(value_) { }

   /// the parameter name
   String name;

   /// the parameter value
   String value;
};

/// a linked list of parameters
M_LIST_OWN(MimeParameterList, MimeParameter);

// ----------------------------------------------------------------------------
// MimeXferEncoding: transfer encoding as defined by RFC 2045
// ----------------------------------------------------------------------------

/** This enum describes the transfer encodings

    NB: their values coincide with those of c-client, don't change!
 */
enum MimeXferEncoding
{
   /// 7 bit SMTP semantic data
   MIME_ENC_7BIT = 0,

   /// 8 bit SMTP semantic data
   MIME_ENC_8BIT = 1,

   /// 8 bit binary data
   MIME_ENC_BINARY = 2,

   /// base-64 encoded data
   MIME_ENC_BASE64 = 3,

   /// human-readable = 8-as-7 bit data
   MIME_ENC_QUOTEDPRINTABLE = 4,

   /// unknown
   MIME_ENC_OTHER = 5,

   /// maximum encoding code
   MIME_ENC_INVALID = 10
};

// ----------------------------------------------------------------------------
// MimePart: represents a MIME message part of the main message
// ----------------------------------------------------------------------------

class MimePart
{
public:
   /// Virtual dtor for the base class
   virtual ~MimePart() { }

   /** @name Navigation through the MIME tree

       MIME structure of the message is a tree where the nodes are parts of
       type MULTIPART/<*> or MESSAGE/RFC822 and the leaves parts of all other
       types.

       There is always the root in this tree corresponding to the main message
       itself (_unlike_ in IMAP RFC 2060!). For a non multipart message the
       tree is reduced to the single node which has the IMAP part spec "1".

       For a multipart message, the root has IMAP pseudo spec "0" but it
       shouldn't matter as this part is not used directly, only the subparts
       are.

       Example of the tree and the corresponding IMAP specs for a real-life
       messsage:

       MULTIPART/RELATED                              0
            MULTIPART/ALTERNATIVE                     1
                  TEXT/PLAIN                          1.1
                  TEXT/HTML                           1.2
            IMAGE/GIF                                 2
            IMAGE/JPEG                                3

       The results of calling GetXXX() for this tree:

       part    parent   next  nested
       -----------------------------
       0        NULL    NULL    1
       1         0       2      1.1
       1.1       1       1.2   NULL
       1.2       1      NULL   NULL
       2         0       3     NULL
       3         0      NULL   NULL
    */
   //@{

   /// get the parent MIME part (NULL for the top level one)
   virtual MimePart *GetParent() const = 0;

   /// get the next MIME part (NULL if this is the last part of multipart)
   virtual MimePart *GetNext() const = 0;

   /// get the first child part (NULL if not multipart or message)
   virtual MimePart *GetNested() const = 0;

   //@}

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

   /// get the disposition specified in Content-Disposition
   virtual String GetDisposition() const = 0;

   /// get the IMAP part spec (of the #.#.#.# form)
   virtual String GetPartSpec() const = 0;

   /// get the value of the specified Content-Type parameter
   virtual String GetParam(const String& name) const = 0;

   /// get the value of the specified Content-Disposition parameter
   virtual String GetDispositionParam(const String& name) const = 0;

   /// get the list of all parameters (from Content-Type)
   virtual const MimeParameterList& GetParameters() const = 0;

   /// get the list of all disposition parameters (from Content-Disposition)
   virtual const MimeParameterList& GetDispositionParameters() const = 0;

   //@}

   /** @name Data access

       Allows to get the part data. Note that it is always returned in the
       decoded form (why would we need it otherwise?) so its size in general
       won't be the same as the number returned by GetSize()
    */
   //@{

   /// get the raw (un-decoded) contents of this part
   virtual const void *GetRawContent(unsigned long *len = NULL) const = 0;

   /**
       get the decoded contents of this part

       WARNING: the pointer is not NULL terminated, len must be used!
    */
   virtual const void *GetContent(unsigned long *len) const = 0;

   /**
       get contents of this part as text.

       This function is more expensive than GetContent() as it copies the data
       one extra time just to append the terminating NUL to it, so it should be
       avoided if you don't need NUL-terminated data.
    */
   virtual String GetTextContent() const = 0;

   /// get all headers as one string
   virtual String GetHeaders() const = 0;

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

// ----------------------------------------------------------------------------
// for backwards compatibility only, don't use in new code
// ----------------------------------------------------------------------------

typedef MimeType::Primary MessageContentType;
typedef MimeParameter MessageParameter;
typedef MimeParameterList MessageParameterList;

#endif // _MIMEPART_H_

