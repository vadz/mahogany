///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimePartCCBase.h
// Purpose:     MimePart implementation using c-client data structures
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-13
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/**
   @file MimePartCCBase.h
   @brief MimePartCCBase is the common base to MimePartVirtual and MimePartCC.
 */

#ifndef _M_MIMEPARTCCBASE_H_
#define _M_MIMEPARTCCBASE_H_

#include "MimePart.h"

#include <wx/fontenc.h>

/**
   MimePartCCBase uses c-client structures for storing the message.
 */
class MimePartCCBase : public MimePart
{
public:
   /// dtor deletes all subparts and siblings
   virtual ~MimePartCCBase();

   // MIME tree navigation
   MimePart *GetParent() const override { return m_parent; }
   MimePart *GetNext() const override { return m_next; }
   MimePart *GetNested() const override { return m_nested; }

   // headers access
   MimeType GetType() const override;
   String GetDescription() const override;
   String GetFilename() const override;
   String GetDisposition() const override;
   String GetPartSpec() const override;
   String GetParam(const String& name) const override;
   String GetDispositionParam(const String& name) const override;
   const MimeParameterList& GetParameters() const override;
   const MimeParameterList& GetDispositionParameters() const override;
   MimeXferEncoding GetTransferEncoding() const override;
   size_t GetSize() const override;

   // text part additional info
   wxFontEncoding GetTextEncoding() const override;
   size_t GetNumberOfLines() const override;

   // data access
   const void *GetContent(unsigned long *len = NULL) const override;
   String GetTextContent() const override;


   // return the total number (recursively) of all our subparts
   size_t GetPartsCount() const;

protected:
   /// find the parameter in the list by name
   static String FindParam(const MimeParameterList& list, const String& name);

   /// fill our param list with values from c-client
   static void InitParamList(MimeParameterList *list,
                             struct mail_body_parameter *par);

   /**
      Initializes this part and all its nested subparts.

      @param body body structure of this MIME part
      @param parent the parent MIME part, if any
      @param nPart the order among our siblings
    */
   void Create(struct mail_bodystruct *body,
               MimePartCCBase *parent = NULL,
               size_t nPart = 1u);

   /// common part of all ctors
   void Init();

   /// default ctor, Create() must be called
   MimePartCCBase() { Init(); }

   /// full ctor, same argument as for Create()
   MimePartCCBase(struct mail_bodystruct *body,
                  MimePartCCBase *parent = NULL,
                  size_t nPart = 1u)
   {
      Init();

      Create(body, parent, nPart);
   }

   /// the meat of GetContent()
   const void *DecodeRawContent(const void *raw, unsigned long *lenptr);


   /// the parent part (NULL for top level one)
   MimePartCCBase *m_parent;

   /// first child part for multipart or message parts
   MimePartCCBase *m_nested;

   /// next part in the message
   MimePartCCBase *m_next;

   /// the c-client BODY struct we stand for
   struct mail_bodystruct *m_body;

   /// MIME/IMAP4 part spec (#.#.#.#)
   String m_spec;

   /// list of parameters if we already have them: never access directly!
   MimeParameterList *m_parameterList;

   /// list of disposition parameters if we already have them
   MimeParameterList *m_dispositionParameterList;

   /**
     A temporarily allocated buffer for GetContent().

     It holds the information returned by that function and is only
     valid until its next call.

     We should free it only if m_ownsContent flag is true!
   */
   void *m_content;

   /// length of m_content if it is not NULL
   size_t m_lenContent;

   /// Flag telling whether we should free m_content or not
   bool m_ownsContent;


   /// cached default encoding: only use GetTextEncoding() to access it
   mutable wxFontEncoding m_encoding;
};

#endif // _M_MIMEPARTCCBASE_H_

