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

/**
   MimePartCCBase uses c-client structures for storing the message.
 */
class MimePartCCBase : public MimePart
{
public:
   /// dtor deletes all subparts and siblings
   virtual ~MimePartCCBase();

   // MIME tree navigation
   virtual MimePart *GetParent() const { return m_parent; }
   virtual MimePart *GetNext() const { return m_next; }
   virtual MimePart *GetNested() const { return m_nested; }

   // headers access
   virtual MimeType GetType() const;
   virtual String GetDescription() const;
   virtual String GetFilename() const;
   virtual String GetDisposition() const;
   virtual String GetPartSpec() const;
   virtual String GetParam(const String& name) const;
   virtual String GetDispositionParam(const String& name) const;
   virtual const MimeParameterList& GetParameters() const;
   virtual const MimeParameterList& GetDispositionParameters() const;
   virtual MimeXferEncoding GetTransferEncoding() const;
   virtual size_t GetSize() const;

   // text part additional info
   virtual wxFontEncoding GetTextEncoding() const;
   virtual size_t GetNumberOfLines() const;

   // data access
   virtual const void *GetContent(unsigned long *len = NULL) const;
   virtual String GetTextContent() const;


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

   /// number of MIME subparts in this part
   size_t m_numParts;

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
};

#endif // _M_MIMEPARTCCBASE_H_

