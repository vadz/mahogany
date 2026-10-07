//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/VMessage.h: a Message from virtual mail folder
// Purpose:     MessageVirt represents a virtual message, i.e. it implements
//              Message base class interface by delegating it to the real
//              message
// Author:      Vadim Zeitlin
// Modified by:
// Created:     17.07.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAIL_VMESSAGE_H_
#define _MAIL_VMESSAGE_H_

#include "Message.h"

// ----------------------------------------------------------------------------
// MessageVirt
// ----------------------------------------------------------------------------

class MessageVirt : public Message
{
public:
   /**
      Create a new virtual message from a "real" message (which we take
      ownership of).

      NB: the flags are passed by pointer because they can be changed during
          our life time and we wouldn't know about it. Of course, because of
          this the flags pointer must have scope greater than the life time of
          this object.

      @param mf the virtual folder we live in
      @param uid our UID in that folder
      @param flags pointer to our flags
      @param message the real message we represent (we take ownership of it)
    */
   static MessageVirt *Create(MailFolder *mf,
                              UIdType uid,
                              int *flags,
                              Message *message);

   /** @name Headers access */
   //@{

   wxArrayString GetHeaderLines(const char **headers,
                                wxArrayInt *encodings = NULL) const override
      { return m_message->GetHeaderLines(headers, encodings); }

   String GetHeader() const override
      { return m_message->GetHeader(); }

   size_t GetAddresses(MessageAddressType type,
                       wxArrayString& addresses) const override
      { return m_message->GetAddresses(type, addresses); }

   AddressList *GetAddressList(MessageAddressType type) const override
      { return m_message->GetAddressList(type); }

   String Subject() const override { return m_message->Subject(); }
   String From() const override { return m_message->From(); }
   String Date() const override { return m_message->Date(); }
   String GetId() const override { return m_message->GetId(); }
   String GetReferences() const override { return m_message->GetReferences(); }
   String GetInReplyTo() const override { return m_message->GetInReplyTo(); }
   String GetNewsgroups() const override { return m_message->GetNewsgroups(); }
   int GetStatus() const override { return *m_flags; }
   unsigned long GetSize() const override { return m_message->GetSize(); }
   time_t GetDate() const override { return m_message->GetDate(); }

   //@}

   /** @name Simple accessors */
   //@{

   MailFolder *GetFolder() const override { return m_mf; }
   UIdType GetUId() const override { return m_uid; }
   Profile *GetProfile() const override { return m_message->GetProfile(); }

   //@}

   /** @name Body access */
   //@{

   const MimePart *GetTopMimePart() const override
      { return m_message->GetTopMimePart(); }

   int CountParts() const override
      { return m_message->CountParts(); }

   const MimePart *GetMimePart(int n) const override
      { return m_message->GetMimePart(n); }

   //@}

   /** @name Operations */
   //@{

   String FetchText() const override
      { return m_message->FetchText(); }

   bool WriteToString(String& str, bool headerFlag = true) const override
      { return m_message->WriteToString(str, headerFlag); }

   //@}

private:
   /**
      Our ctor is private because we're only created by Create().

      NB: the flags are passed by pointer because they can be changed during
          our life time and we wouldn't know about it. Of course, because of
          this the flags pointer must have scope greater than the life time of
          this object.

      @param mf the virtual folder we live in
      @param uid our UID in that folder
      @param flags pointer to our flags
      @param message the real message we represent (we take ownership of it)
    */
   MessageVirt(MailFolder *mf, UIdType uid, int *flags, Message *message);

   /**
      Dtor is private as well because we're never delted directly.
    */
   virtual ~MessageVirt();

   /// the folder we live in (we IncRef() it during our life time)
   MailFolder *m_mf;

   /// our uid in m_mf
   UIdType m_uid;

   /// our flags (pointer should never be NULL!)
   int *m_flags;

   /// the message we represent (should never be NULL!)
   Message *m_message;

   GCC_DTOR_WARN_OFF
};

#endif // _MAIL_VMESSAGE_H_

