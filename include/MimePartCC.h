//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MimePartCC.h: declaration of MimePartCC class
// Purpose:     MimePartCC is an implementation of MimePart ABC using c-client
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 by Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MIMEPARTCC_H_
#define _MIMEPARTCC_H_

#ifdef __GNUG__
   #pragma interface "MimePartCC.h"
#endif

// base class
#include "MimePart.h"

class MessageCC;

// ----------------------------------------------------------------------------
// MimePartCC: represents a MIME message part of the main message
// ----------------------------------------------------------------------------

class MimePartCC : public MimePart
{
public:
   // MIME tree navigation
   virtual MimePart *GetParent() const;
   virtual MimePart *GetNext() const;
   virtual MimePart *GetNested() const;

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

   // data access
   virtual const void *GetRawContent(unsigned long *len = NULL) const;
   virtual const void *GetContent(unsigned long *len = NULL) const;
   virtual String GetTextContent() const;
   virtual String GetHeaders() const;
   virtual MimeXferEncoding GetTransferEncoding() const;
   virtual size_t GetSize() const;

   // text part additional info
   virtual wxFontEncoding GetTextEncoding() const;
   virtual size_t GetNumberOfLines() const;

protected:
   /// find the parameter in the list by name
   static String FindParam(const MimeParameterList& list, const String& name);

   /// fill our param list with values from c-client
   static void InitParamList(MimeParameterList *list,
                             struct mail_body_parameter *par);

   /// get the message we belong to
   MessageCC *GetMessage() const;

private:
   /** @name Ctors/dtor

       They are all private as only MessageCC creates and deletes us
    */
   //@{

   /// common part of all ctors
   void Init();

   /// ctor for the top level part
   MimePartCC(MessageCC *message);

   /// ctor for non top part
   MimePartCC(MimePartCC *parent, size_t nPart = 1);

   /// dtor deletes all subparts and siblings
   virtual ~MimePartCC();

   //@}

   /// the parent part (NULL for top level one)
   MimePartCC *m_parent;

   /// first child part for multipart or message parts
   MimePartCC *m_nested;

   // m_next and m_message could be in a union as only the top level message
   // has m_message pointer but it never has any siblings
   //
   // but is it worth it?

   /// next part in the message
   MimePartCC *m_next;

   /// the message we're part of (never NULL, not a ref for technical reasons)
   MessageCC *m_message;

   /// the c-client BODY struct we stand for
   struct mail_bodystruct *m_body;

   /// MIME/IMAP4 part spec (#.#.#.#)
   String m_spec;

   /// list of parameters if we already have them: never access directly!
   MimeParameterList *m_parameterList;

   /// list of disposition parameters if we already have them
   MimeParameterList *m_dispositionParameterList;

   friend class MessageCC;
};

#endif // _MIMEPARTCC_H_


