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
#include "MimePartCCBase.h"

class MessageCC;

// ----------------------------------------------------------------------------
// MimePartCC: represents a MIME message part of the main message
// ----------------------------------------------------------------------------

class MimePartCC : public MimePartCCBase
{
public:
   // data access
   virtual const void *GetRawContent(unsigned long *len = NULL) const;
   virtual String GetHeaders() const;

protected:
   /// get the message we belong to
   MessageCC *GetMessage() const { return m_message; }

private:
   /** @name Ctors/dtor

       They are all private as only MessageCC creates and deletes us
    */
   //@{

   /// ctor for the top level part
   MimePartCC(MessageCC *message, struct mail_bodystruct *body);

   /// ctor for non top part
   MimePartCC(struct mail_bodystruct *body, MimePartCC *parent, size_t nPart);

   //@}

   // create all subparts of this one, called from both ctors
   void CreateSubParts();


   // the message we're part of (never NULL and never changes)
   MessageCC *m_message;


   // it creates us
   friend class MessageCC;
};

#endif // _MIMEPARTCC_H_


