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

#ifdef __GNUG__
   #pragma interface "MessageCC.h"
#endif

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
   virtual wxArrayString GetHeaderLines(const wxChar **headers,
                                        wxArrayInt *encodings = NULL) const;

   virtual String GetHeader(void) const;

   /** @name Envelop headers */
   //@{
   /** get Subject line
       @return Subject entry
   */
   virtual String Subject(void) const;

   /** return the date of the message */
   virtual time_t GetDate() const;

   /** Return message id. */
   virtual String GetId(void) const ;

   /** Return message references. */
   virtual String GetReferences(void) const;

   virtual String GetInReplyTo(void) const;

   virtual String GetNewsgroups() const;
   //@}

   virtual size_t GetAddresses(MessageAddressType type,
                               wxArrayString& addresses) const;

   virtual AddressList *GetAddressList(MessageAddressType type) const;

   /** get From line
       @return From entry
   */
   virtual String From(void) const;

   /** get Date line
       @return Date when message was sent
   */
   virtual String Date(void) const;

   /** get message text
       @return the uninterpreted message body
   */
   virtual String FetchText(void) const;

   /** get the raw part text
    */
   const char *GetRawPartData(const MimePart& mimepart, unsigned long *len = NULL);

   /** get the decoded part text
    */
   const void *GetPartData(const MimePart& mimepart, unsigned long *len = NULL);

   /**
      Get all headers of this message part.

      @return string containing all headers or an empty string on error
     */
   String GetPartHeaders(const MimePart& mimepart);

   virtual const MimePart *GetTopMimePart() const;

   /** return the number of body parts in message
       @return the number of body parts
   */
   virtual int CountParts(void) const;

   virtual const MimePart *GetMimePart(int n) const;

   /** Returns a pointer to the folder. If the caller needs that
       folder to stay around, it should IncRef() it. It's existence is
       guaranteed for as long as the message exists.
       @return folder pointer (not incref'ed)
   */
   virtual MailFolder * GetFolder(void) const { return m_folder; }

   /** Return the numeric status of message.
       @return flags of message
   */
   virtual int GetStatus() const;

   // get the size in bytes
   virtual unsigned long GetSize() const;

   /** Write the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
       @return FALSE on error
   */
   virtual bool WriteToString(String &str, bool headerFlag = true) const;


   /** Takes this message and tries to send it. Only useful for
       messages in some kind of Outbox folder.
       @param protocol how to send the message or Prot_Illegal to auto-detect
       @param send if true, send, otherwise send or queue depending on setting
       @return true on success
   */
   virtual bool SendOrQueue(Protocol protocol = Prot_Illegal,
                            bool send = FALSE);

   /// Return the numeric uid
   virtual UIdType GetUId(void) const { return m_uid; }

   static MessageCC *Create(const wxChar *text,
                            UIdType uid = UID_ILLEGAL,
                            Profile *profile = NULL)
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
   MessageCC(const wxChar *text,
             UIdType uid = UID_ILLEGAL,
             Profile *profile = NULL);

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

   /// get the envelope information only if necessary
   void CheckEnvelope() const
      { if ( !m_Envelope ) ((MessageCC *)this)->GetEnvelope(); }

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

   /// ParseMIMEStructure() helper
   void DecodeMIME(MimePartCC *mimepart, struct mail_bodystruct *body);

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

   /// total number of MIME parts in the message
   size_t m_numParts;

   /**
     A temporarily allocated buffer for GetPartContent().

     It holds the information returned by that function and is only
     valid until its next call.

     We should free it only if m_ownsPartContent flag is true!
   */
   void *m_partContentPtr;

   /// Flag telling whether we should free m_partContentPtr or not
   bool m_ownsPartContent;
};

#endif // _MESSAGECC_H
