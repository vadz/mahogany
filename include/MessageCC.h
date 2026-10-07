///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MessageCC.h - declares MessageCC class
// Purpose:     implementation of Message using c-client API
// Author:      Karsten Ballüder (Ballueder@gmx.net)
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 M Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MESSAGECC_H
#define _MESSAGECC_H

#ifndef USE_PCH
#  include "Mcclient.h"         // for ADDRESS
#endif  //USE_PCH

#include "Message.h"

// fwd decl
class MailFolderCC;
class MimePartCC;
class HeaderInfo;

/** Message class, containing the most commonly used message headers.
   */
class MessageCC : public Message
{
public:
   // get specfied header lines
   wxArrayString GetHeaderLines(const char **headers,
                                wxArrayInt *encodings = nullptr) const override;

   String GetHeader(void) const override;

   /** @name Envelop headers */
   //@{
   /** get Subject line
       @return Subject entry
   */
   String Subject(void) const override;

   /** return the date of the message */
   time_t GetDate() const override;

   /** Return message id. */
   String GetId(void) const override ;

   /** Return message references. */
   String GetReferences(void) const override;

   String GetInReplyTo(void) const override;

   String GetNewsgroups() const override;
   //@}

   size_t GetAddresses(MessageAddressType type,
                       wxArrayString& addresses) const override;

   AddressList *GetAddressList(MessageAddressType type) const override;

   /** get From line
       @return From entry
   */
   String From(void) const override;

   /** get Date line
       @return Date when message was sent
   */
   String Date(void) const override;

   /** get message text
       @return the uninterpreted message body
   */
   String FetchText(void) const override;

   /** get the raw part text
    */
   const char *GetRawPartData(const MimePart& mimepart, unsigned long *len = nullptr);

   /**
      Get all headers of this message part.

      @return string containing all headers or an empty string on error
     */
   String GetPartHeaders(const MimePart& mimepart);

   const MimePart *GetTopMimePart() const override;

   /** return the number of body parts in message
       @return the number of body parts
   */
   int CountParts(void) const override;

   const MimePart *GetMimePart(int n) const override;

   /** Returns a pointer to the folder. If the caller needs that
       folder to stay around, it should IncRef() it. It's existence is
       guaranteed for as long as the message exists.
       @return folder pointer (not incref'ed)
   */
   MailFolder * GetFolder(void) const override;

   Profile *GetProfile() const override { return m_Profile; }

   /** Return the numeric status of message.
       @return flags of message
   */
   int GetStatus() const override;

   // get the size in bytes
   unsigned long GetSize() const override;

   /** Write the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
       @return FALSE on error
   */
   bool WriteToString(String &str, bool headerFlag = true) const override;

   /// Return the numeric uid
   UIdType GetUId(void) const override { return m_uid; }

   static MessageCC *Create(const char *text,
                            UIdType uid = UID_ILLEGAL,
                            Profile *profile = nullptr)
   {
      return new MessageCC(text, uid, profile);
   }

protected:
   /**@name Constructors and Destructors */
   //@{
   /** constructor, required associated folder reference
       @param folder where this mail is stored
       @param hi header info for this message
   */
   static MessageCC *Create(MailFolderCC *folder, const HeaderInfo& hi);

   /// The MailFolderCC class creates MessageCC objects.
   friend class MailFolderCC;

   /// constructors called by Create()
   MessageCC(MailFolderCC *folder, const HeaderInfo& hi);
   MessageCC(const char *text,
             UIdType uid = UID_ILLEGAL,
             Profile *profile = nullptr);

   /** destructor */
   ~MessageCC();

   //@}

   /// get the ADDRESS struct for the given address header
   ADDRESS *GetAddressStruct(MessageAddressType type) const;

   /// common part of GetRawPartData() and GetPartHeaders()
   const char *DoGetPartAny(const MimePart& mimepart,
                            unsigned long *lenptr,
                            char *(*fetchFunc)(MAILSTREAM *,
                                               unsigned long,
                                               char *,
                                               unsigned long *,
                                               long));

private:
   /// common part of all ctors
   void Init();

   /// Get the body and envelope information into member variables
   void GetBody(void);

   /// call GetBody() if necessary
   void CheckBody() const { if ( !m_Body ) ((MessageCC *)this)->GetBody(); }

   /// get the envelope information only (faster than GetBody!)
   void GetEnvelope();

   /// get the envelope information and return true if ok, false if we failed
   /// (this can happen, notably in case of network problems)
   bool CheckEnvelope() const
   {
      if ( m_Envelope )
         return true;

      const_cast<MessageCC*>(this)->GetEnvelope();
      return m_Envelope != nullptr;
   }

   /// get the cache element for this message
   MESSAGECACHE *GetCacheElement() const;

   /** @name MIME structure decoding

       c-cliet does everything for us in fact
    */
   //@{

   /// ensure that we have MIME structure info
   void CheckMIME() const;

   /// parse the MIME structure of the message and fill m_mimePartTop
   bool ParseMIMEStructure();

   /// GetMimePart() helper
   static MimePart *FindPartInMIMETree(MimePart *mimepart, int& n);

   //@}

   /// reference to the folder this mail is stored in
   MailFolderCC *m_folder;

   /// text of the mail if not linked to a folder
   char *m_msgText;

   /// unique message id
   UIdType m_uid;

   /// the parsed date value which we cache
   time_t m_date;

   /// holds the pointer to a text buffer allocated by cclient lib
   char *m_mailFullText;

   /// length of m_mailFullText
   unsigned long m_MailTextLen;

   /// body of message
   BODY *m_Body;

   /// m_Envelope for messages to be sent
   ENVELOPE *m_Envelope;

   /// Profile pointer, may be NULL
   Profile *m_Profile;

   /// pointer to the main message MIME part, it links to all others
   MimePartCC *m_mimePartTop;
};

#endif // _MESSAGECC_H
